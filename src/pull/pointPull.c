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
  char me[]="pullPointNew", err[BIFF_STRLEN];
  pullPoint *pnt;
  unsigned int ii;
  size_t pntSize;
  
  if (!pctx) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(PULL, err); return NULL;
  }
  if (!pctx->infoTotalLen) {
    sprintf(err, "%s: can't allocate points w/out infoTotalLen set\n", me);
    biffAdd(PULL, err); return NULL;
  }
  /* Allocate the pullPoint so that it has pctx->infoTotalLen doubles.
     The pullPoint declaration has info[1], hence the "- 1" below */
  pntSize = sizeof(pullPoint) + sizeof(double)*(pctx->infoTotalLen - 1);
  pnt = AIR_CAST(pullPoint *, calloc(1, pntSize));
  if (!pnt) {
    sprintf(err, "%s: couldn't allocate point (info len %u)\n", me, 
            pctx->infoTotalLen - 1);
    biffAdd(PULL, err); return NULL;
  }

  pnt->idtag = pctx->idtagNext++;
  pnt->neighPoint = NULL;
  pnt->neighPointNum = 0;
  pnt->neighPointArr = airArrayNew(AIR_CAST(void**, &(pnt->neighPoint)),
                                   &(pnt->neighPointNum),
                                   sizeof(pullPoint *),
                                   PULL_POINT_NEIGH_INCR);
  pnt->neighPointArr->noReallocWhenSmaller = AIR_TRUE;
  pnt->neighDistMean = 0;
  pnt->neighMode = AIR_NAN;
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
  pnt->stepEnergy = pctx->stepInitial;
  pnt->stepConstr = pctx->stepInitial;
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

void
_pullPointHistInit(pullPoint *point) {

#if PULL_PHIST
  airArrayLenSet(point->phistArr, 0);
#else
  AIR_UNUSED(point);
#endif
  return;
}

void
_pullPointHistAdd(pullPoint *point, int cond) {
#if PULL_PHIST
  unsigned int phistIdx;

  phistIdx = airArrayLenIncr(point->phistArr, 1);
  ELL_4V_COPY(point->phist + 5*phistIdx, point->pos);
  (point->phist + 5*phistIdx)[3] = 1.0;
  (point->phist + 5*phistIdx)[4] = cond;
#else
  AIR_UNUSED(point);
  AIR_UNUSED(cond);
#endif
  return;
}

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

