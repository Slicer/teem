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
but the code here was based more on reading the paper, than their code. Also, that code
does not handle point loops, and does not handle smoothing built into tangent estimation,
which were important to GLK, but which added significant implementation complexity.

The functions below do not use any other limnSpline structs or functions, since those
were written a long time ago when GLK was even more ignorant than now about splines.
Hopefully that older code can be revisited and re-organized for a later version of
Teem, at which point the code below can be integrated with it.

NOTE: spline fitting would be useful in 3D (or higher dimensions) too, but currently
this code only supports 2D. "DIM=2" flags places in code where that is explicit, with
the hope that this code can later be generalized.

NOTE: In Teem coding standards, "Cbf" would be better written as "CBF", but all these
initials got annoying with other CamelCase function names.

This code was unusually slow for GLK to write because he struggled to gracefully handle
the combination of possibilities:
  - points may trace an open segment, or a closed loop
  - geometry (spline endpoints and tangents) is computed without or with some smoothing.
  - smoothing may be bounded by the two corners on either side, or smoothing
    may cover all available vertices (which may or may not loop around)
How to represent an interval of vertex *indices* -- termed a "span", in contrast to the
single cubic spline "segment" -- is fundamental to all of this. A uint span[2] array was
tried, but ended up just passing around a lot of loi,hii (low index, high index) pairs to
represent the span.

In the presence of a loop of N points, we could imagine different kinds of indices:
          0 .. N-1          : "actual" indices
 .. -2 -1 0 .. N-1 N N-1 .. : "lifted" indices, in the sense that the real number line
                              is a cover of the circle, and a path in the circle can be
                              lifted up to the real number line. A lifted index J is
                              converted to an actual index by AIR_MOD(J, N) or if J > 0
                              just J % N.

Some computations are easier to do and reason about with lifted indices, but obviously
you can only use actual indices for any memory access. Early iterations of the code used
lifted indices in lots of places, but GLK got confused, so the following summary of who
calls whom ("who -- whom") was made to trace where [loi,hii] spans originate, and to
ensure that all the functions here only take actual (not-lifted) indices as parameters,
including limnCbfTVT (which computes a tangent,vertex,tangent triple at a given point).
Nearly all of the cleverness with lifted indices happens in limnCbfTVT, and its idxLift
helper function ensures it gets actual indices.

limnCbfTVT -- idxLift to convert given actual indices into lifted,
              and does computations with lifted indices (with signed offsets)

findAlpha -- spanLength (with given loi, hii)
             (and otherwise only works with actual indices)

findDist --  spanLength (with given loi, hii)
             internally used lifted indices, but:
             *** sets fctx->distMaxIdx to an actual index

vttvCalcOrCopy -- limnCbfTVT (with given loi, hii, and vvi=loi or hii)

fitSingle -- spanLength (with given loi, hii)
          -- findAlpha (with given loi, hii)
          -- findDist (with given loi, hii)

