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

miteUser *
miteUserNew() {
  miteUser *muu;
  int i;

  muu = (miteUser *)calloc(1, sizeof(miteUser));
  if (!muu)
    return NULL;

  muu->nin = NULL;
  for (i=0; i<MITE_TXF_NUM; i++) {
    muu->txf[i] = NULL;
  }
  muu->txfNum = 0;
  for (i=0; i<MITE_RANGE_NUM; i++) {
    muu->rangeInit[i] = 1.0;
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
  airMopAdd(muu->mop, muu->lit, (airMopper)limnLightNix, airMopAlways);
  return muu;
}

miteUser *
miteUserNix(miteUser *muu) {

  if (muu) {
    airMopOkay(muu->mop);
    AIR_FREE(muu);
  }
  return NULL;
}

int
miteUserValid(miteUser *muu) { 
  char me[]="miteUserValid", err[AIR_STRLEN_MED];

  if (!gageVolumeValid(muu->nin, gageKindScl)) {
    sprintf(err, "%s: trouble with input volume", me);
    biffMove(MITE, err, GAGE); return 0;
  }
  if (!muu->txfNum) {
    sprintf(err, "%s: no transfer functions set", me);
    biffAdd(MITE, err); return 0;
  }
  return 1;
}
