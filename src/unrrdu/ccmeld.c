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

#define INFO "Merge islands into their surrounding"
char *_unrrdu_ccmeldInfoL =
(INFO
 ".  This operates on the output of \"ccfind\". "
 "Connected components (CCs) with only one adjacent CC (that is, islands) "
 "are absorbed into their surrounding if they are smaller than the "
 "surround, and are smaller then some given maximum significant size. ");

int
unrrdu_ccmeldMain(int argc, char **argv, char *me, hestParm *hparm) {
  hestOpt *opt = NULL;
  char *out, *err;
  Nrrd *nin, *nout;
  airArray *mop;
  int conny, pret, maxSize;

  hestOptAdd(&opt, "s", "max size", airTypeInt, 1, 1, &maxSize, NULL,
	     "the largest CC size which will be absorbed "
	     "into surround.  CCs larger than this are deemed to significant "
	     "to alter");
  hestOptAdd(&opt, "c", "connectivity", airTypeInt, 1, 1, &conny, "1",
	     "what kind of connectivity to use: the number of coordinates "
	     "that vary in order to traverse the neighborhood of a given "
	     "sample.  In 2D: \"1\": 4-connected, \"2\": 8-connected");
  OPT_ADD_NIN(nin, "input nrrd");
  OPT_ADD_NOUT(out, "output nrrd");

  mop = airMopNew();
  airMopAdd(mop, opt, (airMopper)hestOptFree, airMopAlways);

  USAGE(_unrrdu_ccmeldInfoL);
  PARSE();
  airMopAdd(mop, opt, (airMopper)hestParseFree, airMopAlways);

  nout = nrrdNew();
  airMopAdd(mop, nout, (airMopper)nrrdNuke, airMopAlways);

  if (nrrdCCMeld(nout, nin, maxSize, conny)) {
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: error doing melding:\n%s", me, err);
    airMopError(mop);
    return 1;
  }

  SAVE(out, nout, NULL);

  airMopOkay(mop);
  return 0;
}

UNRRDU_CMD(ccmeld, INFO);
