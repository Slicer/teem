/*
  teem: Gordon Kindlmann's research software
  Copyright (C) 2002, 2001, 2000, 1999, 1998 University of Utah

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "gage.h"
#include "private.h"

char
_gageKernelStr[][AIR_STRLEN_SMALL] = {
  "(unknown_kernel)",
  "00",
  "10",
  "11",
  "20",
  "21",
  "22"
};

int
_gageKernelVal[] = {
  gageKernelUnknown,
  gageKernel00,
  gageKernel10,
  gageKernel11,
  gageKernel20,
  gageKernel21,
  gageKernel22
};

char
_gageKernelStrEqv[][AIR_STRLEN_SMALL] = {
  "00",
  "10",
  "11",
  "20",
  "21",
  "22",
  ""
};

int
_gageKernelValEqv[] = {
  gageKernel00,
  gageKernel10,
  gageKernel11,
  gageKernel20,
  gageKernel21,
  gageKernel22
};

airEnum
_gageKernel = {
  "kernel",
  GAGE_KERNEL_NUM,
  _gageKernelStr, _gageKernelVal,
  _gageKernelStrEqv, _gageKernelValEqv,
  AIR_FALSE
};
airEnum *
gageKernel = &_gageKernel;


/* ---------------------------- scl ------------------------- */

/*
  gageSclUnknown=-1,  * -1: nobody knows *
  gageSclValue,       *  0: data value: *GT *
  gageSclGradVec,     *  1: gradient vector, un-normalized: GT[3] *
  gageSclGradMag,     *  2: gradient magnitude: *GT *
  gageSclNormal,      *  3: gradient vector, normalized: GT[3] *
  gageSclHessian,     *  4: Hessian: GT[9] *
  gageSclLaplacian,   *  5: Laplacian: Dxx + Dyy + Dzz: *GT *
  gageSclHessEval,    *  6: Hessian's eigenvalues: GT[3] *
  gageSclHessEvec,    *  7: Hessian's eigenvectors: GT[9] *
  gageScl2ndDD,       *  8: 2nd dir.deriv. along gradient: *GT *
  gageSclGeomTens,    *  9: symm. matrix w/ evals 0,K1,K2 and evecs grad,
			     curvature directions: GT[9] *
  gageSclCurvedness,  * 10: L2 norm of K1, K2 (not Koen.'s "C"): *GT *
  gageSclShapeTrace,  * 11, (K1+K2)/Curvedness: *GT *
  gageSclShapeIndex,  * 12: Koen.'s shape index, ("S"): *GT *
  gageSclK1K2,        * 13: principle curvature magnitudes: GT[2] *
  gageSclCurvDir,     * 14: principle curvature directions: GT[6] *
  gageSclLast
*/

char
_gageSclStr[][AIR_STRLEN_SMALL] = {
  "(unknown gageScl)",
  "value",
  "gradient vector",
  "gradient magnitude",
  "normalized gradient",
  "Hessian",
  "Laplacian",
  "Hessian eigenvalues",
  "Hessian eigenvectors",
  "2nd DD along gradient",
  "geometry tensor",
  "curvedness",
  "shape trace",
  "shape index",
  "kappa1 kappa2",
  "curvature directions"
};

int
_gageSclVal[] = {
  gageSclUnknown,
  gageSclValue,
  gageSclGradVec,
  gageSclGradMag,
  gageSclNormal,
  gageSclHessian,
  gageSclLaplacian,
  gageSclHessEval,
  gageSclHessEvec,
  gageScl2ndDD,
  gageSclGeomTens,
  gageSclCurvedness,
  gageSclShapeTrace,
  gageSclShapeIndex,
  gageSclK1K2,
  gageSclCurvDir
};

#define GS_V  gageSclValue
#define GS_GV gageSclGradVec
#define GS_GM gageSclGradMag
#define GS_N  gageSclNormal
#define GS_H  gageSclHessian
#define GS_L  gageSclLaplacian
#define GS_HA gageSclHessEval
#define GS_HE gageSclHessEvec
#define GS_2D gageScl2ndDD
#define GS_GT gageSclGeomTens
#define GS_CV gageSclCurvedness
#define GS_ST gageSclShapeTrace
#define GS_SI gageSclShapeIndex
#define GS_KK gageSclK1K2
#define GS_CD gageSclCurvDir

