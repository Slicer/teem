/*
  Teem: Tools to process and visualize scientific data and images
  Copyright (C) 2009--2024  University of Chicago
  Copyright (C) 2008, 2007, 2006, 2005  Gordon Kindlmann
  Copyright (C) 2004, 2003, 2002, 2001, 2000, 1999, 1998  University of Utah

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
typedef unsigned int uint;
#include <assert.h>

/*
This file contains GLK's implementation of the curve fitting described in:
Philip J. Schneider. "An Algorithm for Automatically Fitting Digitized Curves".
In Graphics Gems, Academic Press, 1990, pp. 612–626.
https://dl.acm.org/doi/10.5555/90767.90941
The author's code is here:
http://www.realtimerendering.com/resources/GraphicsGems/gems/FitCurves.c

The functions below do not use any other limnSpline structs or functions, since those
were written a long time ago when GLK was even more ignorant than now about splines.
Hopefully that other code can be revisited and re-organized for a later version of
Teem, at which point the code below can be integrated with it.

NOTE: spline fitting would be useful in 3D (or higher dimensions) too, but currently
this code only supports 2D. "DIM=2" flags places in code where that is explicit.
*/

#define PNMIN(ISLOOP) ((ISLOOP) ? 4 : 3)

/*
******** limnCBFPointsNew
**
** create a point data container, possibly around given pdata pointer. In an aspirational
** hope of API stability, this is one of the few functions for which the interface itself
** does not expose the specificity to DIM=2 and type double (though the code inside
** does (apologetically) enforce that).
*/
limnCBFPoints * /* Biff: NULL */
limnCBFPointsNew(const void *pdata, int ptype, uint dim, uint pnum, int isLoop) {
  static const char me[] = "limnCBFPointsNew";
  limnCBFPoints *lpnt;
  if (airEnumValCheck(nrrdType, ptype)) {
    biffAddf(LIMN, "%s: point data type %d not valid", me, ptype);
    return NULL;
  }
  if (ptype != nrrdTypeDouble) {
    biffAddf(LIMN, "%s: sorry, only %s-type data implemented now (not %s)", me,
             airEnumStr(nrrdType, nrrdTypeDouble), airEnumStr(nrrdType, ptype));
    return NULL;
  }
  if (2 != dim) {
    biffAddf(LIMN, "%s: sorry, only 2-D data implemented now (not %u)", me, dim);
    return NULL;
  }
  if (pnum < PNMIN(isLoop)) {
    biffAddf(LIMN, "%s: need at least %u points in %s (not %u)", me, PNMIN(isLoop),
             isLoop ? "loop" : "non-loop", pnum);
    return NULL;
  }
  lpnt = AIR_CALLOC(1, limnCBFPoints);
  assert(lpnt);
  if (pdata) {
    /* we are wrapping around a given pre-allocated buffer */
    lpnt->pp = pdata;
    lpnt->ppOwn = NULL;
  } else {
    /* we are allocating our own buffer */
    lpnt->pp = NULL;
    lpnt->ppOwn = AIR_CALLOC(pnum, double);
    assert(lpnt->ppOwn);
  }
  lpnt->num = pnum;
  lpnt->dim = dim; /* but really DIM=2 because of above */
  lpnt->isLoop = isLoop;
  return lpnt;
}

limnCBFPoints * /* Biff: nope */
limnCBFPointsNix(limnCBFPoints *lpnt) {
  if (lpnt) {
    /* don't touch lpnt->pp */
    if (lpnt->ppOwn) free(lpnt->ppOwn);
    free(lpnt);
  }
  return NULL;
}

int /* Biff: 1 */
limnCBFPointsCheck(const limnCBFPoints *lpnt) {
  static const char me[] = "limnCBFPointsCheck";
  uint pnmin;
  int have;

  if (!lpnt) {
    biffAddf(LIMN, "%s: got NULL pointer", me);
    return 1;
  }
  pnmin = PNMIN(lpnt->isLoop);
  if (!(lpnt->num >= pnmin)) {
    biffAddf(LIMN, "%s: need %u or more points in limnCBFPoints (not %u)%s", me, pnmin,
             lpnt->num, lpnt->isLoop ? " for loop" : "");
    return 1;
  }
  have = !!lpnt->pp + !!lpnt->ppOwn;
  if (1 != have) {
    biffAddf(LIMN, "%s: need 1 coord pointers (not %d)", me, have);
    return 1;
  }
  return 0;
}

