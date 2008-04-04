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

pullPoint *
pullPointNew(pullContext *pctx) {
  char me[]="pullPointNew", err[BIFF_STRLEN];
  pullPoint *pnt;
  unsigned int ii;
  
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
  pnt = AIR_CAST(pullPoint *,
                 calloc(1, sizeof(pullPoint)
                        + sizeof(double)*(pctx->infoTotalLen - 1)));
  if (!pnt) {
    sprintf(err, "%s: couldn't allocate point (info len %u)\n", me, 
            pctx->infoTotalLen - 1);
    biffAdd(PULL, err); return NULL;
  }

  pnt->idtag = pctx->idtagNext++;
  pnt->neigh = NULL;
  pnt->neighNum = 0;
  pnt->neighArr = airArrayNew((void**)&(pnt->neigh), &(pnt->neighNum),
                              sizeof(pullPoint *), PULL_POINT_NEIGH_INCR);
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
  dst->neigh = src->neigh;
  dst->neighNum = src->neighNum;
  dst->neighArr = src->neighArr;
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

  /* HEY: shouldn't this be airArrayNuke? */
  pnt->neighArr = airArrayNix(pnt->neighArr);
  airFree(pnt);
  return NULL;
}

unsigned int
_pullPointNumber(const pullContext *pctx) {
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
  char me[]="_pullEnergyTotal";
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

double
_pullPointHeight(const pullContext *pctx, const pullPoint *point,
                 /* output */
                 double grad[4], double hess[9]) {
  const pullInfoSpec *ispec;
  double val;

  ispec = pctx->ispec[pullInfoHeight];
  val = point->info[pctx->infoIdx[pullInfoHeight]];
  val = (val - ispec->zero)*ispec->scale;
  if (grad && pctx->ispec[pullInfoHeightGradient]) {
    ELL_3V_SCALE(grad, ispec->scale, 
                 point->info + pctx->infoIdx[pullInfoHeightGradient]);
    grad[3] = 0;
  }
  if (hess && pctx->ispec[pullInfoHeightHessian]) {
    ELL_3M_COPY(hess, point->info + pctx->infoIdx[pullInfoHeightHessian]);
    ELL_3M_SCALE(hess, ispec->scale, hess);
  }
  return val;
}

double
_pullPointStrength(const pullContext *pctx, const pullPoint *point) {
  const pullInfoSpec *ispec;
  const unsigned int *infoIdx;
  double val;

  ispec = pctx->ispec[pullInfoStrength];
  infoIdx = pctx->infoIdx;
  val = point->info[infoIdx[pullInfoStrength]];
  val = (val - ispec->zero)*ispec->scale;
  return val;
}

int
_pullProbe(pullTask *task, pullPoint *point, double pos[4]) {
  char me[]="_pullProbe", err[BIFF_STRLEN];
  unsigned int ii, gret=0;
  
  if (!ELL_4V_EXISTS(point->pos)) {
    sprintf(err, "%s: got non-exist pos (%g,%g,%g,%g)", me, 
            pos[0], pos[1], pos[2], pos[3]);
    biffAdd(PULL, err); return 1;
  }
  for (ii=0; ii<task->pctx->volNum; ii++) {
    if (task->pctx->iter && task->vol[ii]->seedOnly) {
      /* its after the 1st iteration (#0), and this vol is only for seeding */
      continue;
    }
    if (task->vol[ii]->ninSingle) {
      gret = gageProbeSpace(task->vol[ii]->gctx,
                            pos[0], pos[1], pos[2],
                            AIR_FALSE, AIR_TRUE);
    } else {
      gret = gageStackProbeSpace(task->vol[ii]->gctx,
                                 pos[0], pos[1], pos[2], pos[3],
                                 AIR_FALSE, AIR_TRUE);
    }
    if (gret) {
      break;
    }
  }
  if (gret) {
    sprintf(err, "%s: probe failed on vol %u/%u: (%d) %s\n", me,
            ii, task->pctx->volNum,
            task->vol[ii]->gctx->errNum, task->vol[ii]->gctx->errStr);
    biffAdd(PULL, err); return 1;
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
    }
  }
  return 0;
}

