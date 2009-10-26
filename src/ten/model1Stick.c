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

#define PARM_NUM 5
#define PARM_DESC \
  {                                                           \
    {"B0", 0.0, TEN_MODEL_B0_MAX, AIR_FALSE, 0},              \
    {"diffusivity", 0.0, TEN_MODEL_DIFF_MAX, AIR_FALSE, 0},   \
    {"x", -1.0, 1.0, AIR_TRUE, 0},                            \
    {"y", -1.0, 1.0, AIR_TRUE, 1},                            \
    {"z", -1.0, 1.0, AIR_TRUE, 2},                            \
  }
static const tenModelParmDesc
const pdesc[TEN_MODEL_PARM_MAXNUM] = PARM_DESC;

static void 
simulate(double *dwiSim, const double *parm, const tenExperSpec *espec) {
  unsigned int ii;
  double b0, diff, vec[3];

  b0 = parm[0];
  diff = parm[1];
  vec[0] = parm[2];
  vec[1] = parm[3];
  vec[2] = parm[4];
  for (ii=0; ii<espec->imgNum; ii++) {
    double dot;
    dot = ELL_3V_DOT(vec, espec->grad + 3*ii);
    dwiSim[ii] = b0*exp(-espec->bval[ii]*diff*dot*dot);
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
      /* bump ii by 2; next for loop will increment past */
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
sqeGrad(double *grad, const double *parm,
        const tenExperSpec *espec,
        double *dwiBuff, const double *dwiMeas) {
  
  AIR_UNUSED(grad);
  AIR_UNUSED(parm);
  AIR_UNUSED(espec);
  AIR_UNUSED(dwiBuff);
  AIR_UNUSED(dwiMeas);
  return;
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
_tenModel1Stick = {
  TEN_MODEL_STR_1STICK,
  PARM_NUM,
  PARM_DESC,
  simulate,
  prand,
  sqe, sqeGrad, sqeFit,
  nll, nllGrad, nllFit
};
const tenModel *const tenModel1Stick = &_tenModel1Stick;