char
_gageSclStrEqv[][AIR_STRLEN_SMALL] = {
  "v", "val", "value", 
  "gvec", "gradvec", "grad vec", "gradient vector",
  "g", "gm", "gmag", "gradmag", "grad mag", "gradient magnitude",
  "n", "normal", "gnorm", "normg", "norm", "normgrad", \
       "norm grad", "normalized gradient",
  "h", "hess", "hessian",
  "l", "lapl", "laplacian",
  "heval", "h eval", "hessian eval", "hessian eigenvalues",
  "hevec", "h evec", "hessian evec", "hessian eigenvectors",
  "2d", "2dd", "2nddd", "2nd", "2nd dd", "2nd dd along gradient",
  "gten", "geoten", "geomten", "geometry tensor",
  "cv", "curvedness",
  "st", "shape trace",
  "si", "shape index",
  "k1k2", "k1 k2", "kappa1kappa2", "kappa1 kappa2",
  "cdir", "c dir", "curvdir", "curv dir", "curvature directions",
  ""
};

int
_gageSclValEqv[] = {
  GS_V, GS_V, GS_V,
  GS_GV, GS_GV, GS_GV, GS_GV, 
  GS_GM, GS_GM, GS_GM, GS_GM, GS_GM, GS_GM,
  GS_N, GS_N, GS_N, GS_N, GS_N, GS_N, GS_N, GS_N,
  GS_H, GS_H, GS_H, 
  GS_L, GS_L, GS_L, 
  GS_HA, GS_HA, GS_HA, GS_HA, 
  GS_HE, GS_HE, GS_HE, GS_HE, 
  GS_2D, GS_2D, GS_2D, GS_2D, GS_2D, GS_2D,
  GS_GT, GS_GT, GS_GT, GS_GT, 
  GS_CV, GS_CV,
  GS_ST, GS_ST,
  GS_SI, GS_SI,
  GS_KK, GS_KK, GS_KK, GS_KK, 
  GS_CD, GS_CD, GS_CD, GS_CD, GS_CD
};

airEnum
_gageScl = {
  "gageScl",
  GAGE_SCL_MAX+1,
  _gageSclStr, _gageSclVal,
  _gageSclStrEqv, _gageSclValEqv,
  AIR_FALSE
};
airEnum *
gageScl = &_gageScl;

/* ---------------------------- vec ------------------------- */

char
_gageVecStr[][AIR_STRLEN_SMALL] = {
  "(unknown gageVec)",
  "vector",
  "length",
  "normalized",
  "Jacobian",
  "divergence",
  "curl"
};

int
_gageVecVal[] = {
  gageVecUnknown,
  gageVecVector,
  gageVecLength,
  gageVecNormalized,
  gageVecJacobian,
  gageVecDivergence,
  gageVecCurl,
};

#define GV_V gageVecVector
#define GV_L gageVecLength
#define GV_N gageVecNormalized
#define GV_J gageVecJacobian
#define GV_D gageVecDivergence
#define GV_C gageVecCurl

char
_gageVecStrEqv[][AIR_STRLEN_SMALL] = {
  "vector", "vec",
  "length", "len",
  "normalized", "normalized vector",
  "jacobian", "jac", "j",
  "divergence", "div", "d",
  "curl", "c"
  ""
};

int
_gageVecValEqv[] = {
  GV_V, GV_V,
  GV_L, GV_L,
  GV_N, GV_N,
  GV_J, GV_J, GV_J,
  GV_D, GV_D, GV_D,
  GV_C, GV_C
};

airEnum
_gageVec = {
  "gageVec",
  GAGE_VEC_MAX+1,
  _gageVecStr, _gageVecVal,
  _gageVecStrEqv, _gageVecValEqv,
  AIR_FALSE
};
airEnum *
gageVec = &_gageVec;
