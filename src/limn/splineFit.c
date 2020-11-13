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
typedef unsigned int uint;
#include <assert.h>

/*
  This file contains GLK's implementation of the curve fitting described in:
  Philip J. Schneider. “An Algorithm for Automatically Fitting Digitized
  Curves”. In Graphics Gems, Academic Press, 1990, pp. 612–626.
  https://dl.acm.org/doi/10.5555/90767.90941
  The author's code is here:
  http://www.realtimerendering.com/resources/GraphicsGems/gems/FitCurves.c

  The functions below do not use any existing limnSpline structs or functions;
  those were written a long time ago, and reflect GLK's ignorance about
  splines at the time.  Hopefully this will be revisited and re-organized in a
  later version of Teem, at which point the code below can be integrated with
  the rest of limn, but this too will benefit from ongoing scrutiny and
  re-writing; ignorance persists.
*/

/* CB0, CB1, CB2, CB3 = degree 3 Bernstein polynomials, for *C*ubic
   *B*ezier curves, and their derivatives D0, D1, D2 (not using any
   nice recursion properties for evaluation, oh well) */
#define CB0D0(T) ((1-(T))*(1-(T))*(1-(T)))
#define CB1D0(T) (3*(T)*(1-(T))*(1-(T)))
#define CB2D0(T) (3*(T)*(T)*(1-(T)))
#define CB3D0(T) ((T)*(T)*(T))

#define CB0D1(T) (-3*(1-(T))*(1-(T)))
#define CB1D1(T) (3*((T)-1)*(3*(T)-1))
#define CB2D1(T) (3*(T)*(2-3*(T)))
#define CB3D1(T) (3*(T)*(T))

#define CB0D2(T) (6*(1-(T)))
#define CB1D2(T) (6*(3*(T)-2))
#define CB2D2(T) (6*(1-3*(T)))
#define CB3D2(T) (6*(T))

/* set 4-vector of weights W by evaluating all CBi at T */
#define VCBD0(W,T) ((W)[0] = CB0D0(T), \
                    (W)[1] = CB1D0(T), \
                    (W)[2] = CB2D0(T), \
                    (W)[3] = CB3D0(T))
#define VCBD1(W,T) ((W)[0] = CB0D1(T), \
                    (W)[1] = CB1D1(T), \
                    (W)[2] = CB2D1(T), \
                    (W)[3] = CB3D1(T))
#define VCBD2(W,T) ((W)[0] = CB0D2(T), \
                    (W)[1] = CB1D2(T), \
                    (W)[2] = CB2D2(T), \
                    (W)[3] = CB3D2(T))

/* get 4-vector of weights at T, and evaluate spline by adding up
   control points, using weight vector buffer W */
#define CBDI(P, CB, V0, V1, V2, V3, T, W)             \
  (CB(W, T),                                          \
   ELL_2V_SCALE_ADD4(P,                               \
                     (W)[0], (V0),                    \
                     (W)[1], (V1),                    \
                     (W)[2], (V2),                    \
                     (W)[3], (V3)))
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
  CBD0(vv,
       xy + 0, xy + 2, xy + 4, xy + 6,
       tt, ww);
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
  for (ii=0; ii<pNum; ii++) {
    uint segi = airIndex(0, ii, pNum-1, sNum);
    const limnCBFSeg *seg = path->seg + segi;
    double tmpf = AIR_AFFINE(0, ii, pNum-1, 0, sNum);
    double tt = tmpf - segi;
    limnCBFSegEval(xy + 2*ii, seg, tt);
    /*
    fprintf(stderr, "!%s: %u -> %u (%g) %g -> (%g,%g)\n",
            "limnCBFPathSample", ii, segi, tmpf, tt,
            (xy + 2*ii)[0], (xy + 2*ii)[1]);
    */
  }
  return;
}

