/*
  teem: Gordon Kindlmann's research software
  Copyright (C) 2002, 2001, 2000, 1999, 1998 University of Utah

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "bane.h"
#include "privateBane.h"

#define SCAT_INFO "Make V-G and V-H scatterplots"
char *_baneGkms_scatInfoL =
  (SCAT_INFO
   ". These provide a quick way to inspect a histogram volume, in order to "
   "verify that the derivative inclusion ranges were appropriate, and to "
   "get an initial sense of what sorts of boundaries were present in the "
   "original volume.");
int
baneGkms_scatMain(int argc, char **argv, char *me, hestParm *hparm) {
  hestOpt *opt = NULL;
  char *out[2], *perr, err[AIR_STRLEN_MED];
  NrrdIO *nio;
  Nrrd *hvol, *nvgRaw, *nvhRaw, *nvgQuant, *nvhQuant;
  airArray *mop;
  int pret, E;
  double gamma;

  hestOptAdd(&opt, "g", "gamma", airTypeDouble, 1, 1, &gamma, "1.4",
	     "gamma used to brighten/darken scatterplots. "
	     "gamma > 1.0 brightens; gamma < 1.0 darkens. "
	     "Negative gammas invert values (like in xv). ");
  hestOptAdd(&opt, "i", "hvolIn", airTypeOther, 1, 1, &hvol, NULL,
	     "input histogram volume (from \"gkms hvol\")",
             NULL, NULL, nrrdHestNrrd);
  hestOptAdd(&opt, "o", "vgOut vhOut", airTypeString, 2, 2, out, NULL,
	     "Filenames to use for two output scatterplots, (gradient "
	     "magnitude versus value, and 2nd derivative versus value), "
	     "saved as PGM images");

  mop = airMopInit();
  airMopAdd(mop, opt, (airMopper)hestOptFree, airMopAlways);
  USAGE(_baneGkms_scatInfoL);
  PARSE();
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
    sprintf(err, "%s: trouble creating raw scatterplots", me);
    biffAdd(BANE, err); airMopError(mop); return 1;
  }
  E = 0;
  nrrdMinMaxSet(nvgRaw);
  if (!E) E |= nrrdArithGamma(nvgRaw, nvgRaw, gamma, nvgRaw->min, nvgRaw->max);
  nrrdMinMaxSet(nvhRaw);
  if (!E) E |= nrrdArithGamma(nvhRaw, nvhRaw, gamma, nvhRaw->min, nvhRaw->max);
  if (!E) E |= nrrdQuantize(nvgQuant, nvgRaw, 8);
  if (!E) E |= nrrdQuantize(nvhQuant, nvhRaw, 8);
  if (E) {
    sprintf(err, "%s: trouble doing gamma or quantization", me);
    biffMove(BANE, err, NRRD); airMopError(mop); return 1;
  }

  nio = nrrdIONew();
  airMopAdd(mop, nio, (airMopper)nrrdIONix, airMopAlways);
  nio->format = nrrdFormatPNM;
  if (!E) E |= nrrdSave(out[0], nvgQuant, nio);
  nrrdIOReset(nio);
  nio->format = nrrdFormatPNM;
  if (!E) E |= nrrdSave(out[1], nvhQuant, nio);
  if (E) {
    sprintf(err, "%s: trouble saving scatterplot images", me);
    biffMove(BANE, err, NRRD); airMopError(mop); return 1;
  }

  airMopOkay(mop);
  return 0;
}
BANE_GKMS_CMD(scat, SCAT_INFO);

