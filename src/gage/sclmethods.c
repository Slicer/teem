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

#define RESET(p) p = airFree(p)

/*
******** gageSclContextNew()
**
** creates and initializes a probing context, and returns a pointer to it.
** NULL-return is erroneous, does not use biff.
*/
gageSclContext *
gageSclContextNew() {
  int i;
  gageSclContext *ctx;

  ctx = malloc(sizeof(gageSclContext));
  if (ctx) {
    ctx->query = 0;
    ctx->iv3 = ctx->iv2 = ctx->iv1 = NULL;
    ctx->fsl = NULL;
    ctx->fw00 = NULL;
    ctx->fw10 = ctx->fw11 = NULL;
    ctx->fw20 = ctx->fw21 = ctx->fw22 = NULL;
    ctx->off = NULL;
    gageSclResetKernels(ctx);
    ctx->npad = NULL;
    ctx->verbose = gageDefVerbose;
    ctx->epsilon = gageDefEpsilon;
    ctx->renormalize = gageDefRenormalize;
    
    ctx->maxDeriv = -1;
    ctx->needPad = -1;
    ctx->k3pack = AIR_TRUE;
    ctx->fr = ctx->fd = 0;
    ctx->iv3 = ctx->iv2 = ctx->iv1 = NULL;
    ctx->fsl = NULL;
    ctx->fw00 = NULL;
    ctx->fw10 = ctx->fw11 = NULL;
    ctx->fw20 = ctx->fw21 = ctx->fw21 = NULL;
    ctx->havePad = -1;
    ctx->lup = NULL;
    ctx->sx = ctx->sy = ctx->sz = 0;
    ctx->xs = ctx->ys = ctx->zs = AIR_NAN;
    ctx->bidx = -1;
    ctx->off = NULL;
    ctx->xf = ctx->xf = ctx->xf = AIR_NAN;
    
    for (i=0; i<GAGE_SCL_TOTAL_ANS_LENGTH; i++)
      ctx->ans[i] = AIR_NAN;
    ctx->val   = &(ctx->ans[gageSclAnsOffset[gageSclValue]]);
    ctx->gvec  = &(ctx->ans[gageSclAnsOffset[gageSclGradVec]]);
    ctx->gmag  = &(ctx->ans[gageSclAnsOffset[gageSclGradMag]]);
    ctx->norm  = &(ctx->ans[gageSclAnsOffset[gageSclNormal]]);
    ctx->hess  = &(ctx->ans[gageSclAnsOffset[gageSclHess]]);
    ctx->heval = &(ctx->ans[gageSclAnsOffset[gageSclHessEval]]);
    ctx->hevec = &(ctx->ans[gageSclAnsOffset[gageSclHessEvec]]);
    ctx->scnd  = &(ctx->ans[gageSclAnsOffset[gageScl2ndDD]]);
    ctx->gten  = &(ctx->ans[gageSclAnsOffset[gageSclGeomTens]]);
    ctx->k1k2  = &(ctx->ans[gageSclAnsOffset[gageSclK1K2]]);
    ctx->cdir  = &(ctx->ans[gageSclAnsOffset[gageSclCurvDir]]);
    ctx->S     = &(ctx->ans[gageSclAnsOffset[gageSclShapeIndex]]);
    ctx->C     = &(ctx->ans[gageSclAnsOffset[gageSclCurvedness]]);
  }
  return ctx;
}

/*
******** gageSclContextNix()
**
** destructor method for gageSclContext
*/
gageSclContext *
gageSclContextNix(gageSclContext *ctx) {

  if (ctx) {
    RESET(ctx->iv3); RESET(ctx->iv2); RESET(ctx->iv1);
    RESET(ctx->fsl);
    RESET(ctx->fw00);
    RESET(ctx->fw10); RESET(ctx->fw11); 
    RESET(ctx->fw20); RESET(ctx->fw21); RESET(ctx->fw22);
    RESET(ctx->off);
    airFree(ctx);
  }
  return NULL;
}