/*
** (from paper page 620) solves for the alpha that minimize squared error
** between xy[i] and Q(uu[i]) where Q(t) is cubic Bezier spline through vv0,
** vv0 + alpha[0]*tt1, vv3 + alpha[1]*tt2, and vv3.
**
** There are various conditions where the generated spline ignores the
** xy array and instead is what one could call a "simple arc" (with
** control points at 1/3 and 2/3 the distance between the end points):
**  - having only two points (xy contains only the end points)
**  - the determinant of the 2x2 matrix that is inverted to solve
**    for alpha is too close to zero (this test was not part of the
**    author's code)
**  - the solved alphas are not convincingly positive
** This function is the only place where the "simple arc" is
** generated, and generating the simple arc is not actually an error
** or problem: if it is bad at fitting the data (as determined by
** finddist) then it may be subdivided, and that's ok. What GLK hasn't
** thought through is: what is the interaction of nrp iterations and
** findalpha generating the simple arc on some but not all iterations
** (possibly unstable?)
*/
static void
findalpha(double alpha[2],
          limnCBFInfo *cbfi, /* must be non-NULL */
          const double vv0[2], const double tt1[2],
          const double tt2[2], const double vv3[2],
          const double *xy, const double *uu, uint pNum) {
  const char me[]="findalpha";
  uint ii;
  double det;

  if (pNum > 2) {
    double xx[2], m11, m12, m22, MM[4], MI[4];
    xx[0] = xx[1] = m11 = m12 = m22 = 0;
    for (ii=0; ii<pNum; ii++) {
      double bb[4], Ai1[2], Ai2[2], Pi[2], dmP[2];
      double ui = uu[ii];
      VCBD0(bb, ui);
      ELL_2V_SCALE(Ai1, bb[1], tt1);
      ELL_2V_SCALE(Ai2, bb[2], tt2);
      /* GLK using "m" and "M" instead author's "C". Note that Ai1 and
         Ai2 are scalings of (nominally) unit-length tt1 and tt2 by
         evaluations of the spline basis functions, so they (and the M
         computed from them, and det(M)), are invariant w.r.t over-all
         rescalings of the data points */
      m11 += ELL_2V_DOT(Ai1, Ai1);
      m12 += ELL_2V_DOT(Ai1, Ai2);
      m22 += ELL_2V_DOT(Ai2, Ai2);
      ELL_2V_SCALE_ADD2(Pi, bb[0]+bb[1], vv0, bb[2]+bb[3], vv3);
      ELL_2V_SUB(dmP, xy + 2*ii, Pi);
      xx[0] += ELL_2V_DOT(dmP, Ai1);
      xx[1] += ELL_2V_DOT(dmP, Ai2);
    }
    ELL_4V_SET(MM, m11, m12, m12, m22);
    ELL_2M_INV(MI, MM, det);
    ELL_2MV_MUL(alpha, MI, xx);
  } else {
    det = 1; /* actually bogus */
    alpha[0] = alpha[1] = 0; /* trigger simple arc code */
  }
  /* test if we should return simple arc */
  if (!( AIR_EXISTS(det) && AIR_ABS(det) > cbfi->detMin
         && alpha[0] > (cbfi->lenF2L)*(cbfi->alphaMin)
         && alpha[1] > (cbfi->lenF2L)*(cbfi->alphaMin) )) {
    if (cbfi->verbose) {
      printf("%s: bad |det| %g (vs %g) or alpha %g,%g (vs %g*%g) "
             "--> simple arc\n",
             me, AIR_ABS(det), cbfi->detMin, alpha[0], alpha[1],
             cbfi->lenF2L, cbfi->alphaMin);
    }
    /* generate simple arc: set both alphas to 1/3 of distance from
       first to last point, but also handle non-unit-length tt1 and
       tt2 */
    alpha[0] = cbfi->lenF2L/(3*ELL_2V_LEN(tt1));
    alpha[1] = cbfi->lenF2L/(3*ELL_2V_LEN(tt2));
  } else {
    if (cbfi->verbose > 1) {
      printf("%s: all good: det %g, alpha %g,%g\n",
             me, det, alpha[0], alpha[1]);
    }
  }
  cbfi->alphaDet = det;
  return;
}

