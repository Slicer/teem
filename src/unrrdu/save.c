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
extern void _nrrdGuessFormat(NrrdIO *io, const char *filename);
extern int _nrrdWriteDataHex (Nrrd *nrrd, NrrdIO *io);

int
unrrduFormatPlusParse(void *ptr, char *str, char err[AIR_STRLEN_HUGE]) {
  char me[]="unrrduParsePos";
  int *pos;

  if (!(ptr && str)) {
    sprintf(err, "%s: got NULL pointer", me);
    return 1;
  }
  pos = ptr;
  airToLower(str);
  if (!strcmp("eps", str)) {
    /* invent some unused value */
    pos[0] = 2*nrrdFormatLast;
  } else {
    pos[0] = airEnumVal(nrrdFormat, str);
    if (nrrdFormatUnknown == pos[0]) {
      sprintf(err, "%s: couldn't parse \"%s\" as format", me, str);
      return 1;
    }
  }
  return 0;
}

hestCB unrrduFormatPlusCB = {
  sizeof(int),
  "format",
  unrrduFormatPlusParse,
  NULL
};

int
unrrduEpsSave(char *out, NrrdIO *io, Nrrd *nout) {
  char me[]="unrrduEpsSave", err[AIR_STRLEN_MED];
  int color, sx, sy, fit;

  fit = nrrdFitsInFormat(nout, nrrdEncodingAscii, nrrdFormatPNM, AIR_TRUE);
  if (!fit) {
    sprintf(err, "%s: can't save image into EPS", me);
    biffMove(UNRRDU, err, NRRD); return 1;
  }
  color = (3 == fit);
  
  if (!( nrrdTypeUChar == nout->type )) {
    sprintf(err, "%s: can only save type %s data to EPS", me,
	    airEnumStr(nrrdType, nrrdTypeUChar));
    biffAdd(UNRRDU, err); return 1;
  }
  if (2 == nout->dim) {
    sx = nout->axis[0].size;
    sy = nout->axis[1].size;
  } else {
    sx = nout->axis[1].size;
    sy = nout->axis[2].size;
  }

  if (!( io->dataFile = airFopen(out, stdout, "w") )) {
    sprintf(err, "%s: fopen(\"%s\", \"w\") failed: %s", me,
	    out, strerror(errno));
    biffAdd(UNRRDU, err); return 1;
  }

  fprintf(io->dataFile, "%%!PS-Adobe-2.0 EPSF-2.0\n");
  fprintf(io->dataFile, "%%%%Creator: unu\n");
  fprintf(io->dataFile, "%%%%Title: %s\n",
	  nout->content ? nout->content : NRRD_UNKNOWN);
  fprintf(io->dataFile, "%%%%Pages: 1\n");
  fprintf(io->dataFile, "%%%%BoundingBox: 0 0 %d %d\n", sx, sy);
  fprintf(io->dataFile, "%%%%EndComments\n");
  fprintf(io->dataFile, "%% linestr creates an empty string to hold one scanline\n");
  fprintf(io->dataFile, "/linestr %d string def\n", sx*(color ? 3 : 1));
  fprintf(io->dataFile, "%%%%EndProlog\n");
  fprintf(io->dataFile, "%%%%Page: 1 1\n");
  fprintf(io->dataFile, "gsave\n");
  fprintf(io->dataFile, "%d %d scale\n", sx, sy);
  fprintf(io->dataFile, "%d %d 8\n", sx, sy);
  fprintf(io->dataFile, "[%d 0 0 -%d 0 %d]\n", sx, sy, sy);
  fprintf(io->dataFile, "{currentfile linestr readhexstring pop} %s\n",
	  color ? "false 3 colorimage" : "image");
  _nrrdWriteDataHex(nout, io);
  fprintf(io->dataFile, "\n");
  fprintf(io->dataFile, "grestore\n");

  
  return 0;
}

#define INFO "Write nrrd with specific format, encoding, or endianness"
char *_unrrdu_saveInfoL =
(INFO
 ". Use \"unu\tsave\t-f\tpnm\t|\txv\t-\" to view PPM- or "
 "PGM-compatible nrrds on unix.  Support for the EPS format is limited "
 "to this unu command only.");