/*
******** gageSclSetQuery()
**
** sets query (either the first one, or a new one) for probing.  However,
** you may find that the query stored in the context after calling this
** is different than the query you passed.  That is because this function
** evaluates the recursive expansion of all the prerequisites to the 
** answers indicated by the query.
** 
** This function was implemented in a simple and stupid way, because
** the expectation is that it will not be very called very often 
** (as compared to calling gageScl())
**
** returns non-zero on error, does use biff.
*/
int
gageSclSetQuery(gageSclContext *ctx, unsigned int query) {
  char me[]="gageSclSetQuery", err[128];
  unsigned int lastq, q, mask;
  
  if (!ctx) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(GAGE, err); return 1;
  }
  if (!query) {
    sprintf(err, "%s: why probe if you have no query?", me);
    biffAdd(GAGE, err); return 1;
  }
  mask = (1 << (GAGE_SCL_MAX+1)) - 1;
  if (query != (query & mask)) {
    sprintf(err, "%s: invalid bits set in query", me);
    biffAdd(GAGE, err); return 1;
  }
  
  /* do recursive expansion of pre-requisites */
  ctx->query = query;
  fprintf(stderr, "!%s: original query = %u ...\n", me, ctx->query);
  _gageSclPrint_query(ctx->query);
  do {
    lastq = ctx->query;
    q = GAGE_SCL_MAX+1;
    do {
      q--;
      if ((1<<q) & ctx->query)
	ctx->query |= _gageSclPrereq[q];
    } while (q);
  } while (ctx->query != lastq);
  fprintf(stderr, "!%s: expanded query = %u ...\n", me, ctx->query);
  _gageSclPrint_query(ctx->query);

  _gageSclSetQueryDependent(ctx);

  return 0;
}

/*
** _gageSclSetQueryDependent()
**
** sets things immediately and solely dependent on query:
** maxDeriv
*/
void
_gageSclSetQueryDependent(gageSclContext *ctx) {
  unsigned int q;
  
  /* set maxDeriv, based on query */
  ctx->maxDeriv = 0;
  q = GAGE_SCL_MAX+1;
  do {
    q--;
    if (ctx->query & (1 << q))
      ctx->maxDeriv = AIR_MAX(ctx->maxDeriv, _gageSclNeedDeriv[q]);
  } while (q);
  fprintf(stderr, "!%s: maxDeriv = %d\n",
	  "_gageSclSetQueryDependent", ctx->maxDeriv);
  return;
}

/*
******** gageSclResetKernels()
**
** sets all the kernels to NULL and their params to AIR_NAN
*/
void
gageSclResetKernels(gageSclContext *ctx) {
  int i, j;

  if (ctx) {
    for(i=gageKernelUnknown+1; i<gageKernelLast; i++) {
      ctx->k[i] = NULL;
      for (j=0; j<=NRRD_KERNEL_PARAMS_MAX; j++)
	ctx->kparam[i][j] = AIR_NAN;
    }
    _gageSclResetKernelDependent(ctx);
  }
  return;
}

/*
******** _gageSclResetKernelDependent()
**
** resets (to same state as after gageSclContextNew()) all things
** immediately and solely dependent on kernels:
** needPad, k3pack, fr, fd, iv{3,2,1}, fsl, fw{00,10,11,20,21,22}, off
*/
void
_gageSclResetKernelDependent(gageSclContext *ctx) {

  ctx->needPad = -1;
  ctx->k3pack = AIR_TRUE;
  ctx->fr = ctx->fd = 0;
  RESET(ctx->iv3); RESET(ctx->iv2); RESET(ctx->iv1);
  RESET(ctx->fsl);
  RESET(ctx->fw00);
  RESET(ctx->fw10); RESET(ctx->fw11); 
  RESET(ctx->fw20); RESET(ctx->fw21); RESET(ctx->fw22);
  RESET(ctx->off);
}

/*
******** gageSclSetKernel()
**
** tell the prober what kernels to use for the various tasks
*/
int
gageSclSetKernel(gageSclContext *ctx, int which,
		 nrrdKernel *k, double *param) {
  char me[]="gageSclSetKernel", err[AIR_STRLEN_MED];
  int numParam;
  double support, integral;

  if (!(ctx && k && param)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(GAGE, err); return 1;
  }
  if (!(AIR_BETWEEN(gageKernelUnknown, which, gageKernelLast))) {
    sprintf(err, "%s: \"which\" not in range [%d,%d]", me,
	    gageKernelUnknown+1, gageKernelLast-1);
    biffAdd(GAGE, err); return 1;
  }
  numParam = k->numParam;
  if (numParam > NRRD_KERNEL_PARAMS_MAX) {
    sprintf(err, "%s: kernel's numParam=%d > NRRD_KERNEL_PARAMS_MAX=%d",
	    me, numParam, NRRD_KERNEL_PARAMS_MAX);
    biffAdd(GAGE, err); return 1;
  }
  support = k->support(param);
  if (!( support > 0 )) {
    sprintf(err, "%s: kernel's support (%g) not > 0", me, support);
    biffAdd(GAGE, err); return 1;
  }
  if (gageKernel00 == which ||
      gageKernel10 == which ||
      gageKernel20 == which) {
    integral = k->integral(param);
    if (!( integral > 0 )) {
      sprintf(err, "%s: reconstructionkernel's integral (%g) not > 0",
	      me, integral);
      biffAdd(GAGE, err); return 1;
    }
  }

  /* okay, fine, set the kernel */
  ctx->k[which] = k;
  memcpy(ctx->kparam[which], param, numParam*sizeof(double));

  if (_gageSclSetKernelDependent(ctx)) {
    sprintf(err, "%s:", me);
    biffAdd(GAGE, err); return 1;
  }
  
  return 0;
}