/*
** using Newton iterations to try to find a better places at which
** to evaluate the spline in order to match the given points xy
*/
static double
reparm(const limnCBFInfo *cbfi, /* must be non-NULL */
       const double alpha[2],
       const double vv0[2], const double tt1[2],
       const double tt2[2], const double vv3[2],
       const double *xy, double *uu, uint pNum) {
  const char me[]="reparm";
  uint ii;
  double vv1[2], vv2[2], delta, maxdelu;

  assert(pNum >= 3);
  /* average u[i+1]-u[i] is 1/(pNum-1) */
  maxdelu = cbfi->nrpDeltaScl/(pNum-1);
  ELL_2V_SCALE_ADD2(vv1, 1, vv0, alpha[0], tt1);
  ELL_2V_SCALE_ADD2(vv2, 1, vv3, alpha[1], tt2);
  delta = 0;
  /* only changing parameterization of interior points,
     not the first (ii=0) or last (ii=pNum-1) */
  for (ii=1; ii<pNum-1; ii++) {
    double numer, denom, delu, df[2], ww[4], tt, Q[2], QD[2], QDD[2];
    tt = uu[ii];
    CBD0(Q,   vv0, vv1, vv2, vv3, tt, ww);
    CBD1(QD,  vv0, vv1, vv2, vv3, tt, ww);
    CBD2(QDD, vv0, vv1, vv2, vv3, tt, ww);
    ELL_2V_SUB(df, Q, xy + 2*ii);
    numer = ELL_2V_DOT(df, QD);
    denom = ELL_2V_DOT(QD, QD) + ELL_2V_DOT(df, QDD);
    delu = numer/denom;
    if (AIR_ABS(delu) > maxdelu) {
      /* cap Newton step */
      delu = maxdelu*airSgn(delu);
    }
    uu[ii] = tt - delu;
    delta += AIR_ABS(delu);
    if (cbfi->verbose > 1) {
      printf("%s[%2u]: %g <-- %g - %g\n", me, ii,
             uu[ii], tt, delu);
    }
  }
  delta /= pNum-2;
  /* HEY TODO: need to make sure that half-way between points,
     spline isn't wildly diverging; this can happen with the
     spline making a loop away from a small number of points, e.g.:
     4 points spline defined by vv0 = (1,1), tt1 = (1,2),
     tt2 = (1,2), vv3 = (0,1) */
  return delta;
}

/* sets cbfi->dist to max distance to spline, at point cbfi->distIdx,
   and sets cbfi->distBig */
static void
finddist(limnCBFInfo *cbfi,
         const double alpha[2],
         const double vv0[2], const double tt1[2],
         const double tt2[2], const double vv3[2],
         const double *xy, const double *uu,
         uint pNum) {
  uint ii, distI;
  double vv1[2], vv2[2], dist;
  assert(pNum >= 3);
  ELL_2V_SCALE_ADD2(vv1, 1, vv0, alpha[0], tt1);
  ELL_2V_SCALE_ADD2(vv2, 1, vv3, alpha[1], tt2);
  dist = AIR_NAN;
  for (ii=1; ii<pNum-1; ii++) {
    double len, Q[2], df[2], ww[4];
    CBD0(Q, vv0, vv1, vv2, vv3, uu[ii], ww);
    ELL_2V_SUB(df, Q, xy + 2*ii);
    len = ELL_2V_LEN(df);
    if (!AIR_EXISTS(dist) || len > dist) {
      dist = len;
      distI = ii;
    }
  }
  cbfi->dist = dist;
  cbfi->distIdx = distI;
  cbfi->distBig = (dist <= cbfi->nrpDistScl * cbfi->distMin
                   ? 0
                   : (dist <= cbfi->distMin
                      ? 1
                      : (dist <= cbfi->nrpPsi * cbfi->distMin
                         ? 2
                         : 3)));
  return;
}

void
limnCBFInfoInit(limnCBFInfo *cbfi, int outputOnly) {
  if (!cbfi) return;
  if (!outputOnly) {
    /* defaults for input parameters to various CBF functions */
    cbfi->verbose = 0;
    cbfi->nrpIterMax = 10;
    cbfi->baseIdx = 0;
    cbfi->distMin = 0;
    cbfi->alphaMin = 0.001;
    cbfi->nrpDeltaScl = 3.0;
    cbfi->nrpDistScl = 0.8;
    cbfi->nrpPsi = 6;
    cbfi->nrpDeltaMin = 0.001;
    cbfi->detMin = 0.01;
  }
  /* initialize outputs to bogus valus */
  cbfi->lenF2L = AIR_NAN;
  cbfi->nrpIterDone = (uint)(-1);
  cbfi->distIdx = (uint)(-1);
  cbfi->dist = AIR_POS_INF;
  cbfi->nrpDeltaDone = AIR_POS_INF;
  cbfi->alphaDet = 0;
  cbfi->distBig = 0;
  return;
}

