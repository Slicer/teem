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
extern int _nrrdOneLine(int *lenP, NrrdIO *io, FILE *file);

#define INFO "Print data segment of a nrrd file"
char *_unrrdu_dataInfoL = 
(INFO  ".  The value of this is to pass the data segment in isolation to a "
 "stand-alone decoder, in case this teem compilation lacks an optional "
 "data encoding required for a given nrrd file.  Caveats: "
 "Will start copying "
 "characters from the datafile until EOF is hit, so this won't work "
 "correctly if the datafile has extraneous content at the end.  Will "
 "skip lines (as per \"line skip:\" header field) if needed, but can only "
 "skip bytes (as per \"byte skip:\") if the encoding is NOT a compression. "
 "\n \n "
 "To make vol.raw contain the uncompressed data from vol.nrrd "
 "which uses \"gz\" encoding: \"unu data vol.nrrd | gunzip > vol.raw\"");

int
unrrdu_dataMain(int argc, char **argv, char *me, hestParm *hparm) {
  hestOpt *opt = NULL;
  char *err, *inS=NULL, *outS=NULL;
  Nrrd *nin;
  NrrdIO *io;
  airArray *mop;
  int car, pret;
  FILE *fin, *fout, *dataFile;

  mop = airMopInit();
  hestOptAdd(&opt, NULL, "nin", airTypeString, 1, 1, &inS, NULL,
	     "input nrrd");
  hestOptAdd(&opt, NULL, "out", airTypeString, 0, 1, &outS, "-",
	     "output file");
  airMopAdd(mop, opt, (airMopper)hestOptFree, airMopAlways);

  USAGE(_unrrdu_dataInfoL);
  PARSE();
  airMopAdd(mop, opt, (airMopper)hestParseFree, airMopAlways);

  io = nrrdIONew();
  airMopAdd(mop, io, (airMopper)nrrdIONix, airMopAlways);

  if (!strcmp("-", inS)) {
    fin = stdin;
#ifdef WIN32
    _setmode(_fileno(fin), _O_BINARY);
#endif
  } else {
    if (!( fin = fopen(inS, "rb") )) {
      fprintf(stderr, "%s: couldn't fopen(\"%s\",\"rb\"): %s\n", 
	      me, inS, strerror(errno));
      airMopError(mop); return 1;
    }
    nrrdDirBaseSet(io, inS);
  }
  if (!strcmp("-", outS)) {
    fout = stdout;
#ifdef WIN32
    _setmode(_fileno(fout), _O_BINARY);
#endif
  } else {
    if (!( fout = fopen(outS, "wb") )) {
      fprintf(stderr, "%s: couldn't fopen(\"%s\",\"wb\"): %s\n", 
	      me, inS, strerror(errno));
      airMopError(mop); return 1;
    }
  }
  airMopAdd(mop, fin, (airMopper)airFclose, airMopAlways);
  airMopAdd(mop, fout, (airMopper)airFclose, airMopAlways);
  nin = nrrdNew();
  airMopAdd(mop, nin, (airMopper)nrrdNuke, airMopAlways);

  io->skipData = AIR_TRUE;
  io->keepSeperateDataFileOpen = AIR_TRUE;
  if (nrrdRead(nin, fin, io)) {
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: error reading header:\n%s", me, err);
    airMopError(mop);
    return 1;
  }
  if (!( nrrdMagicOldNRRD == io->magic || nrrdMagicNRRD0001 == io->magic )) {
    fprintf(stderr, "%s: can only print data of NRRD format files\n", me);
    airMopError(mop); return 1;
  }
  if (io->seperateHeader) {
    dataFile = io->dataFile;
    airMopAdd(mop, dataFile, (airMopper)airFclose, airMopAlways);
  } else {
    dataFile = fin;
  }
  car = fgetc(dataFile);
  while (EOF != car) {
    fputc(car, fout);
    car = fgetc(dataFile);
  }
  
  airMopOkay(mop);
  return 0;
}

UNRRDU_CMD(data, INFO);
