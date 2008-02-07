/*
  Teem: Tools to process and visualize scientific data and images
  Copyright (C) 2006, 2005  Gordon Kindlmann
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

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


#include "ten.h"
#include "privateTen.h"

#define SQRT6 2.44948974278317809819
#define SQRT2 1.41421356237309504880
#define SQRT3 1.73205080756887729352

static void
tenQBL_eval_to_XYZ(double XYZ[3], const double eval[3]) {
  double mat[] = 
    {2/SQRT6, -1/SQRT6, -1/SQRT6,
     0,        1/SQRT2, -1/SQRT2,
     1/SQRT3,  1/SQRT3,  1/SQRT3};

  ELL_3MV_MUL(XYZ, mat, eval);
  return;
}

static void
tenQBL_XYZ_to_eval(double eval[3], const double XYZ[3]) {
  double mat[] = 
    {2/SQRT6,   0,       1/SQRT3,
     -1/SQRT6,  1/SQRT2, 1/SQRT3,
     -1/SQRT6, -1/SQRT2, 1/SQRT3};

  ELL_3MV_MUL(eval, mat, XYZ);
  return;
}

#undef SQRT6
#undef SQRT2
#undef SQRT3

static void
tenQGL_eval_to_RThZ(double RThZ[3], const double eval[3]) {
  double XYZ[3];
  
  tenQBL_eval_to_XYZ(XYZ, eval);
  RThZ[0] = sqrt(XYZ[0]*XYZ[0] + XYZ[1]*XYZ[1]);
  RThZ[1] = atan2(XYZ[1], XYZ[0]);
  RThZ[2] = XYZ[2];
}

static void
tenQGL_RThZ_to_eval(double eval[3], const double RThZ[3]) {
  double XYZ[3];

  XYZ[0] = RThZ[0]*cos(RThZ[1]);
  XYZ[1] = RThZ[0]*sin(RThZ[1]);
  XYZ[2] = RThZ[2];
  tenQBL_XYZ_to_eval(eval, XYZ);
}

/* 
** computes (r1 - r0)/(log(r1) - log(r0))
*/
double
_tenQGL_blah(double rr0, double rr1) {
  double bb, ret;

  if (rr1 < rr0) {
    ret = _tenQGL_blah(rr1, rr0);
  } else {
    /* rr1 >= rr0  -->  rr1/rr0 >= 1  -->  rr1/rr0 - 1 >= 0 */
    if (rr0 < 0.0000000001) {
      ret = 0;
    } else {
      bb = rr1/rr0 - 1;  /* from above, bb >= 0 */
      if (bb < 0.001) {
        ret = rr0*(1 + bb*(1.0/2 - bb*(1.0/12 + bb*(1.0/24 - bb*19.0/720))));
      } else {
        ret = (rr1 - rr0)/log(rr1/rr0);
      }
    }
  }
  return ret;
}

#define rr0  (RThZA[0])
#define rr1  (RThZB[0])
#define rr  (oRThZ[0])
#define th0  (RThZA[1])
#define th1  (RThZB[1])
#define th  (oRThZ[1])
#define zz0  (RThZA[2])
#define zz1  (RThZB[2])
#define zz  (oRThZ[2])

void
tenQGLInterpTwoEvalK(double oeval[3],
                     const double evalA[3], const double evalB[3],
                     double tt) {
  double RThZA[3], RThZB[3], oRThZ[3], bb;

  tenQGL_eval_to_RThZ(RThZA, evalA);
  tenQGL_eval_to_RThZ(RThZB, evalB);
  
  rr = AIR_LERP(tt, rr0, rr1);
  zz = AIR_LERP(tt, zz0, zz1);
  bb = rr1/rr0 - 1;
  if (AIR_ABS(bb) < 0.001) {
    double dth;
    dth = th1 - th0;
    /* rr0 and rr1 are similar, use stable approximation */
    th = th0 + tt*(dth
                   + (0.5 + tt/2)*dth*bb 
                   + (-1.0/12 - tt/4 + tt*tt/3)*dth*bb*bb
                   + (1.0/24 + tt/24 + tt*tt/6 - tt*tt*tt/4)*dth*bb*bb*bb);
  } else {
    /* use original formula */
    th = th0 + (th1 - th0)*log(1 + bb*tt)/log(1 + bb);
  }
  
  tenQGL_RThZ_to_eval(oeval, oRThZ);

}

double
_tenQGL_Kdist(const double RThZA[3], const double RThZB[3]) {
  double dr, dth, dz, bl;

  dr = rr1 - rr0;
  bl = _tenQGL_blah(rr0, rr1);
  dth = th1  - th0;
  dz = zz1 - zz0;
  return sqrt(dr*dr + bl*bl*dth*dth + dz*dz);
}