limnCbfCorners -- limnCbfTVT (either: with loi=0 hii=pnum-1
                                      and vvi also=0 or pnum-1
                                  or: with loi=hii=0
                                      and all vvi from 0..pnum-1

limnCbfMulti -- vttvCalcOrCopy (with given loi,hii)
             -- fitSingle (with given loi,hii)
             -- limnCbfTVT (with given loi,hii and fctx->distMaxIdx)
             -- limnCbfMulti (with given loi,hii and fctx->distMaxIdx)
             -- limnCbfPathNew, limnCbfPathJoin, limnCbfPathJoin

limnCbfGo -- limnCbfCtxPrep
          -- limnCbfCorners
          -- limnCbfMulti (either with loi==hii==0 or with loi,hii at corners)
          -- limnCbfPathNew, limnCbfPathJoin, limnCbfPathNix
*/

/*
TODO:
test findDist - is distMaxIdx the correct actual index?
testing corners: corners near and at start==stop of isLoop
corners not at start or stop of isLoop: do spline wrap around from last to first index?

valgrind everything

(DIM=2) explore what would be required to generalize from 2D to 3D,
perhaps at least at the API level, even if 3D is not yet implemented

The initialization of uu[] by arc length is especially wrong when the tangents are
pointing towards each other, and then newton iterations can get stuck in a local minimum.
As a specific example: with these control points: (-0.5,0.5) (2,0.5) (-0.5,0) (0.5,-0.5)
sampled 19 times (uniformly in u), fitSingle does great, but sampled 18 times it gets
stuck on a bad answer.  Would be nice to come up with a heuristic for how to warp
the initial arc-length parameterization to get closer to correct answer, but this
requires exploring what is at least a 4-D space of possible splines (lowered from 8-D
by symmetries).  The cost of not doing this is less economical representations, because
we would split these segments needlessly.

use performance tests to explore optimal settings in fctx:
  nrpIterMax, nrpCap, nrpIota, nrpPsi, nrpDeltaThresh
evaluated in terms of time and #splines needed for fit
(may want to pay in time for more economical representation)
*/

#define PNMIN(ISLOOP) ((ISLOOP) ? 4 : 3)

/*
limnCbfPointsNew

create a point data container, possibly around given pdata pointer. In an aspirational
hope of API stability, this is one of the few functions for which the interface itself
does not expose the specificity to DIM=2 and type double (though the code inside
does (apologetically) enforce that).
*/
limnCbfPoints * /* Biff: NULL */
limnCbfPointsNew(const void *pdata, int ptype, uint dim, uint pnum, int isLoop) {
  static const char me[] = "limnCbfPointsNew";
  limnCbfPoints *lpnt;
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
  if (!(lpnt = AIR_CALLOC(1, limnCbfPoints))) {
    biffAddf(LIMN, "%s: couldn't allocate point container", me);
    return NULL;
  }
  if (pdata) {
    /* we are wrapping around a given pre-allocated buffer */
    lpnt->pp = pdata;
    lpnt->ppOwn = NULL;
  } else {
    /* we are allocating our own buffer */
    lpnt->pp = NULL;
    if (!(lpnt->ppOwn = AIR_CALLOC(dim * pnum, double))) {
      biffAddf(LIMN, "%s: couldn't allocate %u %u-D point", me, pnum, dim);
      return NULL;
    }
  }
  lpnt->num = pnum;
  lpnt->dim = dim; /* but really DIM=2 because of above */
  lpnt->isLoop = isLoop;
  return lpnt;
}

limnCbfPoints * /* Biff: nope */
limnCbfPointsNix(limnCbfPoints *lpnt) {
  if (lpnt) {
    /* don't touch lpnt->pp */
    if (lpnt->ppOwn) free(lpnt->ppOwn);
    free(lpnt);
  }
  return NULL;
}

int /* Biff: 1 */
limnCbfPointsCheck(const limnCbfPoints *lpnt) {
  static const char me[] = "limnCbfPointsCheck";
  uint pnmin;
  int have;

  if (!lpnt) {
    biffAddf(LIMN, "%s: got NULL pointer", me);
    return 1;
  }
  pnmin = PNMIN(lpnt->isLoop);
  if (!(lpnt->num >= pnmin)) {
    biffAddf(LIMN, "%s: need %u or more points in limnCbfPoints (not %u)%s", me, pnmin,
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
  limnCbfSeg *seg = (limnCbfSeg *)_seg;
  ELL_2V_NAN_SET(seg->xy + 0); /* DIM=2 */
  ELL_2V_NAN_SET(seg->xy + 2);
  ELL_2V_NAN_SET(seg->xy + 4);
  ELL_2V_NAN_SET(seg->xy + 6);
  seg->corner[0] = seg->corner[1] = AIR_FALSE;
  seg->pointNum = 0;
  return;
}

limnCbfPath * /* Biff: nope */
limnCbfPathNew(unsigned int segNum) {
  limnCbfPath *path;
  path = AIR_MALLOC(1, limnCbfPath);
  if (path) {
    path->segArr = airArrayNew((void **)(&path->seg), &path->segNum, sizeof(limnCbfSeg),
                               128 /* incr */);
    airArrayStructCB(path->segArr, segInit, NULL);
    path->isLoop = AIR_FALSE;
    if (segNum) {
      airArrayLenSet(path->segArr, segNum);
      if (!path->segArr->data) {
        /* whoa, couldn't allocate requested segments; return NULL and possibly leak */
        path = NULL;
      }
    }
  }
  return path;
}

limnCbfPath * /* Biff: nope */
limnCbfPathNix(limnCbfPath *path) {
  if (path) {
    airArrayNuke(path->segArr);
    free(path);
  }
  return NULL;
}

void
limnCbfPathJoin(limnCbfPath *dst, const limnCbfPath *src) {
  uint bb = airArrayLenIncr(dst->segArr, src->segNum);
  memcpy(dst->seg + bb, src->seg, (src->segNum) * sizeof(limnCbfSeg));
  return;
}

/* initialize a freshly allocated limnCbfCtx struct;
   the pointers therein do not point to anything valid */
static void
ctxInit(limnCbfCtx *fctx) {
  if (!fctx) return;
  /* defaults for input parameters to various Cbf functions */
  fctx->verbose = 0;
  fctx->cornerFind = AIR_TRUE;
  fctx->cornerNMS = AIR_TRUE;
  fctx->nrpIterMax = 40;    /* authors originally thought ~6 */
  fctx->epsilon = 0;        /* NOTE: will need to be set to something valid elsewhere */
  fctx->scale = 0;          /* scale 0 means no filtering at all */
  fctx->nrpCap = 10.0;      /* not much of a cap, really */
  fctx->nrpIota = 1.0 / 16; /* quite stringent */
  fctx->nrpPsi = 100;
  fctx->nrpDeltaThresh = 0.01;
  fctx->alphaMin = 0.001;
  fctx->detMin = 0.01;
  fctx->cornAngle = 120.0; /* degrees */
  /* internal state */
  /* initialize buffer pointers to NULL and buffer lengths to 0 */
  fctx->uu = fctx->vw = fctx->tw = fctx->ctvt = NULL;
  fctx->cidx = NULL;
  fctx->ulen = fctx->wlen = fctx->cnum = 0;
  /* initialize outputs to bogus valus */
  fctx->nrpIterDone = UINT_MAX;
  fctx->distMaxIdx = UINT_MAX;
  fctx->nrpPuntFlop = UINT_MAX;
  fctx->distMax = AIR_POS_INF;
  fctx->nrpDeltaDone = AIR_POS_INF;
  fctx->alphaDet = 0;
  fctx->distBig = 0;
  return;
}

limnCbfCtx * /* Biff: nope */
limnCbfCtxNew() {
  limnCbfCtx *ret;

  ret = AIR_CALLOC(1, limnCbfCtx);
  if (ret) ctxInit(ret);
  return ret;
}

limnCbfCtx * /* Biff: nope */
limnCbfCtxNix(limnCbfCtx *fctx) {
  if (fctx) {
    if (fctx->uu) free(fctx->uu);
    if (fctx->vw) free(fctx->vw);
    if (fctx->tw) free(fctx->tw);
    if (fctx->ctvt) free(fctx->ctvt);
    if (fctx->cidx) free(fctx->cidx);
    free(fctx);
  }
  return NULL;
}

/*
ctxBuffersSet: ensures that some buffers in fctx: uu, vw, tw are set up for current
#points pNum and measurement scale fctx->scale. The buffers are re-allocated only when
necessary. Does NOT touch the corner-related buffers: ctvt, cidx
*/
static int /* Biff: 1 */
ctxBuffersSet(limnCbfCtx *fctx, uint pNum) {
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
  if (ulen != fctx->ulen) {
    airFree(fctx->uu);
    if (!(fctx->uu = AIR_CALLOC(ulen, double))) {
      biffAddf(LIMN, "%s: failed to allocate uu buffer (%u doubles)", me, ulen);
      return 1;
    }
  }
  fctx->ulen = ulen;

  if (0 == scale) {
    /* will do simplest possible finite differences; no need for weights */
    fctx->vw = airFree(fctx->vw);
    fctx->tw = airFree(fctx->tw);
    fctx->wlen = 0;
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
    const double tinysum = 1.0 / 128;
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
         vertex, which is clearly excessive */
      biffAddf(LIMN,
               "%s: weight buffer length %u (from scale %g) seems "
               "too large compared to #points %u",
               me, wlen, scale, pNum);
      return 1;
    }
    if (wlen != fctx->wlen) {
      airFree(fctx->vw);
      airFree(fctx->tw);
      if (!((fctx->vw = AIR_CALLOC(wlen, double))
            && (fctx->tw = AIR_CALLOC(wlen, double)))) {
        biffAddf(LIMN, "%s: couldn't allocate weight buffers (%u doubles)", me, wlen);
        return 1;
      }
      fctx->wlen = wlen;
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
limnCbfCtxPrep

checks the things that are going to be passed around a lot, and makes call to initialize
buffers inside fctx
*/
int /* Biff: 1 */
limnCbfCtxPrep(limnCbfCtx *fctx, const limnCbfPoints *lpnt) {
  static const char me[] = "limnCbfCtxPrep";

  if (!(fctx && lpnt)) {
    biffAddf(LIMN, "%s: got NULL pointer", me);
    return 1;
  }
  if (limnCbfPointsCheck(lpnt)) {
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
limnCbfSegEval

evaluates a single limnCbfSeg at one point tt in [0.0,1.0]
*/
void
limnCbfSegEval(double *vv, const limnCbfSeg *seg, double tt) {
  double ww[4];
  const double *xy = seg->xy;
  CBD0(vv, xy + 0, xy + 2, xy + 4, xy + 6, tt, ww); /* DIM=2 */
  /*
  fprintf(stderr, "!%s: tt=%g -> ww={%g,%g,%g,%g} * "
          "{(%g,%g),(%g,%g),(%g,%g),(%g,%g)} = (%g,%g)\n",
          "limnCbfSegEval", tt, ww[0], ww[1], ww[2], ww[3],
          (xy + 0)[0], (xy + 0)[1],
          (xy + 2)[0], (xy + 2)[1],
          (xy + 4)[0], (xy + 4)[1],
          (xy + 6)[0], (xy + 6)[1], vv[0], vv[1]);
  */
  return;
}

/*
limnCbfPathSample

evaluates limnCbfPath at pNum locations, uniformly (and very naively) distributed among
the path segments, and saves into (pre-allocated) xy
*/
void
limnCbfPathSample(double *xy, uint pNum, const limnCbfPath *path) {
  uint ii, sNum = path->segNum;
  for (ii = 0; ii < pNum; ii++) {
    uint segi = airIndex(0, ii, pNum - 1, sNum);
    const limnCbfSeg *seg = path->seg + segi;
    double tmpf = AIR_AFFINE(0, ii, pNum - 1, 0, sNum);
    double tt = tmpf - segi;
    limnCbfSegEval(xy + 2 * ii, seg, tt); /* DIM=2 */
    /*
    fprintf(stderr, "!%s: %u -> %u (%g) %g -> (%g,%g)\n",
            "limnCbfPathSample", ii, segi, tmpf, tt,
            (xy + 2*ii)[0], (xy + 2*ii)[1]);
    */
  }
  return;
}

/* cheesy macro as short-hand to access either pp or ppOwn */
#define PP(lpnt) ((lpnt)->pp ? (lpnt)->pp : (lpnt)->ppOwn)

/* idxLift: error-checked index lifting (from "actual" to "lifted" indices, explained
above) for limnCbfTVT and maybe others. If no error, *loiP will be same as given gloi,
but in loops *vviP and *hiiP may be lifted (relative to gvvi and ghii), and outside loops
*hiiP may be changed to #points-1.

That sounds like nothing fancy, but this is messy because of the flexibility in how we
handle points: might not be a loop or might not, and, consideration of vertices should
either be bounded in a specific [loi,hii] or be "unbounded" loi==hii==0 (truly unbounded
in a loop, or bounded only as much as needed in non-loop data). */
static int /* Biff: 1 */
idxLift(uint *loiP, uint *hiiP, uint *vviP, int verbose, const limnCbfPoints *lpnt,
        uint gloi, uint ghii, uint gvvi) {
  static const char me[] = "idxLift";
  uint pnum = lpnt->num, loi, hii, vvi;

  *loiP = *hiiP = *vviP = UINT_MAX; /* initialize to bogus indices */
  if (!(pnum < (1U << 29))) {
    /* UB = undefined behavior */
    biffAddf(LIMN, "%s: # points %u seems too big (to stay well clear of UB)", me, pnum);
    return 1;
  }
  if (!(gloi < pnum && ghii < pnum && gvvi < pnum)) {
    biffAddf(LIMN, "%s: given loi %u, hii %u, vvi %u not all < #points %u", me, gloi,
             ghii, gvvi, pnum);
    return 1;
  }
  /* now all of gloi, gvvi, ghii are all valid actual indices */
  if (gloi == ghii && ghii != 0) {
    biffAddf(LIMN,
             "%s: can only have gloi == ghii if both 0 (not %u), "
             "to signify unbounded vertex consideration",
             me, gloi);
    return 1;
  }
  /* initialize values to return */
  loi = gloi;
  hii = ghii;
  vvi = gvvi;
  if (lpnt->isLoop) {
    if (gloi != ghii) { /* implies both == 0 because of test above */
      if (gloi > ghii) hii += pnum;
      if (gloi > gvvi) vvi += pnum;
    }
  } else {
    if (gloi == ghii) { /* (implies both == 0, again) */
      /* we allow loi==hii==0 in non-loop to say: only bounded by data itself
      loi is already 0, but hii needs fixing */
      hii = pnum - 1;
    } else {
      if (gloi > ghii) {
        biffAddf(LIMN,
                 "%s: if loi != hii, need loi (%u) < hii (%u) since not in a "
                 "point loop",
                 me, gloi, ghii);
        return 1;
      }
      if (gloi > gvvi) {
        biffAddf(LIMN, "%s: need given loi (%u) < vvi (%u) since not in point loop", me,
                 gloi, gvvi);
        return 1;
      }
    }
  }
  /* now: must have loi <= vvi and loi <= hii */
  if (verbose) {
    printf("%s: given loi,hii,vvi %u %u %u --> lifted %u %u %u\n", me, gloi, ghii, gvvi,
           loi, hii, vvi);
  }
  if (loi < hii) {
    /* need to check that vvi is inside consequential bounds [loi,hii] */
    if (vvi > hii) {
      biffAddf(LIMN, "%s: vvi %u->%u not in [%u,%u]->[%u,%u] span", me, gvvi, vvi, gloi,
               ghii, loi, hii);
      return 1;
    }
    /* now (if bounded) have vvi <= hii */
  }

  /* all's well, set output values */
  *loiP = loi;
  *hiiP = hii;
  *vviP = vvi;
  return 0;
}

static void
subnorm2(double dir[2], const double aa[2], const double bb[2]) {
  double len;
  ELL_2V_SUB(dir, aa, bb);    /* dir = aa - bb */
  ELL_2V_NORM(dir, dir, len); /* normalize(dir) */
}

/* utility function for getting pointer to position coordinates in lpnt,
for point with signed and lifted index ssi. So this is the single place
that we go from lifted index to actual index */
static const double *
PPlowerI(const limnCbfPoints *lpnt, int ssi) {
  int pnum = AIR_INT(lpnt->num);
  ssi = AIR_MOD(ssi, pnum);
  return PP(lpnt) + 2 * ssi; /* DIM=2 */
}

/* utility function for counting how many vertices are in (actual) index span [loi,hii]
inclusive. It is not our job here to care about lpnt->isLoop; we just assume that if
we're faced with hii<loi, it must be because of a loop */
static uint
spanLength(const limnCbfPoints *lpnt, uint loi, uint hii) {
  uint topi = hii + (hii < loi) * (lpnt->num);
  return topi - loi + 1;
}

/*
limnCbfTVT: Find constraints for spline fitting: incoming/left tangent lt, center or
endpoint vertex vv, outgoing/right tangent rt; any but not all can be NULL. These are
computed from the given points lpnt, at given vertex (actual) index gvvi, looking only
within (actual) index range [gloi, ghii] if gloi!=ghii: that range is probably delimited
by corners, and we have to be blind to anything past the corners on either side of us.
HOWEVER, if gloi==ghii==0 and lpnt is a point loop, then we can look at all the points.

Given that this is the inner-loop of other things, it would make sense to have a
non-public version without all the error checking, but given the prolonged birthing pain
of the code in this file, the error-checking is a useful and welcome safety-net (and
being a public function permits easier testing), and that is all ok until profiling shows
that we are a bottleneck.

NOTE: this assumes that limnCbfCtxPrep(fctx, lpnt) was called without error!
That (via ctxBuffersSet) allocates things that we depend on here.
*/
int /* Biff: 1 */
limnCbfTVT(double lt[2], double vv[2], double rt[2], const limnCbfCtx *fctx,
           const limnCbfPoints *lpnt, uint gloi, uint ghii, uint gvvi, int oneSided) {
  static const char me[] = "limnCbfTVT";
  uint loi, /* error-checked gloi */
    hii,    /* error-checked ghii, lifted if needed */
    vvi;    /* error-checked gvvi, lifted if needed */
  /* we use here (signed) int for things that might seem better as uint, but it
     simplifies implementing arithmetic and comparisons given how indices wrap around in
     point loops */
  int slo, shi, svi; /* signed versions of loi, hii, vvi */

  if (!((lt || vv || rt) && fctx && lpnt)) {
    biffAddf(LIMN, "%s: got NULL pointer (or too many NULL pointers)", me);
    return 1;
  }
  /* so: each of lt, vv, rt can be NULL
     (they just can't be all NULL, or else why are we being called) */
  if (fctx->verbose > 1) {
    printf("%s: hello: %u in [%u,%u] in %sloop with %u points (%s-sided)\n", me, gvvi,
           gloi, ghii, lpnt->isLoop ? "" : "NON-", lpnt->num, oneSided ? "1" : "2");
  }
  if (idxLift(&loi, &hii, &vvi, fctx->verbose > 1, lpnt, gloi, ghii, gvvi)) {
    biffAddf(LIMN, "%s: trouble with given loi %u, hii %u, or vvi %u", me, gloi, ghii,
             gvvi);
    return 1;
  }
  /* NOTE: If scale==0, then we will get NaNs for left tangent if loi == vvi, and for
  right tangent if hii == vvi, but will avoid NaNs if scale > 0. Because it will be too
  annoying to require being called in different ways depending on scale, we do *not* do
  error-checking to prevent NaN generation. */
  if (fctx->verbose > 1) {
    printf("%s: (post-idxLift) %u in [%u,%u] -> %u in [%u,%u]\n", me, gvvi, gloi, ghii,
           vvi, loi, hii);
  }

  /* ----------- now switch to signed (lifted) indices ---------- */
  slo = AIR_INT(loi);
  shi = AIR_INT(hii);
  svi = AIR_INT(vvi);
  if (0 == fctx->scale) {
    /* DIM=2 through-out */
    const double *xyC, *xyP, *xyM;
    int iplus = svi + 1, imnus = svi - 1;
    if (slo < shi) { /* bounded */
      iplus = AIR_CLAMP(slo, iplus, shi);
      imnus = AIR_CLAMP(slo, imnus, shi);
    }
    xyM = PPlowerI(lpnt, imnus);
    xyC = PPlowerI(lpnt, svi);
    xyP = PPlowerI(lpnt, iplus);
    if (fctx->verbose > 1) {
      printf("%s: %d | %d | %d --> (%g,%g)|(%g,%g)|(%g,%g)\n", me, imnus, svi, iplus,
             xyM[0], xyM[1], xyC[0], xyC[1], xyP[0], xyP[1]);
    }
    if (vv) {
      ELL_2V_COPY(vv, xyC);
    }
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
    const double *vwa = fctx->vw;
    const double *twa = fctx->tw;
    int lim = (int)fctx->wlen - 1, /* limit on loop index */
      ci;                          /* loops through [-lim,lim] */
    if (!(vwa && twa)) {
      biffAddf(LIMN, "%s: fctx internal buffers vw and tw not both allocated", me);
      return 1;
    }
    if (twa[0] != 0) {
      biffAddf(LIMN, "%s: first tangent weight fctx->tw[0] %g not zero", me, twa[0]);
      return 1;
    }
    for (ci = -lim; ci <= lim; ci++) {
      uint wi = abs(ci);                 /* weight index into vwa, twa */
      double vw = vwa[wi], tw = twa[wi]; /* current vert and tan weights */
      int sui = svi + ci;                /* signed, unbounded, vertex index */
      int sbi = slo < shi                /* signed, bounded, vertex index */
                ? AIR_CLAMP(slo, sui, shi)
                : sui;
      const double *xy = PPlowerI(lpnt, sbi); /* coords at sbi */
      ELL_2V_SCALE_INCR(posC, vw, xy);
      if (fctx->verbose > 1) {
        printf("%s: ci=%d (in [%d,%d]) idx %d --[%d,%d]--> %d;  v,t w %g,%g on "
               "xy=(%g,%g)\n",
               me, ci, -lim, lim, sui, slo, shi, sbi, vw, tw, xy[0], xy[1]);
        printf("%s:   ---> posC=(%g,%g)\n", me, posC[0], posC[1]);
      }
      if (ci < 0) {
        ELL_2V_SCALE_INCR(posM, tw, xy);
        if (fctx->verbose > 1) {
          printf("%s:   ---> posM=(%g,%g)\n", me, posM[0], posM[1]);
        }
      }
      if (ci > 0) {
        ELL_2V_SCALE_INCR(posP, tw, xy);
        if (fctx->verbose > 1) {
          printf("%s:   ---> posP=(%g,%g)\n", me, posP[0], posP[1]);
        }
      }
    }
    {
      /* limit distance from chosen (x,y) datapoint to posC to be (yes, harcoded) 95% of
      fctx->epsilon. Being allowed to be further away can cause annoyances (for GLK in
      some early stage of debugging) */
      double off[2], offlen, clofflen,
        okoff = 0.95 * fctx->epsilon;         /* DIM=2 throughout */
      const double *xy = PPlowerI(lpnt, svi); /* center vertex in given data */
      ELL_2V_SUB(off, posC, xy);     /* off = posC - xy, from given to computed */
      ELL_2V_NORM(off, off, offlen); /* offlen = |off|; off /= |off| */
      clofflen = AIR_MIN(okoff, offlen);
      /* difference between chosen (x,y) datapoint and spline endpoint
         can be in any direction, but we limit the length */
      ELL_2V_SCALE_ADD2(posC, 1, xy, clofflen, off);
      if (fctx->verbose > 1) {
        printf("%s: clamping |posC - xy[%d]=(%g,%g)| dist %g to %g = %g --> (%g,%g)\n",
               me, svi, xy[0], xy[1], offlen, okoff, clofflen, posC[0], posC[1]);
        printf("%s:   also: posM = (%g,%g)     posP = (%g,%g)\n", me, posM[0], posM[0],
               posP[0], posP[1]);
      }
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
(from paper page 620, after "we need only solve"): solves for the alpha[0,1] that
minimize squared error between xy[i] and Q(uu[i]) where Q(t) is cubic Bezier spline
through vv0, vv0 + alpha[0]*tt1, vv3 + alpha[1]*tt2, and vv3.

There are various conditions where the generated spline (implied by the alpha[0,1] set
here) ignores the xy array and is instead what we could call a "punt", with control
points at 1/3 and 2/3 the distance between the end points:
 - having only two points (xy contains only the end points)
 - the determinant of the 2x2 matrix that is inverted to solve
   for alpha is too close to zero (this test was not part of the
   author's code)
 - the solved alphas are not convincingly positive

This function is *the* place where the punted spline is generated, in which case we
return 1, otherwise (when we set alpha[0,1] computed via the 2x2 matrix solve) we return
0. But generating the punted arc is not actually an error or problem: if the punted arc
is bad at fitting the data (as determined by findDist) then it may be subdivided, and
that's ok.
*/
static int /* Biff: nope */
findAlpha(double alpha[2], int nrpi /* which nrp iter we're on, just for debugging */,
          limnCbfCtx *fctx, const double vv0[2], const double tt1[2],
          const double tt2[2], const double vv3[2], const limnCbfPoints *lpnt, uint loi,
          uint hii) {
  static const char me[] = "findAlpha";
  int ret;
  uint ii, spanlen = spanLength(lpnt, loi, hii);
  double det, F2L[2], lenF2L;
  const double *xy = PP(lpnt), *uu = fctx->uu;

  ELL_2V_SUB(F2L, xy + 2 * hii, xy + 2 * loi); /* DIM=2 throughout this */
  lenF2L = ELL_2V_LEN(F2L);
  if (spanlen > 2) {
    /* GLK using "m" and "M" instead of author's "C". Note that Ai1 and Ai2 are scalings
       of (nominally) unit-length tt1 and tt2 by evaluations of the spline basis
       functions, so they (and the M computed from them, and det(M)), are invariant w.r.t
       over-all rescalings of the data points */
    double xx[2], m11, m12, m22, MM[4], MI[4];
    xx[0] = xx[1] = m11 = m12 = m22 = 0;
    for (ii = 0; ii < spanlen; ii++) {
      const double *xy = PPlowerI(lpnt, AIR_INT(loi + ii)); /* == paper's "d_i" */
      double ui = uu[ii];
      double bb[4], Ai1[2], Ai2[2], Pi[2], dmP[2];
      VCBD0(bb, ui);
      ELL_2V_SCALE(Ai1, bb[1], tt1);
      ELL_2V_SCALE(Ai2, bb[2], tt2);
      m11 += ELL_2V_DOT(Ai1, Ai1);
      m12 += ELL_2V_DOT(Ai1, Ai2);
      m22 += ELL_2V_DOT(Ai2, Ai2);
      /* paper doesn't name what we call P */
      ELL_2V_SCALE_ADD2(Pi, bb[0] + bb[1], vv0, bb[2] + bb[3], vv3);
      ELL_2V_SUB(dmP, xy, Pi);       /* d minus P */
      xx[0] += ELL_2V_DOT(dmP, Ai1); /* column vector on right-hand side */
      xx[1] += ELL_2V_DOT(dmP, Ai2);
    }
    ELL_4V_SET(MM, m11, m12, m12, m22); /* paper's 2x2 [c11 c12; c21 c22]*/
    ELL_2M_INV(MI, MM, det);
    ELL_2MV_MUL(alpha, MI, xx); /* solve for alpha[0,1] */
  } else {                      /* spanlen <= 2 */
    det = 1;                    /* bogus but harmless */
    alpha[0] = alpha[1] = 0;    /* trigger punting */
  }
  /* test if we should return punted arc */
  if (!(AIR_EXISTS(det) && AIR_ABS(det) > fctx->detMin
        && alpha[0] > lenF2L * (fctx->alphaMin)
        && alpha[1] > lenF2L * (fctx->alphaMin))) {
    if (fctx->verbose) {
      if (spanlen > 2) {
        printf("%s(i%d): bad |det| %g (vs %g) or alpha %g,%g (vs %g*%g) "
               "--> punted arc\n",
               me, nrpi, AIR_ABS(det), fctx->detMin, alpha[0], alpha[1], lenF2L,
               fctx->alphaMin);
      } else {
        printf("%s(i%d): [%u,%u] spanlen %u tiny --> punting\n", me, nrpi, loi, hii,
               spanlen);
      }
    }
    /* generate punted arc: set both alphas to 1/3 of distance from first to last point,
    but also handle non-unit-length tt1 and tt2 */
    alpha[0] = lenF2L / (3 * ELL_2V_LEN(tt1));
    alpha[1] = lenF2L / (3 * ELL_2V_LEN(tt2));
    ret = 1;
  } else {
    if (fctx->verbose > 1) {
      printf("%s(i%d): all good: det %g, alpha %g,%g\n", me, nrpi, det, alpha[0],
             alpha[1]);
    }
    ret = 0;
  }
  fctx->alphaDet = det;
  return ret;
}

#if 0
/* Paper pg 621 eqs (7) and (8): the change in the spline parameter to improve how Q(u_i)
approximates vertex i, but naming the paramter "t" since that's what the paper does.
This is not just the Newton step, but a check that the Newton step is an improvement
(a check that is not described in the paper, and is not in the author's code, but which
does matter in cases where the spine is a poor fit) */
static double
delta_t(double t0, double cap, const double P[2], const double V0[2], const double V1[2],
        const double V2[2], const double V3[2]) {
  double Q0[2], Q1[2], Q2[2], QmP[2], denom;
}
#endif

/*
using Newton iterations to try to find a better places at which to evaluate the spline in
order to match the given points xy
*/
static double
reparm(const limnCbfCtx *fctx, /* must be non-NULL */
       const double alpha[2], const double vv0[2], const double tt1[2],
       const double tt2[2], const double vv3[2], const limnCbfPoints *lpnt, uint loi,
       uint hii) {
  static const char me[] = "reparm";
  uint ii, spanlen;
  double vv1[2], vv2[2], delta, cap;
  double *uu = fctx->uu;

  spanlen = spanLength(lpnt, loi, hii);
  assert(spanlen >= 3);
  /* average u[i+1]-u[i] is 1/(spanlen-1) */
  cap = fctx->nrpCap / (spanlen - 1);
  ELL_2V_SCALE_ADD2(vv1, 1, vv0, alpha[0], tt1);
  ELL_2V_SCALE_ADD2(vv2, 1, vv3, alpha[1], tt2);
  delta = 0;
  /* only changing parameterization of interior points,
     not the first (ii=0) or last (ii=pNum-1) */
  for (ii = 1; ii < spanlen - 1; ii++) {
    double denom, delu = 0, QmP[2], ww[4], tt, Q0[2], Q1[2], Q2[2];
    const double *P = PPlowerI(lpnt, AIR_INT(loi + ii));
    tt = uu[ii];
    CBD0(Q0, vv0, vv1, vv2, vv3, tt, ww);
    CBD1(Q1, vv0, vv1, vv2, vv3, tt, ww);
    CBD2(Q2, vv0, vv1, vv2, vv3, tt, ww);
    ELL_2V_SUB(QmP, Q0, P);
    denom = ELL_2V_DOT(Q1, Q1) + ELL_2V_DOT(QmP, Q2);
    if (denom) {
      double absdelu;
      delu = ELL_2V_DOT(QmP, Q1) / denom;
      absdelu = fabs(delu);
      if (absdelu > cap) {
        /* cap Newton step */
        delu *= cap / absdelu;
      }
      uu[ii] = tt - delu;
    }
    /* delu = delta_t(tt, cap, P, vv0, vv1, vv2, vv3); */
    delta += fabs(delu);
    if (fctx->verbose > 1) {
      double R[2], dR[2]; /* R is the new Q */
      CBD0(R, vv0, vv1, vv2, vv3, uu[ii], ww);
      ELL_2V_SUB(dR, R, P);
      printf("%s[%2u]: %g <-- %g - %g\n", me, ii, uu[ii], tt, delu);
      printf("     %g=|(%g,%g)-(%g,%g)|   <--   %g=|(%g,%g)-(%g,%g)|\n", ELL_2V_LEN(dR),
             R[0], R[1], P[0], P[1], /* */
             ELL_2V_LEN(QmP), Q0[0], Q0[1], P[0], P[1]);
    }
  }
  delta /= spanlen - 2; /* number of interior points */
  return delta;
}

/* (assuming current parameterization in fctx->uu) sets fctx->distMax to max distance
   to spline, at point fctx->distMaxIdx, and then sets fctx->distBig accordingly */
static int /* Biff: 1 */
findDist(limnCbfCtx *fctx, const double alpha[2], const double vv0[2],
         const double tt1[2], const double tt2[2], const double vv3[2],
         const limnCbfPoints *lpnt, uint loi, uint hii) {
  static const char me[] = "findDist";
  uint ii, distMaxIdx = UINT_MAX, spanlen;
  double vv1[2], vv2[2], distMax;
  const double *uu = fctx->uu;

  spanlen = spanLength(lpnt, loi, hii);
  if (!(spanlen >= 3)) {
    biffAddf(LIMN, "%s: [loi,hii] [%u,%u] -> spanlen %u too small", me, loi, hii,
             spanlen);
    return 1;
  }
  ELL_2V_SCALE_ADD2(vv1, 1, vv0, alpha[0], tt1); /* DIM=2 everywhere here */
  ELL_2V_SCALE_ADD2(vv2, 1, vv3, alpha[1], tt2);
  distMax = -1.0; /* any computed distance will be >= 0 */
  /* NOTE that the first and last points are actually not part of the max distance
     calculation, which motivates ensuring that the endpoints generated by limnCbfTVT
     are actually sufficiently close to the first and last points (or else the fit spline
     won't meet the expected accuracy threshold) */
  for (ii = 1; ii < spanlen - 1; ii++) {
    const double *xy = PPlowerI(lpnt, AIR_INT(loi + ii));
    double len, Q[2], df[2], ww[4];
    CBD0(Q, vv0, vv1, vv2, vv3, uu[ii], ww);
    ELL_2V_SUB(df, Q, xy);
    len = ELL_2V_LEN(df);
    if (len > distMax) {
      distMax = len;
      distMaxIdx = loi + ii; /* lifted index */
    }
  }
  fctx->distMax = distMax;
  /* we could use a lifted index for internal distMaxIdx,
     but upon saving to fctx it needs to be an actual index */
  fctx->distMaxIdx = distMaxIdx % lpnt->num;
  fctx->distBig = (distMax <= fctx->nrpIota * fctx->epsilon
                     ? 0
                     : (distMax <= fctx->epsilon /* */
                          ? 1
                          : (distMax <= fctx->nrpPsi * fctx->epsilon /* */
                               ? 2
                               : 3)));
  if (fctx->verbose > 2) {
    printf("%s[%u,%u]: distMax %g @ %u (big %d)\n", me, loi, hii, fctx->distMax,
           fctx->distMaxIdx, fctx->distBig);
  }
  return 0;
}

/*
fitSingle: fits a single cubic Bezier spline, with minimal error checking (limnCbfSingle
is the error-checking wrapper around this). The given points coordinates are in
limnCbfPoints lpnt, between low/high indices loi/hii (inclusively); hii can be < loi in
the case of a point loop. From GIVEN initial endpoint vv0, initial tangent tt1, final
tangent tt2 (pointing backwards), and final endpoint vv3, the job of this function is
actually just to set alpha[0],alpha[1] such that the cubic Bezier spline with control
points vv0, vv0+alpha[0]*tt1, vv3+alpha[1]*tt2, vv3 approximates all the given points.

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

This assumes that limnCbfCtxPrep(fctx, lpnt) was called without error!
That (via ctxBuffersSet) allocates fctx->uu that we depend on here
(and we fail via biff if it seems like that buffer was not set)
*/
static int /* Biff: 1 */
fitSingle(double alpha[2], const double vv0[2], const double tt1[2], const double tt2[2],
          const double vv3[2], limnCbfCtx *fctx, const limnCbfPoints *lpnt, uint loi,
          uint hii) {
  static const char me[] = "fitSingle";
  uint iter, spanlen = spanLength(lpnt, loi, hii);

  if (!(alpha && vv0 && tt1 && tt2 && vv3 && fctx && lpnt)) {
    biffAddf(LIMN, "%s: got NULL pointer", me);
    return 1;
  }
  if (!(fctx->uu)) {
    biffAddf(LIMN, "%s: fcgtx->uu NULL; was limnCbfCtxPrep called?", me);
    return 1;
  }
  /* DIM=2 pretty much everywhere here */
  if (fctx->verbose) {
    printf("%s[%d,%d]: hello, vv0=(%g,%g), tt1=(%g,%g), "
           "tt2=(%g,%g), vv3=(%g,%g)\n",
           me, loi, hii, vv0[0], vv0[1], tt1[0], tt1[1], tt2[0], tt2[1], vv3[0], vv3[1]);
  }
  if (2 == spanlen) {
    /* relying on code in findAlpha() that handles slen==2; return should be 1 */
    if (1 != findAlpha(alpha, -2, fctx, vv0, tt1, tt2, vv3, lpnt, loi, hii)) {
      biffAddf(LIMN, "%s: what? findAlpha should have returned 1 with spanlen=2", me);
      return 1;
    }
    /* nrp is moot */
    fctx->nrpIterDone = 0;
    fctx->nrpPuntFlop = 0;
    /* emmulate results of calling findDist() */
    fctx->distMax = fctx->nrpDeltaDone = 0;
    fctx->distMaxIdx = 0;
    fctx->distBig = 0;
  } else {             /* slen >= 3 */
    double delta;      /* avg parameterization change of interior points */
    int lastPunt;      /* last return from fitSingle, ==1 if it punted */
    uint puntFlop = 0; /* # times that return from fitSingle changed */
    {
      /* initialize uu parameterization to chord length */
      unsigned int ii;
      double len;
      const double *xyP, *xyM;
      fctx->uu[0] = len = 0;
      xyM = PPlowerI(lpnt, AIR_INT(loi));
      xyP = PPlowerI(lpnt, AIR_INT(loi + 1));
      for (ii = 1; ii < spanlen; ii++) {
        double dd[2];
        ELL_2V_SUB(dd, xyP, xyM);
        len += ELL_2V_LEN(dd);
        fctx->uu[ii] = len;
        xyM = xyP;
        /* yes on last iter this is set to a coord that's not used */
        xyP = PPlowerI(lpnt, AIR_INT(loi + ii + 1));
      }
      delta = 0;
      for (ii = 0; ii < spanlen; ii++) {
        if (ii < spanlen - 1) {
          fctx->uu[ii] /= len;
          delta += fctx->uu[ii];
        } else {
          /* ii == spanlen-1 last vertex */
          fctx->uu[ii] = 1;
        }
        if (fctx->verbose > 1) {
          printf("%s[%d,%d]: initial uu[%u] = %g\n", me, loi, hii, ii, fctx->uu[ii]);
        }
      }
      delta /= spanlen - 2; /* within the spanlen verts are spanlen-2 interior verts */
      if (fctx->verbose) {
        printf("%s[%d,%d]: initial (chord length) delta = %g\n", me, loi, hii, delta);
      }
    }
    lastPunt = findAlpha(alpha, -1, fctx, vv0, tt1, tt2, vv3, lpnt, loi, hii);
    if (findDist(fctx, alpha, vv0, tt1, tt2, vv3, lpnt, loi, hii)) {
      biffAddf(LIMN, "%s: trouble", me);
      return 1;
    }
    if (fctx->verbose) {
      printf("%s[%d,%d]: found (%s) alpha %g %g, maxdist %g @ %u (big %d) (%u max nrp "
             "iters)\n",
             me, loi, hii, lastPunt ? "punt" : "calc", alpha[0], alpha[1], fctx->distMax,
             fctx->distMaxIdx, fctx->distBig, fctx->nrpIterMax);
    }
    if (fctx->distBig < 3) {
      /* initial fit isn't awful; try making it better with nrp */
      int converged = AIR_FALSE;
      for (iter = 0; fctx->distBig && iter < fctx->nrpIterMax; iter++) {
        int punt;
        if (fctx->verbose > 1) {
          printf("%s[%d,%d]: nrp iter %u starting with alpha %g,%g (det %g) (big %d)\n",
                 me, loi, hii, iter, alpha[0], alpha[1], fctx->alphaDet, fctx->distBig);
        }
        /* *this* is where nrp = Newton-based ReParameterization happens */
        delta = reparm(fctx, alpha, vv0, tt1, tt2, vv3, lpnt, loi, hii);
        punt = findAlpha(alpha, iter, fctx, vv0, tt1, tt2, vv3, lpnt, loi, hii);
        puntFlop += punt != lastPunt;
        /* seems like a good idea, but not obviously needed, and has some false positives
        if (puntFlop > 3 && puntFlop - 1 > iter / 2) {
          biffAddf(LIMN,
                   "%s: (nrp iter %u) previous findAlpha return %d but now %d, and has "
                   "changed %d times => whether to punt too unstable under nrp",
                   me, iter, lastPunt, punt, puntFlop);
          return 1;
        }
        */
        lastPunt = punt;
        if (findDist(fctx, alpha, vv0, tt1, tt2, vv3, lpnt, loi, hii)) {
          biffAddf(LIMN, "%s: trouble", me);
          return 1;
        }
        if (fctx->verbose > 1) {
          printf("%s[%d,%d]: nrp iter %u (reparm) delta = %g (big %d)\n", me, loi, hii,
                 iter, delta, fctx->distBig);
        }
        if (delta <= fctx->nrpDeltaThresh) {
          if (fctx->verbose) {
            printf("%s[%d,%d]: nrp iter %u delta %g <= thresh %g --> break\n", me, loi,
                   hii, iter, delta, fctx->nrpDeltaThresh);
          }
          converged = AIR_TRUE;
          break;
        }
        /* maybe TODO: add logic here to catch if delta is getting bigger and bigger,
        i.e. the reparm is diverging instead of converging. A younger GLK seemed to think
        this could happen with the spline making a loop away from a small number of
        points, e.g.: 4 points on spline defined by vv0 = (1,1), tt1 = (1,2), tt2 =
        (1,2), vv3 = (0,1). On the other hand, it's not like we have a strategy for
        doing a different/smarter reparm to handle that, and if it does happen, our
        failure to fit will likely (in the context of limnCbfMulti) merely trigger
        subdivision, which isn't terrible */
      }
      if (fctx->verbose) {
        printf("%s[%d,%d]: nrp done after %u iters: ", me, loi, hii, iter);
        if (converged) {
          printf("converged! with maxdist %g @ %u (big %d)\n", fctx->distMax,
                 fctx->distMaxIdx, fctx->distBig);
        } else if (!fctx->distBig) {
          printf("NICE small dist %g (<%g) @ %u (big 0)\n", fctx->distMax, fctx->epsilon,
                 fctx->distMaxIdx);
        } else {
          printf("hit nrp itermax %u; maxdist %g @ %u (big %d)\n", fctx->nrpIterMax,
                 fctx->distMax, fctx->distMaxIdx, fctx->distBig);
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
    fctx->nrpPuntFlop = puntFlop;
  }
  if (fctx->verbose) {
    printf("%s[%d,%d]: leaving with alpha %g %g\n", me, loi, hii, alpha[0], alpha[1]);
  }
  return 0;
}

static int
vttvCalcOrCopy(double *vttv[4], int *givenP, const double vv0[2], const double tt1[2],
               const double tt2[2], const double vv3[2], limnCbfCtx *fctx,
               const limnCbfPoints *lpnt, uint loi, uint hii) {
  static const char me[] = "vttvCalcOrCopy";

  /* DIM=2 throughout! */
  if (vv0 && tt1 && tt2 && vv3) {
    /* copy given geometry info */
    ELL_2V_COPY(vttv[0], vv0);
    ELL_2V_COPY(vttv[1], tt1);
    ELL_2V_COPY(vttv[2], tt2);
    ELL_2V_COPY(vttv[3], vv3);
    if (givenP) *givenP = AIR_TRUE;
  } else {
    double v0c[2], t1c[2], t2c[2], v3c[3]; /* locally computed info */
    if (vv0 || tt1 || tt2 || vv3) {
      biffAddf(LIMN,
               "%s: should either give all vv0,tt1,tt2,vv3 or none (not %p,%p,%p,%p)",
               me, (const void *)vv0, (const void *)tt1, (const void *)tt2,
               (const void *)vv3);
      return 1;
    }
    /* do not have geometry info; must find it all */
    if (limnCbfTVT(NULL, v0c, t1c,            /* */
                   fctx, lpnt, loi, hii, loi, /* */
                   AIR_TRUE /* oneSided */)
        || limnCbfTVT(t2c, v3c, NULL,            /* */
                      fctx, lpnt, loi, hii, hii, /* */
                      AIR_TRUE /* oneSided */)) {
      biffAddf(LIMN, "%s: trouble finding geometry info", me);
      return 1;
    }
    if (fctx->verbose) {
      printf("%s[%u,%u]: found geometry (%g,%g) --> (%g,%g) -- (%g,%g) <-- (%g,%g)\n",
             me, loi, hii, v0c[0], v0c[1], t1c[0], t1c[1], t2c[0], t2c[1], v3c[0],
             v3c[1]);
    }
    ELL_2V_COPY(vttv[0], v0c);
    ELL_2V_COPY(vttv[1], t1c);
    ELL_2V_COPY(vttv[2], t2c);
    ELL_2V_COPY(vttv[3], v3c);
    if (givenP) *givenP = AIR_FALSE;
  }
  return 0;
}

/*
limnCbfSingle

Basically a very error-checked version of fitSingle; in the limn API because it's needed
for testing. Unlike fitSingle, the geometry info vv0, tt1, tt2, vv3 can either be punted
on (by passing NULL for all) or not, by passing specific vectors for all. The results are
converted into the fields in the given limnCbfSeg *seg.  Despite misgivings, we set
both seg->corner[0,1] to AIR_TRUE.

Perservating on seg->corner[0,1]: we really don't have the information to consistently
set them with certainty. If not given the geometry vectors, we do assert oneSided when
estimating the vertices and tangents, so maybe then we can set set->corner[0,1] to true,
but on the other hand we don't know what to do when the geometry vectors are given. Since
it is not ok to sometimes set fields in the output struct and sometimes not, we always
set them all.

This function used to also be lower-overhead, by not requiring a fctx, and taking a coord
data pointer instead of a lpnts. But for the sake of testing, there needed to be way of
passing specific loi and hii in a point loop, so that's what this accepts now.  As a
result, that means this function isn't as convenient a function for one-off single-spline
fits, and currently limn does not have one.  The real entry point for general-purpose
spline fitting is limnCbfGo().
*/
int /* Biff: 1 */
limnCbfSingle(limnCbfSeg *seg, const double vv0[2], const double tt1[2],
              const double tt2[2], const double vv3[2], limnCbfCtx *fctx,
              const limnCbfPoints *lpnt, uint loi, uint hii) {
  static const char me[] = "limnCbfSingle";
  double alpha[2], V0[2], T1[2], T2[2], V3[2], *vttv[4] = {V0, T1, T2, V3};
  airArray *mop;

  if (!(seg && fctx && lpnt)) {
    biffAddf(LIMN, "%s: got NULL pointer", me);
    return 1;
  }
  mop = airMopNew();
  if (limnCbfCtxPrep(fctx, lpnt)) {
    biffAddf(LIMN, "%s: problem with fctx or lpnt", me);
    airMopError(mop);
    return 1;
  }
  if (vttvCalcOrCopy(vttv, NULL, vv0, tt1, tt2, vv3, fctx, lpnt, loi, hii)) {
    biffAddf(LIMN, "%s: problem getting vertex or tangent info", me);
    airMopError(mop);
    return 1;
  }
  /* now actually do the work */
  if (fitSingle(alpha, vttv[0], vttv[1], vttv[2], vttv[3], fctx, lpnt, loi, hii)) {
    biffAddf(LIMN, "%s: fitSingle failed", me);
    airMopError(mop);
    return 1;
  }

  /* process the results to generate info in output limnCbfSeg */
  ELL_2V_COPY(seg->xy + 0, V0);
  ELL_2V_SCALE_ADD2(seg->xy + 2, 1, V0, alpha[0], T1);
  ELL_2V_SCALE_ADD2(seg->xy + 4, 1, V3, alpha[1], T2);
  ELL_2V_COPY(seg->xy + 6, V3);
  seg->corner[0] = seg->corner[1] = AIR_TRUE; /* misgivings . . . */
  seg->pointNum = spanLength(lpnt, loi, hii);

  airMopOkay(mop);
  return 0;
}

/*
limnCbfCorners: discover the corners in the given points: i.e. a point where the incoming
and outgoing tangents are so non-colinear that no attempt is made to fit a spline across
the point; instead it is an endpoint for different splines on either side of it.  This
sets fctx->ctvt, fctx->cidx, and fctx->cnum.

NOTE: this assumes that limnCbfCtxPrep(fctx, lpnt) was called without error!
That (via ctxBuffersSet) allocates things that we depend on here.
*/
int /* Biff: 1 */
limnCbfCorners(limnCbfCtx *fctx, const limnCbfPoints *lpnt) {
  static const char me[] = "limnCbfCorners";
  airArray *mop;
  double *angle, /* angle[i] is angle at vertex i */
    *vtvt;       /* 6-by-pnum array of tangent,vertex,tangent
                    for ALL vertices */
  int *corny,    /* corny[i] means vertex i seems like a corner */
    oneSided = AIR_TRUE;
  uint pnum = lpnt->num, hii, cnum, vi;

  if (!(fctx && lpnt)) {
    biffAddf(LIMN, "%s: got NULL pointer", me);
    return 1;
  }
  /* reset corner-related pointers */
  fctx->ctvt = airFree(fctx->ctvt);
  fctx->cidx = airFree(fctx->cidx);
  fctx->cnum = 0;

  if (fctx->verbose) {
    printf("%s: hello; cornerFind = %d; cornerNMS = %d\n", me, fctx->cornerFind,
           fctx->cornerNMS);
  }
  if (!fctx->cornerFind) {
    /* caller not interested in doing computations to discover corners */
    if (!(lpnt->isLoop)) {
      /* but we still have to describe the start and end of the points as "corners" */
      fctx->cnum = 2;
      if (!((fctx->ctvt = AIR_CALLOC(6 * fctx->cnum, double))
            && (fctx->cidx = AIR_CALLOC(fctx->cnum, uint)))) {
        biffAddf(LIMN, "%s: trouble allocating %u points", me, fctx->cnum);
        return 1;
      }
      hii = pnum - 1;
      if (limnCbfTVT(fctx->ctvt + 0, fctx->ctvt + 2, fctx->ctvt + 4, /* */
                     fctx, lpnt, 0, hii, 0, oneSided)
          || limnCbfTVT(fctx->ctvt + 6, fctx->ctvt + 8, fctx->ctvt + 10, /* */
                        fctx, lpnt, 0, hii, hii, oneSided)) {
        biffAddf(LIMN, "%s: trouble with tangents or vertices for endpoints", me);
        return 1;
      }
      fctx->cidx[0] = 0;
      fctx->cidx[1] = hii;
      if (fctx->verbose) {
        printf("%s: leaving with %u \"corners\" at 1st and last point\n", me,
               fctx->cnum);
      }
    }
    return 0;
  }
  if (fctx->verbose) {
    printf("%s: looking for corners among %u points\n", me, pnum);
  }

  /* else we search for corners */
  mop = airMopNew();
  /* allocate arrays and add them to the mop, but bail if any allocation fails */
  if (!((angle = AIR_CALLOC(pnum, double))
        && (airMopAdd(mop, angle, airFree, airMopAlways), 1)
        && (corny = AIR_CALLOC(pnum, int))
        && (airMopAdd(mop, corny, airFree, airMopAlways), 1)
        && (vtvt = AIR_CALLOC(6 * pnum, double))
        && (airMopAdd(mop, vtvt, airFree, airMopAlways), 1))) {
    biffAddf(LIMN, "%s: trouble allocating working buffers for %u points", me, pnum);
    return 1;
  }
  for (vi = 0; vi < pnum; vi++) {
    double *LT = vtvt + 0 + 6 * vi;
    double *VV = vtvt + 2 + 6 * vi;
    double *RT = vtvt + 4 + 6 * vi;
    /* we find TVT for *every* vertex, despite this seeming like computational overkill.
    Why: we don't know which vertex might be corner until we look at the
    tangent-to-tangent angles for EVERY vertex, for which we don't need to know the
    vertex position (non-trivial when scale > 0). But once we do know which vertices are
    corners, we'll need to know where the vertices are, but it would be unfortunate
    revisit the input data (used previously for tangent estimation) to figure out the
    corner vertices. In a non-loop, we know first and last points will be corners, but we
    still need to find the vertex pos and (one-sided) tangent. */
    if (limnCbfTVT(LT, VV, RT, fctx, lpnt, 0 /* loi */, 0 /* hii */, vi, oneSided)) {
      biffAddf(LIMN, "%s: trouble with tangents or vertices for point %u/%u", me, vi,
               pnum);
      airMopError(mop);
      return 1;
    }
    if (!lpnt->isLoop && (!vi || vi == pnum - 1)) {
      /* not a loop, and, either at first or last point ==> necessarily a "corner" */
      corny[vi] = AIR_TRUE;
      /* for later NMS: set high angle, to allow adjacent point to stay as corner */
      angle[vi] = 180;
    } else {
      /* it is a loop, or: not a loop and at an interior point */
      angle[vi] = 180 * ell_2v_angle_d(LT, RT) / AIR_PI;
      corny[vi] = (angle[vi] < fctx->cornAngle);
    }
    if (fctx->verbose > 1) {
      printf("%s: vi=%3u   corny %d   angle %g\n", me, vi, corny[vi], angle[vi]);
      if (corny[vi]) {
        printf("    (%g,%g)<--(%g,%g)-->(%g,%g)\n", LT[0], LT[1], VV[0], VV[1], RT[0],
               RT[1]);
      }
    }
  }
  if (fctx->cornerNMS) {
    for (vi = 0; vi < pnum; vi++) {
      if (!lpnt->isLoop && (!vi || vi == pnum - 1)) {
        /* not a loop, and, either at first or last point ==> must stay a "corner" */
        continue;
      }
      /* a little weird to be re-implementing here index lowering, but oh well */
#define PLUS(I) ((I) < pnum - 1 ? (I) + 1 : (lpnt->isLoop ? 0 : pnum - 1))
#define MNUS(I) ((I) ? (I) - 1 : (lpnt->isLoop ? pnum - 1 : 0))
      uint ip1 = PLUS(vi);
      uint ip2 = PLUS(ip1);
      uint im1 = MNUS(vi);
      uint im2 = MNUS(im1);
#undef PLUS
#undef MNUS
      corny[vi]
        &= (/* stays a corner if angle at vi is smaller (pointier) than neighbors */
            (angle[im1] > angle[vi] && /* */
             angle[vi] < angle[ip1])
            || /* OR, we are part of a pair that is surrounded by less pointy angles */
            (angle[im1] > angle[vi] &&  /* */
             angle[vi] == angle[ip1] && /* either the lower index of the pair */
             angle[ip1] < angle[ip2])   /* */
            ||                          /* */
            (angle[im2] > angle[im1] && /* */
             angle[im1] == angle[vi] && /* or the higher index of the pair */
             angle[vi] < angle[ip1]));
    }
  }
  /* now, corny[vi] iff vert vi really is a corner; so now count # corners */
  cnum = 0;
  for (vi = 0; vi < pnum; vi++) {
    cnum += !!corny[vi];
  }
  if (fctx->verbose > 1) {
    printf("%s: final corner count %u\n", me, cnum);
  }
  if (cnum) {
    unsigned int ci;
    /* can now allocate new corner-related buffers */
    if (!((fctx->ctvt = AIR_CALLOC(6 * cnum, double))
          && (fctx->cidx = AIR_CALLOC(cnum, uint)))) {
      biffAddf(LIMN, "%s: trouble allocating info for %u corners", me, cnum);
      return 1;
    }
    /* now fill in the corner buffers */
    ci = 0;
    for (vi = 0; vi < pnum; vi++) {
      if (corny[vi]) {
        double *id, *od;
        fctx->cidx[ci] = vi;
        id = vtvt + 6 * vi;
        od = fctx->ctvt + 6 * ci;
        ELL_6V_COPY(od, id);
        if (fctx->verbose) {
          printf("%s: corner %u is vertex %u\n  T,V,T = (%g,%g)  (%g,%g)  (%g,%g)\n", me,
                 ci, vi, od[0], od[1], od[2], od[3], od[4], od[5]);
        }
        ci++;
      }
    }
  }
  fctx->cnum = cnum;

  airMopOkay(mop);
  return 0;
}

/*
limnCbfMulti: Fits one or more geometrically continuous splines to a set of points.
Does NOT create new internal "corners" (which break geometric continuity with
non-colinear incoming and outgoing tangents), but does recursively subdivide the points
into left and right sides around points with the highest error from fitSingle.

NOTE: this assumes that limnCbfCtxPrep(fctx, lpnt) was called without error!
That (via ctxBuffersSet) allocates things that we depend on here.
*/
int /* Biff: 1 */
limnCbfMulti(limnCbfPath *path, const double vv0[2], const double tt1[2],
             const double tt2[2], const double vv3[2], unsigned int recDepth,
             limnCbfCtx *fctx, const limnCbfPoints *lpnt, uint loi, uint hii) {
  static const char me[] = "limnCbfMulti";
  double alpha[2], V0[2], T1[2], T2[2], V3[2], *vttv[4] = {V0, T1, T2, V3};
  int geomGiven;

  if (!(path && fctx && lpnt)) {
    biffAddf(LIMN, "%s: got NULL pointer", me);
    return 1;
  }
  if (vttvCalcOrCopy(vttv, &geomGiven, vv0, tt1, tt2, vv3, fctx, lpnt, loi, hii)) {
    biffAddf(LIMN, "%s: problem getting vertex or tangent info", me);
    return 1;
  }
  if (fctx->verbose) {
    printf("%s[%u,%u]_%u: hello; %s v0=(%g,%g), t1=(%g,%g), t2=(%g,%g), "
           "v3=(%g,%g)\n",
           me, loi, hii, recDepth, geomGiven ? "given" : "computed", V0[0], V0[1], T1[0],
           T1[1], T2[0], T2[1], V3[0], V3[1]);
  }

  /* first try fitting a single spline */
  if (fctx->verbose) {
    printf("%s[%u,%u]_%u: trying single fit on all points\n", me, loi, hii, recDepth);
  }
  if (fitSingle(alpha, V0, T1, T2, V3, fctx, lpnt, loi, hii)) {
    biffAddf(LIMN, "%s: fitSingle failed", me);
    return 1;
  }
  if (fctx->distBig <= 1) {
    /* max dist was <= fctx->epsilon: single fit was good enough */
    if (fctx->verbose) {
      printf("%s[%u,%u]_%u: single fit good! nrpi=%u; maxdist=%g @ %u <= %g; "
             "big=%d det=%g alpha=%g,%g\n",
             me, loi, hii, recDepth, fctx->nrpIterDone, fctx->distMax, fctx->distMaxIdx,
             fctx->epsilon, fctx->distBig, fctx->alphaDet, alpha[0], alpha[1]);
    }
    airArrayLenSet(path->segArr, 1);
    ELL_2V_COPY(path->seg[0].xy + 0, V0);
    ELL_2V_SCALE_ADD2(path->seg[0].xy + 2, 1, V0, alpha[0], T1);
    ELL_2V_SCALE_ADD2(path->seg[0].xy + 4, 1, V3, alpha[1], T2);
    ELL_2V_COPY(path->seg[0].xy + 6, V3);
  } else {
    /* need to subdivide at fctx->distMaxIdx and recurse.  But this is NOT an occasion to
    create a new *corner* there (all corners were likely found as pre-process by
    limnCbfCorners, called by limnCbfGo). We find a 2-sided tangent and vertex */
    uint midi = fctx->distMaxIdx;
    double TL[2], VM[2], TR[2];
    const double *pp = PP(lpnt);
    limnCbfCtx fctxL, fctxR;
    limnCbfPath *pRth = limnCbfPathNew(0); /* path on right side of new middle */
    /* holy moly sneakiness: we make two shallow copies of the context, because we
    need each one's output information, but we don't need to re-allocate any of the
    internal buffers, because there is no concurrency */
    memcpy(&fctxL, fctx, sizeof(limnCbfCtx));
    memcpy(&fctxR, fctx, sizeof(limnCbfCtx));
    if (fctx->verbose) {
      printf("%s[%u,%u]_%u: dist %g (big %d) --> split at %u (%g,%g)\n", me, loi, hii,
             recDepth, fctx->distMax, fctx->distBig, midi, (pp + 2 * midi)[0],
             (pp + 2 * midi)[1]);
    }
    if (limnCbfTVT(TL, VM, TR,                 /* */
                   fctx, lpnt, loi, hii, midi, /* */
                   AIR_FALSE /* oneSided */)
        || limnCbfMulti(path, V0, T1, TL, VM, recDepth + 1, &fctxL, lpnt, loi, midi)
        || limnCbfMulti(pRth, VM, TR, T2, V3, recDepth + 1, &fctxR, lpnt, midi, hii)) {
      biffAddf(LIMN, "%s[%u,%u]_%u: trouble on recursive fit (midvert %u)", me, loi, hii,
               recDepth, midi);
      limnCbfPathNix(pRth);
      return 1;
    }
    limnCbfPathJoin(path, pRth);
    limnCbfPathNix(pRth);
    fctx->nrpIterDone = fctxL.nrpIterDone + fctxR.nrpIterDone;
    if (fctxL.distMax > fctxR.distMax) {
      fctx->distMax = fctxL.distMax;
      fctx->distMaxIdx = fctxL.distMaxIdx;
      fctx->distBig = fctxL.distBig;
    } else {
      fctx->distMax = fctxR.distMax;
      fctx->distMaxIdx = fctxR.distMaxIdx;
      fctx->distBig = fctxR.distBig;
    }
    fctx->nrpDeltaDone = AIR_MAX(fctxL.nrpDeltaDone, fctxR.nrpDeltaDone);
    fctx->alphaDet = AIR_MIN(fctxL.alphaDet, fctxR.alphaDet);
  }

  return 0;
}

/*
limnCbfGo

Top-level function for fitting cubic Beziers to given points. The name should probably be
limnCBFit (since CBF stands for Cubic Bezier Fit), but various other clumsy function
names led to a CBF -> Cbf change, so we add the shortest appropriate verb).

All the knobs and controls for how this works are inside the limnCCbfCtx *fctx, and
limn.h describes those "input" fields, which are set directly in the struct (rather than
via some API). The input points have to be wrapped up in the limnCbfPoints *lpnt
via limnCbfPointsNew. The output path is saved in limnCbfPath *path.

*This* is the function that is responsible for setting path->seg[i].corner[0,1] to true
(after they are uniformly initialized to false by segInit()); it would be too confusing
for the recursive limnCbfMulti to set corner[0,1]. The weird limnCbfSingle also sets
corner[0,1], in keeping with it being more public-facing.
*/
int /* Biff: 1 */
limnCbfGo(limnCbfPath *path, limnCbfCtx *fctx, const limnCbfPoints *lpnt) {
  static const char me[] = "limnCbfGo";

  if (!(path && fctx && lpnt)) {
    biffAddf(LIMN, "%s: got NULL pointer", me);
    return 1;
  }
  if (limnCbfCtxPrep(fctx, lpnt)) {
    biffAddf(LIMN, "%s: trouble preparing", me);
    return 1;
  }
  /* if !fctx->cornerFind, this will set fctx internal state to reflect that */
  if (limnCbfCorners(fctx, lpnt)) {
    biffAddf(LIMN, "%s: trouble finding corners", me);
    return 1;
  }
  airArrayLenSet(path->segArr, 0);
  if (!fctx->cnum) {
    assert(lpnt->isLoop);
    /* in a loop and no corners known */
    if (fctx->verbose) {
      printf("%s: no corners: finding path to fit point loop", me);
    }
    if (limnCbfMulti(path, NULL, NULL, NULL, NULL, 0, fctx, lpnt, 0 /* loi */,
                     0 /* hii */)) {
      biffAddf(LIMN, "%s: trouble fitting point loop", me);
      return 1;
    }
  } else {
    /* we do have corners: find segments between corners. The corner vertex part of two
    segments: the last point in segment I and first point in segment I+1 */
    uint cii;
    for (cii = 0; cii < fctx->cnum; cii++) {
      uint cjj = (cii + 1) % fctx->cnum;
      limnCbfPath *subpath = limnCbfPathNew(0);
      /* 0: left tangent   2: vertex   4: right tangent */
      const double *V0 = fctx->ctvt + 2 * cii, *T1 = fctx->ctvt + 4 * cii,
                   *T2 = fctx->ctvt + 0 * cjj, *V3 = fctx->ctvt + 2 * cjj;
      uint loi = fctx->cidx[cii];
      uint hii = fctx->cidx[cjj];
      if (fctx->verbose) {
        printf("%s: finding subpath from between corners [%u,%u] (points [%u,%u])", me,
               cii, cjj, loi, hii);
      }
      if (limnCbfMulti(subpath, V0, T1, T2, V3, 0, fctx, lpnt, loi, hii)) {
        biffAddf(LIMN, "%s: trouble with corners [%u,%u] (points [%u,%u])", me, cii, cjj,
                 loi, hii);
        limnCbfPathNix(subpath);
        return 1;
      }
      subpath->seg[0].corner[0] = 1;
      subpath->seg[subpath->segNum - 1].corner[1] = 1;
      limnCbfPathJoin(path, subpath);
      limnCbfPathNix(subpath);
    }
  }

  path->isLoop = lpnt->isLoop;
  return 0;
}