/*
** _pullPointSetup sets:
**** pctx->pointNumInitial (in case pctx->npos)
**
** This is only called by the master thread
** 
** this should set stuff to be like after an update stage and
** just before the rebinning
*/
int
_pullPointSetup(pullContext *pctx) {
  char me[]="_pullPointSetup", err[BIFF_STRLEN], doneStr[AIR_STRLEN_SMALL];
  unsigned int pointIdx, tick, pn;
  pullPoint *point;
  double *posData;
  airRandMTState *rng;
  int reject;

  pctx->pointNumInitial = (pctx->npos
                           ? pctx->npos->axis[1].size
                           : pctx->pointNumInitial);
  posData = (pctx->npos
             ? AIR_CAST(double *, pctx->npos->data)
             : NULL);
  fprintf(stderr, "!%s: initilizing/seeding ...       ", me);
  fflush(stderr);
  rng = pctx->task[0]->rng;
  tick = pctx->pointNumInitial/1000;
  for (pointIdx=0; pointIdx<pctx->pointNumInitial; pointIdx++) {
    if (tick < 100 || 0 == pointIdx % tick) {
      fprintf(stderr, "%s", airDoneStr(0, pointIdx, pctx->pointNumInitial,
                                       doneStr));
      fflush(stderr);
    }
    if (pctx->verbose > 5) {
      fprintf(stderr, "%s: setting up point = %u/%u\n", me,
              pointIdx, pctx->pointNumInitial);
    }
    point = pullPointNew(pctx);
    if (pctx->npos) {
      ELL_4V_COPY(point->pos, posData + 4*pointIdx);
      /* even though we are dictating the point locations, we still have
         to do the initial probe */
      if (_pullProbe(pctx->task[0], point, point->pos)) {
        sprintf(err, "%s: probing pointIdx %u of npos", me, pointIdx);
        biffAdd(PULL, err); return 1;
      }
    } else {
      do {
        ELL_3V_SET(point->pos,
                   AIR_AFFINE(0.0, airDrandMT_r(rng), 1.0,
                              pctx->bboxMin[0], pctx->bboxMax[0]),
                   AIR_AFFINE(0.0, airDrandMT_r(rng), 1.0,
                              pctx->bboxMin[1], pctx->bboxMax[1]),
                   AIR_AFFINE(0.0, airDrandMT_r(rng), 1.0,
                              pctx->bboxMin[2], pctx->bboxMax[2]));
        if (pctx->haveScale) {
          point->pos[3] = AIR_AFFINE(0.0, airDrandMT_r(rng), 1.0,
                                     pctx->bboxMin[3], pctx->bboxMax[3]);
        } else {
          point->pos[3] = 0.0;
        }
        if (_pullProbe(pctx->task[0], point, point->pos)) {
          sprintf(err, "%s: probing pointIdx %u of world", me, pointIdx);
          biffAdd(PULL, err); return 1;
        }
        reject = AIR_FALSE;
        if (pctx->ispec[pullInfoSeedThresh]) {
          pullInfoSpec *ispec;
          double val;
          ispec = pctx->ispec[pullInfoSeedThresh];
          val = point->info[pctx->infoIdx[pullInfoSeedThresh]];
          val = (val - ispec->zero)*ispec->scale;
          reject |= val < 0;
        }
      } while (reject);
    }
    if (pullBinsPointAdd(pctx, point)) {
      sprintf(err, "%s: trouble binning point %u", me, point->idtag);
      biffAdd(PULL, err); return 1;
    }
  }
  fprintf(stderr, "%s\n", airDoneStr(0, pointIdx, pctx->pointNumInitial,
                                     doneStr));
  pn = _pullPointNumber(pctx);
  pctx->pointBuff = AIR_CAST(pullPoint **,
                             calloc(pn, sizeof(pullPoint*)));
  pctx->pointPerm = AIR_CAST(unsigned int *,
                             calloc(pn, sizeof(unsigned int)));
  if (!( pctx->pointBuff && pctx->pointPerm )) {
      sprintf(err, "%s: couldn't allocate buffers %p %p", me, 
              pctx->pointBuff, pctx->pointPerm);
      biffAdd(PULL, err); return 1;
  }

  return 0;
}

void
_pullPointFinish(pullContext *pctx) {

  airFree(pctx->pointBuff);
  airFree(pctx->pointPerm);
  return ;
}
