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

#include "mite.h"

void
_miteRGBACalc(double *rr, double *gg, double *bb, double *aa,
	      miteThreadInfo *mtt, miteRenderInfo *mrr, miteUserInfo *muu,
	      double samplePosWorld[3], double samplePosIndex[3]) {
  double val, lr, lg, lb;
  int tfi;
  float *tf, min, max, dot;

  dot = ELL_3V_DOT(mrr->san->norm, muu->lit->dir[0]);
  dot = AIR_ABS(dot);
  lr = muu->lit->amb[0] + 0.0*dot;
  lg = muu->lit->amb[1] + 0.0*dot;
  lb = muu->lit->amb[2] + 0.0*dot;

  min = muu->ntf->axis[1].min;
  max = muu->ntf->axis[1].max;
  val = AIR_CLAMP(min, mrr->san->val[0], max);
  tf = muu->ntf->data;
  AIR_INDEX(min, val, max, muu->ntf->axis[1].size, tfi);
  
  *rr = lr*tf[0 + 4*tfi];
  *gg = lg*tf[1 + 4*tfi];
  *bb = lb*tf[2 + 4*tfi];
  *aa = tf[3 + 4*tfi];
  *rr = AIR_CLAMP(0, *rr, 1);
  *gg = AIR_CLAMP(0, *gg, 1);
  *bb = AIR_CLAMP(0, *bb, 1);
  *aa = AIR_CLAMP(0, *aa, 1);

  return;
}

int
miteRayBegin(miteThreadInfo *mtt, miteRenderInfo *mrr, miteUserInfo *muu,
	     int uIndex, int vIndex, 
	     double rayLen,
	     double rayStartWorld[3], double rayStartIndex[3],
	     double rayDirWorld[3], double rayDirIndex[3]) {

  mtt->ui = uIndex;
  mtt->vi = vIndex;
  if (!uIndex) {
    fprintf(stderr, "%d/%d ", vIndex, muu->ctx->imgSize[1]);
    fflush(stderr);
  }

  mtt->R = mtt->G = mtt->B = mtt->A = 0;

  return 0;
}

double
miteSample(miteThreadInfo *mtt, miteRenderInfo *mrr, miteUserInfo *muu,
	   int num, double rayT, int inside,
	   double samplePosWorld[3],
	   double samplePosIndex[3]) {
  char me[]="miteSample", err[AIR_STRLEN_MED];
  double rr, gg, bb, aa, T;

  if (!inside)
    return muu->rayStep;

  if (mtt->A >= muu->near1) {
    /* early ray termination */
    return 0.0;
  }

  if (gageProbe(mrr->gtx,
		samplePosIndex[0],
		samplePosIndex[1],
		samplePosIndex[2])) {
    sprintf(err, "%s: gage trouble: %s (%d)", me, gageErrStr, gageErrNum);
    biffAdd(MITE, err);
    return AIR_NAN;
  }

  _miteRGBACalc(&rr, &gg, &bb, &aa, mtt, mrr, muu,
		samplePosWorld, samplePosIndex);
  if (muu->sum) {
    /*
    mtt->R += aa*rr;
    mtt->G += aa*gg;
    mtt->B += aa*bb;
    mtt->A += aa;
    */
    mtt->R = AIR_MAX(mtt->R, aa*rr);
    mtt->G = AIR_MAX(mtt->G, aa*gg);
    mtt->B = AIR_MAX(mtt->B, aa*bb);
    mtt->A = AIR_MAX(mtt->A, aa);
  } else {
    T = 1 - mtt->A;
    mtt->R = mtt->A*mtt->R + T*aa*rr;
    mtt->G = mtt->A*mtt->G + T*aa*gg;
    mtt->B = mtt->A*mtt->B + T*aa*bb;
    mtt->A = 1 - T*(1-aa);
  }

  return muu->rayStep;
}

int 
miteRayEnd(miteThreadInfo *mtt, miteRenderInfo *mrr, miteUserInfo *muu) {
  
  ELL_4V_SET(mrr->imgData + 4*((mtt->ui) + (mrr->sx)*(mtt->vi)),
	     mtt->R, mtt->G, mtt->B, mtt->A);
  return 0;
}
