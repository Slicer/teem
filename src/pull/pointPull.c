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

/*
** HEY: this has to be threadsafe, at least threadsafe when there
** are no errors, because this can now be called from multiple
** tasks during population control
*/
pullPoint *
pullPointNew(pullContext *pctx) {
  static const char me[]="pullPointNew";
  pullPoint *pnt;
  unsigned int ii;
  size_t pntSize;
  
  if (!pctx) {
    biffAddf(PULL, "%s: got NULL pointer", me);
    return NULL;
  }
  if (!pctx->infoTotalLen) {
    biffAddf(PULL, "%s: can't allocate points w/out infoTotalLen set\n", me);
    return NULL;
  }
  /* Allocate the pullPoint so that it has pctx->infoTotalLen doubles.
     The pullPoint declaration has info[1], hence the "- 1" below */
  pntSize = sizeof(pullPoint) + sizeof(double)*(pctx->infoTotalLen - 1);
  pnt = AIR_CAST(pullPoint *, calloc(1, pntSize));
  if (!pnt) {
    biffAddf(PULL, "%s: couldn't allocate point (info len %u)\n", me, 
             pctx->infoTotalLen - 1);
    return NULL;
  }

  pnt->idtag = pctx->idtagNext++;
  pnt->idCC = 0;
  pnt->neighPoint = NULL;
  pnt->neighPointNum = 0;
  pnt->neighPointArr = airArrayNew(AIR_CAST(void**, &(pnt->neighPoint)),
                                   &(pnt->neighPointNum),
                                   sizeof(pullPoint *),
                                   PULL_POINT_NEIGH_INCR);
  pnt->neighPointArr->noReallocWhenSmaller = AIR_TRUE;
  pnt->neighDistMean = 0;
  pnt->neighMode = AIR_NAN;
  ELL_10V_ZERO_SET(pnt->neighCovar);
  ELL_6V_ZERO_SET(pnt->neighTanCovar);
  pnt->neighInterNum = 0;
  pnt->stuckIterNum = 0;
#if PULL_PHIST
  pnt->phist = NULL;
  pnt->phistNum = 0;
  pnt->phistArr = airArrayNew(AIR_CAST(void**, &(pnt->phist)), 
                              &(pnt->phistNum),
                              5*sizeof(double), 32);
#endif
  pnt->status = 0;
  ELL_4V_SET(pnt->pos, AIR_NAN, AIR_NAN, AIR_NAN, AIR_NAN);
  pnt->energy = AIR_NAN;
  ELL_4V_SET(pnt->force, AIR_NAN, AIR_NAN, AIR_NAN, AIR_NAN);
  pnt->stepEnergy = pctx->sysParm.stepInitial;
  pnt->stepConstr = pctx->sysParm.stepInitial;
  for (ii=0; ii<pctx->infoTotalLen; ii++) {
    pnt->info[ii] = AIR_NAN;
  }
  return pnt;
}

/*
** this is NOT supposed to make a self-contained point- its
** just to make a back-up of the variable values (and whats below
** is certainly overkill) when doing probing for finding discrete
** differences along scale
*/
void
_pullPointCopy(pullPoint *dst, const pullPoint *src, unsigned int ilen) {
  unsigned int ii;

  /* HEY: shouldn't I just do a memcpy? */
  dst->idtag = src->idtag;
  dst->neighPoint = src->neighPoint;
  dst->neighPointNum = src->neighPointNum;
  dst->neighPointArr = src->neighPointArr;
  dst->neighDistMean = src->neighDistMean;
  dst->neighMode = src->neighMode;
  ELL_10V_COPY(dst->neighCovar, src->neighCovar);
  ELL_6V_COPY(dst->neighTanCovar, src->neighTanCovar);
  dst->neighInterNum = src->neighInterNum;
#if PULL_PHIST
  dst->phist = src->phist;
  dst->phistNum = src->phistNum;
  dst->phistArr = src->phistArr;
#endif
  dst->status = src->status;
  ELL_4V_COPY(dst->pos, src->pos);
  dst->energy = src->energy;
  ELL_4V_COPY(dst->force, src->force);
  dst->stepEnergy = src->stepEnergy;
  dst->stepConstr = src->stepConstr;
  for (ii=0; ii<ilen; ii++) {
    dst->info[ii] = src->info[ii];
  }
  return;
}

pullPoint *
pullPointNix(pullPoint *pnt) {

  pnt->neighPointArr = airArrayNuke(pnt->neighPointArr);
#if PULL_PHIST
  pnt->phistArr = airArrayNuke(pnt->phistArr);
#endif
  airFree(pnt);
  return NULL;
}

#if PULL_PHIST
void
_pullPointHistInit(pullPoint *point) {

  airArrayLenSet(point->phistArr, 0);
  return;
}

void
_pullPointHistAdd(pullPoint *point, int cond) {
  unsigned int phistIdx;

  phistIdx = airArrayLenIncr(point->phistArr, 1);
  ELL_4V_COPY(point->phist + 5*phistIdx, point->pos);
  (point->phist + 5*phistIdx)[3] = 1.0;
  (point->phist + 5*phistIdx)[4] = cond;
  return;
}
#endif

/*
** HEY: there should be something like a "map" over all the points,
** which could implement all these redundant functions
*/

