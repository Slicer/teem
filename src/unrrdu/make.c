/*
  The contents of this file are subject to the University of Utah Public
  License (the "License"); you may not use this file except in
  compliance with the License.
  
  Software distributed under the License is distributed on an "AS IS"
  basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See
  the License for the specific language governing rights and limitations
  under the License.

  The Original Source Code is "teem", released March 23, 2001.
  
  The Original Source Code was developed by the University of Utah.
  Portions created by UNIVERSITY are Copyright (C) 2001, 1998 University
  of Utah. All Rights Reserved.
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

int
unuParseEndian(void *ptr, char *str, char err[AIR_STRLEN_HUGE]) {
  char me[]="unuParseEndian";
  int *endianP;

  if (!(ptr && str)) {
    sprintf(err, "%s: got NULL pointer", me);
    return 1;
  }
  endianP = ptr;
  *endianP = nrrdEnumStrToVal(nrrdEnumEndian, str);
  if (airEndianUnknown == *endianP) {
    sprintf(err, "%s: \"%s\" is not a recognized endian", me, str);
    return 1;
  }
  return 0;
}

hestCB unuEndianHestCB = {
  sizeof(int),
  "endian",
  unuParseEndian,
  NULL
};

char *makeName = "make";
#define INFO "Create a nrrd from scratch, starting with raw data"
char *makeInfo = INFO;
char *makeInfoL = (INFO
		   ". This provides a way of providing the bare minimum "
		   "information about raw data so as to wrap it in a "
		   "nrrd. ");

int
makeMain(int argc, char **argv, char *me) {
  hestOpt *opt = NULL;
  char *out, *err;
  Nrrd *nrrd;
  int *size, sizeLen;
  airArray *mop;
  nrrdIO *io;

  mop = airMopInit();
  io = nrrdIONew();
  airMopAdd(mop, io, (airMopper)nrrdIONix, airMopAlways);
  nrrd = nrrdNew();
  airMopAdd(mop, nrrd, (airMopper)nrrdNuke, airMopAlways);

  hestOptAdd(&opt, "i", "file", airTypeOther, 1, 1, &(io->dataFile), "-",
	     "File to read data from; use \"-\" for stdin",
	     NULL, &unuFileHestCB);
  OPT_ADD_TYPE(nrrd->type, "type of data");
  hestOptAdd(&opt, "s", "size0 size1 ", airTypeInt, 1, -1, &size, NULL,
	     "number of samples along each axis (and implicit indicator "
	     "of dimension of nrrd)", &sizeLen);
  hestOptAdd(&opt, "e", "encoding", airTypeOther, 1, 1, &(io->encoding), "raw",
	     "data encoding. Possibilities are \"raw\" and \"ascii\".",
	     NULL, &unuEncodingHestCB);
  hestOptAdd(&opt, "endian", "endian", airTypeOther, 1, 1, &(io->endian),
	     nrrdEnumValToStr(nrrdEnumEndian, airMyEndian),
	     "Endianness of data; relevent for any raw data with value "
	     "representation bigger than 8 bits. Defaults to endianness "
	     "of this machine.",
	     NULL, &unuEndianHestCB);
  OPT_ADD_NOUT(out, "output nrrd");
  airMopAdd(mop, opt, (airMopper)hestOptFree, airMopAlways);

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
  nrrd->dim = sizeLen;
  nrrdAxesSet_nva(nrrd, nrrdAxesInfoSize, size);
  
  if (_nrrdReadData[io->encoding](nrrd, io)) {
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: error reading data:\n%s", me, err);
    airMopError(mop);
    return 1;
  }

  SAVE(nrrd, NULL);

  airMopOkay(mop);
  return 0;
}
