/*
  Teem: Tools to process and visualize scientific data and images
  Copyright (C) 2009--2022  University of Chicago
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

limnPoints * /* Biff: nope */
limnPointsNew(const double *pp, uint nn, int isLoop) {
  limnPoints *lpnt;
  lpnt = AIR_CALLOC(1, limnPoints);
  assert(lpnt);
  if (pp) {
    /* we are wrapping around a given pre-allocated buffer */
    lpnt->pp = pp;
    lpnt->ppOwn = NULL;
  } else {
    /* we are allocating our own buffer */
    lpnt->pp = NULL;
    lpnt->ppOwn = AIR_CALLOC(nn, double);
    assert(lpnt->pp);
  }
  lpnt->num = nn;
  lpnt->isLoop = isLoop;
  return lpnt;
}

limnPoints * /* Biff: nope */
limnPointsNix(limnPoints *lpnt) {
  if (lpnt) {
    /* don't touch lpnt->pp */
    if (lpnt->ppOwn) free(lpnt->ppOwn);
    free(lpnt);
  }
  return NULL;
}

static int /* Biff: 1 */
pointsCheck(const limnPoints *lpnt) {
  static const char me[] = "pointsCheck";
  uint pnmin;
  int have;

  if (!lpnt) {
    biffAddf(LIMN, "%s: got NULL pointer", me);
    return 1;
  }
  pnmin = lpnt->isLoop ? 3 : 2;
  if (!(lpnt->num >= pnmin)) {
    biffAddf(LIMN, "%s: need %u or more points in limnPoints (not %u)%s", me, pnmin,
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

#define PP(lpnt) ((lpnt)->pp ? (lpnt)->pp : (lpnt)->ppOwn)

/* number of points between low,high indices loi,hii */
static uint
pntNum(const limnPoints *lpnt, uint loi, uint hii) {
  if (hii < loi) {
    assert(lpnt->isLoop);
    hii += lpnt->num;
  }
  return hii - loi + 1;
}

/* coordinates of point with index loi+ii */
static const double *
pntCrd(const limnPoints *lpnt, uint loi, uint ii) {
  uint jj = loi + ii;
  while (jj >= lpnt->num)
    jj -= lpnt->num;
  return PP(lpnt) + 2 * jj;
}

/* CB0, CB1, CB2, CB3 = degree 3 Bernstein polynomials, for *C*ubic
   *B*ezier curves, and their derivatives D0, D1, D2 (not using any
   nice recursion properties for evaluation, oh well) */
#define CB0D0(T) ((1 - (T)) * (1 - (T)) * (1 - (T)))
#define CB1D0(T) (3 * (T) * (1 - (T)) * (1 - (T)))
#define CB2D0(T) (3 * (T) * (T) * (1 - (T)))
#define CB3D0(T) ((T) * (T) * (T))

#define CB0D1(T) (-3 * (1 - (T)) * (1 - (T)))
#define CB1D1(T) (3 * ((T)-1) * (3 * (T)-1))
#define CB2D1(T) (3 * (T) * (2 - 3 * (T)))
#define CB3D1(T) (3 * (T) * (T))

#define CB0D2(T) (6 * (1 - (T)))
#define CB1D2(T) (6 * (3 * (T)-2))
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
  CBD0(vv, xy + 0, xy + 2, xy + 4, xy + 6, tt, ww);
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
    limnCBFSegEval(xy + 2 * ii, seg, tt);
    /*
    fprintf(stderr, "!%s: %u -> %u (%g) %g -> (%g,%g)\n",
            "limnCBFPathSample", ii, segi, tmpf, tt,
            (xy + 2*ii)[0], (xy + 2*ii)[1]);
    */
  }
  return;
}

/*
** Find endpoint vertex vv and tangent tt (constraints for spline fitting)
** from the given points lpnt at coord index ii within index range [loi,hoi]
** (e.g. ii=1 means looking at lpnt coord index loi+1). The tangent direction
** dir controls which points are looked at:
** >0: considering only ii and higher-index vertices,
**  0: for tangent centered at ii, using lower- and higher-index vertices
** <0: considering only ii and lower-index vertices
** For >0 and 0: the tangent points towards the positions of higher-
** index vertices.  For <0, it points the other way.
** The only point indices accessed will be in [loi,hii]; this is what
** enforces the possible corner-ness of those indices (which prevents
** vertices past corners influencing how vv or tt are found)
*/
static void
findVT(double vv[2], double tt[2], const limnCBFContext *fctx, const limnPoints *lpnt,
       uint loi, uint hii, uint ii, int dir) {
  /* static const char me[] = "findVT"; */
  double len;
  uint pNum, /* total number of points in lpnts */
    sgsz;    /* segment size: number of points in [loi,hii] */

  dir = airSgn(dir);
  pNum = lpnt->num;
  if (lpnt->isLoop) {
    sgsz = (hii < loi ? pNum : 0) + hii - loi + 1;
  } else {
    sgsz = hii - loi + 1;
  }
  if (0 == fctx->scale) {
    uint mi, pi, iplus, imnus;
    const double *xy, *xyP, *xyM;
    if (lpnt->isLoop) {
      iplus = (loi + ii + 1) % pNum;
      imnus = (uint)AIR_MOD((int)(loi + ii) - 1, (int)pNum);
    } else {
      /* regardless of lpnt->isLoop, we only look in [loi,hii] */
      iplus = loi + AIR_MIN(ii + 1, sgsz - 1);
      imnus = loi + AIR_MAX(1, ii) - 1;
    }
    xy = pntCrd(lpnt, loi, ii);
    if (vv) ELL_2V_COPY(vv, xy);
    switch (dir) {
    case 1:
      pi = iplus;
      mi = ii;
      break;
    case 0:
      pi = iplus;
      mi = imnus;
      break;
    case -1:
      /* mi and pi switched to point other way */
      mi = ii;
      pi = imnus;
      break;
    }
    /* if (with !isLoop) ii=0 and dir=-1, or, ii=pNum-1 and dir=+1
       ==> mi=pi ==> tt will be (nan,nan), which is appropriate */
    xyP = pntCrd(lpnt, loi, pi);
    xyM = pntCrd(lpnt, loi, mi);
    ELL_2V_SUB(tt, xyP, xyM);
    ELL_2V_NORM(tt, tt, len);
  } else {
#if 0
    /* using scale>0 for endpoint and tangent estimation */
    const double *vw = fctx->vw;
    const double *tw = fctx->tw;
    /* various signed indices */
    int sii=(int)ii,       /* we compute around vertex ii */
      smax=(int)fctx->wLen - 1, /* bound of loop index */
      sj;                  /* loop through [-smax,smax] */
    if (vv) ELL_2V_SET(vv, 0, 0);
    ELL_2V_SET(tt, 0, 0);
    /* printf("!%s: ii = %u, dir=%d\n", me, ii, dir); */
    /* j indices are for the local looping */
    for (sj=-smax; sj<=smax; sj++) {
      uint xj, /* eventual index into data */
        asj = (uint)AIR_ABS(sj); /* index into vw, tw */
      int sgn=1,
        sxj = sii + sj; /* signed (tmp) j idx into data */
      double ttw;
      /* printf("!%s[sj=%d,asj=%u]: sxj0 = %d\n", me, sj, asj, sxj); */
      switch (dir) {
      case 1:
        sxj = AIR_MAX(sxj, sii);
        break;
      case -1:
        sgn=-1;
        sxj = AIR_MIN(sxj, sii);
        break;
      }
      /* sxj = sii+sj, but capped at sii according to dir */
      /* printf("!%s[sj=%d]: dir=%d -> sxj1 = %d\n", me, sj, dir, sxj); */
      if (lpnt->isLoop) {
        sxj = AIR_MOD(sxj, (int)pNum);
      } else {
        sxj = AIR_CLAMP(0, sxj, (int)pNum-1);
      }
      xj = (uint)sxj;
      /* printf("!%s[sj=%d]: isLoop=%d -> sxj2 = %d -> xj = %u\n", me, sj, lpnt->isLoop, sxj, xj); */
      if (vv) ELL_2V_SCALE_INCR(vv, vw[asj], xy + 2*xj);
      /* printf("!%s[sj=%d]: vv += %g*(%g,%g) -> (%g,%g)\n", me, sj, vw[asj], (xy + 2*xj)[0], (xy + 2*xj)[1], vv[0], vv[1]); */
      ttw = sgn*airSgn(sj)*tw[asj];
      /* printf("!%s[sj=%d]: %d * %d * %g = %g\n", me, sj, sgn, airSgn(sj), tw[asj], ttw); */
      ELL_2V_SCALE_INCR(tt, ttw, xy + 2*xj);
      /* printf("!%s[sj=%d]: tt += %g*(%g,%g) -> (%g,%g)\n", me, sj, ttw, (xy + 2*xj)[0], (xy + 2*xj)[1], tt[0], tt[1]); */
    }
    ELL_2V_NORM(tt, tt, len);
    /* fix the boundary conditions as a post-process */
    if (     0==ii && -1==dir) ELL_2V_SET(tt, AIR_NAN, AIR_NAN);
    if (pNum-1==ii && +1==dir) ELL_2V_SET(tt, AIR_NAN, AIR_NAN);
    if (vv) {
      /* some post-proceessing of computed spline endpoint */
      double off[2], pp[2], operp;
      ELL_2V_SET(pp, tt[1], -tt[0]); /* pp is perpendicular to tt */
      ELL_2V_SUB(off, vv, xy + 2*ii);
      operp = ELL_2V_DOT(off, pp);
      /* limit distance from chosen (x,y) datapoint to spline endpoint to be
         (HEY harcoded) 95% of fctx->distMin. Being allowed to be further away
         can cause annoyances */
      operp = AIR_MIN(0.95*fctx->distMin, operp);
      /* constrain difference between chosen (x,y) datapoint and spline
         endpoint to be perpendicular to estimated tangent */
      ELL_2V_SCALE_ADD2(vv, 1, xy + 2*ii, operp, pp);
    }
#endif
  }
  return;
}

static int /* Biff: 1 */
setVTTV(int *given, double vv0[2], double tt1[2], double tt2[2], double vv3[2],
        const double _vv0[2], const double _tt1[2], const double _tt2[2],
        const double _vv3[2], const limnCBFContext *fctx, const limnPoints *lpnt,
        uint loi, uint hii) {
  static const char me[] = "setVTTV";

  /* either all the _vv0, _tt1, _tt2, _vv3 can be NULL, or none */
  if (!(_vv0 && _tt1 && _tt2 && _vv3)) {
    if (_vv0 || _tt1 || _tt2 || _vv3) {
      biffAddf(LIMN,
               "%s: either all or none of vv0,tt1,tt2,vv3 "
               "should be NULL",
               me);
      return 1;
    }
    if (lpnt->isLoop) {
      findVT(vv0, tt1, fctx, lpnt, loi, hii, loi, 0);
      ELL_2V_COPY(vv3, vv0);
      ELL_2V_SCALE(tt2, -1, tt1);
    } else {
      findVT(vv0, tt1, fctx, lpnt, loi, hii, loi, +1);
      findVT(vv0, tt1, fctx, lpnt, loi, hii, hii, -1);
    }
    if (given) {
      *given = AIR_FALSE;
    }
  } else {
    /* copy the given endpoint geometry */
    ELL_2V_COPY(vv0, _vv0);
    ELL_2V_COPY(tt1, _tt1);
    ELL_2V_COPY(tt2, _tt2);
    ELL_2V_COPY(vv3, _vv3);
    if (given) {
      *given = AIR_TRUE;
    }
  }
  return 0;
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
findalpha(double alpha[2], limnCBFContext *fctx, /* must be non-NULL */
          const double vv0[2], const double tt1[2], const double tt2[2],
          const double vv3[2], const limnPoints *lpnt, uint loi, uint hii) {
  static const char me[] = "findalpha";
  uint ii, pNum;
  double det;

  pNum = pntNum(lpnt, loi, hii);
  if (pNum > 2) {
    double xx[2], m11, m12, m22, MM[4], MI[4];
    const double *uu = fctx->uu;
    xx[0] = xx[1] = m11 = m12 = m22 = 0;
    for (ii = 0; ii < pNum; ii++) {
      const double *xy;
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
      ELL_2V_SCALE_ADD2(Pi, bb[0] + bb[1], vv0, bb[2] + bb[3], vv3);
      xy = pntCrd(lpnt, loi, ii);
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
        && alpha[0] > (fctx->lenF2L) * (fctx->alphaMin)
        && alpha[1] > (fctx->lenF2L) * (fctx->alphaMin))) {
    if (fctx->verbose) {
      printf("%s: bad |det| %g (vs %g) or alpha %g,%g (vs %g*%g) "
             "--> simple arc\n",
             me, AIR_ABS(det), fctx->detMin, alpha[0], alpha[1], fctx->lenF2L,
             fctx->alphaMin);
    }
    /* generate simple arc: set both alphas to 1/3 of distance from
       first to last point, but also handle non-unit-length tt1 and
       tt2 */
    alpha[0] = fctx->lenF2L / (3 * ELL_2V_LEN(tt1));
    alpha[1] = fctx->lenF2L / (3 * ELL_2V_LEN(tt2));
  } else {
    if (fctx->verbose > 1) {
      printf("%s: all good: det %g, alpha %g,%g\n", me, det, alpha[0], alpha[1]);
    }
  }
  fctx->alphaDet = det;
  return;
}

/*
** using Newton iterations to try to find a better places at which
** to evaluate the spline in order to match the given points xy
*/
static double
reparm(const limnCBFContext *fctx, /* must be non-NULL */
       const double alpha[2], const double vv0[2], const double tt1[2],
       const double tt2[2], const double vv3[2], const limnPoints *lpnt, uint loi,
       uint hii) {
  static const char me[] = "reparm";
  uint ii, pNum;
  double vv1[2], vv2[2], delta, maxdelu;
  double *uu = fctx->uu;

  pNum = pntNum(lpnt, loi, hii);
  assert(pNum >= 3);
  /* average u[i+1]-u[i] is 1/(pNum-1) */
  maxdelu = fctx->nrpDeltaMax / (pNum - 1);
  ELL_2V_SCALE_ADD2(vv1, 1, vv0, alpha[0], tt1);
  ELL_2V_SCALE_ADD2(vv2, 1, vv3, alpha[1], tt2);
  delta = 0;
  /* only changing parameterization of interior points,
     not the first (ii=0) or last (ii=pNum-1) */
  for (ii = 1; ii < pNum - 1; ii++) {
    double numer, denom, delu, df[2], ww[4], tt, Q[2], QD[2], QDD[2];
    const double *xy;
    tt = uu[ii];
    CBD0(Q, vv0, vv1, vv2, vv3, tt, ww);
    CBD1(QD, vv0, vv1, vv2, vv3, tt, ww);
    CBD2(QDD, vv0, vv1, vv2, vv3, tt, ww);
    xy = pntCrd(lpnt, loi, ii);
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
  delta /= pNum - 2;
  /* HEY TODO: need to make sure that half-way between points,
     spline isn't wildly diverging; this can happen with the
     spline making a loop away from a small number of points, e.g.:
     4 points spline defined by vv0 = (1,1), tt1 = (1,2),
     tt2 = (1,2), vv3 = (0,1) */
  return delta;
}

/* sets fctx->dist to max distance to spline, at point fctx->distIdx,
   and then sets fctx->distBig accordingly */
static void
finddist(limnCBFContext *fctx, const double alpha[2], const double vv0[2],
         const double tt1[2], const double tt2[2], const double vv3[2],
         const limnPoints *lpnt, uint loi, uint hii) {
  uint ii, distI, pNum;
  double vv1[2], vv2[2], dist;
  const double *uu = fctx->uu;

  pNum = pntNum(lpnt, loi, hii);
  assert(pNum >= 3);
  ELL_2V_SCALE_ADD2(vv1, 1, vv0, alpha[0], tt1);
  ELL_2V_SCALE_ADD2(vv2, 1, vv3, alpha[1], tt2);
  dist = AIR_NAN;
  /* NOTE that the first and last points are actually not part of the max
     distance calculation, which motivates ensuring that the endpoints
     generated by findVT are actually sufficiently close to the first and last
     points (or else the fit spline won't meet the expected accuracy
     threshold) */
  for (ii = 1; ii < pNum - 1; ii++) {
    double len, Q[2], df[2], ww[4];
    const double *xy;
    CBD0(Q, vv0, vv1, vv2, vv3, uu[ii], ww);
    xy = pntCrd(lpnt, loi, ii);
    ELL_2V_SUB(df, Q, xy);
    len = ELL_2V_LEN(df);
    if (!AIR_EXISTS(dist) || len > dist) {
      dist = len;
      distI = ii;
    }
  }
  fctx->dist = dist;
  fctx->distIdx = distI;
  fctx->distBig
    = (dist <= fctx->nrpDistScl * fctx->distMin
         ? 0
         : (dist <= fctx->distMin ? 1 : (dist <= fctx->nrpPsi * fctx->distMin ? 2 : 3)));
  return;
}

void
limnCBFContextInit(limnCBFContext *fctx, int outputOnly) {
  if (!fctx) return;
  if (!outputOnly) {
    /* defaults for input parameters to various CBF functions */
    fctx->verbose = 0;
    fctx->cornNMS = AIR_TRUE;
    fctx->nrpIterMax = 10;
    fctx->scale = 0;
    fctx->distMin = 0;
    fctx->nrpDeltaMax = 3.0;
    fctx->nrpDistScl = 0.8;
    fctx->nrpPsi = 6;
    fctx->nrpDeltaMin = 0.001;
    fctx->alphaMin = 0.001;
    fctx->detMin = 0.01;
    fctx->cornAngle = 100.0; /* degrees */
  }
  /* internal */
  fctx->uu = fctx->vw = fctx->tw = NULL;
  fctx->mine = NULL;
  fctx->wLen = 0;
  fctx->lenF2L = AIR_NAN;
  /* initialize outputs to bogus valus */
  fctx->nrpIterDone = (uint)(-1);
  fctx->distIdx = (uint)(-1);
  fctx->dist = AIR_POS_INF;
  fctx->nrpDeltaDone = AIR_POS_INF;
  fctx->alphaDet = 0;
  fctx->distBig = 0;
  return;
}

/*
******** limnCBFCheck
**
** checks the things that are going to be passed around a lot
*/
int /* Biff: 1 */
limnCBFCheck(const limnCBFContext *fctx, const limnPoints *lpnt) {
  static const char me[] = "limnCBFCheck";

  if (!(fctx && lpnt)) {
    biffAddf(LIMN, "%s: got NULL pointer", me);
    return 1;
  }
  if (pointsCheck(lpnt)) {
    biffAddf(LIMN, "%s: problem with points", me);
    return 1;
  }
  if (!(fctx->scale >= 0)) {
    biffAddf(LIMN, "%s: need non-negative scale (not %g)", me, fctx->scale);
    return 1;
  }
  if (!(fctx->distMin > 0)) {
    biffAddf(LIMN, "%s: need positive distMin (not %g)", me, fctx->distMin);
    return 1;
  }
  if (fctx->nrpDeltaMin < 0 || fctx->distMin < 0) {
    biffAddf(LIMN,
             "%s: cannot have negative nrpDeltaMin (%g) or "
             "distMin (%g)",
             me, fctx->nrpDeltaMin, fctx->distMin);
    return 1;
  }
  if (!(0 < fctx->nrpDistScl && fctx->nrpDistScl <= 1)) {
    biffAddf(LIMN, "%s: nrpDistScl (%g) must be in (0,1]", me, fctx->nrpDistScl);
    return 1;
  }
  if (!(1 <= fctx->nrpPsi)) {
    biffAddf(LIMN, "%s: nrpPsi (%g) must be >= 1", me, fctx->nrpPsi);
    return 1;
  }
  if (!(fctx->cornAngle < 179)) {
    biffAddf(LIMN, "%s: cornAngle (%g) seems too big", me, fctx->cornAngle);
    return 1;
  }
  return 0;
}

/*
** fitSingle: fits a single cubic Bezier spline, w/out error checking,
** limnCBFSingle is a wrapper around this.
**
** The given points coordinates are in limnPoints lpnt, between low/high
** indices loi/hii (inclusively); hii can be < loi in the case of a point
** loop. From initial endpoint vv0, initial tangent tt1, final endpoint vv3
** and final tangent tt2 (pointing backwards), this function finds alpha such
** that the cubic Bezier spline with control points vv0, vv0 + alpha[0]*tt1,
** vv3 + alpha[1]*tt2, vv3 approximates all the given points.  This is an
** iterative process, in which alpha is solved for multiples times, after
** taking a Newton step to try to optimize the parameterization of the points
** (in an array that is not passed in but instead internal to this function);
** limn.h calls this process "nrp". nrp iterations are stopped after any one
** of following is true (the original published method did not have these
** fine-grained controls):
**  - have done nrpIterMax iterations of nrp
**  - if fctx->nrpDeltaMin > 0: parameterization change falls below deltaMin
**  - if fctx->distMin > 0: distance from spline (as evaluated at the
**    current parameterization) to the given points falls below
**    fctx->nrpDistScl * fctx->distMin
** Information about the results of this process are set in the given
** fctx.
*/
static void
fitSingle(double alpha[2], limnCBFContext *fctx, const double vv0[2],
          const double tt1[2], const double tt2[2], const double vv3[2],
          const limnPoints *lpnt, uint loi, uint hii) {
  static const char me[] = "fitSingle";
  uint iter, pNum;
  const double *xy;

  if (fctx->verbose) {
    printf("%s[%d,%d]: hello, vv0=(%g,%g), tt1=(%g,%g), "
           "tt2=(%g,%g), vv3=(%g,%g)\n",
           me, loi, hii, vv0[0], vv0[1], tt1[0], tt1[1], tt2[0], tt2[1], vv3[0], vv3[1]);
  }
  {
    double F2L[2];
    xy = PP(lpnt);
    ELL_2V_SUB(F2L, xy + 2 * hii, xy + 2 * loi);
    fctx->lenF2L = ELL_2V_LEN(F2L);
  }
  pNum = pntNum(lpnt, loi, hii);
  if (2 == pNum) {
    /* relying on code in findalpha() that handles pNum==2 */
    findalpha(alpha, fctx, vv0, tt1, tt2, vv3, lpnt, loi, hii);
    /* nrp is moot */
    fctx->nrpIterDone = 0;
    /* emmulate results of calling finddist() */
    fctx->dist = fctx->nrpDeltaDone = 0;
    fctx->distIdx = 0;
    fctx->distBig = 0;
  } else {        /* pNum >= 3 */
    double delta; /* avg parameterization change of interior points */
    /* initialize uu parameterization to chord length */
    {
      unsigned int ii;
      double len;
      const double *xyP, *xyM;
      fctx->uu[0] = len = 0;
      xyP = pntCrd(lpnt, loi, 1);
      xyM = pntCrd(lpnt, loi, 0);
      for (ii = 1; ii < pNum; ii++) {
        double dd[2];
        ELL_2V_SUB(dd, xyP, xyM);
        len += ELL_2V_LEN(dd);
        fctx->uu[ii] = len;
        xyM = xyP;
        xyP = pntCrd(lpnt, loi, ii + 1);
      }
      delta = 0;
      for (ii = 0; ii < pNum; ii++) {
        fctx->uu[ii] /= len;
        if (fctx->verbose > 1) {
          printf("%s[%d,%d]: intial uu[%u] = %g\n", me, loi, hii, ii, fctx->uu[ii]);
        }
        delta += AIR_ABS(fctx->uu[ii]);
      }
      delta /= pNum - 2;
      if (fctx->verbose) {
        printf("%s[%d,%d]: initial (chord length) delta = %g\n", me, loi, hii, delta);
      }
    }
    findalpha(alpha, fctx, vv0, tt1, tt2, vv3, lpnt, loi, hii);
    finddist(fctx, alpha, vv0, tt1, tt2, vv3, lpnt, loi, hii);
    if (fctx->distBig < 3) {
      /* initial fit isn't awful; try making it better with nrp */
      for (iter = 0; fctx->distBig && iter < fctx->nrpIterMax; iter++) {
        if (fctx->verbose) {
          printf("%s[%d,%d]: iter %u starting with alpha %g,%g (det %g)\n", me, loi, hii,
                 iter, alpha[0], alpha[1], fctx->alphaDet);
        }
        delta = reparm(fctx, alpha, vv0, tt1, tt2, vv3, lpnt, loi, hii);
        findalpha(alpha, fctx, vv0, tt1, tt2, vv3, lpnt, loi, hii);
        finddist(fctx, alpha, vv0, tt1, tt2, vv3, lpnt, loi, hii);
        if (fctx->verbose) {
          printf("%s[%d,%d]: iter %u (reparm) delta = %g\n", me, loi, hii, iter, delta);
        }
        if (fctx->nrpDeltaMin && delta <= fctx->nrpDeltaMin) {
          if (fctx->verbose) {
            printf("%s[%d,%d]: iter %u delta %g <= min %g --> break\n", me, loi, hii,
                   iter, delta, fctx->nrpDeltaMin);
          }
          break;
        }
      }
      if (fctx->verbose) {
        if (!fctx->distBig) {
          printf("%s[%d,%d]: iter %u finished with good small dist %g\n", me, loi, hii,
                 iter, fctx->dist);
        } else {
          printf("%s[%d,%d]: hit max iters %u with bad (%d) dist %g\n", me, loi, hii,
                 iter, fctx->distBig, fctx->dist);
        }
      }
      fctx->nrpIterDone = iter;
    } else {
      /* else dist so big that we don't even try nrp */
      fctx->nrpIterDone = 0;
    }
    fctx->nrpDeltaDone = delta;
  }
  return;
}

/*
** buffersNew: allocates in fctx:
** uu, vw, tw
*/
static int /* Biff: 1 */
buffersNew(limnCBFContext *fctx, uint pNum) {
  static const char me[] = "buffersNew";
  double kw, kparm[2], vsum, tsum, scl = fctx->scale;
  /* one: what value in summing kernel weights should count as 1.0. This
     should probably be a parm in fctx, but not very interesting to
     change */
  double one = 0.999;
  uint ii, len;

  fctx->uu = AIR_CALLOC(pNum * 2, double);
  if (!fctx->uu) {
    biffAddf(LIMN, "%s: failed to allocate parameter buffer", me);
    return 1;
  }
  if (0 == scl) {
    /* will do simplest possible finite differences; we're done */
    fctx->vw = fctx->tw = NULL;
    return 0;
  }
  /* else need to allocate and set vw and tw buffers */
  kparm[0] = scl;
  kparm[1] = 1000000; /* effectively no cut-off */
  ii = 0;
  vsum = 0;
  do {
    kw = nrrdKernelDiscreteGaussian->eval1_d(ii, kparm);
    vsum += (!ii ? 1 : 2) * kw;
    ii++;
  } while (vsum < one);
  /* intended length of vectors */
  len = ii + 1;
  if (len > 128) {
    biffAddf(LIMN,
             "%s: weight buffer length %u (from scale %g) seems "
             "unreasonable",
             me, len, scl);
    return 1;
  }
  fctx->vw = AIR_CALLOC(len, double);
  fctx->tw = AIR_CALLOC(len, double);
  if (!(fctx->vw && fctx->tw)) {
    biffAddf(LIMN, "%s: couldn't allocate weight buffers (len %u)", me, len);
    return 1;
  }
  fctx->wLen = len;
  /* normalization intent:
     1 = sum_i(vw[|i|]) for i=-(len-1)...len-1
     1 = sum_i(tw[i]) for i=0...len-1
  */
  vsum = tsum = 0;
  for (ii = 0; ii < len; ii++) {
    kw = nrrdKernelDiscreteGaussian->eval1_d(ii, kparm);
    vsum += (!ii ? 1 : 2) * (fctx->vw[ii] = kw);
    tsum += (fctx->tw[ii] = ii * kw);
  }
  for (ii = 0; ii < len; ii++) {
    fctx->vw[ii] /= vsum;
    fctx->tw[ii] /= tsum;
    /* printf("!%s: %u     %g       %g\n", me, ii, fctx->vw[ii], fctx->tw[ii]); */
  }
  return 0;
}

/* returning a pointer so compatible with an airMopper */
static void *
buffersNix(limnCBFContext *fctx) {
  fctx->uu = (double *)airFree(fctx->uu);
  fctx->vw = (double *)airFree(fctx->vw);
  fctx->tw = (double *)airFree(fctx->tw);
  return NULL;
}

/* macros to manage the heap-allocated things inside limnCBFContext; working
   with the idea that each caller passes an OWN variable on their stack, so
   the NIX macro only frees thing when the address of OWN matches that passed
   to the NEW. Nothing else in Teem uses this strategy; it may be exploring
   the clever/stupid boundary that David and Nigel famously identified. */
#define BUFFERS_NEW(FCTX, NN, OWN)                                                      \
  if (!(FCTX)->uu) {                                                                    \
    if (buffersNew((FCTX), (NN))) {                                                     \
      biffAddf(LIMN, "%s: failed to allocate buffers", me);                             \
      return 1;                                                                         \
    }                                                                                   \
    (FCTX)->mine = &(OWN);                                                              \
  }

#define BUFFERS_NIX(FCTX, OWN)                                                          \
  if ((FCTX)->mine == &(OWN)) {                                                         \
    buffersNix(FCTX);                                                                   \
    (FCTX)->mine = NULL;                                                                \
  }

/*
******** limnCBFitSingle
**
** builds a limnPoints around given xy, determines spline
** constraints if necessary, and calls fitSingle
*/
int /* Biff: 1 */
limnCBFitSingle(double alpha[2], limnCBFContext *_fctx, const double _vv0[2],
                const double _tt1[2], const double _tt2[2], const double _vv3[2],
                const double *xy, uint pNum, int isLoop) {
  static const char me[] = "limnCBFitSingle";
  double own, vv0[2], tt1[2], tt2[2], vv3[2];
  uint loi, hii;
  limnCBFContext *fctx, myfctx;
  limnPoints *lpnt;

  if (!(alpha && xy && pNum)) {
    biffAddf(LIMN, "%s: got NULL pointer or 0 points", me);
    return 1;
  }
  lpnt = limnPointsNew(xy, pNum, isLoop);
  loi = 0;
  hii = pNum - 1;
  if (_fctx) {
    fctx = _fctx; /* caller has supplied info */
    if (limnCBFCheck(fctx, lpnt)) {
      biffAddf(LIMN, "%s: problem with fctx", me);
      limnPointsNix(lpnt);
      return 1;
    }
    limnCBFContextInit(fctx, AIR_TRUE /* outputOnly */);
  } else {
    fctx = &myfctx; /* caller supplied nothing: use defaults */
    limnCBFContextInit(fctx, AIR_FALSE /* outputOnly */);
  }
  BUFFERS_NEW(fctx, pNum, own);
  if (setVTTV(NULL, vv0, tt1, tt2, vv3, _vv0, _tt1, _tt2, _vv3, fctx, lpnt, loi, hii)) {
    biffAddf(LIMN, "%s: trouble", me);
    limnPointsNix(lpnt);
    return 1;
  }
  fitSingle(alpha, fctx, vv0, tt1, tt2, vv3, lpnt, loi, hii);
  BUFFERS_NIX(fctx, own);

  limnPointsNix(lpnt);
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

/*
******** limnCBFMulti
**
** Fits one or more geometrically continuous splines to a set of points.  Does
** not look for new internal "corners" (points where the incoming and outgoing
** tangents are different), but does recursively subdivide the points into
** left and right sides around points with the highest error from fitSingle.
*/
int /* Biff: 1 */
limnCBFMulti(limnCBFPath *path, limnCBFContext *fctx, const double _vv0[2],
             const double _tt1[2], const double _tt2[2], const double _vv3[2],
             const limnPoints *lpnt, uint loi, uint hii) {
  static const char me[] = "limnCBFMulti";
  double vv0[2], tt1[2], tt2[2], vv3[2], alpha[2];
  /* &ownbuff determines who frees buffers inside fctx, since each
     function call will have distinct stack location for ownbuff */
  double ownbuff;
  int geomGiven;
  uint pNum;

  /* need non-NULL fctx in order to know fctx->distMin */
  if (limnCBFCheck(fctx, lpnt)) {
    biffAddf(LIMN, "%s: got bad args", me);
    return 1;
  }
  if (!(loi < lpnt->num && hii < lpnt->num)) {
    biffAddf(LIMN, "%s: need loi (%u), hii (%u) < #points %u", me, loi, hii, lpnt->num);
    return 1;
  }
  if (loi == hii) {
    biffAddf(LIMN, "%s: need loi (%u) != hii (%u)", me, loi, hii);
    return 1;
  }
  if (hii < loi && !lpnt->isLoop) {
    biffAddf(LIMN, "%s: hii (%u) can be < loi (%u) only in loop", me, loi, hii);
    return 1;
  }
  pNum = pntNum(lpnt, loi, hii);
  BUFFERS_NEW(fctx, pNum, ownbuff);
  if (setVTTV(&geomGiven, vv0, tt1, tt2, vv3, _vv0, _tt1, _tt2, _vv3, fctx, lpnt, loi,
              hii)) {
    biffAddf(LIMN, "%s: trouble", me);
    return 1;
  }
  if (fctx->verbose) {
    printf("%s[%u,%u]: hello; %s v0=(%g,%g), t1=(%g,%g), t2=(%g,%g), "
           "v3=(%g,%g)\n",
           me, loi, hii, geomGiven ? "given" : "computed", vv0[0], vv0[1], tt1[0],
           tt1[1], tt2[0], tt2[1], vv3[0], vv3[1]);
  }

  /* first try fitting a single spline */
  if (fctx->verbose) {
    printf("%s[%u,%u]: trying single fit on all points\n", me, loi, hii);
  }
  fitSingle(alpha, fctx, vv0, tt1, tt2, vv3, lpnt, loi, hii);
  if (fctx->distBig <= 1) {
    /* max dist was <= fctx->distMin: single fit was good enough */
    if (fctx->verbose) {
      printf("%s[%u,%u]: single fit good: nrpi=%u; dist=%g@%u <= %g; "
             "det=%g; alpha=%g,%g\n",
             me, loi, hii, fctx->nrpIterDone, fctx->dist, fctx->distIdx, fctx->distMin,
             fctx->alphaDet, alpha[0], alpha[1]);
    }
    airArrayLenSet(path->segArr, 1);
    ELL_2V_COPY(path->seg[0].xy + 0, vv0);
    ELL_2V_SCALE_ADD2(path->seg[0].xy + 2, 1, vv0, alpha[0], tt1);
    ELL_2V_SCALE_ADD2(path->seg[0].xy + 4, 1, vv3, alpha[1], tt2);
    ELL_2V_COPY(path->seg[0].xy + 6, vv3);
    path->seg[0].pNum = pNum;
  } else { /* need to subdivide at fctx->distIdx and recurse */
    uint mi = fctx->distIdx;
    double ttL[2], mid[2], ttR[2];
    limnCBFPath *prth = limnCBFPathNew(); /* right path */
    limnCBFContext fctxL, fctxR;
    memcpy(&fctxL, fctx, sizeof(limnCBFContext));
    memcpy(&fctxR, fctx, sizeof(limnCBFContext));
    if (fctx->verbose) {
      printf("%s[%u,%u]: dist %g big (%d) --> split at %u\n", me, loi, hii, fctx->dist,
             fctx->distBig, mi);
    }
    findVT(mid, ttR, fctx, lpnt, loi, hii, mi, 0);
    ELL_2V_SCALE(ttL, -1, ttR);
    /* on recursion, can't be a loop, so isLoop is AIR_FALSE */
    if (limnCBFMulti(path, &fctxL, vv0, tt1, ttL, mid, lpnt, loi, mi)
        || limnCBFMulti(prth, &fctxR, mid, ttR, tt2, vv3, lpnt, mi, hii)) {
      biffAddf(LIMN, "%s[%u,%u]: trouble on recursive fit", me, loi, hii);
      limnCBFPathNix(prth);
      return 1;
    }
    limnCBFPathJoin(path, prth);
    limnCBFPathNix(prth);
    fctx->nrpIterDone = fctxL.nrpIterDone + fctxR.nrpIterDone;
    if (fctxL.dist > fctxR.dist) {
      fctx->dist = fctxL.dist;
      fctx->distIdx = fctxL.distIdx;
      fctx->distBig = fctxL.distBig;
    } else {
      fctx->dist = fctxR.dist;
      fctx->distIdx = fctxR.distIdx;
      fctx->distBig = fctxR.distBig;
    }
    fctx->nrpDeltaDone = AIR_MAX(fctxL.nrpDeltaDone, fctxR.nrpDeltaDone);
    fctx->alphaDet = AIR_MIN(fctxL.alphaDet, fctxR.alphaDet);
  }

  BUFFERS_NIX(fctx, ownbuff);
  return 0;
}

int /* Biff: 1 */
limnCBFCorners(uint **cornIdx, uint *cornNum, limnCBFContext *fctx,
               const limnPoints *lpnt) {
  static const char me[] = "limnCBFCorners";
  airArray *mop, *cornArr;
  double ownbuff, *angle;
  uint ii, pNum, loi, hii;
  int *corn;

  if (!(cornIdx && cornNum && fctx && lpnt)) {
    biffAddf(LIMN, "%s: got NULL pointer", me);
    return 1;
  }
  if (limnCBFCheck(fctx, lpnt)) {
    biffAddf(LIMN, "%s: got bad args", me);
    return 1;
  }
  if (!fctx->cornAngle) {
    /* nothing much to do here, because caller doesn't want corners */
    *cornIdx = NULL;
    *cornNum = 0;
    return 0;
  }
  loi = 0;
  hii = lpnt->num - 1;
  pNum = pntNum(lpnt, loi, hii);
  angle = AIR_CALLOC(pNum, double);
  assert(angle);
  /* why assert: GLK tiring of using biff to report allocation failures */
  corn = AIR_CALLOC(pNum, int);
  assert(corn);
  mop = airMopNew();
  airMopAdd(mop, angle, airFree, airMopAlways);
  airMopAdd(mop, corn, airFree, airMopAlways);
  cornArr = airArrayNew(AIR_CAST(void **, cornIdx), cornNum, sizeof(uint), 32);
  /* free with Nix, not Nuke, because we are managing the given pointers */
  airMopAdd(mop, cornArr, (airMopper)airArrayNix, airMopAlways);
  BUFFERS_NEW(fctx, pNum, ownbuff);
  for (ii = 0; ii < pNum; ii++) {
    double LT[2], RT[2];
    findVT(NULL, LT, fctx, lpnt, loi, hii, ii, -1);
    findVT(NULL, RT, fctx, lpnt, loi, hii, ii, +1);
    angle[ii] = 180 * ell_2v_angle_d(LT, RT) / AIR_PI;
    corn[ii] = (angle[ii] < fctx->cornAngle);
  }
  if (fctx->cornNMS) {
    for (ii = 0; ii < pNum; ii++) {
      uint iplus, imnus;
      iplus = (ii < pNum - 1 ? ii + 1 : (lpnt->isLoop ? 1 : pNum - 1));
      imnus = (ii ? ii - 1 : (lpnt->isLoop ? pNum - 2 : 0));
      /* stays a corner only if angle smaller than neighbors */
      corn[ii] &= (angle[ii] < angle[iplus] && angle[ii] < angle[imnus]);
    }
  }
  for (ii = 0; ii < pNum; ii++) {
    uint ci;
    if (!corn[ii]) continue;
    ci = airArrayLenIncr(cornArr, 1);
    (*cornIdx)[ci] = ii;
  }
  BUFFERS_NIX(fctx, ownbuff);
  airMopOkay(mop);
  return 0;
}

/*
******** limnCBFit
**
** top-level function for fitting cubic beziers to given points
*/
int /* Biff: 1 */
limnCBFit(limnCBFPath *path, limnCBFContext *fctx, const double *xy, uint pNum,
          int isLoop) {
  static const char me[] = "limnCBFit";
  uint *cornIdx = NULL, cornNum = 0, cii, loi, hii;
  limnCBFPath *rpth;
  limnPoints *lpnt;
  int ret;
  airArray *mop;

  if (!(path && fctx && xy)) {
    biffAddf(LIMN, "%s: got NULL pointer", me);
    return 1;
  }
  lpnt = limnPointsNew(xy, pNum, isLoop);
  mop = airMopNew();
  airMopAdd(mop, lpnt, (airMopper)limnPointsNix, airMopAlways);
  if (limnCBFCheck(fctx, lpnt)) {
    biffAddf(LIMN, "%s: got bad args", me);
    airMopError(mop);
    return 1;
  }
  if (fctx->uu) {
    biffAddf(LIMN, "%s: not expecting limnCBFContext buffers allocated", me);
    airMopError(mop);
    return 1;
  }
  if (buffersNew(fctx, pNum)) {
    biffAddf(LIMN, "%s: failed to allocate buffers", me);
    airMopError(mop);
    return 1;
  }
  airMopAdd(mop, fctx, (airMopper)buffersNix, airMopAlways);

  if (limnCBFCorners(&cornIdx, &cornNum, fctx, lpnt)) {
    biffAddf(LIMN, "%s: trouble finding corners", me);
    airMopError(mop);
    return 1;
  }
  if (!cornNum) {
    /* no corners; do everything with one multi call */
    ret = limnCBFMulti(path, fctx, NULL, NULL, NULL, NULL, lpnt, 0, pNum - 1);
    path->isLoop = isLoop;
    if (ret) biffAddf(LIMN, "%s: trouble", me);
    airMopDone(mop, ret);
    return ret;
  }
  /* else do have corners: split points into segments between corners */
  /* TODO: if fitting between corners involves wrapping idx past pNum-1? */
  airArrayLenSet(path->segArr, 0);
  loi = 0;
  for (cii = 0; cii < cornNum; cii++) {
    hii = cornIdx[cii];
    rpth = limnCBFPathNew();
    ret = limnCBFMulti(rpth, fctx, NULL, NULL, NULL, NULL, lpnt, loi, hii);
    if (ret) {
      biffAddf(LIMN, "%s: trouble on corner %u", me, cii);
      airMopError(mop);
      return 1;
    }
    rpth->seg[0].corner[0] = 1;
    rpth->seg[rpth->segNum - 1].corner[1] = 1;
    limnCBFPathJoin(path, rpth);
    limnCBFPathNix(rpth);
    loi = hii;
  }
  rpth = limnCBFPathNew();
  ret = limnCBFMulti(rpth, fctx, NULL, NULL, NULL, NULL, lpnt, loi, pNum - 1);
  if (ret) {
    biffAddf(LIMN, "%s: trouble after last corner", me);
    airMopError(mop);
    return 1;
  }
  rpth->seg[0].corner[0] = 1;
  rpth->seg[rpth->segNum - 1].corner[1] = 1;
  limnCBFPathJoin(path, rpth);
  limnCBFPathNix(rpth);

  path->isLoop = isLoop;
  airMopOkay(mop);
  return 0;
}

/*
TODO:
rewrite things to use limnPointList, with first and last indices,
naturally handling the case that last < first, with isLoop
and new logic: isLoop does NOT depend on duplicate 1st,last coords
and subtlty that if (with isLoop) hii = (loi-1 % #points) then using
all points, with no notion of corner possible

testing corners: corners at start==stop of isLoop
corners not at start or stop of isLoop: do spline wrap around from last to first index?

limnCBFPrune to remove (in-place) coincident and nearly coincident points in xy

use performance tests to explore optimal settings in fctx:
  nrpIterMax, nrpDeltaMax, nrpDistScl, nrpPsi, nrpDeltaMin
evaluated in terms of time and #splines needed for fit
(may want to pay in time for more economical representation)

valgrind everything
*/
