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
extern int _nrrdSplitName(char **dirP, char **baseP, const char *name);
extern void _nrrdGuessFormat(NrrdIO *io, const char *filename);

#define INFO "Write nrrd with specific file format or encoding"
char *_unrrdu_saveInfoL =
(INFO
 ". Use \"unu\tsave\t-f\tpnm\t|\txv\t-\" to view PPM- or "
 "PGM-compatible nrrds");

int
unrrdu_saveMain(int argc, char **argv, char *me, hestParm *hparm) {
  hestOpt *opt = NULL;
  char *out, *err, encInfo[AIR_STRLEN_HUGE];
  Nrrd *nin, *nout;
  int format, encoding;
  airArray *mop;
  NrrdIO *io;

  OPT_ADD_NIN(nin, "input nrrd");
  hestOptAdd(&opt, "f", "format", airTypeEnum, 1, 1, &format, NULL,
	     "output file format. Possibilities include:\n "
	     "\b\bo \"nrrd\": standard nrrd format\n "
	     "\b\bo \"pnm\": PNM image; PPM for color, PGM for grayscale\n "
	     "\b\bo \"text\": plain ASCII text for 1-D and 2-D data",
	     NULL, nrrdFormat);
  strcpy(encInfo,
	 "output file format. Possibilities include:"
	 "\n \b\bo \"raw\": raw encoding"
	 "\n \b\bo \"ascii\": print data in ascii");
  if (nrrdEncodingAvailable[nrrdEncodingZlib]) {
    strcat(encInfo, 
	   "\n \b\bo \"gz\": gzipped compressed raw format");
  }
  hestOptAdd(&opt, "e", "encoding", airTypeEnum, 1, 1, &encoding, "raw",
	     encInfo, NULL, nrrdEncoding);
  OPT_ADD_NOUT(out, "output nrrd");

  mop = airMopInit();
  airMopAdd(mop, opt, (airMopper)hestOptFree, airMopAlways);

  USAGE(_unrrdu_saveInfoL);
  PARSE();
  airMopAdd(mop, opt, (airMopper)hestParseFree, airMopAlways);

  nout = nrrdNew();
  airMopAdd(mop, nout, (airMopper)nrrdNuke, airMopAlways);
  io = nrrdIONew();
  airMopAdd(mop, io, (airMopper)nrrdIONix, airMopAlways);

  nrrdCopy(nout, nin);
  io->format = format;
  io->encoding = encoding;

  if (airEndsWith(out, NRRD_EXT_HEADER)) {
    if (io->format != nrrdFormatNRRD) {
      fprintf(stderr, "%s: WARNING: will use %s format\n", me,
	      airEnumStr(nrrdFormat, nrrdFormatNRRD));
      io->format = nrrdFormatNRRD;
    }
    _nrrdSplitName(&(io->dir), &(io->base), out);
    /* we know exactly what part of this function (since we know
       airEndsWith()) run, so we could copy the code, but let's not */
    _nrrdGuessFormat(io, out);
  }

  SAVE(out, nout, io);

  airMopOkay(mop);
  return 0;
}

UNRRDU_CMD(save, INFO);
