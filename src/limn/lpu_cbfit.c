/*
  Teem: Tools to process and visualize scientific data and images
  Copyright (C) 2009--2024  University of Chicago
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

static void
getLoHi(unsigned int *loiP, unsigned int *hiiP, const limnCbfPoints *lpnt, int slo,
        int shi) {
  unsigned int loi, hii;
  int pnum = AIR_INT(lpnt->num);
  if (lpnt->isLoop || (0 == slo && -1 == shi)) {
    loi = AIR_UINT(AIR_MOD(slo, pnum));
    hii = AIR_UINT(AIR_MOD(shi, pnum));
  } else {
    loi = AIR_UINT(AIR_CLAMP(0, slo, pnum - 1));
    hii = AIR_UINT(AIR_CLAMP(0, shi, pnum - 1));
  }
  *loiP = loi;
  *hiiP = hii;
  return;
}

static void
getVTTV(const double *VTTV[4], const limnCbfPoints *lpnt, const double fitTT[4],
        unsigned int loi, unsigned int hii) {

  if (ELL_4V_LEN(fitTT)) {
    /* help out limnCbfSingle with specific V,T,T,V */
    VTTV[0] = lpnt->pp + 0 + 2 * loi;
    VTTV[1] = fitTT + 0;
    VTTV[2] = fitTT + 2;
    VTTV[3] = lpnt->pp + 0 + 2 * hii;
  } else {
    /* no help will be given */
    VTTV[0] = VTTV[1] = VTTV[2] = VTTV[3] = NULL;
  }
  return;
}

static void
pathPrint(const char *me, const limnCbfPath *path) {
  unsigned int si;
  printf("%s: path has %u segments in %sloop:\n", me, path->segNum,
         path->isLoop ? "" : "NON-");
  for (si = 0; si < path->segNum; si++) {
    limnCbfSeg *seg = path->seg + si;
    printf("seg[%u]      %g %g     %g %g     %g %g     %g %g     %c (%u) %c\n", si,
           seg->xy[0], seg->xy[1], seg->xy[2], seg->xy[3], seg->xy[4], seg->xy[5],
           seg->xy[6], seg->xy[7], seg->corner[0] ? 'C' : '-', seg->pointNum,
           seg->corner[1] ? 'C' : '-');
  }
  return;
}

