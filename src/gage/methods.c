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
** _gageKernelReset()
**
** reset kernels and the things that depend on them:
** k[], kparm, needPad, fr, fsl, fw[], off
** However, this obviously does not handle kernel-dependent things which
** are specific to scalar, vector, etc, probing
*/
void
_gageKernelReset(gageContext *ctx) {
  int i, j;

  if (ctx) {
    for(i=gageKernelUnknown+1; i<gageKernelLast; i++) {
      ctx->k[i] = NULL;
      for (j=0; j<NRRD_KERNEL_PARMS_NUM; j++)
	ctx->kparm[i][j] = AIR_NAN;
      RESET(ctx->fw[i]);
    }
    ctx->needPad = -1;
    ctx->fr = ctx->fd = 0;
    RESET(ctx->fsl);
    RESET(ctx->off);
  }
  return;
}

/*
** _gageContextInit()
**
** initializing everything in a gageContext
*/
void
_gageContextInit(gageContext *ctx) {
  int i;

  if (ctx) {
    ctx->verbose = gageDefVerbose;
    ctx->renormalize = gageDefRenormalize;
    ctx->checkIntegrals = gageDefCheckIntegrals;
    /* malloc/calloc doesn't generate NULLs ... */
    ctx->fsl = NULL;
    ctx->off = NULL;
    for(i=gageKernelUnknown+1; i<gageKernelLast; i++) {
      ctx->fw[i] = NULL;
    }
    _gageKernelReset(ctx);
    ctx->havePad = -1;
    ctx->sx = ctx->sy = ctx->sz = 0;
    ctx->xs = ctx->ys = ctx->zs = AIR_NAN;
    ctx->bidx = -1;
    ctx->xf = ctx->xf = ctx->xf = AIR_NAN;
  }
}

/*
** _gageContextDone()
**
** frees dynamically allocated memory in a gageContext, which (surprise
** surprise) is wholly dependent on the kernel set
*/
void
_gageContextDone(gageContext *ctx) {

  if (ctx) {
    _gageKernelReset(ctx);
  }
}

/*
** _gageKernelSet()
**
** sets one kernel in a gageContext; but the value of this function
** is all the error checking it does.  
**
** Does use biff.
*/
int
_gageKernelSet(gageContext *ctx, 
	       int which, NrrdKernel *k, double *kparm) {
  char me[]="_gageKernelSet", err[AIR_STRLEN_MED];
  int numParm;
  double support, integral;

  if (!(ctx && k && kparm)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(GAGE, err); return 1;
  }
  if (!(AIR_BETWEEN(gageKernelUnknown, which, gageKernelLast))) {
    sprintf(err, "%s: \"which\" not in range [%d,%d]", me,
	    gageKernelUnknown+1, gageKernelLast-1);
    biffAdd(GAGE, err); return 1;
  }
  numParm = k->numParm;
  if (numParm > NRRD_KERNEL_PARMS_NUM) {
    sprintf(err, "%s: kernel's numParm=%d > NRRD_KERNEL_PARMS_NUM=%d",
	    me, numParm, NRRD_KERNEL_PARMS_NUM);
    biffAdd(GAGE, err); return 1;
  }
  support = k->support(kparm);
  if (!( support > 0 )) {
    sprintf(err, "%s: kernel's support (%g) not > 0", me, support);
    biffAdd(GAGE, err); return 1;
  }
  if (ctx->checkIntegrals) {
    integral = k->integral(kparm);
    if (gageKernel00 == which ||
	gageKernel10 == which ||
	gageKernel20 == which) {
      if (!( integral > 0 )) {
	sprintf(err, "%s: reconstruction kernel's integral (%g) not > 0",
		me, integral);
	biffAdd(GAGE, err); return 1;
      }
    } else {
      /* its a derivative, so integral must be zero */
      if (!( integral == 0 )) {
	sprintf(err, "%s: derivative kernel's integral (%g) not == 0",
		me, integral);
	biffAdd(GAGE, err); return 1;
      }
    }
  }

  /* okay, fine, set the kernel */
  ctx->k[which] = k;
  memcpy(ctx->kparm[which], kparm, numParm*sizeof(double));

  if (_gageKernelDependentSet(ctx)) {
    sprintf(err, "%s:", me);
    biffAdd(GAGE, err); return 1;
  }
  return 0;
}

