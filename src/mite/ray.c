/*
  teem: Gordon Kindlmann's research software
  Copyright (C) 2004, 2003, 2002, 2001, 2000, 1999, 1998 University of Utah

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
#include "privateMite.h"

int
miteRayBegin(miteThread *mtt, miteRender *mrr, miteUser *muu,
	     int uIndex, int vIndex, 
	     double rayLen,
	     double rayStartWorld[3], double rayStartIndex[3],
	     double rayDirWorld[3], double rayDirIndex[3]) {

  mtt->ui = uIndex;
  mtt->vi = vIndex;
  mtt->rayStep = (muu->rayStep*rayLen /
		  (muu->hctx->cam->vspFaar - muu->hctx->cam->vspNeer));
  if (!uIndex) {
    fprintf(stderr, "%d/%d ", vIndex, muu->hctx->imgSize[1]);
    fflush(stderr);
  }
  mtt->verbose = (uIndex == muu->verbUi && vIndex == muu->verbVi);
  mtt->RR = mtt->GG = mtt->BB = 0.0;
  mtt->TT = 1.0;
  ELL_3V_SCALE(mtt->V, -1, rayDirWorld);

  return 0;
}

void
_miteRGBACalc(mite_t *R, mite_t *G, mite_t *B, mite_t *A,
	      miteThread *mtt, miteRender *mrr, miteUser *muu) {
  char me[]="_miteRGBACalc";
  mite_t tmp,
    ad[3],                          /* ambient+diffuse light contribution */
    s[3] = {0,0,0},                 /* specular light contribution */
    col[3], E, ka, kd, ks, sp,      /* txf-determined rendering variables */
    LdotN=0, HdotN, H[3], N[3];     /* for lighting calculation */

  col[0] = mtt->range[miteRangeRed];
  col[1] = mtt->range[miteRangeGreen];
  col[2] = mtt->range[miteRangeBlue];
  E = mtt->range[miteRangeEmissivity];
  ka = mtt->range[miteRangeKa];
  kd = mtt->range[miteRangeKd];
  ks = mtt->range[miteRangeKs];
  ELL_3V_SCALE(ad, ka, muu->lit->amb);
  switch (mrr->shpec->method) {
  case miteShadeMethodNone:
    /* nothing to do */
    break;
  case miteShadeMethodPhong:
    /* this is dicey- we're *writing* to the answer buffer */
    if (kd || ks) {
      ELL_3V_NORM(mtt->shadeVec0, mtt->shadeVec0, tmp);
      if (muu->normalSide) {
	ELL_3V_SCALE(N, -muu->normalSide, mtt->shadeVec0);
      } else {
	ELL_3V_COPY(N, mtt->shadeVec0);
      }
      if (kd) {
	LdotN = ELL_3V_DOT(muu->lit->dir[0], N);
	if (!muu->normalSide) {
	  LdotN = AIR_ABS(LdotN);
	}
	if (LdotN > 0) {
	  ELL_3V_SCALE_INCR(ad, LdotN*kd, muu->lit->col[0]);
	}
      }
      if (ks) {
	sp = mtt->range[miteRangeSP];
	ELL_3V_ADD2(H, muu->lit->dir[0], mtt->V);
	ELL_3V_NORM(H, H, tmp);
	HdotN = ELL_3V_DOT(H, N);
	if (!muu->normalSide) {
	  HdotN = AIR_ABS(HdotN);
	}
	if (HdotN > 0) {
	  HdotN = pow(HdotN, sp);
	  ELL_3V_SCALE(s, HdotN*ks, muu->lit->col[0]);
	}
      }
    }
    break;
  case miteShadeMethodLitTen:
    fprintf(stderr, "!%s: lit-tensor not yet implemented\n", me);
    break;
  default:
    fprintf(stderr, "!%s: PANIC, shadeMethod %d unimplemented\n", 
	    me, mrr->shpec->method);
    exit(1);
    break;
  }
  *R = (E - 1 + ad[0])*col[0] + s[0];
  *G = (E - 1 + ad[1])*col[1] + s[1];
  *B = (E - 1 + ad[2])*col[2] + s[2];
  *A = mtt->range[miteRangeAlpha];
  *A = AIR_CLAMP(0.0, *A, 1.0);
  if (mtt->verbose) {
    fprintf(stderr, "%s: col[] = %g,%g,%g; A,E = %g,%g; Kads = %g,%g,%g\n", me,
	    col[0], col[1], col[2], mtt->range[miteRangeAlpha], E, ka, kd, ks);
    fprintf(stderr, "%s: N = (%g,%g,%g), L = (%g,%g,%g) ---> LdotN = %g\n",
	    me, N[0], N[1], N[2], muu->lit->dir[0][0], muu->lit->dir[0][1],
	    muu->lit->dir[0][2], LdotN);
    fprintf(stderr, "%s: ad[] = %g,%g,%g\n", me, ad[0], ad[1], ad[2]);
    fprintf(stderr, "%s:  --> R,G,B,A = %g,%g,%g,%g\n", me, *R, *G, *B, *A);
  }
  return;
}

