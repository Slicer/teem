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

#include "limn.h"
#include "privateLimn.h"

#define INFO "Fit Bezier cubic spline to points"
static const char *myinfo
  = (INFO ". \"nrp\" == Newton-based ReParameterization of spline domain");

static int
limnPu_cbfitMain(int argc, const char **argv, const char *me, hestParm *hparm) {
  hestOpt *hopt = NULL;
  char *err, *perr;
  airArray *mop;
  int pret;

  Nrrd *_nin, *nin;
  double *xy, alpha[2], vv0[2], tt1[2], tt2[2], vv3[2], deltaMin, psi, cangle, distMin,
    distScl, utt1[2], utt2[2], time0, dtime, scale;
  unsigned int ii, pNum, iterMax;
  int loop, petc, verbose, synth, nofit;
  char *synthOut;
  limnCBFContext fctx;
  limnCBFPath *path;

  hestOptAdd_1_Other(&hopt, "i", "input", &_nin, NULL, "input xy points", nrrdHestNrrd);
  hestOptAdd_1_Int(&hopt, "v", "verbose", &verbose, "1", "verbosity level");
  hestOptAdd_Flag(&hopt, "s", &synth, "synthesize xy points from control points");
  hestOptAdd_1_String(&hopt, "so", "synth out", &synthOut, "",
                      "if non-empty, filename in which to save synthesized xy pts");
  hestOptAdd_Flag(&hopt, "snf", &nofit,
                  "actually do not fit, just save -so synthetic "
                  "output and quit");
  hestOptAdd_2_Double(&hopt, "t1", "tan", utt1, "nan nan",
                      "if non-nan, the outgoing tangent from the first point");
  hestOptAdd_2_Double(&hopt, "t2", "tan", utt2, "nan nan",
                      "if non-nan, the incoming tangent to the last point");
  hestOptAdd_1_UInt(&hopt, "im", "max", &iterMax, "0",
                    "(if non-zero) max # nrp iterations to run");
  hestOptAdd_1_Double(&hopt, "deltam", "delta", &deltaMin, "0.0005",
                      "(if non-zero) stop nrp when change in spline "
                      "domain sampling goes below this");
  hestOptAdd_1_Double(&hopt, "distm", "dist", &distMin, "0.01",
                      "(if non-zero) stop nrp when distance between spline "
                      "and points goes below this");
  hestOptAdd_1_Double(&hopt, "dists", "scl", &distScl, "0.25",
                      "scaling on nrp distMin check");
  hestOptAdd_1_Double(&hopt, "psi", "psi", &psi, "10", "psi, of course");
  hestOptAdd_1_Double(&hopt, "ca", "angle", &cangle, "100", "angle indicating a corner");
  hestOptAdd_1_Double(&hopt, "scl", "scale", &scale, "0",
                      "scale for geometry estimation");
  hestOptAdd_Flag(&hopt, "loop", &loop,
                  "given xy points are actually a loop; BUT "
                  "the first and last points need to be the same!");
  hestOptAdd_Flag(&hopt, "petc", &petc, "(Press Enter To Continue) ");
  /*
  hestOptAdd_1_String(&hopt, NULL, "output", &out, NULL, "output nrrd filename");
   */

  mop = airMopNew();
  airMopAdd(mop, hopt, (airMopper)hestOptFree, airMopAlways);

  USAGE(myinfo);
  PARSE();
  airMopAdd(mop, hopt, (airMopper)hestParseFree, airMopAlways);

  if (!(2 == _nin->dim && 2 == _nin->axis[0].size)) {
    fprintf(stderr,
            "%s: want 2-D (not %u) array with axis[0].size "
            "2 (not %u)\n",
            me, _nin->dim, (unsigned int)_nin->axis[0].size);
    airMopError(mop);
    return 1;
  }
  if (synth && 6 != _nin->axis[1].size) {
    fprintf(stderr, "%s: need 2-by-6 array (not 2-by-%u) for synthetic xy\n", me,
            (unsigned int)_nin->axis[1].size);
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
    xy = (double *)nin->data;
    pNum = (unsigned int)nin->axis[1].size;
  } else {
    /* synthesize data from control points */
    double *cpt = (double *)nin->data;
    limnCBFSeg seg;
    pNum = (unsigned int)cpt[1];
    if (!(0 == cpt[0] && pNum == cpt[1])) {
      fprintf(stderr, "%s: need 0,int for first 2 cpt values (not %g,%g)\n", me, cpt[0],
              cpt[1]);
      airMopError(mop);
      return 1;
    }
    ELL_2V_COPY(alpha, cpt + 2);
    ELL_2V_COPY(vv0, cpt + 4);
    ELL_2V_COPY(seg.xy + 0, vv0);
    ELL_2V_COPY(tt1, cpt + 6);
    ELL_2V_COPY(tt2, cpt + 8);
    ELL_2V_COPY(vv3, cpt + 10);
    ELL_2V_COPY(seg.xy + 6, vv3);
    ELL_2V_SCALE_ADD2(seg.xy + 2, 1, vv0, alpha[0], tt1);
    ELL_2V_SCALE_ADD2(seg.xy + 4, 1, vv3, alpha[1], tt2);
    printf("%s: synth seg: (%g,%g) -- (%g,%g) -- (%g,%g) -- (%g,%g)\n", me, seg.xy[0],
           seg.xy[1], seg.xy[2], seg.xy[3], seg.xy[4], seg.xy[5], seg.xy[6], seg.xy[7]);
    xy = AIR_MALLOC(2 * pNum, double);
    airMopAdd(mop, xy, airFree, airMopAlways);
    for (ii = 0; ii < pNum; ii++) {
      limnCBFSegEval(xy + 2 * ii, &seg, AIR_AFFINE(0, ii, pNum - 1, 0, 1));
    }
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
      if (nofit) {
        fprintf(stderr, "%s: got -nf nofit; bye\n", me);
        airMopOkay(mop);
        return 0;
      }
    }
  }
  {
    /* set up 2-vector-valued arguments to fitting */
    double len;
    ELL_2V_COPY(vv0, xy);
    ELL_2V_COPY(vv3, xy + 2 * (pNum - 1));
    if (ELL_2V_EXISTS(utt1)) {
      ELL_2V_COPY(tt1, utt1);
    } else {
      /* TODO: better tangent estimation */
      ELL_2V_SUB(tt1, xy + 2, xy);
    }
    if (ELL_2V_EXISTS(utt2)) {
      ELL_2V_COPY(tt2, utt2);
    } else {
      ELL_2V_SUB(tt2, xy + 2 * (pNum - 2), vv3);
    }
    ELL_2V_NORM(tt1, tt1, len);
    ELL_2V_NORM(tt2, tt2, len);
  }
  path = limnCBFPathNew();
  airMopAdd(mop, path, (airMopper)limnCBFPathNix, airMopAlways);
  limnCBFContextInit(&fctx, AIR_FALSE);
  fctx.nrpIterMax = iterMax;
  fctx.nrpDeltaMin = deltaMin;
  fctx.distMin = distMin;
  fctx.nrpDistScl = distScl;
  fctx.verbose = verbose;
  fctx.nrpPsi = psi;
  fctx.cornAngle = cangle;
  fctx.scale = scale;
  time0 = airTime();
  if (petc) {
    fprintf(stderr, "%s: Press Enter to Continue ... ", me);
    fflush(stderr);
    getchar();
  }
  if (limnCBFit(path, &fctx, xy, pNum, loop)) {
    airMopAdd(mop, err = biffGetDone(LIMN), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble:\n%s", me, err);
    airMopError(mop);
    return 1;
  }
  dtime = (airTime() - time0) * 1000;
  printf("%s: time= %g ms;iterDone= %u ;deltaDone=%g, dist=%g (@%u)\n", me, dtime,
         fctx.nrpIterDone, fctx.nrpDeltaDone, fctx.dist, fctx.distIdx);
  {
    unsigned int si;
    printf("%s: path has %u segments:\n", me, path->segNum);
    for (si = 0; si < path->segNum; si++) {
      limnCBFSeg *seg = path->seg + si;
      printf("seg %u (%3u): (%g,%g) -- (%g,%g) -- (%g,%g) -- (%g,%g)\n", si, seg->pNum,
             seg->xy[0], seg->xy[1], seg->xy[2], seg->xy[3], seg->xy[4], seg->xy[5],
             seg->xy[6], seg->xy[7]);
    }
  }

  if (1) {
    unsigned int oNum = pNum * 100;
    double *pp = AIR_MALLOC(oNum * 2, double);
    airMopAdd(mop, pp, airFree, airMopAlways);
    limnCBFPathSample(pp, oNum, path);
    for (ii = 0; ii < oNum; ii++) {
      printf("done %u %g %g\n", ii, (pp + 2 * ii)[0], (pp + 2 * ii)[1]);
    }
  }

  airMopOkay(mop);
  return 0;
}

const unrrduCmd limnPu_cbfitCmd = {"cbfit", INFO, limnPu_cbfitMain, AIR_FALSE};