/*
******** limnCBFSingle
**
** Fits a single cubic Bezier spline: from pNum (x,y) points in xy, and from
** initial endpoint vv0, initial tangent tt1, final endpoint vv3 and final
** tangent tt2 (pointing backwards), find alpha such that the cubic Bezier
** spline with control points vv0, vv0 + alpha[0]*tt1, vv3 + alpha[1]*tt2, vv3
** approximates all the given points.  This is an iterative process, in which
** alpha is solved for multiples times, after taking a Newton step to try to
** optimize the parameterization of the points (in an array that is not passed
** in but instead internal to this function); limn.h calls this process
** "nrp". nrp iterations are stopped after any one of following is true
** (the original published method did not have these fine-grained controls):
**  - have done nrpIterMax iterations of nrp
**  - if cbfi->nrpDeltaMin > 0: parameterization change falls below deltaMin
**  - if cbfi->distMin > 0: distance from spline (as evaluated at the
**    current parameterization) to the given points falls below
**    cbfi->nrpDistScl * cbfi->distMin
** Information about the results of this process are set in the given
** _cbfi, if non-NULL.
*/
int
limnCBFSingle(double alpha[2], limnCBFInfo *_cbfi,
              const double vv0[2], const double tt1[2],
              const double tt2[2], const double vv3[2],
              const double *xy, uint pNum) {
  const char me[]="limnCBFSingle";
  double *uu;
  uint iter;
  int loi, hii;
  limnCBFInfo *cbfi, mycbfi;

  if (_cbfi) {
    cbfi = _cbfi;   /* caller has supplied info */
    limnCBFInfoInit(cbfi, AIR_TRUE /* outputOnly */);
    loi = (int)cbfi->baseIdx;
    hii = (int)cbfi->baseIdx+pNum-1;
  } else {
    cbfi = &mycbfi; /* caller wants default parms */
    limnCBFInfoInit(cbfi, AIR_FALSE /* outputOnly */);
    loi = hii = -1;
  }
  if (!(alpha && vv0 && tt1 && tt2 && vv3 && xy)) {
    biffAddf(LIMN, "%s[%d,%d]: got NULL pointer", me, loi, hii);
    return 1;
  }
  if (!(pNum >= 2)) {
    biffAddf(LIMN, "%s[%d,%d]: need 2 or more points (not %u)",
             me, loi, hii, pNum);
    return 1;
  }
  /* TODO: figure out how to avoid repeating these next tests on cbfi;
     will be needlessly repeated when called from limnCBFMulti */
  if (!( cbfi->nrpIterMax > 0 )) {
    biffAddf(LIMN, "%s[%d,%d]: need nrpIterMax > 0", me, loi, hii);
    return 1;
  }
  if (cbfi->nrpDeltaMin < 0 || cbfi->distMin < 0) {
    biffAddf(LIMN, "%s[%d,%d]: cannot have negative nrpDeltaMin (%g) or "
             "distMin (%g)", me, loi, hii, cbfi->nrpDeltaMin, cbfi->distMin);
    return 1;
  }
  if (!( 0 < cbfi->nrpDistScl && cbfi->nrpDistScl <= 1 )) {
    biffAddf(LIMN, "%s[%d,%d]: nrpDistScl (%g) must be in (0,1]",
             me, loi, hii, cbfi->nrpDistScl);
    return 1;
  }
  if (!( 1 <= cbfi->nrpPsi )) {
    biffAddf(LIMN, "%s[%d,%d]: nrpPsi (%g) must be >= 1",
             me, loi, hii, cbfi->nrpPsi);
    return 1;
  }

  if (cbfi->verbose) {
    printf("%s[%d,%d]: hello, vv0=(%g,%g), tt1=(%g,%g), tt2=(%g,%g), vv3=(%g,%g)\n",
           me, loi, hii, vv0[0], vv0[1], tt1[0], tt1[1],
           tt2[0], tt2[1], vv3[0], vv3[1]);
  }
  { double F2L[2];
    ELL_2V_SUB(F2L, xy + 2*(pNum-1), xy);
    cbfi->lenF2L = ELL_2V_LEN(F2L);
  }
  if (2 == pNum) {
    /* relying on code in findalpha() that handles pNum==2 */
    findalpha(alpha, cbfi, vv0, tt1, tt2, vv3, NULL, NULL, 2);
    /* nrp is moot */
    cbfi->nrpIterDone = 0;
    /* emmulate results of calling finddist() */
    cbfi->dist = cbfi->nrpDeltaDone = 0;
    cbfi->distIdx = 0;
    cbfi->distBig = 0;
  } else { /* pNum >= 3 */
    double delta; /* avg parameterization change of interior points */
    uu = AIR_CALLOC(pNum*2, double);
    if (!uu) {
      biffAddf(LIMN, "%s[%d,%d]: failed to allocate parameter buffer",
               me, loi, hii);
      return 1;
    }
    /* initialize uu parameterization to chord length */
    { unsigned int ii; double len;
      uu[0] = len = 0;
      for (ii=1; ii<pNum; ii++) {
        double dd[2];
        ELL_2V_SUB(dd, xy + 2*ii, xy + 2*(ii-1));
        len += ELL_2V_LEN(dd);
        uu[ii] = len;
      }
      delta = 0;
      for (ii=0; ii<pNum; ii++) {
        uu[ii] /= len;
        if (cbfi->verbose > 1) {
          printf("%s[%d,%d]: intial uu[%u] = %g\n", me, loi, hii, ii, uu[ii]);
        }
        delta += AIR_ABS(uu[ii]);
      }
      delta /= pNum-2;
      if (cbfi->verbose) {
        printf("%s[%d,%d]: initial (chord length) delta = %g\n",
               me, loi, hii, delta);
      }
    }
    findalpha(alpha, cbfi, vv0, tt1, tt2, vv3, xy, uu, pNum);
    finddist(cbfi, alpha, vv0, tt1, tt2, vv3, xy, uu, pNum);
    if (cbfi->distBig < 3) {
      for (iter=0; cbfi->distBig && iter<cbfi->nrpIterMax; iter++) {
        if (cbfi->verbose) {
          printf("%s[%d,%d]: iter %u starting with alpha %g,%g (det %g)\n",
                 me, loi, hii, iter, alpha[0], alpha[1], cbfi->alphaDet);
        }
        delta = reparm(cbfi, alpha, vv0, tt1, tt2, vv3, xy, uu, pNum);
        findalpha(alpha, cbfi, vv0, tt1, tt2, vv3, xy, uu, pNum);
        finddist(cbfi, alpha, vv0, tt1, tt2, vv3, xy, uu, pNum);
        if (cbfi->verbose) {
          printf("%s[%d,%d]: iter %u (reparm) delta = %g\n", me, loi, hii,
                 iter, delta);
        }
        if (cbfi->nrpDeltaMin && delta <= cbfi->nrpDeltaMin) {
          if (cbfi->verbose) {
            printf("%s[%d,%d]: iter %u delta %g <= min %g --> break\n", me,
                   loi, hii, iter, delta, cbfi->nrpDeltaMin);
          }
          break;
        }
      }
      if (cbfi->verbose) {
        if (!cbfi->distBig) {
          printf("%s[%d,%d]: iter %u finished with good small dist %g\n",
                 me, loi, hii, iter, cbfi->dist);
        } else {
          printf("%s[%d,%d]: hit max iters %u with bad (%d) dist %g\n",
                 me, loi, hii, iter, cbfi->distBig, cbfi->dist);
        }
      }
      cbfi->nrpIterDone = iter;
    } else {
      /* else dist so big that we don't even try nrp */
      cbfi->nrpIterDone = 0;
    }
    free(uu);
    cbfi->nrpDeltaDone = delta;
  }
  return 0;
}

