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

/* NB: not the same as char sliceName[] = "slice"; Read your C FAQs */
char *sliceName = "slice";
#define INFO "Slice at a position along an axis"
char *sliceInfo = INFO;
char *sliceInfoL = (INFO
		    ". Output nrrd dimension is one less than input nrrd "
		    "dimension.  Per-axis information is preserved.");

int
sliceMain(int argc, char **argv, char *me) {
  hestOpt *opt = NULL;
  char *out, *err;
  Nrrd *nin, *nout;
  int axis, pos;
  airArray *mop;

  OPT_ADD_NIN(nin, "input nrrd");
  OPT_ADD_AXIS(axis, "axis to slice along");
  hestOptAdd(&opt, "p", "pos", airTypeInt, 1, 1, &pos, NULL,
	     "position (in index space) to slice at");
  OPT_ADD_NOUT(out, "output nrrd");

  mop = airMopInit();
  airMopAdd(mop, opt, (airMopper)hestOptFree, airMopAlways);

  USAGE(sliceInfoL);
  PARSE();
  airMopAdd(mop, opt, (airMopper)hestParseFree, airMopAlways);

  nout = nrrdNew();
  airMopAdd(mop, nout, (airMopper)nrrdNuke, airMopAlways);

  if (nrrdSlice(nout, nin, axis, pos)) {
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: error slicing nrrd:\n%s", me, err);
    airMopError(mop);
    return 1;
  }

  SAVE(nout, NULL);

  airMopOkay(mop);
  return 0;
}
