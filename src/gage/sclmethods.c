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
******** gageSclContextNew()
**
** creates and initializes a scalar probing context, and returns a
** pointer to it.  NULL-return is erroneous, does not use biff.
*/
gageSclContext *
gageSclContextNew() {
  int i;
  gageSclContext *sctx;

  sctx = malloc(sizeof(gageSclContext));
  if (sctx) {
    _gageContextInit(&sctx->c);
    sctx->npad = NULL;
    sctx->query = 0;
    sctx->epsilon = gageDefSclEpsilon;
    memset(sctx->needK, 0, GAGE_KERNEL_NUM*sizeof(int));
    sctx->iv3 = sctx->iv2 = sctx->iv1 = NULL;
    sctx->lup = NULL;
    for (i=0; i<GAGE_SCL_TOTAL_ANS_LENGTH; i++)
      sctx->ans[i] = AIR_NAN;
    sctx->val   = &(sctx->ans[gageSclAnsOffset[gageSclValue]]);
    sctx->gvec  = &(sctx->ans[gageSclAnsOffset[gageSclGradVec]]);
    sctx->gmag  = &(sctx->ans[gageSclAnsOffset[gageSclGradMag]]);
    sctx->norm  = &(sctx->ans[gageSclAnsOffset[gageSclNormal]]);
    sctx->hess  = &(sctx->ans[gageSclAnsOffset[gageSclHessian]]);
    sctx->lapl  = &(sctx->ans[gageSclAnsOffset[gageSclLaplacian]]);
    sctx->heval = &(sctx->ans[gageSclAnsOffset[gageSclHessEval]]);
    sctx->hevec = &(sctx->ans[gageSclAnsOffset[gageSclHessEvec]]);
    sctx->scnd  = &(sctx->ans[gageSclAnsOffset[gageScl2ndDD]]);
    sctx->gten  = &(sctx->ans[gageSclAnsOffset[gageSclGeomTens]]);
    sctx->k1k2  = &(sctx->ans[gageSclAnsOffset[gageSclK1K2]]);
    sctx->cdir  = &(sctx->ans[gageSclAnsOffset[gageSclCurvDir]]);
    sctx->S     = &(sctx->ans[gageSclAnsOffset[gageSclShapeIndex]]);
    sctx->C     = &(sctx->ans[gageSclAnsOffset[gageSclCurvedness]]);
  }
  return sctx;
}

/*
******** gageSclContextNix()
**
** destructor method for gageSclContext
*/
gageSclContext *
gageSclContextNix(gageSclContext *sctx) {

  if (sctx) {
    _gageContextDone(&sctx->c);
    sctx->npad = NULL;  /* nothing else */
    RESET(sctx->iv3);
    RESET(sctx->iv2);
    RESET(sctx->iv1);
    airFree(sctx);
  }
  return NULL;
}

/*
******** gageSclKernelSet()
**
** tell the prober what kernels to use for the various tasks
*/
int
gageSclKernelSet(gageSclContext *sctx,
		 int which, NrrdKernel *k, double *kparm) {
  char me[]="gageSclKernelSet", err[AIR_STRLEN_MED];
  
  if (_gageKernelSet(&sctx->c, which, k, kparm)) {
    sprintf(err, "%s:", me);
    biffAdd(GAGE, err); return 1;
  }
  if (_gageSclKernelDependentSet(sctx)) {
    sprintf(err, "%s:", me);
    biffAdd(GAGE, err); return 1;
  }
  
  return 0;
}

