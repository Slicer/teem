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

int
_pullInfoSetup(pullContext *pctx) {
  char me[]="_pullInfoSetup", err[BIFF_STRLEN];
  unsigned int ii;

  for (ii=0; ii<pctx->volNum; ii++) {
    if (gageUpdate(pctx->vol[ii]->gctx)) {
      sprintf(err, "%s: trouble setting up gage on vol %u/%u",
              me, ii, pctx->volNum);
      biffMove(PULL, err, GAGE); return 1;
    }
  }
  pctx->infoTotalLen = 0;
  for (ii=0; ii<pctx->ispecNum; ii++) {
    pctx->infoTotalLen += pullInfoLen(pctx->ispec[ii]->info);
  }
  fprintf(stderr, "!%s: infoTotalLen = %u\n", me, pctx->infoTotalLen);
  return 0;
}

pullTask *
_pullTaskNew(pullContext *pctx, int threadIdx) {
  char me[]="_pullTaskNew", err[BIFF_STRLEN];
  pullTask *task;
  unsigned int ii;

  task = (pullTask *)calloc(1, sizeof(pullTask));
  if (!task) {
    sprintf(err, "%s: couldn't allocate task", me);
    biffAdd(PULL, err); return NULL;
  }    

  task->pctx = pctx;
  for (ii=0; ii<pctx->volNum; ii++) {
    if (!(task->vol[ii] = _pullVolumeCopy(pctx->vol[ii]))) {
      sprintf(err, "%s: trouble copying vol %u/%u", me, ii, pctx->volNum);
      biffAdd(PULL, err); return NULL;
    }
  }
#if 0    
  task->tenAns = gageAnswerPointer(task->gctx, task->gctx->pvl[0],
                                   tenGageTensor);
  task->invAns = gageAnswerPointer(task->gctx, task->gctx->pvl[1],
                                   tenGageTensor);
  task->cntAns = gageAnswerPointer(task->gctx, task->gctx->pvl[2],
                                   gageSclGradVec);
  if (tenGageUnknown != task->pctx->gravItem) {
    task->gravAns = gageAnswerPointer(task->gctx, task->gctx->pvl[0],
                                      task->pctx->gravItem);
    task->gravGradAns = gageAnswerPointer(task->gctx, task->gctx->pvl[0],
                                          task->pctx->gravGradItem);
  } else {
    task->gravAns = NULL;
    task->gravGradAns = NULL;
  }
  if (tenGageUnknown != task->pctx->seedThreshItem) {
    task->seedThreshAns = gageAnswerPointer(task->gctx, task->gctx->pvl[0],
                                            task->pctx->seedThreshItem);
  } else {
    task->seedThreshAns = NULL;
  }
  if (threadIdx) {
    task->thread = airThreadNew();
  }
  task->rng = airRandMTStateNew(pctx->seedRNG + threadIdx);
  task->threadIdx = threadIdx;
  task->pointNum = 0;
  task->energySum = 0;
  task->deltaFracSum = 0;
  task->returnPtr = NULL;
#endif
  return task;
}

pullTask *
_pullTaskNix(pullTask *task) {
#if 0
  if (task) {
    task->gctx = gageContextNix(task->gctx);
    if (task->threadIdx) {
      task->thread = airThreadNix(task->thread);
    }
    task->rng = airRandMTStateNix(task->rng);
    airFree(task);
  }
#endif
  return NULL;
}

/*
** _pullTaskSetup sets:
**** pctx->task
**** pctx->task[]
*/
int
_pullTaskSetup(pullContext *pctx) {
  char me[]="_pullTaskSetup", err[BIFF_STRLEN];
  unsigned int tidx;

  pctx->task = (pullTask **)calloc(pctx->threadNum, sizeof(pullTask *));
  if (!(pctx->task)) {
    sprintf(err, "%s: couldn't allocate array of tasks", me);
    biffAdd(PULL, err); return 1;
  }
  for (tidx=0; tidx<pctx->threadNum; tidx++) {
    if (pctx->verbose) {
      fprintf(stderr, "%s: creating task %u/%u\n", me, tidx, pctx->threadNum);
    }
    pctx->task[tidx] = _pullTaskNew(pctx, tidx);
    if (!(pctx->task[tidx])) {
      sprintf(err, "%s: couldn't allocate task %d", me, tidx);
      biffAdd(PULL, err); return 1;
    }
  }
  return 0;
}

