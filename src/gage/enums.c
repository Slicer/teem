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
  "kappa1 kappa2",
  "curvature directions",
  "shape index",
  "curvedness"
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
  gageSclK1K2,
  gageSclCurvDir,
  gageSclShapeIndex,
  gageSclCurvedness
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
#define GS_KK gageSclK1K2
#define GS_CD gageSclCurvDir
#define GS_SI gageSclShapeIndex
#define GS_CV gageSclCurvedness

char
_gageSclStrEqv[][AIR_STRLEN_SMALL] = {
  "v", "val", "value", 
  "gvec", "gradvec", "grad vec", "gradient vector",
  "g", "gmag", "gradmag", "grad mag", "gradient magnitude",
  "n", "gnorm", "normg", "norm", "normgrad", "norm grad","normalized gradient",
  "h", "hess", "hessian",
  "l", "lapl", "laplacian",
  "heval", "h eval", "hessian eval", "hessian eigenvalues",
  "hevec", "h evec", "hessian evec", "hessian eigenvectors",
  "2d", "2dd", "2nddd", "2nd dd", "2nd dd along gradient",
  "gten", "geoten", "geomten", "geometry tensor",
  "k1k2", "k1 k2", "kappa1kappa2", "kappa1 kappa2",
  "cdir", "c dir", "curvdir", "curv dir", "curvature directions",
  "si", "shape index",
  "cv", "curvedness"
};

int
_gageSclValEqv[] = {
  GS_V, GS_V, GS_V,
  GS_GV, GS_GV, GS_GV, GS_GV, 
  GS_GM, GS_GM, GS_GM, GS_GM, GS_GM, 
  GS_N, GS_N, GS_N, GS_N, GS_N, GS_N, GS_N,
  GS_H, GS_H, GS_H, 
  GS_L, GS_L, GS_L, 
  GS_HA, GS_HA, GS_HA, GS_HA, 
  GS_HE, GS_HE, GS_HE, GS_HE, 
  GS_2D, GS_2D, GS_2D, GS_2D, GS_2D,
  GS_GT, GS_GT, GS_GT, GS_GT, 
  GS_KK, GS_KK, GS_KK, GS_KK, 
  GS_CD, GS_CD, GS_CD, GS_CD, GS_CD, 
  GS_SI, GS_SI,
  GS_CV, GS_CV
};

airEnum
gageScl = {
  "gageScl",
  GAGE_SCL_MAX+1,
  _gageSclStr, _gageSclVal,
  _gageSclStrEqv, _gageSclValEqv,
  AIR_FALSE
};

