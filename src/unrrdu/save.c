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

char *saveName = "save";
char *saveInfo = "Write nrrd with specific file format or encoding";

int
unuParseFormat(void *ptr, char *str, char err[AIR_STRLEN_HUGE]) {
  char me[]="unuParseFormat";
  int *formatP;

  if (!(ptr && str)) {
    sprintf(err, "%s: got NULL pointer", me);
    return 1;
  }
  formatP = ptr;
  *formatP = nrrdEnumStrToVal(nrrdEnumFormat, str);
  if (nrrdFormatUnknown == *formatP) {
    sprintf(err, "%s: \"%s\" is not a recognized format", me, str);
    return 1;
  }
  return 0;
}

hestCB unuFormatHestCB = {
  sizeof(int),
  "format",
  unuParseFormat,
  NULL
};


int
saveMain(int argc, char **argv, char *me) {
  hestOpt *opt = NULL;
  char *out, *err;
  Nrrd *nin, *nout;
  int format, encoding;
  airArray *mop;
  nrrdIO *io;

  OPT_ADD_NIN(nin, "input nrrd");
  hestOptAdd(&opt, "f", "format", airTypeOther, 1, 1, &format, "nrrd",
	     "output file format. Possibilities include:\n "
	     "\b\bo \"nrrd\": standard nrrd format\n "
	     "\b\bo \"pnm\": PNM image; PPM for color, PGM for grayscale\n "
	     "\b\bo \"table\": plain ASCII table for 2-D data",
	     NULL, &unuFormatHestCB);
  hestOptAdd(&opt, "e", "encoding", airTypeOther, 1, 1, &encoding, "raw",
	     "output file format. Possibilities are \"raw\" and \"ascii\"",
	     NULL, &unuEncodingHestCB);
  OPT_ADD_NOUT(out, "output nrrd");

  mop = airMopInit();
  airMopAdd(mop, opt, (airMopper)hestOptFree, airMopAlways);

  USAGE(saveInfo);
  PARSE();
  airMopAdd(mop, opt, (airMopper)hestParseFree, airMopAlways);

  nout = nrrdNew();
  airMopAdd(mop, nout, (airMopper)nrrdNuke, airMopAlways);
  io = nrrdIONew();
  airMopAdd(mop, io, (airMopper)nrrdIONix, airMopAlways);

  nrrdCopy(nout, nin);
  io->format = format;
  io->encoding = encoding;
  
  SAVE(nout, io);

  airMopOkay(mop);
  return 0;
}