/*
******** _gageSclKernelDependentSet()
**
** Sets all things immediately and solely dependent on kernels:
** k3pack, iv{3,2,1}
*/
int
_gageSclKernelDependentSet(gageSclContext *sctx) {
  char me[]="_gageSclKernelDependentSet", err[AIR_STRLEN_MED];
  int i, fd, E;

  sctx->k3pack = AIR_TRUE;
  for (i=gageKernelUnknown+1; i<gageKernelLast; i++) {
    if (sctx->c.k[i]) {
      if (gageKernel10 == i || gageKernel20 == i || gageKernel21 == i)
	sctx->k3pack = AIR_FALSE;
    }
  }
  /* it is in fact somewhat silly to be freeing and reallocating these
     every time a new kernel is set, especially since we may not actually
     need all of them for the current query, but it simplifies the task of
     preparing things prior to the gageSclCheck() and gageSclProbe() calls */
  RESET(sctx->iv3);
  RESET(sctx->iv2);
  RESET(sctx->iv1);
  E = 0;
  fd = sctx->c.fd;
  if (!E) E |= !(sctx->iv3 = calloc(fd*fd*fd, sizeof(gage_t)));
  if (!E) E |= !(sctx->iv2 = calloc(fd*fd, sizeof(gage_t)));
  if (!E) E |= !(sctx->iv1 = calloc(fd, sizeof(gage_t)));
  if (E) {
    sprintf(err, "%s: couldn't allocate all caches for fd=%d", me, fd);
    biffAdd(GAGE, err); return 1;
  }
  
  return 0;
}

/*
******** gageSclQuerySet()
**
** sets query (either the first one, or a new one) for probing.  However,
** you may find that the query stored in the context after calling this
** is different than the query you passed.  That is because this function
** evaluates the recursive expansion of all the prerequisites to the 
** answers indicated by the query.
** 
** This function was implemented in a simple and stupid way, because
** the expectation is that it will not be very called very often 
** (as compared to calling gageSclProbe())
**
** returns non-zero on error, does use biff.
*/
int
gageSclQuerySet(gageSclContext *sctx, unsigned int query) {
  char me[]="gageSclQuerySet", err[128];
  unsigned int lastq, q, mask;
  
  if (!sctx) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(GAGE, err); return 1;
  }
  if (!query) {
    sprintf(err, "%s: why probe if you have no query?", me);
    biffAdd(GAGE, err); return 1;
  }
  mask = (1U << (GAGE_SCL_MAX+1)) - 1;
  if (query != (query & mask)) {
    sprintf(err, "%s: invalid bits set in query", me);
    biffAdd(GAGE, err); return 1;
  }
  
  /* do recursive expansion of pre-requisites */
  /* HEY: this functionality is (eventually) not scalar-specific */
  sctx->query = query;
  fprintf(stderr, "!%s: original query = %u ...\n", me, sctx->query);
  _gageSclPrint_query(sctx->query);
  do {
    lastq = sctx->query;
    q = GAGE_SCL_MAX+1;
    do {
      q--;
      if ((1<<q) & sctx->query)
	sctx->query |= _gageSclPrereq[q];
    } while (q);
  } while (sctx->query != lastq);
  fprintf(stderr, "!%s: expanded query = %u ...\n", me, sctx->query);
  _gageSclPrint_query(sctx->query);

  if (_gageSclQueryDependentSet(sctx)) {
    sprintf(err, "%s:", me);
    biffAdd(GAGE, err); return 1;
  }

  return 0;
}

/*
** _gageSclQueryDependentSet()
**
** sets needK
**
** currently, always returns 0, but that might change
*/
int
_gageSclQueryDependentSet(gageSclContext *sctx) {
  /* char me[]="_gageSclQueryDependentSet", err[AIR_STRLEN_MED]; */
  unsigned int q;
  
  memset(sctx->needK, 0, GAGE_KERNEL_NUM*sizeof(int));
  sctx->doV = sctx->doD1 = sctx->doD2 = AIR_FALSE;
  q = GAGE_SCL_MAX+1;
  do {
    q--;
    if (sctx->query & (1 << q)) {
      switch(_gageSclNeedDeriv[q]) {
      case 0:
	sctx->doV = AIR_TRUE;
	sctx->needK[gageKernel00] = AIR_TRUE;
	break;
      case 1:
	sctx->doD1 = AIR_TRUE;
	sctx->needK[gageKernel11] = AIR_TRUE;
	if (sctx->k3pack) {
	  sctx->needK[gageKernel00] = AIR_TRUE;
	} else {
	  sctx->needK[gageKernel10] = AIR_TRUE;
	}
	break;
      case 2:
	sctx->doD2 = AIR_TRUE;
	sctx->needK[gageKernel22] = AIR_TRUE;
	if (sctx->k3pack) {
	  sctx->needK[gageKernel00] = AIR_TRUE;
	  sctx->needK[gageKernel11] = AIR_TRUE;
	} else {
	  sctx->needK[gageKernel20] = AIR_TRUE;
	  sctx->needK[gageKernel21] = AIR_TRUE;
	}
	break;
      }
    }
  } while (q);
  fprintf(stderr, "!%s: needK = %d ; %d %d ; %d %d %d\n",
	  "_gageSclQueryDependentSet",
	  sctx->needK[gageKernel00],
	  sctx->needK[gageKernel10],
	  sctx->needK[gageKernel11],
	  sctx->needK[gageKernel20],
	  sctx->needK[gageKernel21],
	  sctx->needK[gageKernel22]);

  return 0;
}