/*
**
** _gageKernelDependentSet()
**
** sets things which depend on the kernel set, but which are not
** specific to scalar, volume, etc, probing:
** fr, fd, needPad, fsl, off
** Is called after a kernel is set in the context.
**
** fw[] is not set because that is done by _gageUpdate()
**
** Does use biff.
*/
int
_gageKernelDependentSet(gageContext *ctx) {
  char me[]="_gageKernelDependentSet", err[AIR_STRLEN_MED];
  double maxRad;
  int i, E;

  maxRad = 0;
  for (i=gageKernelUnknown+1; i<gageKernelLast; i++) {
    if (ctx->k[i]) {
      maxRad = AIR_MAX(maxRad, ctx->k[i]->support(ctx->kparm[i]));
    }
  }
  ctx->fr = AIR_ROUNDUP(maxRad);
  ctx->fd = 2*ctx->fr;
  ctx->needPad = ctx->fr - 1;
  if (ctx->verbose) {
    fprintf(stderr, "%s: fr = %d, fd = %d, needPad = %d\n",
	    me, ctx->fr, ctx->fd, ctx->needPad);
  }
	  
  airFree(ctx->fsl);
  airFree(ctx->off);
  E = 0;
  if (!E) E |= !(ctx->fsl = calloc(ctx->fd*3, sizeof(gage_t)));
  if (!E) E |= !(ctx->off = calloc(ctx->fd*ctx->fd*ctx->fd, sizeof(int)));
  if (E) {
    sprintf(err, "%s: couldn't allocate caches for fd=%d", me, ctx->fd);
    biffAdd(GAGE, err); return 1;
  }

  return 0;
}

/*
** _gageVolumeSet
**
** this doesn't really set a volume so much as volume dimensions and
** spacings
**
** currently there is no cleverness to deal with possibility that
** one gageContext is being used simultaneously for different kinds
** of probing.
*/
int
_gageVolumeSet(gageContext *ctx, int pad, Nrrd *npad, int baseDim) {
  char me[]="_gageVolumeSet", err[AIR_STRLEN_MED];

  if (!( ctx && npad )) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(GAGE, err); return 1;
  }
  if (!( ctx->needPad >= 0 )) {
    sprintf(err, "%s: known needed padding (%d) invalid", me,
	    ctx->needPad);
    biffAdd(GAGE, err); return 1;
  }
  if (!( pad >= ctx->needPad)) {
    sprintf(err, "%s: given pad (%d) not >= needed padding (%d)", 
	    me, pad, ctx->needPad);
    biffAdd(GAGE, err); return 1;
  }
  if (!nrrdValid(npad)) {
    sprintf(err, "%s: basic nrrd validity check failed", me);
    biffMove(GAGE, err, NRRD); return 1;
  }
  if (nrrdTypeBlock == npad->dim) {
    sprintf(err, "%s: need a non-block type nrrd", me);
    biffAdd(GAGE, err); return 1;
  }

  ctx->havePad = pad;
  /* don't actually set any Nrrd */
  
  if (_gageVolumeDependentSet(ctx, npad, baseDim)) {
    sprintf(err, "%s:", me);
    biffAdd(GAGE, err); return 1;
  }

  return 0;
}