static void
segInit(void *_seg) {
  limnCBFSeg *seg = (limnCBFSeg *)_seg;
  ELL_2V_NAN_SET(seg->xy + 0); /* DIM=2 */
  ELL_2V_NAN_SET(seg->xy + 2);
  ELL_2V_NAN_SET(seg->xy + 4);
  ELL_2V_NAN_SET(seg->xy + 6);
  seg->corner[0] = seg->corner[1] = AIR_FALSE;
  return;
}

limnCBFPath * /* Biff: nope */
limnCBFPathNew() {
  limnCBFPath *path;
  path = AIR_MALLOC(1, limnCBFPath);
  if (path) {
    path->segArr = airArrayNew((void **)(&path->seg), &path->segNum, sizeof(limnCBFSeg),
                               128 /* incr */);
    airArrayStructCB(path->segArr, segInit, NULL);
    path->isLoop = AIR_FALSE;
  }
  return path;
}

limnCBFPath * /* Biff: nope */
limnCBFPathNix(limnCBFPath *path) {
  if (path) {
    airArrayNuke(path->segArr);
    free(path);
  }
  return NULL;
}

static void
limnCBFPathJoin(limnCBFPath *dst, const limnCBFPath *src) {
  uint bb = airArrayLenIncr(dst->segArr, src->segNum);
  memcpy(dst->seg + bb, src->seg, (src->segNum) * sizeof(limnCBFSeg));
  return;
}

/* initialize a freshly allocated limnCBFCtx struct;
   the pointers therein do not point to anything valid */
static void
ctxInit(limnCBFCtx *fctx) {
  if (!fctx) return;
  /* defaults for input parameters to various CBF functions */
  fctx->verbose = 0;
  fctx->cornerFind = AIR_TRUE;
  fctx->cornerNMS = AIR_TRUE;
  fctx->nrpIterMax = 10;
  fctx->epsilon = 0; /* will need to be set to something valid elsewhere */
  fctx->scale = 0;   /* will need to be set to something valid elsewhere */
  fctx->nrpCap = 3.0;
  fctx->nrpIota = 0.8;
  fctx->nrpPsi = 10;
  fctx->nrpDeltaThresh = 0.01;
  fctx->alphaMin = 0.001;
  fctx->detMin = 0.01;
  fctx->cornAngle = 120.0; /* degrees */
  /* internal state */
  /* initialize buffer pointers to NULL and buffer lengths to 0 */
  fctx->uu = fctx->vw = fctx->tw = fctx->cpp = fctx->clt = fctx->crt = NULL;
  fctx->uLen = fctx->wLen = fctx->cNum = 0;
  /* initialize outputs to bogus valus */
  fctx->nrpIterDone = (uint)(-1);
  fctx->distMaxIdx = (uint)(-1);
  fctx->distMax = AIR_POS_INF;
  fctx->nrpDeltaDone = AIR_POS_INF;
  fctx->alphaDet = 0;
  fctx->distBig = 0;
  return;
}

limnCBFCtx * /* Biff: nope */
limnCBFCtxNew() {
  limnCBFCtx *ret;

  ret = AIR_CALLOC(1, limnCBFCtx);
  assert(ret);
  ctxInit(ret);
  return ret;
}

limnCBFCtx * /* Biff: nope */
limnCBFCtxNix(limnCBFCtx *fctx) {
  if (fctx) {
    if (fctx->uu) free(fctx->uu);
    if (fctx->vw) free(fctx->vw);
    if (fctx->tw) free(fctx->tw);
    if (fctx->cpp) free(fctx->cpp);
    if (fctx->clt) free(fctx->clt);
    if (fctx->crt) free(fctx->crt);
    free(fctx);
  }
  return NULL;
}

