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
******** gageSet()
**
** for setting the boolean-ish flags in the context in an intelligent
** manner, since changing some of them can have many consequences
*/
void
gageSet(gageContext *ctx, int which, int val) {
  char me[]="gageSet";
  
  switch (which) {
  case gageVerbose:
    ctx->verbose = val;
    break;
  case gageRenormalize:
    ctx->renormalize = !!val;
    /* we have to make sure that any existing filter weights
       are not re-used; because gageUpdage() is not called mid-probing,
       we don't use the flag machinery.  Instead we just invalidate
       the last known fractional probe locations */
    ctx->xf = ctx->xf = ctx->xf = AIR_NAN;
    break;
  case gageCheckIntegrals:
    ctx->checkIntegrals = !!val;
    /* nothing changes because of this, it just affects future calls to
       gageKernelSet() */
    break;
  case gageK3Pack:
    ctx->k3pack = !!val;
    ctx->flag[gageFlagK3Pack] = AIR_TRUE;
    break;
  case gageNoRepadWhenSmaller:
    ctx->noRepadWhenSmaller = !!val;
    ctx->flag[gageFlagNRWS] = AIR_TRUE;
    break;
  default:
    fprintf(stderr, "\n%s: which = %d unknown!!\n\n", me, which);
    break;
  }
  return;
}

/*
******** gagePerVolumeAttach()
**
** attaches a pervolume to a context, which actually involves 
** very little work
*/
int
gagePerVolumeAttach(gageContext *ctx, gagePerVolume *pvl) {
  char me[]="gagePerVolumeAttach", err[AIR_STRLEN_MED];

  if (!( ctx && pvl )) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(GAGE, err); return 1;
  }
  if (_gagePerVolumeAttached(ctx, pvl)) {
    sprintf(err, "%s: given pervolume already attached", me);
    biffAdd(GAGE, err); return 1;
  }
  if (ctx->numPvl == GAGE_PERVOLUME_NUM) {
    sprintf(err, "%s: sorry, already have GAGE_PERVOLUME_NUM == %d "
	    "pervolumes attached", me, GAGE_PERVOLUME_NUM);
    biffAdd(GAGE, err); return 1;
  }

  /* here we go */
  ctx->pvl[ctx->numPvl++] = pvl;

  return 0;
}

/*
******** gagePerVolumeDetach()
**
** detaches a pervolume from a context, but does nothing else
** with the pervolume; caller may still want to call gagePerVolumeNix
*/
int
gagePerVolumeDetach(gageContext *ctx, gagePerVolume *pvl) {
  char me[]="gagePerVolumeDetach", err[AIR_STRLEN_MED];
  int i, idx;

  if (!( ctx && pvl )) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(GAGE, err); return 1;
  }
  idx = -1;
  for (i=0; i<ctx->numPvl; i++) {
    if (pvl == ctx->pvl[i]) {
      idx = i;
    }
  }
  if (-1 == idx) {
    sprintf(err, "%s: given pervolume not currently attached", me);
    biffAdd(GAGE, err); return 1;
  }
  for (i=idx+1; i<ctx->numPvl; i++) {
    ctx->pvl[i-1] = ctx->pvl[i];
  }
  ctx->pvl[ctx->numPvl--] = NULL;
  return 0;
}

/*
******** gageAnswerPointer()
**
** way of getting a pointer to a specific answer in a pervolume's
** ansStruct, assuming the pervolume is valid, the requested measure
** is a valid, and has an ansStruct allocted, and the ansStruct's main
** answer array is where it should be in the struct.
*/
gage_t *
gageAnswerPointer(gagePerVolume *pvl, int measure) {
  gage_t *ret = NULL;
  gageSclAnswer *san;

  if (pvl && airEnumValidVal(pvl->kind->enm, measure)) {
    san = (gageSclAnswer *)(pvl->ansStruct);
    if (san) {
      ret = san->ans + pvl->kind->ansOffset[measure];
    }
  }
  return ret;
}

/*
******** gageQuerySet()
**
** sets a query in a pervolume.  Does recursive expansion of query
** to cover all prerequisite measures.  
**
** Sets: pvl->query, pvl->needD[]
*/
int
gageQuerySet(gageContext *ctx, gagePerVolume *pvl, unsigned int query) {
  char me[]="gageQuerySet", err[AIR_STRLEN_MED];
  unsigned int mask, lastq, q;
  int d;
  
  if (!( ctx && pvl )) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(GAGE, err); return 1;
  }
  mask = (1U << (pvl->kind->queryMax+1)) - 1;
  if (query != (query & mask)) {
    sprintf(err, "%s: invalid bits set in query", me);
    biffAdd(GAGE, err); return 1;
  }
  pvl->query = query;
  if (ctx->verbose) {
    fprintf(stderr, "%s: original query = %u ...\n", me, pvl->query);
    pvl->kind->queryPrint(stderr, pvl->query);
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
  if (ctx->verbose) {
    fprintf(stderr, "!%s: expanded query = %u ...\n", me, pvl->query);
    pvl->kind->queryPrint(stderr, pvl->query);
  }
  pvl->needD[0] = pvl->needD[1] = pvl->needD[2] = AIR_FALSE;
  q = pvl->kind->queryMax+1;
  do {
    q--;
    if (pvl->query & (1 << q)) {
      for (d=0; d<=2; d++) {
	pvl->needD[d]  |= (pvl->kind->needDeriv[q] & (1 << d));
      }
    }
  } while (q);

  ctx->pvlFlag[gageFlagNeedD] = AIR_TRUE;

  return 0;
}

