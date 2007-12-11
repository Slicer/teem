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

pullContext *
pullContextNew(void) {
  pullContext *pctx;
  unsigned int ii;

  pctx = (pullContext *)calloc(1, sizeof(pullContext));
  if (!pctx) {
    return NULL;
  }
  
  pctx->verbose = 0;
  pctx->pointNum = 0;
  pctx->npos = NULL;
  for (ii=0; ii<PULL_VOLUME_MAXNUM; ii++) {
    pctx->vol[ii] = NULL;
  }
  pctx->volNum = 0;
  for (ii=0; ii<=PULL_INFO_MAX; ii++) {
    pctx->ispec[ii] = NULL;
  }

  pctx->stepInitial = 1;
  pctx->interScl = 1;
  pctx->wallScl = 0;
  pctx->neighborTrueProb = 1;
  pctx->probeProb = 1;
  pctx->deltaLimit = 0.3;
  pctx->deltaFracMin = 0.2;
  pctx->energyStepFrac = 0.9;
  pctx->deltaFracStepFrac = 0.5;
  pctx->energyImprovMin = 0.01;

  pctx->seedRNG = 42;
  pctx->threadNum = 1;
  pctx->maxIter = 0;
  pctx->snap = 0;
  
  pctx->ensp = pullEnergySpecNew();
  
  pctx->binSingle = AIR_FALSE;
  pctx->binIncr = 32;

  ELL_3V_SET(pctx->bboxMin, AIR_NAN, AIR_NAN, AIR_NAN);
  ELL_3V_SET(pctx->bboxMax, AIR_NAN, AIR_NAN, AIR_NAN);
  pctx->infoTotalLen = 0; /* will be set later */
  pctx->idtagNext = 0;
  pctx->finished = AIR_FALSE;

  pctx->bin = NULL;
  ELL_3V_SET(pctx->binsEdge, 0, 0, 0);
  pctx->binNum = 0;
  pctx->binIdx = 0;
  pctx->binMutex = NULL;

  pctx->step = AIR_NAN;
  pctx->maxDist = AIR_NAN;
  pctx->energySum = 0;
  pctx->task = NULL;
  pctx->iterBarrierA = NULL;
  pctx->iterBarrierB = NULL;
  pctx->deltaFrac = AIR_NAN;

  pctx->timeIteration = 0;
  pctx->timeRun = 0;
  pctx->iter = 0;
  pctx->noutPos = nrrdNew();
  return pctx;
}

/*
** this should only nix things created by pullContextNew
*/
pullContext *
pullContextNix(pullContext *pctx) {
  unsigned int ii;
  
  if (pctx) {
    for (ii=0; ii<pctx->volNum; ii++) {
      pctx->vol[ii] = pullVolumeNix(pctx->vol[ii]);
    }
    pctx->volNum = 0;
    for (ii=0; ii<=PULL_INFO_MAX; ii++) {
      if (pctx->ispec[ii]) {
        pctx->ispec[ii] = pullInfoSpecNix(pctx->ispec[ii]);
      }
    }
    pctx->ensp = pullEnergySpecNix(pctx->ensp);
    pctx->noutPos = nrrdNuke(pctx->noutPos);
    airFree(pctx);
  }
  return NULL;
}

