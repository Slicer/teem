/*
  Teem: Tools to process and visualize scientific data and images
  Copyright (C) 2009--2023  University of Chicago
  Copyright (C) 2005--2008  Gordon Kindlmann
  Copyright (C) 1998--2004  University of Utah

  This library is free software; you can redistribute it and/or modify it under the terms
  of the GNU Lesser General Public License (LGPL) as published by the Free Software
  Foundation; either version 2.1 of the License, or (at your option) any later version.
  The terms of redistributing and/or modifying this software also include exceptions to
  the LGPL that facilitate static linking.

  This library is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
  PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License along with
  this library; if not, write to Free Software Foundation, Inc., 51 Franklin Street,
  Fifth Floor, Boston, MA 02110-1301 USA
*/

#include "unrrdu.h"
#include "privateUnrrdu.h"

#define INFO "Select subset of slices along an axis"
static const char *_unrrdu_sselectInfoL
  = (INFO ". The choice to keep or nix a slice is determined by whether the "
          "values in a given 1-D line of values is above or below a given "
          "threshold.\n "
          "* Uses nrrdSliceSelect");

static int
unrrdu_sselectMain(int argc, const char **argv, const char *me, hestParm *hparm) {
  hestOpt *opt = NULL;
  char *err;
  Nrrd *nin, *noutAbove, *noutBelow, *nline;
  unsigned int axis;
  double thresh;
  int pret;
  airArray *mop;
  char *outS[2];

  OPT_ADD_NIN(nin, "input nrrd");
  OPT_ADD_AXIS(axis, "axis to slice along");
  hestOptAdd_1_Other(&opt, "s,selector", "nline", &nline, NULL,
                     "the 1-D nrrd of values to compare with threshold", nrrdHestNrrd);
  hestOptAdd_1_Double(&opt, "th", "thresh", &thresh, NULL, "threshold on selector line");
  hestOptAdd_2_String(&opt, "o,output", "above below", outS, "- x",
                      "outputs for slices corresponding to values "
                      "above (first) and below (second) given threshold. "
                      "Use \"x\" to say that no output is desired.");

  mop = airMopNew();
  airMopAdd(mop, opt, hestOptFree_vp, airMopAlways);

  USAGE_OR_PARSE(_unrrdu_sselectInfoL);
  airMopAdd(mop, opt, (airMopper)hestParseFree, airMopAlways);

  if (!strcmp(outS[0], "x") && !strcmp(outS[1], "x")) {
    fprintf(stderr,
            "%s: need to save either above or below slices "
            "(can't use \"x\" for both)\n",
            me);
    airMopError(mop);
    return 1;
  }
  if (strcmp(outS[0], "x")) {
    noutAbove = nrrdNew();
    airMopAdd(mop, noutAbove, (airMopper)nrrdNuke, airMopAlways);
  } else {
    noutAbove = NULL;
  }
  if (strcmp(outS[1], "x")) {
    noutBelow = nrrdNew();
    airMopAdd(mop, noutBelow, (airMopper)nrrdNuke, airMopAlways);
  } else {
    noutBelow = NULL;
  }

  if (nrrdSliceSelect(noutAbove, noutBelow, nin, axis, nline, thresh)) {
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: error selecting slices:\n%s", me, err);
    airMopError(mop);
    return 1;
  }

  if (noutAbove) {
    SAVE(outS[0], noutAbove, NULL);
  }
  if (noutBelow) {
    SAVE(outS[1], noutBelow, NULL);
  }

  airMopOkay(mop);
  return 0;
}

UNRRDU_CMD(sselect, INFO);