int
gageSclNeedPadGet(gageSclContext *sctx) {

  return sctx ? sctx->c.needPad : -1;
}

int
gageSclVolumeSet(gageSclContext *sctx, int pad, Nrrd *npad) {
  char me[]="gageSclVolumeSet", err[AIR_STRLEN_MED];

  if (!( sctx && npad )) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(GAGE, err); return 1;
  }
  if (3 != npad->dim) {
    sprintf(err, "%s: need a 3-dimensional nrrd (not %d)", me, npad->dim);
    biffAdd(GAGE, err); return 1;
  }
  if (_gageVolumeSet(&sctx->c, pad, npad, 0)) {
    sprintf(err, "%s:", me);
    biffAdd(GAGE, err); return 1;
  }

  sctx->npad = npad;

  if (_gageSclVolumeDependentSet(sctx)) {
    sprintf(err, "%s:", me);
    biffAdd(GAGE, err); return 1;
  }

  return 0;
}

/*
** _gageSclVolumeDependentSet()
**
*/
int
_gageSclVolumeDependentSet(gageSclContext *sctx) {
  /* char me[]="_gageSclVolumeDependentSet", err[AIR_STRLEN_MED]; */

  sctx->lup = nrrdLOOKUP[sctx->npad->type];
  return 0;
}

/*
******** gageSclUpdate()
**
** makes sure that everything needed by gageSclProbe() is set, and set to
** something reasonable, so that gageSclProbe() doesn't have to do any
** error checking.
**
** May at some point actually create new information based on input
** paramters, but for the time being this responsibility has been
** pushed towards the Set methods.
*/
int
gageSclUpdate(gageSclContext *sctx) {
  char me[]="gageSclUpdate", err[AIR_STRLEN_MED];

  if (!sctx) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(GAGE, err); return 1;
  }
  if (!sctx->npad) {
    sprintf(err, "%s: no padded volume has been set", me);
    biffAdd(GAGE, err); return 1;
  }
  if (!sctx->query) {
    sprintf(err, "%s: no query has been set", me);
    biffAdd(GAGE, err); return 1;
  }
  /* this will check on kernels */
  if (_gageUpdate(&sctx->c, sctx->needK)) {
    sprintf(err, "%s:", me);
    biffAdd(GAGE, err); return 1;
  }
  
  return 0;
}

gageSclContext *
gageSclContextCopy(gageSclContext *sctx) {
  char me[]="gageSclContextCopy", err[AIR_STRLEN_MED];
  gageSclContext *nsctx;
  int E, i;
  
  if (!sctx) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(GAGE, err); return NULL;
  }
  nsctx = gageSclContextNew();
  E = 0;
  for (i=gageKernelUnknown+1; i<gageKernelLast; i++) {
    if (sctx->c.k[i]) {
      if (!E) E |= gageSclKernelSet(nsctx, i, sctx->c.k[i], sctx->c.kparm[i]);
    }
  }
  if (!E) E |= gageSclVolumeSet(nsctx, sctx->c.havePad, sctx->npad);
  if (!E) E |= gageSclQuerySet(nsctx, sctx->query);
  if (!E) E |= gageSclUpdate(nsctx);
  if (E) {
    sprintf(err, "%s:", me);
    biffAdd(GAGE, err); return NULL;
  }
  return nsctx;
}