/*
** ctxBuffersSet: ensures that some buffers in fctx: uu, vw, tw
** are set up for current #points pNum and measurement scale fctx->scale.
** The buffers are re-allocated only when necessary.
** Does NOT touch the corner-related buffers: cpp, clt, crt
*/
static int /* Biff: 1 */
ctxBuffersSet(limnCBFCtx *fctx, uint pNum) {
  static const char me[] = "ctxBuffersSet";
  double scale;
  uint ulen, ii;

  if (!fctx) {
    biffAddf(LIMN, "%s: got NULL pointer", me);
    return 1;
  }
  scale = fctx->scale;
  if (!pNum || scale < 0 || !AIR_EXISTS(scale)) {
    biffAddf(LIMN, "%s: pNum %u or scale %g not valid", me, pNum, scale);
    return 1;
  }

  /* assuming pNum = lpnt->num for points lpnt in consideration, this allocation size
     (big enough to parameterize all the points at once) is safe, though it will likely
     be excessive given how the path may be split at corners into separate segments. */
  ulen = pNum * 2; /* DIM=2 */
  if (ulen != fctx->uLen) {
    airFree(fctx->uu);
    if (!(fctx->uu = AIR_CALLOC(ulen, double))) {
      biffAddf(LIMN, "%s: failed to allocate uu buffer (%u doubles)", me, ulen);
      return 1;
    }
  }
  fctx->uLen = ulen;

  if (0 == scale) {
    /* will do simplest possible finite differences; no need for weights */
    fctx->vw = airFree(fctx->vw);
    fctx->tw = airFree(fctx->tw);
    fctx->wLen = 0;
  } else {
    /* one: what value in summing kernel weights should count as 1.0. This should
    probably be a parm in fctx, but not very interesting to change; it reflects something
    about the confidence that the nrrdKernelDiscreteGaussian is working as expected,
    rather than something about tuning cubic spline fitting */
    const double one = 0.999;
    /* if vw and tw are allocated for length wlbig (or bigger) something isn't right */
    const uint wlbig = 128;
    /* if the initial weights for the tangent computation sum to smaller than this
       (they will be later normalized to sum to 1) then something isn't right */
    const double tinytsum = 1.0 / 64;
    double kparm[2], vsum, tsum;
    uint wlen;
    /* else need to (possibly allocate and) set vw and tw buffers */
    kparm[0] = scale;
    kparm[1] = 100000; /* effectively no cut-off; sanity check comes later */
    ii = 0;
    vsum = 0;
    do {
      vsum += (!ii ? 1 : 2) * nrrdKernelDiscreteGaussian->eval1_d(ii, kparm);
      ii++;
    } while (vsum < one && ii < wlbig);
    /* wlen = intended length of blurring kernel weight vectors */
    wlen = ii;
    if (wlen > wlbig) {
      biffAddf(LIMN,
               "%s: weight buffer length %u (from scale %g) seems "
               "too large",
               me, wlen, scale);
      return 1;
    }
    if (wlen > pNum / 2) {
      biffAddf(LIMN,
               "%s: weight buffer length %u (from scale %g) seems "
               "too large compared to #points %u",
               me, wlen, scale, pNum);
      return 1;
    }
    if (wlen != fctx->wLen) {
      airFree(fctx->vw);
      airFree(fctx->tw);
      if (!((fctx->vw = AIR_CALLOC(wlen, double))
            && (fctx->tw = AIR_CALLOC(wlen, double)))) {
        biffAddf(LIMN, "%s: couldn't allocate weight buffers (%u doubles)", me, wlen);
        return 1;
      }
      fctx->wLen = wlen;
    }
    /* normalization intent:
       1 = sum_i(vw[|i|]) for i=-(len-1)...len-1
       1 = sum_i(tw[i]) for i=1...len-1 */
    vsum = tsum = 0;
    for (ii = 0; ii < wlen; ii++) {
      double kw = nrrdKernelDiscreteGaussian->eval1_d(ii, kparm);
      vsum += (!ii ? 1 : 2) * (fctx->vw[ii] = kw);
      tsum += (fctx->tw[ii] = ii * kw);
    }
    if (tsum < tinytsum) {
      biffAddf(LIMN,
               "%s: scale %g led to tiny unnormalized tangent weight sum %g; "
               "purpose of scale is to do blurring but scale %g won't do that",
               me, scale, tsum, scale);
      return 1;
    }
    for (ii = 0; ii < wlen; ii++) {
      fctx->vw[ii] /= vsum;
      fctx->tw[ii] /= tsum;
      if (fctx->verbose) {
        printf("%s: ii=%3u    v=%0.17g    t=%0.17g\n", me, ii, fctx->vw[ii],
               fctx->tw[ii]);
      }
    }
  } /* else scale > 0 */

  return 0;
}

