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
gageValSet(gageContext *ctx, int which, int val) {
  
  if (ctx && AIR_BETWEEN(gageValUnknown, which, gageValLast)) {
    switch (which) {
    case gageValVerbose:
      ctx->verbose = val;
      break;
    case gageValRenormalize:
      ctx->renormalize = val;
      break;
    case gageValCheckIntegrals:
      ctx->checkIntegrals = val;
      break;
    case gageValK3Pack:
      ctx->k3pack = val;
      break;
    case gageValNeedPad:
      ctx->needPad = val;
      break;
    case gageValHavePad:
      ctx->havePad = val;
      break;
    }
  }
  return;
}

int
gageValGet(gageContext *ctx, int which) {
  int val;

  if (ctx && AIR_BETWEEN(gageValUnknown, which, gageValLast)) {
    switch (which) {
    case gageValVerbose:
      val = ctx->verbose;
      break;
    case gageValRenormalize:
      val = ctx->renormalize;
      break;
    case gageValCheckIntegrals:
      val = ctx->checkIntegrals;
      break;
    case gageValK3Pack:
      val = ctx->k3pack;
      break;
    case gageValNeedPad:
      val = ctx->needPad;
      break;
    case gageValHavePad:
      val = ctx->havePad;
      break;
    }
  }
  return val;
}