void
_tenQGL_Klog(double klog[3],
             const double RThZA[3], const double RThZB[3]) {
  double dr, bl, dth, dz;

  dr = rr1 - rr0;
  bl = _tenQGL_blah(rr0, rr1);
  dth = th1  - th0;
  dz = zz1 - zz0;
  ELL_3V_SET(klog, dr, bl*dth, dz);
  return;
}

void
_tenQGL_Kexp(double RThZB[3], 
             const double RThZA[3], const double klog[3]) {
  double bl;
  
  rr1 = rr0 + klog[0];
  bl = _tenQGL_blah(rr0, rr1);
  th1 = th0 + (bl ? klog[1]/bl : 0);
  zz1 = zz0 + klog[2];
  return;
}

#undef rr0
#undef rr1
#undef rr
#undef th0
#undef th1
#undef th
#undef zz0
#undef zz1
#undef zz


static void
tenQGL_eval_to_RThPh(double RThPh[3], const double eval[3]) {
  double XYZ[3];
  
  tenQBL_eval_to_XYZ(XYZ, eval);
  RThPh[0] = sqrt(XYZ[0]*XYZ[0] + XYZ[1]*XYZ[1] + XYZ[2]*XYZ[2]);
  RThPh[1] = atan2(XYZ[1], XYZ[0]);
  RThPh[2] = asin(sqrt(XYZ[0]*XYZ[0] + XYZ[1]*XYZ[1])/RThPh[0]);
}

static void
tenQGL_RThPh_to_eval(double eval[3], const double RThPh[3]) {
  double XYZ[3];

  XYZ[0] = RThPh[0]*cos(RThPh[1])*sin(RThPh[2]);
  XYZ[1] = RThPh[0]*sin(RThPh[1])*sin(RThPh[2]);
  XYZ[2] = RThPh[0]*cos(RThPh[2]);
  tenQBL_XYZ_to_eval(eval, XYZ);
}

#define rr0  (RThPhA[0])
#define rr1  (RThPhB[0])
#define rr  (oRThPh[0])
#define th0  (RThPhA[1])
#define th1  (RThPhB[1])
#define th  (oRThPh[1])
#define ph0  (RThPhA[2])
#define ph1  (RThPhB[2])
#define ph  (oRThPh[2])

void
tenQGLInterpTwoEvalR(double oeval[3],
                     const double evalA[3], const double evalB[3],
                     double tt) {
  double RThPhA[3], RThPhB[3], oRThPh[3], bb, ltph, ltph0, ltph1;

  tenQGL_eval_to_RThPh(RThPhA, evalA);
  tenQGL_eval_to_RThPh(RThPhB, evalB);
  
  rr = AIR_LERP(tt, rr0, rr1);
  /* HEY: CUT AND PASTE (with th -> ph) FROM ABOVE */
  bb = rr1/rr0 - 1;
  if (AIR_ABS(bb) < 0.001) {
    double dph;
    dph = ph1 - ph0;
    /* rr0 and rr1 are similar, use stable approximation */
    ph = ph0 + tt*(dph
                   + (0.5 + tt/2)*dph*bb 
                   + (-1.0/12 - tt/4 + tt*tt/3)*dph*bb*bb
                   + (1.0/24 + tt/24 + tt*tt/6 - tt*tt*tt/4)*dph*bb*bb*bb);
  } else {
    /* use original formula */
    ph = ph0 + (ph1 - ph0)*log(1 + bb*tt)/log(rr1/rr0);
  }
  ltph = log(tan(ph/2));
  ltph0 = log(tan(ph0/2));
  ltph1 = log(tan(ph1/2));
  th = th0 + (th1 - th0)*(ltph - ltph0)/(ltph1 - ltph0);

  tenQGL_RThPh_to_eval(oeval, oRThPh);
  return;
}

double
_tenQGL_Rdist(const double RThPhA[3], const double RThPhB[3]) {
  double dr, dth, dph, bl, dlt;

  dr = rr1 - rr0;
  dth = th1 - th0;
  dph = ph1 - ph0;
  bl = _tenQGL_blah(rr0, rr1);
  dlt = log(tan(ph1/2)) - log(tan(ph0/2));
  return sqrt(dr*dr + bl*bl*dph*dph*(dth*dth/(dlt*dlt) + 1));
}

