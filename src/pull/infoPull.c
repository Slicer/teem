/*
  Teem: Tools to process and visualize scientific data and images              
  Copyright (C) 2008, 2007, 2006, 2005  Gordon Kindlmann
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

  You should have received a copy of the GNU Lesser General Public License
  along with this library; if not, write to Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "pull.h"
#include "privatePull.h"

const char *
_pullInfoStr[] = {
  "(unknown pullInfo)",
  "ten",
  "teni",
  "hess",
  "in",
  "ingradvec",
  "hght",
  "hghtgradvec",
  "hghthessian",
  "hghtlapl",
  "seedprethresh",
  "seedthresh",
  "livethresh",
  "livethresh2",
  "livethresh3",
  "tan1",
  "tan2",
  "tanmode",
  "isoval",
  "isogradvec",
  "isohessian",
  "strength",
  "quality"
};

const int
_pullInfoVal[] = {
  pullInfoUnknown,
  pullInfoTensor,             /* [7] tensor here */
  pullInfoTensorInverse,      /* [7] inverse of tensor here */
  pullInfoHessian,            /* [9] hessian used for force distortion */
  pullInfoInside,             /* [1] containment scalar */
  pullInfoInsideGradient,     /* [3] containment vector */
  pullInfoHeight,             /* [1] for gravity */
  pullInfoHeightGradient,     /* [3] for gravity */
  pullInfoHeightHessian,      /* [9] for gravity */
  pullInfoHeightLaplacian,    /* [1]  */
  pullInfoSeedPreThresh,      /* [1] */
  pullInfoSeedThresh,         /* [1] scalar for thresholding seeding */
  pullInfoLiveThresh,         /* [1] */ 
  pullInfoLiveThresh2,        /* [1] */ 
  pullInfoLiveThresh3,        /* [1] */ 
  pullInfoTangent1,           /* [3] first tangent to constraint surf */
  pullInfoTangent2,           /* [3] second tangent to constraint surf */
  pullInfoTangentMode,        /* [1] for morphing between co-dim 1 and 2 */
  pullInfoIsovalue,           /* [1] */
  pullInfoIsovalueGradient,   /* [3] */
  pullInfoIsovalueHessian,    /* [9] */
  pullInfoStrength,           /* [1] */
  pullInfoQuality             /* [1] */
};

const char *
_pullInfoStrEqv[] = {
  "ten",
  "teni",
  "hess",
  "in",
  "ingradvec",
  "hght", "h",
  "hghtgradvec", "hgvec",
  "hghthessian", "hhess",
  "hghtlapl", "hlapl",
  "seedprethresh", "spthr",
  "seedthresh", "sthr",
  "livethresh", "lthr",
  "livethresh2", "lthr2",
  "livethresh3", "lthr3",
  "tan1",
  "tan2",
  "tanmode", "tmode",
  "isoval", "iso",
  "isogradvec", "isogvec",
  "isohessian", "isohess",
  "strength", "strn",
  "quality", "qual",
  ""
};

const int
_pullInfoValEqv[] = {
  pullInfoTensor,
  pullInfoTensorInverse,
  pullInfoHessian,
  pullInfoInside,
  pullInfoInsideGradient,
  pullInfoHeight, pullInfoHeight,
  pullInfoHeightGradient, pullInfoHeightGradient,
  pullInfoHeightHessian, pullInfoHeightHessian,
  pullInfoHeightLaplacian, pullInfoHeightLaplacian,
  pullInfoSeedPreThresh, pullInfoSeedPreThresh,
  pullInfoSeedThresh, pullInfoSeedThresh,
  pullInfoLiveThresh, pullInfoLiveThresh,
  pullInfoLiveThresh2, pullInfoLiveThresh2,
  pullInfoLiveThresh3, pullInfoLiveThresh3,
  pullInfoTangent1,
  pullInfoTangent2,
  pullInfoTangentMode, pullInfoTangentMode,
  pullInfoIsovalue, pullInfoIsovalue,
  pullInfoIsovalueGradient, pullInfoIsovalueGradient,
  pullInfoIsovalueHessian, pullInfoIsovalueHessian,
  pullInfoStrength, pullInfoStrength,
  pullInfoQuality, pullInfoQuality
};

const airEnum
_pullInfo = {
  "pullInfo",
  PULL_INFO_MAX,
  _pullInfoStr, _pullInfoVal,
  NULL,
  _pullInfoStrEqv, _pullInfoValEqv,
  AIR_FALSE
};
const airEnum *const
pullInfo = &_pullInfo;

