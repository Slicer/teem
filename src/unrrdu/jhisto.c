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

char *jhistoName = "jhisto";
#define INFO "Create joint histogram of two or more nrrds"
char *jhistoInfo = INFO;
char *jhistoInfoL = (INFO
		    ". Each axis of the output corresponds to one of the "
		    "input nrrds, and each bin in the output records the "
		    "number of corresponding positions in the inputs with "
		    "a combination of values represented by the coordinates "
		    "of the bin.");

int
jhistoMain(int argc, char **argv, char *me) {
  hestOpt *opt = NULL;
  char *out, *err;
  Nrrd **nin, *nout;
  int type, d, ninLen, *bin, binLen, clamp[NRRD_DIM_MAX];
  airArray *mop;

  hestOptAdd(&opt, "i", "nin0 nin1", airTypeOther, 2, -1, &nin, NULL,
	     "All input nrrds",
	     &ninLen, NULL, nrrdHestNrrd);
  hestOptAdd(&opt, "b", "bins0 bins1", airTypeInt, 2, -1, &bin, NULL,
	     "bins<i> is the number of bins to use along axis i (of joint "
	     "histogram), which represents the values of nin<i> ",
	     &binLen);
  OPT_ADD_TYPE(type, "type to use for output (the type used to store hit "
	       "counts in the joint histogram).  Clamping is done on hit "
	       "counts so that they never overflow a fixed-point type.",
	       "uint");
  OPT_ADD_NOUT(out, "output nrrd");

  mop = airMopInit();
  airMopAdd(mop, opt, (airMopper)hestOptFree, airMopAlways);

  USAGE(jhistoInfo);
  PARSE();
  airMopAdd(mop, opt, (airMopper)hestParseFree, airMopAlways);

  if (ninLen != binLen) {
    fprintf(stderr, "%s: # input nrrds (%d) != # bin specifications (%d)\n",
	    me, ninLen, binLen);
    airMopError(mop);
    return 1;
  }
  for (d=0; d<=ninLen-1; d++) {
    if (nrrdMinMaxCleverSet(nin[d])) {
      airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
      fprintf(stderr, "%s: trouble determining range in nrrd %d:\n%s",
	      me, d, err);
      airMopError(mop);
      return 1;
    }
    clamp[d] = 0;
  }

  nout = nrrdNew();
  airMopAdd(mop, nout, (airMopper)nrrdNuke, airMopAlways);

  if (nrrdHistoJoint(nout, nin, ninLen, bin, type, clamp)) {
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: error doing joint histogram:\n%s", me, err);
    airMopError(mop);
    return 1;
  }

  SAVE(out, nout, NULL);

  airMopOkay(mop);
  return 0;
}