static void
segInit(void *_seg) {
  limnCBFSeg *seg = (limnCBFSeg *)_seg;
  ELL_2V_NAN_SET(seg->xy + 0);
  ELL_2V_NAN_SET(seg->xy + 2);
  ELL_2V_NAN_SET(seg->xy + 4);
  ELL_2V_NAN_SET(seg->xy + 6);
  seg->corner[0] = seg->corner[1] = AIR_FALSE;
  seg->pNum = 0;
  return;
}

limnCBFPath *
limnCBFPathNew() {
  limnCBFPath *path;
  path = AIR_MALLOC(1, limnCBFPath);
  if (path) {
    path->segArr = airArrayNew((void**)(&path->seg), &path->segNum,
                               sizeof(limnCBFSeg), 128 /* incr */);
    airArrayStructCB(path->segArr, segInit, NULL);
    path->closed = AIR_FALSE;
  }
  return path;
}

limnCBFPath *
limnCBFPathNix(limnCBFPath *path) {
  if (path) {
    airArrayNuke(path->segArr);
    free(path);
  }
  return NULL;
}

void
limnCBFPathJoin(limnCBFPath *dst, const limnCBFPath *src) {
  uint bb = airArrayLenIncr(dst->segArr, src->segNum);
  memcpy(dst->seg + bb, src->seg, (src->segNum)*sizeof(limnCBFSeg));
  return;
}

