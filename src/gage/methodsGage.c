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
******** gageContextNew()
**
** doesn't use biff
*/
gageContext *
gageContextNew() {
  gageContext *ctx;
  int i;
  
  ctx = (gageContext*)calloc(1, sizeof(gageContext));
  if (ctx) {
    ctx->verbose = gageDefVerbose;
    ctx->gradMagMin = gageDefGradMagMin;
    ctx->renormalize = gageDefRenormalize;
    ctx->checkIntegrals = gageDefCheckIntegrals;
    ctx->noRepadWhenSmaller = gageDefNoRepadWhenSmaller;
    ctx->integralNearZero = gageDefIntegralNearZero;
    ctx->k3pack = gageDefK3Pack;
    gageKernelReset(ctx);
    for (i=0; i<GAGE_PERVOLUME_NUM; i++)
      ctx->pvl[i] = NULL;
    ctx->numPvl = 0;
    ctx->padder = _gageStandardPadder;
    ctx->nixer = _gageStandardNixer;
    for (i=0; i<GAGE_FLAG_NUM; i++) {
      ctx->flag[i] = AIR_FALSE;
      ctx->pvlFlag[i] = AIR_FALSE;
    }
    ctx->thisIsACopy = AIR_FALSE;
    ctx->needPad = ctx->havePad = ctx->fr = ctx->fd = -1;
    ctx->fsl = ctx->fw = NULL;
    ctx->off = NULL;
    ctx->sx = ctx->sy = ctx->sz = -1;
    ctx->xs = ctx->ys = ctx->zs = AIR_NAN;
    for (i=gageKernelUnknown+1; i<gageKernelLast; i++) {
      ctx->fwScale[i][0] = ctx->fwScale[i][1] = ctx->fwScale[i][2] = AIR_NAN;
    }
    ctx->needD[0] = ctx->needD[1] = ctx->needD[2] = AIR_FALSE;
    for (i=gageKernelUnknown+1; i<gageKernelLast; i++) {
      ctx->needK[i] = AIR_FALSE;
    }
    ctx->xf = ctx->xf = ctx->xf = AIR_NAN;
    ctx->bidx = -1;
  }
  return ctx;
}

/*
******** gageContextCopy()
**
** the semantics and utility of this are purposefully limited: given a context
** on which has just been successfully called gageUpdate(), this will create
** a new context and new attached pervolumes so that you can call gageProbe()
** on the new context and by probing the same volumes with the same kernels
** and the same queries.  And that's all you can do- you can't change any
** state in either the original or any of the copy contexts.  This is only
** intended as a simple way to supported multi-threaded usages which want
** to do the exact same thing in many different threads.  If you want to 
** change state, then gageContextNix() all the copy contexts, gage...Set(),
** gageUpdate(), and gageContextCopy() again.
**
** Does use biff, unlike all other functions in this file
*/
gageContext *
gageContextCopy(gageContext *ctx) {
  char me[]="gageContextCopy", err[AIR_STRLEN_MED];
  gageContext *ntx;
  gagePerVolume *pvl;
  int i, E;

  ntx = gageContextNew();
  if (!ntx) {
    sprintf(err, "%s: couldn't make a gageContext", me);
    biffAdd(GAGE, err); return NULL;
  }
  gageSet(ntx, gageVerbose, ctx->verbose);
  gageSet(ntx, gageRenormalize, ctx->renormalize);
  gageSet(ntx, gageCheckIntegrals, ctx->checkIntegrals);
  gageSet(ntx, gageNoRepadWhenSmaller, ctx->noRepadWhenSmaller);
  gageSet(ntx, gageK3Pack, ctx->k3pack);

  /* This is how to do the cleverness which makes this function useful.
     We'll explicitly set pvl->nin below, but because we're setting the
     padder and nixer to NULL, nothing is ever done with npad, so that
     only the original context will free it on gageContextNix() */
  gagePadderSet(ntx, NULL);
  gageNixerSet(ntx, NULL);

  /* gageSet is only good for ints... */
  ntx->gradMagMin = ctx->gradMagMin;
  ntx->integralNearZero = ctx->integralNearZero;

  /* now start the things that can generate errors */
  E = 0;
  for (i=0; i<GAGE_KERNEL_NUM; i++) {
    if (ctx->k[i]) {
      if (!E) E |= gageKernelSet(ntx, i, ctx->k[i], ctx->kparm[i]);
    }
  }
  if (E) {
    sprintf(err, "%s: trouble setting kernels", me);
    biffAdd(GAGE, err); return NULL;
  }
  for (i=0; i<ctx->numPvl; i++) {
    pvl = gagePerVolumeNew(ctx->pvl[i]->kind);
    if (!pvl) {
      sprintf(err, "%s: couldn't create pvl %d", me, i);
      biffAdd(GAGE, err); return NULL;
    }
    if (!E) E |= gagePerVolumeAttach(ntx, pvl);
    if (!E) E |= gageQuerySet(ntx, pvl, ctx->pvl[i]->query);
    if (!E) E |= gageVolumeSet(ntx, pvl, ctx->pvl[i]->nin);
    if (!E) pvl->padInfo = ctx->pvl[i]->padInfo;
    /* the sneakiness */
    if (!E) pvl->npad = ctx->pvl[i]->npad;
  }
  if (!E) E |= gageUpdate(ntx);
  if (E) {
    sprintf("%s:", me);
    biffAdd(GAGE, err); return NULL;
  }

  return ntx;
}


