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
limnCBFPointsNew

create a point data container, possibly around given pdata pointer. In an aspirational
hope of API stability, this is one of the few functions for which the interface itself
does not expose the specificity to DIM=2 and type double (though the code inside
does (apologetically) enforce that).
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
  seg->pointNum = 0;
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
  fctx->nrpIterMax = 12;
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
ctxBuffersSet: ensures that some buffers in fctx: uu, vw, tw are set up for current
#points pNum and measurement scale fctx->scale. The buffers are re-allocated only when
necessary. Does NOT touch the corner-related buffers: cpp, clt, crt
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
    about the confidence that the nrrdKernelDiscreteGaussian is working as expected
    (specifically: its weights should really sum to unity), rather than something about
    tuning cubic spline fitting. We could compare fctx->scale with
    nrrdKernelDiscreteGaussianGoodSigmaMax but that is set conservatively low (only 6, as
    of June 2024) */
    const double one = 0.99;
    /* if vw and tw are allocated for length wlbig (or bigger) something isn't right */
    const uint wlbig = 128;
    /* if the initial weights for the tangent computation sum to smaller than this
       (they will be later normalized to sum to 1) then something isn't right */
    const double tinysum = 1.0 / 64;
    double kw, kparm[2], vsum, tsum;
    uint wlen;
    /* else need to (possibly allocate and) set vw and tw buffers */
    kparm[0] = scale;
    kparm[1] = 1000000; /* effectively no cut-off; sanity check comes later */
    ii = 0;
    vsum = 0;
    do {
      kw = nrrdKernelDiscreteGaussian->eval1_d(ii, kparm);
      kw = fabs(kw); /* should be moot, but discrete kernels aren't bullet-proof */
      vsum += (!ii ? 1 : 2) * kw;
      if (fctx->verbose > 1) {
        printf("%s: kw[%u] = %g --> vsum = %g\n", me, ii, kw, vsum);
      }
      ii++;
    } while (vsum < one && kw);
    /* wlen = intended length of blurring kernel weight vectors */
    wlen = ii;
    if (wlen > wlbig) {
      biffAddf(LIMN,
               "%s: weight buffer length %u (from scale %g) seems "
               "too large",
               me, wlen, scale);
      return 1;
    }
    if (2 * wlen > pNum) {
      /* note: #verts involved in computation == 2*wlen - 1, so this is only going to
         complain only when ~all the verts are contributing to computations for each
         vertex, which is pretty excessive */
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
       1 = sum_i(tw[i]) for i=0...len-1 */
    vsum = tsum = 0;
    for (ii = 0; ii < wlen; ii++) {
      double kw = nrrdKernelDiscreteGaussian->eval1_d(ii, kparm);
      vsum += (!ii ? 1 : 2) * (fctx->vw[ii] = kw);
      tsum += (fctx->tw[ii] = ii * kw);
    }
    if (tsum < tinysum) {
      biffAddf(LIMN,
               "%s: scale %g led to tiny unnormalized tangent weight sum %g; "
               "purpose of scale is to do blurring but scale %g won't do that",
               me, scale, tsum, scale);
      return 1;
    }
    if (vsum < tinysum) {
      biffAddf(LIMN, "%s: scale %g led to unexpected tiny vertex weight sum %g", me,
               scale, vsum);
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
limnCBFCtxPrep

checks the things that are going to be passed around a lot, and makes call to initialize
buffers inside fctx
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
  if (!(fctx->nrpIterMax >= 1)) {
    biffAddf(LIMN, "%s: need at least 1 nrp iteration (not %u)", me, fctx->nrpIterMax);
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
limnCBFSegEval

evaluates a single limnCBFSeg at one point tt in [0.0,1.0]
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
limnCBFPathSample

evaluates limnCBFPath at pNum locations, uniformly (and very naively) distributed among
the path segments, and saves into (pre-allocated) xy
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

/* error-checked index processing for limnCBFFindTVT and maybe others */
static int /* Biff: 1 */
idxPrep(int *sloP, int *shiP, int *loopyP, const limnCBFCtx *fctx,
        const limnCBFPoints *lpnt, uint loi, uint hii) {
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
  if (fctx->verbose > 1) {
    printf("%s: span as uint [%u,%u] -> int [%d,%d]\n", me, loi, hii, slo, shi);
  }
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

/*
limnCBFFindTVT: Find constraints for spline fitting: incoming/left tangent lt, center or
endpoint vertex vv, outgoing/right tangent rt; any but not all can be NULL. These are
computed from the given points lpnt, at offset index ofi within index range [loi, hii]
and only that range: that range is probably delimited by corners, and we have to be blind
to anything past the corners on either side of us (except if loi==hii==0 in a loop, in
which case we can look at all the points).

Given that this is the inner-loop of other things, it would make sense to have a
non-public version without all the error checking, but given the birthing pains of this
code, the error-checking is a safety-net, and is welcome until profiling shows it is
actually a problem.

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
  if (fctx->verbose > 1) {
    printf("%s: hello: %u in [%u,%u] in %sloop with %u points (%s-sided)\n", me, ofi,
           loi, hii, lpnt->isLoop ? "" : "NON-", lpnt->num, oneSided ? "1" : "2");
  }
  /* so: each of lt, vv, rt can be NULL (they just can't be all NULL) */
  if (idxPrep(&slo, &shi, &loopy, fctx, lpnt, loi, hii)) {
    biffAddf(LIMN, "%s: trouble with loi %u or hii %u", me, loi, hii);
    return 1;
  }
  spanlen = shi - slo + 1;
  pnum = AIR_INT(lpnt->num);
  if (fctx->verbose) {
    printf("%s: %d pnts: [%u,%u]->[%d,%d] (len=%d) (loopy=%u)\n", me, pnum, loi, hii,
           slo, shi, spanlen, loopy);
  }

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
    xyP = PP(lpnt) + 2 * iplus;
    xyM = PP(lpnt) + 2 * imnus;
    if (rt) {
      subnorm2(rt, xyP, oneSided ? xyC : xyM);
    }
    if (lt) {
      subnorm2(lt, xyM, oneSided ? xyC : xyP);
    }
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
      uint wi = abs(ci);     /* weight index into vw, tw */
      int adi,               /* actual data index */
        di = slo + sof + ci; /* signed (and not %-ed) index into data */
      const double *xy;
      if (!loopy) di = AIR_CLAMP(slo, di, shi);
      adi = AIR_MOD(di, pnum);
      xy = PP(lpnt) + 2 * adi;
      ELL_2V_SCALE_INCR(posC, vw[wi], xy);
      if (fctx->verbose > 1) {
        printf("%s: ci=%d (in [%d,%d]) --> di=%d --> adi=%d; v,t weights %g,%g\n", me,
               ci, -lim, lim, di, adi, vw[wi], tw[wi]);
      }
      if (ci < 0) {
        ELL_2V_SCALE_INCR(posM, tw[wi], xy);
      }
      if (ci > 0) {
        ELL_2V_SCALE_INCR(posP, tw[wi], xy);
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

/* utility function for counting how many vertices are in index span [loi,hii] inclusive.
It is not our job here to care about lpnt->isLoop; we just assume that if we're faced
with hii<loi, it must be because of a loop */
static uint
spanLength(const limnCBFPoints *lpnt, uint loi, uint hii) {
  uint topi = hii + (hii < loi) * (lpnt->num);
  return topi - loi + 1;
}

/* utility function for getting pointer to coords in lpnt, for point with index loi+ofi,
while accounting for possibility of wrapping */
static const double *
pointPos(const limnCBFPoints *lpnt, uint loi, uint ofi) {
  uint ii = (loi + ofi) % lpnt->num;
  return PP(lpnt) + 2 * ii; /* DIM=2 */
}

/*
(from paper page 620) solves for the alpha that minimize squared error between xy[i]
and Q(uu[i]) where Q(t) is cubic Bezier spline through vv0, vv0 + alpha[0]*tt1,
vv3 + alpha[1]*tt2, and vv3.

There are various conditions where the generated spline ignores the xy array and
instead is what one could call a "simple arc" (with control points at 1/3 and 2/3 the
distance between the end points):
 - having only two points (xy contains only the end points)
 - the determinant of the 2x2 matrix that is inverted to solve
   for alpha is too close to zero (this test was not part of the
   author's code)
 - the solved alphas are not convincingly positive

This function is the only place where the "simple arc" is generated, and generating the
simple arc is not actually an error or problem: if it is bad at fitting the data (as
determined by findDist) then it may be subdivided, and that's ok. What GLK hasn't thought
through is: what is the interaction of nrp iterations and findAlpha generating the simple
arc on some but not all iterations (possibly unstable?)
*/
static void
findAlpha(double alpha[2], limnCBFCtx *fctx, /* must be non-NULL */
          const double vv0[2], const double tt1[2], const double tt2[2],
          const double vv3[2], const limnCBFPoints *lpnt, uint loi, uint hii) {
  static const char me[] = "findAlpha";
  uint ii, spanlen;
  double det, F2L[2], lenF2L;
  const double *xy = PP(lpnt);

  ELL_2V_SUB(F2L, xy + 2 * hii, xy + 2 * loi); /* DIM=2 throughout this */
  lenF2L = ELL_2V_LEN(F2L);
  spanlen = spanLength(lpnt, loi, hii);
  if (spanlen > 2) {
    double xx[2], m11, m12, m22, MM[4], MI[4];
    const double *uu = fctx->uu;
    xx[0] = xx[1] = m11 = m12 = m22 = 0;
    for (ii = 0; ii < spanlen; ii++) {
      const double *xy;
      double bb[4], Ai1[2], Ai2[2], Pi[2], dmP[2];
      double ui = uu[ii];
      VCBD0(bb, ui);
      ELL_2V_SCALE(Ai1, bb[1], tt1);
      ELL_2V_SCALE(Ai2, bb[2], tt2);
      /* GLK using "m" and "M" instead of author's "C". Note that Ai1 and Ai2 are
         scalings of (nominally) unit-length tt1 and tt2 by evaluations of the spline
         basis functions, so they (and the M computed from them, and det(M)), are
         invariant w.r.t over-all rescalings of the data points */
      m11 += ELL_2V_DOT(Ai1, Ai1);
      m12 += ELL_2V_DOT(Ai1, Ai2);
      m22 += ELL_2V_DOT(Ai2, Ai2);
      ELL_2V_SCALE_ADD2(Pi, bb[0] + bb[1], vv0, bb[2] + bb[3], vv3);
      xy = pointPos(lpnt, loi, ii);
      ELL_2V_SUB(dmP, xy, Pi);
      xx[0] += ELL_2V_DOT(dmP, Ai1);
      xx[1] += ELL_2V_DOT(dmP, Ai2);
    }
    ELL_4V_SET(MM, m11, m12, m12, m22);
    ELL_2M_INV(MI, MM, det);
    ELL_2MV_MUL(alpha, MI, xx);
  } else {                   /* pNum <= 2 */
    det = 1;                 /* bogus but harmless */
    alpha[0] = alpha[1] = 0; /* trigger simple arc code */
  }
  /* test if we should return simple arc */
  if (!(AIR_EXISTS(det) && AIR_ABS(det) > fctx->detMin
        && alpha[0] > lenF2L * (fctx->alphaMin)
        && alpha[1] > lenF2L * (fctx->alphaMin))) {
    if (fctx->verbose) {
      if (spanlen > 2) {
        printf("%s: bad |det| %g (vs %g) or alpha %g,%g (vs %g*%g) "
               "--> simple arc\n",
               me, AIR_ABS(det), fctx->detMin, alpha[0], alpha[1], lenF2L,
               fctx->alphaMin);
      } else {
        printf("%s: [%u,%u] spanlen %u tiny --> simple arc\n", me, loi, hii, spanlen);
      }
    }
    /* generate simple arc: set both alphas to 1/3 of distance from
       first to last point, but also handle non-unit-length tt1 and
       tt2 */
    alpha[0] = lenF2L / (3 * ELL_2V_LEN(tt1));
    alpha[1] = lenF2L / (3 * ELL_2V_LEN(tt2));
  } else {
    if (fctx->verbose > 1) {
      printf("%s: all good: det %g, alpha %g,%g\n", me, det, alpha[0], alpha[1]);
    }
  }
  fctx->alphaDet = det;
  return;
}

/*
using Newton iterations to try to find a better places at which to evaluate the spline in
order to match the given points xy
*/
static double
reparm(const limnCBFCtx *fctx, /* must be non-NULL */
       const double alpha[2], const double vv0[2], const double tt1[2],
       const double tt2[2], const double vv3[2], const limnCBFPoints *lpnt, uint loi,
       uint hii) {
  static const char me[] = "reparm";
  uint ii, spanlen;
  double vv1[2], vv2[2], delta, maxdelu;
  double *uu = fctx->uu;

  spanlen = spanLength(lpnt, loi, hii);
  assert(spanlen >= 3);
  /* average interior u[i+1]-u[i] is 1/(spanlen-2) */
  maxdelu = fctx->nrpCap / (spanlen - 2);
  ELL_2V_SCALE_ADD2(vv1, 1, vv0, alpha[0], tt1);
  ELL_2V_SCALE_ADD2(vv2, 1, vv3, alpha[1], tt2);
  delta = 0;
  /* only changing parameterization of interior points,
     not the first (ii=0) or last (ii=pNum-1) */
  for (ii = 1; ii < spanlen - 1; ii++) {
    double numer, denom, delu, df[2], ww[4], tt, Q[2], QD[2], QDD[2];
    const double *xy;
    tt = uu[ii];
    CBD0(Q, vv0, vv1, vv2, vv3, tt, ww);
    CBD1(QD, vv0, vv1, vv2, vv3, tt, ww);
    CBD2(QDD, vv0, vv1, vv2, vv3, tt, ww);
    xy = pointPos(lpnt, loi, ii);
    ELL_2V_SUB(df, Q, xy);
    numer = ELL_2V_DOT(df, QD);
    denom = ELL_2V_DOT(QD, QD) + ELL_2V_DOT(df, QDD);
    delu = numer / denom;
    if (AIR_ABS(delu) > maxdelu) {
      /* cap Newton step */
      delu = maxdelu * airSgn(delu);
    }
    uu[ii] = tt - delu;
    delta += AIR_ABS(delu);
    if (fctx->verbose > 1) {
      printf("%s[%2u]: %g <-- %g - %g\n", me, ii, uu[ii], tt, delu);
    }
  }
  delta /= spanlen - 2; /* number of interior points */
  /* HEY TODO: need to make sure that half-way between points,
     spline isn't wildly diverging; this can happen with the
     spline making a loop away from a small number of points, e.g.:
     4 points spline defined by vv0 = (1,1), tt1 = (1,2),
     tt2 = (1,2), vv3 = (0,1) */
  return delta;
}

/* (assuming current parameterization in fctx->uu) sets fctx->distMax to max distance
   to spline, at point fctx->distMaxIdx, and then sets fctx->distBig accordingly */
static void
findDist(limnCBFCtx *fctx, const double alpha[2], const double vv0[2],
         const double tt1[2], const double tt2[2], const double vv3[2],
         const limnCBFPoints *lpnt, uint loi, uint hii) {
  uint ii, distMaxIdx, spanlen;
  double vv1[2], vv2[2], distMax;
  const double *uu = fctx->uu;

  spanlen = spanLength(lpnt, loi, hii);
  assert(spanlen >= 3);
  ELL_2V_SCALE_ADD2(vv1, 1, vv0, alpha[0], tt1); /* DIM=2 everywhere here */
  ELL_2V_SCALE_ADD2(vv2, 1, vv3, alpha[1], tt2);
  distMax = AIR_NAN;
  /* NOTE that the first and last points are actually not part of the max distance
     calculation, which motivates ensuring that the endpoints generated by limnCBFFindTVT
     are actually sufficiently close to the first and last points (or else the fit spline
     won't meet the expected accuracy threshold) */
  for (ii = 1; ii < spanlen - 1; ii++) {
    double len, Q[2], df[2], ww[4];
    const double *xy;
    CBD0(Q, vv0, vv1, vv2, vv3, uu[ii], ww);
    xy = pointPos(lpnt, loi, ii);
    ELL_2V_SUB(df, Q, xy);
    len = ELL_2V_LEN(df);
    if (!AIR_EXISTS(distMax) || len > distMax) {
      distMax = len;
      distMaxIdx = ii;
    }
  }
  fctx->distMax = distMax;
  fctx->distMaxIdx = distMaxIdx;
  fctx->distBig = (distMax <= fctx->nrpIota * fctx->epsilon
                     ? 0
                     : (distMax <= fctx->epsilon /* */
                          ? 1
                          : (distMax <= fctx->nrpPsi * fctx->epsilon /* */
                               ? 2
                               : 3)));
  return;
}

/*
fitSingle: fits a single cubic Bezier spline, w/out error checking (limnCBFSingle is a
wrapper around this). The given points coordinates are in limnCBFPoints lpnt, between
low/high indices loi/hii (inclusively); hii can be < loi in the case of a point loop.
From *given* initial endpoint vv0, initial tangent tt1, final tangent tt2 (pointing
backwards), and final endpoint vv3, the job of this function is actually just to set
alpha[0],alpha[1] such that the cubic Bezier spline with control points vv0,
vv0+alpha[0]*tt1, vv3+alpha[1]*tt2, vv3 approximates all the given points.

This is an iterative process, in which alpha is solved for multiples times, after taking
a Newton step to try to optimize the parameterization of the points (in the
already-allocated fctx->uu array); we calls this process "nrp" for Newton
Re-Parameterization. nrp iterations are stopped after any one of following is true (the
original published method did not have these fine-grained controls):
 - have done nrpIterMax iterations of nrp, or,
 - distance from spline (as evaluated at the current parameterization) to the given
   points falls below fctx->nrpIota * fctx->epsilon, or,
 - parameterization change falls below fctx->nrpDeltaThresh
Information about the results of this process are set in the given fctx.
*/
static void
fitSingle(double alpha[2], limnCBFCtx *fctx, const double vv0[2], const double tt1[2],
          const double tt2[2], const double vv3[2], const limnCBFPoints *lpnt, uint loi,
          uint hii) {
  static const char me[] = "fitSingle";
  uint iter, pNum;

  /* DIM=2 pretty much everywhere here */
  if (fctx->verbose) {
    printf("%s[%d,%d]: hello, vv0=(%g,%g), tt1=(%g,%g), "
           "tt2=(%g,%g), vv3=(%g,%g)\n",
           me, loi, hii, vv0[0], vv0[1], tt1[0], tt1[1], tt2[0], tt2[1], vv3[0], vv3[1]);
  }
  pNum = spanLength(lpnt, loi, hii);
  if (2 == pNum) {
    /* relying on code in findAlpha() that handles pNum==2 */
    findAlpha(alpha, fctx, vv0, tt1, tt2, vv3, lpnt, loi, hii);
    /* nrp is moot */
    fctx->nrpIterDone = 0;
    /* emmulate results of calling findDist() */
    fctx->distMax = fctx->nrpDeltaDone = 0;
    fctx->distMaxIdx = 0;
    fctx->distBig = 0;
  } else {        /* pNum >= 3 */
    double delta; /* avg parameterization change of interior points */
    {
      /* initialize uu parameterization to chord length */
      unsigned int ii;
      double len;
      const double *xyP, *xyM;
      fctx->uu[0] = len = 0;
      xyP = pointPos(lpnt, loi, 1);
      xyM = pointPos(lpnt, loi, 0);
      for (ii = 1; ii < pNum; ii++) {
        double dd[2];
        ELL_2V_SUB(dd, xyP, xyM);
        len += ELL_2V_LEN(dd);
        fctx->uu[ii] = len;
        xyM = xyP;
        xyP = pointPos(lpnt, loi, ii + 1);
      }
      delta = 0;
      for (ii = 0; ii < pNum; ii++) {
        if (ii < pNum - 1) {
          fctx->uu[ii] /= len;
          delta += fctx->uu[ii];
        } else {
          /* ii == pNum-1 last vertex */
          fctx->uu[ii] = 1;
        }
        if (fctx->verbose > 1) {
          printf("%s[%d,%d]: intial uu[%u] = %g\n", me, loi, hii, ii, fctx->uu[ii]);
        }
      }
      delta /= pNum - 2; /* within the pNum verts are pNum-2 interior verts */
      if (fctx->verbose) {
        printf("%s[%d,%d]: initial (chord length) delta = %g\n", me, loi, hii, delta);
      }
    }
    findAlpha(alpha, fctx, vv0, tt1, tt2, vv3, lpnt, loi, hii);
    findDist(fctx, alpha, vv0, tt1, tt2, vv3, lpnt, loi, hii);
    if (fctx->verbose) {
      printf("%s[%d,%d]: found alpha %g %g, dist %g @ %u (big %d) (%u max nrp iters)\n",
             me, loi, hii, alpha[0], alpha[1], fctx->distMax, fctx->distMaxIdx,
             fctx->distBig, fctx->nrpIterMax);
    }
    if (fctx->distBig < 3) {
      /* initial fit isn't awful; try making it better with nrp */
      int converged = AIR_FALSE;
      for (iter = 0; fctx->distBig && iter < fctx->nrpIterMax; iter++) {
        if (fctx->verbose) {
          printf("%s[%d,%d]: nrp iter %u starting with alpha %g,%g (det %g) (big %d)\n",
                 me, loi, hii, iter, alpha[0], alpha[1], fctx->alphaDet, fctx->distBig);
        }
        delta = reparm(fctx, alpha, vv0, tt1, tt2, vv3, lpnt, loi, hii);
        findAlpha(alpha, fctx, vv0, tt1, tt2, vv3, lpnt, loi, hii);
        findDist(fctx, alpha, vv0, tt1, tt2, vv3, lpnt, loi, hii);
        if (fctx->verbose) {
          printf("%s[%d,%d]: nrp iter %u (reparm) delta = %g (big %d)\n", me, loi, hii,
                 iter, delta, fctx->distBig);
        }
        if (delta <= fctx->nrpDeltaThresh) {
          if (fctx->verbose) {
            printf("%s[%d,%d]: nrp iter %u delta %g <= min %g --> break\n", me, loi, hii,
                   iter, delta, fctx->nrpDeltaThresh);
          }
          converged = AIR_TRUE;
          break;
        }
      }
      if (fctx->verbose) {
        printf("%s[%d,%d]: nrp done after %u iters: ", me, loi, hii, iter);
        if (converged) {
          printf("converged!\n");
        } else if (!fctx->distBig) {
          printf("nice small dist %g @ %u\n", fctx->distMax, fctx->distMaxIdx);
        } else {
          printf("hit itermax %u\n", fctx->nrpIterMax);
        }
      }
      fctx->nrpIterDone = iter;
    } else {
      /* else fctx->distBig == 3: dist so big that we don't even try nrp */
      fctx->nrpIterDone = 0;
      if (fctx->verbose) {
        printf("%s[%d,%d]: such big (%d) dist %g > %g we didn't try nrp\n", me, loi, hii,
               fctx->distBig, fctx->distMax, fctx->nrpPsi * fctx->epsilon);
      }
    }
    fctx->nrpDeltaDone = delta;
  }
  if (fctx->verbose) {
    printf("%s[%d,%d]: leaving with alpha %g %g\n", me, loi, hii, alpha[0], alpha[1]);
  }
  return;
}

/*
limnCBFitSingle

builds a limnCBFPoints around given xy, determines spline constraints if necessary, and
calls fitSingle
*/
int /* Biff: 1 */
limnCBFitSingle(limnCBFSeg *seg, const double vv0[2], const double tt1[2],
                const double tt2[2], const double vv3[2], limnCBFCtx *_fctx,
                const double *xy, uint pNum) {
  static const char me[] = "limnCBFitSingle";
  double v0c[2], t1c[2], t2c[2], v3c[2]; /* locally computed geometry info */
  double alpha[2];                       /* recovered by fitting */
  const double *v0p, *t1p, *t2p, *v3p;   /* pointers for the geometry info */
  uint loi, hii;
  limnCBFCtx *fctx;
  limnCBFPoints *lpnt;
  airArray *mop;

  if (!(seg && xy && pNum)) {
    biffAddf(LIMN, "%s: got NULL pointer or 0 points", me);
    return 1;
  }
  mop = airMopNew();
  lpnt = limnCBFPointsNew(xy, nrrdTypeDouble, 2, pNum, AIR_FALSE /* isLoop */);
  airMopAdd(mop, lpnt, (airMopper)limnCBFPointsNix, airMopAlways);
  loi = 0;
  hii = pNum - 1;
  if (_fctx) {
    fctx = _fctx; /* caller has supplied info */
  } else {
    fctx = limnCBFCtxNew();
    airMopAdd(mop, fctx, (airMopper)limnCBFCtxNix, airMopAlways);
    fctx->scale = 0;
    fctx->epsilon = 0.01; /* HEY maybe instead some multiple of stdv? */
    /* (if you don't like these hard-coded defaults then pass your own fctx) */
  }
  if (limnCBFCtxPrep(fctx, lpnt)) {
    biffAddf(LIMN, "%s: problem with %s fctx or lpnt", me, _fctx ? "given" : "own");
    airMopError(mop);
    return 1;
  }
  if (vv0 && tt1 && tt2 && vv3) {
    /* point to the given containers of geometry info */
    v0p = vv0; /* DIM=2 ? */
    t1p = tt1;
    t2p = tt2;
    v3p = vv3;
  } else {
    int oneSided = AIR_TRUE;
    if (vv0 || tt1 || tt2 || vv3) {
      biffAddf(LIMN,
               "%s: should either give all vv0,tt1,tt2,vv3 or none (not %p,%p,%p,%p)",
               me, (const void *)vv0, (const void *)tt1, (const void *)tt2,
               (const void *)vv3);
      airMopError(mop);
      return 1;
    }
    /* have NO given geometry info; must find it all */
    if (limnCBFFindTVT(NULL, v0c, t1c, /* */
                       fctx, lpnt, loi, hii, 0 /* ofi */, oneSided)
        || limnCBFFindTVT(t2c, v3c, NULL, /* */
                          fctx, lpnt, loi, hii, hii /* ofi */, oneSided)) {
      biffAddf(LIMN, "%s: trouble finding geometry info", me);
      airMopError(mop);
      return 1;
    }
    if (fctx->verbose) {
      printf("%s: found geometry (%g,%g) --> (%g,%g) -- (%g,%g) <-- (%g,%g)\n", me,
             v0c[0], v0c[1], t1c[0], t1c[1], t2c[0], t2c[1], v3c[0], v3c[1]);
    }
    v0p = v0c;
    t1p = t1c;
    t2p = t2c;
    v3p = v3c;
  }
  fitSingle(alpha, fctx, v0p, t1p, t2p, v3p, lpnt, loi, hii);

  ELL_2V_COPY(seg->xy + 0, v0p);
  ELL_2V_SCALE_ADD2(seg->xy + 2, 1, v0p, alpha[0], t1p);
  ELL_2V_SCALE_ADD2(seg->xy + 4, 1, v3p, alpha[1], t2p);
  ELL_2V_COPY(seg->xy + 6, v3p);
  /* HEY is it our job to set seg->corner[] ?!? */
  seg->pointNum = pNum;

  airMopOkay(mop);
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

debug limnCBFitSingle with various conditions

testing corners: corners near and at start==stop of isLoop
corners not at start or stop of isLoop: do spline wrap around from last to first index?

use performance tests to explore optimal settings in fctx:
  nrpIterMax, nrpCap, nrpIota, nrpPsi, nrpDeltaThresh
evaluated in terms of time and #splines needed for fit
(may want to pay in time for more economical representation)

"What GLK hasn't thought through is: what is the interaction of nrp iterations and
findAlpha generating the simple arc on some but not all iterations (possibly
unstable?)"

resurrect in segment struct: how many points were represented by a single spline

reparm: "HEY TODO: need to make sure that half-way between points,
     spline isn't wildly diverging; this can happen with the
     spline making a loop away from a small number of points, e.g.:
     4 points spline defined by vv0 = (1,1), tt1 = (1,2),
     tt2 = (1,2), vv3 = (0,1)"

valgrind everything

search for HEY

remove big commented-out print statements: at least in limnCBFSegEval, limnCBFPathSample

(DIM=2) explore what would be required to generalized from 2D to 3D,
perhaps at least at the API level, even if 3D is not yet implemented
*/