/*
******** limnCBFMulti
**
** Fits one or more geometrically continuous splines to a set of points.  Does
** not look for new internal "corners" (points where the incoming and outgoing
** tangents are different), but does recursively subdivide the points into
** left and right sides around points with the highest error from
** limnCBFSingle.
*/
int
limnCBFMulti(limnCBFPath *path, limnCBFInfo *cbfi,
             const double _vv0[2], const double _tt1[2],
             const double _tt2[2], const double _vv3[2],
             const double *xy, uint pNum) {
  const char me[]="limnCBFMulti";
  double vv0[2], tt1[2], tt2[2], vv3[2], alpha[2];
  int geomGiven;
  uint loi, hii;

  /* need non-NULL cbfi in order to know cbfi->distMin */
  if (!(cbfi && xy)) {
    biffAddf(LIMN, "%s: got NULL pointer", me);
    return 1;
  }
  if (!(cbfi->distMin > 0)) {
    biffAddf(LIMN, "%s: need positive distMin (not %g)", me, cbfi->distMin);
    return 1;
  }
  if (!(pNum >= 2)) {
    biffAddf(LIMN, "%s: need 2 or more points (not %u)", me, pNum);
    return 1;
  }
  /* either all the _vv0, _tt1, _tt2, _vv3 can be NULL, or none */
  if (!( _vv0 && _tt1 && _tt2 && _vv3 )) {
    double len;
    if ( _vv0 || _tt1 || _tt2 || _vv3 ) {
      biffAddf(LIMN, "%s: either all or none of vv0, tt1, tt2, vv3 should be "
               "NULL", me);
      return 1;
    }
    geomGiven = 0;
    ELL_2V_COPY(vv0, xy);
    /* TODO: permit some smoothing as part of tangent estimation,
       but make sure to not ask for more points than are really there
       (could be as few as 2 points) */
    ELL_2V_SUB(tt1, xy + 2, xy); ELL_2V_NORM(tt1, tt1, len);
    ELL_2V_SUB(tt2, xy + 2*(pNum-1), xy + 2*(pNum-2)); ELL_2V_NORM(tt2, tt2, len);
    ELL_2V_COPY(vv3, xy + 2*(pNum-1));
  } else {
    /* copy the given endpoint geometry */
    geomGiven = 1;
    ELL_2V_COPY(vv0, _vv0);  ELL_2V_COPY(tt1, _tt1);
    ELL_2V_COPY(tt2, _tt2);  ELL_2V_COPY(vv3, _vv3);
  }
  loi = cbfi->baseIdx;
  hii = cbfi->baseIdx+pNum-1;
  if (cbfi->verbose) {
    printf("%s[%u,%u]: hello; %s v0=(%g,%g), t1=(%g,%g), t2=(%g,%g), "
           "v3=(%g,%g)\n", me, loi, hii,
           geomGiven ? "given" : "computed",
           vv0[0], vv0[1], tt1[0], tt1[1], tt2[0], tt2[1], vv3[0], vv3[1]);
  }

  /* TODO: figure out how outer-most decomposition of loop at corner
     points ends up in limnCBFSeg->corner, or what actually ends up
     setting limnCBFSeg->corner, ever */

  /* first try fitting a single spline */
  if (cbfi->verbose) {
    printf("%s[%u,%u]: trying single fit on all points\n", me, loi, hii);
  }
  if (limnCBFSingle(alpha, cbfi, vv0, tt1, tt2, vv3, xy, pNum)) {
    biffAddf(LIMN, "%s[%u,%u]: trouble on initial fit", me, loi, hii);
    return 1;
  }
  if (cbfi->distBig <= 1) {
    /* max dist was <= cbfi->distMin: single fit was good enough */
    if (cbfi->verbose) {
      printf("%s[%u,%u]: single fit good: nrpi=%u; dist=%g@%u <= %g; "
             "det=%g; alpha=%g,%g\n", me, loi, hii, cbfi->nrpIterDone,
             cbfi->dist, cbfi->distIdx, cbfi->distMin,
             cbfi->alphaDet, alpha[0], alpha[1]);
    }
    airArrayLenSet(path->segArr, 1);
    ELL_2V_COPY(path->seg[0].xy + 0, vv0);
    ELL_2V_SCALE_ADD2(path->seg[0].xy + 2, 1, vv0, alpha[0], tt1);
    ELL_2V_SCALE_ADD2(path->seg[0].xy + 4, 1, vv3, alpha[1], tt2);
    ELL_2V_COPY(path->seg[0].xy + 6, vv3);
    path->seg[0].pNum = pNum;
  } else { /* need to subdivide at cbfi->distIdx and recurse */
    uint mi = cbfi->distIdx;
    double ttL[2], mid[2], ttR[2], len;
    limnCBFPath *prth = limnCBFPathNew();
    limnCBFInfo cbfiL, cbfiR;
    memcpy(&cbfiL, cbfi, sizeof(limnCBFInfo));
    memcpy(&cbfiR, cbfi, sizeof(limnCBFInfo));
    if (cbfi->verbose) {
      printf("%s[%u,%u]: dist %g big (%d) --> split at %u (%u) xy=(%g,%g)\n",
             me, loi, hii, cbfi->dist, cbfi->distBig,
             mi, cbfi->baseIdx+mi, (xy + 2*mi)[0], (xy + 2*mi)[1]);
    }
    /* TODO: permit some smoothing as part of tangent estimation,
       but make sure to not ask for more points than are really
       there (can be as few as 3) */
    ELL_2V_COPY(mid, xy + 2*mi);
    ELL_2V_SUB(ttR, xy + 2*(mi+1), xy + 2*(mi-1));
    ELL_2V_NORM(ttR, ttR, len);
    ELL_2V_SCALE(ttL, -1, ttR);
    /* cbfiL.baseIdx == cbfi.baseIdx */
    cbfiR.baseIdx = cbfi->baseIdx + mi;
    /* recurse! */
    if (limnCBFMulti(path, &cbfiL, vv0, tt1, ttL, mid,
                     xy, mi+1) ||
        limnCBFMulti(prth, &cbfiR, mid, ttR, tt2, vv3,
                     xy + 2*mi, pNum - mi)) {
      biffAddf(LIMN, "%s[%u,%u]: trouble on recursive fit", me, loi, hii);
      limnCBFPathNix(prth); return 1;
    }
    limnCBFPathJoin(path, prth);
    limnCBFPathNix(prth);
    cbfi->nrpIterDone = cbfiL.nrpIterDone + cbfiR.nrpIterDone;
    if (cbfiL.dist > cbfiR.dist) {
      cbfi->dist    = cbfiL.dist;
      cbfi->distIdx = cbfiL.distIdx;
      cbfi->distBig = cbfiL.distBig;
    } else {
      cbfi->dist    = cbfiR.dist;
      cbfi->distIdx = cbfiR.distIdx;
      cbfi->distBig = cbfiR.distBig;
    }
    cbfi->nrpDeltaDone = AIR_MAX(cbfiL.nrpDeltaDone, cbfiR.nrpDeltaDone);
    cbfi->alphaDet = AIR_MIN(cbfiL.alphaDet, cbfiR.alphaDet);
  }
  return 0;
}

/*
TODO:
limnCBFCorners to find corners in data (with flag to indicate that xy is a loop)

tangent estimation at start and end that handles xy being loop

limnCBFLoop to handle a whole loop (typical use-case)

limnCBFPrune to remove (in-place) coincident and nearly coincident points in xy

use performance tests to explore optimal settings in cbfi:
  nrpIterMax, nrpDeltaScl, nrpDistScl, nrpPsi, nrpDeltaMin

valgrind everything
*/
