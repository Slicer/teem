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
 ".  The data can be in a file or coming from stdin. "
 "This provides an easy way of providing the bare minimum "
 "information about some data so as to wrap it in a "
 "nrrd, either to pass on for further unu processing, "
 "or to save to disk.  However, with \"-h\", this creates "
 "only a detached nrrd header file, without ever reading "
 "or writing data. ");

int
unrrdu_makeMain(int argc, char **argv, char *me, hestParm *hparm) {
  hestOpt *opt = NULL;
  char *out, *err, *dataFileName, *content;
  Nrrd *nrrd;
  int *size, sizeLen, spacingLen, headerOnly;
  double *spacing;
  airArray *mop;
  NrrdIO *io;
  FILE *fileOut;

  mop = airMopInit();
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
  hestOptAdd(&opt, "i", "file", airTypeString, 1, 1, &dataFileName, NULL,
	     "Filename of data file; use \"-\" for stdin");
  hestOptAdd(&opt, "t", "type", airTypeEnum, 1, 1, &(nrrd->type), NULL,
	     "type of data (e.g. \"uchar\", \"int\", \"float\", "
	     "\"double\", etc.)",
	     NULL, nrrdType);
  hestOptAdd(&opt, "s", "sz0 sz1", airTypeInt, 1, -1, &size, NULL,
	     "number of samples along each axis (and implicit indicator "
	     "of dimension of nrrd)", &sizeLen);
  hestOptAdd(&opt, "sp", "spc0 spc1", airTypeDouble, 1, -1, &spacing, NULL,
	     "spacing between samples on each axis.  Use \"nan\" for "
	     "any non-spatial axes (e.g. spacing between red, green, and blue "
	     "along axis 0 of interleaved RGB image data)", &spacingLen);
  hestOptAdd(&opt, "c", "content", airTypeString, 1, 1, &content, "",
	     "Specifies the content string of the nrrd, which is built upon "
	     "by many nrrd function to record a history of operations");
  hestOptAdd(&opt, "ls", "lineskip", airTypeInt, 1, 1, &(io->lineSkip), "0",
	     "number of ascii lines to skip before reading data");
  hestOptAdd(&opt, "bs", "byteskip", airTypeInt, 1, 1, &(io->byteSkip), "0",
	     "number of bytes to skip (after skipping ascii lines, if any) "
	     "before reading data");
  hestOptAdd(&opt, "e", "encoding", airTypeEnum, 1, 1, &(io->encoding), "raw",
	     "data encoding. Possibilities are \"raw\" and \"ascii\"",
	     NULL, nrrdEncoding);
  hestOptAdd(&opt, "en", "endian", airTypeEnum, 1, 1, &(io->endian),
	     airEnumStr(airEndian, airMyEndian),
	     "Endianness of data; relevent for any raw data with value "
	     "representation bigger than 8 bits: \"little\" for Intel and "
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
  if (!( AIR_INSIDE(1, sizeLen, NRRD_DIM_MAX) )) {
    fprintf(stderr, "%s: # axis sizes (%d) not in valid nrrd dimension "
	    "range ([1,%d])\n", me, sizeLen, NRRD_DIM_MAX);
    airMopError(mop);
    return 1;
  }
  if (sizeLen != spacingLen) {
    fprintf(stderr,
	    "%s: got different numbers of sizes (%d) and spacings (%d)\n",
	    me, sizeLen, spacingLen);
    airMopError(mop);
    return 1;
  }
  nrrd->dim = sizeLen;
  nrrdAxesSet_nva(nrrd, nrrdAxesInfoSize, size);
  nrrdAxesSet_nva(nrrd, nrrdAxesInfoSpacing, spacing);
  if (airStrlen(content)) {
    nrrd->content = airStrdup(content);
  }
  
  if (headerOnly) {
    /* we don't have to fopen() any input; all we care about
       is the name of the input datafile.  We disallow stdin here */
    if (!strcmp("-", dataFileName)) {
      fprintf(stderr, "%s: can't store stdin as data file in header\n", me);
      airMopError(mop); return 1;
    }
    io->dataFN = airStrdup(dataFileName);
    io->seperateHeader = AIR_TRUE;
    /* we open and hand off the output FILE* to _nrrdWriteNrrd,
       which not write any data (because of the AIR_FALSE) */
    if (!strcmp("-", out)) {
      fileOut = stdout;
    } else {
      if (!( fileOut = fopen(out, "wb") )) {
	fprintf(stderr, "%s: couldn't fopen(\"%s\",\"wb\"): %s\n", 
		me, out, strerror(errno));
	airMopError(mop); return 1;
      }
      airMopAdd(mop, fileOut, (airMopper)airFclose, airMopAlways);
    }
    _nrrdWriteNrrd(fileOut, nrrd, io, AIR_FALSE /* don't write data */);
  } else {
    /* we're not actually using the handy unrrduHestFileCB,
       since we have to open the input data file by hand */
    if (!strcmp("-", dataFileName)) {
      io->dataFile  = stdin;
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
    if (nrrdByteSkip(io)) {
      sprintf(err, "%s: couldn't skip bytes", me);
      biffAdd(NRRD, err); return 1;
    }
    if (nrrdReadData[io->encoding](nrrd, io)) {
      airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
      fprintf(stderr, "%s: error reading data:\n%s", me, err);
      airMopError(mop);
      return 1;
    }
    /* we are saving normally- no need to subvert nrrdSave() here;
       we just pass it the output filename */
    SAVE(out, nrrd, NULL);
  }

  airMopOkay(mop);
  return 0;
}

UNRRDU_CMD(make, INFO);
