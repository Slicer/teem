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
_gageSclAnswer(gageContext *ctx, gagePerVolume *pvl) {
  char me[]="_gageSclAnswer";
  unsigned int query;
  gage_t len, tmpMat[9], tmpVec[3], sHess[9],
    gp1[3], gp2[3], ZPP[9], 
    ginv, T, N, D, K1, K2;
  double m[9], hevec[9], heval[3];
  gageSclAnswer *san;

  query = pvl->query;
  san = (gageSclAnswer *)pvl->ans;
  if (1 & (query >> gageSclValue)) {
    /* done if doV */
    if (ctx->verbose) {
      fprintf(stderr, "val = % 15.7f\n", (float)(san->val[0]));
    }
  }
  if (1 & (query >> gageSclGradVec)) {
    /* done if doD1 */
    if (ctx->verbose) {
      fprintf(stderr, "gvec = ");
      ell3vPRINT(stderr, san->gvec);
    }
  }
  if (1 & (query >> gageSclGradMag)) {
    san->gmag[0] = sqrt(ELL_3V_DOT(san->gvec, san->gvec)
			+ ctx->gradMagMin*ctx->gradMagMin);
  }
  if (1 & (query >> gageSclNormal)) {
    ELL_3V_SCALE(san->norm, 1.0/san->gmag[0], san->gvec);
    len = sqrt(ELL_3V_DOT(san->norm, san->norm));
    if (len) {
      ELL_3V_SCALE(san->norm, 1.0/len, san->norm);
    } else {
      ELL_3V_COPY(san->norm, gageSclZeroNormal);
    }
  }
  if (1 & (query >> gageSclHessian)) {
    /* done if doD2 */
    if (ctx->verbose) {
      fprintf(stderr, "%s: hess = \n", me);
      ell3mPRINT(stderr, san->hess);
    }
  }
  if (1 & (query >> gageSclLaplacian)) {
    san->lapl[0] = san->hess[0] + san->hess[4] + san->hess[8];
  }
  if (1 & (query >> gageSclHessEval)) {
    ELL_3M_COPY(m, san->hess);
    /* HEY: look at the return value for root multiplicity? */
    ell3mEigensolve(heval, hevec, m, AIR_TRUE);
    ELL_3V_COPY(san->heval, heval);
  }
  if (1 & (query >> gageSclHessEvec)) {
    ELL_3M_COPY(san->hevec, hevec);
  }
  if (1 & (query >> gageScl2ndDD)) {
    ELL_3MV_MUL(tmpVec, san->hess, san->norm);
    san->scnd[0] = ELL_3V_DOT(san->norm, tmpVec);
  }
  if (1 & (query >> gageSclGeomTens)) {
    ginv = 1.0/san->gmag[0];
    /* we also flip the sign here, so that when values "inside"
       an isosurface are higher, we get the expected sense */
    ELL_3M_SCALE(sHess, -ginv, san->hess);
    ELL_3MV_OUTER(ZPP, san->norm, san->norm);
    ZPP[0] -= 1;
    ZPP[4] -= 1;
    ZPP[8] -= 1;

    /* san->gen = ZPP * sHess * ZPP */
    ELL_3M_MUL(tmpMat, sHess, ZPP);
    ELL_3M_MUL(san->gten, ZPP, tmpMat);

    if (ctx->verbose) {
      ELL_3MV_MUL(tmpVec, san->gten, san->norm); len = ELL_3V_LEN(tmpVec);
      fprintf(stderr, "should be small: %30.15lf\n", len);
      ell3vPERP(gp1, san->norm);
      ELL_3MV_MUL(tmpVec, san->gten, gp1); len = ELL_3V_LEN(tmpVec);
      fprintf(stderr, "should be bigger: %30.15lf\n", len);
      ELL_3V_CROSS(gp2, gp1, san->norm);
      ELL_3MV_MUL(tmpVec, san->gten, gp2); len = ELL_3V_LEN(tmpVec);
      fprintf(stderr, "should be bigger: %30.15lf\n", len);
    }
  }
  if (1 && (query >> gageSclK1K2)) {
    T = ELL_3M_TRACE(san->gten);
    N = ELL_3M_L2NORM(san->gten);
    D = 2*N*N - T*T;
    if (D < 0) {
      fprintf(stderr, "%s: !!! D = % 22.10f\n", me, D);
    }
    D = AIR_MAX(D, 0);
    D = sqrt(D);
    san->k1k2[0] = K1 = 0.5*(T + D);
    san->k1k2[1] = K2 = 0.5*(T - D);
  }
  if (1 & (query >> gageSclCurvDir)) {
    /* HEY */
    fprintf(stderr, "%s: sorry, gageSclCurvDir not implemented\n", me);
  }
  if (1 & (query >> gageSclShapeIndex)) {
    san->S[0] = -(2/M_PI)*atan2(K1 + K2, K1 - K2);
  }
  if (1 & (query >> gageSclCurvedness)) {
    san->C[0] = sqrt(K1*K1 + K2*K2);
  }
  return;
}

void
_gageSclFilter(gageContext *ctx, gagePerVolume *pvl) {
  char me[]="_gageSclFilter";
  int fd;
  gageSclAnswer *san;

  fd = ctx->fd;
  san = (gageSclAnswer *)pvl->ans;
  /* perform the filtering */
  if (ctx->k3pack) {
    switch (fd) {
    case 2:
      _gageScl3PFilter2(pvl->iv3, pvl->iv2, pvl->iv1, 
			ctx->fw + fd*3*gageKernel00,
			ctx->fw + fd*3*gageKernel11,
			ctx->fw + fd*3*gageKernel22,
			san->val, san->gvec, san->hess,
			pvl->doV, pvl->doD1, pvl->doD2);
      break;
    case 4:
      _gageScl3PFilter4(pvl->iv3, pvl->iv2, pvl->iv1, 
			ctx->fw + fd*3*gageKernel00,
			ctx->fw + fd*3*gageKernel11,
			ctx->fw + fd*3*gageKernel22,
			san->val, san->gvec, san->hess,
			pvl->doV, pvl->doD1, pvl->doD2);
      break;
    default:
      _gageScl3PFilterN(ctx->fd,
			pvl->iv3, pvl->iv2, pvl->iv1, 
			ctx->fw + fd*3*gageKernel00,
			ctx->fw + fd*3*gageKernel11,
			ctx->fw + fd*3*gageKernel22,
			san->val, san->gvec, san->hess,
			pvl->doV, pvl->doD1, pvl->doD2);
      break;
    }
  } else {
    fprintf(stderr, "!%s: sorry, 6pack filtering not implemented\n", me);
  }

  return;
}
