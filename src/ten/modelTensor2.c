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
static const tenModelParmDesc
const parmDesc[] = {
  {"B0", 0.0, TEN_MODEL_B0_MAX, AIR_FALSE, 0},
  {"Dxx", -TEN_MODEL_DIFF_MAX,     TEN_MODEL_DIFF_MAX,     AIR_FALSE, 0},
  {"Dxy", -TEN_MODEL_DIFF_MAX*OST, TEN_MODEL_DIFF_MAX*OST, AIR_FALSE, 0},
  {"Dxz", -TEN_MODEL_DIFF_MAX*OST, TEN_MODEL_DIFF_MAX*OST, AIR_FALSE, 0},
  {"Dyy", -TEN_MODEL_DIFF_MAX,     TEN_MODEL_DIFF_MAX,     AIR_FALSE, 0},
  {"Dyz", -TEN_MODEL_DIFF_MAX*OST, TEN_MODEL_DIFF_MAX*OST, AIR_FALSE, 0},
  {"Dzz", -TEN_MODEL_DIFF_MAX,     TEN_MODEL_DIFF_MAX,     AIR_FALSE, 0}
};

static void 
simulate(double *dwiSim, const double *parm, const tenExperSpec *espec) {
  unsigned int ii;
  double b0;

  b0 = parm[0];
  for (ii=0; ii<espec->imgNum; ii++) {
    double adc, bb;
    bb = espec->bval[ii];
    /* safe because TEN_T3V_CONTR never looks at parm[0] */
    adc = TEN_T3V_CONTR(parm, espec->grad + 3*ii);
    dwiSim[ii] = b0*exp(-bb*adc);
  }
  return;
}

static char *
parmSprint(char str[AIR_STRLEN_MED], const double *parm) {
  sprintf(str, "(%g) [%g %g %g;  %g %g;   %g]", parm[0],
          parm[1], parm[2], parm[3],
          parm[4], parm[5],
          parm[6]);
  return str;
}

_TEN_PARM_RAND
_TEN_PARM_STEP
_TEN_PARM_DIST
_TEN_PARM_COPY

static double
sqe(const double *parm, const tenExperSpec *espec,
    double *dwiBuff, const double *dwiMeas, int knownB0) {

  simulate(dwiBuff, parm, espec);
  return _tenExperSpec_sqe(dwiBuff, dwiMeas, espec, knownB0);
}

static void
sqeGrad(double *grad, const double *parm0,
        const tenExperSpec *espec,
        double *dwiBuff, const double *dwiMeas,
        int knownB0) {
  double parm1[PARM_NUM], sqeForw, sqeBack, dp;
  unsigned int ii, i0;

  i0 = knownB0 ? 1 : 0;
  for (ii=0; ii<PARM_NUM; ii++) {
    /* have to copy all parms, even B0 with knownB0, and even if we
       aren't going to diddle these values, because the
       simulation depends on knowing the values */
    parm1[ii] = parm0[ii];
  }
  for (ii=i0; ii<PARM_NUM; ii++) {
    dp = (parmDesc[ii].max - parmDesc[ii].min)*TEN_MODEL_PARM_GRAD_EPS;
    parm1[ii] = parm0[ii] + dp;
    sqeForw = sqe(parm1, espec, dwiBuff, dwiMeas, knownB0);
    parm1[ii] = parm0[ii] - dp;
    sqeBack = sqe(parm1, espec, dwiBuff, dwiMeas, knownB0);
    grad[ii] = (sqeForw - sqeBack)/(2*dp);
    parm1[ii] = parm0[ii];
  }
  if (knownB0) {
    grad[0] = 0;
  }
  return;
}

static double
sqeFit(double *parm, double *convFrac, const tenExperSpec *espec,
       double *dwiBuff, const double *dwiMeas,
       const double *parmInit, int knownB0,
       unsigned int minIter, unsigned int maxIter, double convEps) {
  /* static const char me[]= TEN_MODEL_STR_TENSOR2 ":sqeFit"; */
  unsigned int iter;
  double step, bak, opp, val, testval,
    dist, testparm[PARM_NUM], grad[PARM_NUM];
  int done;

  step = 1;
  parmCopy(parm, parmInit);
  val = sqe(parm, espec, dwiBuff, dwiMeas, knownB0);
  sqeGrad(grad, parm, espec, dwiBuff, dwiMeas, knownB0);

  opp = 2;
  bak = 0.1;
  iter = 0;
  do {
    do {
      parmStep(testparm, -step, grad, parm);
      testval = sqe(testparm, espec, dwiBuff, dwiMeas, knownB0);
      if (testval > val) {
        step *= bak;
      }
    } while (testval > val);
    dist = parmDist(testparm, parm);
    val = testval;
    parmCopy(parm, testparm);
    sqeGrad(grad, parm, espec, dwiBuff, dwiMeas, knownB0);
    step *= opp;
    iter++;
    done = (iter < minIter
            ? AIR_FALSE
            : (iter > maxIter) || dist < convEps);
  } while (!done);
  *convFrac = dist/convEps;
  return val;
}

_TEN_NLL
_TEN_NLL_GRAD_STUB
_TEN_NLL_FIT_STUB


tenModel
_tenModelTensor2 = {
  TEN_MODEL_STR_TENSOR2,
  _TEN_MODEL_FIELDS
};
const tenModel *const tenModelTensor2 = &_tenModelTensor2;
