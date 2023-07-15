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

#define OPAC_INFO "Generate opacity functions"
static const char *_baneGkms_opacInfoL
  = (OPAC_INFO ". Takes information from an \"info\" file and from a \"boundary "
               "emphasis function\" to generate 1D or 2D (depending on info file) "
               "opacity functions. ");
static int /* Biff: 1 */
baneGkms_opacMain(int argc, const char **argv, const char *me, hestParm *hparm) {
  hestOpt *opt = NULL;
  char *outS, *perr, *befS;
  Nrrd *ninfo, *nbef, *nout, *nmax, *npos, *nopac;
  airArray *mop;
  int pret, radius, idim;
  float sigma, gthrInfo[2], gthresh;

  hestOptAdd_1_Other(&opt, "b", "bef", &nbef, "1,1,0,1",
                     "boundary emphasis function mapping from \"position\" to "
                     "opacity. Can be either:\n "
                     "\b\bo filename of nrrd suitable for \"unu imap\", or:\n "
                     "\b\bo comma-separated list of four floats, with no spaces: "
                     "\"s,w,c,a\", where\n "
                     "s = shape of function, between 0.0 for box and "
                     "1.0 for tent\n "
                     "w = full-width half-max of function support\n "
                     "c = where to center function support\n "
                     "a = maximum opacity\n "
                     "If all goes well, the units for \"w\" and \"c\" are voxels.",
                     baneGkmsHestBEF);
  hestOptAdd_1_Float(&opt, "s", "sigma", &sigma, "nan",
                     "scaling in position calculation, accounts for thickness "
                     "of transition region between materials. Lower sigmas lead to "
                     "wider peaks in opacity function. "
                     "Calculated automatically by default.");
  hestOptAdd_1_Other(&opt, "g", "gthresh", gthrInfo, "x0.04",
                     "minimum significant gradient magnitude.  Can be given "
                     "in two different ways:\n "
                     "\b\bo \"<float>\": specify gthresh as <float> exactly.\n "
                     "\b\bo \"x<float>\": gthresh is a scaling, by <float>, of "
                     "the maximum gradient magnitude in the info file.",
                     baneGkmsHestGthresh);
  hestOptAdd_1_Int(&opt, "r", "radius", &radius, "0",
                   "radius of median filtering to apply to opacity function, "
                   "use \"0\" to signify no median filtering");
  hestOptAdd_1_String(&opt, "m", "befOut", &befS, "",
                      "if boundary emphasis function given via \"-b\" "
                      "is in the \"s,w,c,a\" form, then save out the "
                      "corresponding nrrd to <befOut>, suitable for use in this "
                      "command or \"unu imap\"");
  hestOptAdd_1_Other(&opt, "i", "infoIn", &ninfo, NULL,
                     "input info file (from \"gkms info\")", nrrdHestNrrd);
  hestOptAdd_1_String(&opt, "o", "opacOut", &outS, NULL,
                      "output 1D or 2D opacity function");

  mop = airMopNew();
  airMopAdd(mop, opt, (airMopper)hestOptFree, airMopAlways);
  USAGE_PARSE(_baneGkms_opacInfoL);
  airMopAdd(mop, opt, (airMopper)hestParseFree, airMopAlways);
  airMopAdd(mop, nmax = nrrdNew(), (airMopper)nrrdNuke, airMopAlways);
  airMopAdd(mop, npos = nrrdNew(), (airMopper)nrrdNuke, airMopAlways);
  airMopAdd(mop, nopac = nrrdNew(), (airMopper)nrrdNuke, airMopAlways);
  airMopAdd(mop, nout = nrrdNew(), (airMopper)nrrdNuke, airMopAlways);

  if (baneInfoCheck(ninfo, AIR_FALSE)) {
    biffAddf(BANE, "%s: didn't get a valid histogram info file", me);
    airMopError(mop);
    return 1;
  }
  idim = ninfo->dim - 1;
  if (nbef->ptr && airStrlen(befS)) {
    if (nrrdSave(befS, nbef, NULL)) {
      biffMovef(BANE, NRRD, "%s: trouble saving boundary emphasis", me);
      airMopError(mop);
      return 1;
    }
  }
  if (!AIR_EXISTS(sigma)) {
    if (baneSigmaCalc(&sigma, ninfo)) {
      biffAddf(BANE, "%s: trouble calculating sigma", me);
      airMopError(mop);
      return 1;
    }
    fprintf(stderr, "%s: calculated sigma = %g\n", me, sigma);
  }
  if (0 == gthrInfo[0]) {
    gthresh = gthrInfo[1];
  } else {
    if (2 == idim) {
      gthresh = AIR_FLOAT(gthrInfo[1] * ninfo->axis[2].max);
    } else {
      if (nrrdProject(nmax, ninfo, 1, nrrdMeasureMax, nrrdTypeDefault)) {
        biffAddf(BANE, "%s: couldn't do max projection of 1D histo-info", me);
        airMopError(mop);
        return 1;
      }
      gthresh = gthrInfo[1] * nrrdFLookup[nmax->type](nmax->data, 0);
    }
    fprintf(stderr, "%s: calculated gthresh = %g\n", me, gthresh);
  }
  if (banePosCalc(npos, sigma, gthresh, ninfo) || baneOpacCalc(nopac, nbef, npos)) {
    biffAddf(BANE, "%s: trouble calculating position or opacity", me);
    airMopError(mop);
    return 1;
  }
  if (radius) {
    if (nrrdCheapMedian(nout, nopac, AIR_TRUE, AIR_FALSE, radius, 1.0, 2048)) {
      biffMovef(BANE, NRRD, "%s: error in median filtering", me);
      airMopError(mop);
      return 1;
    }
  } else {
    if (nrrdCopy(nout, nopac)) {
      biffMovef(BANE, NRRD, "%s: error in copying output", me);
      airMopError(mop);
      return 1;
    }
  }
  if (nrrdSave(outS, nout, NULL)) {
    biffMovef(BANE, NRRD, "%s: trouble saving opacity function", me);
    airMopError(mop);
    return 1;
  }

  airMopOkay(mop);
  return 0;
}
BANE_GKMS_CMD(opac, OPAC_INFO);
