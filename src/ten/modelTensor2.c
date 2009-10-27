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

_TEN_SQE

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
  sqe0 = sqe(parm0, espec, dwiBuff, dwiMeas, knownB0);
  for (ii=i0; ii<PARM_NUM; ii++) {
    dp = (pdesc[ii].max - pdesc[ii].min)*_TEN_PARM_GRAD_EPS;
    parm1[ii] += dp;
    sqe1 = sqe(parm1, espec, dwiBuff, dwiMeas, knownB0);
    grad[ii] = (sqe1 - sqe0)/dp;
    parm1[ii] = parm0[ii];
  }
  if (knownB0) {
    grad[ii] = 0;
  }
  return;
}

static double
sqeFit(double *parm, double *convFrac, const tenExperSpec *espec,
       double *dwiBuff, const double *dwiMeas,
       const double *parmInit, int knownB0,
       unsigned int minIter, unsigned int maxIter, double convEps) {
  static const char me[]= TEN_MODEL_STR_TENSOR2 ":sqeFit";
  unsigned int i0, iter;
  double step, bak, opp, val, tval,
    dist, tparm[PARM_NUM], grad[PARM_NUM];
  char str[AIR_STRLEN_MED];
  int done;

  i0 = knownB0 ? 1 : 0;
  parmCopy(parm, parmInit);
  val = sqe(parm, espec, dwiBuff, dwiMeas, knownB0);
  sqeGrad(grad, parm, espec, dwiBuff, dwiMeas, knownB0);
  step = 10;
  fprintf(stderr, "!%s: -------- at %s\n", me, parmSprint(str, parm));
  fprintf(stderr, "!%s: initial step = %g, convEps = %g\n",
          me, step, convEps);

  opp = 1.1;
  bak = 0.1;
  iter = 0;
  do {
    do {
      parmStep(tparm, -step, grad, parm);
      tval = sqe(tparm, espec, dwiBuff, dwiMeas, knownB0);
      if (tval > val) {
        step *= bak;
        fprintf(stderr, "!%s(%u): tval %g > val %g ==> step -> %g\n", 
                me, iter, tval, val, step);
      }
    } while (tval > val);
    dist = parmDist(tparm, parm);
    fprintf(stderr, "!%s(%u): step %g (dist %g) -> %g @ %s\n", me, iter,
            step, dist, tval, parmSprint(str, tparm));
    val = tval;
    parmCopy(parm, tparm);
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