void
_tenQGL_Rlog(double rlog[3],
             const double RThPhA[3], const double RThPhB[3]) {
  double dr, dth, dph, bl, dlt;

  dr = rr1 - rr0;
  dth = th1 - th0;
  dph = ph1 - ph0;
  bl = _tenQGL_blah(rr0, rr1);
  dlt = log(tan(ph1/2)) - log(tan(ph0/2));
  /*             rlog[0]    rlog[1]     rlog[2] */
  ELL_3V_SET(rlog,  dr,  dph*bl*dth/dlt, dph*bl);
}

void
_tenQGL_Rexp(double RThPhB[3], 
             const double RThPhA[3], const double rlog[3]) {
  double dph, bl, dlt;
  
  rr1 = rr0 + rlog[0];
  bl = _tenQGL_blah(rr0, rr1);
  ph1 = ph0 + (bl ? rlog[2]/bl : 0);
  dph = ph1 - ph0;
  dlt = log(tan(ph1/2)) - log(tan(ph0/2));
  th1 = th0 + rlog[1]*dlt/(bl*dph);
  return;
}

#undef rr0
#undef rr1
#undef rr
#undef th0
#undef th1
#undef th
#undef ph0
#undef ph1
#undef ph

/* returns the index into unitq[] of the quaternion that led to the
   right alignment.  If it was already aligned, this will be 0, 
   because unitq[0] is the identity quaternion */
int
_tenQGL_q_align(double qOut[4], const double qRef[4], const double qIn[4]) {
  unsigned int ii, maxDotIdx;
  double unitq[8][4] = {{+1, 0, 0, 0},
                        {-1, 0, 0, 0},
                        {0, +1, 0, 0},
                        {0, -1, 0, 0},
                        {0, 0, +1, 0},
                        {0, 0, -1, 0},
                        {0, 0, 0, +1},
                        {0, 0, 0, -1}};
  double dot[8], qInMul[8][4], maxDot;
  
  for (ii=0; ii<8; ii++) {
    ell_q_mul_d(qInMul[ii], unitq[ii], qIn);
    dot[ii] = ELL_4V_DOT(qRef, qInMul[ii]);
  }
  maxDotIdx = 0;
  maxDot = dot[maxDotIdx];
  for (ii=1; ii<8; ii++) {
    if (dot[ii] > maxDot) {
      maxDotIdx = ii;
      maxDot = dot[maxDotIdx];
    }
  }
  ELL_4V_COPY(qOut, qInMul[maxDotIdx]);
  return maxDotIdx;
}

void
tenQGLInterpTwoEvec(double oevec[9],
                    const double evecA[9], const double evecB[9],
                    double tt) {
  double oq[4], qA[4], qB[4], _qB[4], qdiv[4], angle, axis[3], qq[4];

  ell_3m_to_q_d(qA, evecA);
  ell_3m_to_q_d(_qB, evecB);
  _tenQGL_q_align(qB, qA, _qB);
  /* there's probably a faster way to do this slerp qA --> qB */
  ell_q_div_d(qdiv, qA, qB); /* div = A^-1 * B */
  angle = ell_q_to_aa_d(axis, qdiv);
  ell_aa_to_q_d(qq, angle*tt, axis);
  ell_q_mul_d(oq, qA, qq);
  ell_q_to_3m_d(oevec, oq);
}

void
tenQGLInterpTwo(double oten[7],
                const double tenA[7], const double tenB[7],
                int ptype, double tt, tenPathParm *_tpp) {
  airArray *mop;
  tenPathParm *tpp;
  double oeval[3], evalA[3], evalB[3], oevec[9], evecA[9], evecB[9], cc;

  mop = airMopNew();
  if (_tpp) {
    tpp = _tpp;
  } else {
    tpp = tenPathParmNew();
    airMopAdd(mop, tpp, (airMopper)tenPathParmNix, airMopAlways);
  }

  tenEigensolve_d(evalA, evecA, tenA);
  tenEigensolve_d(evalB, evecB, tenB);
  cc = AIR_LERP(tt, tenA[0], tenB[0]);

  if (tenPathTypeQuatGeoLoxK == ptype) {
    tenQGLInterpTwoEvalK(oeval, evalA, evalB, tt);
  } else {
    tenQGLInterpTwoEvalR(oeval, evalA, evalB, tt);
  }
  tenQGLInterpTwoEvec(oevec, evecA, evecB, tt);
  tenMakeOne_d(oten, cc, oeval, oevec);

  airMopOkay(mop);
  return;
}

