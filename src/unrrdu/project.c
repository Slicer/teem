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

char *projectName = "project";
#define INFO "Collapse scanlines to scalars along some axis"
char *projectInfo = INFO;
char *projectInfoL = (INFO
		      ". The scanline is reduced to a single scalar by "
		      "\"measuring\" all the values in the scanline "
		      "with some measure.  The output nrrd has dimension "
		      "one less than input; the output type depends on "
		      "the measure.");

int
unuParseMeasure(void *ptr, char *str, char err[AIR_STRLEN_HUGE]) {
  char me[]="unuParseMeasure";
  int *measrP;

  if (!(ptr && str)) {
    sprintf(err, "%s: got NULL pointer", me);
    return 1;
  }
  measrP = ptr;
  *measrP = nrrdEnumStrToVal(nrrdEnumMeasure, str);
  if (nrrdMeasureUnknown == *measrP) {
    sprintf(err, "%s: \"%s\" is not a recognized measure", me, str);
    return 1;
  }
  return 0;
}

hestCB unuMeasureHestCB = {
  sizeof(int),
  "measure",
  unuParseMeasure,
  NULL
};

int
projectMain(int argc, char **argv, char *me) {
  hestOpt *opt = NULL;
  char *out, *err;
  Nrrd *nin, *nout;
  int axis, measr;
  airArray *mop;

  OPT_ADD_NIN(nin, "input nrrd");
  OPT_ADD_AXIS(axis, "axis to project along");
  hestOptAdd(&opt, "m", "measr", airTypeOther, 1, 1, &measr, NULL,
	     "How to \"measure\" a scanline.  Possibilities include:\n "
	     "\b\bo \"min\", \"max\", \"mean\", \"median\", \"mode\": "
	     "(self-explanatory)\n "
	     "\b\bo \"product\", \"sum\": product or sum of all values along "
	     "scanline\n "
	     "\b\bo \"L1\", \"L2\", \"Linf\": different norms\n "
	     "\b\bo \"histo-min\",  \"histo-max\", \"histo-mean\", "
	     "\"histo-median\", \"histo-mode\", \"histo-product\", "
	     "\"histo-sum\", \"histo-variance\": same measures, but for when "
	     "the scanlines are histograms of values, not the values "
	     "themselves.", 
	     NULL, &unuMeasureHestCB);
  OPT_ADD_NOUT(out, "output nrrd");

  mop = airMopInit();
  airMopAdd(mop, opt, (airMopper)hestOptFree, airMopAlways);

  USAGE(projectInfoL);
  PARSE();
  airMopAdd(mop, opt, (airMopper)hestParseFree, airMopAlways);

  nout = nrrdNew();
  airMopAdd(mop, nout, (airMopper)nrrdNuke, airMopAlways);

  printf("%s: axis = %d, measr = %d\n", me, axis, measr);
  if (nrrdProject(nout, nin, axis, measr)) {
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: error projecting nrrd:\n%s", me, err);
    airMopError(mop);
    return 1;
  }

  SAVE(NULL);

  airMopOkay(mop);
  return 0;
}
