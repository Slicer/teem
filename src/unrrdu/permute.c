/*
  The contents of this file are subject to the University of Utah Public
  License (the "License"); you may not use this file except in
  compliance with the License.
  
  Software distributed under the License is distributed on an "AS IS"
  basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See
  the License for the specific language governing rights and limitations
  under the License.

  The Original Source Code is "teem", released March 23, 2001.
  
  The Original Source Code was developed by the University of Utah.
  Portions created by UNIVERSITY are Copyright (C) 2001, 1998 University
  of Utah. All Rights Reserved.
*/

#include "private.h"

char *permuteName = "permute";
char *permuteInfo = "Permute scan-line ordering of axes";

int
permuteMain(int argc, char **argv, char *me) {
  hestOpt *opt = NULL;
  char *out, *err;
  Nrrd *nin, *nout;
  int *perm, permLen;
  airArray *mop;

  OPT_ADD_NIN(nin, "input");
  hestOptAdd(&opt, "p|permute", "ax0 ax1 ", airTypeInt, 1, -1, &perm, NULL,
	     "new axis ordering", &permLen);
  OPT_ADD_NOUT(out, "output nrrd");

  mop = airMopInit();
  airMopAdd(mop, opt, (airMopper)hestOptFree, airMopAlways);

  USAGE(permuteInfo);
  PARSE();

  nout = nrrdNew();
  airMopAdd(mop, nout, (airMopper)nrrdNuke, airMopAlways);

  if (!( permLen == nin->dim )) {
    fprintf(stderr,
	    "%s: # axes in permutation (%d) != nrrd dim (%d)\n",
	    me, permLen, nin->dim);
    airMopError(mop);
    return 1;
  }
  printf("!%s: perm: %d %d %d\n", me, perm[0], perm[1], perm[2]);

  if (nrrdPermuteAxes(nout, nin, perm)) {
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: error permuting nrrd:\n%s", me, err);
    airMopError(mop);
    return 1;
  }

  SAVE();

  airMopOkay(mop);
  return 0;
}
