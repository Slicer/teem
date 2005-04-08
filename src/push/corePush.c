/*
  Teem: Gordon Kindlmann's research software
  Copyright (C) 2005  Gordon Kindlmann
  Copyright (C) 2004, 2003, 2002, 2001, 2000, 1999, 1998  University of Utah

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

#include "push.h"
#include "privatePush.h"

pushTask *
_pushTaskNew(pushContext *pctx, int threadIdx) {
  pushTask *task;

  task = (pushTask *)calloc(1, sizeof(pushTask));
  if (task) {
    task->pctx = pctx;
    task->gctx = gageContextCopy(pctx->gctx);
    /* 
    ** HEY: its a limitation in gage that we have to know a priori
    ** the ordering of per-volumes in the context ...
    */
    task->tenAns = gageAnswerPointer(task->gctx, task->gctx->pvl[0],
                                     tenGageTensor);
    task->cntAns = gageAnswerPointer(task->gctx, task->gctx->pvl[1],
                                     gageSclGradVec);
    if (threadIdx) {
      task->thread = airThreadNew();
    }
    task->threadIdx = threadIdx;
    task->returnPtr = NULL;
  }
  return task;
}

pushTask *
_pushTaskNix(pushTask *task) {

  if (task) {
    task->gctx = gageContextNix(task->gctx);
    if (task->threadIdx) {
      task->thread = airThreadNix(task->thread);
    }
    free(task);
  }
  return NULL;
}

void
_pushProcessDummy(pushTask *task, int bin, 
                  double parm[PUSH_STAGE_PARM_MAX]) {
  char me[]="_pushProcessDummy";
  int i, j;

  fprintf(stderr, "%s(%d): dummy processing bin %d (stage %d)\n", me,
          task->threadIdx, bin, task->pctx->stage);
  j = 0;
  for (i=0; i<=1000000*(1+task->threadIdx); i++) {
    j += i;
  }
  for (i=0; i<=1000000*(1+task->threadIdx); i++) {
    j -= i;
  }
  for (i=0; i<=1000000*(1+task->threadIdx); i++) {
    j += i;
  }
  for (i=0; i<=1000000*(1+task->threadIdx); i++) {
    j -= i;
  }
  return;
}

/*
** this is run once per task (thread), per stage
*/
void
_pushStageRun(pushTask *task, int stage) {
  int bin;
  
  while (task->pctx->bin < task->pctx->numBin) {
    if (task->pctx->numThread > 1) {
      airThreadMutexLock(task->pctx->binMutex);
    }
    do {
      bin = task->pctx->bin;
      if (task->pctx->bin < task->pctx->numBin) {
        task->pctx->bin++;
      }
    } while (bin < task->pctx->numBin
             && 0 == task->pctx->pidxArr[bin]->len);
    if (task->pctx->numThread > 1) {
      airThreadMutexUnlock(task->pctx->binMutex);
    }

    if (bin == task->pctx->numBin) {
      /* no more bins to process */
      break;
    }

    task->pctx->process[stage](task, bin, task->pctx->stageParm[stage]);
  }
  return;
}

void *
_pushWorker(void *_task) {
  char me[]="_pushWorker";
  pushTask *task;
  
  task = (pushTask *)_task;

  while (1) {
    if (task->pctx->verbose > 1) {
      fprintf(stderr, "%s(%d): waiting to check finished\n",
              me, task->threadIdx);
    }
    /* pushFinish sets finished prior to the barriers */
    airThreadBarrierWait(task->pctx->stageBarrierA);
    if (task->pctx->finished) {
      if (task->pctx->verbose > 1) {
        fprintf(stderr, "%s(%d): done!\n", me, task->threadIdx);
      }
      break;
    }
    /* else there's work to do ... */
    
    if (task->pctx->verbose > 1) {
      fprintf(stderr, "%s(%d): starting to run stage %d\n",
              me, task->threadIdx, task->pctx->stage);
    }
    _pushStageRun(task, task->pctx->stage);
    airThreadBarrierWait(task->pctx->stageBarrierB);
  }

  return _task;
}

