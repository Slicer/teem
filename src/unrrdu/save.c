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