unsigned int
_pullInfoAnswerLen[PULL_INFO_MAX+1] = {
  0, /* pullInfoUnknown */
  7, /* pullInfoTensor */
  7, /* pullInfoTensorInverse */
  9, /* pullInfoHessian */
  1, /* pullInfoInside */
  3, /* pullInfoInsideGradient */
  1, /* pullInfoHeight */
  3, /* pullInfoHeightGradient */
  9, /* pullInfoHeightHessian */
  1, /* pullInfoHeightLaplacian */
  1, /* pullInfoSeedPreThresh */
  1, /* pullInfoSeedThresh */
  1, /* pullInfoLiveThresh */
  1, /* pullInfoLiveThresh2 */
  1, /* pullInfoLiveThresh3 */
  3, /* pullInfoTangent1 */
  3, /* pullInfoTangent2 */
  1, /* pullInfoTangentMode */
  1, /* pullInfoIsovalue */
  3, /* pullInfoIsovalueGradient */
  9, /* pullInfoIsovalueHessian */
  1, /* pullInfoStrength */
  1, /* pullInfoQuality */
}; 

unsigned int
pullInfoAnswerLen(int info) {
  unsigned int ret;
  
  if (!airEnumValCheck(pullInfo, info)) {
    ret = _pullInfoAnswerLen[info];
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
    ispec->item = 0;
    ispec->scale = AIR_NAN;
    ispec->zero = AIR_NAN;
    ispec->constraint = AIR_FALSE;
    ispec->volIdx = UINT_MAX;
  }
  return ispec;
}

pullInfoSpec *
pullInfoSpecNix(pullInfoSpec *ispec) {

  if (ispec) {
    ispec->volName = airFree(ispec->volName);
    airFree(ispec);
  }
  return NULL;
}

int
pullInfoSpecAdd(pullContext *pctx, pullInfoSpec *ispec) {
  static const char me[]="pullInfoSpecAdd";
  unsigned int ii, vi, haveLen, needLen;
  const gageKind *kind;
  
  if (!( pctx && ispec )) {
    biffAddf(PULL, "%s: got NULL pointer", me);
    return 1;
  }
  if (airEnumValCheck(pullInfo, ispec->info)) {
    biffAddf(PULL, "%s: %d not a valid %s value", me,
             ispec->info, pullInfo->name);
    return 1;
  }
  if (pctx->ispec[ispec->info]) {
    biffAddf(PULL, "%s: already set info %s (%d)", me, 
             airEnumStr(pullInfo, ispec->info), ispec->info);
    return 1;
  }
  for (ii=0; ii<=PULL_INFO_MAX; ii++) {
    if (pctx->ispec[ii] == ispec) {
      biffAddf(PULL, "%s(%s): already got ispec %p as ispec[%u]", me,
               airEnumStr(pullInfo, ispec->info), ispec, ii);
      return 1;
    }
  }
  vi = _pullVolumeIndex(pctx, ispec->volName);
  if (UINT_MAX == vi) {
    biffAddf(PULL, "%s(%s): no volume has name \"%s\"", me,
             airEnumStr(pullInfo, ispec->info), ispec->volName);
    return 1;
  }
  kind = pctx->vol[vi]->kind;
  if (airEnumValCheck(kind->enm, ispec->item)) {
    biffAddf(PULL, "%s(%s): %d not a valid \"%s\" item", me, 
             airEnumStr(pullInfo, ispec->info), ispec->item, kind->name);
    return 1;
  }
  needLen = pullInfoAnswerLen(ispec->info);
  haveLen = kind->table[ispec->item].answerLength;
  if (needLen != haveLen) {
    biffAddf(PULL, "%s(%s): needs len %u, but \"%s\" item \"%s\" has len %u",
             me, airEnumStr(pullInfo, ispec->info), needLen,
             kind->name, airEnumStr(kind->enm, ispec->item), haveLen);
    return 1;
  }
  if (haveLen > 9) {
    biffAddf(PULL, "%s: sorry, answer length (%u) > 9 unsupported", me,
             haveLen);
    return 1;
  }

  /* very tricky: seedOnly is initialized to true for everything */
  if (pullInfoSeedThresh != ispec->info
      && pullInfoSeedPreThresh != ispec->info) {
    /* if the info is neither seedthreh nor seedprethresh, then the
       volume will have to be probed after the first iter, so turn
       off seedOnly */
    pctx->vol[vi]->seedOnly = AIR_FALSE;
  }
  
  /* now set item in gage query */
  if (gageQueryItemOn(pctx->vol[vi]->gctx, pctx->vol[vi]->gpvl, ispec->item)) {
    biffMovef(PULL, GAGE, "%s: trouble adding item %u to vol %u", me,
              ispec->item, vi);
    return 1;
  }

  ispec->volIdx = vi;
  pctx->ispec[ispec->info] = ispec;
  
  return 0;
}

