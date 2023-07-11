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

#define INFO "Pad along each axis to make a bigger nrrd"
static const char *_unrrdu_padInfoL = (INFO ".\n "
                                            "* Uses nrrdPad_nva");

static int
unrrdu_padMain(int argc, const char **argv, const char *me, hestParm *hparm) {
  hestOpt *opt = NULL;
  char *out, *err;
  Nrrd *nin, *nout;
  unsigned int minLen, maxLen, ai;
  int bb, pret;
  long int *minOff, *maxOff;
  ptrdiff_t min[NRRD_DIM_MAX], max[NRRD_DIM_MAX];
  double padVal;
  airArray *mop;

  OPT_ADD_BOUND("min,minimum", 1, minOff, NULL,
                "low corner of bounding box.\n "
                "\b\bo <int> gives 0-based index\n "
                "\b\bo M, M+<int>, M-<int> give index relative "
                "to the last sample on the axis (M == #samples-1).",
                minLen);
  OPT_ADD_BOUND("max,maximum", 1, maxOff, NULL,
                "high corner of bounding box.  "
                "Besides the specification styles described above, "
                "there's also:\n "
                "\b\bo m+<int> give index relative to minimum.",
                maxLen);
  hestOptAdd_1_Enum(&opt, "b,boundary", "behavior", &bb, "bleed",
                    "How to handle samples beyond the input bounds:\n "
                    "\b\bo \"pad\": use some specified value\n "
                    "\b\bo \"bleed\": extend border values outward\n "
                    "\b\bo \"mirror\": repeated reflections\n "
                    "\b\bo \"wrap\": wrap-around to other side",
                    nrrdBoundary);
  hestOptAdd_1_Double(&opt, "v,value", "val", &padVal, "0.0",
                      "for \"pad\" boundary behavior, pad with this value");
  OPT_ADD_NIN(nin, "input nrrd");
  OPT_ADD_NOUT(out, "output nrrd");

  mop = airMopNew();
  airMopAdd(mop, opt, hestOptFree_vp, airMopAlways);

  USAGE_OR_PARSE(_unrrdu_padInfoL);
  airMopAdd(mop, opt, (airMopper)hestParseFree, airMopAlways);

  if (!(minLen == nin->dim && maxLen == nin->dim)) {
    fprintf(stderr, "%s: # min coords (%u) or max coords (%u) != nrrd dim (%u)\n", me,
            minLen, maxLen, nin->dim);
    airMopError(mop);
    return 1;
  }
  for (ai = 0; ai < nin->dim; ai++) {
    if (-1 == minOff[0 + 2 * ai]) {
      fprintf(stderr, "%s: can't use m+<int> specification for axis %u min\n", me, ai);
      airMopError(mop);
      return 1;
    }
  }
  for (ai = 0; ai < nin->dim; ai++) {
    min[ai] = minOff[0 + 2 * ai] * (nin->axis[ai].size - 1) + minOff[1 + 2 * ai];
    if (-1 == maxOff[0 + 2 * ai]) {
      max[ai] = min[ai] + maxOff[1 + 2 * ai];
    } else {
      max[ai] = maxOff[0 + 2 * ai] * (nin->axis[ai].size - 1) + maxOff[1 + 2 * ai];
    }
    /*
    fprintf(stderr, "%s: ai %2d: min = %4d, max = %4d\n",
            me, ai, min[ai], mai[ai]);
    */
  }

  nout = nrrdNew();
  airMopAdd(mop, nout, (airMopper)nrrdNuke, airMopAlways);

  if (nrrdPad_nva(nout, nin, min, max, bb, padVal)) {
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: error padding nrrd:\n%s", me, err);
    airMopError(mop);
    return 1;
  }

  SAVE(out, nout, NULL);

  airMopOkay(mop);
  return 0;
}

UNRRDU_CMD(pad, INFO);