/*
******** limnCBFCtxPrep
**
** checks the things that are going to be passed around a lot,
** and makes call to initialize buffers inside fctx
*/
int /* Biff: 1 */
limnCBFCtxPrep(limnCBFCtx *fctx, const limnCBFPoints *lpnt) {
  static const char me[] = "limnCBFCtxPrep";

  if (!(fctx && lpnt)) {
    biffAddf(LIMN, "%s: got NULL pointer", me);
    return 1;
  }
  if (limnCBFPointsCheck(lpnt)) {
    biffAddf(LIMN, "%s: problem with points", me);
    return 1;
  }
  if (!(fctx->epsilon > 0)) {
    biffAddf(LIMN, "%s: need positive epsilon (not %g)", me, fctx->epsilon);
    return 1;
  }
  if (!(fctx->scale >= 0)) {
    biffAddf(LIMN, "%s: need non-negative scale (not %g)", me, fctx->scale);
    return 1;
  }
  if (!(fctx->nrpCap > 0)) {
    biffAddf(LIMN, "%s: need positive nrpCap (not %g)", me, fctx->nrpCap);
    return 1;
  }
  if (!(0 < fctx->nrpIota && fctx->nrpIota <= 1)) {
    biffAddf(LIMN, "%s: nrpIota (%g) must be in (0,1]", me, fctx->nrpIota);
    return 1;
  }
  if (!(1 <= fctx->nrpPsi)) {
    biffAddf(LIMN, "%s: nrpPsi (%g) must be >= 1", me, fctx->nrpPsi);
    return 1;
  }
  if (!(fctx->nrpDeltaThresh > 0)) {
    biffAddf(LIMN, "%s: need positive nrpDeltaThresh (not %g) ", me,
             fctx->nrpDeltaThresh);
    return 1;
  }
  if (!(fctx->alphaMin > 0)) {
    biffAddf(LIMN, "%s: need positive alphaMin (not %g) ", me, fctx->alphaMin);
    return 1;
  }
  if (!(fctx->detMin > 0)) {
    biffAddf(LIMN, "%s: need positive detMin (not %g) ", me, fctx->detMin);
    return 1;
  }
  {
    const double amin = 60;
    const double amax = 180;
    if (!(amin <= fctx->cornAngle && fctx->cornAngle <= amax)) {
      biffAddf(LIMN, "%s: cornAngle (%g) outside sane range [%g,%g]", me,
               fctx->cornAngle, amin, amax);
      return 1;
    }
  }
  if (ctxBuffersSet(fctx, lpnt->num)) {
    biffAddf(LIMN, "%s: trouble setting up buffers", me);
    return 1;
  }

  return 0;
}

/* CB0, CB1, CB2, CB3 = degree 3 Bernstein polynomials, for *C*ubic
   *B*ezier curves, and their derivatives D0, D1, D2 (not using any
   nice recursion properties for evaluation, oh well) */
#define CB0D0(T) ((1 - (T)) * (1 - (T)) * (1 - (T)))
#define CB1D0(T) (3 * (T) * (1 - (T)) * (1 - (T)))
#define CB2D0(T) (3 * (T) * (T) * (1 - (T)))
#define CB3D0(T) ((T) * (T) * (T))

#define CB0D1(T) (-3 * (1 - (T)) * (1 - (T)))
#define CB1D1(T) (3 * ((T) - 1) * (3 * (T) - 1))
#define CB2D1(T) (3 * (T) * (2 - 3 * (T)))
#define CB3D1(T) (3 * (T) * (T))

#define CB0D2(T) (6 * (1 - (T)))
#define CB1D2(T) (6 * (3 * (T) - 2))
#define CB2D2(T) (6 * (1 - 3 * (T)))
#define CB3D2(T) (6 * (T))

/* set 4-vector of weights W by evaluating all CBi at T */
#define VCBD0(W, T)                                                                     \
  ((W)[0] = CB0D0(T), (W)[1] = CB1D0(T), (W)[2] = CB2D0(T), (W)[3] = CB3D0(T))
#define VCBD1(W, T)                                                                     \
  ((W)[0] = CB0D1(T), (W)[1] = CB1D1(T), (W)[2] = CB2D1(T), (W)[3] = CB3D1(T))
#define VCBD2(W, T)                                                                     \
  ((W)[0] = CB0D2(T), (W)[1] = CB1D2(T), (W)[2] = CB2D2(T), (W)[3] = CB3D2(T))

