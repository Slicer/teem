/*
  teem: Gordon Kindlmann's research software
  Copyright (C) 2002, 2001, 2000, 1999, 1998 University of Utah

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "unrrdu.h"
#include "privateUnrrdu.h"

/* bad bad bad Gordon */
extern int _nrrdWriteNrrd(FILE *file, Nrrd *nrrd, NrrdIO *io, int writeData);


#define INFO "Create a nrrd (or nrrd header) from scratch"
char *_unrrdu_makeInfoL =
(INFO
 ".  The data can be in one or more files, or coming from stdin. "
 "This provides an easy way of providing the bare minimum "
 "information about some data so as to wrap it in a "
 "nrrd, either to pass on for further unu processing, "
 "or to save to disk.  However, with \"-h\", this creates "
 "only a detached nrrd header file, without ever reading "
 "or writing data. When reading multiple files, nearly all the options below "
 "refer the finished nrrd resulting from getting all the data from all "
 "the files.  The only exceptions are \"-ls\" and \"-bs\", which apply "
 "to every input file. ");

int
unrrdu_makeMain(int argc, char **argv, char *me, hestParm *hparm) {
  hestOpt *opt = NULL;
  char *out, *err, **dataFileNames, *content, encInfo[AIR_STRLEN_LARGE];
  Nrrd *nrrd;
  int *size, nameLen, sizeLen, spacingLen, labelLen, headerOnly, pret;
  double *spacing;
  airArray *mop;
  NrrdIO *io;
  FILE *fileOut;
  char **label;

  mop = airMopNew();
  io = nrrdIONew();
  airMopAdd(mop, io, (airMopper)nrrdIONix, airMopAlways);
  nrrd = nrrdNew();
  airMopAdd(mop, nrrd, (airMopper)nrrdNuke, airMopAlways);
  
  hestOptAdd(&opt, "h", NULL, airTypeBool, 0, 0, &headerOnly, NULL,
	     "Generate header ONLY: don't write out the whole nrrd, "
	     "don't even bother reading the input data, just output the "
	     "detached nrrd header file (usually with a \".nhdr\" "
	     "extension) determined by the options below. "
	     "The filename given with \"-i\" should probably start "
	     "with \"./\" to indicate that the "
	     "data file is to be found relative to the header file "
	     "(as opposed to the current working directory of whomever "
	     "is reading the nrrd)");
  hestOptAdd(&opt, "i", "file", airTypeString, 1, -1, &dataFileNames, NULL,
	     "Filename(s) of data file(s); use \"-\" for stdin. "
	     "Detached headers (using \"-h\") are incompatible with "
	     "using stdin as the data source, or using multiple data "
	     "files", &nameLen);
  hestOptAdd(&opt, "t", "type", airTypeEnum, 1, 1, &(nrrd->type), NULL,
	     "type of data (e.g. \"uchar\", \"int\", \"float\", "
	     "\"double\", etc.)",
	     NULL, nrrdType);
  hestOptAdd(&opt, "s", "sz0 sz1", airTypeInt, 1, -1, &size, NULL,
	     "number of samples along each axis (and implicit indicator "
	     "of dimension of nrrd)", &sizeLen);
  hestOptAdd(&opt, "sp", "spc0 spc1", airTypeDouble, 1, -1, &spacing, "nan",
	     "spacing between samples on each axis.  Use \"nan\" for "
	     "any non-spatial axes (e.g. spacing between red, green, and blue "
	     "along axis 0 of interleaved RGB image data)", &spacingLen);
  hestOptAdd(&opt, "l", "lab0 lab1", airTypeString, 1, -1, &label, "",
	     "short string labels for each of the axes", &labelLen);
  hestOptAdd(&opt, "c", "content", airTypeString, 1, 1, &content, "",
	     "Specifies the content string of the nrrd, which is built upon "
	     "by many nrrd function to record a history of operations");
  hestOptAdd(&opt, "ls", "lineskip", airTypeInt, 1, 1, &(io->lineSkip), "0",
	     "number of ascii lines to skip before reading data");
  hestOptAdd(&opt, "bs", "byteskip", airTypeInt, 1, 1, &(io->byteSkip), "0",
	     "number of bytes to skip (after skipping ascii lines, if any) "
	     "before reading data.  Can use \"-bs -1\" to skip a binary "
	     "header of unknown length in raw-encoded data");
  strcpy(encInfo,
	 "output file format. Possibilities include:"
	 "\n \b\bo \"raw\": raw encoding"
	 "\n \b\bo \"ascii\": ascii values, one scanline per line of text, "
	 "values within line are delimited by space, tab, or comma"
	 "\n \b\bo \"hex\": two hex digits per byte");
  if (nrrdEncodingIsAvailable[nrrdEncodingGzip]) {
    strcat(encInfo, 
	   "\n \b\bo \"gzip\", \"gz\": gzip compressed raw data");
  }
  if (nrrdEncodingIsAvailable[nrrdEncodingBzip2]) {
    strcat(encInfo, 
	   "\n \b\bo \"bzip2\", \"bz2\": bzip2 compressed raw data");
  }
  hestOptAdd(&opt, "e", "encoding", airTypeEnum, 1, 1, &(io->encoding), "raw",
	     encInfo, NULL, nrrdEncoding);
  hestOptAdd(&opt, "en", "endian", airTypeEnum, 1, 1, &(io->endian),
	     airEnumStr(airEndian, airMyEndian),
	     "Endianness of data; relevent for any data with value "
	     "representation bigger than 8 bits, with a non-ascii encoding: "
	     "\"little\" for Intel and "
	     "friends; \"big\" for everyone else. "
	     "Defaults to endianness of this machine",
	     NULL, airEndian);
  OPT_ADD_NOUT(out, "output filename");
  airMopAdd(mop, opt, (airMopper)hestOptFree, airMopAlways);

  USAGE(_unrrdu_makeInfoL);
  PARSE();
  airMopAdd(mop, opt, (airMopper)hestParseFree, airMopAlways);

  /* given the information we have, we set the fields in the nrrdIO
     so as to simulate having read the information from a header */
  if (!( AIR_IN_CL(1, sizeLen, NRRD_DIM_MAX) )) {
    fprintf(stderr, "%s: # axis sizes (%d) not in valid nrrd dimension "
	    "range ([1,%d])\n", me, sizeLen, NRRD_DIM_MAX);
    airMopError(mop);
    return 1;
  }
  if (AIR_EXISTS(spacing[0]) && sizeLen != spacingLen) {
    fprintf(stderr,
	    "%s: got different numbers of sizes (%d) and spacings (%d)\n",
	    me, sizeLen, spacingLen);
    airMopError(mop);
    return 1;
  }
  if (airStrlen(label[0]) && sizeLen != labelLen) {
    fprintf(stderr,
	    "%s: got different numbers of sizes (%d) and labels (%d)\n",
	    me, sizeLen, labelLen);
    airMopError(mop);
    return 1;
  }
  nrrd->dim = sizeLen;
  nrrdAxesSet_nva(nrrd, nrrdAxesInfoSize, size);
  if (AIR_EXISTS(spacing[0])) {
    nrrdAxesSet_nva(nrrd, nrrdAxesInfoSpacing, spacing);
  }
  if (airStrlen(label[0])) {
    nrrdAxesSet_nva(nrrd, nrrdAxesInfoLabel, label);
  }
  if (airStrlen(content)) {
    nrrd->content = airStrdup(content);
  }
  
  if (headerOnly) {
    /* we don't have to fopen() any input; all we care about
       is the name of the input datafile.  We disallow stdin here */
    if (!strcmp("-", dataFileName)) {
      fprintf(stderr, "%s: can't use detached headers with stdin as "
	      "data source\n", me);
      airMopError(mop); return 1;
    }
    if (1 != nameLen) {
      fprintf(stderr, "%s: can't use detached headers with multiple "
	      "data files\n", me);
      airMopError(mop); return 1;
    }
    io->dataFN = airStrdup(dataFileName);
    io->seperateHeader = AIR_TRUE;
    /* we open and hand off the output FILE* to _nrrdWriteNrrd,
       which will not write any data (because of the AIR_FALSE) */
    if (!strcmp("-", out)) {
      fileOut = stdout;
#ifdef WIN32
    _setmode(_fileno(fileOut), _O_BINARY);
#endif
    } else {
      if (!( fileOut = fopen(out, "wb") )) {
	fprintf(stderr, "%s: couldn't fopen(\"%s\",\"wb\"): %s\n", 
		me, out, strerror(errno));
	airMopError(mop); return 1;
      }
      airMopAdd(mop, fileOut, (airMopper)airFclose, airMopAlways);
    }
    /* whatever line and byte skipping is required will be simply
       recorded in the header, and done by the next reader */
    _nrrdWriteNrrd(fileOut, nrrd, io, AIR_FALSE /* don't write data */);
  } else {
    /* we're not actually using the handy unrrduHestFileCB,
       since we have to open the input data file by hand */
    if (!strcmp("-", dataFileName)) {
      io->dataFile  = stdin;
#ifdef WIN32
      _setmode(_fileno(io->dataFile), _O_BINARY);
#endif

    } else {
      if (!( io->dataFile = fopen(dataFileName, "rb") )) {
	fprintf(stderr, "%s:  couldn't fopen(\"%s\",\"rb\"): %s\n", 
		me, dataFileName, strerror(errno));
	airMopError(mop); return 1;
      }
      airMopAdd(mop, io->dataFile, (airMopper)airFclose, airMopAlways);
    }
    if (nrrdLineSkip(io)) {
      sprintf(err, "%s: couldn't skip lines", me);
      biffAdd(NRRD, err); return 1;
    }
    if (!nrrdEncodingIsCompression[io->encoding]) {
      if (nrrdByteSkip(nrrd, io)) {
	sprintf(err, "%s: couldn't skip bytes", me);
	biffAdd(NRRD, err); return 1;
      }
    }
    if (nrrdReadData[io->encoding](nrrd, io)) {
      airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
      fprintf(stderr, "%s: error reading data:\n%s", me, err);
      airMopError(mop);
      return 1;
    }
    if (1 < nrrdElementSize(nrrd)
	&& nrrdEncodingEndianMatters[io->encoding]
	&& io->endian != AIR_ENDIAN) {
      /* endianness exposed in encoding, and its wrong */
      nrrdSwapEndian(nrrd);
    }
    /* we are saving normally- no need to subvert nrrdSave() here;
       we just pass it the output filename */
    SAVE(out, nrrd, NULL);
  }

  airMopOkay(mop);
  return 0;
}

UNRRDU_CMD(make, INFO);
