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
  pctx->pointNumInitial = 0;
  pctx->npos = NULL;
  for (ii=0; ii<PULL_VOLUME_MAXNUM; ii++) {
    pctx->vol[ii] = NULL;
  }
  pctx->volNum = 0;
  for (ii=0; ii<=PULL_INFO_MAX; ii++) {
    pctx->ispec[ii] = NULL;
    pctx->infoIdx[ii] = UINT_MAX;
  }

  pctx->stepInitial = 1;
  pctx->radiusSpace = 1;
  pctx->radiusScale = 1;
  pctx->neighborTrueProb = 1.0;
  pctx->probeProb = 1.0;
  pctx->opporStepScale = 1.0;
  pctx->stepScale = 0.5;
  pctx->energyImprovMin = 0.01;
  pctx->constraintStepMin = 0.00001;
  pctx->wall = 1;

  pctx->seedRNG = 42;
  pctx->threadNum = 1;
  pctx->iterMax = 0;
  pctx->constraintIterMax = 15;
  pctx->snap = 0;
  
  pctx->energySpec = pullEnergySpecNew();
  pctx->alpha = 0.5;
  pctx->beta = 1.0;
  pctx->radiusSingle = AIR_TRUE;

  pctx->binSingle = AIR_FALSE;
  pctx->binIncr = 32;

  ELL_4V_SET(pctx->bboxMin, AIR_NAN, AIR_NAN, AIR_NAN, AIR_NAN);
  ELL_4V_SET(pctx->bboxMax, AIR_NAN, AIR_NAN, AIR_NAN, AIR_NAN);
  pctx->infoTotalLen = 0; /* will be set later */
  pctx->idtagNext = 0;
  pctx->haveScale = AIR_FALSE;
  pctx->constraint = 0;
  pctx->finished = AIR_FALSE;
  pctx->maxDist = AIR_NAN;
  pctx->constraintVoxelSize = AIR_NAN;

  pctx->bin = NULL;
  ELL_3V_SET(pctx->binsEdge, 0, 0, 0);
  pctx->binNum = 0;
  pctx->binNextIdx = 0;
  pctx->pointPerm = NULL;
  pctx->pointBuff = NULL;
  pctx->binMutex = NULL;

  pctx->task = NULL;
  pctx->iterBarrierA = NULL;
  pctx->iterBarrierB = NULL;

  pctx->timeIteration = 0;
  pctx->timeRun = 0;
  pctx->stuckNum = 0;
  pctx->iter = 0;
  pctx->energy = AIR_NAN;
  pctx->noutPos = nrrdNew();
  return pctx;
}

/*
** this should only nix things created by pullContextNew, or the things
** (vols and ispecs) that were explicitly added to this context
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
    pctx->energySpec = pullEnergySpecNix(pctx->energySpec);
    /* handled elsewhere: bin, task, iterBarrierA, iterBarrierB */
    pctx->noutPos = nrrdNuke(pctx->noutPos);
    airFree(pctx);
  }
  return NULL;
}