/* get 4-vector of weights at T, and evaluate spline by adding up
   control points, using weight vector buffer W */
#define CBDI(P, CB, V0, V1, V2, V3, T, W)                                               \
  (CB(W, T),                                                                            \
   ELL_2V_SCALE_ADD4(P, (W)[0], (V0), (W)[1], (V1), (W)[2], (V2), (W)[3], (V3)))
/* _2V_ above: DIM=2 */
#define CBD0(P, V0, V1, V2, V3, T, W) CBDI(P, VCBD0, V0, V1, V2, V3, T, W)
#define CBD1(P, V0, V1, V2, V3, T, W) CBDI(P, VCBD1, V0, V1, V2, V3, T, W)
#define CBD2(P, V0, V1, V2, V3, T, W) CBDI(P, VCBD2, V0, V1, V2, V3, T, W)

/*
******** limnCBFSegEval
**
** evaluates a single limnCBFSeg at one point tt in [0.0,1.0]
*/
void
limnCBFSegEval(double *vv, const limnCBFSeg *seg, double tt) {
  double ww[4];
  const double *xy = seg->xy;
  CBD0(vv, xy + 0, xy + 2, xy + 4, xy + 6, tt, ww); /* DIM=2 */
  /*
  fprintf(stderr, "!%s: tt=%g -> ww={%g,%g,%g,%g} * "
          "{(%g,%g),(%g,%g),(%g,%g),(%g,%g)} = (%g,%g)\n",
          "limnCBFSegEval", tt, ww[0], ww[1], ww[2], ww[3],
          (xy + 0)[0], (xy + 0)[1],
          (xy + 2)[0], (xy + 2)[1],
          (xy + 4)[0], (xy + 4)[1],
          (xy + 6)[0], (xy + 6)[1], vv[0], vv[1]);
  */
  return;
}

/*
******** limnCBFPathSample
**
** evaluates limnCBFPath at pNum locations, uniformly (and very naively)
** distributed among the path segments, and saves into (pre-allocated) xy
*/
void
limnCBFPathSample(double *xy, uint pNum, const limnCBFPath *path) {
  uint ii, sNum = path->segNum;
  for (ii = 0; ii < pNum; ii++) {
    uint segi = airIndex(0, ii, pNum - 1, sNum);
    const limnCBFSeg *seg = path->seg + segi;
    double tmpf = AIR_AFFINE(0, ii, pNum - 1, 0, sNum);
    double tt = tmpf - segi;
    limnCBFSegEval(xy + 2 * ii, seg, tt); /* DIM=2 */
    /*
    fprintf(stderr, "!%s: %u -> %u (%g) %g -> (%g,%g)\n",
            "limnCBFPathSample", ii, segi, tmpf, tt,
            (xy + 2*ii)[0], (xy + 2*ii)[1]);
    */
  }
  return;
}

/* cheesy macro as short-hand to access either pp or ppOwn */
#define PP(lpnt) ((lpnt)->pp ? (lpnt)->pp : (lpnt)->ppOwn)

/* error-checked index processing for limnCBFFindVT and maybe others */
static int /* Biff: 1 */
idxPrep(int *sloP, int *shiP, int *loopyP, const limnCBFPoints *lpnt, uint loi,
        uint hii) {
  static const char me[] = "idxPrep";
  int slo, shi, loopy, spanlen;

  *sloP = *shiP = 10 * lpnt->num; /* initialize to bogus indices */
  if (!(loi < lpnt->num && hii < lpnt->num)) {
    biffAddf(LIMN, "%s: loi %u, hii %u not both < #points %u", me, loi, hii, lpnt->num);
    return 1;
  }
  if (loi == hii && hii != 0) {
    biffAddf(LIMN,
             "%s: can only have loi == hii if both 0 (not %u), to signify no bounds in "
             "point loop",
             me, loi);
    return 1;
  }
  slo = AIR_INT(loi);
  shi = AIR_INT(hii);
  if (lpnt->isLoop) {
    loopy = (0 == loi && 0 == hii);
    if (hii < loi) {
      shi += lpnt->num;
    }
  } else {
    if (0 == loi && 0 == hii) {
      biffAddf(LIMN, "%s: can only have loi == hii == 0 with point loop", me);
      return 1;
    }
    if (hii < loi) {
      biffAddf(LIMN, "%s: can only have hii (%u) < loi (%u) in a point loop", me, hii,
               loi);
      return 1;
    }
    loopy = AIR_FALSE;
  }
  spanlen = shi - slo + 1;
  if (spanlen <= 1 && !loopy) {
    biffAddf(LIMN, "%s: [%u,%u]->[%d,%d] span length %d <= 1 but not in loop", me, loi,
             hii, slo, shi, spanlen);
    return 1;
  }
  /* all's well, set output values */
  *sloP = slo;
  *shiP = shi;
  *loopyP = loopy;
  return 0;
}

