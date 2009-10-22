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

static double
sqe(double *parm, const tenExperSpec *espec,
    double *dwiBuff, const double *dwiMeas) {

  simulate(dwiBuff, parm, espec);
  return _tenModel_sqe(dwiMeas, dwiBuff, espec);
}

static int
sqeFit(double *parm, const tenExperSpec *espec,
       const double *dwiMeas, const double *parmInit,
       int knownB0) {
  unsigned int pp;

  AIR_UNUSED(espec);
  AIR_UNUSED(dwiMeas);
  AIR_UNUSED(knownB0);
  for (pp=0; pp<PARM_NUM; pp++) {
    parm[pp] = parmInit[pp];
  }
  return 0;
}

static double
nll(double *parm, const tenExperSpec *espec,
    double *dwiBuff, const double *dwiMeas,
    int rician, double sigma) {

  simulate(dwiBuff, parm, espec);
  return _tenModel_nll(dwiMeas, dwiBuff, espec, rician, sigma);
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

/* 1/sqrt(2) */
#define OST 0.70710678118654752440

tenModel
_tenModelTensor2 = {
  "tensor2",
  PARM_NUM,
  {
    {"B0", 0.0, TEN_MODEL_B0_MAX, AIR_FALSE, 0},
    {"Dxx", -TEN_MODEL_DIFF_MAX,     TEN_MODEL_DIFF_MAX,     AIR_FALSE, 0},
    {"Dxy", -TEN_MODEL_DIFF_MAX*OST, TEN_MODEL_DIFF_MAX*OST, AIR_FALSE, 0},
    {"Dxz", -TEN_MODEL_DIFF_MAX*OST, TEN_MODEL_DIFF_MAX*OST, AIR_FALSE, 0},
    {"Dyy", -TEN_MODEL_DIFF_MAX,     TEN_MODEL_DIFF_MAX,     AIR_FALSE, 0},
    {"Dyz", -TEN_MODEL_DIFF_MAX*OST, TEN_MODEL_DIFF_MAX*OST, AIR_FALSE, 0},
    {"Dzz", -TEN_MODEL_DIFF_MAX,     TEN_MODEL_DIFF_MAX,     AIR_FALSE, 0},
  },
  simulate,
  sqe,
  sqeFit,
  nll,
  nllFit
};
const tenModel *const tenModelTensor2 = &_tenModelTensor2;
