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
  gageSclValue,           0: "v", data value: *GT   
  gageSclGradVec,         1: "grad", gradient vector, un-normalized: GT[3]   
  gageSclGradMag,         2: "gm", gradient magnitude: *GT   
  gageSclNormal,          3: "n", gradient vector, normalized: GT[3]   
  gageSclHessian,         4: "h", Hessian: GT[9] (column-order)   
  gageSclLaplacian,       5: "l", Laplacian: Dxx + Dyy + Dzz: *GT   
  gageSclHessEval,        6: "heval", Hessian's eigenvalues: GT[3]   
  gageSclHessEvec,        7: "hevec", Hessian's eigenvectors: GT[9]   
  gageScl2ndDD,           8: "2d", 2nd dir.deriv. along gradient: *GT   
  gageSclGeomTens,        9: "gten", sym. matx w/ evals 0,K1,K2 and evecs grad,
			     curvature directions: GT[9]   
  gageSclK1,             10: "k1", 1st principle curvature: *GT   
  gageSclK2,             11: "k2", 2nd principle curvature (k2 <= k1): *GT   
  gageSclCurvedness,     12: "cv", L2 norm of K1, K2 (not Koen.'s "C"): *GT   
  gageSclShapeTrace,     13, "st", (K1+K2)/Curvedness: *GT   
  gageSclShapeIndex,     14: "si", Koen.'s shape index, ("S"): *GT   
  gageSclMeanCurv,       15: "mc", mean curvature (K1 + K2)/2: *GT   
  gageSclGaussCurv,      16: "gc", gaussian curvature K1*K2: *GT   
  gageSclCurvDir,        17: "cdir", principle curvature directions: GT[6]   
  0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15  16  17
*/

/*
******** gageSclAnsLength[]
**
** the number of gage_t used for each answer
*/
int
gageSclAnsLength[GAGE_SCL_MAX+1] = {
  1,  3,  1,  3,  9,  1,  3,  9,  1,  9,  1,  1,  1,  1,  1,  1,  1,  6
};

/*
******** gageSclAnsOffset[]
**
** the index into the answer array of the first element of the answer
*/
int
gageSclAnsOffset[GAGE_SCL_MAX+1] = {
  0,  1,  4,  5,  8, 17, 18, 21, 30, 31, 40, 41, 42, 43, 44, 45, 46, 47 /*53*/
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
  /* 0: gageSclValue */
  0,

  /* 1: gageSclGradVec */
  0,

  /* 2: gageSclGradMag */
  (1<<gageSclGradVec),

  /* 3: gageSclNormal */
  (1<<gageSclGradVec) | (1<<gageSclGradMag),

  /* 4: gageSclHessian */
  0,

  /* 5: gageSclLaplacian */
  (1<<gageSclHessian),   /* not really true, but this is simpler */

  /* 6: gageSclHessEval */
  (1<<gageSclHessian),

  /* 7: gageSclHessEvec */
  (1<<gageSclHessian) | (1<<gageSclHessEval),

  /* 8: gageScl2ndDD */
  (1<<gageSclHessian) | (1<<gageSclNormal),

  /* 9: gageSclGeomTens */
  (1<<gageSclHessian) | (1<<gageSclNormal) | (1<<gageSclGradMag),
  
  /* 10: gageSclK1 */
  (1<<gageSclCurvedness) | (1<<gageSclShapeTrace),

  /* 11: gageSclK2 */
  (1<<gageSclCurvedness) | (1<<gageSclShapeTrace),

  /* 12: gageSclCurvedness */
  (1<<gageSclGeomTens),

  /* 13: gageSclShapeTrace */
  (1<<gageSclGeomTens),

  /* 14: gageSclShapeIndex */
  (1<<gageSclK1) | (1<<gageSclK2),

  /* 15: gageSclMeanCurv */
  (1<<gageSclK1) | (1<<gageSclK2),

  /* 16: gageSclGaussCurv */
  (1<<gageSclK1) | (1<<gageSclK2),

  /* 17: gageSclCurvDir */
  (1<<gageSclGeomTens) | (1<<gageSclK1) | (1<<gageSclK2),
  
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
  "kappa1",
  "kappa2",
  "curvedness",
  "shape trace",
  "shape index",
  "mean curvature",
  "Gaussian curvature",
  "curvature directions"
};

char
_gageSclDesc[][AIR_STRLEN_MED] = {
  "unknown gageScl query",
  "reconstructed scalar data value",
  "gradient vector, un-normalized",
  "gradient magnitude (length of gradient vector)",
  "normalized gradient vector",
  "3x3 Hessian matrix",
  "Laplacian",
  "Hessian's eigenvalues",
  "Hessian's eigenvectors",
  "2nd directional derivative along gradient",
  "geometry tensor",
  "1st principal curvature (K1)",
  "2nd principal curvature (K2)",
  "curvedness (L2 norm of K1, K2)",
  "shape trace = (K1+K2)/curvedness",
  "Koenderink's shape index",
  "mean curvature = (K1+K2)/2",
  "gaussian curvature = K1*K2",
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
  gageSclK1,
  gageSclK2,
  gageSclCurvedness,
  gageSclShapeTrace,
  gageSclShapeIndex,
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
#define GS_K1 gageSclK1
#define GS_K2 gageSclK2
#define GS_CV gageSclCurvedness
#define GS_ST gageSclShapeTrace
#define GS_SI gageSclShapeIndex
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
  "k1", "kap1", "kappa1",
  "k2", "kap2", "kappa2",
  "cv", "curvedness",
  "st", "shape trace",
  "si", "shape index",
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
  GS_K1, GS_K1, GS_K1,
  GS_K2, GS_K2, GS_K2,
  GS_CV, GS_CV,
  GS_ST, GS_ST,
  GS_SI, GS_SI,
  GS_MC, GS_MC, GS_MC, GS_MC,
  GS_GC, GS_GC, GS_GC, GS_GC,
  GS_CD, GS_CD, GS_CD, GS_CD, GS_CD
};

airEnum
_gageScl = {
  "gageScl",
  GAGE_SCL_MAX+1,
  _gageSclStr, _gageSclVal,
  _gageSclDesc,
  _gageSclStrEqv, _gageSclValEqv,
  AIR_FALSE
};
airEnum *
gageScl = &_gageScl;

/*
** _gageSclIv3Fill()
**
** based on ctx's shape and havePad, and the (xi,yi,zi) determined from
** the probe location, fills the iv3 cache in the given pervolume
*/
void
_gageSclIv3Fill (gageContext *ctx, gagePerVolume *pvl) {
  char me[]="_gageSclIv3Fill";
  int i, sx, sy, sz, fd, bidx, diff;
  void *here;

  PADSIZE(sx, sy, sz, ctx);
  fd = GAGE_FD(ctx);
  diff = -ctx->havePad + (nrrdCenterCell == ctx->shape.center);
  diff = -ctx->havePad;
  bidx = (ctx->point.xi + diff
	  + sx*(ctx->point.yi + diff
		+ sy*(ctx->point.zi + diff)));
  if (ctx->verbose) {
    fprintf(stderr, "%s: padded size = (%d,%d,%d);\n"
	    "  fd = %d; point (padded) coord = (%d,%d,%d) --> bidx = %d\n", me,
	    sx, sy, sz, fd, ctx->point.xi, ctx->point.yi, ctx->point.zi, bidx);
  }
  here = ((char*)(pvl->npad->data) + (bidx * pvl->kind->valLen * 
				      nrrdTypeSize[pvl->npad->type]));
  for (i=0; i<fd*fd*fd; i++)
    pvl->iv3[i] = pvl->lup(here, ctx->off[i]);

  return;
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
  _gageSclIv3Fill,
  _gageSclIv3Print,
  _gageSclFilter,
  _gageSclAnswer
};
gageKind *
gageKindScl = &_gageKindScl;

