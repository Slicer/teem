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

char *jhistName = "jhist";
#define INFO "Create joint histogram of two or more nrrds"
char *jhistInfo = INFO;
char *jhistInfoL = (INFO
		    ". Each axis of the output corresponds to one of the "
		    "input nrrds, and each bin in the output records the "
		    "number of corresponding positions in the inputs with "
		    "a combination of values represented by the coordinates "
		    "of the bin.");

int
jhistMain(int argc, char **argv, char *me) {
  hestOpt *opt = NULL;
  char *out, *err;
  Nrrd **nin, *nout;
  int type, d, ninLen, *bin, binLen, clamp[NRRD_DIM_MAX];
  airArray *mop;

  hestOptAdd(&opt, "i", "nin0 nin1", airTypeOther, 2, -1, &nin, NULL,
	     "All input nrrds",
	     &ninLen, &unuNrrdHestCB);
  hestOptAdd(&opt, "b", "bins0 bins1", airTypeInt, 2, -1, &bin, NULL,
	     "bins<i> is the number of bins to use along axis i (of joint "
	     "histogram), which represents the values of nin<i> ",
	     &binLen);
  OPT_ADD_TYPE(type, "type to use for output (the type used to store hit "
	       "counts in the joint histogram).  Clamping is done on hit "
	       "counts so that they never overflow a fixed-point type.");
  OPT_ADD_NOUT(out, "output nrrd");

  mop = airMopInit();
  airMopAdd(mop, opt, (airMopper)hestOptFree, airMopAlways);

  USAGE(jhistInfo);
  PARSE();
  airMopAdd(mop, opt, (airMopper)hestParseFree, airMopAlways);

  if (ninLen != binLen) {
    fprintf(stderr, "%s: # input nrrds (%d) != # bin specifications (%d)\n",
	    me, ninLen, binLen);
    airMopError(mop);
    return 1;
  }
  for (d=0; d<=ninLen-1; d++) {
    if (nrrdCleverMinMax(nin[d])) {
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

  SAVE(nout, NULL);

  airMopOkay(mop);
  return 0;
}
