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

char *shuffleName = "shuffle";
#define INFO "Permute samples along one axis"
char *shuffleInfo = INFO;
char *shuffleInfoL = (INFO
		      ". Slices along one axis are re-arranged as units "
		      "according to the given permutation (or its inverse). "
		      "The permutation tells which old slice to put at each "
		      "new position.  For example, the shuffle "
		      "0->1,\t1->2,\t2->0 would be \"2 0 1\".");
		      

int
shuffleMain(int argc, char **argv, char *me) {
  hestOpt *opt = NULL;
  char *out, *err;
  Nrrd *nin, *nout;
  int axis, inverse;
  int *perm, *iperm, **whichperm, permLen;
  airArray *mop;

  /* so that long permutations can be read from file */
  hparm->respFileEnable = AIR_TRUE;

  OPT_ADD_NIN(nin, "input nrrd");
  hestOptAdd(&opt, "inv", NULL, airTypeInt, 0, 0, &inverse, NULL,
	     "use inverse of given permutation");
  hestOptAdd(&opt, "p", "slc0 slc1", airTypeInt, 1, -1, &perm, NULL,
	     "new slice ordering", &permLen);
  OPT_ADD_AXIS(axis, "axis to shuffle along");
  OPT_ADD_NOUT(out, "output nrrd");

  mop = airMopInit();
  airMopAdd(mop, opt, (airMopper)hestOptFree, airMopAlways);

  USAGE(shuffleInfoL);
  PARSE();
  airMopAdd(mop, opt, (airMopper)hestParseFree, airMopAlways);

  nout = nrrdNew();
  airMopAdd(mop, nout, (airMopper)nrrdNuke, airMopAlways);
  
  /* we have to do error checking on axis in order to do error
     checking on length of permutation */
  if (!( AIR_INSIDE(0, axis, nin->dim-1) )) {
    fprintf(stderr, "%s: axis %d not in valid range [%d,%d]\n", 
	    me, axis, 0, nin->dim-1);
    airMopError(mop);
    return 1;
  }
  if (!( permLen == nin->axis[axis].size )) {
    fprintf(stderr, "%s: permutation length (%d) != axis %d's size (%d)\n",
	    me, permLen, axis, nin->axis[axis].size);
    airMopError(mop);
    return 1;
  }
  if (inverse) {
    iperm = calloc(permLen, sizeof(int));
    airMopAdd(mop, iperm, airFree, airMopAlways);
    if (nrrdInvertPerm(iperm, perm, permLen)) {
      fprintf(stderr,
	      "%s: couldn't compute inverse of given permutation\n", me);
      airMopError(mop);
      return 1;
    }
    whichperm = &iperm;
  }
  else {
    whichperm = &perm;
  }

  if (nrrdShuffle(nout, nin, axis, *whichperm)) {
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: error shuffling nrrd:\n%s", me, err);
    airMopError(mop);
    return 1;
  }

  SAVE(nout, NULL);

  airMopOkay(mop);
  return 0;
}
