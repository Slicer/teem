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

void
_gageSclAnswer(gageSclContext *sctx) {
  char me[]="_gageSclAnswer";
  unsigned int query;
  gage_t len, tmpMat[9], tmpVec[3], sHess[9],
    gp1[3], gp2[3], ZPP[9], 
    ginv, T, N, D, K1, K2;
  double m[9], hevec[9], heval[3];

  query = sctx->query;
  if (1 & (query >> gageSclValue)) {
    /* done if doV */
    if (sctx->c.verbose) {
      printf("val = % 15.7f", (float)(sctx->val[0]));
    }
  }
  if (1 & (query >> gageSclGradVec)) {
    /* done if doD1 */
    if (sctx->c.verbose) {
      printf("gvec = ");
      ell3vPRINT(stdout, sctx->gvec);
    }
  }
  if (1 & (query >> gageSclGradMag)) {
    sctx->gmag[0] = sqrt(ELL_3V_DOT(sctx->gvec, sctx->gvec)
			 + sctx->epsilon*sctx->epsilon);
  }
  if (1 & (query >> gageSclNormal)) {
    ELL_3V_SCALE(sctx->norm, 1.0/sctx->gmag[0], sctx->gvec);
    len = sqrt(ELL_3V_DOT(sctx->norm, sctx->norm));
    if (len) {
      ELL_3V_SCALE(sctx->norm, 1.0/len, sctx->norm);
    }
    else {
      ELL_3V_COPY(sctx->norm, gageSclZeroNormal);
    }
  }
  if (1 & (query >> gageSclHessian)) {
    /* done if doD2 */
    if (sctx->c.verbose) {
      fprintf(stderr, "%s: hess = \n", me);
      ell3mPRINT(stdout, sctx->hess);
    }
  }
  if (1 & (query >> gageSclLaplacian)) {
    sctx->lapl[0] = sctx->hess[0] + sctx->hess[4] + sctx->hess[8];
  }
  if (1 & (query >> gageSclHessEval)) {
    ELL_3M_COPY(m, sctx->hess);
    /* HEY: look at the return value for root multiplicity? */
    ell3mEigensolve(heval, hevec, m, AIR_TRUE);
    ELL_3V_COPY(sctx->heval, heval);
  }
  if (1 & (query >> gageSclHessEvec)) {
    ELL_3M_COPY(sctx->hevec, hevec);
  }
  if (1 & (query >> gageScl2ndDD)) {
    ELL_3MV_MUL(tmpVec, sctx->hess, sctx->norm);
    sctx->scnd[0] = ELL_3V_DOT(sctx->norm, tmpVec);
  }
  if (1 & (query >> gageSclGeomTens)) {
    ginv = 1.0/sctx->gmag[0];
    /* we also flip the sign here, so that when values "inside"
       an isosurface are higher, we get the expected sense */
    ELL_3M_SCALE(sHess, -ginv, sctx->hess);
    ELL_3MV_OUTER(ZPP, sctx->norm, sctx->norm);
    ZPP[0] -= 1;
    ZPP[4] -= 1;
    ZPP[8] -= 1;

    /* sctx->gen = ZPP * sHess * ZPP */
    ELL_3M_MUL(tmpMat, sHess, ZPP);
    ELL_3M_MUL(sctx->gten, ZPP, tmpMat);

    if (sctx->c.verbose) {
      ELL_3MV_MUL(tmpVec, sctx->gten, sctx->norm); len = ELL_3V_LEN(tmpVec);
      printf("should be small: %30.15lf\n", len);
      ell3vPERP(gp1, sctx->norm);
      ELL_3MV_MUL(tmpVec, sctx->gten, gp1); len = ELL_3V_LEN(tmpVec);
      printf("should be bigger: %30.15lf\n", len);
      ELL_3V_CROSS(gp2, gp1, sctx->norm);
      ELL_3MV_MUL(tmpVec, sctx->gten, gp2); len = ELL_3V_LEN(tmpVec);
      printf("should be bigger: %30.15lf\n", len);
    }
  }
  if (1 && (query >> gageSclK1K2)) {
    T = ELL_3M_TRACE(sctx->gten);
    N = ELL_3M_L2NORM(sctx->gten);
    D = 2*N*N - T*T;
    if (D < 0) {
      fprintf(stderr, "%s: !!! D = % 22.10f\n", me, D);
    }
    D = AIR_MAX(D, 0);
    D = sqrt(D);
    sctx->k1k2[0] = K1 = 0.5*(T + D);
    sctx->k1k2[1] = K2 = 0.5*(T - D);
  }
  if (1 & (query >> gageSclCurvDir)) {
    /* HEY */
    fprintf(stderr, "%s: sorry, gageSclCurvDir not implemented\n", me);
  }
  if (1 & (query >> gageSclShapeIndex)) {
    sctx->S[0] = -(2/M_PI)*atan2(K1 + K2, K1 - K2);
  }
  if (1 & (query >> gageSclCurvedness)) {
    sctx->C[0] = sqrt(K1*K1 + K2*K2);
  }
  return;
}

int
gageSclProbe(gageSclContext *sctx, gage_t x, gage_t y, gage_t z) {
  char me[]="gageSclProbe";
  int i, newBidx;
  char *here;                /* points somewhere into sctx->npad->data */

  if (_gageLocationSet(&sctx->c, &newBidx, x, y, z)) {
    /* we're outside the volume; leave gageErrStr and gageErrNum set
       (as they should be) */
    return 1;
  }
  
  /* if necessary, refill the iv3 cache */
  if (newBidx) {
    here = ((char*)(sctx->npad->data)
	    + sctx->c.bidx*nrrdTypeSize[sctx->npad->type]);
    for (i=0; i<sctx->c.fd*sctx->c.fd*sctx->c.fd; i++)
      sctx->iv3[i] = sctx->lup(here, sctx->c.off[i]);
    if (sctx->c.verbose > 1)
      _gageSclPrint_iv3(sctx);
  }

  /* perform the filtering */
  if (sctx->k3pack) {
    switch (sctx->c.fd) {
    case 2:
      _gageScl3PFilter2(sctx->iv3, sctx->iv2, sctx->iv1, 
			sctx->c.fw[gageKernel00],
			sctx->c.fw[gageKernel11],
			sctx->c.fw[gageKernel22],
			sctx->val, sctx->gvec, sctx->hess,
			sctx->doV, sctx->doD1, sctx->doD2);
      break;
      /*
    case 4:
      _gageScl3PFilter4(ctx->iv3, ctx->iv2, ctx->iv1,
			ctx->fw00, ctx->fw11, ctx->fw22,
			ctx->val, ctx->gvec, ctx->hess,
			doD1, doD2);
      break;
    default:
      _gageScl3PFilterN(sctx->c.fd,
			ctx->iv3, ctx->iv2, ctx->iv1,
			ctx->fw00, ctx->fw11, ctx->fw22,
			ctx->val, ctx->gvec, ctx->hess,
			doD1, doD2);
      break;
      */
    }
  }
  else {
    fprintf(stderr, "!%s: non-3pack filtering unimplemented!!\n", me);
  }

  /* now do all the computations to answer the query */
  _gageSclAnswer(sctx);

  return 0;
}