int
_pullProbe(pullTask *task, pullPoint *point) {
  char me[]="_pullProbe", err[BIFF_STRLEN];
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
    sprintf(err, "%s: got non-exist pos (%g,%g,%g,%g)", me, 
            point->pos[0], point->pos[1], point->pos[2], point->pos[3]);
    biffAdd(PULL, err); return 1;
  }
  if (task->pctx->verbose > 3) {
    printf("%s: hello; probing %u volumes\n", me, task->pctx->volNum);
  }
  edge = AIR_FALSE;
  for (ii=0; ii<task->pctx->volNum; ii++) {
    if (task->pctx->iter && task->vol[ii]->seedOnly) {
      /* its after the 1st iteration (#0), and this vol is only for seeding */
      continue;
    }
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
        printf("%s: vol[%u] has scale (%u)-> gageStackProbeSpace(%p) (v %d)\n",
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
      break;
    }
    edge |= !!task->vol[ii]->gctx->edgeFrac;
  }
  if (gret) {
    sprintf(err, "%s: probe failed on vol %u/%u: (%d) %s", me,
            ii, task->pctx->volNum,
            task->vol[ii]->gctx->errNum, task->vol[ii]->gctx->errStr);
    biffAdd(PULL, err); return 1;
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
                             int taskOrder[3]) {
  char me[]="_pullPointInitializePerVoxel", err[BIFF_STRLEN];
  unsigned int voxNum, vidx[3], sidx, yzi, upointIdx;
  unsigned int numScales;
  double iPos[3];
  airRandMTState *rng;
  pullVolume *seedVol;
  gageShape *seedShape;
  double sigmaValue;
  double tau0, tau1, deltaS=0.0;
  int reject, constrFail;
  unsigned int k, task;

  seedVol = pctx->vol[pctx->ispec[pullInfoSeedThresh]->volIdx]; 
  seedShape = seedVol->gctx->shape; 
  voxNum = seedShape->size[0]*seedShape->size[1]*seedShape->size[2];
  
  rng = pctx->task[0]->rng;

  if (pctx->haveScale) {
    numScales = pctx->numSamplesScale;
  } else {
    numScales = 1;
  }

  /* Obtain voxel and indexex from upointId */
  /* We assume that scale is the fastest axis, then x, then y and then z */
  sidx = pointIdx % numScales;
  upointIdx = (pointIdx - sidx)/numScales;
  if (pctx->pointPerVoxel > 0) {
    upointIdx /= pctx->pointPerVoxel;
  } else {
    upointIdx *= -pctx->pointPerVoxel;
  }
  /*
  printf("!%s: pointIdx %u ppv %d -> sidx %u -> upi %u\n", me,
         pointIdx, pctx->pointPerVoxel, sidx, upointIdx);
  */
  vidx[0] = upointIdx % seedShape->size[0];
  yzi = (upointIdx - vidx[0])/seedShape->size[0];
  vidx[1] = yzi % seedShape->size[1];
  vidx[2] = (yzi- vidx[1])/seedShape->size[1];
  /*
  printf("!%s: upi %u -> vidx %u %u %u\n", me, upointIdx,
         vidx[0], vidx[1], vidx[2]);
  */

  /* Compute sigma value for sidx */
  if (pctx->haveScale) {
    tau0 = gageTauOfSig(pctx->bboxMin[3]);
    tau1 = gageTauOfSig(pctx->bboxMax[3]);
    deltaS = (pctx->bboxMax[3]-pctx->bboxMin[3])/pctx->numSamplesScale;
    sigmaValue = gageSigOfTau(tau0+(sidx+1)*(tau1-tau0)/(numScales+1));
  } else {
    sigmaValue = 0;
  }
  /* Compute true point location from indexes */
  for (k=0; k<3; k++) {
    iPos[k] = vidx[k] + pctx->jitter*airDrandMT_r(rng) - 0.5;
  }
  gageShapeItoW(seedShape, point->pos, iPos);
  if (pctx->haveScale) {
    point->pos[3] = (sigmaValue 
                     + pctx->jitter*airDrandMT_r(rng)*deltaS - deltaS/2);
  } else {
    point->pos[3] = 0.0;
  }
  /*
  printf("!%s: idx %u %u %u --> wrl %g %g %g %g\n", me, 
         vidx[0], vidx[1], vidx[2],
         point->pos[0], point->pos[1], point->pos[2], point->pos[3]);
  */

  /* Do a tentative probe */
  if (_pullProbe(pctx->task[0], point)) {
   sprintf(err, "%s: probing pointIdx %u of world", me, pointIdx);
   biffAdd(PULL, err); return 1;
  }

  /* Check that point is fit enough to be included */
  /* Task = 0 -> PreThreshold;
     Task = 1 -> SeedThreshold;
     Task = 2 -> Constraint; */
  if (pctx->constraintBeforeSeedThresh) {
    ELL_3V_SET(taskOrder, 0, 2, 1);
  } else {
    ELL_3V_SET(taskOrder, 0, 1, 2);
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
      break;
    case 2:
      if (!reject && pctx->constraint) {
        if (_pullConstraintSatisfy(pctx->task[0], point, &constrFail)) {
          sprintf(err, "%s: on pnt %u",
                  me, pointIdx);
          biffAdd(PULL, err); return 1;
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
    return 1;
  }

  return 0;
}

int
_pullPointInitializeRandom(pullContext *pctx,
                           const unsigned int pointIdx,
                           pullPoint *point, pullVolume *scaleVol,
                           int taskOrder[3]) {
  char me[]="_pullPointInitializeRandom", err[BIFF_STRLEN];
  int reject, threshFail, constrFail;
  airRandMTState *rng;
  unsigned int threshFailCount = 0, constrFailCount = 0;
  rng = pctx->task[0]->rng;

  /* Check that point is fit enough to be included */
  do {
    unsigned int task;
    double rx, ry, rz;
    reject = AIR_FALSE;
    _pullPointHistInit(point);
    /* Populate tentative random point */
    rx = airDrandMT_r(rng);
    ry = airDrandMT_r(rng);
    rz = airDrandMT_r(rng);
    point->pos[0] = AIR_AFFINE(0.0, rx, 1.0,
                               pctx->bboxMin[0], pctx->bboxMax[0]);
    point->pos[1] = AIR_AFFINE(0.0, ry, 1.0,
                               pctx->bboxMin[1], pctx->bboxMax[1]);
    point->pos[2] = AIR_AFFINE(0.0, rz, 1.0,
                               pctx->bboxMin[2], pctx->bboxMax[2]);
    if (pctx->haveScale) {
      double sridx;
      int outside;
      sridx = AIR_AFFINE(0.0, airDrandMT_r(rng), 1.0,
                         0, scaleVol->scaleNum-1);
      point->pos[3] = gageStackItoW(scaleVol->gctx, sridx, &outside);
    } else {
      point->pos[3] = 0.0;
    }
    _pullPointHistAdd(point, pullCondOld);
    /* Do a tentative probe */
    if (_pullProbe(pctx->task[0], point)) {
      sprintf(err, "%s: probing pointIdx %u of world", me, pointIdx);
      biffAdd(PULL, err); return 1;
    }
    /* Enforce constrains and thresholds */
    threshFail = AIR_FALSE;
    for (task=0; task<3; task++) {
      switch (taskOrder[task]) {
      case 0:
        /* Check we pass pre-threshold */
        if (!reject && pctx->ispec[pullInfoSeedPreThresh]) {
          double seedv;
          seedv = _pullPointScalar(pctx, point, pullInfoSeedPreThresh, NULL, NULL);
          reject |= (seedv < 0);
        }
      case 1:
        if (!reject && pctx->ispec[pullInfoSeedThresh]) {
          double val;
          val = _pullPointScalar(pctx, point, pullInfoSeedThresh,
                                 NULL, NULL);
          threshFailCount += (threshFail = (val < 0));
        } else {
          threshFail = AIR_FALSE;
        }
        reject |= threshFail;
        break;
      case 2:
        /* Avoid doing constraint if the threshold has already failed */
        if (!reject && pctx->constraint) {
          if (_pullConstraintSatisfy(pctx->task[0], point, &constrFail)) {
            sprintf(err, "%s: trying constraint on point %u", me, pointIdx);
            biffAdd(PULL, err); return 1;
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
      if (threshFailCount + constrFailCount > 1000) {
        /* Very bad luck; we've too many times */
        sprintf(err, "%s: failed too often placing %u "
                "(thresh %s/%u, constr %s/%u)",
                me, pointIdx,
                threshFail ? "true" : "false", threshFailCount,
                constrFail ? "true" : "false", constrFailCount);
        biffAdd(PULL, err); return 1;
      }
    }
  } while (reject);

  return 0;
}

/*
** _pullPointSetup sets:
**** pctx->pointNumInitial (in case pctx->npos)
**** pctx->pointPerVoxel (in case pctx->npos)
**
** This is only called by the master thread
** 
** this should set stuff to be like after an update stage and
** just before the rebinning
*/
int
_pullPointSetup(pullContext *pctx) {
  char me[]="_pullPointSetup", err[BIFF_STRLEN], doneStr[AIR_STRLEN_SMALL];
  unsigned int pointIdx, binIdx, tick, pn;
  pullPoint *point;
  pullBin *bin;
  int createFail,added, random, taskOrder[3];
  airArray *mop;
  Nrrd *npos;
  double *posData;
  pullVolume *seedVol, *scaleVol;
  gageShape *seedShape;
  double factor;
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
  if (pctx->npos) {
    npos = nrrdNew();
    airMopAdd(mop, npos, (airMopper)nrrdNuke, airMopAlways);
    if (nrrdConvert(npos, pctx->npos, nrrdTypeDouble)) {
      sprintf(err, "%s: trouble converting npos", me);
      biffMove(PULL, err, NRRD); airMopError(mop); return 1;
    }
    if (pctx->pointNumInitial || pctx->pointPerVoxel) {
      printf("%s: with npos, overriding both pointNumInitial (%u) "
             "and pointPerVoxel (%d)", me, pctx->pointNumInitial,
             pctx->pointPerVoxel);
    }
    pctx->pointNumInitial = npos->axis[1].size;
    pctx->pointPerVoxel = 0;
    posData = npos->data;
    totalNumPoints = pctx->pointNumInitial;
    random = AIR_FALSE;
  } else if (pctx->pointPerVoxel) {
    npos = NULL;
    posData = NULL;
    if (pctx->pointNumInitial) {
      printf("%s: pointPerVoxel %d overrides pointNumInitial (%u)\n",
             me, pctx->pointPerVoxel, pctx->pointNumInitial);
    }
    /* Obtain number of voxels */
    seedVol = pctx->vol[pctx->ispec[pullInfoSeedThresh]->volIdx];
    seedShape = seedVol->gctx->shape;
    voxNum = seedShape->size[0]*seedShape->size[1]*seedShape->size[2];
    printf("%s: vol size %u %u %u -> voxNum %u\n", me, 
           seedShape->size[0], seedShape->size[1], seedShape->size[2],
           voxNum);

    /* Compute total number of points */
    if (pctx->pointPerVoxel > 0) {
      factor = pctx->pointPerVoxel;
    } else {
      factor = -1.0/pctx->pointPerVoxel;
    }
    if (pctx->haveScale) {
      totalNumPoints = voxNum * factor * pctx->numSamplesScale;
    } else {
      totalNumPoints = voxNum * factor;
    }
    printf("!%s: ppv %d -> factor %g -> tot # %u\n", me, 
           pctx->pointPerVoxel, factor, totalNumPoints);

    /* Reset pointNumInitial to zero */
    pctx->pointNumInitial = 0;
    random = AIR_FALSE;
  } else {
    /* using pctx->pointNumInitial */
    npos = NULL;
    posData = NULL;
    totalNumPoints = pctx->pointNumInitial;
    random = AIR_TRUE;
  }
  printf("!%s: initializing/seeding ...       ", me);
  fflush(stdout);

  /* find first scale volume, if there is one */
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
  if (pctx->constraintBeforeSeedThresh) {
    ELL_3V_SET(taskOrder, 0, 2, 1);
  } else {
    ELL_3V_SET(taskOrder, 0, 1, 2);
  }
  
  /* Start adding points */
  tick = totalNumPoints/1000;
  point = NULL;
  for (pointIdx = 0; pointIdx < totalNumPoints; pointIdx++) {
    if (tick < 100 || 0 == pointIdx % tick) {
      printf("%s", airDoneStr(0, pointIdx, totalNumPoints, doneStr));
      fflush(stdout);
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
    if (pctx->pointPerVoxel) {
      createFail = _pullPointInitializePerVoxel(pctx, pointIdx,
                                                point, scaleVol,
                                                taskOrder);
    } else if (random) {
      /* random sampling */
      createFail = _pullPointInitializeRandom(pctx, pointIdx,
                                              point, scaleVol,
                                              taskOrder);
    } else {
      /* npos is given to us */
      /* Copy nrrd point into pullPoint */
      ELL_4V_COPY(point->pos, posData + 4*pointIdx);
      createFail = AIR_FALSE;
      /* even though we are dictating the point locations, we still have
         to do the initial probe */
      if (_pullProbe(pctx->task[0], point)) {
        sprintf(err, "%s: probing pointIdx %u of npos", me, pointIdx);
        biffAdd(PULL, err); airMopError(mop); return 1;
      }
    }
    
    if (createFail) {
      /* We were not succesful creating a point */
      continue;
    }
 
    /* If we get here, the point is ready for binning */
    if (pctx->constraint) {
      if (pullBinsPointMaybeAdd(pctx, point, NULL, &added)) {
        sprintf(err, "%s: trouble binning point %u", me, point->idtag);
        biffAdd(PULL, err); airMopError(mop); return 1;
      }
      if (added) {
        point = NULL;
        if (pctx->pointPerVoxel) {
          pctx->pointNumInitial += 1;
        }
      }
    } else {
      if (pullBinsPointAdd(pctx, point, NULL)) {
          sprintf(err, "%s: trouble binning point %u", me, point->idtag);
          biffAdd(PULL, err); airMopError(mop); return 1;
      }
      point = NULL;
      if (pctx->pointPerVoxel) {
        pctx->pointNumInitial += 1;
      }
    }
  } /* Done looping through total number of points */
  printf("%s\n", airDoneStr(0, pointIdx, totalNumPoints,
                            doneStr));
  if (point) {
    /* we created a new test point, but it was never placed in the volume */
    /* so, HACK: undo pullPointNew ... */
    point = pullPointNix(point);
    pctx->idtagNext -= 1;
  }
  
  /* Final check: do we have any point?. This is for pointPerVoxel */
  if (!pctx->pointNumInitial) {
    sprintf(err, "%s: seeding never succeeded (bad seedthresh? %g)",
            me, pctx->ispec[pullInfoSeedThresh]->zero);
    biffAdd(PULL, err); airMopError(mop); return 1;
  }

  pn = pullPointNumber(pctx);
  if (!pn) {
    sprintf(err, "%s: point initialization failed, no points!\n", me);
    biffAdd(PULL, err); airMopError(mop); return 1;
  }
  printf("%s: ended up with %u points\n", me, pn);
  pctx->tmpPointPtr = AIR_CAST(pullPoint **,
                               calloc(pn, sizeof(pullPoint*)));
  pctx->tmpPointPerm = AIR_CAST(unsigned int *,
                                calloc(pn, sizeof(unsigned int)));
  if (!( pctx->tmpPointPtr && pctx->tmpPointPerm )) {
    sprintf(err, "%s: couldn't allocate tmp buffers %p %p", me, 
            pctx->tmpPointPtr, pctx->tmpPointPerm);
    biffAdd(PULL, err); airMopError(mop); return 1;
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
