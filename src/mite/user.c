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

miteUserInfo *
miteUserInfoNew(hoovContext *ctx) {
  miteUserInfo *muu;

  fprintf(stderr, "%s: ctx = %p\n", "miteUserInfoNew", ctx);
  muu = (miteUserInfo *)calloc(1, sizeof(miteUserInfo));
  fprintf(stderr, "%s: muu = %p\n", "miteUserInfoNew", muu);
  if (muu) {
    muu->nin = NULL;
    muu->ntf = NULL;
    muu->refStep = miteDefRefStep;
    muu->rayStep = AIR_NAN;
    muu->ksp00 = muu->ksp11 = muu->ksp22 = NULL;
    muu->ctx = ctx;
    muu->lit = limnLightNew();
    muu->renorm = miteDefRenorm;
    muu->sum = AIR_FALSE;

    muu->mop = airMopInit();

    muu->outS = NULL;  /* allocated by hest */
  }
  fprintf(stderr, "%s: muu->ctx = %p\n", "miteUserInfoNew", muu->ctx);
  return muu;
}

miteUserInfo *
miteUserInfoNix(miteUserInfo *muu) {

  if (muu) {
    limnLightNix(muu->lit);
    airMopOkay(muu->mop);
    airFree(muu);
  }
  return NULL;
}


