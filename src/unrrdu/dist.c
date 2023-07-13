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

#define INFO "Euclidean distance transform"
static const char *_unrrdu_distInfoL
  = (INFO ". Based on \"Distance Transforms of Sampled Functions\" by "
          "Pedro F. Felzenszwalb and Daniel P. Huttenlocher, "
          "Cornell Computing and Information Science TR2004-1963. "
          "This function first thresholds at the specified value and then "
          "does the distance transform of the resulting binary image. "
          "The signed distance (negative values inside object) is also available. "
          "Distances between non-isotropic samples are handled correctly.\n "
          "* Uses nrrdDistanceL2 or nrrdDistanceL2Signed");

static int
unrrdu_distMain(int argc, const char **argv, const char *me, hestParm *hparm) {
  hestOpt *opt = NULL;
  char *out, *err;
  Nrrd *nin, *nout;
  int pret;

  int E, typeOut, invert, sign;
  double thresh, bias;
  airArray *mop;

  hestOptAdd_1_Double(&opt, "th,thresh", "val", &thresh, NULL,
                      "threshold value to separate inside from outside");
  hestOptAdd_1_Double(&opt, "b,bias", "val", &bias, "0.0",
                      "if non-zero, bias the distance transform by this amount "
                      "times the difference in value from the threshold");
  hestOptAdd_1_Enum(&opt, "t,type", "type", &typeOut, "float", "type to save output in",
                    nrrdType);
  hestOptAdd_Flag(&opt, "sgn", &sign,
                  "also compute signed (negative) distances inside objects, "
                  "instead of leaving them as zero");
  hestOptAdd_Flag(&opt, "inv", &invert,
                  "values *below* threshold are considered interior to object. "
                  "By default (not using this option), values above threshold "
                  "are considered interior. ");
  OPT_ADD_NIN(nin, "input nrrd");
  OPT_ADD_NOUT(out, "output nrrd");

  mop = airMopNew();
  airMopAdd(mop, opt, hestOptFree_vp, airMopAlways);

  USAGE_OR_PARSE(_unrrdu_distInfoL);
  airMopAdd(mop, opt, (airMopper)hestParseFree, airMopAlways);

  nout = nrrdNew();
  airMopAdd(mop, nout, (airMopper)nrrdNuke, airMopAlways);

  if (bias && sign) {
    fprintf(stderr,
            "%s: sorry, signed and biased transform not "
            "yet implemented\n",
            me);
    airMopError(mop);
    return 1;
  }

  if (sign) {
    E = nrrdDistanceL2Signed(nout, nin, typeOut, NULL, thresh, !invert);
  } else {
    if (bias) {
      E = nrrdDistanceL2Biased(nout, nin, typeOut, NULL, thresh, bias, !invert);
    } else {
      E = nrrdDistanceL2(nout, nin, typeOut, NULL, thresh, !invert);
    }
  }
  if (E) {
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: error doing distance transform:\n%s", me, err);
    airMopError(mop);
    return 1;
  }

  SAVE(out, nout, NULL);

  airMopOkay(mop);
  return 0;
}

UNRRDU_CMD(dist, INFO);
