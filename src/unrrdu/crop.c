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

char *cropName = "crop";
char *cropInfo = "Crop along each axis to make a smaller nrrd";

int
cropMain(int argc, char **argv, char *me) {
  hestOpt *opt = NULL;
  char *out, *err;
  Nrrd *nin, *nout;
  int *minOff, minLen, *maxOff, maxLen, ax,
    min[NRRD_DIM_MAX], max[NRRD_DIM_MAX];
  airArray *mop;

  OPT_ADD_NIN(nin, "input");
  OPT_ADD_BOUND("min", minOff, "low corner of bounding box", minLen);
  OPT_ADD_BOUND("max", maxOff, "high corner of bounding box", maxLen);
  OPT_ADD_NOUT(out, "output nrrd");

  mop = airMopInit();
  airMopAdd(mop, opt, (airMopper)hestOptFree, airMopAlways);

  USAGE(cropInfo);
  PARSE();

  if (!( minLen == nin->dim && maxLen == nin->dim )) {
    fprintf(stderr,
	    "%s: # min coords (%d) or max coords (%d) != nrrd dim (%d)\n",
	    me, minLen, maxLen, nin->dim);
    airMopError(mop);
    return 1;
  }
  for (ax=0; ax<=nin->dim-1; ax++) {
    min[ax] = minOff[0 + 2*ax]*(nin->axis[ax].size-1) + minOff[1 + 2*ax];
    max[ax] = maxOff[0 + 2*ax]*(nin->axis[ax].size-1) + maxOff[1 + 2*ax];
    fprintf(stderr, "%s: ax %2d: min = %4d, max = %4d\n",
	    me, ax, min[ax], max[ax]);
  }

  nout = nrrdNew();
  airMopAdd(mop, nout, (airMopper)nrrdNuke, airMopAlways);

  if (nrrdCrop(nout, nin, min, max)) {
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: error cropping nrrd:\n%s", me, err);
    airMopError(mop);
    return 1;
  }

  SAVE();

  airMopOkay(mop);
  return 0;
}