unsigned int
pullPointNumber(const pullContext *pctx) {
  unsigned int binIdx, pointNum;
  const pullBin *bin;

  pointNum = 0;
  for (binIdx=0; binIdx<pctx->binNum; binIdx++) {
    bin = pctx->bin + binIdx;
    pointNum += bin->pointNum;
  }
  return pointNum;
}

double
_pullEnergyTotal(const pullContext *pctx) {
  unsigned int binIdx, pointIdx;
  const pullBin *bin;
  const pullPoint *point;
  double sum;
  
  sum = 0;
  for (binIdx=0; binIdx<pctx->binNum; binIdx++) {
    bin = pctx->bin + binIdx;
    for (pointIdx=0; pointIdx<bin->pointNum; pointIdx++) {
      point = bin->point[pointIdx];
      sum += point->energy;
    }
  }
  return sum;
}

void
_pullPointStepEnergyScale(pullContext *pctx, double scale) {
  unsigned int binIdx, pointIdx;
  const pullBin *bin;
  pullPoint *point;

  for (binIdx=0; binIdx<pctx->binNum; binIdx++) {
    bin = pctx->bin + binIdx;
    for (pointIdx=0; pointIdx<bin->pointNum; pointIdx++) {
      point = bin->point[pointIdx];
      point->stepEnergy *= scale;
    }
  }
  return;
}

double
_pullStepInterAverage(const pullContext *pctx) {
  unsigned int binIdx, pointIdx, pointNum;
  const pullBin *bin;
  const pullPoint *point;
  double sum, avg;

  sum = 0;
  pointNum = 0;
  for (binIdx=0; binIdx<pctx->binNum; binIdx++) {
    bin = pctx->bin + binIdx;
    pointNum += bin->pointNum;
    for (pointIdx=0; pointIdx<bin->pointNum; pointIdx++) {
      point = bin->point[pointIdx];
      sum += point->stepEnergy;
    }
  }
  avg = (!pointNum ? AIR_NAN : sum/pointNum);
  return avg;
}
/* ^^^  vvv HEY HEY HEY: COPY + PASTE COPY + PASTE COPY + PASTE */
double
_pullStepConstrAverage(const pullContext *pctx) {
  unsigned int binIdx, pointIdx, pointNum;
  const pullBin *bin;
  const pullPoint *point;
  double sum, avg;

  sum = 0;
  pointNum = 0;
  for (binIdx=0; binIdx<pctx->binNum; binIdx++) {
    bin = pctx->bin + binIdx;
    pointNum += bin->pointNum;
    for (pointIdx=0; pointIdx<bin->pointNum; pointIdx++) {
      point = bin->point[pointIdx];
      sum += point->stepConstr;
    }
  }
  avg = (!pointNum ? AIR_NAN : sum/pointNum);
  return avg;
}

/*
** convenience function for learning a scalar AND its gradient or hessian 
*/
double
_pullPointScalar(const pullContext *pctx, const pullPoint *point, int sclInfo,
                 /* output */
                 double grad[3], double hess[9]) {
  double scl;
  const pullInfoSpec *ispec;
  int gradInfo[1+PULL_INFO_MAX] = {
    0,                        /* pullInfoUnknown */
    0,                        /* pullInfoTensor */
    0,                        /* pullInfoTensorInverse */
    0,                        /* pullInfoHessian */
    pullInfoInsideGradient,   /* pullInfoInside */
    0,                        /* pullInfoInsideGradient */
    pullInfoHeightGradient,   /* pullInfoHeight */
    0,                        /* pullInfoHeightGradient */
    0,                        /* pullInfoHeightHessian */
    0,                        /* pullInfoHeightLaplacian */
    0,                        /* pullInfoSeedPreThresh */
    0,                        /* pullInfoSeedThresh */
    0,                        /* pullInfoLiveThresh */
    0,                        /* pullInfoLiveThresh2 */
    0,                        /* pullInfoLiveThresh3 */
    0,                        /* pullInfoTangent1 */
    0,                        /* pullInfoTangent2 */
    0,                        /* pullInfoTangentMode */
    pullInfoIsovalueGradient, /* pullInfoIsovalue */
    0,                        /* pullInfoIsovalueGradient */
    0,                        /* pullInfoIsovalueHessian */
    0,                        /* pullInfoStrength */
  };
  int hessInfo[1+PULL_INFO_MAX] = {
    0,                        /* pullInfoUnknown */
    0,                        /* pullInfoTensor */
    0,                        /* pullInfoTensorInverse */
    0,                        /* pullInfoHessian */
    0,                        /* pullInfoInside */
    0,                        /* pullInfoInsideGradient */
    pullInfoHeightHessian,    /* pullInfoHeight */
    0,                        /* pullInfoHeightGradient */
    0,                        /* pullInfoHeightHessian */
    0,                        /* pullInfoHeightLaplacian */
    0,                        /* pullInfoSeedPreThresh */
    0,                        /* pullInfoSeedThresh */
    0,                        /* pullInfoLiveThresh */
    0,                        /* pullInfoLiveThresh2 */
    0,                        /* pullInfoLiveThresh3 */
    0,                        /* pullInfoTangent1 */
    0,                        /* pullInfoTangent2 */
    0,                        /* pullInfoTangentMode */
    pullInfoIsovalueHessian,  /* pullInfoIsovalue */
    0,                        /* pullInfoIsovalueGradient */
    0,                        /* pullInfoIsovalueHessian */
    0,                        /* pullInfoStrength */
  };
  const unsigned int *infoIdx;

  infoIdx = pctx->infoIdx;
  ispec = pctx->ispec[sclInfo];
  scl = point->info[infoIdx[sclInfo]];
  scl = (scl - ispec->zero)*ispec->scale;
  /*
    learned: this wasn't thought through: the idea was that the height
    *laplacian* answer should be transformed by the *height* zero and
    scale.  scale might make sense, but not zero.  This cost a few
    hours of tracking down the fact that the first zero-crossing
    detection phase of the lapl constraint was failing because the
    laplacian was vacillating around hspec->zero, not 0.0 ...
  if (pullInfoHeightLaplacian == sclInfo) {
    const pullInfoSpec *hspec;
    hspec = pctx->ispec[pullInfoHeight];
    scl = (scl - hspec->zero)*hspec->scale;
  } else {
    scl = (scl - ispec->zero)*ispec->scale;
  }
  */
  /*
  printf("%s = (%g - %g)*%g = %g*%g = %g = %g\n",
         airEnumStr(pullInfo, sclInfo),
         point->info[infoIdx[sclInfo]], 
         ispec->zero, ispec->scale,
         point->info[infoIdx[sclInfo]] - ispec->zero, ispec->scale,
         (point->info[infoIdx[sclInfo]] - ispec->zero)*ispec->scale,
         scl);
  */
  if (grad && gradInfo[sclInfo]) {
    const double *ptr = point->info + infoIdx[gradInfo[sclInfo]];
    ELL_3V_SCALE(grad, ispec->scale, ptr);
  }
  if (hess && hessInfo[sclInfo]) {
    const double *ptr = point->info + infoIdx[hessInfo[sclInfo]];
    ELL_3M_SCALE(hess, ispec->scale, ptr);
  }
  return scl;
}