/*
** sets:
** pctx->infoIdx[]
** pctx->infoTotalLen
** pctx->constraint
** pctx->constraintDim
** pctx->targetDim (non-trivial logic for scale-space!)
*/
int
_pullInfoSetup(pullContext *pctx) {
  static const char me[]="_pullInfoSetup";
  unsigned int ii;

  pctx->infoTotalLen = 0;
  pctx->constraint = 0;
  pctx->constraintDim = 0;
  for (ii=0; ii<=PULL_INFO_MAX; ii++) {
    if (pctx->ispec[ii]) {
      pctx->infoIdx[ii] = pctx->infoTotalLen;
      if (pctx->verbose) {
        printf("!%s: infoIdx[%u] (%s) = %u\n", me,
               ii, airEnumStr(pullInfo, ii), pctx->infoIdx[ii]);
      }
      pctx->infoTotalLen += pullInfoAnswerLen(ii);
      if (!pullInfoAnswerLen(ii)) {
        biffAddf(PULL, "%s: got zero-length answer for ispec[%u]", me, ii);
        return 1;
      }
      if (pctx->ispec[ii]->constraint) {
        pullVolume *cvol;
        pctx->constraint = ii;
        cvol = pctx->vol[pctx->ispec[ii]->volIdx];
      }
    }
  }
  if (pctx->constraint) {
    if (pctx->ispec[pullInfoTangentMode]) {
      pctx->constraintDim = 1.5;
    } else {
      pctx->constraintDim = _pullConstraintDim(pctx, NULL, NULL);
      if (!pctx->constraintDim) {
        biffAddf(PULL, "%s: got constr dim 0.0", me);
        return 1;
      }
    }
    if (pctx->haveScale) {
      double *parmS, denS,
        (*evalS)(double *, double, const double parm[PULL_ENERGY_PARM_NUM]);
      switch (pctx->interType) {
      case pullInterTypeUnivariate:
      case pullInterTypeSeparable:
        /* assume repulsive along both r and s */
        pctx->targetDim = 1 + pctx->constraintDim;
        break;
      case pullInterTypeAdditive:
        parmS = pctx->energySpecS->parm;
        evalS = pctx->energySpecS->energy->eval;
        evalS(&denS, _PULL_TARGET_DIM_S_PROBE, parmS);
        if (denS > 0) {
          /* at small positive s, derivative was positive ==> attractive */
          pctx->targetDim = pctx->constraintDim;
        } else {
          /* derivative was negative ==> repulsive */
          pctx->targetDim = 1 + pctx->constraintDim;
        }
        break;
      default:
        biffAddf(PULL, "%s: sorry, intertype %s not handled here", me, 
                 airEnumStr(pullInterType, pctx->interType));
        break;
      }
    } else {
      pctx->targetDim = pctx->constraintDim;
    }
  } else {
    pctx->constraintDim = 0;
    pctx->targetDim = 0;
  }
  if (pctx->verbose) {
    printf("!%s: infoTotalLen=%u, constr=%d, constr,targetDim = %g,%g\n", 
           me, pctx->infoTotalLen, pctx->constraint,
           pctx->constraintDim, pctx->targetDim);
  }
  return 0;
}

static void
_infoCopy1(double *dst, const double *src) {
  dst[0] = src[0]; 
}

static void
_infoCopy2(double *dst, const double *src) {
  dst[0] = src[0]; dst[1] = src[1];
}

static void
_infoCopy3(double *dst, const double *src) {
  dst[0] = src[0]; dst[1] = src[1]; dst[2] = src[2];
}

static void
_infoCopy4(double *dst, const double *src) {
  dst[0] = src[0]; dst[1] = src[1]; dst[2] = src[2]; dst[3] = src[3];
}

static void
_infoCopy5(double *dst, const double *src) {
  dst[0] = src[0]; dst[1] = src[1]; dst[2] = src[2]; dst[3] = src[3];
  dst[4] = src[4];
}

static void
_infoCopy6(double *dst, const double *src) {
  dst[0] = src[0]; dst[1] = src[1]; dst[2] = src[2]; dst[3] = src[3];
  dst[4] = src[4]; dst[5] = src[5];
}

static void
_infoCopy7(double *dst, const double *src) {
  dst[0] = src[0]; dst[1] = src[1]; dst[2] = src[2]; dst[3] = src[3];
  dst[4] = src[4]; dst[5] = src[5]; dst[6] = src[6];
}

static void
_infoCopy8(double *dst, const double *src) {
  dst[0] = src[0]; dst[1] = src[1]; dst[2] = src[2]; dst[3] = src[3];
  dst[4] = src[4]; dst[5] = src[5]; dst[6] = src[6]; dst[7] = src[7];
}

static void
_infoCopy9(double *dst, const double *src) {
  dst[0] = src[0]; dst[1] = src[1]; dst[2] = src[2]; dst[3] = src[3];
  dst[4] = src[4]; dst[5] = src[5]; dst[6] = src[6]; dst[7] = src[7];
  dst[8] = src[8];
}

void (*_pullInfoAnswerCopy[10])(double *, const double *) = {
  NULL,
  _infoCopy1,
  _infoCopy2,
  _infoCopy3,
  _infoCopy4,
  _infoCopy5,
  _infoCopy6,
  _infoCopy7,
  _infoCopy8,
  _infoCopy9
};

