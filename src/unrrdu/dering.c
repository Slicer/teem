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

#define INFO "Ring removal for CT"
static const char *_unrrdu_deringInfoL = (INFO
                                          ". Should be considered a work-in-progress. ");

static int
unrrdu_deringMain(int argc, const char **argv, const char *me, hestParm *hparm) {
  hestOpt *opt = NULL;
  char *out, *err;
  Nrrd *nin, *nout;
  airArray *mop;
  int pret;

  Nrrd *nmask;
  NrrdDeringContext *drc;
  double center[2], radScale, clampPerc[2], backval;
  int verbose, linterp, vertSeam;
  unsigned int thetaNum;
  NrrdKernelSpec *rkspec, *tkspec;

  /* HEY is this needed? (to display -rk and -tk kernels) */
  hparm->elideSingleOtherDefault = AIR_FALSE;

  hestOptAdd_2_Double(&opt, "c,center", "x y", center, NULL,
                      "center of rings, in index space of fastest two axes");
  hestOptAdd_1_Int(&opt, "v,verbose", "v", &verbose, "0", "verbosity level");
  hestOptAdd_1_Bool(&opt, "li,linterp", "bool", &linterp, "false",
                    "whether to use linear interpolation during polar transform");
  hestOptAdd_1_Bool(&opt, "vs,vertseam", "bool", &vertSeam, "false",
                    "whether to dering left and right sides separately "
                    "(requires an even value for -tn thetanum)");
  hestOptAdd_1_UInt(&opt, "tn,thetanum", "# smpls", &thetaNum, "20",
                    "# of theta samples");
  hestOptAdd_1_Double(&opt, "rs,radscale", "scale", &radScale, "1.0",
                      "scaling on radius in polar transform");
  hestOptAdd_1_Other(&opt, "rk,radiuskernel", "kern", &rkspec, "gauss:3,4",
                     "kernel for high-pass filtering along radial direction",
                     nrrdHestKernelSpec);
  hestOptAdd_1_Other(&opt, "tk,thetakernel", "kern", &tkspec, "box",
                     "kernel for blurring along theta direction.", nrrdHestKernelSpec);
  hestOptAdd_2_Double(&opt, "cp,clampperc", "lo hi", clampPerc, "0.0 0.0",
                      "when clamping values as part of ring estimation, the "
                      "clamping range is set to exclude this percent of values "
                      "from the low and high end of the data range");
  hestOptAdd_1_Other(&opt, "m,mask", "mask", &nmask, "",
                     "optional: after deringing, output undergoes a lerp, "
                     "parameterized by this array, from the background value "
                     "(via \"-b\") where mask=0 to the original deringing "
                     "output where mask=1.  This lerp is effectively the same "
                     "as a \"unu 3op lerp\", so this should either be match the "
                     "input in size, or match its slices along the slowest axis.",
                     nrrdHestNrrd);
  hestOptAdd_1_Double(&opt, "b,back", "val", &backval, "0.0",
                      "when using a mask (\"-m\"), the background value to "
                      "lerp with.");
  OPT_ADD_NIN(nin, "input nrrd");
  OPT_ADD_NOUT(out, "output nrrd");

  mop = airMopNew();
  airMopAdd(mop, opt, hestOptFree_vp, airMopAlways);

  USAGE_OR_PARSE(_unrrdu_deringInfoL);
  airMopAdd(mop, opt, (airMopper)hestParseFree, airMopAlways);

  nout = nrrdNew();
  airMopAdd(mop, nout, (airMopper)nrrdNuke, airMopAlways);

  if (nmask) {
    if (!(2 == nmask->dim && nrrdTypeBlock != nmask->type
          && nmask->axis[0].size == nin->axis[0].size
          && nmask->axis[1].size == nin->axis[1].size)) {
      fprintf(stderr, "%s: given mask not 2-D %u-by-%u array of scalar type", me,
              AIR_UINT(nin->axis[0].size), AIR_UINT(nin->axis[1].size));
      airMopError(mop);
      return 1;
    }
  }

  drc = nrrdDeringContextNew();
  airMopAdd(mop, drc, (airMopper)nrrdDeringContextNix, airMopAlways);
  if (nrrdDeringVerboseSet(drc, verbose) || nrrdDeringLinearInterpSet(drc, linterp)
      || nrrdDeringVerticalSeamSet(drc, vertSeam) || nrrdDeringInputSet(drc, nin)
      || nrrdDeringCenterSet(drc, center[0], center[1])
      || nrrdDeringRadiusScaleSet(drc, radScale) || nrrdDeringThetaNumSet(drc, thetaNum)
      || nrrdDeringRadialKernelSet(drc, rkspec->kernel, rkspec->parm)
      || nrrdDeringThetaKernelSet(drc, tkspec->kernel, tkspec->parm)
      || nrrdDeringClampPercSet(drc, clampPerc[0], clampPerc[1])
      || nrrdDeringExecute(drc, nout)) {
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: error deringing:\n%s", me, err);
    airMopError(mop);
    return 1;
  }

  if (nmask) {
    NrrdIter *nitout, *nitmask, *nitback;
    Nrrd *ntmp;
    nitout = nrrdIterNew();
    airMopAdd(mop, nitout, (airMopper)nrrdIterNix, airMopAlways);
    nitmask = nrrdIterNew();
    airMopAdd(mop, nitmask, (airMopper)nrrdIterNix, airMopAlways);
    nitback = nrrdIterNew();
    airMopAdd(mop, nitback, (airMopper)nrrdIterNix, airMopAlways);
    nrrdIterSetValue(nitback, backval);

    ntmp = nrrdNew();
    airMopAdd(mop, ntmp, (airMopper)nrrdNuke, airMopAlways);

    nrrdIterSetNrrd(nitout, nout);
    nrrdIterSetNrrd(nitmask, nmask);
    if (nrrdArithIterTernaryOpSelect(ntmp, nrrdTernaryOpLerp, nitmask, nitback, nitout,
                                     2)
        || nrrdCopy(nout, ntmp)) {
      airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
      fprintf(stderr, "%s: error masking:\n%s", me, err);
      airMopError(mop);
      return 1;
    }
  }

  SAVE(out, nout, NULL);

  airMopOkay(mop);
  return 0;
}

UNRRDU_CMD_HIDE(dering, INFO);