int
_pushContextCheck(pushContext *pctx) {
  char me[]="_pushContextCheck", err[AIR_STRLEN_MED];
  int sidx, nul;
  
  if (!pctx) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(PUSH, err); return 1;
  }
  if (!( AIR_IN_CL(1, pctx->numStage, PUSH_STAGE_MAX) )) {
    sprintf(err, "%s: pctx->numStage (%d) outside valid range [1,%d]", me,
            pctx->numStage, PUSH_STAGE_MAX);
    biffAdd(PUSH, err); return 1;
  }
  nul = AIR_FALSE;
  for (sidx=0; sidx<pctx->numStage; sidx++) {
    nul |= !(pctx->process[sidx]);
  }
  if (nul) {
    sprintf(err, "%s: one or more of the process functions was NULL", me);
    biffAdd(PUSH, err); return 1;
  }
  if (!( pctx->numPoint >= 1 )) {
    sprintf(err, "%s: pctx->numPoint (%d) not >= 1\n", me, pctx->numPoint);
    biffAdd(PUSH, err); return 1;
  }
  if (!( pctx->numThread >= 1 )) {
    sprintf(err, "%s: pctx->numThread (%d) not >= 1\n", me, pctx->numThread);
    biffAdd(PUSH, err); return 1;
  }
  if (!( AIR_IN_CL(1, pctx->numThread, PUSH_THREAD_MAX) )) {
    sprintf(err, "%s: pctx->numThread (%d) outside valid range [1,%d]", me,
            pctx->numThread, PUSH_THREAD_MAX);
    biffAdd(PUSH, err); return 1;
  }

  if (nrrdCheck(pctx->nin)) {
    sprintf(err, "%s: got a broken input nrrd", me);
    biffMove(PUSH, err, NRRD); return 1;
  }
  if (!( (3 == pctx->nin->dim && 4 == pctx->nin->axis[0].size) 
         || (4 == pctx->nin->dim && 7 == pctx->nin->axis[0].size) )) {
    sprintf(err, "%s: input doesn't look like 2D or 3D masked tensors", me);
    biffAdd(PUSH, err); return 1;
  }
  return 0;
}

int
pushStart(pushContext *pctx) {
  char me[]="pushStart", err[AIR_STRLEN_MED];
  int tidx, E;

  if (_pushContextCheck(pctx)) {
    sprintf(err, "%s: trouble", me);
    biffAdd(PUSH, err); return 1;
  }

  airSrand48(pctx->seed);

  pctx->nten = nrrdNew();
  pctx->nmask = nrrdNew();

  E = AIR_FALSE;
  E |= nrrdMaybeAlloc(pctx->nPointAttr, push_nrrdType, 2,
                      PUSH_ATTR_LEN, pctx->numPoint);
  E |= nrrdMaybeAlloc(pctx->nVelAcc, push_nrrdType, 2,
                      2*3, pctx->numPoint);
  if (E) {
    sprintf(err, "%s: couldn't allocate internal arrays", me);
    biffMove(PUSH, err, NRRD); return 1;
  }

  if (_pushInputProcess(pctx)) {
    sprintf(err, "%s: trouble creating internal input", me);
    biffAdd(PUSH, err); return 1;
  }
  /* initialize point positions and velocities in nPosVel */
  _pushInitialize(pctx);

  /* we create tasks for ALL threads, including me, thread 0 */
  pctx->task = (pushTask **)calloc(pctx->numThread, sizeof(pushTask *));
  if (!(pctx->task)) {
    sprintf(err, "%s: couldn't allocate array of tasks", me);
    biffAdd(PUSH, err); return 1;
  }
  for (tidx=0; tidx<pctx->numThread; tidx++) {
    pctx->task[tidx] = _pushTaskNew(pctx, tidx);
    if (!(pctx->task[tidx])) {
      sprintf(err, "%s: couldn't allocate task %d", me, tidx);
      biffAdd(PUSH, err); return 1;
    }
  }
  
  pctx->finished = AIR_FALSE;
  if (pctx->numThread > 1) {
    pctx->binMutex = airThreadMutexNew();
    pctx->stageBarrierA = airThreadBarrierNew(pctx->numThread);
    pctx->stageBarrierB = airThreadBarrierNew(pctx->numThread);
  }

  /* start threads 1 and up running; they'll all hit stageBarrierA  */
  for (tidx=1; tidx<pctx->numThread; tidx++) {
    if (pctx->verbose > 1) {
      fprintf(stderr, "%s: spawning thread %d\n", me, tidx);
    }
    airThreadStart(pctx->task[tidx]->thread, _pushWorker,
                   (void *)(pctx->task[tidx]));
  }

  return 0;
}

