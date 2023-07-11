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

#define INFO "Quantize values to 8, 16, or 32 bits"
static const char *_unrrdu_quantizeInfoL
  = (INFO ". Input values can be fixed point (e.g. quantizing ushorts down to "
          "uchars) or floating point.  Values are clamped to the min and max before "
          "they are quantized, so there is no risk of getting 255 where you expect 0 "
          "(with unsigned char output, for example).  The min and max can be specified "
          "explicitly (as a regular number), or in terms of percentiles (a number "
          "suffixed with \"" NRRD_MINMAX_PERC_SUFF "\", no space in between). "
          "This does only linear quantization. "
          "See also \"unu convert\", \"unu 2op x\", "
          "and \"unu 3op clamp\".\n "
          "* Uses nrrdQuantize");

static int
unrrdu_quantizeMain(int argc, const char **argv, const char *me, hestParm *hparm) {
  hestOpt *opt = NULL;
  char *out, *err;
  Nrrd *nin, *nout;
  char *minStr, *maxStr, *gammaS;
  int pret, zeroCenter, blind8BitRange, srgb, E = 0;
  unsigned int bits, hbins, srgbIdx;
  double gamma;
  NrrdRange *range;
  airArray *mop;
  NrrdIoState *nio = NULL;

  hestOptAdd_1_Other(&opt, "b,bits", "bits", &bits, NULL,
                     "Number of bits to quantize down to; determines the type "
                     "of the output nrrd:\n "
                     "\b\bo \"8\": unsigned char\n "
                     "\b\bo \"16\": unsigned short\n "
                     "\b\bo \"32\": unsigned int",
                     &unrrduHestBitsCB);
  hestOptAdd_1_String(&opt, "min,minimum", "value", &minStr, "nan",
                      "The value to map to zero, given explicitly as a regular number, "
                      "*or*, if the number is given with a \"" NRRD_MINMAX_PERC_SUFF
                      "\" suffix, this "
                      "minimum is specified in terms of the percentage of samples in "
                      "input that are lower. "
                      "\"0" NRRD_MINMAX_PERC_SUFF "\" means the "
                      "lowest input value is used, "
                      "\"1" NRRD_MINMAX_PERC_SUFF "\" means that the "
                      "1% of the lowest values are all mapped to zero. "
                      "By default (not using this option), the lowest input value is "
                      "used.");
  hestOptAdd_1_String(&opt, "max,maximum", "value", &maxStr, "nan",
                      "The value to map to the highest unsigned integral value, given "
                      "explicitly as a regular number, "
                      "*or*, if the number is given with "
                      "a \"" NRRD_MINMAX_PERC_SUFF "\" suffix, "
                      "this maximum is specified "
                      "in terms of the percentage of samples in input that are higher. "
                      "\"0" NRRD_MINMAX_PERC_SUFF "\" means the highest input value is "
                      "used, which is also the default "
                      "behavior (same as not using this option).");
  /* NOTE -zc shared with unrrdu histax, histo, quantize */
  hestOptAdd_Flag(&opt, "zc,zero-center", &zeroCenter,
                  "if used, percentile-based min,max determine a zero-centered "
                  "range (rather than treating min and max independently), which "
                  "may help process signed values in an expected way.");
  hestOptAdd_1_String(&opt, "g,gamma", "gamma", &gammaS, "1.0",
                      "gamma > 1.0 brightens; gamma < 1.0 darkens. "
                      "Negative gammas invert values. Or, can be the string "
                      "\"srgb\" to apply the roughly 2.2 gamma associated "
                      "with sRGB (see https://en.wikipedia.org/wiki/SRGB). ");
  srgbIdx = /* HEY copied from overrgb.c */
    hestOptAdd_1_Enum(&opt, "srgb", "intent", &srgb, "none",
                      /* the default is "none" for backwards compatibility: until now
                         Teem's support of PNG hasn't handled the sRGB intent, so
                         we shouldn't start using it without being asked */
                      "If saving to PNG (when supported), how to set the rendering "
                      "intent in the sRGB chunk of the PNG file format. Can be "
                      "absolute, relative, perceptual, saturation, or none. This is "
                      "independent of using \"srgb\" as the -g gamma",
                      nrrdFormatPNGsRGBIntent);
  hestOptAdd_1_UInt(&opt, "hb,bins", "bins", &hbins, "5000",
                    "number of bins in histogram of values, for determining min "
                    "or max by percentiles.  This has to be large enough so that "
                    "any errant very high or very low values do not compress the "
                    "interesting part of the histogram to an inscrutably small "
                    "number of bins.");
  hestOptAdd_1_Bool(&opt, "blind8", "bool", &blind8BitRange,
                    nrrdStateBlind8BitRange ? "true" : "false",
                    "if not using \"-min\" or \"-max\", whether to know "
                    "the range of 8-bit data blindly (uchar is always [0,255], "
                    "signed char is [-128,127])");
  OPT_ADD_NIN(nin, "input nrrd");
  OPT_ADD_NOUT(out, "output nrrd");

  mop = airMopNew();
  airMopAdd(mop, opt, hestOptFree_vp, airMopAlways);

  USAGE_OR_PARSE(_unrrdu_quantizeInfoL);
  airMopAdd(mop, opt, (airMopper)hestParseFree, airMopAlways);

  /* HEY copied from overrgb */
  if (!(!strcmp(gammaS, "srgb") || 1 == sscanf(gammaS, "%lf", &gamma))) {
    fprintf(stderr,
            "%s: gamma \"%s\" neither \"srgb\" nor "
            "parseable as double",
            me, gammaS);
    airMopError(mop);
    return 1;
  }

  range = nrrdRangeNew(AIR_NAN, AIR_NAN);
  airMopAdd(mop, range, (airMopper)nrrdRangeNix, airMopAlways);
  nout = nrrdNew();
  airMopAdd(mop, nout, (airMopper)nrrdNuke, airMopAlways);
  if (nrrdRangePercentileFromStringSet(range, nin, minStr, maxStr, zeroCenter, hbins,
                                       blind8BitRange)) {
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: error learning range:\n%s", me, err);
    airMopError(mop);
    return 1;
  }
  if (!strcmp(gammaS, "srgb")) {
    E = nrrdArithSRGBGamma(nin, nin, range, AIR_TRUE);
  } else if (1 != gamma) {
    E = nrrdArithGamma(nin, nin, range, gamma);
  }
  if (E) {
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: error going gamma:\n%s", me, err);
    airMopError(mop);
    return 1;
  }
  if (nrrdQuantize(nout, nin, range, bits)) {
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: error quantizing:\n%s", me, err);
    airMopError(mop);
    return 1;
  }

  if (hestSourceUser == opt[srgbIdx].source) {
    /* HEY copied from overrgb.c */
    nio = nrrdIoStateNew();
    airMopAdd(mop, nio, (airMopper)nrrdIoStateNix, airMopAlways);
    nio->PNGsRGBIntentKnown = AIR_TRUE;
    nio->PNGsRGBIntent = srgb; /* even if it is nrrdFormatPNGsRGBIntentNone;
                                  that's handled by the writer */
  }

  SAVE(out, nout, nio);

  airMopOkay(mop);
  return 0;
}

UNRRDU_CMD(quantize, INFO);