int
_tenQGLInterpNEval(double evalOut[3],
                   const double *evalIn, /* size 3 -by- NN */
                   const double *wght,   /* size NN */
                   unsigned int NN,
                   int ptype, tenPathParm *tpp) {
  char me[]="_tenQGLInterpNEvalK", err[BIFF_STRLEN];
  double RTh_Out[3], *RTh_In;
  airArray *mop;
  unsigned int ii;

  if (!( NN >= 2 )) {
    sprintf(err, "%s: need N >= 2 (not %u)", me, NN);
    biffAdd(TEN, err); return 1;
  }
  if (!( evalOut && evalIn && wght )) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(TEN, err); return 1;
  }
  
  mop = airMopNew();
  RTh_In = AIR_CAST(double *, calloc(3*NN, sizeof(double)));
  if (!( RTh_In )) {
    sprintf(err, "%s: couldn't alloc local buffer", me);
    biffAdd(TEN, err); airMopError(mop); return 1;
  }
  airMopAdd(mop, RTh_In, airFree, airMopAlways);

  /* convert to (R,Th,_) and initialize RTh_Out */
  ELL_3V_SET(RTh_Out, 0, 0, 0);
  for (ii=0; ii<NN; ii++) {
    if (tenPathTypeQuatGeoLoxK == ptype) {
      tenQGL_eval_to_RThZ(RTh_In + 3*ii, evalIn + 3*ii);
    } else {
      tenQGL_eval_to_RThPh(RTh_In + 3*ii, evalIn + 3*ii);
    }
    ELL_3V_SCALE_INCR(RTh_Out, wght[ii], RTh_In + 3*ii);
  }

  /* compute iterated weighted mean, stored in RTh_Out */
  /* ... */

  /* finish, convert to eval */
  if (tenPathTypeQuatGeoLoxK == ptype) {
    tenQGL_RThZ_to_eval(evalOut, RTh_Out);
  } else {
    tenQGL_RThPh_to_eval(evalOut, RTh_Out);
  }

  airMopOkay(mop);
  return 0;
}

double
_tenQGL_q_interdot(unsigned int *centerIdxP,
                   double *qq, double *inter, unsigned int NN) {
  unsigned int ii, jj;
  double sum, dot, max;

  for (jj=0; jj<NN; jj++) {
    for (ii=0; ii<NN; ii++) {
      inter[ii + NN*jj] = 0;
    }
  }
  sum = 0;
  for (jj=0; jj<NN; jj++) {
    inter[jj + NN*jj] = 1.0;
    for (ii=jj+1; ii<NN; ii++) {
      dot = ELL_4V_DOT(qq + 4*ii, qq + 4*jj);
      inter[ii + NN*jj] = dot;
      inter[jj + NN*ii] = dot;
      sum += dot;
    }
  }
  for (jj=0; jj<NN; jj++) {
    for (ii=1; ii<NN; ii++) {
      inter[0 + NN*jj] += inter[ii + NN*jj];
    }
  }
  *centerIdxP = 0;
  max = inter[0 + NN*(*centerIdxP)];
  for (jj=1; jj<NN; jj++) {
    if (inter[0 + NN*jj] > max) {
      *centerIdxP = jj;
      max = inter[0 + NN*(*centerIdxP)];
    }
  }
  return sum;
}