static void
subnorm2(double dir[2], const double aa[2], const double bb[2]) {
  double len;
  ELL_2V_SUB(dir, aa, bb);    /* dir = aa - bb */
  ELL_2V_NORM(dir, dir, len); /* normalize(dir) */
}

/* limnCBFFindTVT: Find constraints for spline fitting: incoming/left tangent lt, center
or endpoint vertex vv, outgoing/right tangent rt; any but not all can be NULL. These are
computed from the given points lpnt, at offset index ofi within index range loi, hii
and only that range: that range is probably delimited by corners, and we have to be blind
to anything past the corners on either side of us (except if loi==hii==0, in which case
we can look at all the points in a loop).

NOTE: this assumes that limnCBFCtxPrep(fctx, lpnt) was called without error!
It (via ctxBuffersSet) allocates things that we depend on here
*/
int /* Biff: 1 */
limnCBFFindTVT(double lt[2], double vv[2], double rt[2], const limnCBFCtx *fctx,
               const limnCBFPoints *lpnt, uint loi, uint hii, uint ofi, int oneSided) {
  static const char me[] = "limnCBFFindTVT";
  /* we use here (signed) int for things that might seem better as uint, but it
     simplifies implementing arithmetic and comparisons given how indices wrap around in
     point loops */
  int slo, /* signed version of loi */
    shi,   /* signed version of hii, but shi = hii + lpnt->num if hii < loi in loop */
    sof,   /* signed versions of ofi */
    loopy, /* lpnt->isLoop && loi == 0 && hii = 0, i.e. there are no bounds on indices */
    pnum,  /* total number of points in lpnts */
    spanlen, /* span length: number of points in [loi,hii] */
    icent, iplus,
    imnus; /* icent is the actual data index corresponding to loi + ofi; it is used for
              both the scale==0 and scale>0 cases; iplus and imnus are only needed with
              scale==0, but too annoying to break those out into that specific branch */

  if (!((lt || vv || rt) && fctx && lpnt)) {
    biffAddf(LIMN, "%s: got NULL pointer (or too many NULL pointers)", me);
    return 1;
  }
  /* so: each of lt, vv, rt can be NULL (they just can't be all NULL) */
  if (idxPrep(&slo, &shi, &loopy, lpnt, loi, hii)) {
    biffAddf(LIMN, "%s: trouble with loi %u or hii %u", me, loi, hii);
    return 1;
  }
  spanlen = shi - slo + 1;
  pnum = AIR_INT(lpnt->num);

  /* now:
  0 == slo == shi implies lpnt->isLoop (and this is called "loopy")
  slo == shi != 0 is always impossible
  always: slo < shi (even if given hii < loi), and thus any indices in range [slo,shi]
  need to be mod'd with pnum before indexing into PP(lpnt)
  spanlen >= 2 */
  if (!loopy && !(ofi < AIR_UINT(spanlen))) {
    biffAddf(LIMN,
             "%s: ofi %u too high for [%u,%u]->[%d,%d] span length %d (not in loop)", me,
             ofi, loi, hii, slo, shi, spanlen);
    return 1;
  }
  /* now ofi is a valid index in [0,..,spanlen-1] */
  sof = AIR_INT(ofi);
  icent = slo + sof;
  iplus = icent + 1;
  imnus = icent - 1;
  if (!loopy) {
    /* this is the code that motivated if (hii < loi) shi += pnum;
       otherwise clamping is too annoying */
    icent = AIR_CLAMP(slo, icent, shi);
    iplus = AIR_CLAMP(slo, iplus, shi);
    imnus = AIR_CLAMP(slo, imnus, shi);
  }
  /* now map to actual indices */
  icent = AIR_MOD(icent, pnum);
  iplus = AIR_MOD(iplus, pnum);
  imnus = AIR_MOD(imnus, pnum);
  if (0 == fctx->scale) {
    /* DIM=2 through-out */
    const double *xyC = PP(lpnt) + 2 * icent, *xyP, *xyM;
    if (vv) {
      ELL_2V_COPY(vv, xyC);
    }
    /* printf("!%s: iplus=%u  imnus=%u  xy=%g %g\n", me, iplus, imnus, xy[0], xy[1]); */
    xyP = PP(lpnt) + 2 * iplus;
    xyM = PP(lpnt) + 2 * imnus;
    if (rt) {
      subnorm2(rt, xyP, oneSided ? xyC : xyM);
    }
    if (lt) {
      subnorm2(lt, xyM, oneSided ? xyC : xyP);
    }
    /* printf("!%s: xyP=%g %g   xyM=%g %g  len=%g  tt=%g %g\n", me, xyP[0], xyP[1],
       xyM[0], xyM[1], len, tt[0], tt[1]); */
  } else {
    /* using scale>0 for endpoint and tangent estimation */
    /* for simplicity: regardless of dir, we compute average positions for points
       centered around loi + ofi (posC), and for lower/higher indices (posM/posP) */
    double posM[2] = {0, 0}, posC[2] = {0, 0}, posP[2] = {0, 0};
    const double *vw = fctx->vw;
    const double *tw = fctx->tw;
    int lim = (int)fctx->wLen - 1, /* limit on loop index */
      ci;                          /* loops through [-lim,lim] */
    if (!(vw && tw)) {
      biffAddf(LIMN, "%s: fctx internal buffers vw and tw not both allocated", me);
      return 1;
    }
    if (tw[0] != 0) {
      biffAddf(LIMN, "%s: first tangent weight fctx->tw[0] %g not zero", me, tw[0]);
      return 1;
    }
    for (ci = -lim; ci <= lim; ci++) {
      uint wi = abs(ci);       /* weight index into vw, tw */
      int di = slo + sof + ci; /* signed index into data */
      const double *xy;
      if (!loopy) di = AIR_CLAMP(slo, di, shi);
      di = AIR_MOD(di, pnum);
      xy = PP(lpnt) + 2 * di;
      ELL_2V_SCALE_INCR(posC, vw[wi], xy);
      /* printf("!%s: (ci=%d/wi=%u) posC += %g*(%g %g) --> %g %g\n", me, ci, wi,
         vw[wi], xy[0], xy[1], posC[0], posC[1]); */
      if (ci < 0) {
        ELL_2V_SCALE_INCR(posM, tw[wi], xy);
        /* printf("!%s: (ci=%d/wi=%u) posM += %g*(%g %g) --> %g %g\n", me, ci, wi,
           tw[wi], xy[0], xy[1], posM[0], posM[1]); */
      }
      if (ci > 0) {
        ELL_2V_SCALE_INCR(posP, tw[wi], xy);
        /* printf("!%s: (ci=%d/wi=%u) posP += %g*(%g %g) --> %g %g\n", me, ci, wi,
           tw[wi], xy[0], xy[1], posP[0], posP[1]); */
      }
    }
    {
      /* limit distance from chosen (x,y) datapoint to posC to be (HEY harcoded) 95% of
         fctx->epsilon. Being allowed to be further away can cause annoyances (for GLK in
         some early stage of debugging) */
      double off[2], offlen, okoff = 0.95 * fctx->epsilon; /* DIM=2 throughout */
      const double *xy = PP(lpnt) + 2 * icent; /* center vertex in given data */
      ELL_2V_SUB(off, posC, xy); /* off = posC - xy, from given to computed */
      ELL_2V_NORM(off, off, offlen);
      offlen = AIR_MIN(okoff, offlen);
      /* difference between chosen (x,y) datapoint and spline endpoint
         can be in any direction, but we limit the length */
      ELL_2V_SCALE_ADD2(posC, 1, xy, offlen, off);
    }
    if (lt) {
      subnorm2(lt, posM, oneSided ? posC : posP);
    }
    if (rt) {
      subnorm2(rt, posP, oneSided ? posC : posM);
    }
    if (vv) {
      ELL_2V_COPY(vv, posC);
    }
  }
  return 0;
}