/*
** gageContextNix()
**
** responsible for freeing and clearing up everything hanging off a 
** context so that things can be returned to the way they were prior
** to gageContextNew().
**
** does not use biff
*/
gageContext *
gageContextNix(gageContext *ctx) {
  int i;

  if (ctx) {
    RESET(ctx->fw);
    RESET(ctx->fsl);
    RESET(ctx->off);
    for (i=0; i<ctx->numPvl; i++) {
      gagePerVolumeNix(ctx, ctx->pvl[i]);
    }
  }
  return airFree(ctx);
}

/*
******** gagePerVolumeNew()
**
** creates a new pervolume of a known kind, but nothing besides the
** answer struct is allocated
**
** does not use biff
*/
gagePerVolume *
gagePerVolumeNew(gageKind *kind) {
  gagePerVolume *pvl;

  if (!( kind )) {
    return NULL;
  }
  pvl = (gagePerVolume *)calloc(1, sizeof(gagePerVolume));
  if (pvl) {
    pvl->kind = kind;
    pvl->query = 0;
    pvl->nin = NULL;
    pvl->padInfo = NULL;
    pvl->npad = NULL;
    pvl->thisIsACopy = AIR_FALSE;
    pvl->flagNin = AIR_FALSE;
    pvl->iv3 = pvl->iv2 = pvl->iv1 = NULL;
    pvl->lup = NULL;
    pvl->needD[0] = pvl->needD[1] = pvl->needD[2] = AIR_FALSE;
    pvl->ansStruct = pvl->kind->ansNew();
  }
  return pvl;
}

/*
******** gagePerVolumeNix()
**
** uses the nixer to remove the padded volume, and frees all other
** dynamically allocated memory assocated with a pervolume
**
** Unfortunately, because the nixer is in the gageContext, that has
** to be passed to this function.
**
** does not use biff
*/
gagePerVolume *
gagePerVolumeNix(gageContext *ctx, gagePerVolume *pvl) {
  void *padInfo;

  RESET(pvl->iv3);
  RESET(pvl->iv2);
  RESET(pvl->iv1);
  padInfo = (ctx->nixer == _gageStandardNixer
	     ? NULL
	     : pvl->padInfo);
  /* a NULL nixer is a no-op */
  if (ctx->nixer) {
    ctx->nixer(pvl->npad, pvl->kind, padInfo);
  }
  pvl->kind->ansNix(pvl->ansStruct);
  return airFree(pvl);
}

