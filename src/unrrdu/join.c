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
