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
  int axis, _pos[2], pos;
  airArray *mop;

  OPT_ADD_NIN(nin, "input nrrd");
  OPT_ADD_AXIS(axis, "axis to slice along");
  hestOptAdd(&opt, "p", "pos", airTypeOther, 1, 1, _pos, NULL,
	     "position to slice at:\n "
	     "\b\bo <int> gives 0-based index\n "
	     "\b\bo M-<int> give index relative "
	     "to the last sample on the axis (M == #samples-1).",
	     NULL, &unuPosHestCB);
  OPT_ADD_NOUT(out, "output nrrd");

  mop = airMopInit();
  airMopAdd(mop, opt, (airMopper)hestOptFree, airMopAlways);

  USAGE(sliceInfoL);
  PARSE();
  airMopAdd(mop, opt, (airMopper)hestParseFree, airMopAlways);
  if (!( AIR_INSIDE(0, axis, nin->dim-1) )) {
    fprintf(stderr, "%s: axis %d not in range [0,%d]\n", me, axis, nin->dim-1);
    return 1;
  }
  pos = _pos[0]*nin->axis[axis].size + _pos[1];

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