void
_pullValProbe(pullTask *task, pullVolume *vol,
              double xx, double yy, double zz, double ss) {
  double pos[4];
  
  ELL_4V_SET(pos, xx, yy, zz, ss);
  if (GAGE_QUERY_ITEM_TEST(vol->pullValQuery, pullValSlice)) {
    task->pullValDirectAnswer[pullValSlice][0] =
      ELL_3V_DOT(pos, task->pctx->_sliceNormal);
  }
  if (GAGE_QUERY_ITEM_TEST(vol->pullValQuery, pullValSliceGradVec)) {
    ELL_3V_COPY(task->pullValDirectAnswer[pullValSliceGradVec],
                task->pctx->_sliceNormal);
  }
  return;
}

int
_pullProbe(pullTask *task, pullPoint *point) {
  static const char me[]="_pullProbe";
  unsigned int ii, gret=0;
  int edge;

#if 0
  static int logIdx=0, logDone=AIR_FALSE, logStarted=AIR_FALSE;
  static Nrrd *nlog;
  static double *log=NULL;
  if (!logStarted) {
    if (81 == point->idtag) {
      printf("\n\n%s: ###### HELLO begin logging ....\n\n\n", me);
      /* knowing the logIdx at the end of logging ... */
      nlog = nrrdNew();
      nrrdMaybeAlloc_va(nlog, nrrdTypeDouble, 2,
                        AIR_CAST(size_t, 25),
                        AIR_CAST(size_t, 2754));
      log = AIR_CAST(double*, nlog->data);
      logStarted = AIR_TRUE;
    }
  }
#endif

  if (!ELL_4V_EXISTS(point->pos)) {
    fprintf(stderr, "%s: got non-exist pos (%g,%g,%g,%g)\n\n!!!\n\n\n", me, 
            point->pos[0], point->pos[1], point->pos[2], point->pos[3]);
    /*  HEY: NEED TO track down how this can happen!
      biffAddfPULL, err); return 1;
    */
    /* can't probe, but make it go away as quickly as possible */
    ELL_4V_SET(point->pos, 0, 0, 0, 0);
    point->status |= PULL_STATUS_NIXME_BIT;
    return 0;
  }
  if (task->pctx->verbose > 3) {
    printf("%s: hello; probing %u volumes\n", me, task->pctx->volNum);
  }
  edge = AIR_FALSE;
  for (ii=0; ii<task->pctx->volNum; ii++) {
    pullVolume *vol;
    vol = task->vol[ii];
    if (task->pctx->iter && vol->seedOnly) {
      /* its after the 1st iteration (#0), and this vol is only for seeding */
      continue;
    }
    if (pullValGageKind == vol->kind) {
      /* we don't use gage for this, its a pull "kind" */
      _pullValProbe(task, task->vol[ii],
                    point->pos[0], point->pos[1], point->pos[2],
                    task->vol[ii]->scaleNum ? point->pos[3] : 0.0);
      continue;
    } else { /* pullValGageKind != vol->kind */
      /* HEY should task->vol[ii]->scaleNum be the using-scale-space test? */
      if (!task->vol[ii]->ninScale) {
        /*
          if (81 == point->idtag) {
          printf("%s: probing vol[%u] @ %g %g %g\n", me, ii,
          point->pos[0], point->pos[1], point->pos[2]);
          }
        */
        gret = gageProbeSpace(task->vol[ii]->gctx,
                              point->pos[0], point->pos[1], point->pos[2],
                              AIR_FALSE /* index-space */,
                              AIR_TRUE /* clamp */);
      } else {
        if (task->pctx->verbose > 3) {
          printf("%s: vol[%u] has scale (%u)-> "
                 "gageStackProbeSpace(%p) (v %d)\n",
                 me, ii, task->vol[ii]->scaleNum,
                 task->vol[ii]->gctx, task->vol[ii]->gctx->verbose);
        }
        /*
        if (81 == point->idtag) {
          printf("%s: probing vol[%u] @ %g %g %g %g\n", me, ii,
                 point->pos[0], point->pos[1], point->pos[2], point->pos[3]);
        }
        */
        gret = gageStackProbeSpace(task->vol[ii]->gctx,
                                   point->pos[0], point->pos[1],
                                   point->pos[2], point->pos[3],
                                   AIR_FALSE /* index-space */,
                                   AIR_TRUE /* clamp */);
      }
      if (gret) {
        biffAddf(PULL, "%s: probe failed on vol %u/%u: (%d) %s", me,
                 ii, task->pctx->volNum,
                 task->vol[ii]->gctx->errNum, task->vol[ii]->gctx->errStr);
        return 1;
      }
      edge |= !!task->vol[ii]->gctx->edgeFrac;
    } /* else pullValGageKind != vol->kind */
  }
  if (edge) {
    point->status |= PULL_STATUS_EDGE_BIT;
  } else {
    point->status &= ~PULL_STATUS_EDGE_BIT;
  }

  /* maybe is a little stupid to have the infos indexed this way, 
     since it means that we always have to loop through all indices,
     but at least the compiler can unroll it... */
  for (ii=0; ii<=PULL_INFO_MAX; ii++) {
    unsigned int alen, aidx;
    if (task->ans[ii]) {
      alen = _pullInfoAnswerLen[ii];
      aidx = task->pctx->infoIdx[ii];
      _pullInfoAnswerCopy[alen](point->info + aidx, task->ans[ii]);
      /*
      if (81 == point->idtag) {
        pullVolume *vol;
        pullInfoSpec *isp;
        isp = task->pctx->ispec[ii];
        vol = task->pctx->vol[isp->volIdx];
        if (1 == alen) {
          printf("!%s: info[%u] %s: %s(\"%s\") = %g\n", me,
                 ii, airEnumStr(pullInfo, ii),
                 airEnumStr(vol->kind->enm, isp->item),
                 vol->name, task->ans[ii][0]);
        } else {
          unsigned int vali;
          printf("!%s: info[%u] %s: %s(\"%s\") =\n", me,
                 ii, airEnumStr(pullInfo, ii),
                 airEnumStr(vol->kind->enm, isp->item), vol->name);
          for (vali=0; vali<alen; vali++) {
            printf("!%s:    [%u]  %g\n", me, vali, 
                   task->ans[ii][vali]);
          }
        }
      }
      */
    }
  }

#if 0
  if (logStarted && !logDone) {
    unsigned int ai;
    /* the actual logging */
    log[0] = point->idtag;
    ELL_4V_COPY(log + 1, point->pos);
    for (ai=0; ai<20; ai++) {
      log[5 + ai] = point->info[ai];
    }
    log += nlog->axis[0].size;
    logIdx++;
    if (1 == task->pctx->iter && 81 == point->idtag) {
      printf("\n\n%s: ###### OKAY done logging (%u)....\n\n\n", me, logIdx);
      nrrdSave("probelog.nrrd", nlog, NULL);
      nlog = nrrdNuke(nlog);
      logDone = AIR_TRUE;
    }
  }
#endif
  
  return 0;
}

