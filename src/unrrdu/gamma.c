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

char *gammaName = "gamma";
#define INFO "Brighten or darken values with a gamma"
char *gammaInfo = INFO;
char *gammaInfoL = (INFO
		    ". ");

int
gammaMain(int argc, char **argv, char *me) {
  hestOpt *opt = NULL;
  char *out, *err;
  Nrrd *nin, *nout;
  double min, max, gamma;
  airArray *mop;

  OPT_ADD_NIN(nin, "input nrrd");
  hestOptAdd(&opt, "min", "value", airTypeDouble, 1, 1, &min, "nan",
	     "Value to implicitly map to 0.0 prior to calling pow(). "
	     "Defaults to lowest value found in input nrrd.");
  hestOptAdd(&opt, "max", "value", airTypeDouble, 1, 1, &max, "nan",
	     "Value to implicitly map to 1.0 prior to calling pow(). "
	     "Defaults to highest value found in input nrrd.");
  hestOptAdd(&opt, "g", "gamma", airTypeDouble, 1, 1, &gamma, NULL,
	     "The power given to pow() to transform values.  1.0 does "
	     "nothing.  Negative gammas invert values (like in xv). ");
  OPT_ADD_NOUT(out, "output nrrd");

  mop = airMopInit();
  airMopAdd(mop, opt, (airMopper)hestOptFree, airMopAlways);

  USAGE(gammaInfoL);
  PARSE();
  airMopAdd(mop, opt, (airMopper)hestParseFree, airMopAlways);

  nout = nrrdNew();
  airMopAdd(mop, nout, (airMopper)nrrdNuke, airMopAlways);

  nin->min = min;
  nin->max = max;
  if (nrrdCleverMinMax(nin)) {
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble determining range in nrrd:\n%s", me, err);
    airMopError(mop);
    return 1;
  }
  if (nrrdArithGamma(nout, nin, gamma, nin->min, nin->max)) {
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: error doing gamma:\n%s", me, err);
    airMopError(mop);
    return 1;
  }

  SAVE(nout, NULL);

  airMopOkay(mop);
  return 0;
}
