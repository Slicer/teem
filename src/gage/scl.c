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
_gageSclSetFslw(gageSclContext *ctx,
		double xf, double yf, double zf,
		int doD1, int doD2) {
  int fd, i;
  GAGE_TYPE *fsl;
  double T, xs, ys, zs;

  /* float *p;*/

  /* fsl[? + fd*?]
         |      |
         |      +- along which axis (0:x, 1:y, 2:z)
         |
         +- position along axis (0 through fd-1)
  */
  fsl = ctx->fsl;
  fd = ctx->fd;
  switch (fd) {
  case 2:
    T = -xf; fsl[0+2*0]=T++; fsl[1+2*0]=T;
    T = -yf; fsl[0+2*1]=T++; fsl[1+2*1]=T;
    T = -zf; fsl[0+2*2]=T++; fsl[1+2*2]=T;
    break;
  case 4:
    T = -xf-1; fsl[0+4*0]=T++; fsl[1+4*0]=T++; fsl[2+4*0]=T++; fsl[3+4*0]=T;
    T = -yf-1; fsl[0+4*1]=T++; fsl[1+4*1]=T++; fsl[2+4*1]=T++; fsl[3+4*1]=T;
    T = -zf-1; fsl[0+4*2]=T++; fsl[1+4*2]=T++; fsl[2+4*2]=T++; fsl[3+4*2]=T;
    break;
  default:
    /* filter diameter is bigger than 4 */
    for (i=0; i<ctx->fd; i++) {
      fsl[i + fd*0] = -xf - ctx->needPad + i;
      fsl[i + fd*1] = -yf - ctx->needPad + i;
      fsl[i + fd*2] = -zf - ctx->needPad + i;
    }
    break;
  }

  /* the horror, the horror */
#if GT_FLOAT
#define EVAL(wch,fd)                                             \
  ctx->k[gageKernel##wch]->evalN_f(ctx->fw##wch, fsl, 3*fd,      \
                                   ctx->kparam[gageKernel##wch])
#else
#define EVAL(wch,fd)                                             \
  ctx->k[gageKernel##wch]->evalN_d(ctx->fw##wch, fsl, 3*fd,      \
                                   ctx->kparam[gageKernel##wch])
#endif
#define EVAL_ALL(fd)                    \
    EVAL(00,fd);                        \
    if (doD1) {                         \
      EVAL(11,fd);                      \
      if (doD2)                         \
	EVAL(22,fd);                    \
    }                                   \
    if (!ctx->k3pack) {                 \
      if (doD1) {                       \
	EVAL(10,fd);                    \
	if (doD2) {                     \
	  EVAL(20,fd); EVAL(21,fd);     \
	}                               \
      }                                 \
    }

  /* set values in fw{00,10,11,20,21,22} */
  switch (fd) {
  case 2:
    EVAL_ALL(2);
    break;
  case 4:
    EVAL_ALL(4);
    break;
  default:
    EVAL_ALL(fd);
    break;
  }

  /*
  if (ctx->renormalize) {
    _gageSclRenormalize(ctx, fw0, fw1, fw2, doD1, doD2);
  }
  */

  /* fix scalings for anisotropic voxels */
  xs = ctx->xs;
  ys = ctx->ys;
  zs = ctx->zs;
  if (!( 1.0 == xs && 1.0 == ys && 1.0 == zs )) {
    if (doD1) {
      for (i=0; i<fd; i++) {
	ctx->fw11[i + fd*0] /= xs;
	ctx->fw11[i + fd*1] /= ys;
	ctx->fw11[i + fd*2] /= zs;
	if (doD2) {
	  ctx->fw22[i + fd*0] /= xs*xs;
	  ctx->fw22[i + fd*1] /= ys*ys;
	  ctx->fw22[i + fd*2] /= zs*zs;
	}
      }
    }
    if (!ctx->k3pack) {
      if (doD1) {
	for (i=0; i<fd; i++) {
	  ctx->fw10[i + fd*0] /= xs;
	  ctx->fw10[i + fd*1] /= ys;
	  ctx->fw10[i + fd*2] /= zs;
	  if (doD2) {
	    ctx->fw20[i + fd*0] /= xs*xs;
	    ctx->fw21[i + fd*0] /= xs*xs;
	    ctx->fw20[i + fd*1] /= ys*ys;
	    ctx->fw21[i + fd*1] /= ys*ys;
	    ctx->fw20[i + fd*2] /= zs*zs;
	    ctx->fw21[i + fd*2] /= zs*zs;
	  }
	}
      }
    }
  }

  return;
}

void
_gageSclAnswer(gageSclContext *ctx) {
  unsigned int query;
  GT *gvec, *hess;
  double len, norm[3], tmpMat[9], tmpVec[3], sHess[9],
    gp1[3], gp2[3], ZPPr[9], ZPPc[9], ZPP[9], 
    ginv, T, N, D, K1, K2, m[9], hevec[9], heval[3];

  gvec = ctx->gvec;
  hess = ctx->hess;

  query = ctx->query;
  if (1 & (query >> gageSclValue)) {
    /* always done */
    if (ctx->verbose) {
      printf("val = % 15.7f", (float)(ctx->val[0]));
    }
  }
  if (1 & (query >> gageSclGradVec)) {
    /* done if doD1 */
    if (ctx->verbose) {
      printf("gvec = ");
      /* HEY */ ell3vPrint_f(stdout, ctx->gvec);
    }
  }
  if (1 & (query >> gageSclGradMag)) {
    ctx->gmag[0] = sqrt(ELL_3V_DOT(gvec, gvec)
			+ ctx->epsilon*ctx->epsilon);
  }
  if (1 & (query >> gageSclNormal)) {
    ELL_3V_SCALE(ctx->norm, gvec, 1.0/ctx->gmag[0]);
    len = sqrt(ELL_3V_DOT(ctx->norm, ctx->norm) + ctx->epsilon*ctx->epsilon);
    ELL_3V_SCALE(ctx->norm, ctx->norm, 1.0/len);
  }
  if (1 & (query >> gageSclHess)) {
    if (ctx->verbose) {
      printf("hess = \n");
      /* HEY */ ell3mPrint_f(stdout, hess);
    }
  }
  if (1 & (query >> gageSclHessEval)) {
    ELL_3M_COPY(m, hess);
    /* HEY: look at the return value for root multiplicity? */
    ell3mEigensolve(heval, hevec, m, AIR_TRUE);
    ELL_3V_COPY(ctx->heval, heval);
  }
  if (1 & (query >> gageSclHessEvec)) {
    ELL_3M_COPY(ctx->hevec, hevec);
  }
  if (1 & (query >> gageScl2ndDD)) {
    ELL_3MV_MUL(tmpVec, hess, ctx->norm);
    ctx->scnd[0] = ELL_3V_DOT(ctx->norm, tmpVec);
  }
  if (1 & (query >> gageSclGeomTens)) {
    ginv = 1.0/ctx->gmag[0];
    /* we also flip the sign here, so that when values "inside"
       an isosurface are higher, we get the expected sense */
    ELL_3M_SCALE(sHess, hess, -ginv);
    /* find a vector perpendicular to the normal */
    ell3vPerp_d(gp1, norm);
    ELL_3V_NORM(gp1, gp1, len);
    ELL_3V_CROSS(gp2, norm, gp1);
    /* set up ZPPr as the matrix that projects (x,y,z) vectors onto 
       the right-handed basis (norm , gp1, gp2), and then
       totally zeroes out the norm component; then set ZPPc
       is the transpose of that.
       remember that the storage order of the matrix is:
       0 3 6 
       1 4 7
       2 5 8
    */
    ZPPr[0] = ZPPr[3] = ZPPr[6] = 0;
    ELL_3MV_SET_ROW1(ZPPr, gp1);
    ELL_3MV_SET_ROW2(ZPPr, gp2);
    ELL_3M_TRANSPOSE(ZPPc, ZPPr);
    
    /* ZPP = ZPPc * ZPPr  (this also happens to be equal to ZPPr * ZPPc) */
    ELL_3M_MUL(ZPP, ZPPc, ZPPr);

    /* ctx->gen = ZPP * sHess * ZPP */
    ELL_3M_MUL(tmpMat, sHess, ZPP);
    ELL_3M_MUL(ctx->gten, ZPP, tmpMat);

    if (ctx->verbose) {
      ELL_3MV_MUL(tmpVec, ctx->gten, ctx->norm); len = ELL_3V_LEN(tmpVec);
      printf("should be small: %30.15lf\n", len);
      ELL_3MV_MUL(tmpVec, ctx->gten, gp1); len = ELL_3V_LEN(tmpVec);
      printf("should be bigger: %30.15lf\n", len);
      ELL_3MV_MUL(tmpVec, ctx->gten, gp2); len = ELL_3V_LEN(tmpVec);
      printf("should be bigger: %30.15lf\n", len);
    }
  }
  if (1 && (query >> gageSclK1K2)) {
    T = ELL_3M_TRACE(ctx->gten);
    N = ELL_3M_L2NORM(ctx->gten);
    D = 2*N*N - T*T;
    if (D < 0) {
      printf("D = % 22.10f\n", D);
    }
    D = AIR_MAX(D, 0);
    D = sqrt(D);
    ctx->k1k2[0] = K1 = 0.5*(T + D);
    ctx->k1k2[1] = K2 = 0.5*(T - D);
  }
  if (1 & (query >> gageSclCurvDir)) {
    
  }
  if (1 & (query >> gageSclShapeIndex)) {
    ctx->S[0] = -(2/M_PI)*atan2(K1 + K2, K1 - K2);
  }
  if (1 & (query >> gageSclCurvedness)) {
    ctx->C[0] = sqrt(K1*K1 + K2*K2);
  }
  return;
}

void
gageSclProbe(gageSclContext *ctx, float x, float y, float z) {
  char me[]="gageSclProbe";
  int i,
    doD1, doD2,              /* if we need first or second derivatives */
    hpad, fd,
    sx, sy, sz,
    tx, ty, tz,
    bidx,                    /* base index: lowest index of sample in cache */
    xi, yi, zi;              /* integral components of position (x,y,z) */
  double 
    xf, yf, zf;              /* fractional components of position (x,y,z) */
  char *here;                /* points somewhere into ctx->npad->data */

  doD1 = (1 <= ctx->maxDeriv);
  doD2 = (2 <= ctx->maxDeriv);
  fd = ctx->fd;
  hpad = ctx->havePad;
  sx = ctx->sx; tx = sx-2*hpad-1;
  sy = ctx->sy; ty = sy-2*hpad-1;
  sz = ctx->sz; tz = sz-2*hpad-1;
  if (!( AIR_INSIDE(0,x,tx) && AIR_INSIDE(0,y,ty) && AIR_INSIDE(0,z,tz) )) {
    fprintf(stderr, "%s: position (%g,%g,%g) outside volume!\n", me, x, y, z);
    return;
  }
  
  xi = x; xi -= xi == tx; xf = x - xi;
  yi = y; yi -= yi == ty; yf = y - yi;
  zi = z; zi -= zi == tz; zf = z - zi;
  bidx = xi + sx*(yi + sy*zi);
  if (ctx->verbose > 1) {
    fprintf(stderr, 
	    "%s: \n"
	    "        pos (% 15.7f,% 15.7f,% 15.7f) \n"
	    "        -> i(%5d,%5d,%5d) \n"
	    "         + f(% 15.7f,% 15.7f,% 15.7f) \n"
	    "        -> bidx = %d\n",
	    me, (float)x, (float)y, (float)z,
	    xi, yi, zi, (float)xf, (float)yf, (float)zf, bidx);
  }
  
  /* if necessary, refill the iv3 cache */
  if (ctx->bidx != bidx) {
    here = (char*)(ctx->npad->data) + bidx*nrrdTypeSize[ctx->npad->type];
    for (i=0; i<fd*fd*fd; i++)
      ctx->iv3[i] = ctx->lup(here, ctx->off[i]);
    if (ctx->verbose > 1)
      _gageSclPrint_iv3(ctx);
    ctx->bidx = bidx;
  }

  /* set values in fsl */
  if (!( ctx->xf == xf && ctx->yf == yf && ctx->zf == zf )) {
    _gageSclSetFslw(ctx, xf, yf, zf, doD1, doD2);
    if (ctx->verbose > 1) {
      _gageSclPrint_fslw(ctx, doD1, doD2);
    }
    ctx->xf = xf;
    ctx->yf = yf;
    ctx->zf = zf;
  }
  
  /* perform the filtering */
  if (ctx->k3pack) {
    switch (fd) {
    case 2:
      _gageScl3PFilter2(ctx->iv3, ctx->iv2, ctx->iv1, 
			ctx->fw00, ctx->fw11, ctx->fw22,
			ctx->val, ctx->gvec, ctx->hess,
			doD1, doD2);
      break;
    case 4:
      _gageScl3PFilter4(ctx->iv3, ctx->iv2, ctx->iv1,
			ctx->fw00, ctx->fw11, ctx->fw22,
			ctx->val, ctx->gvec, ctx->hess,
			doD1, doD2);
      break;
    default:
      _gageScl3PFilterN(fd,
			ctx->iv3, ctx->iv2, ctx->iv1,
			ctx->fw00, ctx->fw11, ctx->fw22,
			ctx->val, ctx->gvec, ctx->hess,
			doD1, doD2);
      break;
    }
  }
  else {
    fprintf(stderr, "!%s: non-3pack filtering unimplemented!!\n", me);
  }

  /* now do all the computations to answer the query */
  _gageSclAnswer(ctx);

  return;
}
