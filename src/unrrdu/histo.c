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

#define INFO "Create 1-D histogram of values in a nrrd"
static const char *_unrrdu_histoInfoL
  = (INFO ". Can explicitly set bounds of histogram domain or can learn these "
          "from the data.\n "
          "* Uses nrrdHisto");

static int
unrrdu_histoMain(int argc, const char **argv, const char *me, hestParm *hparm) {
  hestOpt *opt = NULL;
  char *out, *err;
  Nrrd *nin, *nout, *nwght;
  char *minStr, *maxStr;
  int type, pret, zeroCenter, blind8BitRange;
  unsigned int bins;
  NrrdRange *range;
  airArray *mop;

  hestOptAdd_1_UInt(&opt, "b,bins", "num", &bins, NULL, "# of bins in histogram");
  hestOptAdd_1_Other(&opt, "w,weight", "nweight", &nwght, "",
                     "how to weigh contributions to histogram.  By default "
                     "(not using this option), the increment is one bin count per "
                     "sample, but by giving a nrrd, the value in the nrrd at the "
                     "corresponding location will be the bin count increment ",
                     nrrdHestNrrd);
  hestOptAdd_1_String(&opt, "min,minimum", "value", &minStr, "nan",
                      "Value at low end of histogram, given explicitly as a "
                      "regular number, "
                      "*or*, if the number is given with a \"" NRRD_MINMAX_PERC_SUFF
                      "\" suffix, this "
                      "minimum is specified in terms of the percentage of samples in "
                      "input that are lower. "
                      "By default (not using this option), the lowest value "
                      "found in input nrrd.");
  hestOptAdd_1_String(&opt, "max,maximum", "value", &maxStr, "nan",
                      "Value at high end of histogram, given "
                      "explicitly as a regular number, "
                      "*or*, if the number is given with "
                      "a \"" NRRD_MINMAX_PERC_SUFF "\" suffix, "
                      "this maximum is specified "
                      "in terms of the percentage of samples in input that are higher. "
                      "Defaults to highest value found in input nrrd.");
  /* NOTE -zc shared with unrrdu histax, histo, quantize */
  hestOptAdd_Flag(&opt, "zc,zero-center", &zeroCenter,
                  "if used, percentile-based min,max determine a zero-centered "
                  "range (rather than treating min and max independently), which "
                  "may help process signed values in an expected way.");
  hestOptAdd_1_Bool(&opt, "blind8", "bool", &blind8BitRange,
                    nrrdStateBlind8BitRange ? "true" : "false",
                    "Whether to know the range of 8-bit data blindly "
                    "(uchar is always [0,255], signed char is [-128,127]).");
  OPT_ADD_TYPE(type, "type to use for bins in output histogram", "uint");
  OPT_ADD_NIN(nin, "input nrrd");
  OPT_ADD_NOUT(out, "output nrrd");

  mop = airMopNew();
  airMopAdd(mop, opt, hestOptFree_vp, airMopAlways);

  USAGE_OR_PARSE(_unrrdu_histoInfoL);
  airMopAdd(mop, opt, (airMopper)hestParseFree, airMopAlways);

  range = nrrdRangeNew(AIR_NAN, AIR_NAN);
  airMopAdd(mop, range, (airMopper)nrrdRangeNix, airMopAlways);
  nout = nrrdNew();
  airMopAdd(mop, nout, (airMopper)nrrdNuke, airMopAlways);
  if (nrrdRangePercentileFromStringSet(range, nin, minStr, maxStr, zeroCenter,
                                       10 * bins /* HEY magic */, blind8BitRange)
      || nrrdHisto(nout, nin, range, nwght, bins, type)) {
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: error with range or quantizing:\n%s", me, err);
    airMopError(mop);
    return 1;
  }

  SAVE(out, nout, NULL);

  airMopOkay(mop);
  return 0;
}

UNRRDU_CMD(histo, INFO);