int
_pullPointInitializePerVoxel(const pullContext *pctx,
                             const unsigned int pointIdx,
                             pullPoint *point, pullVolume *scaleVol,
                             int taskOrder[3],
                             /* output */
                             int *createFailP) {
  static const char me[]="_pullPointInitializePerVoxel";
  unsigned int vidx[3], pix;
  double iPos[3];
  airRandMTState *rng;
  pullVolume *seedVol;
  gageShape *seedShape;
  int reject, constrFail;
  unsigned int k, task;

  seedVol = pctx->vol[pctx->ispec[pullInfoSeedThresh]->volIdx]; 
  seedShape = seedVol->gctx->shape; 
  rng = pctx->task[0]->rng;

  /* Obtain voxel and indices from pointIdx */
  /* axis ordering for this is x, y, z, scale */
  pix = pointIdx;
  if (pctx->initParm.pointPerVoxel > 0) {
    pix /= pctx->initParm.pointPerVoxel;
  } else {
    pix *= -pctx->initParm.pointPerVoxel;
  }
  vidx[0] = pix % seedShape->size[0];
  pix = (pix - vidx[0])/seedShape->size[0];
  vidx[1] = pix % seedShape->size[1];
  pix = (pix - vidx[1])/seedShape->size[1];
  if (pctx->initParm.ppvZRange[0] <= pctx->initParm.ppvZRange[1]) {
    unsigned int zrn;
    zrn = pctx->initParm.ppvZRange[1] - pctx->initParm.ppvZRange[0] + 1;
    vidx[2] = (pix % zrn) + pctx->initParm.ppvZRange[0];
    pix = (pix - (pix % zrn))/zrn;
  } else {
    vidx[2] = pix % seedShape->size[2];
    pix = (pix - vidx[2])/seedShape->size[2];
  }
  for (k=0; k<=2; k++) {
    iPos[k] = vidx[k] + pctx->initParm.jitter*(airDrandMT_r(rng)-0.5);
  }
  gageShapeItoW(seedShape, point->pos, iPos);
  /*
  printf("!%s: pointIdx %u -> vidx %u %u %u (%u)\n"
         "       -> iPos %g %g %g -> wPos %g %g %g\n",
         me, pointIdx, vidx[0], vidx[1], vidx[2], pix,
         iPos[0], iPos[1], iPos[2], 
         point->pos[0], point->pos[1], point->pos[2]);
  */

  /* Compute sigma coordinate from pix */
  if (pctx->haveScale) {
    int outside;
    double aidx, bidx;
    /* pix should already be integer in [0, pctx->samplesAlongScaleNum-1)]. */
    aidx = pix + pctx->initParm.jitter*(airDrandMT_r(rng)-0.5);
    bidx = AIR_AFFINE(-0.5, aidx, pctx->initParm.samplesAlongScaleNum-0.5, 
                      0.0, scaleVol->scaleNum-1);
    point->pos[3] = gageStackItoW(scaleVol->gctx, bidx, &outside);
    /*
    printf("!%s: pix %u -> a %g b %g -> wpos %g\n", me, 
           pix, aidx, bidx, point->pos[3]);
    */
  } else {
    point->pos[3] = 0;
  }

  /* Do a tentative probe */
  if (_pullProbe(pctx->task[0], point)) {
   biffAddf(PULL, "%s: probing pointIdx %u of world", me, pointIdx);
   return 1;
  }

  constrFail = AIR_FALSE;
  reject = AIR_FALSE;
  for (task=0; task<3; task++) {
    switch (taskOrder[task]) {
    case 0:
      /* Check we pass pre-threshold */
      if (!reject && pctx->ispec[pullInfoSeedPreThresh]) {
        double seedv;
        seedv = _pullPointScalar(pctx, point, pullInfoSeedPreThresh, NULL, NULL);
        reject |= (seedv < 0);
      }
      break;
    case 1:
      /* we should be guaranteed to have a seed thresh info */
      if (!reject && pctx->ispec[pullInfoSeedThresh]) {
        double seedv;
        seedv = _pullPointScalar(pctx, point, pullInfoSeedThresh, NULL, NULL);
        reject |= (seedv < 0);
      }
      if (pctx->initParm.liveThreshUse) {
        if (!reject && pctx->ispec[pullInfoLiveThresh]) {
          double seedv;
          seedv = _pullPointScalar(pctx, point, pullInfoLiveThresh,
                                   NULL, NULL);
          reject |= (seedv < 0);
        }
        /* HEY copy & paste */
        if (!reject && pctx->ispec[pullInfoLiveThresh2]) { 
          double seedv;
          seedv = _pullPointScalar(pctx, point, pullInfoLiveThresh2,
                                   NULL, NULL);
          reject |= (seedv < 0);
        }
        /* HEY copy & paste */
        if (!reject && pctx->ispec[pullInfoLiveThresh3]) { 
          double seedv;
          seedv = _pullPointScalar(pctx, point, pullInfoLiveThresh3,
                                   NULL, NULL);
          reject |= (seedv < 0);
        }
      }
      break;
    case 2:
      if (!reject && pctx->constraint) {
        if (_pullConstraintSatisfy(pctx->task[0], point, &constrFail)) {
          biffAddf(PULL, "%s: on pnt %u",
                   me, pointIdx);
          return 1;
        }
      } else {
        constrFail = AIR_FALSE;
      }
      reject |= constrFail;
      break;
    }
  }
  /* Gather consensus */
  if (reject) {
    *createFailP = AIR_TRUE;
  } else {
    *createFailP = AIR_FALSE;
  }

  return 0;
}

