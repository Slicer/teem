/*
  Teem: Tools to process and visualize scientific data and images              
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

#include "ten.h"
#include "privateTen.h"

#define PARM_NUM 7
/* 1/sqrt(2) */
#define OST 0.70710678118654752440
#define PARM_DESC                                                           \
  {                                                                         \
    {"B0", 0.0, TEN_MODEL_B0_MAX, AIR_FALSE, 0},                            \
    {"Dxx", -TEN_MODEL_DIFF_MAX,     TEN_MODEL_DIFF_MAX,     AIR_FALSE, 0}, \
    {"Dxy", -TEN_MODEL_DIFF_MAX*OST, TEN_MODEL_DIFF_MAX*OST, AIR_FALSE, 0}, \
    {"Dxz", -TEN_MODEL_DIFF_MAX*OST, TEN_MODEL_DIFF_MAX*OST, AIR_FALSE, 0}, \
    {"Dyy", -TEN_MODEL_DIFF_MAX,     TEN_MODEL_DIFF_MAX,     AIR_FALSE, 0}, \
    {"Dyz", -TEN_MODEL_DIFF_MAX*OST, TEN_MODEL_DIFF_MAX*OST, AIR_FALSE, 0}, \
    {"Dzz", -TEN_MODEL_DIFF_MAX,     TEN_MODEL_DIFF_MAX,     AIR_FALSE, 0}, \
  }
static const tenModelParmDesc
const pdesc[TEN_MODEL_PARM_MAXNUM] = PARM_DESC;

static void 
simulate(double *dwiSim, const double *parm, const tenExperSpec *espec) {
  unsigned int ii;
  double b0, ten[7];

  b0 = parm[0];
  ten[0] = 1;
  ten[1] = parm[1];
  ten[2] = parm[2];
  ten[3] = parm[3];
  ten[4] = parm[4];
  ten[5] = parm[5];
  ten[6] = parm[6];
  for (ii=0; ii<espec->imgNum; ii++) {
    double adc, bb;
    bb = espec->bval[ii];
    adc = TEN_T3V_CONTR(ten, espec->grad + 3*ii);
    dwiSim[ii] = b0*exp(-bb*adc);
  }
  return;
}

static void
prand(double *parm, airRandMTState *rng) {
  unsigned int ii;

  for (ii=0; ii<PARM_NUM; ii++) {
    if (pdesc[ii].vec3) {
      /* its unit vector */
      double xx, yy, zz, theta, rr;
      
      zz = AIR_AFFINE(0.0, airDrandMT_r(rng), 1.0, -1.0, 1.0);
      theta = AIR_AFFINE(0.0, airDrandMT_r(rng), 1.0, 0.0, 2*AIR_PI);
      rr = sqrt(1 - zz*zz);
      xx = rr*cos(theta);
      yy = rr*sin(theta);
      parm[ii + 0] = xx;
      parm[ii + 1] = yy;
      parm[ii + 2] = zz;
      /* bump ii by 2, anticipating completion of this for-loop iter */
      ii += 2;
    } else {
      parm[ii] = AIR_AFFINE(0.0, airDrandMT_r(rng), 1.0,
                            pdesc[ii].min, pdesc[ii].max);
    }
  }
  return;
}

SQE;

static void
sqeGrad(double *grad, const double *parm0,
        const tenExperSpec *espec,
        double *dwiBuff, const double *dwiMeas,
        int knownB0) {
  double parm1[PARM_NUM], sqe0, sqe1, dp;
  unsigned int ii, i0;

  i0 = knownB0 ? 1 : 0;
  for (ii=i0; ii<PARM_NUM; ii++) {
    parm1[ii] = parm0[ii];
  }
  sqe0 = sqe(parm0, espec, dwiBuff, dwiMeas);
  for (ii=i0; ii<PARM_NUM; ii++) {
    dp = (pdesc[ii].max - pdesc[ii].min)/3000;
    parm1[ii] += dp;
    sqe1 = sqe(parm1, espec, dwiBuff, dwiMeas);
    grad[ii] = (sqe1 - sqe0)/dp;
    parm1[ii] = parm0[ii];
  }
  if (knownB0) {
    grad[ii] = 0;
  }
  return;
}

