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


gageSimple *
gageSimpleNew() {
  gageSimple *spl;
  int i, j;

  spl = (gageSimple *)malloc(sizeof(gageSimple));
  if (spl) {
    spl->nin = NULL;
    spl->kind = NULL;
    for(i=gageKernelUnknown+1; i<gageKernelLast; i++) {
      spl->k[i] = NULL;
      for (j=0; j<NRRD_KERNEL_PARMS_NUM; j++)
	spl->kparm[i][j] = AIR_NAN;
    }
    spl->query = 0;
    spl->npad = NULL;
    if (!(spl->ctx = gageContextNew()))
      return NULL;
    spl->pvl = NULL;
    spl->ansStruct = NULL;
    spl->ansVec = NULL;
  }
  return spl;
}

int
gageSimpleUpdate(gageSimple *spl) {
  char me[]="gageSimpleUpdate", err[AIR_STRLEN_MED];
  int i, needPad, min[NRRD_DIM_MAX], max[NRRD_DIM_MAX], baseDim;

  if (!spl) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(GAGE, err); return 1;
  }
  /* context already created by gageSimpleNew */
  for (i=gageKernelUnknown+1; i<gageKernelLast; i++) {
    if (spl->k[i]) {
      if (gageKernelSet(spl->ctx, i, spl->k[i], spl->kparm[i])) {
	sprintf(err, "%s: trouble setting kernel %s", me,
		airEnumStr(gageKernel, i));
	biffAdd(GAGE, err); return 1;
      }
    }
  }
  needPad = gageValGet(spl->ctx, gageValNeedPad);
  if (!(spl->pvl = gagePerVolumeNew(needPad, spl->kind))) {
    sprintf(err, "%s: trouble creating pervolume", me);
    biffAdd(GAGE, err); return 1;
  }
  if (gageQuerySet(spl->pvl, spl->query)) {
    sprintf(err, "%s: trouble setting query", me);
    biffAdd(GAGE, err); return 1;
  }
  /* to create npad, the non-{x,y,z} axes are not altered */
  baseDim = spl->kind->baseDim;
  for (i=0; i<baseDim; i++) {
    min[i] = 0;
    max[i] = spl->nin->axis[i].size - 1;
    /*
    fprintf(stderr, "##%s: min, max[%d] = %d, %d\n", me, i, min[i], max[i]);
    */
  }
  /* the x,y,z axes are padded by needPad above and below */
  /* fprintf(stderr, "##%s: needPad = %d\n", me, needPad); */
  min[0 + baseDim] = -needPad;
  min[1 + baseDim] = -needPad;
  min[2 + baseDim] = -needPad;
  max[0 + baseDim] = spl->nin->axis[0 + baseDim].size - 1 + needPad;
  max[1 + baseDim] = spl->nin->axis[1 + baseDim].size - 1 + needPad;
  max[2 + baseDim] = spl->nin->axis[2 + baseDim].size - 1 + needPad;
  /*
  fprintf(stderr, "##%s: min, max[%d] = %d %d\n",
	  me, 0 + baseDim, min[0 + baseDim], max[0 + baseDim]);
  fprintf(stderr, "##%s: min, max[%d] = %d %d\n",
	  me, 1 + baseDim, min[1 + baseDim], max[1 + baseDim]);
  fprintf(stderr, "##%s: min, max[%d] = %d %d\n",
	  me, 2 + baseDim, min[2 + baseDim], max[2 + baseDim]);
  */
  if (nrrdPad(spl->npad=nrrdNew(), spl->nin, min, max, nrrdBoundaryBleed)) {
    sprintf(err, "%s: trouble padding input volume", me);
    biffMove(GAGE, err, NRRD); return 1;
  }
  if (gageVolumeSet(spl->ctx, spl->pvl, spl->npad, needPad)) {
    sprintf(err, "%s: trouble setting padded volume", me);
    biffAdd(GAGE, err); return 1;
  }
  if (gageUpdate(spl->ctx, spl->pvl)) {
    sprintf(err, "%s: trouble", me);
    biffAdd(GAGE, err); return 1;
  }
  spl->ansStruct = spl->pvl->ansStruct;
  spl->ansVec = ((gageSclAnswer*)(spl->pvl->ansStruct))->ans;

  return 0;
}

int
gageSimpleKernelSet(gageSimple *spl,
		    int which, NrrdKernel *k, double *kparm) {
  char me[]="gageSimpleKernelSet", err[AIR_STRLEN_MED];
  
  if (!(spl && k && kparm)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(GAGE, err); return 1;
  }
  if (!airEnumValidVal(gageKernel, which)) {
    sprintf(err, "%s: \"which\" (%d) not in range [%d,%d]", me,
	    which, gageKernelUnknown+1, gageKernelLast-1);
    biffAdd(GAGE, err); return 1;
  }

  spl->k[which] = k;
  memcpy(spl->kparm[which], kparm, NRRD_KERNEL_PARMS_NUM*sizeof(double));
  return 0;
}

int
gageSimpleProbe(gageSimple *spl, gage_t x, gage_t y, gage_t z) {
  
  return gageProbe(spl->ctx, spl->pvl, x, y, z);
}

gageSimple *
gageSimpleNix(gageSimple *spl) {

  if (spl) {
    if (spl->npad)     /* we did allocate this one */
      spl->npad = nrrdNuke(spl->npad);
    if (spl->ctx)      /* and this */
      spl->ctx = gageContextNix(spl->ctx);
    if (spl->pvl)      /* and this */
      spl->pvl= gagePerVolumeNix(spl->pvl);
    /* but we don't spl->kind->ansNix(spl->ans) because spl->ans is
       just a copy of the answer point already freed by
       gagePerVolumeNix(spl->pvl) */
    free(spl);
  }
  return NULL;
}