int
_pullPointInitializeRandom(pullContext *pctx,
                           const unsigned int pointIdx,
                           pullPoint *point, pullVolume *scaleVol,
                           int taskOrder[3],
                           /* output */
                           int *createFailP) {
  static const char me[]="_pullPointInitializeRandom";
  int reject, threshFail, constrFail, verbo;
  airRandMTState *rng;
  unsigned int threshFailCount = 0, constrFailCount = 0;
  rng = pctx->task[0]->rng;

  /* Check that point is fit enough to be included */
  do {
    unsigned int task;
    reject = AIR_FALSE;
    _pullPointHistInit(point);
    /* Populate tentative random point */
    point->pos[0] = AIR_AFFINE(0.0, airDrandMT_r(rng), 1.0,
                               pctx->bboxMin[0], pctx->bboxMax[0]);
    point->pos[1] = AIR_AFFINE(0.0, airDrandMT_r(rng), 1.0,
                               pctx->bboxMin[1], pctx->bboxMax[1]);
    point->pos[2] = AIR_AFFINE(0.0, airDrandMT_r(rng), 1.0,
                               pctx->bboxMin[2], pctx->bboxMax[2]);
    if (pctx->haveScale) {
      double sridx, rnd;
      int outside;
      rnd = airDrandMT_r(rng);
      sridx = AIR_AFFINE(0.0, rnd, 1.0, 0, scaleVol->scaleNum-1);
      point->pos[3] = gageStackItoW(scaleVol->gctx, sridx, &outside);
    } else {
      point->pos[3] = 0.0;
    }
    /*
    verbo = (AIR_ABS(-0.246015 - point->pos[0]) < 0.1 &&
             AIR_ABS(-144.78 - point->pos[0]) < 0.1 &&
             AIR_ABS(-85.3813 - point->pos[0]) < 0.1);
    */
    verbo = AIR_FALSE;
    if (verbo) {
      fprintf(stderr, "%s: verbo on for point %u at %g %g %g %g\n", me,
              point->idtag, point->pos[0], point->pos[1],
              point->pos[2], point->pos[3]);
    }
    _pullPointHistAdd(point, pullCondOld);
    /* Do a tentative probe */
    if (_pullProbe(pctx->task[0], point)) {
      biffAddf(PULL, "%s: probing pointIdx %u of world", me, pointIdx);
      return 1;
    }
    /* Enforce constraints and thresholds */
    threshFail = AIR_FALSE;
    for (task=0; task<3; task++) {
      double seedv;
      switch (taskOrder[task]) {
      case 0:
        /* Check we pass pre-threshold */
        if (!reject && pctx->ispec[pullInfoSeedPreThresh]) {
          seedv = _pullPointScalar(pctx, point, pullInfoSeedPreThresh,
                                   NULL, NULL);
          reject |= (seedv < 0);
        }
        break;
      case 1:
        if (!reject && pctx->ispec[pullInfoSeedThresh]) {
          seedv = _pullPointScalar(pctx, point, pullInfoSeedThresh,
                                   NULL, NULL);
          threshFailCount += (threshFail = (seedv < 0));
        } else {
          threshFail = AIR_FALSE;
        }
        reject |= threshFail;
        if (pctx->initParm.liveThreshUse) {
          if (!reject && pctx->ispec[pullInfoLiveThresh]) {
            seedv = _pullPointScalar(pctx, point, pullInfoLiveThresh,
                                     NULL, NULL);
            threshFailCount += (threshFail = (seedv < 0));
          } else {
            threshFail = AIR_FALSE;
          }
          /* HEY copy & paste */
          if (!reject && pctx->ispec[pullInfoLiveThresh2]) {
            seedv = _pullPointScalar(pctx, point, pullInfoLiveThresh2,
                                     NULL, NULL);
            threshFailCount += (threshFail = (seedv < 0));
          } else {
            threshFail = AIR_FALSE;
          }
          /* HEY copy & paste */
          if (!reject && pctx->ispec[pullInfoLiveThresh3]) {
            seedv = _pullPointScalar(pctx, point, pullInfoLiveThresh3,
                                     NULL, NULL);
            threshFailCount += (threshFail = (seedv < 0));
          } else {
            threshFail = AIR_FALSE;
          }
        }
        reject |= threshFail;
        break;
      case 2:
        /* Avoid doing constraint if the threshold has already failed */
        if (!reject && pctx->constraint) {
          if (_pullConstraintSatisfy(pctx->task[0], point, &constrFail)) {
            biffAddf(PULL, "%s: trying constraint on point %u", me, pointIdx);
            return 1;
          }
          constrFailCount += constrFail;
        } else {
          constrFail = AIR_FALSE;
        }
        reject |= constrFail;
        break;
      }
    }
    /* Gather consensus from tasks */
    if (reject) {
      if (threshFailCount + constrFailCount > _PULL_RANDOM_SEED_TRY_MAX) {
        /* Very bad luck; we've too many times */
        biffAddf(PULL, "%s: failed too often (%u times) placing %u "
                 "(failed on thresh %s/%u, constr %s/%u)",
                 me, _PULL_RANDOM_SEED_TRY_MAX, pointIdx,
                 threshFail ? "true" : "false", threshFailCount,
                 constrFail ? "true" : "false", constrFailCount);
        return 1;
      }
    }
  } while (reject);

  /* by design, either we succeed on placing a point, or we have a 
     biff error */
  *createFailP = AIR_FALSE;

  return 0;
}

