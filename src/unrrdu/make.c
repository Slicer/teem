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

#include "private.h"

void *
unuMaybeFclose(void *_file) {
  FILE *file;
  
  file = _file;
  if (stdin != file) {
    return airFclose(file);
  }
  return NULL;
}

int
unuParseFile(void *ptr, char *str, char err[AIR_STRLEN_HUGE]) {
  char me[]="unuParseFile";
  FILE **fileP;

  if (!(ptr && str)) {
    sprintf(err, "%s: got NULL pointer", me);
    return 1;
  }
  fileP = ptr;
  if (!strcmp("-", str)) {
    *fileP = stdin;
  }
  else {
    *fileP = fopen(str, "rb");
    if (!*fileP) {
      sprintf(err, "%s: fopen(\"%s\",\"rb\") failed: %s",
	      me, str, strerror(errno));
      return 1;
    }
  }
  return 0;
}

hestCB unuFileHestCB = {
  sizeof(FILE *),
  "filename",
  unuParseFile,
  unuMaybeFclose,
};

char *makeName = "make";
#define INFO "Create a nrrd from scratch, starting with raw or ascii data"
char *makeInfo = INFO;
char *makeInfoL = (INFO
		   ". The data can be in a file or coming from stdin. "
		   "This provides an easy way of providing the bare minimum "
		   "information about some data so as to wrap it in a "
		   "nrrd, either to pass on for further unu processing, "
		   "or to save to disk.");

int
makeMain(int argc, char **argv, char *me) {
  hestOpt *opt = NULL;
  char *out, *err;
  Nrrd *nrrd;
  int *size, sizeLen, spacingLen;
  double *spacing;
  airArray *mop;
  NrrdIO *io;

  mop = airMopInit();
  io = nrrdIONew();
  airMopAdd(mop, io, (airMopper)nrrdIONix, airMopAlways);
  nrrd = nrrdNew();
  airMopAdd(mop, nrrd, (airMopper)nrrdNuke, airMopAlways);

  hestOptAdd(&opt, "i", "file", airTypeOther, 1, 1, &(io->dataFile), "-",
	     "File to read data from; use \"-\" for stdin",
	     NULL, NULL, &unuFileHestCB);
  hestOptAdd(&opt, "t,type", "type", airTypeEnum, 1, 1, &(nrrd->type),
	     NULL, "type of data (e.g. \"uchar\", \"int\", etc)",
	     NULL, nrrdType);
  hestOptAdd(&opt, "s,size", "size0 size1", airTypeInt, 1, -1, &size, NULL,
	     "number of samples along each axis (and implicit indicator "
	     "of dimension of nrrd)", &sizeLen);
  hestOptAdd(&opt, "sp,spacing", "spc0 spc1", airTypeDouble, 1, -1, &spacing,
	     NULL, "spacing between samples on each axis", &spacingLen);
  hestOptAdd(&opt, "ls,lineskip", "skip", airTypeInt, 1, 1,
	     &(io->lineSkip), "0",
	     "number of ascii lines to skip before reading data");
  hestOptAdd(&opt, "bs,byteskip", "skip", airTypeInt, 1, 1,
	     &(io->byteSkip), "0",
	     "number of bytes to skip (after skipping ascii lines, if any) "
	     "before reading data");
  hestOptAdd(&opt, "e,encoding", "encoding", airTypeEnum, 1, 1,
	     &(io->encoding), "raw",
	     "data encoding. Possibilities are \"raw\" and \"ascii\".",
	     NULL, nrrdEncoding);
  hestOptAdd(&opt, "en,endian", "endian", airTypeEnum, 1, 1, &(io->endian),
	     airEnumStr(airEndian, airMyEndian),
	     "Endianness of data; relevent for any raw data with value "
	     "representation bigger than 8 bits: \"little\" for Intel and "
	     "friends; \"big\" for everyone else.\n "
	     "Defaults to endianness of this machine.",
	     NULL, airEndian);
  OPT_ADD_NOUT(out, "output nrrd");
  airMopAdd(mop, opt, (airMopper)hestOptFree, airMopAlways);

  fprintf(stderr, "%s: argc = %d\n", me, argc);
  USAGE(makeInfoL);
  PARSE();
  airMopAdd(mop, opt, (airMopper)hestParseFree, airMopAlways);

  /* given the information we have, we set the fields in the nrrdIO
     so as to simulate having read the information from a header */
  if (!( AIR_INSIDE(1, sizeLen, NRRD_DIM_MAX) )) {
    fprintf(stderr, "%s: # axis sizes (%d) not in valid nrrd dimension "
	    "range ([1,%d])", me, sizeLen, NRRD_DIM_MAX);
    airMopError(mop);
    return 1;
  }
  if (sizeLen != spacingLen) {
    fprintf(stderr,
	    "%s: got different numbers of sizes (%d) and spacings (%d)",
	    me, sizeLen, spacingLen);
  }
  nrrd->dim = sizeLen;
  nrrdAxesSet_nva(nrrd, nrrdAxesInfoSize, size);
  nrrdAxesSet_nva(nrrd, nrrdAxesInfoSpacing, spacing);
  
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

  SAVE(nrrd, NULL);

  airMopOkay(mop);
  return 0;
}