int
unrrdu_saveMain(int argc, char **argv, char *me, hestParm *hparm) {
  hestOpt *opt = NULL;
  char *out, *err, encInfo[AIR_STRLEN_HUGE], fmtInfo[AIR_STRLEN_HUGE];
  Nrrd *nin, *nout;
  airArray *mop;
  NrrdIO *io;
  int pret, enc[3];

  mop = airMopNew();
  io = nrrdIONew();
  airMopAdd(mop, io, (airMopper)nrrdIONix, airMopAlways);

  strcpy(fmtInfo,
	 "output file format. Possibilities include:\n "
	 "\b\bo \"nrrd\": standard nrrd format\n "
	 "\b\bo \"pnm\": PNM image; PPM for color, PGM for grayscale\n "
	 "\b\bo \"text\": plain ASCII text for 1-D and 2-D data\n "
	 "\b\bo \"vtk\": VTK \"STRUCTURED_POINTS\" dataset");
  if (nrrdFormatIsAvailable[nrrdFormatPNG]) {
    strcat(fmtInfo,
	   "\n \b\bo \"png\": PNG image");
  }
  strcat(fmtInfo,
	 "\n \b\bo \"eps\": EPS file");
  hestOptAdd(&opt, "f", "format", airTypeOther, 1, 1, &(io->format), NULL,
	     fmtInfo, NULL, NULL, &unrrduFormatPlusCB);
  strcpy(encInfo,
	 "encoding of data in file.  Not all encodings are supported in "
	 "a given format. Possibilities include:"
	 "\n \b\bo \"raw\": raw encoding"
	 "\n \b\bo \"ascii\": print data in ascii"
	 "\n \b\bo \"hex\": two hex digits per byte");
  if (nrrdEncodingIsAvailable[nrrdEncodingGzip]) {
    strcat(encInfo, 
	   "\n \b\bo \"gzip\", \"gz\": gzip compressed raw data");
  }
  if (nrrdEncodingIsAvailable[nrrdEncodingBzip2]) {
    strcat(encInfo, 
	   "\n \b\bo \"bzip2\", \"bz2\": bzip2 compressed raw data");
  }
  if (nrrdEncodingIsAvailable[nrrdEncodingGzip]
      || nrrdEncodingIsAvailable[nrrdEncodingBzip2]) {
    strcat(encInfo,
	   "\n The specifiers for compressions may be followed by a colon "
	   "\":\", followed by an optional digit giving compression \"level\" "
	   "(for gzip) or \"block size\" (for bzip2).  For gzip, this can be "
	   "followed by an optional character for a compression strategy:\n "
	   "\b\bo \"d\": default, Huffman with string match\n "
	   "\b\bo \"h\": Huffman alone\n "
	   "\b\bo \"f\": specialized for filtered data\n "
	   "For example, \"gz\", \"gz:9\", \"gz:9f\" are all valid");
  }
  hestOptAdd(&opt, "e", "encoding", airTypeOther, 1, 1, enc, "raw",
	     encInfo, NULL, NULL, &unrrduHestEncodingCB);
  hestOptAdd(&opt, "en", "endian", airTypeEnum, 1, 1, &(io->endian),
	     airEnumStr(airEndian, airMyEndian),
	     "Endianness to save data out as; \"little\" for Intel and "
	     "friends; \"big\" for everyone else. "
	     "Defaults to endianness of this machine",
	     NULL, airEndian);
  OPT_ADD_NIN(nin, "input nrrd");
  OPT_ADD_NOUT(out, "output nrrd");

  airMopAdd(mop, opt, (airMopper)hestOptFree, airMopAlways);

  USAGE(_unrrdu_saveInfoL);
  PARSE();
  airMopAdd(mop, opt, (airMopper)hestParseFree, airMopAlways);
  nout = nrrdNew();
  airMopAdd(mop, nout, (airMopper)nrrdNuke, airMopAlways);

  if (2*nrrdFormatLast == io->format) {
    if (unrrduEpsSave(out, io, nin)) {
      airMopAdd(mop, err = biffGetDone(UNRRDU), airFree, airMopAlways);
      fprintf(stderr, "%s: error saving nrrd to \"%s\":\n%s\n", me, out, err);
      airMopError(mop);
      return 1;
    }
    airMopOkay(mop);
    return 0;
  }

  nrrdCopy(nout, nin);
  
  io->encoding = enc[0];
  if (nrrdEncodingGzip == enc[0]) {
    io->zlibLevel = enc[1];
    io->zlibStrategy = enc[2];
  } else if (nrrdEncodingBzip2 == enc[0]) {
    io->bzip2BlockSize = enc[1];
  }
  if (AIR_ENDIAN != io->endian) {
    nrrdSwapEndian(nout);
  }
  if (airEndsWith(out, NRRD_EXT_HEADER)) {
    if (io->format != nrrdFormatNRRD) {
      fprintf(stderr, "%s: WARNING: will use %s format\n", me,
	      airEnumStr(nrrdFormat, nrrdFormatNRRD));
      io->format = nrrdFormatNRRD;
    }
    nrrdDirBaseSet(io, out);
    /* we know exactly what part of this function will run (since we
       know airEndsWith()), so we could copy the code, but let's not */
    _nrrdGuessFormat(io, out);
  }

  SAVE(out, nout, io);

  airMopOkay(mop);
  return 0;
}

UNRRDU_CMD(save, INFO);

