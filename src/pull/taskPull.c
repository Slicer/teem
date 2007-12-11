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

pullTask *
_pullTaskNew(pullContext *pctx, int threadIdx) {
  char me[]="_pullTaskNew", err[BIFF_STRLEN];
  pullTask *task;
  unsigned int ii, offset;

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
  offset = 0;
  for (ii=0; ii<=PULL_INFO_MAX; ii++) {
    unsigned int volIdx;
    if (pctx->ispec[ii]) {
      volIdx = pctx->ispec[ii]->volIdx;
      task->ans[ii] = gageAnswerPointer(task->vol[volIdx]->gctx,
                                        task->vol[volIdx]->gpvl,
                                        pctx->ispec[ii]->item);
      task->infoOffset[ii] = offset;
      offset += _pullInfoAnswerLen[ii];
    }
  }
  if (pctx->threadNum > 1) {
    task->thread = airThreadNew();
  }
  task->rng = airRandMTStateNew(pctx->seedRNG + threadIdx);
  task->threadIdx = threadIdx;
  task->pointNum = 0;
  task->energySum = 0;
  task->deltaFracSum = 0;
  task->returnPtr = NULL;
  return task;
}

pullTask *
_pullTaskNix(pullTask *task) {
  unsigned int ii;

  if (task) {
    for (ii=0; ii<task->pctx->volNum; ii++) {
      task->vol[ii] = pullVolumeNix(task->vol[ii]);
    }
    if (task->pctx->threadNum > 1) {
      task->thread = airThreadNix(task->thread);
    }
    task->rng = airRandMTStateNix(task->rng);
    airFree(task);
  }
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

void
_pullTaskFinish(pullContext *pctx) {
  unsigned int tidx;
  
  for (tidx=0; tidx<pctx->threadNum; tidx++) {
    pctx->task[tidx] = _pullTaskNix(pctx->task[tidx]);
  }
  pctx->task = airFree(pctx->task);
  return;
}