/*
******** _gageSclSetKernelDependent()
**
** Sets all things immediately and solely dependent on kernels:
** needPad, k3pack, fr, fd, iv{3,2,1}, fsl, fw{00,10,11,20,21,22}, off
*/
int
_gageSclSetKernelDependent(gageSclContext *ctx) {
  char me[]="_gageSclSetKernelDependent", err[AIR_STRLEN_MED];
  float maxRad;
  int fd, k, E;

  maxRad = 0;
  for (k=gageKernel00; k<=gageKernel22; k++) {
    if (ctx->k[k]) {
      if (gageKernel10 == k || gageKernel20 == k || gageKernel21 == k)
	ctx->k3pack = AIR_FALSE;
      maxRad = AIR_MAX(maxRad, ctx->k[k]->support(ctx->kparam[k]));
    }
  }
  ctx->fr = AIR_ROUNDUP(maxRad);
  ctx->needPad = ctx->fr-1;
  ctx->fd = 2*ctx->fr;
  fprintf(stderr, "!%s: fr = %d, needPad = %d, fd = %d\n",
	  "_gageSclSetKernelDependent", ctx->fr, ctx->needPad, ctx->fd);
	  

  /* it is in fact somewhat silly to be freeing and reallocating these
     every time a new kernel is set, especially since we may not actually
     need all of them for the current query, but it simplifies the task of
     preparing things prior to the gageSclCheck() and gageScl() calls */
  RESET(ctx->iv3); RESET(ctx->iv2); RESET(ctx->iv1);
  RESET(ctx->fsl);
  RESET(ctx->fsl);
  RESET(ctx->fw00);
  RESET(ctx->fw10); RESET(ctx->fw11); 
  RESET(ctx->fw20); RESET(ctx->fw21); RESET(ctx->fw22);
  RESET(ctx->off);
  E = 0;
  fd = ctx->fd;
  if (!E) E |= !(ctx->iv3 = calloc(fd*fd*fd, sizeof(GT)));
  if (!E) E |= !(ctx->iv2 = calloc(fd*fd, sizeof(GT)));
  if (!E) E |= !(ctx->iv1 = calloc(fd, sizeof(GT)));
  if (!E) E |= !(ctx->fsl = calloc(fd*3, sizeof(GT)));
  /* we allocate all of these regardless of the query for generality */
  if (!E) E |= !(ctx->fw00 = calloc(fd*3, sizeof(GT)));
  if (!E) E |= !(ctx->fw10 = calloc(fd*3, sizeof(GT)));
  if (!E) E |= !(ctx->fw11 = calloc(fd*3, sizeof(GT)));
  if (!E) E |= !(ctx->fw20 = calloc(fd*3, sizeof(GT)));
  if (!E) E |= !(ctx->fw21 = calloc(fd*3, sizeof(GT)));
  if (!E) E |= !(ctx->fw22 = calloc(fd*3, sizeof(GT)));
  if (!E) E |= !(ctx->off = calloc(fd*fd*fd, sizeof(int)));
  if (E) {
    sprintf(err, "%s: couldn't allocate all caches for fd=%d", me, fd);
    biffAdd(GAGE, err); return 1;
  }
  
  return 0;
}

int
gageSclGetNeedPad(gageSclContext *ctx) {

  return ctx ? ctx->needPad : -1;
}

int
gageSclSetPaddedVolume(gageSclContext *ctx, int pad, Nrrd *npad) {
  char me[]="gageSclSetPaddedVolume", err[AIR_STRLEN_MED];

  if (!( ctx && npad )) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(GAGE, err); return 1;
  }
  if (!( ctx->needPad >= 0 )) {
    sprintf(err, "%s: known needed padding (%d) invalid", me, ctx->needPad);
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
  if (3 != npad->dim) {
    sprintf(err, "%s: need a 3-dimensional nrrd (not %d)", me, npad->dim);
    biffAdd(GAGE, err); return 1;
  }
  if (nrrdTypeBlock == npad->dim) {
    sprintf(err, "%s: need a non-block type nrrd", me);
    biffAdd(GAGE, err); return 1;
  }

  ctx->havePad = pad;
  ctx->npad = npad;

  if (_gageSclSetVolumeDependent(ctx)) {
    sprintf(err, "%s:", me);
    biffAdd(GAGE, err); return 1;
  }

  return 0;
}