/*
******** limnCBFit
**
** top-level function for fitting cubic beziers to given points
*/
int /* Biff: 1 */
limnCBFit(limnCBFPath *pathWhole, limnCBFCtx *fctx, const limnCBFPoints *lpnt) {
  static const char me[] = "limnCBFit";
  uint *cornIdx, /* (if non-NULL) array of logical indices into PP(lpnt) of corners */
    cornNum,     /* length of cornIdx array */
    cii;
  int ret;
  airArray *mop;

  if (!(pathWhole && fctx && lpnt)) {
    biffAddf(LIMN, "%s: got NULL pointer", me);
    return 1;
  }
  if (limnCBFCtxPrep(fctx, lpnt)) {
    biffAddf(LIMN, "%s: trouble preparing", me);
    return 1;
  }
  if (fctx->cornerFind) {
    /* HEY RESURRECT ME
    if (limnCBFCorners(&cornIdx, &cornNum, fctx, lpnt)) {
      biffAddf(LIMN, "%s: trouble finding corners", me);
      return 1;
    }
    */
  } else {
    if (lpnt->isLoop) {
      /* there really are no corners */
      cornIdx = NULL;
      cornNum = 0;
    } else {
      /* even without "corners": if not a loop, first and last verts act as corners */
      cornIdx = AIR_CALLOC(2, uint);
      assert(cornIdx);
      cornNum = 2;
      cornIdx[0] = 0;
      cornIdx[1] = lpnt->num - 1;
    }
  }
  mop = airMopNew();
  if (cornIdx) {
    airMopAdd(mop, cornIdx, airFree, airMopAlways);
  }
  airArrayLenSet(pathWhole->segArr, 0);
  if (!cornNum) {
/* no corners; do everything with one multi call */
#if 0 /* HEY RESURRECT ME */
    if (limnCBFMulti(pathWhole, fctx, NULL, NULL, NULL, NULL, lpnt, 0 /* loi */,
                     0 /* hii */)) {
      biffAddf(LIMN, "%s: trouble", me);
      airMopError(mop);
      return 1;
    }
#endif
  } else {
    /* do have corners: split points into segments between corners. The corner vertex is
       both last point in segment I and first point in segment I+1 */
    for (cii = 0; cii < cornNum; cii++) {
      uint cjj = (cii + 1) % cornNum;
      uint loi = cornIdx[cii];
      uint hii = cornIdx[cjj];
      limnCBFPath *subpath = limnCBFPathNew();
#if 0 /* HEY RESURRECT ME */
        ret = limnCBFMulti(subpath, fctx, NULL, NULL, NULL, NULL, lpnt, loi, hii);
        if (ret) {
          biffAddf(LIMN, "%s: trouble from corners [%u,%u] (points [%u,%u])", me, cii,
        cjj, loi, hii); limnCBFPathNix(subpath); airMopError(mop); return 1;
        }
#endif
      subpath->seg[0].corner[0] = 1;
      subpath->seg[subpath->segNum - 1].corner[1] = 1;
      limnCBFPathJoin(pathWhole, subpath);
      limnCBFPathNix(subpath);
    }
  }

  pathWhole->isLoop = lpnt->isLoop;
  airMopOkay(mop);
  return 0;
}