int
_tenQGLInterpNEvec(double evecOut[9],
                   const double *evecIn, /* size 9 -by- NN */
                   const double *wght,   /* size NN */
                   unsigned int NN,
                   tenPathParm *tpp) {
  char me[]="_tenQGLInterpNEvec", err[BIFF_STRLEN];
  double *qIn, qOut[4], maxWght, len, *inter, dsum;
  airArray *mop;
  unsigned int ii, centerIdx=0, jj, fix;

  if (!( NN >= 2 )) {
    sprintf(err, "%s: need N >= 2 (not %u)", me, NN);
    biffAdd(TEN, err); return 1;
  }
  if (!( evecOut && evecIn && wght )) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(TEN, err); return 1;
  }
  
  mop = airMopNew();
  qIn = AIR_CAST(double *, calloc(4*NN, sizeof(double)));
  inter = AIR_CAST(double *, calloc(NN*NN, sizeof(double)));
  if (!( qIn && inter)) {
    sprintf(err, "%s: couldn't alloc local buffers", me);
    biffAdd(TEN, err); airMopError(mop); return 1;
  }
  airMopAdd(mop, qIn, airFree, airMopAlways);

  /* convert to quaternions */
  for (ii=0; ii<NN; ii++) {
    ell_3m_to_q_d(qIn + 4*ii, evecIn + 4*ii);
  }
  dsum = _tenQGL_q_interdot(&centerIdx, qIn, inter, NN);
  fprintf(stderr, "!%s: 0 interdot = %g\n", me, dsum);

  /* find quaternion with maximal weight, use it as is (decree that
     its the right representative), and then align rest with that.
     This is actually principled; symmetry allows it */
  centerIdx = 0;
  maxWght = wght[centerIdx];
  for (ii=1; ii<NN; ii++) {
    if (wght[ii] > maxWght) {
      centerIdx = ii;
      maxWght = wght[centerIdx];
    }
  }
  for (ii=0; ii<NN; ii++) {
    if (ii == centerIdx) {
      continue;
    }
    _tenQGL_q_align(qIn + 4*ii, qIn + 4*centerIdx, qIn + 4*ii);
  }
  dsum = _tenQGL_q_interdot(&centerIdx, qIn, inter, NN);
  fprintf(stderr, "!%s: 1 interdot = %g, center = %u\n", me, dsum, centerIdx);

  /* try to settle on tightest set of representatives */
  do {
    fix = 0;
    for (ii=0; ii<NN; ii++) {
      unsigned int ff;
      if (ii == centerIdx) {
        continue;
      }
      ff = _tenQGL_q_align(qIn + 4*ii, qIn + 4*centerIdx, qIn + 4*ii);
      fix = AIR_MAX(fix, ff);
    }
    dsum = _tenQGL_q_interdot(&centerIdx, qIn, inter, NN);
    fprintf(stderr, "!%s: interdot = %g -> maxfix = %u; center = %u\n", me,
            dsum, fix, centerIdx);
  } while (fix);
  /* make sure they're normalized */
  for (ii=0; ii<NN; ii++) {
    ELL_4V_NORM(qIn + 4*ii, qIn + 4*ii, len);
  }

  /* compute iterated weighted mean, stored in qOut */
  if (ell_q_avgN_d(qOut, NULL, qIn, wght, NN, tpp->convEps, tpp->maxIter)) {
    sprintf(err, "%s: problem doing quaternion mean", me);
    biffMove(TEN, err, ELL); airMopError(mop); return 1;
  }
  
  /* finish, convert back to evec */
  ell_q_to_3m_d(evecOut, qOut);

  airMopOkay(mop);
  return 0;
}

int
tenQGLInterpN(double tenOut[7],
              const double *tenIn,
              const double *wght, 
              unsigned int NN,
              int ptype, tenPathParm *_tpp) {
  char me[]="tenQGLInterpN", err[BIFF_STRLEN];
  airArray *mop;
  double *evalIn, *evecIn, evalOut[3], evecOut[9], conf, wghtSum;
  tenPathParm *tpp;
  unsigned int ii;

  if (!( NN >= 2 )) {
    sprintf(err, "%s: need N >= 2 (not %u)", me, NN);
    biffAdd(TEN, err); return 1;
  }
  if (!( tenOut && tenIn && wght )) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(TEN, err); return 1;
  }

  mop = airMopNew();
  if (_tpp) {
    tpp = _tpp;
  } else {
    tpp = tenPathParmNew();
    airMopAdd(mop, tpp, (airMopper)tenPathParmNix, airMopAlways);
  }
  evalIn = AIR_CAST(double *, calloc(3*NN, sizeof(double)));
  evecIn = AIR_CAST(double *, calloc(9*NN, sizeof(double)));
  if (!( evalIn && evecIn )) {
    sprintf(err, "%s: couldn't alloc local buffers", me);
    biffAdd(TEN, err); airMopError(mop); return 1;
  }
  airMopAdd(mop, evalIn, airFree, airMopAlways);
  airMopAdd(mop, evecIn, airFree, airMopAlways);
  
  conf = 0;
  wghtSum = 0;
  for (ii=0; ii<NN; ii++) {
    tenEigensolve_d(evalIn + 3*ii, evecIn + 9*ii, tenIn + 7*ii);
    wghtSum += wght[ii];
    conf += wght[ii]*(tenIn + 7*ii)[0];
  }
  if (!( AIR_IN_CL(1 - tpp->wghtSumEps, wghtSum, 1 + tpp->wghtSumEps) )) {
    sprintf(err, "%s: wght sum %g not within %g of 1.0", me,
            wghtSum, tpp->wghtSumEps);
    biffAdd(TEN, err); airMopError(mop); return 1;
  }

  if (_tenQGLInterpNEval(evalOut, evalIn, wght, NN, ptype, tpp)
      || _tenQGLInterpNEvec(evecOut, evecIn, wght, NN, tpp)) {
    sprintf(err, "%s: trouble computing", me);
    biffAdd(TEN, err); airMopError(mop); return 1;
  }
  tenMakeOne_d(tenOut, conf, evalOut, evecOut);

  airMopOkay(mop);
  return 0;
}

