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

#define INFO "Print header of a nrrd file"
char *_unrrdu_headInfoL = 
(INFO  ".  The value of this is simply to print the contents of a nrrd "
 "header, avoiding the use of \"head -N\", where N has to be determined "
 "manually, and thereby avoiding the risk of printing raw binary data "
 "(following the header) to screen, as this tends to be annoying and "
 "clobber terminal settings.");

int
unrrdu_headMain(int argc, char **argv, char *me, hestParm *hparm) {
  hestOpt *opt = NULL;
  char *err, *inS=NULL, *outS=NULL;
  NrrdIO *io;
  airArray *mop;
  int len, magic, pret;
  FILE *fin, *fout;

  mop = airMopInit();
  hestOptAdd(&opt, NULL, "nin", airTypeString, 1, 1, &inS, NULL,
	     "input nrrd");
  hestOptAdd(&opt, NULL, "out", airTypeString, 0, 1, &outS, "-",
	     "output file");
  airMopAdd(mop, opt, (airMopper)hestOptFree, airMopAlways);

  USAGE(_unrrdu_headInfoL);
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

  if (_nrrdOneLine(&len, io, fin)) {
    fprintf(stderr, "%s: error getting first line of file \"%s\"", me, inS);
    airMopError(mop); return 1;
  }
  if (!len) {
    fprintf(stderr, "%s: immediately hit EOF", me);
    airMopError(mop); return 1;
  }
  magic = airEnumVal(nrrdMagic, io->line);
  if (!( nrrdMagicOldNRRD == magic || nrrdMagicNRRD0001 == magic )) {
    fprintf(stderr, "%s: first line (\"%s\") isn't a nrrd magic\n", 
	    me, io->line);
    airMopError(mop); return 1;
  }
  while (len > 1) {
    fprintf(fout, "%s\n", io->line);
    _nrrdOneLine(&len, io, fin);
  };

  airMopOkay(mop);
  return 0;
}

UNRRDU_CMD(head, INFO);