/*
** gageKernelSet()
**
** sets one kernel in a gageContext; but the value of this function
** is all the error checking it does.
**
** Refers to ctx->checkIntegrals and acts appropriately.
**
** Does use biff.
*/
int
gageKernelSet(gageContext *ctx, 
	      int which, NrrdKernel *k, double *kparm) {
  char me[]="_gageKernelSet", err[AIR_STRLEN_MED];
  int numParm;
  double support, integral;

  if (ctx->verbose) {
    fprintf(stderr, "%s: which = %d -> %s\n", me, which,
	    airEnumStr(gageKernel, which));
  }
  if (!(ctx && k && kparm)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(GAGE, err); return 1;
  }
  if (!airEnumValidVal(gageKernel, which)) {
    sprintf(err, "%s: \"which\" (%d) not in range [%d,%d]", me,
	    which, gageKernelUnknown+1, gageKernelLast-1);
    biffAdd(GAGE, err); return 1;
  }
  numParm = k->numParm;
  if (!(AIR_INSIDE(0, numParm, NRRD_KERNEL_PARMS_NUM))) {
    sprintf(err, "%s: kernel's numParm (%d) not in range [%d,%d]",
	    me, numParm, 0, NRRD_KERNEL_PARMS_NUM);
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
** gageKernelReset()
**
** reset kernels and the things that depend on them:
** k[], kparm, needPad, fr, fd, fsl, fw, off
** However, this obviously does not handle kernel-dependent things which
** are specific to scalar, vector, etc, probing
*/
void
gageKernelReset(gageContext *ctx) {
  int i, j;

  if (ctx) {
    for(i=gageKernelUnknown+1; i<gageKernelLast; i++) {
      ctx->k[i] = NULL;
      for (j=0; j<NRRD_KERNEL_PARMS_NUM; j++)
	ctx->kparm[i][j] = AIR_NAN;
    }
    ctx->needPad = -1;
    ctx->fr = ctx->fd = -1;
    RESET(ctx->fw);
    RESET(ctx->fsl);
    RESET(ctx->off);
  }
  return;
}

/*
**
** _gageKernelDependentSet()
**
** sets things which depend on the kernel set, but which are not
** specific to scalar, volume, etc, probing:
** fr, fd, needPad, fsl, fw, off
** Is called after a kernel is set in the context.
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
  airFree(ctx->fw);
  airFree(ctx->off);
  E = 0;
  if (!E) E |= !(ctx->fsl = calloc(ctx->fd*3, sizeof(gage_t)));
  if (!E) E |= !(ctx->fw = calloc(ctx->fd*3*GAGE_KERNEL_NUM, sizeof(gage_t)));
  if (!E) E |= !(ctx->off = calloc(ctx->fd*ctx->fd*ctx->fd, sizeof(int)));
  if (E) {
    sprintf(err, "%s: couldn't allocate caches for fd=%d", me, ctx->fd);
    biffAdd(GAGE, err); return 1;
  }

  return 0;
}

/*
******** gageVolumeSet()
**
** Associates a volume with a context and a pervolume.
**
** Requires that the volume's axis spacings are set (in contrast to
** previous versions of this function).
*/
int
gageVolumeSet(gageContext *ctx, gagePerVolume *pvl,
	      Nrrd *npad, int havePad) {
  char me[]="gageVolumeSet", err[AIR_STRLEN_MED];
  int baseDim;

  if (!( ctx && pvl && npad )) {
    sprintf(err, "%s: got NULL pointer", me);
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
  baseDim = pvl->kind->baseDim;
  if (3 + baseDim != npad->dim) {
    sprintf(err, "%s: nrrd should have dimension %d, not %d",
	    me, 3 + baseDim, npad->dim);
    biffAdd(GAGE, err); return 1;
  }
  if (!( AIR_EXISTS(npad->axis[baseDim+0].spacing) &&
	 AIR_EXISTS(npad->axis[baseDim+1].spacing) &&
	 AIR_EXISTS(npad->axis[baseDim+2].spacing) )) {
    sprintf(err, "%s: spacings for axis %d,%d,%d don't all exist",
	    me, baseDim+0, baseDim+1, baseDim+2);
    biffAdd(GAGE, err); return 1;
  }
  if (!( ctx->needPad >= 0 )) {
    sprintf(err, "%s: known needed padding (%d) invalid", me,
	    ctx->needPad);
    biffAdd(GAGE, err); return 1;
  }
  if (!( havePad >= ctx->needPad)) {
    sprintf(err, "%s: given pad (%d) not >= needed padding (%d)", 
	    me, havePad, ctx->needPad);
    biffAdd(GAGE, err); return 1;
  }

  if (ctx->haveVolume) {
    if (!( ctx->havePad == havePad )) {
      sprintf(err, "%s: known padding (%d) != new padding (%d)",
	      me, ctx->havePad, havePad);
      biffAdd(GAGE, err); return 1;
    }
    if (!( ctx->sx == npad->axis[baseDim+0].size &&
	   ctx->sy == npad->axis[baseDim+1].size &&
	   ctx->sz == npad->axis[baseDim+2].size )) {
      sprintf(err, "%s: known dims (%d,%d,%d) != new dims (%d,%d,%d)",
	      me, ctx->sx, ctx->sy, ctx->sz,
	      npad->axis[baseDim+0].size,
	      npad->axis[baseDim+1].size,
	      npad->axis[baseDim+2].size);
      biffAdd(GAGE, err); return 1;
    }
    if (!( ctx->xs == npad->axis[baseDim+0].spacing &&
	   ctx->ys == npad->axis[baseDim+1].spacing &&
	   ctx->zs == npad->axis[baseDim+2].spacing )) {
      sprintf(err, "%s: known spacings (%g,%g,%g) != new spacings (%g,%g,%g)",
	      me, ctx->xs, ctx->ys, ctx->zs,
	      npad->axis[baseDim+0].spacing,
	      npad->axis[baseDim+1].spacing,
	      npad->axis[baseDim+2].spacing);
      biffAdd(GAGE, err); return 1;
    }
  } else {
    ctx->havePad = havePad;
    if (_gageVolumeDependentSet(ctx, npad, pvl->kind)) {
      sprintf(err, "%s:", me);
      biffAdd(GAGE, err); return 1;
    }
    ctx->haveVolume = AIR_TRUE;

    /* set things in the pvl */
    pvl->npad = npad;
    /* iv3,iv2,iv1 allocated by gagePerVolumeNew() */
    pvl->lup = nrrdLOOKUP[npad->type];
  }

  return 0;
}

int
_gageVolumeDependentSet(gageContext *ctx, Nrrd *npad, gageKind *kind) {
  char me[]="_gageVolumeDependentSet", err[AIR_STRLEN_MED];
  int i, j, k, fd, baseDim;
  
  /* first, set things in the context */
  baseDim = kind->baseDim;
  ctx->sx = npad->axis[baseDim+0].size;
  ctx->sy = npad->axis[baseDim+1].size;
  ctx->sz = npad->axis[baseDim+2].size;
  ctx->xs = npad->axis[baseDim+0].spacing;
  ctx->ys = npad->axis[baseDim+1].spacing;
  ctx->zs = npad->axis[baseDim+2].spacing;
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
    fprintf(stderr, "%s: offset array for %d x %d x %d volume:\n",
	    me, ctx->sx, ctx->sy, ctx->sz);
    _gagePrint_off(ctx);
  }

  return 0;
}

int
gageQuerySet(gagePerVolume *pvl, unsigned int query) {
  char me[]="gageQuerySet", err[AIR_STRLEN_MED];
  unsigned int mask, lastq, q;
  
  if (!( pvl )) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(GAGE, err); return 1;
  }
  if (!( pvl->kind )) {
    sprintf(err, "%s: pvl->kind NULL", me);
    biffAdd(GAGE, err); return 1;
  }
  mask = (1U << (pvl->kind->queryMax+1)) - 1;
  if (query != (query & mask)) {
    sprintf(err, "%s: invalid bits set in query", me);
    biffAdd(GAGE, err); return 1;
  }
  pvl->query = query;
  if (pvl->verbose) {
    fprintf(stderr, "%s: original query = %u ...\n", me, pvl->query);
    pvl->kind->queryPrint(pvl->query);
  }
  /* recursive expansion of prerequisites */
  do {
    lastq = pvl->query;
    q = pvl->kind->queryMax+1;
    do {
      q--;
      if ((1<<q) & pvl->query)
	pvl->query |= pvl->kind->queryPrereq[q];
    } while (q);
  } while (pvl->query != lastq);
  if (pvl->verbose) {
    fprintf(stderr, "!%s: expanded query = %u ...\n", me, pvl->query);
    pvl->kind->queryPrint(pvl->query);
  }

  return 0;
}

void
_gageNeedKSet(gageContext *ctx, gagePerVolume *pvl) {
  char me[]="_gageNeedKSet";
  unsigned int q;
  int i;

  /* we DO NOT reset all the needK and do{V,D1,D2} in ctx to
     AIR_FALSE, because we need to accumulate all the needs across the
     different pervolumes that may be associated with this context.
     We DO reset them in pvl because they are pervolume-specific */
  /* determine which kernels are needed */
  pvl->doV = pvl->doD1 = pvl->doD2 = AIR_FALSE;
  for (i=gageKernelUnknown+1; i<gageKernelLast; i++) {
    pvl->needK[i] = AIR_FALSE;
  }
  q = pvl->kind->queryMax+1;
  do {
    q--;
    if (pvl->query & (1 << q)) {
      if (pvl->kind->needDeriv[q] & (1 << 0)) {
	pvl->doV = AIR_TRUE;
	pvl->needK[gageKernel00] = ctx->needK[gageKernel00] = AIR_TRUE;
      }
      if (pvl->kind->needDeriv[q] & (1 << 1)) {
	pvl->doD1 = AIR_TRUE;
	pvl->needK[gageKernel11] = ctx->needK[gageKernel11] = AIR_TRUE;
	if (ctx->k3pack) {
	  pvl->needK[gageKernel00] = ctx->needK[gageKernel00] = AIR_TRUE;
	} else {
	  pvl->needK[gageKernel10] = ctx->needK[gageKernel10] = AIR_TRUE;
	}
      }
      if (pvl->kind->needDeriv[q] & (1 << 2)) {
	pvl->doD2 = AIR_TRUE;
	pvl->needK[gageKernel22] = ctx->needK[gageKernel22] = AIR_TRUE;
	if (ctx->k3pack) {
	  pvl->needK[gageKernel00] = ctx->needK[gageKernel00] = AIR_TRUE;
	  pvl->needK[gageKernel11] = ctx->needK[gageKernel11] = AIR_TRUE;
	} else {
	  pvl->needK[gageKernel20] = ctx->needK[gageKernel20] = AIR_TRUE;
	  pvl->needK[gageKernel21] = ctx->needK[gageKernel21] = AIR_TRUE;
	}
      }
    }
  } while (q);
  if (ctx->verbose) {
    fprintf(stderr,
	    "%s: needK = %d/%d ; %d/%d %d/%d ; %d/%d %d/%d %d/%d\n", me,
	    pvl->needK[gageKernel00], ctx->needK[gageKernel00],
	    pvl->needK[gageKernel10], ctx->needK[gageKernel10],
	    pvl->needK[gageKernel11], ctx->needK[gageKernel11],
	    pvl->needK[gageKernel20], ctx->needK[gageKernel20],
	    pvl->needK[gageKernel21], ctx->needK[gageKernel21],
	    pvl->needK[gageKernel22], ctx->needK[gageKernel22]);
  }
  return;
}

/*
******** gageUpdate()
**
** call just before probing begins.
**
** If probing multiple volumes with one context, this needs to be called
** once on each pervolume.
*/
int
gageUpdate(gageContext *ctx, gagePerVolume *pvl) {
  char me[]="gageUpdate", err[AIR_STRLEN_MED];
  int i;

  if (!( ctx && pvl )) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(GAGE, err); return 1;
  }
  if (!( ctx->fr > 0 && ctx->fd == 2*ctx->fr )) {
    sprintf(err, "%s: max filter radius, diameter (%d,%d) unset/invalid",
	    me, ctx->fr, ctx->fd);
    biffAdd(GAGE, err); return 1;
  }

  _gageNeedKSet(ctx, pvl);
  for (i=gageKernelUnknown+1; i<gageKernelLast; i++) {
    if (ctx->needK[i] && !ctx->k[i]) {
      sprintf(err, "%s: need kernel %s for given query", me,
	      airEnumStr(gageKernel, i));
      biffAdd(GAGE, err); return 1;
    }
  }

  if (!( pvl->npad )) {
    sprintf(err, "%s: no padded volume set", me);
    biffAdd(GAGE, err); return 1;
  }
  
  return 0;
}