int
_gageVolumeDependentSet(gageContext *ctx, Nrrd *npad, int baseDim) {
  char me[]="_gageVolumeDependentSet", err[AIR_STRLEN_MED];
  int i, j, k, fd;
  
  ctx->sx = npad->axis[baseDim+0].size;
  ctx->sy = npad->axis[baseDim+1].size;
  ctx->sz = npad->axis[baseDim+2].size;
  ctx->xs = npad->axis[baseDim+0].spacing;
  ctx->ys = npad->axis[baseDim+1].spacing;
  ctx->zs = npad->axis[baseDim+2].spacing;
  ctx->xs = AIR_EXISTS(ctx->xs) ? ctx->xs : nrrdDefSpacing;
  ctx->ys = AIR_EXISTS(ctx->ys) ? ctx->ys : nrrdDefSpacing;
  ctx->zs = AIR_EXISTS(ctx->zs) ? ctx->zs : nrrdDefSpacing;
  if (ctx->verbose) {
    fprintf(stderr, 
	    "%s: padded (%d) volume: sizes (%d,%d,%d) spacings (%g,%g,%g)\n",
	    me, ctx->havePad, ctx->sx, ctx->sy, ctx->sz,
	    ctx->xs, ctx->ys, ctx->zs);
  }
  for (i=gageKernelUnknown+1; i<gageKernelLast; i++) {
    switch (i) {
    case gageKernel00:
    case gageKernel10:
    case gageKernel20:
      /* interpolation requires no re-weighting for non-unit spacing */
      ctx->fwScl[i][0] = 1.0;
      ctx->fwScl[i][1] = 1.0;
      ctx->fwScl[i][2] = 1.0;
      break;
    case gageKernel11:
    case gageKernel21:
      ctx->fwScl[i][0] = 1.0/ctx->xs;
      ctx->fwScl[i][1] = 1.0/ctx->ys;
      ctx->fwScl[i][2] = 1.0/ctx->zs;
      break;
    case gageKernel22:
      ctx->fwScl[i][0] = 1.0/(ctx->xs*ctx->xs);
      ctx->fwScl[i][1] = 1.0/(ctx->ys*ctx->ys);
      ctx->fwScl[i][2] = 1.0/(ctx->zs*ctx->zs);
      break;
    }
  }
  if (ctx->verbose) {
    fprintf(stderr, "%s: fw re-scaling for non-unit spacing:\n", me);
    fprintf(stderr, "              X               Y               Z\n");
#define PRINT(NN) \
    fprintf(stderr, "   "#NN": % 15.7f % 15.7f % 15.7f\n", \
	    ctx->fwScl[gageKernel##NN][0], \
	    ctx->fwScl[gageKernel##NN][1], \
	    ctx->fwScl[gageKernel##NN][2]);
    PRINT(00);
    PRINT(11);
    PRINT(22);
  }
  if (!ctx->off) {
    sprintf(err, "%s: offset array (ctx->off) not allocated", me);
    biffAdd(GAGE, err); return 1;
  }
  fd = ctx->fd;
  for (k=0; k<fd; k++)
    for (j=0; j<fd; j++)
      for (i=0; i<fd; i++)
	ctx->off[i + fd*(j + fd*k)] = i + ctx->sx*(j + ctx->sy*k);
  if (ctx->verbose > 2) {
    fprintf(stderr, "%s: newly calculated offset array\n", me);
    _gagePrint_off(ctx);
  }

  return 0;
}
		    
int
_gageUpdate(gageContext *ctx, int needK[GAGE_KERNEL_NUM]) {
  char me[]="_gageUpdate", err[AIR_STRLEN_MED];
  int i;
  
  if (!ctx) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(GAGE, err); return 1;
  }
  if (!( ctx->havePad >= ctx->needPad)) {
    sprintf(err, "%s: current padding (%d) not >= needed padding (%d)", 
	    me, ctx->havePad, ctx->needPad);
    biffAdd(GAGE, err); return 1;
  }
  if (!( ctx->fr > 0 && ctx->fd == 2*ctx->fr )) {
    sprintf(err, "%s: max filter radius, diameter (%d,%d) unset/invalid",
	    me, ctx->fr, ctx->fd);
    biffAdd(GAGE, err); return 1;
  }
  
  for (i=gageKernelUnknown+1; i<gageKernelLast; i++) {
    if (!needK[i]) {
      /* this kernel NOT needed */
      RESET(ctx->fw[i]);
    } else {
      if (!ctx->k[i]) {
	sprintf(err, "%s: ctx->k[%d] unset, but needed for query", me, i);
	biffAdd(GAGE, err); return 1;
      }
      if (!(ctx->fw[i] = calloc(3*ctx->fd, sizeof(gage_t)))) {
	sprintf(err, "%s: couldn't calloc(%d,%d) for ctx->fw[%d]",
		me, ctx->fd, (int)sizeof(gage_t), i);
	biffAdd(GAGE, err); return 1;
      }
    }
  }

  return 0;
}
