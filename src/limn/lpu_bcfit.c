/*
  Teem: Tools to process and visualize scientific data and images             .
  Copyright (C) 2009--2020  University of Chicago
  Copyright (C) 2008, 2007, 2006, 2005  Gordon Kindlmann
  Copyright (C) 2004, 2003, 2002, 2001, 2000, 1999, 1998  University of Utah

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public License
  (LGPL) as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  The terms of redistributing and/or modifying this software also
  include exceptions to the LGPL that facilitate static linking.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with this library; if not, write to Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "limn.h"
#include "privateLimn.h"

#define INFO "Fit Bezier cubic spline to points"
static const char *myinfo =
(INFO
 ". ");

int
limnpu_bcfitMain(int argc, const char **argv, const char *me,
                 hestParm *hparm) {
  hestOpt *hopt = NULL;
  char *err, *perr;
  airArray *mop;
  int pret;

  Nrrd *_nin, *nin;
  double *xy, alpha[2], vv0[2], tt1[2], tt2[2], vv3[2],
    deltaMin, deltaDone, distMin, distDone, time0, time1;
  unsigned int pNum, iterMax, iterDone, distIdx;
  int verbose, synth;
  char *synthOut;
  limnCBFitState cbfs;

  hestOptAdd(&hopt, "i", "input", airTypeOther, 1, 1, &_nin, NULL,
             "input xy points",
             NULL, NULL, nrrdHestNrrd);
  hestOptAdd(&hopt, "v", "verbose", airTypeInt, 1, 1, &verbose, "1",
             "verbosity level");
  hestOptAdd(&hopt, "s", "synth", airTypeInt, 0, 0, &synth, NULL,
             "synthesize xy points from control points");
  hestOptAdd(&hopt, "so", "synth out", airTypeString, 1, 1, &synthOut, "",
             "if non-empty, filename in which to save synthesized xy pts");
  hestOptAdd(&hopt, "im", "max", airTypeUInt, 1, 1, &iterMax, "1",
             "(if non-zero) max # iterations to run");
  hestOptAdd(&hopt, "deltam", "delta", airTypeDouble, 1, 1, &deltaMin, "0.001",
             "(if non-zero) stop refinements when change in spline "
             "domain sampling goes below this");
  hestOptAdd(&hopt, "distm", "dist", airTypeDouble, 1, 1, &distMin, "0",
             "(if non-zero) stop refinements when distance between spline "
             "and points goes below this");
  /*
  hestOptAdd(&hopt, NULL, "output", airTypeString, 1, 1, &out, NULL,
             "output nrrd filename");
  */

  mop = airMopNew();
  airMopAdd(mop, hopt, (airMopper)hestOptFree, airMopAlways);

  USAGE(myinfo);
  PARSE();
  airMopAdd(mop, hopt, (airMopper)hestParseFree, airMopAlways);

  if (!( 2 == _nin->dim && 2 == _nin->axis[0].size )) {
    fprintf(stderr, "%s: want 2-D (not %u) array with axis[0].size "
            "2 (not %u)\n", me, _nin->dim,
            (unsigned int)_nin->axis[0].size);
    airMopError(mop);
    return 1;
  }
  if (synth && 6 != _nin->axis[1].size) {
    fprintf(stderr, "%s: need 2-by-6 array (not 2-by-%u) for synthetic xy\n",
            me, (unsigned int)_nin->axis[1].size);
    airMopError(mop);
    return 1;
  }

  nin = nrrdNew();
  airMopAdd(mop, nin, (airMopper)nrrdNuke, airMopAlways);
  if (nrrdConvert(nin, _nin, nrrdTypeDouble)) {
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble:\n%s", me, err);
    airMopError(mop);
    return 1;
  }

  if (!synth) {
    xy = (double*)nin->data;
    pNum = (unsigned int)nin->axis[1].size;
  } else {
    /* synthesize data from control points */
    double vv1[2], vv2[2], *cpt = (double*)nin->data;
    pNum = (unsigned int)cpt[1];
    if (!( 0 == cpt[0] && pNum == cpt[1] )) {
      fprintf(stderr, "%s: need 0,int for first 2 cpt values (not %g,%g)\n",
              me, cpt[0], cpt[1]);
      airMopError(mop);
      return 1;
    }
    ELL_2V_COPY(alpha, cpt + 2);
    ELL_2V_COPY(vv0, cpt + 4);
    ELL_2V_COPY(tt1, cpt + 6);
    ELL_2V_COPY(tt2, cpt + 8);
    ELL_2V_COPY(vv3, cpt + 10);
    ELL_2V_SCALE_ADD2(vv1, 1, vv0, alpha[0], tt1);
    ELL_2V_SCALE_ADD2(vv2, 1, vv3, alpha[1], tt2);
    xy = AIR_MALLOC(2*pNum, double);
    airMopAdd(mop, xy, airFree, airMopAlways);
    limnCBSample(xy, pNum, vv0, vv1, vv2, vv3);
    if (airStrlen(synthOut)) {
      Nrrd *nsyn = nrrdNew();
      airMopAdd(mop, nsyn, (airMopper)nrrdNix, airMopAlways);
      if (nrrdWrap_va(nsyn, xy, nrrdTypeDouble, 2, (size_t)2, (size_t)pNum)
          || nrrdSave(synthOut, nsyn, NULL)) {
        airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
        fprintf(stderr, "%s: trouble saving synthetic data:\n%s", me, err);
        airMopError(mop);
        return 1;
      }
    }
  }
  {
    /* set up 2-vector-valued arguments to fitting */
    double len;
    ELL_2V_COPY(vv0, xy);
    ELL_2V_COPY(vv3, xy + 2*(pNum-1));
    ELL_2V_SUB(tt1, xy + 2, xy); ELL_2V_NORM(tt1, tt1, len);
    ELL_2V_SUB(tt2, xy + 2*(pNum-2), vv3); ELL_2V_NORM(tt2, tt2, len);
  }
  limnCBFitStateInit(&cbfs, AIR_FALSE);
  cbfs.iterMax = iterMax;
  cbfs.deltaMin = deltaMin;
  cbfs.distMin = distMin;
  cbfs.verbose = verbose;
  if (limnCBFitSingle(&cbfs, alpha,
                      vv0, tt1, tt2, vv3, xy, pNum)) {
    airMopAdd(mop, err = biffGetDone(LIMN), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble:\n%s", me, err);
    airMopError(mop);
    return 1;
  }
  printf("%s: time=%gms, iterDone=%u, deltaDone=%g, distDone=%g (@%u)\n", me,
         cbfs.timeMs, cbfs.iterDone, cbfs.deltaDone,
         cbfs.distDone, cbfs.distIdx);

  {
    double tt, pp[2], vv1[2], vv2[2], ww[4];
    unsigned int ii;
    ELL_2V_SCALE_ADD2(vv1, 1, vv0, alpha[0], tt1);
    ELL_2V_SCALE_ADD2(vv2, 1, vv3, alpha[1], tt2);
    pNum *= 10;
    for (ii=0; ii<pNum; ii++) {
      tt = AIR_AFFINE(0, ii, pNum-1, 0, 1);
      limnCBWeights(ww, tt, 0);
      ELL_2V_SCALE_ADD4(pp,
                        ww[0], vv0,
                        ww[1], vv1,
                        ww[2], vv2,
                        ww[3], vv3);
      printf("done %u %g %g\n", ii, pp[0], pp[1]);
    }
  }

  airMopOkay(mop);
  return 0;
}

unrrduCmd limnpu_bcfitCmd = { "bcfit", INFO, limnpu_bcfitMain, AIR_FALSE };
