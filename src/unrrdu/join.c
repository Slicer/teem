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

char *joinName = "join";
#define INFO "Connect slices and/or slabs into a bigger nrrd"
char *joinInfo = INFO;
char *joinInfoL = (INFO
		   ". Can stich images into volumes, or tile images side "
		   "by side, or attach images onto volumes.");

int
joinMain(int argc, char **argv, char *me) {
  hestOpt *opt = NULL;
  char *out, *err;
  Nrrd **nin, *nout;
  int ninLen, axis, incrDim;
  airArray *mop;

  hestOptAdd(&opt, "i", "nin0", airTypeOther, 1, -1, &nin, NULL,
	     "everything to be joined together",
	     &ninLen, &unuNrrdHestCB);
  OPT_ADD_AXIS(axis, "axis to join along");
  hestOptAdd(&opt, "incr", NULL, airTypeInt, 0, 0, &incrDim, NULL,
	     "in situations where the join axis is among the axes of "
	     "the input nrrds, then this flag signifies that the join "
	     "axis should be *inserted*, and the output dimension should "
	     "be one greater than input dimension.  Without this flag, the "
	     "nrrds are joined side-by-side, along an existing axis.");
  OPT_ADD_NOUT(out, "output nrrd");

  mop = airMopInit();
  airMopAdd(mop, opt, (airMopper)hestOptFree, airMopAlways);

  USAGE(joinInfoL);
  PARSE();
  airMopAdd(mop, opt, (airMopper)hestParseFree, airMopAlways);

  nout = nrrdNew();
  airMopAdd(mop, nout, (airMopper)nrrdNuke, airMopAlways);

  if (nrrdJoin(nout, nin, ninLen, axis, incrDim)) {
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: error joining nrrds:\n%s", me, err);
    airMopError(mop);
    return 1;
  }

  SAVE(nout, NULL);

  airMopOkay(mop);
  return 0;
}
