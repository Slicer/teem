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
miteRenderBegin(miteRenderInfo **mrrP, miteUserInfo *muu) {
  char me[]="miteRenderBegin", err[AIR_STRLEN_MED];
  gagePerVolume *pvl;
  int E;
 
  fprintf(stderr, "%s: hi %p %p\n", me, mrrP, muu);
  if (!(mrrP && muu)) {
    sprintf(err, "%s: got NULL pointer", me);
    return 1;
  }
  if (!( *mrrP = (miteRenderInfo *)calloc(1, sizeof(miteRenderInfo)) )) {
    sprintf(err, "%s: couldn't alloc miteRenderInfo", me);
    return 1;
  }
  airMopAdd(muu->mop, *mrrP, airFree, airMopAlways);
  if (!( ((*mrrP)->gtx = gageContextNew()) &&
	 (pvl = gagePerVolumeNew(gageKindScl)) )) {
    sprintf(err, "%s: couldn't calloc gage structs", me);
    return 1;
  }
  airMopAdd(muu->mop, (*mrrP)->gtx, (airMopper)gageContextNix, airMopAlways);

  fprintf(stderr, "%s: hi %p %p\n", me, muu, muu->ctx);

  gageSet((*mrrP)->gtx, gageRenormalize, muu->renorm);
  gageSet((*mrrP)->gtx, gageCheckIntegrals, AIR_TRUE);
  E = 0;
  if (!E) E |= gagePerVolumeAttach((*mrrP)->gtx, pvl);
  if (!E) E |= gageKernelSet((*mrrP)->gtx, gageKernel00,
			     muu->ksp00->kernel, muu->ksp00->parm);
  if (!E) E |= gageKernelSet((*mrrP)->gtx, gageKernel11,
			     muu->ksp11->kernel, muu->ksp11->parm);
  /* HEY HEY HEY this is only for value and normal */
  /*
  if (!E) E |= gageKernelSet((*mrrP)->gtx, gageKernel22,
			     muu->ksp22->kernel, muu->ksp22->parm);
  */
  if (!E) E |= gageVolumeSet((*mrrP)->gtx, pvl, muu->nin);
  if (!E) E |= gageQuerySet((*mrrP)->gtx, pvl,
			    (1<<gageSclValue)|(1<<gageSclNormal));
  if (!E) E |= gageUpdate((*mrrP)->gtx);
  if (E) {
    sprintf(err, "%s: gage trouble", me);
    biffMove(MITE, err, GAGE);
    return 1;
  }
  (*mrrP)->san = (gageSclAnswer*)((*mrrP)->gtx->pvl[0]->ansStruct);
  
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

  return 0;
}

int
miteRenderEnd(miteRenderInfo *mrr, miteUserInfo *muu) {
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
