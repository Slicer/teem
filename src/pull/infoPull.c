/*
  Teem: Tools to process and visualize scientific data and images
  Copyright (C) 2006, 2005  Gordon Kindlmann
  Copyright (C) 2004, 2003, 2002, 2001, 2000, 1999, 1998  University of Utah

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public License
  (LGPL) as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  The terms of redistributing and/or modifying this software also
  include exceptions to the LGPL that facilitate static linking.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "pull.h"
#include "privatePull.h"

char
_pullInfoStr[][AIR_STRLEN_SMALL] = {
  "(unknown pullInfo)",
  "ten",
  "teni",
  "hess",
  "in",
  "ingradvec",
  "hght",
  "hghtgradvec",
  "hghthessian",
  "seedthresh",
  "tan1",
  "tan2",
  "tanmode",
  "isoval",
  "isogradvec",
  "isohessian",
};

int
_pullInfoVal[] = {
  pullInfoUnknown,
  pullInfoTensor,             /*  1: [7] tensor here */
  pullInfoTensorInverse,      /*  2: [7] inverse of tensor here */
  pullInfoHessian,            /*  3: [9] hessian used for force distortion */
  pullInfoInside,             /*  4: [1] containment scalar */
  pullInfoInsideGradient,     /*  5: [3] containment vector */
  pullInfoHeight,             /*  6: [1] for gravity */
  pullInfoHeightGradient,     /*  7: [3] for gravity */
  pullInfoHeightHessian,      /*  8: [9] for gravity */
  pullInfoSeedThresh,         /*  9: [1] scalar for thresholding seeding */
  pullInfoTangent1,           /* 10: [3] first tangent to constraint surf */
  pullInfoTangent2,           /* 11: [3] second tangent to constraint surf */
  pullInfoTangentMode,        /* 12: [1] for morphing between co-dim 1 and 2 */
  pullInfoIsosurfaceValue,    /* 13: [1] */
  pullInfoIsosurfaceGradient, /* 14: [3] */
  pullInfoIsosurfaceHessian,  /* 15: [9] */
};

airEnum
_pullInfo = {
  "pullInfo",
  PULL_INFO_MAX+1,
  _pullInfoStr, _pullInfoVal,
  NULL,
  NULL, NULL,
  AIR_FALSE
};
airEnum *const
pullInfo = &_pullInfo;

unsigned int
_pullInfoLen[PULL_INFO_MAX+1] = {
  0, /* pullInfoUnknown */
  7, /* pullInfoTensor */
  7, /* pullInfoTensorInverse */
  9, /* pullInfoHessian */
  1, /* pullInfoInside */
  3, /* pullInfoInsideGradient */
  1, /* pullInfoHeight */
  3, /* pullInfoHeightGradient */
  9, /* pullInfoHeightHessian */
  1, /* pullInfoSeedThresh */
  3, /* pullInfoTangent1 */
  3, /* pullInfoTangent2 */
  1, /* pullInfoTangentMode */
  1, /* pullInfoIsosurfaceValue */
  3, /* pullInfoIsosurfaceGradient */
  9, /* pullInfoIsosurfaceHessian */
}; 

unsigned int
pullInfoLen(int info) {
  unsigned int ret;
  
  if (!airEnumValCheck(pullInfo, info)) {
    ret = _pullInfoLen[info];
  } else {
    ret = 0;
  }
  return ret;
}

pullInfoSpec *
pullInfoSpecNew(void) {
  pullInfoSpec *ispec;

  ispec = AIR_CAST(pullInfoSpec *, calloc(1, sizeof(pullInfoSpec)));
  if (ispec) {
    ispec->info = pullInfoUnknown;
    ispec->volName = NULL;
    ispec->itemName = NULL;
    ispec->scaling = AIR_NAN;
    ispec->thresh = AIR_NAN;
    ispec->zero = AIR_NAN;
    ispec->volIdx = UINT_MAX;
    ispec->item = 0;
  }
  return ispec;
}

int
pullInfoSpecAdd(pullContext *pctx, pullInfoSpec *ispec,
                int info, const char *volName, const char *itemName) {
  char me[]="pullInfoSpecAdd", err[BIFF_STRLEN];
  unsigned int ii, haveLen, needLen;
  const gageKind *kind;
  const pullVolume *vol;
  int item;
  
  if (!( pctx && ispec && volName && itemName )) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(PULL, err); return 1;
  }
  if (airEnumValCheck(pullInfo, info)) {
    sprintf(err, "%s: %d not a valid %s value", me, info, pullInfo->name);
    biffAdd(PULL, err); return 1;
  }
  for (ii=0; ii<pctx->ispecNum; ii++) {
    if (pctx->ispec[ii]->info == info) {
      sprintf(err, "%s: existing ispec[%u] already has info %u (%s)", me, 
              ii, info, airEnumStr(pullInfo, info));
      biffAdd(PULL, err); return 1;
    }
  }
  if (0 == pctx->volNum) {
    sprintf(err, "%s: given context has no volumes", me);
    biffAdd(PULL, err); return 1;
  }
  for (ii=0; ii<pctx->volNum; ii++) {
    if (!strcmp(pctx->vol[ii]->name, volName)) {
      break;
    }
  }
  if (ii == pctx->volNum) {
    sprintf(err, "%s: no volume has name \"%s\"", me, volName);
    biffAdd(PULL, err); return 1;
  }
  vol = pctx->vol[ii];
  kind = vol->kind;
  item = airEnumVal(kind->enm, itemName);
  if (!item) {
    sprintf(err, "%s: \"%s\" not a valid \"%s\" item", me, 
            itemName, kind->name);
    biffAdd(PULL, err); return 1;
  }
  needLen = pullInfoLen(info);
  haveLen = kind->table[item].answerLength;
  if (needLen != haveLen) {
    sprintf(err, "%s: info \"%s\" needs len %u, "
            "but \"%s\" item \"%s\" has len %u",
            me, airEnumStr(pullInfo, info), needLen,
            kind->name, airEnumStr(kind->enm, item), haveLen);
    biffAdd(PULL, err); return 1;
  }

  ispec->info = info;
  ispec->volName = airStrdup(volName);
  ispec->itemName = airStrdup(itemName);
  ispec->volIdx = ii;
  ispec->item = item;

  /* now set item in gage query */
  gageQueryItemOn(vol->gctx, vol->gpvl, item);

  pctx->ispec[pctx->ispecNum++] = ispec;
  
  return 0;
}

pullInfoSpec *
pullInfoSpecNix(pullInfoSpec *ispec) {

  if (ispec) {
    ispec->volName = airFree(ispec->volName);
    ispec->itemName = airFree(ispec->itemName);
    airFree(ispec);
  }
  return NULL;
}