static void
parmAdd(double *parm1, const double scl,
        const double *grad, const double *parm0) {
  unsigned int ii;

  for (ii=0; ii<PARM_NUM; ii++) {
    parm1[ii] = scl*grad[ii] + parm0[ii];
    parm1[ii] = AIR_CLAMP(pdesc[ii].min, parm1[ii], pdesc[ii].max);
  }
}

static double
parmDist(const double *parmA, const double *parmB) {
  unsigned int ii;
  double dist, dd;

  dist = 0;
  for (ii=0; ii<PARM_NUM; ii++) {
    dd = (parmA[ii] - parmB[ii])/(pdesc[ii].max - pdesc[ii].min);
    dist += dd*dd;
  }
  return sqrt(dist);
}

static char *
parmSprint(char str[AIR_STRLEN_MED], double *parm) {
  sprintf(str, "(%g) [%g %g %g;  %g %g;   %g]", parm[0],
          parm[1], parm[2], parm[3],
          parm[4], parm[5],
          parm[6]);
  return str;
}

#define PARM_COPY(A, B)                         \
  {                                             \
    unsigned int pidx;                          \
    for (pidx=0; pidx<PARM_NUM; pidx++) {       \
      A[pidx] = B[pidx];                        \
    }                                           \
  }

static double
sqeFit(double *parm, double *convFrac, const tenExperSpec *espec,
       double *dwiBuff, const double *dwiMeas,
       const double *parmInit, int knownB0,
       unsigned int maxIter, double convEps) {
  static const char me[]= TEN_MODEL_STR_TENSOR2 ":sqeFit";
  unsigned int ii, i0, iter;
  double step, bak, opp, val, tval,
    dist, tparm[PARM_NUM], grad[PARM_NUM];
  char str[AIR_STRLEN_MED];

  i0 = knownB0 ? 1 : 0;
  PARM_COPY(parm, parmInit);
  val = sqe(parm, espec, dwiBuff, dwiMeas);
  sqeGrad(grad, parm, espec, dwiBuff, dwiMeas, knownB0);
  step = 0;
  for (ii=i0; ii<PARM_NUM; ii++) {
    step += (pdesc[ii].max - pdesc[ii].min)/(10*grad[ii]);
  }
  step /= (PARM_NUM - i0);
  fprintf(stderr, "!%s: at %s\n", me, parmSprint(str, parm));
  fprintf(stderr, "!%s: initial step = %g\n", me, step);

  opp = 1.5;
  bak = 0.1;
  iter = 0;
  do {
    do {
      parmAdd(tparm, -step, grad, parm);
      tval = sqe(tparm, espec, dwiBuff, dwiMeas);
      if (tval > val) {
        step *= bak;
      }
    } while (tval > val);
    fprintf(stderr, "!%s: step %g got to tval %g at %s\n", me,
            step, tval, parmSprint(str, tparm));
    dist = parmDist(tparm, parm);
    val = tval;
    PARM_COPY(parm, tparm);
    step *= opp;
    iter++;
  } while ((iter < maxIter) && (dist > convEps));
  *convFrac = dist/convEps;
  return val;
}

NLL;

static void
nllGrad(double *grad, const double *parm,
        const tenExperSpec *espec,
        double *dwiBuff, const double *dwiMeas,
        int rician, double sigma) {

  AIR_UNUSED(grad);
  AIR_UNUSED(parm);
  AIR_UNUSED(espec);
  AIR_UNUSED(dwiBuff);
  AIR_UNUSED(dwiMeas);
  AIR_UNUSED(rician);
  AIR_UNUSED(sigma);
  return;
}

static int
nllFit(double *parm, const tenExperSpec *espec,
       const double *dwiMeas, const double *parmInit,
       int rician, double sigma, int knownB0) {
  unsigned int pp;

  AIR_UNUSED(espec);
  AIR_UNUSED(dwiMeas);
  AIR_UNUSED(rician);
  AIR_UNUSED(sigma);
  AIR_UNUSED(knownB0);
  for (pp=0; pp<PARM_NUM; pp++) {
    parm[pp] = parmInit[pp];
  }
  return 0;
}

tenModel
_tenModelTensor2 = {
  TEN_MODEL_STR_TENSOR2,
  PARM_NUM,
  PARM_DESC,
  simulate,
  prand,
  sqe, sqeGrad, sqeFit,
  nll, nllGrad, nllFit
};
const tenModel *const tenModelTensor2 = &_tenModelTensor2;
