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
#include "privateGage.h"

/*
  gageSclUnknown=-1,   -1: nobody knows 
  gageSclValue,         0: data value: *GT 
  gageSclGradVec,       1: gradient vector, un-normalized: GT[3] 
  gageSclGradMag,       2: gradient magnitude: *GT 
  gageSclNormal,        3: gradient vector, normalized: GT[3] 
  gageSclHessian,       4: Hessian: GT[9] (column-order) 
  gageSclLaplacian,     5: Laplacian: Dxx + Dyy + Dzz: *GT 
  gageSclHessEval,      6: Hessian's eigenvalues: GT[3] 
  gageSclHessEvec,      7: Hessian's eigenvectors: GT[9] 
  gageScl2ndDD,         8: 2nd dir.deriv. along gradient: *GT 
  gageSclGeomTens,      9: symm. matrix w/ evals 0,K1,K2 and evecs grad,
			     curvature directions: GT[9] 
  gageSclCurvedness,   10: L2 norm of K1, K2 (not Koen.'s "C"): *GT 
  gageSclShapeTrace,   11, (K1+K2)/Curvedness: *GT 
  gageSclShapeIndex,   12: Koen.'s shape index, ("S"): *GT 
  gageSclK1K2,         13: principle curvature magnitudes: GT[2] 
  gageSclMeanCurve,    14: mean curvatuve (K1 + K2)/2: *GT 
  gageSclGaussCurv,    15: gaussian curvature K1*K2: *GT 
  gageSclCurvDir,      16: principle curvature directions: GT[6] 
  0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15  16
*/

/*
******** gageSclAnsLength[]
**
** the number of gage_t used for each answer
*/
int
gageSclAnsLength[GAGE_SCL_MAX+1] = {
  1,  3,  1,  3,  9,  1,  3,  9,  1,  9,  1,  1,  1,  2,  1,  1,  6
};

/*
******** gageSclAnsOffset[]
**
** the index into the answer array of the first element of the answer
*/
int
gageSclAnsOffset[GAGE_SCL_MAX+1] = {
  0,  1,  4,  5,  8, 17, 18, 21, 30, 31, 40, 41, 42, 43, 45, 46, 47 /* 53 */
};

/*
** _gageSclNeedDeriv[]
**
** each value is a BIT FLAG representing the different value/derivatives
** that are needed to calculate the quantity.  
*/
int
_gageSclNeedDeriv[GAGE_SCL_MAX+1] = {
  1,  2,  2,  2,  4,  4,  4,  4,  6,  6,  6,  6,  6,  6,  6,  6,  6
};

/*
** _gageSclPrereq[]
** 
** this records the measurements which are needed as ingredients for any
** given measurement, but it is not necessarily the recursive expansion of
** that requirement (that role is performed by gageSclSetQuery())
*/
unsigned int
_gageSclPrereq[GAGE_SCL_MAX+1] = {
  /* gageSclValue */
  0,

  /* gageSclGradVec */
  0,

  /* gageSclGradMag */
  (1<<gageSclGradVec),

  /* gageSclNormal */
  (1<<gageSclGradVec) | (1<<gageSclGradMag),

  /* gageSclHessian */
  0,

  /* gageSclLaplacian */
  (1<<gageSclHessian),   /* not really true, but this is simpler */

  /* gageSclHessEval */
  (1<<gageSclHessian),

  /* gageSclHessEvec */
  (1<<gageSclHessian) | (1<<gageSclHessEval),

  /* gageScl2ndDD */
  (1<<gageSclHessian) | (1<<gageSclNormal),

  /* gageSclGeomTens */
  (1<<gageSclHessian) | (1<<gageSclNormal) | (1<<gageSclGradMag),
  
  /* gageSclCurvedness */
  (1<<gageSclGeomTens),

  /* gageSclShapeTrace */
  (1<<gageSclGeomTens),

  /* gageSclShapeIndex */
  (1<<gageSclK1K2),

  /* gageSclK1K2 */
  (1<<gageSclCurvedness) | (1<<gageSclShapeTrace),

  /* gageSclMeanCurv */
  (1<<gageSclK1K2),

  /* gageSclGaussCurv */
  (1<<gageSclK1K2),

  /* gageSclCurvDir */
  (1<<gageSclGeomTens) | (1<<gageSclK1K2)
  
};

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
  "mean curvature",
  "Gaussian curvature",
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
  gageSclMeanCurv,
  gageSclGaussCurv,
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
#define GS_MC gageSclMeanCurv
#define GS_GC gageSclGaussCurv
#define GS_CD gageSclCurvDir

