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

/*
  This file contains GLK's implementation of the curve fitting described in:
  Philip J. Schneider. “An Algorithm for Automatically Fitting Digitized
  Curves”. In Graphics Gems, Academic Press, 1990, pp. 612–626.
  https://dl.acm.org/doi/10.5555/90767.90941
  The author's code is here:
  http://www.realtimerendering.com/resources/GraphicsGems/gems/FitCurves.c
  The main thing there that is not attempted here is the "Wu/Barsky heuristic"
  (never actually named as such in the paper) for dealing with the solver
  producing negative alpha.

  The functions below do not actually use any of the existing limnSpline
  structs or functions; those were written a long time ago, and represent
  GLK's beginning to learn about splines, rather than any well-thought-out
  implementation of essential functionality.  Hopefully this will be revisited
  and re-organized in a later version of Teem, at which point the code below
  can be integrated with the rest of limn, but this too will benefit from
  ongoing scrutiny and re-writing; there is always more to learn about
  splines.
*/

/* degree 3 Bernstein polynomials, for *C*ubic *B*ezier curves,
   and their derivatives (not using any nice recursion properties) */
#define CB0(t) ((1-(t))*(1-(t))*(1-(t)))
#define CB1(t) (3*(t)*(1-(t))*(1-(t)))
#define CB2(t) (3*(t)*(t)*(1-(t)))
#define CB3(t) ((t)*(t)*(t))
#define VCB(a,t) ((a)[0] = CB0(t),                 \
                  (a)[1] = CB1(t),                 \
                  (a)[2] = CB2(t),                 \
                  (a)[3] = CB3(t))
#define CB0D(t) (-3*(1-(t))*(1-(t)))
#define CB1D(t) (3*((t)-1)*(3*(t)-1))
#define CB2D(t) (3*(t)*(2-3*(t)))
#define CB3D(t) (3*(t)*(t))
#define VCBD(a,t) ((a)[0] = CB0D(t),                 \
                   (a)[1] = CB1D(t),                 \
                   (a)[2] = CB2D(t),                 \
                   (a)[3] = CB3D(t))
#define CB0DD(t) (6*(1-(t)))
#define CB1DD(t) (6*(3*(t)-2))
#define CB2DD(t) (6*(1-3*(t)))
#define CB3DD(t) (6*(t))
#define VCBDD(a,t) ((a)[0] = CB0DD(t),                \
                    (a)[1] = CB1DD(t),                \
                    (a)[2] = CB2DD(t),                \
                    (a)[3] = CB3DD(t))

/*
******** limnCBFPathSample
**
** evaluates limnCBFPath at pNum locations (uniformly distributed among the
** segments in the path), and saves into (pre-allocated) xy
*/
void
limnCBFSegEval(double *xy, const limnCBFSeg *seg, double tt) {
  double ww[4];
  VCB(ww, tt);
  ELL_2V_SCALE_ADD4(xy,
                    ww[0], seg->xy + 0,
                    ww[1], seg->xy + 2,
                    ww[2], seg->xy + 4,
                    ww[3], seg->xy + 6);
  return;
}

/*
******** limnCBFPathSample
**
** evaluates limnCBFPath at pNum locations (uniformly distributed among the
** segments in the path), and saves into (pre-allocated) xy
*/
void
limnCBFPathSample(double *xy, uint pNum, const limnCBFPath *path) {
  uint ii, sNum = path->segNum;
  for (ii=0; ii<pNum; ii++) {
    uint segi = airIndex(0, ii, pNum-1, sNum);
    double tmpf = AIR_AFFINE(0, ii, pNum-1, 0, sNum);
    double tt = segi - tmpf;
    const limnCBFSeg *seg = path->seg + segi;
    printf("!%s: %u -> %u (%g) %g\n", "limnCBFPathSample", ii, segi, tmpf, tt);
    limnCBFSegEval(xy + 2*ii, seg, tt);
  }
  return;
}

