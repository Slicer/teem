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

#define INFO "Write nrrd with specific format, encoding, or endianness"
char *_unrrdu_saveInfoL =
(INFO
 ". Use \"unu\tsave\t-f\tpnm\t|\txv\t-\" to view PPM- or "
 "PGM-compatible nrrds");

int
unrrdu_saveMain(int argc, char **argv, char *me, hestParm *hparm) {
  hestOpt *opt = NULL;
  char *out, *err, encInfo[AIR_STRLEN_LARGE];
  Nrrd *nin, *nout;
  airArray *mop;
  NrrdIO *io;

  mop = airMopInit();
  io = nrrdIONew();
  airMopAdd(mop, io, (airMopper)nrrdIONix, airMopAlways);

  OPT_ADD_NIN(nin, "input nrrd");
  hestOptAdd(&opt, "f", "format", airTypeEnum, 1, 1, &(io->format), NULL,
	     "output file format. Possibilities include:\n "
	     "\b\bo \"nrrd\": standard nrrd format\n "
	     "\b\bo \"pnm\": PNM image; PPM for color, PGM for grayscale\n "
	     "\b\bo \"text\": plain ASCII text for 1-D and 2-D data",
	     NULL, nrrdFormat);
  strcpy(encInfo,
	 "output file format. Possibilities include:"
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
  hestOptAdd(&opt, "e", "encoding", airTypeEnum, 1, 1, &(io->encoding), "raw",
	     encInfo, NULL, nrrdEncoding);
  hestOptAdd(&opt, "en", "endian", airTypeEnum, 1, 1, &(io->endian),
	     airEnumStr(airEndian, airMyEndian),
	     "Endianness of to save data out as; \"little\" for Intel and "
	     "friends; \"big\" for everyone else. "
	     "Defaults to endianness of this machine",
	     NULL, airEndian);
  OPT_ADD_NOUT(out, "output nrrd");

  airMopAdd(mop, opt, (airMopper)hestOptFree, airMopAlways);

  USAGE(_unrrdu_saveInfoL);
  PARSE();
  airMopAdd(mop, opt, (airMopper)hestParseFree, airMopAlways);
  nout = nrrdNew();
  airMopAdd(mop, nout, (airMopper)nrrdNuke, airMopAlways);

  nrrdCopy(nout, nin);
  
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
    /* we know exactly what part of this function (since we know
       airEndsWith()) run, so we could copy the code, but let's not */
    _nrrdGuessFormat(io, out);
  }

  SAVE(out, nout, io);

  airMopOkay(mop);
  return 0;
}

UNRRDU_CMD(save, INFO);