/*
TODO:

-- avoid repeated calls to limnCBFCtxPrep
-- when should lenF2L be replaced by posStdv?

test logic at end of limnCBFFindTVT about capping distance between VV and given pos
test limnCBFFindTVT with span length as low as 2
debug limnCBFitSingle with various conditions

testing corners: corners near and at start==stop of isLoop
corners not at start or stop of isLoop: do spline wrap around from last to first index?

do profiling to see if non-error-checking version of limnCBFFindVT is warranted

use performance tests to explore optimal settings in fctx:
  nrpIterMax, nrpCap, nrpIota, nrpPsi, nrpDeltaThresh
evaluated in terms of time and #splines needed for fit
(may want to pay in time for more economical representation)

"What GLK hasn't thought through is: what is the interaction of nrp iterations and
findalpha generating the simple arc on some but not all iterations (possibly
unstable?)"

resurrect in segment struct: how many points were represented by a single spline

reparm: "HEY TODO: need to make sure that half-way between points,
     spline isn't wildly diverging; this can happen with the
     spline making a loop away from a small number of points, e.g.:
     4 points spline defined by vv0 = (1,1), tt1 = (1,2),
     tt2 = (1,2), vv3 = (0,1)"

valgrind everything

remove big debugging comment blocks

(DIM=2) explore what would be required to generalized from 2D to 3D,
perhaps at least at the API level, even if 3D is not yet implemented
*/
