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

int
gageSimpleUpdate(gageSimple *gsl) {
  char me[]="gageSimpleUpdate", err[AIR_STRLEN_MED];
  int i, needPad, min[NRRD_DIM_MAX], max[NRRD_DIM_MAX];

  if (!gsl) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(GAGE, err); return 1;
  }
  /* context already created by gageSimpleNew */
  for (i=gageKernelUnknown+1; i<gageKernelLast; i++) {
    if (gsl->k[i]) {
      if (gageKernelSet(gsl->ctx, i, gsl->k[i], gsl->kparm[i])) {
	sprintf(err, "%s: trouble setting kernel %s", me,
		airEnumStr(gageKernel, i));
	biffAdd(GAGE, err); return 1;
      }
    }
  }
  needPad = gageValGet(gsl->ctx, gageValNeedPad);
  if (!(gsl->pvl = gagePerVolumeNew(needPad, gsl->kind))) {
    sprintf(err, "%s: trouble creating pervolume", me);
    biffAdd(GAGE, err); return 1;
  }
  if (gageQuerySet(gsl->pvl, gsl->query)) {
    sprintf(err, "%s: trouble setting query", me);
    biffAdd(GAGE, err); return 1;
  }
  /* to create npad, the non-{x,y,z} axes are not altered */
  for (i=0; i<gsl->kind->baseDim; i++) {
    min[i] = 0;
    max[i] = gsl->nin->axis[i].size - 1;
  }
  /* the x,y,z axes are padded by needPad above and below */
  min[0 + gsl->kind->baseDim] = -needPad;
  min[1 + gsl->kind->baseDim] = -needPad;
  min[2 + gsl->kind->baseDim] = -needPad;
  max[0 + gsl->kind->baseDim] = gsl->nin->axis[0].size + needPad;
  max[1 + gsl->kind->baseDim] = gsl->nin->axis[1].size + needPad;
  max[2 + gsl->kind->baseDim] = gsl->nin->axis[2].size + needPad;
  if (nrrdPad(gsl->npad=nrrdNew(), gsl->nin, min, max, nrrdBoundaryBleed)) {
    sprintf(err, "%s: trouble padding input volume", me);
    biffMove(GAGE, err, NRRD); return 1;
  }
  if (gageVolumeSet(gsl->ctx, gsl->pvl, gsl->npad, needPad)) {
    sprintf(err, "%s: trouble setting padded volume", me);
    biffAdd(GAGE, err); return 1;
  }
  if (gageUpdate(gsl->ctx, gsl->pvl)) {
    sprintf(err, "%s: trouble", me);
    biffAdd(GAGE, err); return 1;
  }
  gsl->ans = gsl->pvl->ans;

  return 0;
}

int
gageSimpleKernelSet(gageSimple *gsl,
		    int which, NrrdKernel *k, double *kparm) {
  char me[]="gageSimpleKernelSet", err[AIR_STRLEN_MED];
  
  if (!(gsl && k && kparm)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(GAGE, err); return 1;
  }
  if (!airEnumValidVal(gageKernel, which)) {
    sprintf(err, "%s: \"which\" (%d) not in range [%d,%d]", me,
	    which, gageKernelUnknown+1, gageKernelLast-1);
    biffAdd(GAGE, err); return 1;
  }

  gsl->k[which] = k;
  memcpy(gsl->kparm[which], kparm, NRRD_KERNEL_PARMS_NUM*sizeof(double));
  return 0;
}

int
gageSimpleProbe(gageSimple *gsl, gage_t x, gage_t y, gage_t z) {
  
  return gageProbe(gsl->ctx, gsl->pvl, x, y, z);
}
