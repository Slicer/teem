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

#include "mite.h"

int 
miteRenderBegin(miteRender **mrrP, miteUser *muu) {
  char me[]="miteRenderBegin", err[AIR_STRLEN_MED];
  gagePerVolume *pvl;
  int T, E, thr, td;
  miteTxf *txf;
 
  if (!(mrrP && muu)) {
    sprintf(err, "%s: got NULL pointer", me);
    return 1;
  }
  if (!gageVolumeValid(muu->nin, gageKindScl)) {
    sprintf(err, "%s: trouble with input volume", me);
    biffMove(MITE, err, GAGE); return 1;
  }
  if (!( *mrrP = (miteRender *)calloc(1, sizeof(miteRenderInfo)) )) {
    sprintf(err, "%s: couldn't alloc miteRenderInfo", me);
    return 1;
  }
  airMopAdd(muu->mop, *mrrP, airFree, airMopAlways);
  if (!muu->ntxfNum) {
    sprintf(err, "%s: no transfer functions set", me);
    biffAdd(MITE, err); return 1;
  }
  td = 0;
  for (T=0; T<muu->ntxfNum; T++) {
    txf = miteTxfNew(muu->ntxf[T]);
    if (!txf) {
      sprintf(err, "%s: trouble with ntxf %d as transfer function", me, T);
      biffAdd(MITE, err); return 1;
    }
    airMopAdd(muu->mop, txf, (airMopper)miteTxfNix, airMopAlways);
    td += txf->ntxf->dim - 1;
    if (td > MITE_TXF_NUM) {
      sprintf(err, "%s: ntxf %d (with dim %d --> %d) exceeded total txf "
	      "dimensionality of %d",
	      me, T, txf->ntxf->dim - 1, txf->ntxf->dim, MITE_TXF_NUM);
      biffAdd(MITE, err); return 1;
    }
    (*mrrP)->txf[T] = txf;
  }
  (*mrrP)->txfNum = muu->ntxfNum;

  pvl = gagePerVolumeNew(muu->nin, gageKindScl);

  E = 0;
  if (!E) E |= gageKernelSet(muu->gctx0, gageKernel00,
			     muu->ksp[gageKernel00]->kernel,
			     muu->ksp[gageKernel00]->parm);
  if (!E) E |= gageKernelSet(muu->gctx0, gageKernel11,
			     muu->ksp[gageKernel11]->kernel,
			     muu->ksp[gageKernel11]->parm);
  /* HEY HEY HEY this is only for value and normal */
  /*
  if (!E) E |= gageKernelSet((*mrrP)->gtx0, gageKernel22,
			     muu->ksp22->kernel, muu->ksp22->parm);
  */
  if (!E) E |= gageQuerySet(pvl, (1<<gageSclValue)|(1<<gageSclNormal));
  if (!E) E |= gageUpdate((*mrrP)->gtx0);
  if (E) {
    sprintf(err, "%s: gage trouble", me);
    biffMove(MITE, err, GAGE);
    return 1;
  }
  
  fprintf(stderr, "%s: bye %d %d \n", me,
	  muu->ctx->imgSize[0], muu->ctx->imgSize[1]);
  (*mrrP)->sx = muu->ctx->imgSize[0];
  (*mrrP)->sy = muu->ctx->imgSize[1];
  if (nrrdAlloc((*mrrP)->nout=nrrdNew(), nrrdTypeFloat, 3,
		4, (*mrrP)->sx, (*mrrP)->sy)) {
    sprintf(err, "%s: nrrd trouble", me);
    biffMove(MITE, err, NRRD);
    return 1;
  }
  airMopAdd(muu->mop, (*mrrP)->nout, (airMopper)nrrdNuke, airMopAlways);
  (*mrrP)->nout->axis[0].center = nrrdCenterCell;
  (*mrrP)->nout->axis[0].min = muu->ctx->cam->uRange[0];
  (*mrrP)->nout->axis[0].max = muu->ctx->cam->uRange[1];
  (*mrrP)->nout->axis[1].center = nrrdCenterCell;
  (*mrrP)->nout->axis[1].min = muu->ctx->cam->vRange[0];
  (*mrrP)->nout->axis[1].max = muu->ctx->cam->vRange[1];
  (*mrrP)->imgData = (*mrrP)->nout->data;

  for (thr=0; thr<muu->ctx->numThreads; thr++) {
    (*mrrP)->tt[thr] = (miteThreadInfo *)calloc(1, sizeof(miteThreadInfo));
    airMopAdd(muu->mop, (*mrrP)->tt[thr], airFree, airMopAlways);
  }

  return 0;
}

int
miteRenderEnd(miteRenderInfo *mrr, miteUser *muu) {
  char me[]="miteRenderEnd", err[AIR_STRLEN_MED];

  mrr->time1 = airTime();
  fprintf(stderr, "\n%s: rendering time = %g secs\n", me,
	  mrr->time1 - mrr->time0);
  fprintf(stderr, "%s: muu->outS = %p = |%s|, mrr->nout = %p\n",
	  me, muu->outS, muu->outS, mrr->nout);
  if (nrrdSave(muu->outS, mrr->nout, NULL)) {
    sprintf(err, "%s: trouble saving image", me);
    biffMove(MITE, err, NRRD);
    return 1;
  }

  return 0;
}