/*
******** gageProbe()
**
** how to do probing
**
** doesn't actually do much more than call callbacks in the gageKind
** struct.
*/
int
gageProbe(gageContext *ctx, gagePerVolume *pvl,
	  gage_t x, gage_t y, gage_t z) {
  char me[]="gageProbe";
  int newBidx;
  char *here;
  
  /* fprintf(stderr, "##%s: bingo 0\n", me); */
  if (_gageLocationSet(ctx, &newBidx, x, y, z)) {
    /* we're outside the volume; leave gageErrStr and gageErrNum set
       (as they should be) */
    return 1;
  }
  
  /* fprintf(stderr, "##%s: bingo 1\n", me); */
  /* if necessary, refill the iv3 cache */
  if (newBidx) {
    here = ((char*)(pvl->npad->data)
	    + (ctx->bidx * pvl->kind->valLen * nrrdTypeSize[pvl->npad->type]));
    pvl->kind->iv3Fill(ctx, pvl, here);
  }
  /* fprintf(stderr, "##%s: bingo 2\n", me); */
  if (ctx->verbose > 1) {
    fprintf(stderr,
	    "%s: value cache with bidx = %d:\n", me, ctx->bidx);
    pvl->kind->iv3Print(ctx, pvl);
  }

  /* fprintf(stderr, "##%s: bingo 3\n", me); */
  /* do filtering convolution to get basic answers */
  pvl->kind->filter(ctx, pvl);

  /* fprintf(stderr, "##%s: bingo 4\n", me); */
  /* generate remaining answers */
  pvl->kind->answer(ctx, pvl);
  
  /* fprintf(stderr, "##%s: bingo 5\n", me); */
  return 0;
}
