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
char *sliceInfo = "Slice at a position along an axis";

int
sliceMain(int argc, char **argv, char *me) {
  hestOpt *opt = NULL;
  char *out, *err;
  Nrrd *nin, *nout;
  int axis, pos;
  airArray *mop;

  OPT_ADD_NIN(nin, "input");
  OPT_ADD_AXIS(axis, "axis to slice along");
  OPT_ADD_POS(pos, "position (in index space) to slice at");
  OPT_ADD_NOUT(out, "output nrrd");

  mop = airMopInit();
  airMopAdd(mop, opt, (airMopper)hestOptFree, airMopAlways);

  if (!argc) {
    hestInfo(stderr, me, sliceInfo, hparm);
    hestUsage(stderr, opt, me, hparm);
    hestGlossary(stderr, opt, hparm);
    airMopError(mop);
    return 1;
  }

  if (hestParse(opt, argc, argv, &err, hparm)) {
    fprintf(stderr, "%s: %s\n", me, err); free(err);
    hestUsage(stderr, opt, me, hparm);
    hestGlossary(stderr, opt, hparm);
    airMopError(mop);
    return 1;
  }
  airMopAdd(mop, opt, (airMopper)hestParseFree, airMopAlways);

  nout = nrrdNew();
  airMopAdd(mop, nout, (airMopper)nrrdNuke, airMopAlways);
  if (nrrdSlice(nout, nin, axis, pos)) {
    err = biffGet(NRRD);
    fprintf(stderr, "%s: error slicing nrrd:\n%s", me, err);
    free(err);
    airMopError(mop);
    return 1;
  }

  if (nrrdSave(out, nout, NULL)) {
    err = biffGet(NRRD);
    fprintf(stderr, "%s: error saving nrrd to \"%s\":\n%s\n", me, out, err);
    free(err);
    airMopError(mop);
    return 1;
  }

  airMopOkay(mop);
  return 0;
}