static int
limnPu_cbfitMain(int argc, const char **argv, const char *me, hestParm *hparm) {
  hestOpt *hopt = NULL;
  char *err, *perr;
  airArray *mop;
  int pret;

  Nrrd *_nin, *nin;
  double *xy, deltaThresh, psi, cangle, epsilon, nrpIota, time0, dtime, scale, nrpCap,
    synthPow, fitTT[4];
  unsigned int size0, size1, ii, synthNum, pNum, nrpIterMax;
  int loop, roll, petc, verbose, tvt[4], fitSingleLoHi[2], fitMultiLoHi[2], corner2[2];
  char *synthOut, buff[AIR_STRLEN_SMALL + 1];
  limnCbfCtx *fctx;
  limnCbfPath *path;
  limnCbfPoints *lpnt;

  mop = airMopNew();
  airMopAdd(mop, hopt, (airMopper)hestOptFree, airMopAlways);
  fctx = limnCbfCtxNew();
  airMopAdd(mop, fctx, (airMopper)limnCbfCtxNix, airMopAlways);

  hestOptAdd_1_Other(&hopt, "i", "input", &_nin, NULL, "input xy points", nrrdHestNrrd);
  hestOptAdd_Flag(&hopt, "loop", &loop,
                  "-i input xy points are actually a loop: the first point logically "
                  "follows the last point");
  hestOptAdd_1_Int(
    &hopt, "roll", "n", &roll, "0",
    "if points are in a loop, then it shouldn't really matter which point has index 0. "
    "For debugging, roll the input data by this amount prior to doing any work.");
  hestOptAdd_1_Int(&hopt, "v", "verbose", &verbose, "1", "verbosity level");
  hestOptAdd_1_UInt(&hopt, "synthn", "num", &synthNum, "51",
                    "if saving spline sampling to -so, how many sample.");
  hestOptAdd_1_String(
    &hopt, "syntho", "synth out", &synthOut, "",
    "if non-empty, input xy points are actually either: (1) 2-by-4 array of control "
    "points for a single spline segment, or (2) an 8-by-N array for a sequence of "
    "splines; either way the path should be sampled -sn times, and this is the filename "
    "into which to save the synthesized xy pts, and then quit without any fitting.");
  hestOptAdd_1_Double(&hopt, "sup", "expo", &synthPow, "1",
                      "when synthesizing data on a single segment, warp U parameters "
                      "by raising to this power.");
  hestOptAdd_4_Int(&hopt, "tvt", "loi hii vvi 1s", tvt, "0 0 0 -1",
                   "if last value is >= 0: make single call to limnCbfTVT and quit");
  sprintf(buff, "%u", fctx->nrpIterMax);
  hestOptAdd_1_UInt(&hopt, "nim", "max", &nrpIterMax, buff,
                    "max # nrp iterations to run");
  sprintf(buff, "%.17g", fctx->nrpDeltaThresh);
  hestOptAdd_1_Double(&hopt, "deltathr", "delta", &deltaThresh, buff,
                      "(if non-zero) stop nrp when change in spline "
                      "domain sampling goes below this");
  /* fctx does not come with useful default epsilon */
  hestOptAdd_1_Double(&hopt, "eps", "dist", &epsilon, "0.01",
                      "(if non-zero) stop nrp when distance between spline "
                      "and points goes below this");
  sprintf(buff, "%.17g", fctx->nrpIota);
  hestOptAdd_1_Double(&hopt, "iota", "scl", &nrpIota, buff,
                      "scaling on nrp epsilon check");
  sprintf(buff, "%.17g", fctx->nrpPsi);
  hestOptAdd_1_Double(&hopt, "psi", "psi", &psi, buff, "psi, of course");
  sprintf(buff, "%.17g", fctx->cornAngle);
  hestOptAdd_1_Double(&hopt, "ca", "angle", &cangle, buff, "angle indicating a corner");
  sprintf(buff, "%.17g", fctx->scale);
  hestOptAdd_1_Double(&hopt, "scl", "scale", &scale, buff,
                      "scale for geometry estimation");
  sprintf(buff, "%.17g", fctx->nrpCap);
  hestOptAdd_1_Double(&hopt, "cap", "cap", &nrpCap, buff,
                      "nrp cap parameterization change");
  hestOptAdd_2_Int(&hopt, "fs", "loi hii", fitSingleLoHi, "-1 -1",
                   "(if loi is >= 0) just do a single call to limnCbfSingle and "
                   "quit, using the -i input points, and fitting a spline between "
                   "the loi and hii indices given here. A negative hii will be "
                   "incremented by the number of points, so -1 works to indicate "
                   "the last point.");
  hestOptAdd_4_Double(&hopt, "ftt", "T1x T1y T2x T2y", fitTT, "0 0 0 0",
                      "(if non-zero): help out call to either -fs limnCbfSingle or -fm "
                      "limnCbfMulti by giving these vectors for T1 (outgoing from V0) "
                      "and T2 (incoming to V3) tangents, so they are not estimated from "
                      "the data. If this is used; V0 and V3 are set as the first and "
                      "last points (there is currently no ability to set only some of "
                      "the 4 vector args to limnCbfSingle or limnCbfMulti)");
  hestOptAdd_2_Int(&hopt, "fm", "loi hii", fitMultiLoHi, "-1 -1",
                   "(if loi is >= 0) just do a single call to limnCbfMulti and "
                   "quit, using the -i input points, fitting a multi-spline path "
                   "between the loi and hii indices given here. A negative hii will be "
                   "incremented by the number of points, so -1 works to indicate "
                   "the last point.");
  hestOptAdd_2_Int(&hopt, "corn", "val nms", corner2, "0 0",
                   "if 1st val non-zero: call limnCbfCorners and quit. fctx->cornerFind "
                   "is set to true if 1st value given here is positive, fctx->cornerNMS "
                   "is set to !! of second value");
  hestOptAdd_Flag(&hopt, "petc", &petc, "(Press Enter To Continue) ");
  /*
  hestOptAdd_1_String(&hopt, NULL, "output", &out, NULL, "output nrrd filename");
   */

  USAGE(myinfo);
  PARSE(myinfo);
  airMopAdd(mop, hopt, (airMopper)hestOptFree, airMopAlways);
  airMopAdd(mop, hopt, (airMopper)hestParseFree, airMopAlways);

  if (!(2 == _nin->dim)) {
    fprintf(stderr, "%s: need 2-D (not %u) input array\n", me, _nin->dim);
    airMopError(mop);
    return 1;
  }
  size0 = AIR_UINT(_nin->axis[0].size);
  size1 = AIR_UINT(_nin->axis[1].size);
  if (airStrlen(synthOut)) {
    if (!((2 == size0 && 4 == size1) || (8 == size0))) {
      fprintf(stderr,
              "%s: for synthesizing, need either 2-by-4 array or "
              "8-by-N (not %u-by-%u)\n",
              me, size0, size1);
      airMopError(mop);
      return 1;
    }
  } else if (!(2 == size0)) {
    fprintf(stderr, "%s: need 2-by-N input XY points (not %u-by-N)", me, size0);
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
  if (roll) {
    Nrrd *ntmp;
    double *xy, *tmp;
    int pnum, pi;
    if (airStrlen(synthOut)) {
      fprintf(stderr, "%s: can only roll (%d) input XY points, not splines\n", me, roll);
      airMopError(mop);
      return 1;
    }
    if (!loop) {
      fprintf(stderr, "%s: can only roll (%d) a point loop (no -loop)\n", me, roll);
      airMopError(mop);
      return 1;
    }
    if (!(nrrdTypeDouble == nin->type)) {
      fprintf(stderr, "%s: need type %s (not %s) point data\n", me,
              airEnumStr(nrrdType, nrrdTypeDouble), airEnumStr(nrrdType, nin->type));
      airMopError(mop);
      return 1;
    }
    ntmp = nrrdNew();
    airMopAdd(mop, ntmp, (airMopper)nrrdNuke, airMopAlways);
    if (nrrdCopy(ntmp, nin)) {
      airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
      fprintf(stderr, "%s: trouble:\n%s", me, err);
      airMopError(mop);
      return 1;
    }
    pnum = AIR_INT(nin->axis[1].size);
    xy = nin->data;
    tmp = ntmp->data;
    for (pi = 0; pi < pnum; pi++) {
      int pt = AIR_MOD(pi - roll, pnum);
      ELL_2V_COPY(xy + 2 * pi, tmp + 2 * pt);
      if (!pi) {
        printf("%s: with roll=%d; xy[0] is now original xy[%d]: %g %g\n", me, roll, pt,
               xy[0], xy[1]);
      }
    }
  }

  if (airStrlen(synthOut)) {
    /* synthesize data from control points */
    double *cpt = (double *)nin->data;
    limnCbfSeg seg;
    Nrrd *nsyn;
    if (!(synthNum >= 3)) {
      fprintf(stderr, "%s: for data synthesis need at least 3 samples (not %u)\n", me,
              synthNum);
      airMopError(mop);
      return 1;
    }
    xy = AIR_MALLOC(2 * synthNum, double);
    airMopAdd(mop, xy, airFree, airMopAlways);
    if (2 == size0) {
      unsigned int ci;
      printf("%s: synthetically sampling single spline with %u points\n", me, synthNum);
      for (ci = 0; ci < 8; ci++) {
        seg.xy[ci] = cpt[ci];
      }
      printf("%s: synth seg: (%g,%g) -- (%g,%g) -- (%g,%g) -- (%g,%g)\n", me, seg.xy[0],
             seg.xy[1], seg.xy[2], seg.xy[3], seg.xy[4], seg.xy[5], seg.xy[6],
             seg.xy[7]);
      for (ii = 0; ii < synthNum; ii++) {
        double uu = AIR_AFFINE(0, ii, synthNum - 1, 0, 1);
        uu = pow(uu, synthPow);
        limnCbfSegEval(xy + 2 * ii, &seg, uu);
      }
    } else {
      unsigned int ci, si;
      limnCbfPath *spath = limnCbfPathNew(size1);
      airMopAdd(mop, spath, (airMopper)limnCbfPathNix, airMopAlways);
      printf("%s: synthetically sampling %u splines with %u points\n", me, size1,
             synthNum);
      /* copy in control point data */
      for (si = 0; si < size1; si++) {
        for (ci = 0; ci < 8; ci++) {
          spath->seg[si].xy[ci] = (cpt + 8 * si)[ci];
        }
      }
      limnCbfPathSample(xy, synthNum, spath);
    }

    nsyn = nrrdNew();
    airMopAdd(mop, nsyn, (airMopper)nrrdNix, airMopAlways);
    if (nrrdWrap_va(nsyn, xy, nrrdTypeDouble, 2, (size_t)2, (size_t)synthNum)
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

  xy = (double *)nin->data;
  pNum = (unsigned int)nin->axis[1].size;
  if (!(lpnt = limnCbfPointsNew(xy, nrrdTypeDouble, 2, pNum, loop))) {
    airMopAdd(mop, err = biffGetDone(LIMN), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble setting up points:\n%s", me, err);
    airMopError(mop);
    return 1;
  }
  airMopAdd(mop, lpnt, (airMopper)limnCbfPointsNix, airMopAlways);
  path = limnCbfPathNew(0);
  airMopAdd(mop, path, (airMopper)limnCbfPathNix, airMopAlways);
  fctx->verbose = verbose;
  fctx->nrpIterMax = nrpIterMax;
  fctx->scale = scale;
  fctx->nrpCap = nrpCap;
  fctx->epsilon = epsilon;
  fctx->nrpDeltaThresh = deltaThresh;
  fctx->nrpIota = nrpIota;
  fctx->nrpPsi = psi;
  fctx->cornAngle = cangle;

  if (tvt[3] >= 0) { /* here just to call limnCbfTVT once */
    double lt[2], vv[2], rt[2];
    int pnum = AIR_INT(lpnt->num);
    /* whoa - this is how GLK learned that AIR_MOD is garbage if args differ in
       sign-ed-ness */
    unsigned int loi, hii, vvi;
    int E, oneSided = !!tvt[3];
    if (lpnt->isLoop) {
      vvi = AIR_UINT(AIR_MOD(tvt[2], pnum));
    } else {
      vvi = AIR_UINT(AIR_CLAMP(0, tvt[2], pnum - 1));
    }
    getLoHi(&loi, &hii, lpnt, tvt[0], tvt[1]);
    E = 0;
    if (!E && fctx->verbose)
      printf("%s: int %d in [%d,%d] --> uint %u in [%u,%u]\n", me, /* */
             tvt[2], tvt[0], tvt[1], vvi, loi, hii);
    if (!E) E |= limnCbfCtxPrep(fctx, lpnt);
    if (!E && fctx->verbose) printf("%s: limnCbfCtxPrep done, calling limnCbfTVT\n", me);
    if (!E) E |= limnCbfTVT(lt, vv, rt, fctx, lpnt, loi, hii, vvi, oneSided);
    if (E) {
      airMopAdd(mop, err = biffGetDone(LIMN), airFree, airMopAlways);
      fprintf(stderr, "%s: trouble doing lone tangent-vertex-tangent:\n%s", me, err);
      airMopError(mop);
      return 1;
    }
    printf("%s: loi,hii=[%d,%d] vvi=%d oneSided=%d limnCbfTVT:\n", me, loi, hii, vvi,
           oneSided);
    printf("lt = %g %g\n", lt[0], lt[1]);
    printf("vv = %g %g\n", vv[0], vv[1]);
    printf("rt = %g %g\n", rt[0], rt[1]);
    printf("(quitting)\n");
    airMopOkay(mop);
    return 0;
  }

  if (fitSingleLoHi[0] >= 0) { /* here to call limnCbfSingle once */
    unsigned int loi, hii;
    const double *VTTV[4];
    limnCbfSeg seg;
    getLoHi(&loi, &hii, lpnt, fitSingleLoHi[0], fitSingleLoHi[1]);
    getVTTV(VTTV, lpnt, fitTT, loi, hii);
    if (limnCbfSingle(&seg, VTTV[0], VTTV[1], VTTV[2], VTTV[3], fctx, lpnt, loi, hii)) {
      airMopAdd(mop, err = biffGetDone(LIMN), airFree, airMopAlways);
      fprintf(stderr, "%s: trouble doing single segment fit:\n%s", me, err);
      airMopError(mop);
      return 1;
    }
    printf("%s: nrpIterDone %u    nrpPuntFlop %u    distMax %g @ %u/%u (big %d)\n", me,
           fctx->nrpIterDone, fctx->nrpPuntFlop, fctx->distMax, fctx->distMaxIdx,
           lpnt->num, fctx->distBig);
    printf("%s: limnCbfSingle spline result:\n", me);
    for (ii = 0; ii < 4; ii++) {
      printf("%g %g\n", seg.xy[0 + 2 * ii], seg.xy[1 + 2 * ii]);
    }
    airMopOkay(mop);
    return 0;
  }

  if (corner2[0]) { /* here to call limnCbfCorners once */
    fctx->cornerFind = corner2[0] > 0;
    fctx->cornerNMS = !!corner2[1];
    if (limnCbfCorners(fctx, lpnt)) {
      airMopAdd(mop, err = biffGetDone(LIMN), airFree, airMopAlways);
      fprintf(stderr, "%s: trouble finding corners:\n%s", me, err);
      airMopError(mop);
      return 1;
    }
    if (!fctx->cnum) {
      printf("%s: Zero corners found!\n", me);
    } else {
      unsigned int ci;
      const double *tvt;
      printf("%s: %u corners found:\n", me, fctx->cnum);
      for (ci = 0; ci < fctx->cnum; ci++) {
        tvt = fctx->ctvt + 6 * ci;
        printf("%3u: vi=%3u  lt=(%g,%g)  vv=(%g,%g)  rt=(%g,%g)\n", ci, fctx->cidx[ci],
               tvt[0], tvt[1], tvt[2], tvt[3], tvt[4], tvt[5]);
      }
    }
    airMopOkay(mop);
    return 0;
  }

  if (fitMultiLoHi[0] >= 0) { /* here to call limnCbfMulti once */
    unsigned int loi, hii;
    const double *VTTV[4];
    getLoHi(&loi, &hii, lpnt, fitMultiLoHi[0], fitMultiLoHi[1]);
    getVTTV(VTTV, lpnt, fitTT, loi, hii);
    if (limnCbfCtxPrep(fctx, lpnt)
        || limnCbfMulti(path, VTTV[0], VTTV[1], VTTV[2], VTTV[3], 0, fctx, lpnt, loi,
                        hii)) {
      airMopAdd(mop, err = biffGetDone(LIMN), airFree, airMopAlways);
      fprintf(stderr, "%s: trouble doing multi fit:\n%s", me, err);
      airMopError(mop);
      return 1;
    }
    printf("%s: limnCbfMulti results:\n", me);
    pathPrint(me, path);
    airMopOkay(mop);
    return 0;
  }

  /* whoa - we're actually here to call limnCbfGo! */
  time0 = airTime();
  if (petc) {
    fprintf(stderr, "%s: Press Enter to Continue ... ", me);
    fflush(stderr);
    getchar();
  }

  if (limnCbfGo(path, fctx, lpnt)) {
    airMopAdd(mop, err = biffGetDone(LIMN), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble doing fitting:\n%s", me, err);
    airMopError(mop);
    return 1;
  }

  dtime = (airTime() - time0) * 1000;
  printf("%s: time=%g ms   iterDone=%u   deltaDone=%g   distMax=%g @ %u\n", me, dtime,
         fctx->nrpIterDone, fctx->nrpDeltaDone, fctx->distMax, fctx->distMaxIdx);
  pathPrint(me, path);

  airMopOkay(mop);
  return 0;
}

const unrrduCmd limnPu_cbfitCmd = {"cbfit", INFO, limnPu_cbfitMain, AIR_FALSE};