int
_pullPointInitializeGivenPos(pullContext *pctx,
                             const double *posData,
                             const unsigned int pointIdx,
                             pullPoint *point,
                             /* output */
                             int *createFailP) {
  static const char me[]="_pullPointInitializeGivenPos";
  double seedv;
  int reject;

  /* Copy nrrd point into pullPoint */
  ELL_4V_COPY(point->pos, posData + 4*pointIdx);
  /* we're dictating positions, but still have to do initial probe,
     and possibly liveThresholding */
  if (_pullProbe(pctx->task[0], point)) {
    biffAddf(PULL, "%s: probing pointIdx %u of npos", me, pointIdx);
    return 1;
  }
  reject = AIR_FALSE;
  if (pctx->flag.nixAtVolumeEdgeSpace
      && (point->status & PULL_STATUS_EDGE_BIT)) {
    reject = AIR_TRUE;
  }
  if (!reject && pctx->initParm.liveThreshUse) {
    if (!reject && pctx->ispec[pullInfoLiveThresh]) {
      seedv = _pullPointScalar(pctx, point, pullInfoLiveThresh,
                               NULL, NULL);
      reject |= (seedv < 0);
    }
    /* HEY copy & paste */
    if (!reject && pctx->ispec[pullInfoLiveThresh2]) {
      seedv = _pullPointScalar(pctx, point, pullInfoLiveThresh2,
                               NULL, NULL);
      reject |= (seedv < 0);
    }
    /* HEY copy & paste */
    if (!reject && pctx->ispec[pullInfoLiveThresh3]) {
      seedv = _pullPointScalar(pctx, point, pullInfoLiveThresh3,
                               NULL, NULL);
      reject |= (seedv < 0);
    }
  }
  if (reject) {
    *createFailP = AIR_TRUE;
  } else {
    *createFailP = AIR_FALSE;
  }

  return 0;
}

