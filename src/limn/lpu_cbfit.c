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
  double *xy, deltaThresh, psi, cangle, epsilon, nrpIota, time0, dtime, scale, supow;
  unsigned int ii, pNum, iterMax;
  int loop, petc, verbose, tvt[4];
  char *synthOut;
  limnCBFCtx *fctx;
  limnCBFPath *path;
  limnCBFPoints *lpnt;

  hestOptAdd_1_Other(&hopt, "i", "input", &_nin, NULL, "input xy points", nrrdHestNrrd);
  hestOptAdd_1_Int(&hopt, "v", "verbose", &verbose, "1", "verbosity level");
  hestOptAdd_1_String(&hopt, "so", "synth out", &synthOut, "",
                      "if non-empty, filename in which to save synthesized xy pts, "
                      "and then quit before any fitting.");
  hestOptAdd_1_Double(&hopt, "sup", "expo", &supow, "1",
                      "when synthesizing data on a single segment, warp U parameters "
                      "by raising to this power.");
  hestOptAdd_4_Int(&hopt, "tvt", "loi hii absi 1s", tvt, "0 0 0 -1",
                   "if last value is >= 0: make single call to limnCBFFindTVT and quit, "
                   "but note that the given loi,hii,absi args are not exactly what is "
                   "passed to limnCBFFindTVT");
  hestOptAdd_1_UInt(&hopt, "im", "max", &iterMax, "0",
                    "(if non-zero) max # nrp iterations to run");
  hestOptAdd_1_Double(&hopt, "deltathr", "delta", &deltaThresh, "0.0005",
                      "(if non-zero) stop nrp when change in spline "
                      "domain sampling goes below this");
  hestOptAdd_1_Double(&hopt, "eps", "dist", &epsilon, "0.01",
                      "(if non-zero) stop nrp when distance between spline "
                      "and points goes below this");
  hestOptAdd_1_Double(&hopt, "iota", "scl", &nrpIota, "0.25",
                      "scaling on nrp epsilon check");
  hestOptAdd_1_Double(&hopt, "psi", "psi", &psi, "10", "psi, of course");
  hestOptAdd_1_Double(&hopt, "ca", "angle", &cangle, "100", "angle indicating a corner");
  hestOptAdd_1_Double(&hopt, "scl", "scale", &scale, "0",
                      "scale for geometry estimation");
  hestOptAdd_Flag(&hopt, "loop", &loop,
                  "given xy points are actually a loop: the first point logically "
                  "follows the last point");
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
  if (airStrlen(synthOut) && 6 != _nin->axis[1].size) {
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

  if (!airStrlen(synthOut)) {
    xy = (double *)nin->data;
    pNum = (unsigned int)nin->axis[1].size;
  } else {
    double alpha[2], vv0[2], tt1[2], tt2[2], vv3[2];
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
      double uu = AIR_AFFINE(0, ii, pNum - 1, 0, 1);
      uu = pow(uu, supow);
      limnCBFSegEval(xy + 2 * ii, &seg, uu);
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
      printf("%s: saved synthetic output to %s; bye\n", me, synthOut);
      airMopOkay(mop);
      return 0;
    }
  }
  if (!(lpnt = limnCBFPointsNew(xy, nrrdTypeDouble, 2, pNum, loop))) {
    airMopAdd(mop, err = biffGetDone(LIMN), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble setting up points:\n%s", me, err);
    airMopError(mop);
    return 1;
  }
  path = limnCBFPathNew();
  airMopAdd(mop, path, (airMopper)limnCBFPathNix, airMopAlways);
  fctx = limnCBFCtxNew();
  fctx->verbose = verbose;
  fctx->nrpIterMax = iterMax;
  fctx->scale = scale;
  fctx->epsilon = epsilon;
  fctx->nrpDeltaThresh = deltaThresh;
  fctx->nrpIota = nrpIota;
  fctx->nrpPsi = psi;
  fctx->cornAngle = cangle;
  if (tvt[3] >= 0) {
    double lt[2], vv[2], rt[2];
    int pnum = AIR_INT(lpnt->num);
    /* whoa - this is how GLK learned that AIR_MOD is garbage if args differ in
       sign-ed-ness */
    unsigned int loi = AIR_UINT(AIR_MOD(tvt[0], pnum));
    unsigned int hii = AIR_UINT(AIR_MOD(tvt[1], pnum));
    unsigned int ofi = AIR_UINT(AIR_MOD(tvt[2] - tvt[0], pnum));
    int E, oneSided = !!tvt[3];
    E = 0;
    if (!E && fctx->verbose)
      printf("%s: TVT %d (absolute) in [%d,%d] --> %u (offset) in [%u,%u]\n", me, /* */
             tvt[2], tvt[0], tvt[1], ofi, loi, hii);
    if (!E) E |= limnCBFCtxPrep(fctx, lpnt);
    if (!E && fctx->verbose)
      printf("%s: limnCBFCtxPrep done, calling limnCBFFindTVT\n", me);
    if (!E) E |= limnCBFFindTVT(lt, vv, rt, fctx, lpnt, loi, hii, ofi, oneSided);
    if (E) {
      airMopAdd(mop, err = biffGetDone(LIMN), airFree, airMopAlways);
      fprintf(stderr, "%s: trouble doing single tangent-vertex-tangent:\n%s", me, err);
      airMopError(mop);
      return 1;
    }
    printf("%s: loi,hii=[%d,%d] ofi=%d oneSided=%d limnCBFFindTVT:\n", me, loi, hii, ofi,
           oneSided);
    printf("lt = %g %g\n", lt[0], lt[1]);
    printf("vv = %g %g\n", vv[0], vv[1]);
    printf("rt = %g %g\n", rt[0], rt[1]);
    printf("(quitting)\n");
    airMopOkay(mop);
    return 0;
  }
  time0 = airTime();
  if (petc) {
    fprintf(stderr, "%s: Press Enter to Continue ... ", me);
    fflush(stderr);
    getchar();
  }
  if (limnCBFit(path, fctx, lpnt)) {
    airMopAdd(mop, err = biffGetDone(LIMN), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble doing fitting:\n%s", me, err);
    airMopError(mop);
    return 1;
  }
  dtime = (airTime() - time0) * 1000;
  printf("%s: time= %g ms;iterDone= %u ;deltaDone=%g, distMax=%g (@%u)\n", me, dtime,
         fctx->nrpIterDone, fctx->nrpDeltaDone, fctx->distMax, fctx->distMaxIdx);
  {
    unsigned int si;
    printf("%s: path has %u segments:\n", me, path->segNum);
    for (si = 0; si < path->segNum; si++) {
      limnCBFSeg *seg = path->seg + si;
      printf("seg %u: (%g,%g) -- (%g,%g) -- (%g,%g) -- (%g,%g)\n", si, seg->xy[0],
             seg->xy[1], seg->xy[2], seg->xy[3], seg->xy[4], seg->xy[5], seg->xy[6],
             seg->xy[7]);
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