int
_pullBinSetup(pullContext *pctx) {
  char me[]="_pullBinSetup", err[BIFF_STRLEN];
  unsigned ii;
  double volEdge[3];

  gageShapeBoundingBox(pctx->bboxMin, pctx->bboxMax,
                       pctx->vol[0]->gctx->shape);
  for (ii=1; ii<pctx->volNum; ii++) {
    double min[3], max[3];
    gageShapeBoundingBox(min, max, pctx->vol[ii]->gctx->shape);
    ELL_3V_MIN(pctx->bboxMin, pctx->bboxMin, min);
    ELL_3V_MIN(pctx->bboxMax, pctx->bboxMax, max);
  }
  fprintf(stderr, "!%s: bbox min (%g,%g,%g) max (%g,%g,%g)\n", me,
          pctx->bboxMin[0], pctx->bboxMin[1], pctx->bboxMin[2],
          pctx->bboxMax[0], pctx->bboxMax[1], pctx->bboxMax[2]);
  
  pctx->maxDist = (pctx->interScl ? pctx->interScl : 0.2);
  fprintf(stderr, "!%s: interScl = %g --> maxDist = %g\n", me, 
          pctx->interScl, pctx->maxDist);

  if (pctx->binSingle) {
    pctx->binsEdge[0] = 1;
    pctx->binsEdge[1] = 1;
    pctx->binsEdge[2] = 1;
    pctx->binNum = 1;
  } else {
    volEdge[0] = pctx->bboxMax[0] - pctx->bboxMin[0];
    volEdge[1] = pctx->bboxMax[1] - pctx->bboxMin[1];
    volEdge[2] = pctx->bboxMax[2] - pctx->bboxMin[2];
    fprintf(stderr, "!%s: volEdge = %g %g %g\n", me,
            volEdge[0], volEdge[1], volEdge[2]);
    pctx->binsEdge[0] = AIR_CAST(unsigned int,
                                 floor(volEdge[0]/pctx->maxDist));
    pctx->binsEdge[0] = pctx->binsEdge[0] ? pctx->binsEdge[0] : 1;
    pctx->binsEdge[1] = AIR_CAST(unsigned int,
                                 floor(volEdge[1]/pctx->maxDist));
    pctx->binsEdge[1] = pctx->binsEdge[1] ? pctx->binsEdge[1] : 1;
    pctx->binsEdge[2] = AIR_CAST(unsigned int,
                                 floor(volEdge[2]/pctx->maxDist));
    pctx->binsEdge[2] = pctx->binsEdge[2] ? pctx->binsEdge[2] : 1;
    fprintf(stderr, "!%s: binsEdge=(%u,%u,%u)\n", me,
            pctx->binsEdge[0], pctx->binsEdge[1], pctx->binsEdge[2]);
    pctx->binNum = pctx->binsEdge[0]*pctx->binsEdge[1]*pctx->binsEdge[2];
  }
  pctx->bin = (pullBin *)calloc(pctx->binNum, sizeof(pullBin));
  if (!( pctx->bin )) {
    sprintf(err, "%s: trouble allocating bin arrays", me);
    biffAdd(PULL, err); return 1;
  }
  for (ii=0; ii<pctx->binNum; ii++) {
    pullBinInit(pctx->bin + ii, pctx->binIncr);
  }
  pullBinAllNeighborSet(pctx);
  return 0;
}