double
miteSample(miteThread *mtt, miteRender *mrr, miteUser *muu,
	   int num, double rayT, int inside,
	   double samplePosWorld[3],
	   double samplePosIndex[3]) {
  char me[]="miteSample", err[AIR_STRLEN_MED];
  mite_t R, G, B, A;
  double kn[3], knd[3], len;

  if (!inside) {
    return mtt->rayStep;
  }

  if (1-mtt->TT >= muu->opacNear1) {
    /* early ray termination */
    mtt->TT = 0.0;
    return 0.0;
  }
  mtt->samples += 1;
  if (gageProbe(mtt->gctx,
		samplePosIndex[0], samplePosIndex[1], samplePosIndex[2])) {
    sprintf(err, "%s: gage trouble: %s (%d)", me, gageErrStr, gageErrNum);
    biffAdd(MITE, err); return AIR_NAN;
  }
  /* HEY: NONE of this should be done if the txfs don't need any miteVal */
  mtt->ansMiteVal[miteValXw] = samplePosWorld[0];
  mtt->ansMiteVal[miteValXi] = samplePosIndex[0];
  mtt->ansMiteVal[miteValYw] = samplePosWorld[1];
  mtt->ansMiteVal[miteValYi] = samplePosIndex[1];
  mtt->ansMiteVal[miteValZw] = samplePosWorld[2];
  mtt->ansMiteVal[miteValZi] = samplePosIndex[2];
  mtt->ansMiteVal[miteValTw] = rayT;
  mtt->ansMiteVal[miteValTi] = num;
#if 0

  what if shading is none?

  mtt->ansMiteVal[miteValNdotV] =
    -muu->normalSide*ELL_3V_DOT(mtt->V, mtt->shadeVec0);
  mtt->ansMiteVal[miteValNdotL] =
    -muu->normalSide*ELL_3V_DOT(mtt->shadeVec0, muu->lit->dir[0]);
#endif
  if (!muu->normalSide) {
    mtt->ansMiteVal[miteValNdotV] = AIR_ABS(mtt->ansMiteVal[miteValNdotV]);
    mtt->ansMiteVal[miteValNdotL] = AIR_ABS(mtt->ansMiteVal[miteValNdotL]);
  }

#if 0
  this is bullshit

  ELL_3MV_MUL(kn, mtt->nPerp, mtt->V);
  ELL_3V_NORM(kn, kn, len);
  ELL_3MV_MUL(knd, mtt->gten, kn);
  mtt->ansMiteVal[miteValGTdotV] = ELL_3V_DOT(knd, kn);
#endif
  
  memcpy(mtt->range, muu->rangeInit, MITE_RANGE_NUM*sizeof(mite_t));
  _miteStageRun(mtt);
  if (mtt->range[miteRangeAlpha]) {
    /* fprintf(stderr, "%s: mtt->TT = %g\n", me, mtt->TT); */
    if (mtt->verbose) {
      fprintf(stderr, "%s: before compositing: RGBT = %g,%g,%g,%g\n",
	      me, mtt->RR, mtt->GG, mtt->BB, mtt->TT);
    }
    _miteRGBACalc(&R, &G, &B, &A, mtt, mrr, muu);
    mtt->RR += mtt->TT*A*R;
    mtt->GG += mtt->TT*A*G;
    mtt->BB += mtt->TT*A*B;
    mtt->TT *= 1-A;
    if (mtt->verbose) {
      fprintf(stderr, "%s: after compositing: RGBT = %g,%g,%g,%g\n",
	      me, mtt->RR, mtt->GG, mtt->BB, mtt->TT);
    }
    /* fprintf(stderr, "%s: mtt->TT = %g\n", me, mtt->TT); */
  }

  return mtt->rayStep;
}

int 
miteRayEnd(miteThread *mtt, miteRender *mrr, miteUser *muu) {
  int idx;
  mite_t *imgData, A;
  
  idx = mtt->ui + (muu->nout->axis[1].size)*mtt->vi;
  imgData = (mite_t*)muu->nout->data;
  A = 1 - mtt->TT;
  if (A) {
    ELL_4V_SET(imgData + 5*idx, mtt->RR/A, mtt->GG/A, mtt->BB/A, A);
  } else {
    ELL_4V_SET(imgData + 5*idx, 0, 0, 0, 0);
  }
  return 0;
}