int
_pullContextCheck(pullContext *pctx) {
  char me[]="_pullContextCheck", err[BIFF_STRLEN];
  unsigned int ii;
  int gotIspec;

  if (!pctx) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(PULL, err); return 1;
  }
  if (pctx->npos) {
    if (nrrdCheck(pctx->npos)) {
      sprintf(err, "%s: got a broken npos", me);
      biffMove(PULL, err, NRRD); return 1;
    }
    if (!( 2 == pctx->npos->dim 
           && 4 == pctx->npos->axis[0].size
           && nrrdTypeDouble == pctx->npos->type )) {
      sprintf(err, "%s: npos not a 2-D 4-by-N array of %s "
              "(got %u-D %u-by-X of %s)", me,
              airEnumStr(nrrdType, nrrdTypeDouble),
              pctx->npos->dim,
              AIR_CAST(unsigned int, pctx->npos->axis[0].size),
              airEnumStr(nrrdType, pctx->npos->type));
      biffAdd(PULL, err); return 1;
    }
  } else {
    if (!( pctx->pointNum >= 1 )) {
      sprintf(err, "%s: pctx->pointNum (%d) not >= 1\n", me, pctx->pointNum);
      biffAdd(PULL, err); return 1;
    }
  }
  if (!pctx->volNum) {
    sprintf(err, "%s: have no volumes set", me);
    biffAdd(PULL, err); return 1;
  }
  gotIspec = AIR_FALSE;
  for (ii=0; ii<=PULL_INFO_MAX; ii++) {
    if (pctx->ispec[ii]) {
      /* make sure we have extra info as necessary */
      switch (ii) {
      case pullInfoInside:
      case pullInfoHeight:
      case pullInfoIsosurfaceValue:
        if (!( AIR_EXISTS(pctx->ispec[ii]->scaling)
               && AIR_EXISTS(pctx->ispec[ii]->zero) )) {
          sprintf(err, "%s: %s info needs scaling (%g) and zero (%g)", me, 
                  airEnumStr(pullInfo, ii),
                  pctx->ispec[ii]->scaling, pctx->ispec[ii]->zero);
          biffAdd(PULL, err); return 1;
        }
        break;
      }
      gotIspec = AIR_TRUE;
    }
  }
  if (!gotIspec) {
    sprintf(err, "%s: have no infos set", me);
    biffAdd(PULL, err); return 1;
  }
  if (pctx->ispec[pullInfoInside]) {
    if (!pctx->ispec[pullInfoInsideGradient]) {
      sprintf(err, "%s: want %s but don't have %s set", me, 
              airEnumStr(pullInfo, pullInfoInside),
              airEnumStr(pullInfo, pullInfoInsideGradient));
      biffAdd(PULL, err); return 1;
    }
  }
  if (pctx->ispec[pullInfoHeight]) {
    if (!( pctx->ispec[pullInfoHeightGradient]
           && pctx->ispec[pullInfoHeightHessian] )) {
      sprintf(err, "%s: want %s but don't have %s and %s set", me, 
              airEnumStr(pullInfo, pullInfoHeight),
              airEnumStr(pullInfo, pullInfoHeightGradient),
              airEnumStr(pullInfo, pullInfoHeightHessian));
      biffAdd(PULL, err); return 1;
    }
  }
  if (pctx->ispec[pullInfoIsosurfaceValue]) {
    if (!( pctx->ispec[pullInfoIsosurfaceGradient]
           && pctx->ispec[pullInfoIsosurfaceHessian] )) {
      sprintf(err, "%s: want %s but don't have %s and %s set", me, 
              airEnumStr(pullInfo, pullInfoIsosurfaceValue),
              airEnumStr(pullInfo, pullInfoIsosurfaceGradient),
              airEnumStr(pullInfo, pullInfoIsosurfaceHessian));
      biffAdd(PULL, err); return 1;
    }
  }
  if (!( AIR_IN_CL(1, pctx->threadNum, PULL_THREAD_MAXNUM) )) {
    sprintf(err, "%s: pctx->threadNum (%d) outside valid range [1,%d]", me,
            pctx->threadNum, PULL_THREAD_MAXNUM);
    biffAdd(PULL, err); return 1;
  }

  return 0;
}

int
pullOutputGet(Nrrd *nPosOut, Nrrd *nTenOut, Nrrd *nEnrOut, pullContext *pctx) {
  char me[]="pullOutputGet", err[BIFF_STRLEN];
  unsigned int binIdx, pointRun, pointNum, pointIdx;
  int E;
  float *posOut, *tenOut, *enrOut;
  pullBin *bin;
  pullPoint *point;
  double sclmin, sclmax, sclmean;

  pointNum = _pullPointTotal(pctx);
  E = AIR_FALSE;
  if (nPosOut) {
    E |= nrrdMaybeAlloc_va(nPosOut, nrrdTypeFloat, 2,
                           AIR_CAST(size_t, 3),
                           AIR_CAST(size_t, pointNum));
  }
  if (nTenOut) {
    E |= nrrdMaybeAlloc_va(nTenOut, nrrdTypeFloat, 2, 
                           AIR_CAST(size_t, 7),
                           AIR_CAST(size_t, pointNum));
  }
  if (nEnrOut) {
    E |= nrrdMaybeAlloc_va(nEnrOut, nrrdTypeFloat, 1, 
                           AIR_CAST(size_t, pointNum));
  }
  if (E) {
    sprintf(err, "%s: trouble allocating outputs", me);
    biffMove(PULL, err, NRRD); return 1;
  }
  posOut = nPosOut ? (float*)(nPosOut->data) : NULL;
  tenOut = nTenOut ? (float*)(nTenOut->data) : NULL;
  enrOut = nEnrOut ? (float*)(nEnrOut->data) : NULL;

  pointRun = 0;
  sclmean = 0;
  sclmin = sclmax = AIR_NAN;
  for (binIdx=0; binIdx<pctx->binNum; binIdx++) {
    bin = pctx->bin + binIdx;
    for (pointIdx=0; pointIdx<bin->pointNum; pointIdx++) {
      point = bin->point[pointIdx];
      if (posOut) {
        ELL_3V_SET(posOut + 3*pointRun,
                   point->pos[0], point->pos[1], point->pos[2]);
      }
      if (tenOut) {
        TEN_T_SET(tenOut + 7*pointRun, 1, 1, 0, 0, 1, 0, 1);
      }
      if (enrOut) {
        enrOut[pointRun] = point->energy;
      }
      pointRun++;
    }
  }

  return 0;
}