char
_gageSclStrEqv[][AIR_STRLEN_SMALL] = {
  "v", "val", "value", 
  "grad", "gvec", "gradvec", "grad vec", "gradient vector",
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
  "mc", "mcurv", "meancurv", "mean curvature",
  "gc", "gcurv", "gausscurv", "gaussian curvature",
  "cdir", "c dir", "curvdir", "curv dir", "curvature directions",
  ""
};

int
_gageSclValEqv[] = {
  GS_V, GS_V, GS_V,
  GS_GV, GS_GV, GS_GV, GS_GV, GS_GV, 
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
  GS_MC, GS_MC, GS_MC, GS_MC,
  GS_GC, GS_GC, GS_GC, GS_GC,
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

gageSclAnswer *
_gageSclAnswerNew() {
  gageSclAnswer *san;
  int i;

  san = (gageSclAnswer *)calloc(1, sizeof(gageSclAnswer));
  if (san) {
    for (i=0; i<GAGE_SCL_TOTAL_ANS_LENGTH; i++)
      san->ans[i] = AIR_NAN;
    san->val   = &(san->ans[gageSclAnsOffset[gageSclValue]]);
    san->gvec  = &(san->ans[gageSclAnsOffset[gageSclGradVec]]);
    san->gmag  = &(san->ans[gageSclAnsOffset[gageSclGradMag]]);
    san->norm  = &(san->ans[gageSclAnsOffset[gageSclNormal]]);
    san->hess  = &(san->ans[gageSclAnsOffset[gageSclHessian]]);
    san->lapl  = &(san->ans[gageSclAnsOffset[gageSclLaplacian]]);
    san->heval = &(san->ans[gageSclAnsOffset[gageSclHessEval]]);
    san->hevec = &(san->ans[gageSclAnsOffset[gageSclHessEvec]]);
    san->scnd  = &(san->ans[gageSclAnsOffset[gageScl2ndDD]]);
    san->gten  = &(san->ans[gageSclAnsOffset[gageSclGeomTens]]);
    san->C     = &(san->ans[gageSclAnsOffset[gageSclCurvedness]]);
    san->St    = &(san->ans[gageSclAnsOffset[gageSclShapeTrace]]);
    san->Si    = &(san->ans[gageSclAnsOffset[gageSclShapeIndex]]);
    san->k1k2  = &(san->ans[gageSclAnsOffset[gageSclK1K2]]);
    san->mc    = &(san->ans[gageSclAnsOffset[gageSclMeanCurv]]);
    san->gc    = &(san->ans[gageSclAnsOffset[gageSclGaussCurv]]);
    san->cdir  = &(san->ans[gageSclAnsOffset[gageSclCurvDir]]);
  }
  return san;
}

gageSclAnswer *
_gageSclAnswerNix(gageSclAnswer *san) {

  return airFree(san);
}

gageKind
_gageKindScl = {
  "scalar",
  &_gageScl,
  0,
  1,
  GAGE_SCL_MAX,
  gageSclAnsLength,
  gageSclAnsOffset,
  GAGE_SCL_TOTAL_ANS_LENGTH,
  _gageSclNeedDeriv,
  _gageSclPrereq,
  _gageSclPrint_query,
  (void *(*)(void))_gageSclAnswerNew,
  (void *(*)(void*))_gageSclAnswerNix,
  _gageSclIv3Fill,
  _gageSclIv3Print,
  _gageSclFilter,
  _gageSclAnswer
};
gageKind *
gageKindScl = &_gageKindScl;

