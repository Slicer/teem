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

#include "bane.h"
#include "privateBane.h"

#define SCAT_INFO "Make V-G and V-H scatterplots"
static const char *_baneGkms_scatInfoL
  = (SCAT_INFO ". These provide a quick way to inspect a histogram volume, in order to "
               "verify that the derivative inclusion ranges were appropriate, and to "
               "get an initial sense of what sorts of boundaries were present in the "
               "original volume.");
static int /* Biff: 1 */
baneGkms_scatMain(int argc, const char **argv, const char *me, hestParm *hparm) {
  hestOpt *opt = NULL;
  char *out[2], *perr;
  Nrrd *hvol, *nvgRaw, *nvhRaw, *nvgQuant, *nvhQuant;
  NrrdRange *vgRange, *vhRange;
  airArray *mop;
  int pret, E;
  double _gamma;

  hestOptAdd_1_Double(&opt, "g", "gamma", &_gamma, "1.0",
                      "gamma used to brighten/darken scatterplots. "
                      "gamma > 1.0 brightens; gamma < 1.0 darkens. "
                      "Negative gammas invert values (like in xv). ");
  hestOptAdd_1_Other(&opt, "i", "hvolIn", &hvol, NULL,
                     "input histogram volume (from \"gkms hvol\")", nrrdHestNrrd);
  hestOptAdd_2_String(&opt, "o", "vgOut vhOut", out, NULL,
                      "Filenames to use for two output scatterplots, (gradient "
                      "magnitude versus value, and 2nd derivative versus value); "
                      "can use PGM or PNG format");

  mop = airMopNew();
  airMopAdd(mop, opt, (airMopper)hestOptFree, airMopAlways);
  USAGE_PARSE(_baneGkms_scatInfoL);
  airMopAdd(mop, opt, (airMopper)hestParseFree, airMopAlways);

  nvgRaw = nrrdNew();
  nvhRaw = nrrdNew();
  nvgQuant = nrrdNew();
  nvhQuant = nrrdNew();
  airMopAdd(mop, nvgRaw, (airMopper)nrrdNuke, airMopAlways);
  airMopAdd(mop, nvhRaw, (airMopper)nrrdNuke, airMopAlways);
  airMopAdd(mop, nvgQuant, (airMopper)nrrdNuke, airMopAlways);
  airMopAdd(mop, nvhQuant, (airMopper)nrrdNuke, airMopAlways);
  if (baneRawScatterplots(nvgRaw, nvhRaw, hvol, AIR_TRUE)) {
    biffAddf(BANE, "%s: trouble creating raw scatterplots", me);
    airMopError(mop);
    return 1;
  }
  vgRange = nrrdRangeNewSet(nvgRaw, nrrdBlind8BitRangeFalse);
  vhRange = nrrdRangeNewSet(nvhRaw, nrrdBlind8BitRangeFalse);
  airMopAdd(mop, vgRange, (airMopper)nrrdRangeNix, airMopAlways);
  airMopAdd(mop, vhRange, (airMopper)nrrdRangeNix, airMopAlways);
  E = 0;
  if (!E) E |= nrrdArithGamma(nvgRaw, nvgRaw, vgRange, _gamma);
  if (!E) E |= nrrdArithGamma(nvhRaw, nvhRaw, vhRange, _gamma);
  if (!E) E |= nrrdQuantize(nvgQuant, nvgRaw, vgRange, 8);
  if (!E) E |= nrrdQuantize(nvhQuant, nvhRaw, vhRange, 8);
  if (E) {
    biffMovef(BANE, NRRD, "%s: trouble doing gamma or quantization", me);
    airMopError(mop);
    return 1;
  }

  if (!E) E |= nrrdSave(out[0], nvgQuant, NULL);
  if (!E) E |= nrrdSave(out[1], nvhQuant, NULL);
  if (E) {
    biffMovef(BANE, NRRD, "%s: trouble saving scatterplot images", me);
    airMopError(mop);
    return 1;
  }

  airMopOkay(mop);
  return 0;
}
BANE_GKMS_CMD(scat, SCAT_INFO);