/*
** _pullPointSetup sets:
**** pctx->pointNum (in case pctx->npos)
**
** This is only called by the master thread
** 
** this should set stuff to be like after an update stage and
** just before the rebinning
*/
int
_pullPointSetup(pullContext *pctx) {
  char me[]="_pullPointSetup", err[BIFF_STRLEN];
#if 0
  double (*lup)(const void *v, size_t I), maxDet;
  unsigned int pointIdx;
  pullPoint *point;

  pctx->pointNum = (pctx->npos
                    ? pctx->npos->axis[1].size
                    : pctx->pointNum);
  lup = pctx->npos ? nrrdDLookup[pctx->npos->type] : NULL;
  fprintf(stderr, "!%s: initilizing/seeding ... \n", me);
  /* HEY: we end up keeping a local copy of maxDet because convolution
     can produce a tensor with higher determinant than that of any
     original sample.  However, if this is going into effect,
     detReject should probably *not* be enabled... */
  maxDet = pctx->maxDet;
  for (pointIdx=0; pointIdx<pctx->pointNum; pointIdx++) {
    double detProbe;
    /*
    fprintf(stderr, "!%s: pointIdx = %u/%u\n", me, pointIdx, pctx->pointNum);
    */
    point = pullPointNew(pctx);
    if (pctx->npos) {
      ELL_3V_SET(point->pos,
                 lup(pctx->npos->data, 0 + 3*pointIdx),
                 lup(pctx->npos->data, 1 + 3*pointIdx),
                 lup(pctx->npos->data, 2 + 3*pointIdx));
      if (_pullProbe(pctx->task[0], point)) {
        sprintf(err, "%s: probing pointIdx %u of npos", me, pointIdx);
        biffAdd(PULL, err); return 1;
      }
    } else {
      do {
        double posIdx[4], posWorld[4];
        posIdx[0] = AIR_AFFINE(0.0, airDrandMT(), 1.0,
                               -0.5, pctx->gctx->shape->size[0]-0.5);
        posIdx[1] = AIR_AFFINE(0.0, airDrandMT(), 1.0,
                               -0.5, pctx->gctx->shape->size[1]-0.5);
        posIdx[2] = AIR_AFFINE(0.0, airDrandMT(), 1.0,
                               -0.5, pctx->gctx->shape->size[2]-0.5);
        posIdx[3] = 1.0;
        if (2 == pctx->dimIn) {
          posIdx[pctx->sliceAxis] = 0.0;
        }
        ELL_4MV_MUL(posWorld, pctx->gctx->shape->ItoW, posIdx);
        ELL_34V_HOMOG(point->pos, posWorld);
        /*
        fprintf(stderr, "%s: posIdx = %g %g %g --> posWorld = %g %g %g "
                "--> %g %g %g\n", me,
                posIdx[0], posIdx[1], posIdx[2],
                posWorld[0], posWorld[1], posWorld[2],
                point->pos[0], point->pos[1], point->pos[2]);
        */
        if (_pullProbe(pctx->task[0], point)) {
          sprintf(err, "%s: probing pointIdx %u of world", me, pointIdx);
          biffAdd(PULL, err); return 1;
        }
        detProbe = TEN_T_DET(point->ten);

        if (2 == pctx->dimIn) {
          /* see above HEY! HEY! */
          detProbe = (0 == pctx->sliceAxis
                      ? TEN_T_DET_YZ(point->ten)
                      : (1 == pctx->sliceAxis
                         ? TEN_T_DET_XZ(point->ten)
                         : TEN_T_DET_XY(point->ten)));
        } else {
          detProbe = TEN_T_DET(point->ten);
        }
        maxDet = AIR_MAX(maxDet, detProbe);
        /* assuming that we're not using some very blurring kernel,
           this will eventually succeed, because we previously checked
           the range of values in the mask */
        /* HEY: can't ensure that this will eventually succeed with
           seedThresh enabled! */
        /*
        fprintf(stderr, "!%s: ten[0] = %g\n", me, point->ten[0]);
        */
        /* we OR together all the tests that would
           make us REJECT this last sample */
      } while (point->ten[0] < 0.5
               || (tenGageUnknown != pctx->seedThreshItem
                   && ((pctx->seedThresh - point->seedThresh)
                       *pctx->seedThreshSign > 0)
                   )
               || (pctx->detReject 
                   && (airDrandMT() < detProbe/maxDet))
               );
    }
    if (pullBinPointAdd(pctx, point)) {
      sprintf(err, "%s: trouble binning point %u", me, point->ttaagg);
      biffAdd(PULL, err); return 1;
    }
  }
  fprintf(stderr, "!%s: ... seeding DONE\n", me);
#endif
  return 0;
}