/*
** _gageSclSetVolumeDependent()
**
** Sets all things immediately and solely dependent on the volume
** lup, {x,y,z}s, s{x,y,z}, o2, o4
*/
int
_gageSclSetVolumeDependent(gageSclContext *ctx) {
  char me[]="_gageSclSetVolumeDependent", err[AIR_STRLEN_MED];
  int i, j, k, fd;

#if GT_FLOAT
  ctx->lup = nrrdFLookup[ctx->npad->type];
#else
  ctx->lup = nrrdDLookup[ctx->npad->type];
#endif
  ctx->sx = ctx->npad->axis[0].size;
  ctx->sy = ctx->npad->axis[1].size;
  ctx->sz = ctx->npad->axis[2].size;
  ctx->xs = ctx->npad->axis[0].spacing;
  ctx->ys = ctx->npad->axis[1].spacing;
  ctx->zs = ctx->npad->axis[2].spacing;
  ctx->xs = AIR_EXISTS(ctx->xs) ? ctx->xs : nrrdDefSpacing;
  ctx->ys = AIR_EXISTS(ctx->ys) ? ctx->ys : nrrdDefSpacing;
  ctx->zs = AIR_EXISTS(ctx->zs) ? ctx->zs : nrrdDefSpacing;
  if (!ctx->off) {
    sprintf(err, "%s: offset array (ctx->off) not allocated", me);
    biffAdd(GAGE, err); return 1;
  }
  fd = ctx->fd;
  for (k=0; k<fd; k++)
    for (j=0; j<fd; j++)
      for (i=0; i<fd; i++)
	ctx->off[i+fd*(j+fd*k)] = i + ctx->sx*(j + ctx->sy*k);
  return 0;
}

/*
******** gageSclCheck()
**
** makes sure that everything needed by gageScl() is set, and set to
** something reasonable, so that gageScl() doesn't have to do any
** error checking.
*/
int
gageSclUpdate(gageSclContext *ctx) {
  char me[]="gageSclUpdate", err[AIR_STRLEN_MED];

  if (!ctx) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(GAGE, err); return 1;
  }
  if (!ctx->query) {
    sprintf(err, "%s: no query has been set", me);
    biffAdd(GAGE, err); return 1;
  }
  if (!( ctx->havePad >= ctx->needPad)) {
    sprintf(err, "%s: current padding (%d) not >= needed padding (%d)", 
	    me, ctx->havePad, ctx->needPad);
    biffAdd(GAGE, err); return 1;
  }
  if (0 == ctx->fr) {
    sprintf(err, "%s: calculated max filter radius to be 0", me);
    biffAdd(GAGE, err); return 1;
  }
  if (2 == ctx->maxDeriv) {
    if (!ctx->k[gageKernel22]) {
      sprintf(err, "%s: need 2nd deriv. for query, but no kernel set", me);
      biffAdd(GAGE, err); return 1;
    }
    if (!!ctx->k[gageKernel20] ^ !!ctx->k[gageKernel21]) {
      sprintf(err, "%s: need both or neither of gageKernel20, gageKernel21",
	      me);
      biffAdd(GAGE, err); return 1;
    }
  }
  else if (1 == ctx->maxDeriv) {
    if (!ctx->k[gageKernel11]) {
      sprintf(err, "%s: need 1st deriv. for query, but no kernel set", me);
      biffAdd(GAGE, err); return 1;
    }
  }
  
  
  return 0;
}

gageSclContext *
gageSclContextCopy(gageSclContext *ctx) {
  char me[]="gageSclContextClone", err[AIR_STRLEN_MED];
  gageSclContext *nctx;
  int E, k;
  
  if (!ctx) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(GAGE, err); return NULL;
  }
  nctx = gageSclContextNew();
  E = 0;
  if (!E) E |= gageSclSetQuery(nctx, ctx->query);
  for (k=gageKernelUnknown+1; k<gageKernelLast; k++) {
    if (ctx->k[k]) {
      if (!E) E |= gageSclSetKernel(nctx, k, ctx->k[k], ctx->kparam[k]);
    }
  }
  if (!E) E |= gageSclSetPaddedVolume(nctx, ctx->havePad, ctx->npad);
  if (!E) E |= gageSclUpdate(nctx);
  if (E) {
    sprintf(err, "%s:", me);
    biffAdd(GAGE, err); return NULL;
  }
  return nctx;
}
