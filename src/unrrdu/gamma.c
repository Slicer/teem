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

#define INFO "Brighten or darken values with a gamma"
static const char *_unrrdu_gammaInfoL
  = (INFO ". Just as in xv, the gamma value here is actually the "
          "reciprocal of the exponent actually used to transform "
          "the values. Can also do the non-linear transforms used "
          "in the sRGB standard (see https://en.wikipedia.org/wiki/SRGB)\n "
          "* Uses nrrdArithGamma or nrrdArithGammaSRGB");

static int
unrrdu_gammaMain(int argc, const char **argv, const char *me, hestParm *hparm) {
  hestOpt *opt = NULL;
  char *out, *err;
  Nrrd *nin, *nout;
  char *GammaS;
  double min, max, Gamma;
  airArray *mop;
  int pret, blind8BitRange, srgb, forward, E;
  NrrdRange *range;

  hestOptAdd_1_String(&opt, "g,gamma", "gamma", &GammaS, NULL,
                      "gamma > 1.0 brightens; gamma < 1.0 darkens. "
                      "Negative gammas invert values (like in xv). "
                      "Or, can used \"srgb\" for ~2.2 gamma of sRGB encoding, or "
                      "\"1/srgb\" for ~0.455 gamma of inverse sRGB encoding");
  hestOptAdd_1_Double(&opt, "min,minimum", "value", &min, "nan",
                      "Value to implicitly map to 0.0 prior to calling pow(). "
                      "Defaults to lowest value found in input nrrd.");
  hestOptAdd_1_Double(&opt, "max,maximum", "value", &max, "nan",
                      "Value to implicitly map to 1.0 prior to calling pow(). "
                      "Defaults to highest value found in input nrrd.");
  hestOptAdd_1_Bool(&opt, "blind8", "bool", &blind8BitRange,
                    nrrdStateBlind8BitRange ? "true" : "false",
                    "Whether to know the range of 8-bit data blindly "
                    "(uchar is always [0,255], signed char is [-128,127]).");
  OPT_ADD_NIN(nin, "input nrrd");
  OPT_ADD_NOUT(out, "output nrrd");

  mop = airMopNew();
  airMopAdd(mop, opt, hestOptFree_vp, airMopAlways);

  USAGE_OR_PARSE(_unrrdu_gammaInfoL);
  airMopAdd(mop, opt, (airMopper)hestParseFree, airMopAlways);

  if (!strcmp(GammaS, "srgb")) {
    srgb = AIR_TRUE;
    forward = AIR_TRUE;
  } else if (!strcmp(GammaS, "1/srgb")) {
    srgb = AIR_TRUE;
    forward = AIR_FALSE;
  } else {
    srgb = AIR_FALSE;
    forward = AIR_FALSE;
    if (1 != airSingleSscanf(GammaS, "%lf", &Gamma)) {
      fprintf(stderr,
              "%s: couldn't parse gamma \"%s\" as double, and wasn't either "
              "\"srgb\" or \"1/srgb\"\n",
              me, GammaS);
      airMopError(mop);
      return 1;
    }
  }
  nout = nrrdNew();
  airMopAdd(mop, nout, (airMopper)nrrdNuke, airMopAlways);

  range = nrrdRangeNew(min, max);
  airMopAdd(mop, range, (airMopper)nrrdRangeNix, airMopAlways);
  nrrdRangeSafeSet(range, nin, blind8BitRange);
  if (srgb) {
    E = nrrdArithSRGBGamma(nout, nin, range, forward);
  } else {
    E = nrrdArithGamma(nout, nin, range, Gamma);
  }
  if (E) {
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: error doing gamma:\n%s", me, err);
    airMopError(mop);
    return 1;
  }
  SAVE(out, nout, NULL);

  airMopOkay(mop);
  return 0;
}

UNRRDU_CMD(gamma, INFO);