/*
** (from paper page 620) solves for the alpha that minimize squared error
** between xy[i] and Q(uu[i]) where Q(t) is cubic Bezier spline through vv0,
** vv0 + alpha[0]*tt1, vv3 + alpha[1]*tt2, and vv3.
*/
static double
findalpha(double alpha[2],
          const double vv0[2], const double tt1[2],
          const double tt2[2], const double vv3[2],
          const double *xy, const double *uu, uint pNum) {
  /* const char me[]="findalpha"; */
  uint ii;
  double det, xx[2], MM[4], MI[4], m11, m12, m22;

  xx[0] = xx[1] = m11 = m12 = m22 = 0;
  for (ii=0; ii<pNum; ii++) {
    double bb[4], Ai1[2], Ai2[2], Pi[2], dmP[2];
    double ui = uu[ii];
    VCB(bb, ui);
    ELL_2V_SCALE(Ai1, bb[1], tt1);
    ELL_2V_SCALE(Ai2, bb[2], tt2);
    m11 += ELL_2V_DOT(Ai1, Ai1); /* using m not C */
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
  return det;
}

/*
** using Newton iterations to try to find a better places at which
** to evaluate the spline in order to match the given points xy
*/
static double
reparm(double *uuOut,
       const double alpha[2],
       const double vv0[2], const double tt1[2],
       const double tt2[2], const double vv3[2],
       const double *xy, const double *uuIn, uint pNum,
       int verbose) {
  const char me[]="reparm";
  uint ii;
  double vv1[2], vv2[2], Q[2], QD[2], QDD[2], delta, maxdelu;

  /* max change in parameterization is related to average u[i+1]-u[i] */
  maxdelu = 2.0/(pNum-1);
  uuOut[0] = uuIn[0];
  uuOut[pNum-1] = uuIn[pNum-1];
  ELL_2V_SCALE_ADD2(vv1, 1, vv0, alpha[0], tt1);
  ELL_2V_SCALE_ADD2(vv2, 1, vv3, alpha[1], tt2);
  delta = 0;
  for (ii=1; ii<pNum-1; ii++) {
    double numer, denom, delu, df[2], ww[4], tt;
    tt = uuIn[ii];
    VCB(ww, tt);
    ELL_2V_SCALE_ADD4(Q, ww[0], vv0, ww[1], vv1, ww[2], vv2, ww[3], vv3);
    VCBD(ww, tt);
    ELL_2V_SCALE_ADD4(QD, ww[0], vv0, ww[1], vv1, ww[2], vv2, ww[3], vv3);
    VCBDD(ww, tt);
    ELL_2V_SCALE_ADD4(QDD, ww[0], vv0, ww[1], vv1, ww[2], vv2, ww[3], vv3);
    ELL_2V_SUB(df, Q, xy + 2*ii);
    numer = ELL_2V_DOT(df, QD);
    denom = ELL_2V_DOT(QD, QD) + ELL_2V_DOT(df, QDD);
    delu = -numer/denom;
    if (AIR_ABS(delu) > maxdelu) {
      /* not in paper but helps stabilize things: capping Newton step */
      delu = maxdelu*airSgn(delu);
    }
    uuOut[ii] = tt + delu;
    delta += AIR_ABS(delu);
    if (verbose > 1) {
      printf("%s[%2u]: tt %g <-- %g - %g\n", me, ii,
             uuOut[ii], tt, delu);
    }
  }
  delta /= pNum-2;
  /* HEY TODO: need to make sure that half-way between point,
     spline isn't wildly diverging; this can happen with the
     spline making a loop away from a small number of points, e.g.:
     4 points spline defined by vv0 = (1,1), tt1 = (1,2),
     tt2 = (1,2), vv3 = (0,1) */
  return delta;
}

static double finddist(uint *distIdx,
                       const double alpha[2],
                       const double vv0[2], const double tt1[2],
                       const double tt2[2], const double vv3[2],
                       const double *xy, const double *uu,
                       uint pNum) {
  uint ii;
  double len, df[2], ww[4], vv1[2], vv2[2], Q[2], dist;
  /* yes, some copy-and-paste from above */
  ELL_2V_SCALE_ADD2(vv1, 1, vv0, alpha[0], tt1);
  ELL_2V_SCALE_ADD2(vv2, 1, vv3, alpha[1], tt2);
  dist = 0;
  for (ii=0; ii<pNum; ii++) {
    VCB(ww, uu[ii]);
    ELL_2V_SCALE_ADD4(Q, ww[0], vv0, ww[1], vv1, ww[2], vv2, ww[3], vv3);
    ELL_2V_SUB(df, Q, xy + 2*ii);
    len = ELL_2V_LEN(df);
    if (len > dist) {
      dist = len;
      *distIdx = ii;
    }
  }
  return dist;
}

void
limnCBFInfoInit(limnCBFInfo *cbfi, int outputOnly) {
  if (!cbfi) return;
  if (!outputOnly) {
    /* inputs */
    cbfi->verbose = 0;
    cbfi->nrpIterMax = 10;
    cbfi->distMin = 0;
    cbfi->nrpDeltaMin = 0.0002;
    cbfi->nrpDetMin = 0.001;
  }
  /* outputs */
  cbfi->nrpIterDone = (uint)(-1);
  cbfi->distIdx = (uint)(-1);
  cbfi->distDone = AIR_POS_INF;
  cbfi->nrpDeltaDone = AIR_POS_INF;
  cbfi->nrpDetDone = 0;
  cbfi->timeMs = AIR_POS_INF;
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
** alpha is solved for multiples times, after taking Newton steps to try to
** optimize the parameterization of the points (in an array that is not passed
** in but instead internal to this function); limn.h calls this process
** "nrp". nrp iterations are stopped after any one of following is true
** (the original published method did not have these fine-grained controls):
**  - if cbfi->nrpIterMax > 0: have done nrpIterMax iterations of nrp
**  - if cbfi->distMin > 0: distance from spline (as evaluated at the
**    current parameterization) to the given points falls below distMin
**  - if cbfi->nrpDeltaMin > 0: parameterization change falls below deltaMin
** At least one of these thresholds has to be non-zero and positive.
** Information about how things went are set in the given (non-NULL)
** _cbfi.
*/
int
limnCBFSingle(double alpha[2], limnCBFInfo *_cbfi,
              const double vv0[2], const double tt1[2],
              const double tt2[2], const double vv3[2],
              const double *xy, uint pNum) {
  const char me[]="limnCBFSingle";
  /* the array of spline parameters are bounced between uu[0] and uu[1] */
  double delta, len, *uu[2];
  airArray *mop;
  uint ii, iter, distI,
    iterLimit = 100; /* sanity check on max number of iterations */
  double time0, dist, det;
  limnCBFInfo *cbfi, mycbfi;

  time0 = airTime();
  if (_cbfi) {
    /* caller has supplied state */
    cbfi = _cbfi;
    limnCBFInfoInit(cbfi, AIR_TRUE /* outputOnly */);
  } else {
    /* caller wants default parms */
    cbfi = &mycbfi;
    limnCBFInfoInit(cbfi, AIR_FALSE /* outputOnly */);
  }
  if (!(alpha && vv0 && tt1 && tt2 && vv3 && xy)) {
    biffAddf(LIMN, "%s: got NULL pointer", me);
    return 1;
  }
  if (!(pNum >= 3)) {
    biffAddf(LIMN, "%s: need 3 or more points (not %u)", me, pNum);
    return 1;
  }
  if (!( cbfi->nrpIterMax > 0 || cbfi->nrpDeltaMin > 0
         || cbfi->distMin > 0 )) {
    biffAddf(LIMN, "%s: need positive nrpIterMax, nrpDeltaMin, or distMin",
             me);
    return 1;
  }
  if (cbfi->nrpDeltaMin < 0 || cbfi->distMin < 0) {
    biffAddf(LIMN, "%s: cannot have negative nrpDeltaMin (%g) or "
             "distMin (%g)", me, cbfi->nrpDeltaMin, cbfi->distMin);
    return 1;
  }
  if (cbfi->verbose) {
    printf("%s: hello, vv0=(%g,%g), tt1=(%g,%g), tt2=(%g,%g), vv3=(%g,%g)\n",
           me, vv0[0], vv0[1], tt1[0], tt1[1], tt2[0], tt2[1], vv3[0], vv3[1]);
  }

  mop = airMopNew();
  uu[0] = AIR_CALLOC(pNum*2, double);
  uu[1] = AIR_CALLOC(pNum*2, double);
  airMopAdd(mop, uu[0], airFree, airMopAlways);
  airMopAdd(mop, uu[1], airFree, airMopAlways);
  if (!(uu[0] && uu[1])) {
    biffAddf(LIMN, "%s: failed to allocate parameter buffers", me);
    airMopError(mop); return 1;
  }

  /* initialize progress indicators */
  iter = 0;
  delta = 0;
  /* iter=0 -> iter%2=0 -> UU0=uu[0]; UU1=uu[1] */
#define UU0 (uu[iter % 2])
#define UU1 (uu[(iter+1) % 2])

  /* initialize parameter to chord length */
  UU0[0] = len = 0;
  for (ii=1; ii<pNum; ii++) {
    double dd[2];
    ELL_2V_SUB(dd, xy + 2*ii, xy + 2*(ii-1));
    len += ELL_2V_LEN(dd);
    UU0[ii] = len;
  }
  for (ii=0; ii<pNum; ii++) {
    UU0[ii] /= len;
    if (cbfi->verbose > 1) {
      printf("%s: iter %u uu[%u] = %g\n", me, iter, ii, UU0[ii]);
    }
    delta += AIR_ABS(UU0[ii]);
  }
  delta /= pNum-2;
  if (cbfi->verbose) {
    printf("%s: iter %u (chord length) delta = %g\n", me, iter, delta);
  }

  /* iterate */
  while (1) {
    det = findalpha(alpha, vv0, tt1, tt2, vv3, xy, UU0, pNum);
    if (cbfi->verbose) {
      printf("%s: iter %u found alpha %g %g (det %g)\n", me, iter,
             alpha[0], alpha[1], det);
    }
    /* determinant should really be scaled so that this test is
       invariant w.r.t. rescaling of all points */
    if (!( AIR_ABS(det) > cbfi->nrpDetMin && AIR_EXISTS(det) )) {
      biffAddf(LIMN, "%s: got det %g (vs %g) on iter %u, bailing", me,
               det, cbfi->nrpDetMin, iter);
      airMopError(mop); return 1;
    }
    if (!iter) {
      /* test dist 1st time through; may bail at iter == nrpIterMax == 1 */
      dist = finddist(&distI, alpha, vv0, tt1, tt2, vv3, xy, UU0, pNum);
      if (cbfi->distMin && dist <= cbfi->distMin) {
        if (cbfi->verbose) {
          printf("%s: iter 0 dist %g <= min %g --> break\n", me,
                 dist, cbfi->distMin);
        }
        break;
      }
    }
    iter++; /* NOTE: this swaps UU0 and UU1 */
    if (cbfi->nrpIterMax && iter >= cbfi->nrpIterMax) {
      if (cbfi->verbose) {
        printf("%s: iter %u >= max %u --> break\n", me,
               iter, cbfi->nrpIterMax);
      }
      break;
    }
    if (iter >= iterLimit) {
      biffAddf(LIMN, "%s: ran for unreasonable # iters (%u); stopping",
               me, iterLimit);
      airMopError(mop); return 1;
    }
    delta = reparm(UU0, alpha, vv0, tt1, tt2, vv3, xy, UU1, pNum,
                   cbfi->verbose);
    if (cbfi->verbose) {
      printf("%s: iter %u (reparm) delta = %g\n", me, iter, delta);
    }
    if (cbfi->nrpDeltaMin && delta <= cbfi->nrpDeltaMin) {
      if (cbfi->verbose) {
        printf("%s: iter %u delta %g <= min %g --> break\n", me, iter,
               delta, cbfi->nrpDeltaMin);
      }
      break;
    }
    dist = finddist(&distI, alpha, vv0, tt1, tt2, vv3, xy, UU0, pNum);
    if (cbfi->distMin && dist <= cbfi->distMin) {
      if (cbfi->verbose) {
        printf("%s: iter %u dist %g <= min %g --> break\n", me, iter,
               dist, cbfi->distMin);
      }
      break;
    }
  }
#undef UU0
#undef UU1

  cbfi->nrpIterDone = iter;
  cbfi->nrpDeltaDone = delta;
  cbfi->distDone = dist;
  cbfi->distIdx = distI;
  cbfi->nrpDetDone = det;
  airMopOkay(mop);
  cbfi->timeMs = (airTime() - time0)*1000;
  return 0;
}

static void
segInit(void *_seg) {
  limnCBFSeg *seg = (limnCBFSeg *)_seg;
  seg->corner[0] = seg->corner[1] = AIR_FALSE;
  ELL_2V_NAN_SET(seg->xy + 0);
  ELL_2V_NAN_SET(seg->xy + 2);
  ELL_2V_NAN_SET(seg->xy + 4);
  ELL_2V_NAN_SET(seg->xy + 6);
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
  unsigned int bb;
  bb = airArrayLenIncr(dst->segArr, src->segNum);
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
  double time0, vv0[2], tt1[2], tt2[2], vv3[2], alpha[2];

  time0 = airTime();
  /* need non-NULL cbfi in order to know cbfi->distMin */
  if (!(cbfi && xy)) {
    biffAddf(LIMN, "%s: got NULL pointer", me);
    return 1;
  }
  if (!(cbfi->distMin > 0)) {
    biffAddf(LIMN, "%s: need positive distMin (not %g)", me, cbfi->distMin);
    return 1;
  }
  if (!(pNum >= 3)) {
    biffAddf(LIMN, "%s: need 3 or more points (not %u)", me, pNum);
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
    ELL_2V_COPY(vv0, xy);
    /* TODO: permit some smoothing as part of tangent estimation,
       but make sure to not ask for more points than are really there */
    ELL_2V_SUB(tt1, xy + 2, xy); ELL_2V_NORM(tt1, tt1, len);
    ELL_2V_SUB(tt2, xy + 2*(pNum-1), xy + 2*(pNum-2)); ELL_2V_NORM(tt2, tt2, len);
    ELL_2V_COPY(vv3, xy + 2*(pNum-1));
  } else {
    /* copy the given endpoint geometry */
    ELL_2V_COPY(vv0, _vv0);
    ELL_2V_COPY(tt1, _tt1);
    ELL_2V_COPY(tt2, _tt2);
    ELL_2V_COPY(vv3, _vv3);
  }

  /* TODO: figure out how outer-most decomposition of loop at corner
     points ends up in limnCBFSeg->corner */

  /* first try fitting a single spline */
  if (limnCBFSingle(alpha, cbfi, vv0, tt1, tt2, vv3, xy, pNum)) {
    biffAddf(LIMN, "%s: trouble on initial fit", me);
    return 1;
  }
  if (cbfi->distDone < cbfi->distMin) {
    /* single fit was good enough */
    printf("!%s: done with single fit (%g < %g); alpha = %g,%g\n",
           me, cbfi->distDone, cbfi->distMin, alpha[0], alpha[1]);
    airArrayLenSet(path->segArr, 1);
    ELL_2V_COPY(path->seg[0].xy + 0, vv0);
    ELL_2V_SCALE_ADD2(path->seg[0].xy + 1, 1, vv0, alpha[0], tt1);
    ELL_2V_SCALE_ADD2(path->seg[0].xy + 2, 1, vv3, alpha[1], tt2);
    ELL_2V_COPY(path->seg[0].xy + 3, vv3);
  } else {
    /* need to subdivide at cbfi->distIdx and recurse */
    uint mi;
    double ttL[2], mid[2], ttR[2], len;
    limnCBFInfo cbfiL, cbfiR;
    limnCBFPath *prth;
    memcpy(&cbfiL, cbfi, sizeof(limnCBFInfo));
    memcpy(&cbfiR, cbfi, sizeof(limnCBFInfo));
    mi = cbfi->distIdx;
    /* TODO: figure out what happens if splitting point is so close to
       segment endpoint that only 1 or 2 points sit on that side */
    printf("!%s: (%g < %g) --> splitting at %u\n",
           me, cbfi->distDone, cbfi->distMin, mi);
    /* TODO: permit some smoothing as part of tangent estimation,
       but make sure to not ask for more points than are really there */
    ELL_2V_COPY(mid, xy + 2*mi);
    ELL_2V_SUB(ttR, xy + 2*(mi+1), xy + 2*(mi-1)); ELL_2V_NORM(ttR, ttR, len);
    ELL_2V_SCALE(ttL, -1, ttR);
    prth = limnCBFPathNew();
    if (limnCBFMulti(path, &cbfiL, vv0, tt1, ttL, mid, xy, mi) ||
        limnCBFMulti(prth, &cbfiR, mid, ttR, tt2, vv3, xy + 2*mi, pNum - mi)) {
      biffAddf(LIMN, "%s: trouble on recursive fit", me);
      limnCBFPathNix(prth); return 1;
    }
    limnCBFPathJoin(path, prth);
    limnCBFPathNix(prth);
    cbfi->nrpIterDone = cbfiL.nrpIterDone + cbfiR.nrpIterDone;
    cbfi->distDone = AIR_MAX(cbfiL.distDone, cbfiR.distDone);
    cbfi->nrpDeltaDone = AIR_MAX(cbfiL.nrpDeltaDone, cbfiR.nrpDeltaDone);
    cbfi->nrpDetDone = AIR_MIN(cbfiL.nrpDetDone, cbfiR.nrpDetDone);
    /* actually ignoring cbfiL.timeMS, cbfiR.timeMS for now */
  }
  cbfi->timeMs = (airTime() - time0)*1000;
  return 0;
}
