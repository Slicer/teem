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

/*
******** gageContextNew()
**
*/
gageContext *
gageContextNew() {
  int i;
  gageContext *ctx;
  
  ctx = (gageContext*)calloc(1, sizeof(gageContext));
  if (ctx) {
    ctx->verbose = gageDefVerbose;
    ctx->gradMagMin = gageDefGradMagMin;
    ctx->renormalize = gageDefRenormalize;
    ctx->checkIntegrals = gageDefCheckIntegrals;
    ctx->k3pack = gageDefK3Pack;

    ctx->haveVolume = AIR_FALSE;

    ctx->fsl = ctx->fw = NULL;
    ctx->off = NULL;
    gageKernelReset(ctx);

    ctx->havePad = -1;
    ctx->sx = ctx->sy = ctx->sz = -1;
    ctx->xs = ctx->ys = ctx->zs = AIR_NAN;
    for (i=gageKernelUnknown+1; i<gageKernelLast; i++) {
      ctx->fwScl[i][0] = ctx->fwScl[i][1] = ctx->fwScl[i][2] = AIR_NAN;
    }

    for (i=gageKernelUnknown+1; i<gageKernelLast; i++) {
      ctx->needK[i] = AIR_FALSE;
    }

    ctx->bidx = -1;
    ctx->xf = ctx->xf = ctx->xf = AIR_NAN;
  }
  return ctx;
}

/*
** gageContextNix()
**
*/
gageContext *
gageContextNix(gageContext *ctx) {

  if (ctx) {
    gageKernelReset(ctx);
  }
  return airFree(ctx);
}

/*
******** gagePerVolumeNew()
**
** allocates a new gagePerVolume, given needPad (so as to allocate
** the iv3, iv2, and iv1 buffers).  Returns a pointer to the new
** struct if all is well, or NULL if otherwise.
**
** It does seem strange that needPad isn't actually stored in this
** struct, doesn't it.  Currently, it is stored in the gageContext.
**
** DOES use biff, unlike other gage methods.
*/
gagePerVolume *
gagePerVolumeNew(int needPad, gageKind *kind) {
  char me[]="gagePerVolumeNew", err[AIR_STRLEN_MED];
  gagePerVolume *pvl;
  int i, fd, E;

  if (!( needPad >= 0 )) {
    sprintf(err, "%s: given needPad (%d) not >= 0", me, needPad);
    biffAdd(GAGE, err); return NULL;
  }
  if (!( kind )) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(GAGE, err); return NULL;
  }

  fd = 2*(needPad + 1);
  pvl = (gagePerVolume *)calloc(1, sizeof(gagePerVolume));
  if (!( pvl )) {
    sprintf(err, "%s: couldn't allocate gagePerVolume struct", me);
    biffAdd(GAGE, err); return NULL;
  }

  E = 0;
  if (!E) E |= !(pvl->iv3 = calloc(fd*fd*fd*kind->valLen, sizeof(gage_t)));
  if (!E) E |= !(pvl->iv2 = calloc(fd*fd*kind->valLen, sizeof(gage_t)));
  if (!E) E |= !(pvl->iv1 = calloc(fd*kind->valLen, sizeof(gage_t)));
  if (E) {
    sprintf(err, "%s: couldn't allocate buffers for fd = %d", me, fd);
    biffAdd(GAGE, err); return NULL;
  }

  /* query- and volume-dependent stuff is initialized to NULL/non-values*/
  pvl->query = 0;
  pvl->npad = NULL;
  pvl->kind = kind;
  pvl->lup = NULL;
  pvl->doV = pvl->doD1 = pvl->doD2 = AIR_FALSE;
  for (i=gageKernelUnknown+1; i<gageKernelLast; i++) {
    pvl->needK[i] = AIR_FALSE;
  }

  pvl->ans = pvl->kind->ansNew();

  return pvl;
}

gagePerVolume *
gagePerVolumeNix(gagePerVolume *pvl) {

  airFree(pvl->iv3);
  airFree(pvl->iv2);
  airFree(pvl->iv1);
  pvl->kind->ansNix(pvl->ans);
  return airFree(pvl);
}

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
    san->cdir  = &(san->ans[gageSclAnsOffset[gageSclCurvDir]]);
  }
  return san;
}

gageSclAnswer *
_gageSclAnswerNix(gageSclAnswer *san) {

  return airFree(san);
}

gageVecAnswer *
_gageVecAnswerNew() {
  gageVecAnswer *van;
  int i;

  van = (gageVecAnswer *)calloc(1, sizeof(gageVecAnswer));
  if (van) {
    for (i=0; i<GAGE_VEC_TOTAL_ANS_LENGTH; i++)
      van->ans[i] = AIR_NAN;
    van->vec  = &(van->ans[gageVecAnsOffset[gageVecVector]]);
    van->len  = &(van->ans[gageVecAnsOffset[gageVecLength]]);
    van->norm = &(van->ans[gageVecAnsOffset[gageVecNormalized]]);
    van->jac  = &(van->ans[gageVecAnsOffset[gageVecJacobian]]);
    van->div  = &(van->ans[gageVecAnsOffset[gageVecDivergence]]);
    van->curl = &(van->ans[gageVecAnsOffset[gageVecCurl]]);
  }
  return van;
}

gageVecAnswer *
_gageVecAnswerNix(gageVecAnswer *van) {

  return airFree(van);
}