void
gagePadderSet(gageContext *ctx, gagePadder_t *padder) {

  if (ctx) {
    ctx->padder = padder;
    ctx->flag[gageFlagPadder] = AIR_TRUE;
  }
}

void
gageNixerSet(gageContext *ctx, gageNixer_t *nixer) {
  
  if (ctx) {
    ctx->nixer = nixer;
  }
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
**
** Sets: ctx->k[which], ctx->kparm[which]
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
	sprintf(err, "%s: reconstruction kernel's integral (%g) not > 0.0",
		me, integral);
	biffAdd(GAGE, err); return 1;
      }
    } else {
      /* its a derivative, so integral must be near zero */
      if (!( AIR_ABS(integral) <= ctx->integralNearZero )) {
	sprintf(err, "%s: derivative kernel's integral (%g) not within "
		"%g of 0.0",
		me, integral, ctx->integralNearZero);
	biffAdd(GAGE, err); return 1;
      }
    }
  }

  /* okay, fine, set the kernel */
  ctx->k[which] = k;
  memcpy(ctx->kparm[which], kparm, numParm*sizeof(double));
  ctx->flag[gageFlagKernel] = AIR_TRUE;

  return 0;
}

/*
** gageKernelReset()
**
** reset kernels and parameters.
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
    ctx->flag[gageFlagKernel] = AIR_TRUE;
  }
  return;
}

/*
******** gageVolumeSet()
**
** Associates an unpadded volume with a pervolume.
**
** Requires that the volume's axis spacings are set (in contrast to
** previous versions of this function).
**
** Sets: pvl->nin, pvl->lup
*/
int
gageVolumeSet(gageContext *ctx, gagePerVolume *pvl,
	      Nrrd *nin) {
  char me[]="gageVolumeSet", err[AIR_STRLEN_MED];
  int bd;
  double xs, ys, zs;

  if (!( ctx && pvl && nin )) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(GAGE, err); return 1;
  }
  if (!_gagePerVolumeAttached(ctx, pvl)) {
    sprintf(err, "%s: given pervolume not currently attached", me);
    biffAdd(GAGE, err); return 1;
  }
  if (!nrrdValid(nin)) {
    sprintf(err, "%s: basic nrrd validity check failed", me);
    biffMove(GAGE, err, NRRD); return 1;
  }
  if (nrrdTypeBlock == nin->dim) {
    sprintf(err, "%s: need a non-block type nrrd", me);
    biffAdd(GAGE, err); return 1;
  }
  bd = pvl->kind->baseDim;
  if (3 + bd != nin->dim) {
    sprintf(err, "%s: nrrd should have dimension %d, not %d",
	    me, 3 + bd, nin->dim);
    biffAdd(GAGE, err); return 1;
  }
  xs = nin->axis[bd+0].spacing;
  ys = nin->axis[bd+1].spacing;
  zs = nin->axis[bd+2].spacing;
  if (!( AIR_EXISTS(xs) && AIR_EXISTS(ys) && AIR_EXISTS(zs) )) {
    sprintf(err, "%s: spacings (axes %d,%d,%d) don't all exist",
	    me, bd+0, bd+1, bd+2);
    biffAdd(GAGE, err); return 1;
  }
  if (!( xs != 0 && ys != 0 && zs != 0 )) {
    sprintf(err, "%s: spacings (%g,%g,%g) (axes %d,%d,%d) not all non-zero",
	    me, xs, ys, zs, bd+0, bd+1, bd+2);
    biffAdd(GAGE, err); return 1;
  }
  /* the checks to make sure than all the volumes match in size
     and spacing, and setting ctx->xs, ctx->ys, ctx->zs, ctx->fwScale[],
     will have to wait until gageUpdate() */
  pvl->nin = nin;
  pvl->lup = nrrdLOOKUP[nin->type];
  pvl->flagNin = AIR_TRUE;
  ctx->pvlFlag[gageFlagNin] = AIR_TRUE;

  return 0;
}

/*
******** gageProbe()
**
** how to do probing
**
** doesn't actually do much more than call callbacks in the gageKind
** structs of the attached pervolumes
*/
int
gageProbe(gageContext *ctx, gage_t x, gage_t y, gage_t z) {
  char me[]="gageProbe";
  int newBidx, i;
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
    for (i=0; i<ctx->numPvl; i++) {
      here = ((char*)(ctx->pvl[i]->npad->data)
	      + (ctx->bidx * 
		 ctx->pvl[i]->kind->valLen * 
		 nrrdTypeSize[ctx->pvl[i]->npad->type]));
      ctx->pvl[i]->kind->iv3Fill(ctx, ctx->pvl[i], here);
    }
  }
  /* fprintf(stderr, "##%s: bingo 2\n", me); */
  if (ctx->verbose > 1) {
    for (i=0; i<ctx->numPvl; i++) {
      fprintf(stderr, "%s: pvl[%d]'s value cache with bidx = %d:\n",
	      me, i, ctx->bidx);
      ctx->pvl[i]->kind->iv3Print(stderr, ctx, ctx->pvl[i]);
    }
  }

  /* fprintf(stderr, "##%s: bingo 3\n", me); */
  /* do filtering convolution to get basic answers */
  for (i=0; i<ctx->numPvl; i++) {
    ctx->pvl[i]->kind->filter(ctx, ctx->pvl[i]);
  }

  /* fprintf(stderr, "##%s: bingo 4\n", me); */
  /* generate remaining answers */
  for (i=0; i<ctx->numPvl; i++) {
    ctx->pvl[i]->kind->answer(ctx, ctx->pvl[i]);
  }
  
  /* fprintf(stderr, "##%s: bingo 5\n", me); */
  return 0;
}

