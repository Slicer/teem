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

void
_gageSclAnswer(gageContext *ctx, gagePerVolume *pvl) {
  char me[]="_gageSclAnswer";
  unsigned int query;
  gage_t len, sHess[9], gp1[3], gp2[3], nPerp[9], 
    ginv, gmag=0;
  double tmpMat[9], tmpVec[3], hevec[9], heval[3], T, N, D;
  gageSclAnswer *san;

  query = pvl->query;
  san = (gageSclAnswer *)pvl->ansStruct;
  if (1 & (query >> gageSclValue)) {
    /* done if doV */
    if (ctx->verbose) {
      fprintf(stderr, "val = % 15.7f\n", (double)(san->val[0]));
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
    san->gmag[0] = sqrt(ELL_3V_DOT(san->gvec, san->gvec));
  }
  if (1 & (query >> gageSclNormal)) {
    /* this will always set gmag */
    if (san->gmag[0]) {
      ELL_3V_SCALE(san->norm, 1.0/san->gmag[0], san->gvec);
      /* polishing ... 
      len = sqrt(ELL_3V_DOT(san->norm, san->norm));
      ELL_3V_SCALE(san->norm, 1.0/len, san->norm);
      */
      gmag = san->gmag[0];
    } else {
      ELL_3V_COPY(san->norm, gageZeroNormal);
      gmag = ctx->gradMagMin;
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
    if (ctx->verbose) {
      fprintf(stderr, "%s: lapl = %g + %g + %g  = %g\n", me,
	      san->hess[0], san->hess[4], san->hess[8],
	      san->lapl[0]);
    }
  }
  if (1 & (query >> gageSclHessEval)) {
    ELL_3M_COPY(tmpMat, san->hess);
    /* HEY: look at the return value for root multiplicity? */
    ell3mEigensolve(heval, hevec, tmpMat, AIR_TRUE);
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
    if (gmag - ctx->gradMagMin >= ctx->gradMagCurvMin) {
      ginv = 1.0/gmag;
      /* we flip the sign as well as scaling the Hessian, so that when
	 values "inside" an isosurface are higher, we get the expected
	 sense: the normal points outwards from a surface */
      ELL_3M_SCALE(sHess, -ginv, san->hess);
      /* nPerp = I - outer(norm, norm): matrix which projects onto the
	 plane perpendicular to the normal */
      ELL_3MV_OUTER(nPerp, san->norm, san->norm);
      ELL_3M_SCALE(nPerp, -1, nPerp);
      nPerp[0] += 1;
      nPerp[4] += 1;
      nPerp[8] += 1;
      
      /* san->gten = nPerp * sHess * nPerp */
      ELL_3M_MUL(tmpMat, sHess, nPerp);
      ELL_3M_MUL(san->gten, nPerp, tmpMat);
      

      if (ctx->verbose) {
	fprintf(stderr, "gten: \n");
	ell3mPRINT(stderr, san->gten);
	ELL_3MV_MUL(tmpVec, san->gten, san->norm); len = ELL_3V_LEN(tmpVec);
	fprintf(stderr, "should be small: %30.15f\n", (double)len);
	ell3vPERP(gp1, san->norm);
	ELL_3MV_MUL(tmpVec, san->gten, gp1); len = ELL_3V_LEN(tmpVec);
	fprintf(stderr, "should be bigger: %30.15f\n", (double)len);
	ELL_3V_CROSS(gp2, gp1, san->norm);
	ELL_3MV_MUL(tmpVec, san->gten, gp2); len = ELL_3V_LEN(tmpVec);
	fprintf(stderr, "should be bigger: %30.15f\n", (double)len);
      }
    } else {
      ELL_3M_SET_ZERO(san->gten);
    }
  }
  if (1 && (query >> gageSclCurvedness)) {
    san->C[0] = ELL_3M_L2NORM(san->gten);
  }
  if (1 && (query >> gageSclShapeTrace)) {
    san->St[0] = (san->C[0] 
		  ? ELL_3M_TRACE(san->gten)/san->C[0]
		  : 0);
  }
  if (1 && (query >> gageSclK1K2)) {
    T = ELL_3M_TRACE(san->gten);
    N = san->C[0];
    D = 2*N*N - T*T;
    if (D < -0.0000001) {
      fprintf(stderr, "%s: %g %g\n", me, T, N);
      fprintf(stderr, "%s: !!! D curv determinant % 22.10f < 0.0\n", me, D);
	fprintf(stderr, "gten: \n");
	ell3mPRINT(stderr, san->gten);
    }
    D = AIR_MAX(D, 0);
    D = sqrt(D);
    san->k1k2[0] = 0.5*(T + D);
    san->k1k2[1] = 0.5*(T - D);
  }
  if (1 && (query >> gageSclMeanCurv)) {
    san->mc[0] = (san->k1k2[0] + san->k1k2[1])/2;
  }
  if (1 && (query >> gageSclGaussCurv)) {
    san->gc[0] = san->k1k2[0]*san->k1k2[1];
  }
  if (1 && (query >> gageSclShapeIndex)) {
    san->Si[0] = -(2/M_PI)*atan2(san->k1k2[0] + san->k1k2[1],
				 san->k1k2[0] - san->k1k2[1]);
  }
  if (1 & (query >> gageSclCurvDir)) {
    /* HEY: this only works when K1, K2, 0 are all well mutually distinct,
       since these are the eigenvalues of the geometry tensor, and this
       code assumes that the eigenspaces are all one-dimensional */
    ELL_3M_COPY(tmpMat, san->gten);
    ELL_3M_SET_DIAG(tmpMat,
		    san->gten[0]-san->k1k2[0],
		    san->gten[4]-san->k1k2[0],
		    san->gten[8]-san->k1k2[0]);
    ell3mNullspace1(tmpVec, tmpMat);
    ELL_3V_COPY(san->cdir+0, tmpVec);
    ELL_3M_SET_DIAG(tmpMat,
		    san->gten[0]-san->k1k2[1],
		    san->gten[4]-san->k1k2[1],
		    san->gten[8]-san->k1k2[1]);
    ell3mNullspace1(tmpVec, tmpMat);
    ELL_3V_COPY(san->cdir+3, tmpVec);
  }
  return;
}

