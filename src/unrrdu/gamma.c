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
