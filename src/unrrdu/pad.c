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

char *padName = "pad";
char *padInfo = "Pad along each axis to make a bigger nrrd";

int
padMain(int argc, char **argv, char *me) {
  hestOpt *opt = NULL;
  char *out, *err;
  Nrrd *nin, *nout;
  int *minOff, minLen, *maxOff, maxLen, ax, bb, ret,
    min[NRRD_DIM_MAX], max[NRRD_DIM_MAX];
  double padVal;
  airArray *mop;

  OPT_ADD_NIN(nin, "input nrrd");
  OPT_ADD_BOUND("min", minOff,
		"low corner of bounding box.\n "
		"\b\bo <int> gives 0-based index\n "
		"\b\bo M+<int>, M-<int> give index relative "
		"to the last sample on the axis (M == #samples-1).",
		minLen);
  OPT_ADD_BOUND("max", maxOff, "high corner of bounding box", maxLen);
  hestOptAdd(&opt, "b", "behavior", airTypeOther, 1, 1, &bb, "bleed",
	     "How to handle samples beyond the input bounds:\n "
	     "\b\bo \"pad\": use some specified value\n "
	     "\b\bo \"bleed\": extend border values outward\n "
	     "\b\bo \"wrap\": wrap-around to other side", 
	     NULL, &unuBoundaryHestCB);
  hestOptAdd(&opt, "v", "value", airTypeDouble, 1, 1, &padVal, "0.0",
	     "for \"pad\" boundary behavior, pad with this value");
  OPT_ADD_NOUT(out, "output nrrd");

  mop = airMopInit();
  airMopAdd(mop, opt, (airMopper)hestOptFree, airMopAlways);

  USAGE(padInfo);
  PARSE();
  airMopAdd(mop, opt, (airMopper)hestParseFree, airMopAlways);

  if (!( minLen == nin->dim && maxLen == nin->dim )) {
    fprintf(stderr,
	    "%s: # min coords (%d) or max coords (%d) != nrrd dim (%d)\n",
	    me, minLen, maxLen, nin->dim);
    airMopError(mop);
    return 1;
  }
  for (ax=0; ax<=nin->dim-1; ax++) {
    min[ax] = minOff[0 + 2*ax]*(nin->axis[ax].size-1) + minOff[1 + 2*ax];
    max[ax] = maxOff[0 + 2*ax]*(nin->axis[ax].size-1) + maxOff[1 + 2*ax];
    fprintf(stderr, "%s: ax %2d: min = %4d, max = %4d\n",
	    me, ax, min[ax], max[ax]);
  }

  nout = nrrdNew();
  airMopAdd(mop, nout, (airMopper)nrrdNuke, airMopAlways);

  if (nrrdBoundaryPad == bb) {
    ret = nrrdPad(nout, nin, min, max, bb, padVal);
  }
  else {
    ret = nrrdPad(nout, nin, min, max, bb);
  }
  if (ret) {
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: error padding nrrd:\n%s", me, err);
    airMopError(mop);
    return 1;
  }

  SAVE(nout, NULL);

  airMopOkay(mop);
  return 0;
}