/*
******** pushIterate
**
** (documentation)
**
** NB: this implements the body of thread 0
*/
int
pushIterate(pushContext *pctx) {
  char me[]="pushIterate", err[AIR_STRLEN_MED];
  int ti;

  if (!pctx) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(PUSH, err); return 1;
  }
  
  if (pctx->verbose) {
    fprintf(stderr, "%s: starting iteration\n", me);
  }
  /* the _pushWorker checks finished after the barriers */
  pctx->finished = AIR_FALSE;
  pctx->bin=0;
  pctx->stage=0;
  for (ti=0; ti<pctx->numThread; ti++) {
    pctx->task[ti]->sumVel = 0;
  }
  do {
    if (pctx->numThread > 1) {
      airThreadBarrierWait(pctx->stageBarrierA);
    }
    if (pctx->verbose) {
      fprintf(stderr, "%s: starting stage %d\n", me, pctx->stage);
    }
    _pushStageRun(pctx->task[0], pctx->stage);
    if (pctx->numThread > 1) {
      airThreadBarrierWait(pctx->stageBarrierB);
    }
    /* This is the only code to happen between barriers */
    pctx->stage++;
    pctx->bin=0;
  } while (pctx->stage < pctx->numStage);
  pctx->meanVel = 0;
  for (ti=0; ti<pctx->numThread; ti++) {
    pctx->meanVel += pctx->task[ti]->sumVel;
  }
  pctx->meanVel /= pctx->numPoint;
  if (_pushBinPointsRebin(pctx)) {
    sprintf(err, "%s: problem with new point locations", me);
    biffAdd(PUSH, err); return 1;
  }
  
  return 0;
}

/*
** blows away nten and gctx
*/
int
pushFinish(pushContext *pctx) {
  char me[]="pushFinish", err[AIR_STRLEN_MED];
  int ii, tidx;

  if (!pctx) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(PUSH, err); return 1;
  }

  pctx->nten = nrrdNuke(pctx->nten);
  pctx->nmask = nrrdNuke(pctx->nmask);
  pctx->gctx = gageContextNix(pctx->gctx);
  pctx->tenAns = NULL;

  nrrdEmpty(pctx->nPointAttr);
  nrrdEmpty(pctx->nVelAcc);

  for (ii=0; ii<pctx->numBin; ii++) {
    pctx->pidxArr[ii] = airArrayNuke(pctx->pidxArr[ii]);
  }
  pctx->pidxArr = airFree(pctx->pidxArr);
  pctx->pidx = airFree(pctx->pidx);

  if (pctx->verbose > 1) {
    fprintf(stderr, "%s: finishing workers\n", me);
  }
  pctx->finished = AIR_TRUE;
  if (pctx->numThread > 1) {
    airThreadBarrierWait(pctx->stageBarrierA);
  }
  for (tidx=pctx->numThread-1; tidx>=0; tidx--) {
    if (tidx) {
      airThreadJoin(pctx->task[tidx]->thread, &(pctx->task[tidx]->returnPtr));
    }
    pctx->task[tidx]->thread = airThreadNix(pctx->task[tidx]->thread);
    pctx->task[tidx] = _pushTaskNix(pctx->task[tidx]);
  }
  pctx->task = airFree(pctx->task);

  if (pctx->numThread > 1) {
    pctx->binMutex = airThreadMutexNix(pctx->binMutex);
    pctx->stageBarrierA = airThreadBarrierNix(pctx->stageBarrierA);
    pctx->stageBarrierB = airThreadBarrierNix(pctx->stageBarrierB);
  }

  return 0;
}