int
_pullContextCheck(pullContext *pctx) {
  char me[]="_pullContextCheck", err[BIFF_STRLEN];
  unsigned int ii, sclvi;
  int gotIspec, gotConstr;

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
    if (!( pctx->pointNumInitial >= 1 )) {
      sprintf(err, "%s: pctx->pointNumInitial (%d) not >= 1\n", me,
              pctx->pointNumInitial);
      biffAdd(PULL, err); return 1;
    }
  }
  if (!pctx->volNum) {
    sprintf(err, "%s: have no volumes set", me);
    biffAdd(PULL, err); return 1;
  }
  for (ii=0; ii<pctx->volNum; ii++) {
    if (pctx->vol[ii]->ninScale) {
      sclvi = ii;
      for (ii=sclvi+1; ii<pctx->volNum; ii++) {
        if (pctx->vol[ii]->ninScale) {
          sprintf(err, "%s: can have only 1 scale volume (not both %u and %u)",
                  me, ii, sclvi);
          biffAdd(PULL, err); return 1;
        }
      }
    }
  }
  gotConstr = 0;
  gotIspec = AIR_FALSE;
  for (ii=0; ii<=PULL_INFO_MAX; ii++) {
    if (pctx->ispec[ii]) {
      if (pctx->ispec[ii]->constraint) {
        if (1 != pullInfoAnswerLen(ii)) {
          sprintf(err, "%s: can't use non-scalar (len %u) %s as constraint",
                  me, pullInfoAnswerLen(ii), airEnumStr(pullInfo, ii));
          biffAdd(PULL, err); return 1;
        }
        if (gotConstr) {
          sprintf(err, "%s: can't also have %s constraint, already have "
                  "constraint on %s ", me, airEnumStr(pullInfo, ii),
                  airEnumStr(pullInfo, gotConstr));
          biffAdd(PULL, err); return 1;
        }
        /* elso no problems having constraint on ii */
        gotConstr = ii;
      }
      /* make sure we have extra info as necessary */
      switch (ii) {
      case pullInfoInside:
      case pullInfoHeight:
      case pullInfoIsovalue:
      case pullInfoStrength:
        if (!( AIR_EXISTS(pctx->ispec[ii]->scale)
               && AIR_EXISTS(pctx->ispec[ii]->zero) )) {
          sprintf(err, "%s: %s info needs scale (%g) and zero (%g)", me, 
                  airEnumStr(pullInfo, ii),
                  pctx->ispec[ii]->scale, pctx->ispec[ii]->zero);
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
    if (!( pctx->ispec[pullInfoHeightGradient] )) {
      sprintf(err, "%s: want %s but don't have %s set", me, 
              airEnumStr(pullInfo, pullInfoHeight),
              airEnumStr(pullInfo, pullInfoHeightGradient));
      biffAdd(PULL, err); return 1;
    }
    if (pctx->ispec[pullInfoHeight]->constraint
        && !pctx->ispec[pullInfoHeightHessian]) {
      sprintf(err, "%s: want constrained %s but don't have %s set", me,
              airEnumStr(pullInfo, pullInfoHeight),
              airEnumStr(pullInfo, pullInfoHeightHessian));
      biffAdd(PULL, err); return 1;
    }
  }
  if (pctx->ispec[pullInfoHeightLaplacian]) {
    if (!( pctx->ispec[pullInfoHeight] )) {
      sprintf(err, "%s: want %s but don't have %s set", me, 
              airEnumStr(pullInfo, pullInfoHeightLaplacian),
              airEnumStr(pullInfo, pullInfoHeight));
      biffAdd(PULL, err); return 1;
    }
  }
  if (pctx->ispec[pullInfoIsovalue]) {
    if (!( pctx->ispec[pullInfoIsovalueGradient]
           && pctx->ispec[pullInfoIsovalueHessian] )) {
      sprintf(err, "%s: want %s but don't have %s and %s set", me, 
              airEnumStr(pullInfo, pullInfoIsovalue),
              airEnumStr(pullInfo, pullInfoIsovalueGradient),
              airEnumStr(pullInfo, pullInfoIsovalueHessian));
      biffAdd(PULL, err); return 1;
    }
  }
  if (pctx->ispec[pullInfoTangent2]) {
    if (!pctx->ispec[pullInfoTangent1]) {
      sprintf(err, "%s: want %s but don't have %s set", me, 
              airEnumStr(pullInfo, pullInfoTangent2),
              airEnumStr(pullInfo, pullInfoTangent1));
      biffAdd(PULL, err); return 1;
    }
  }
  if (pctx->ispec[pullInfoTangentMode]) {
    if (!( pctx->ispec[pullInfoTangent1]
           && pctx->ispec[pullInfoTangent2] )) {
      sprintf(err, "%s: want %s but don't have %s and %s set", me, 
              airEnumStr(pullInfo, pullInfoTangentMode),
              airEnumStr(pullInfo, pullInfoTangent1),
              airEnumStr(pullInfo, pullInfoTangent2));
      biffAdd(PULL, err); return 1;
    }
  }
  
  if (!( AIR_IN_CL(1, pctx->threadNum, PULL_THREAD_MAXNUM) )) {
    sprintf(err, "%s: pctx->threadNum (%d) outside valid range [1,%d]", me,
            pctx->threadNum, PULL_THREAD_MAXNUM);
    biffAdd(PULL, err); return 1;
  }

#define CHECK(thing, min, max)                                   \
  if (!( AIR_EXISTS(pctx->thing)                                 \
         && min <= pctx->thing && pctx->thing <= max )) {        \
    sprintf(err, "%s: pctx->" #thing " %g not in range [%g,%g]", \
            me, pctx->thing, min, max);                          \
    biffAdd(PULL, err); return 1;                                \
  }
  /* these reality-check bounds are somewhat arbitrary */
  CHECK(radiusScale, 0.000001, 15.0);
  CHECK(radiusSpace, 0.000001, 15.0);
  CHECK(neighborTrueProb, 0.02, 1.0);
  CHECK(probeProb, 0.02, 1.0);
  CHECK(opporStepScale, 1.0, 1.5);
  CHECK(stepScale, 0.01, 0.99);
  CHECK(energyImprovMin, -0.2, 1.0);
  CHECK(constraintStepMin, 0.00000000000000001, 0.1);
  CHECK(wall, 0.0, 100.0);
  CHECK(alpha, 0.0, 1.0);
  CHECK(beta, 0.0, 1.0);
#undef CHECK
  if (!( 1 <= pctx->constraintIterMax
         && pctx->constraintIterMax <= 50 )) {
    sprintf(err, "%s: pctx->constraintIterMax %u not in range [%u,%u]",
            me, pctx->constraintIterMax, 1, 50);
    biffAdd(PULL, err); return 1;
  }
  
  return 0;
}

int
pullOutputGet(Nrrd *nPosOut, Nrrd *nTenOut, Nrrd *nEnrOut,
              Nrrd *nStatOut, Nrrd *nIdOut,
              pullContext *pctx, int typeOut, int pos4,
              double sthresh, double hthresh,
              int scaleSwapDo, unsigned int scaleAxis,
              double scaleScl) {
  char me[]="pullOutputGet", err[BIFF_STRLEN];
  unsigned int binIdx, pointRun, pointNum, pointIdx;
  int E, dosth, dohth;
  float *posOut_f, *tenOut_f, *enrOut_f;
  double *posOut_d, *tenOut_d, *enrOut_d;
  pullBin *bin;
  pullPoint *point;
  double sclmin, sclmax, sclmean;
  unsigned int *statOut, *idOut;

  if (!( nrrdTypeFloat == typeOut || nrrdTypeDouble == typeOut )) {
    sprintf(err, "%s: typeOut (%d,%s) not %s or %s", me, 
            typeOut, airEnumStr(nrrdType, typeOut),
            airEnumStr(nrrdType, nrrdTypeFloat),
            airEnumStr(nrrdType, nrrdTypeDouble));
    biffAdd(PULL, err); return 1;
  }
  if (AIR_EXISTS(sthresh) && sthresh > 0) {
    dosth = AIR_TRUE;
  } else {
    dosth = AIR_FALSE;
  }
  if (AIR_EXISTS(hthresh) && pctx->ispec[pullInfoHeight]) {
    dohth = AIR_TRUE;
  } else {
    dohth = AIR_FALSE;
  }
  if (scaleSwapDo) {
    if (!( scaleAxis <= 2 )) {
      sprintf(err, "%s: scaleAxis %u invalid", me, scaleAxis);
      biffAdd(PULL, err); return 1;
    }
    if (!AIR_EXISTS(scaleScl)) {
      sprintf(err, "%s: scaleScl %g doesn't exist", me, scaleScl);
      biffAdd(PULL, err); return 1;
    }
  }
  pointNum = _pullPointNumber(pctx);
  E = AIR_FALSE;
  if (nPosOut) {
    E |= nrrdMaybeAlloc_va(nPosOut, typeOut, 2,
                           pos4 ? AIR_CAST(size_t, 4) : AIR_CAST(size_t, 3),
                           AIR_CAST(size_t, pointNum));
  }
  if (nTenOut) {
    E |= nrrdMaybeAlloc_va(nTenOut, typeOut, 2, 
                           AIR_CAST(size_t, 7),
                           AIR_CAST(size_t, pointNum));
  }
  if (nEnrOut) {
    E |= nrrdMaybeAlloc_va(nEnrOut, typeOut, 1, 
                           AIR_CAST(size_t, pointNum));
  }
  if (nStatOut) {
    E |= nrrdMaybeAlloc_va(nStatOut, nrrdTypeUInt, 1, 
                           AIR_CAST(size_t, pointNum));
  }
  if (nIdOut) {
    E |= nrrdMaybeAlloc_va(nIdOut, nrrdTypeUInt, 1, 
                           AIR_CAST(size_t, pointNum));
  }
  if (E) {
    sprintf(err, "%s: trouble allocating outputs", me);
    biffMove(PULL, err, NRRD); return 1;
  }
  posOut_f = nPosOut ? (float*)(nPosOut->data) : NULL;
  tenOut_f = nTenOut ? (float*)(nTenOut->data) : NULL;
  enrOut_f = nEnrOut ? (float*)(nEnrOut->data) : NULL;
  posOut_d = nPosOut ? (double*)(nPosOut->data) : NULL;
  tenOut_d = nTenOut ? (double*)(nTenOut->data) : NULL;
  enrOut_d = nEnrOut ? (double*)(nEnrOut->data) : NULL;
  statOut = nStatOut ? (unsigned int*)(nStatOut->data) : NULL;
  idOut = nIdOut ? (unsigned int*)(nIdOut->data) : NULL;

  pointRun = 0;
  sclmean = 0;
  sclmin = sclmax = AIR_NAN;
  for (binIdx=0; binIdx<pctx->binNum; binIdx++) {
    bin = pctx->bin + binIdx;
    for (pointIdx=0; pointIdx<bin->pointNum; pointIdx++) {
      point = bin->point[pointIdx];
      if (dosth) {
        if (_pullPointScalar(pctx, point, pullInfoStrength,
                             NULL, NULL) < sthresh) {
          continue;
        }
      }
      if (dohth) {
        if (_pullPointScalar(pctx, point, pullInfoHeight,
                             NULL, NULL) > hthresh) {
          continue;
        }
      }
      if (nPosOut) {
        double tpos[4];
        ELL_4V_COPY(tpos, point->pos);
        if (scaleSwapDo) {
          tpos[scaleAxis] = tpos[3]*scaleScl;
          tpos[3] = 0;
        }
        if (nrrdTypeFloat == typeOut) {
          ELL_3V_SET(posOut_f + (pos4 ? 4 : 3)*pointRun,
                     tpos[0], tpos[1], tpos[2]);
          if (pos4) {
            (posOut_f + (pos4 ? 4 : 3)*pointRun)[3] = tpos[3];
          }
        } else {
          ELL_3V_SET(posOut_d + (pos4 ? 4 : 3)*pointRun,
                     tpos[0], tpos[1], tpos[2]);
          if (pos4) {
            (posOut_d + (pos4 ? 4 : 3)*pointRun)[3] = tpos[3];
          }
        }
      }
      if (nTenOut) {
        double scl, tout[7];
        scl = 1;
        if (pctx->ispec[pullInfoHeightHessian]) {
          double *hess, eval[3], evec[9], eceil, len;
          hess = point->info + pctx->infoIdx[pullInfoHeightHessian];
          ell_3m_eigensolve_d(eval, evec, hess, 10);
          eval[0] = AIR_ABS(eval[0]);
          eval[1] = AIR_ABS(eval[1]);
          eval[2] = AIR_ABS(eval[2]);
          eceil = 7/ELL_3V_LEN(eval);
          eval[0] = AIR_MIN(eceil, 1.0/eval[0]);
          eval[1] = AIR_MIN(eceil, 1.0/eval[1]);
          eval[2] = AIR_MIN(eceil, 1.0/eval[2]);
          ELL_3V_NORM(eval, eval, len);
          tenMakeSingle_d(tout, 1, eval, evec);
        } else {
          TEN_T_SET(tout, 1, 1, 0, 0, 1, 0, 1);
        }
        if (!scaleSwapDo) {
          TEN_T_SCALE(tout, scl, tout);
        }
        if (nrrdTypeFloat == typeOut) {
          TEN_T_COPY(tenOut_f + 7*pointRun, tout);
        } else {
          TEN_T_COPY(tenOut_d + 7*pointRun, tout);
        }
      }
      if (nEnrOut) {
        if (nrrdTypeFloat == typeOut) {
          enrOut_f[pointRun] = point->energy;
        } else {
          enrOut_d[pointRun] = point->energy;
        }
      }
      if (nStatOut) {
        statOut[pointRun] = point->status;
      }
      if (nIdOut) {
        idOut[pointRun] = point->idtag;
      }
      pointRun++;
    }
  }
  /* if we skipped points because of strength thresh,
     this will set the output sizes correctly */
  if (nPosOut) {
    nPosOut->axis[1].size = AIR_CAST(size_t, pointRun);
  }
  if (nTenOut) {
    nTenOut->axis[1].size = AIR_CAST(size_t, pointRun);
  }
  if (nEnrOut) {
    nEnrOut->axis[0].size = AIR_CAST(size_t, pointRun);
  }
  if (nStatOut) {
    nStatOut->axis[0].size = AIR_CAST(size_t, pointRun);
  }
  if (nIdOut) {
    nIdOut->axis[0].size = AIR_CAST(size_t, pointRun);
  }

  return 0;
}
