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

/*
******** unrrduHestSaveEncCB
** 
** for parsing output encoding information.  The compression encodings
** can have flags for controlling the compression level and strategy
** val[0]: 
*/
int
unrrduParseSaveEnc(void *ptr, char *_str, char err[AIR_STRLEN_HUGE]) {
  char me[]="unrrduParseSaveEnc", *str;
  int *val;
  char *col;

  if (!(ptr && _str)) {
    sprintf(err, "%s: got NULL pointer", me);
    return 1;
  }
  val = ptr;
  str = airStrdup(_str);
  val[0] = val[1] = val[2] = -1;
  val[0] = airEnumVal(nrrdEncoding, str);
  if (val[0] != nrrdEncodingUnknown) {
    /* we got a simple encoding string, we're done */
    free(str); return 0;
  }
  col = strchr(str, ':');
  if (!col) {
    sprintf(err, "%s: \"%s\" not simple encoding, but no colon seen", me, str);
    free(str); return 1;
  }
  *col = '\0';
  val[0] = airEnumVal(nrrdEncoding, str);
  if (nrrdEncodingUnknown == val[0]) {
    sprintf(err, "%s: \"%s\" not a recognized encoding", me, str);
    free(str); return 0;
  }
  if (!(nrrdEncodingIsCompression[val[0]])) {
    sprintf(err, "%s: only compression encodings have parameters", me);
    free(str); return 0;
  }
  
  return 0;
}

hestCB unrrduHestSaveEncCB = {
  3*sizeof(int),
  "output encoding",
  unrrduParseSaveEnc,
  NULL
};

/* bad bad bad Gordon */
extern void _nrrdGuessFormat(NrrdIO *io, const char *filename);

#define INFO "Write nrrd with specific format, encoding, or endianness"
char *_unrrdu_saveInfoL =
(INFO
 ". Use \"unu\tsave\t-f\tpnm\t|\txv\t-\" to view PPM- or "
 "PGM-compatible nrrds on unix.");

int
unrrdu_saveMain(int argc, char **argv, char *me, hestParm *hparm) {
  hestOpt *opt = NULL;
  char *out, *err, encInfo[AIR_STRLEN_LARGE];
  Nrrd *nin, *nout;
  airArray *mop;
  NrrdIO *io;
  int pret;

  mop = airMopInit();
  io = nrrdIONew();
  airMopAdd(mop, io, (airMopper)nrrdIONix, airMopAlways);

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
  OPT_ADD_NIN(nin, "input nrrd");
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