/*
** _pullPointSetup sets:
**
** This is only called by the master thread
** 
** this should set stuff to be like after an update stage and
** just before the rebinning
*/
int
_pullPointSetup(pullContext *pctx) {
  static const char me[]="_pullPointSetup";
  char doneStr[AIR_STRLEN_SMALL];
  unsigned int pointIdx, binIdx, tick, pn;
  pullPoint *point;
  pullBin *bin;
  int createFail,added, taskOrder[3];
  airArray *mop;
  Nrrd *npos;
  pullVolume *seedVol, *scaleVol;
  gageShape *seedShape;
  double factor, *posData;
  unsigned int totalNumPoints, voxNum, ii;

  /* on using pullBinsPointMaybeAdd: This is used only in the context
     of constraints; it makes sense to be a little more selective
     about adding points, so that they aren't completely piled on top
     of each other. This relies on _PULL_BINNING_MAYBE_ADD_THRESH: its
     tempting to set this value high, to more aggressively limit the
     number of points added, but that's really the job of population
     control, and we can't always guarantee that constraint manifolds
     will be well-sampled (with respect to pctx->radiusSpace) to start
     with */
  mop = airMopNew();
  switch (pctx->initParm.method) {
  case pullInitMethodGivenPos:
    npos = nrrdNew();
    airMopAdd(mop, npos, (airMopper)nrrdNuke, airMopAlways);
    /* even if npos came in as double, we have to copy it */
    if (nrrdConvert(npos, pctx->initParm.npos, nrrdTypeDouble)) {
      biffMovef(PULL, NRRD, "%s: trouble converting npos", me);
      airMopError(mop); return 1;
    }
    posData = AIR_CAST(double *, npos->data);
    if (pctx->initParm.numInitial || pctx->initParm.pointPerVoxel) {
      printf("%s: with npos, overriding both numInitial (%u) "
             "and pointPerVoxel (%d)\n", me, pctx->initParm.numInitial,
             pctx->initParm.pointPerVoxel);
    }
    totalNumPoints = AIR_CAST(unsigned int, npos->axis[1].size);
    break;
  case pullInitMethodPointPerVoxel:
    npos = NULL;
    posData = NULL;
    if (pctx->initParm.numInitial) {
      printf("%s: pointPerVoxel %d overrides numInitial (%u)\n", me,
             pctx->initParm.pointPerVoxel, pctx->initParm.numInitial);
    }
    /* Obtain number of voxels */
    seedVol = pctx->vol[pctx->ispec[pullInfoSeedThresh]->volIdx];
    seedShape = seedVol->gctx->shape;
    if (pctx->initParm.ppvZRange[0] <= pctx->initParm.ppvZRange[1]) {
      unsigned int zrn;
      if (!( pctx->initParm.ppvZRange[0] < seedShape->size[2]
             && pctx->initParm.ppvZRange[1] < seedShape->size[2] )) {
        biffAddf(PULL, "%s: ppvZRange[%u,%u] outside volume [0,%u]", me,
                 pctx->initParm.ppvZRange[0], pctx->initParm.ppvZRange[1],
                 seedShape->size[2]-1);
        airMopError(mop); return 1;
      }
      zrn = pctx->initParm.ppvZRange[1] - pctx->initParm.ppvZRange[0] + 1;
      voxNum = seedShape->size[0]*seedShape->size[1]*zrn;
      printf("%s: vol size %u %u [%u,%u] -> voxNum %u\n", me, 
             seedShape->size[0], seedShape->size[1],
             pctx->initParm.ppvZRange[0], pctx->initParm.ppvZRange[1],
             voxNum);
    } else {
      voxNum = seedShape->size[0]*seedShape->size[1]*seedShape->size[2];
      printf("%s: vol size %u %u %u -> voxNum %u\n", me, 
             seedShape->size[0], seedShape->size[1], seedShape->size[2],
             voxNum);
    }

    /* Compute total number of points */
    if (pctx->initParm.pointPerVoxel > 0) {
      factor = pctx->initParm.pointPerVoxel;
    } else {
      factor = -1.0/pctx->initParm.pointPerVoxel;
    }
    if (pctx->haveScale) {
      unsigned int sasn;
      sasn = pctx->initParm.samplesAlongScaleNum;
      totalNumPoints = AIR_CAST(unsigned int, voxNum * factor * sasn);
    } else {
      totalNumPoints = AIR_CAST(unsigned int, voxNum * factor);
    }
    break;
  case pullInitMethodRandom:
    npos = NULL;
    posData = NULL;
    totalNumPoints = pctx->initParm.numInitial;
    break;
  default:
    biffAddf(PULL, "%s: pullInitMethod %d not handled!", me,
             pctx->initParm.method);
    airMopError(mop); return 1;
    break;
  }
  if (pctx->verbose) {
    printf("!%s: initializing/seeding ...       ", me);
    fflush(stdout);
  }

  /* find first scale volume, if there is one; this is used by some
     seeders to determine placement along the scale axis */
  scaleVol = NULL;
  for (ii=0; ii<pctx->volNum; ii++) {
    if (pctx->vol[ii]->ninScale) {
      scaleVol = pctx->vol[ii];
      break;
    }
  }
  /* Task = 0 -> PreThreshold;
     Task = 1 -> SeedThreshold;
     Task = 2 -> Constraint; */
  if (pctx->flag.constraintBeforeSeedThresh) {
    ELL_3V_SET(taskOrder, 0, 2, 1);
  } else {
    ELL_3V_SET(taskOrder, 0, 1, 2);
  }
  
  /* Start adding points */
  tick = totalNumPoints/1000;
  point = NULL;
  for (pointIdx = 0; pointIdx < totalNumPoints; pointIdx++) {
    int E;
    if (pctx->verbose) {
      if (tick < 100 || 0 == pointIdx % tick) {
        printf("%s", airDoneStr(0, pointIdx, totalNumPoints, doneStr));
        fflush(stdout);
      }
    }
    if (pctx->verbose > 5) {
      printf("\n%s: setting up point = %u/%u\n", me,
             pointIdx, totalNumPoints);
    }
    /* Create point */
    if (!point) {
      point = pullPointNew(pctx);
    }
    /* Filling array according to initialization method */
    E = 0;
    switch(pctx->initParm.method) {
    case pullInitMethodRandom:
      E = _pullPointInitializeRandom(pctx, pointIdx, point, scaleVol,
                                     taskOrder, &createFail);
      break;
    case pullInitMethodPointPerVoxel:
      E = _pullPointInitializePerVoxel(pctx, pointIdx, point, scaleVol,
                                       taskOrder, &createFail);
      break;
    case pullInitMethodGivenPos:
      E = _pullPointInitializeGivenPos(pctx, posData, pointIdx, point,
                                       &createFail);
      break;
    }
    if (E) {
      biffAddf(PULL, "%s: trouble trying point %u (id %u)", me,
               pointIdx, point->idtag);
      airMopError(mop); return 1;
    }
    
    if (createFail) {
      /* We were not successful in creating a point */
      continue;
    }
    
    /* else, the point is ready for binning */
    if (pctx->constraint) {
      if (pullBinsPointMaybeAdd(pctx, point, NULL, &added)) {
        biffAddf(PULL, "%s: trouble binning point %u", me, point->idtag);
        airMopError(mop); return 1;
      }
      if (added) {
        point = NULL;
      }
    } else {
      if (pullBinsPointAdd(pctx, point, NULL)) {
          biffAddf(PULL, "%s: trouble binning point %u", me, point->idtag);
          airMopError(mop); return 1;
      }
      point = NULL;
    }
  } /* Done looping through total number of points */
  if (pctx->verbose) {
    printf("%s\n", airDoneStr(0, pointIdx, totalNumPoints,
                              doneStr));
  }
  if (point) {
    /* we created a new test point, but it was never placed in the volume */
    /* so, HACK: undo pullPointNew ... */
    point = pullPointNix(point);
    pctx->idtagNext -= 1;
  }
  
  /* Final check: do we have any points? */
  pn = pullPointNumber(pctx);
  if (!pn) {
    if (pctx->ispec[pullInfoSeedThresh]) {
      biffAddf(PULL, "%s: zero points: seeding failed (bad seedthresh? %g)",
               me, pctx->ispec[pullInfoSeedThresh]->zero);
    } else {
      biffAddf(PULL, "%s: zero points: seeding failed", me);
    }
    airMopError(mop); return 1;
  }
  if (pctx->verbose) {
    fprintf(stderr, "%s: initialized to %u points\n", me, pn);
  }
  pctx->tmpPointPtr = AIR_CAST(pullPoint **,
                               calloc(pn, sizeof(pullPoint*)));
  pctx->tmpPointPerm = AIR_CAST(unsigned int *,
                                calloc(pn, sizeof(unsigned int)));
  if (!( pctx->tmpPointPtr && pctx->tmpPointPerm )) {
    biffAddf(PULL, "%s: couldn't allocate tmp buffers %p %p", me, 
             pctx->tmpPointPtr, pctx->tmpPointPerm);
    airMopError(mop); return 1;
  }
  pctx->tmpPointNum = pn;
  
  /* now that all points have been added, set their energy to help
     inspect initial state */
  for (binIdx=0; binIdx<pctx->binNum; binIdx++) {
    bin = pctx->bin + binIdx;
    for (pointIdx=0; pointIdx<bin->pointNum; pointIdx++) {
      point = bin->point[pointIdx];
      point->energy = _pullPointEnergyTotal(pctx->task[0], bin, point,
                                            AIR_FALSE, /* ignoreImage */
                                            point->force);
    }
  }

  airMopOkay(mop);
  return 0;
}

void
_pullPointFinish(pullContext *pctx) {
  
  airFree(pctx->tmpPointPtr);
  airFree(pctx->tmpPointPerm);
  return ;
}
