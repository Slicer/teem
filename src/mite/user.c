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

char
miteTxfIdent[] = "RGBAEadsp";

miteUserInfo *
miteUserInfoNew() {
  miteUserInfo *muu;
  int i;

  muu = (miteUserInfo *)calloc(1, sizeof(miteUserInfo));
  if (muu) {
    muu->nin = NULL;
    for (i=0; i<GAGE_KERNEL_NUM; i++) {
      muu->ntxf[i] = NULL;
    }
    muu->refStep = miteDefRefStep;
    muu->rayStep = AIR_NAN;
    muu->near1 = miteDefNear1;
    muu->hctx = hooverContextNew();
    for (i=0; i<GAGE_KERNEL_NUM; i++) {
      muu->ksp[i] = NULL;
    }
    muu->gctx0 = gageContextNew();
    muu->lit = limnLightNew();
    muu->justSum = AIR_FALSE;
    muu->noDirLight = AIR_FALSE;
    muu->outS = NULL;  /* managed by hest */
    muu->mop = airMopNew();
    airMopAdd(muu->mop, muu->hctx, (airMopper)hooverContextNix, airMopAlways);
    airMopAdd(muu->mop, muu->gctx0, (airMopper)gageContextNix, airMopAlways);
    airMopAdd(muu->mop, muu->gctx0, (airMopper)limnLightNix, airMopAlways);
  }
  return muu;
}

miteUserInfo *
miteUserInfoNix(miteUserInfo *muu) {

  if (muu) {
    airMopOkay(muu->mop);
    airFree(muu);
  }
  return NULL;
}

int
_miteNtxfValid(Nrrd *ntxf) {
  char me[]="_miteNtxfValid", err[AIR_STRLEN_MED];
  int i;
  
  if (!nrrdValid(ntxf)) {
    sprintf(err, "%s: basic nrrd validity check failed", me);
    biffMove(GAGE, err, NRRD); return 1;
  }
  if (nrrdTypeFloat != ntxf->type) {
    sprintf(err, "%s: need a type %s nrrd (not %s)", me,
	    airEnumStr(nrrdType, nrrdTypeFloat),
	    airEnumStr(nrrdType, ntxf->type));
    biffAdd(GAGE, err); return 1;
  }
  if (0 == airStrlen(ntxf->content)) {
    sprintf(err, "%s: given nrrd's \"content\" doesn't specify txf range", me);
    biffAdd(GAGE, err); return 1;
  }
  
  return 0;
}

int
miteUserInfoValid(miteUserInfo *muu) { 
  char me[]="miteUserInfoValid", err[AIR_STRLEN_MED];

  if (!gageVolumeValid(muu->nin, gageKindScl)) {
    sprintf(err, "%s: trouble with input volume", me);
    biffMove(MITE, err, GAGE); return 0;
  }
  return 1;
}


