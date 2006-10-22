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

#include "push.h"
#include "privatePush.h"

/*
** this is run once per task (thread)
*/
int
_pushProcess(pushTask *task) {
  char me[]="_pushProcess", err[BIFF_STRLEN];
  unsigned int binIdx;
  
  while (task->pctx->binIdx < task->pctx->binNum) {
    /* get the index of the next bin to process */
    if (task->pctx->threadNum > 1) {
      airThreadMutexLock(task->pctx->binMutex);
    }
    do {
      binIdx = task->pctx->binIdx;
      if (task->pctx->binIdx < task->pctx->binNum) {
        task->pctx->binIdx++;
      }
    } while (binIdx < task->pctx->binNum
             && 0 == task->pctx->bin[binIdx].thingNum
             && 0 == task->pctx->bin[binIdx].pointNum);
    if (task->pctx->threadNum > 1) {
      airThreadMutexUnlock(task->pctx->binMutex);
    }
    if (binIdx == task->pctx->binNum) {
      /* no more bins to process! */
      break;
    }
    
    if (pushBinProcess(task, binIdx)) {
      sprintf(err, "%s(%u): had trouble on bin %u", me,
              task->threadIdx, binIdx);
      biffAdd(PUSH, err); return 1;
    }

  }
  return 0;
}

/* the main loop for each worker thread */
void *
_pushWorker(void *_task) {
  char me[]="_pushWorker", *err;
  pushTask *task;
  
  task = (pushTask *)_task;

  while (1) {
    if (task->pctx->verbose > 1) {
      fprintf(stderr, "%s(%u): waiting to check finished\n",
              me, task->threadIdx);
    }
    /* pushFinish sets finished prior to the barriers */
    airThreadBarrierWait(task->pctx->iterBarrierA);
    if (task->pctx->finished) {
      if (task->pctx->verbose > 1) {
        fprintf(stderr, "%s(%u): done!\n", me, task->threadIdx);
      }
      break;
    }
    /* else there's work to do ... */
    
    if (task->pctx->verbose > 1) {
      fprintf(stderr, "%s(%u): starting to process\n", me, task->threadIdx);
    }
    if (_pushProcess(task)) {
      err = biffGetDone(PUSH);
      fprintf(stderr, "%s: thread %u trouble:\n%s", me,
              task->threadIdx, err);
      task->pctx->finished = AIR_TRUE;
      /* HEY: we should be using the "finished" mechanism to
         shut the whole production down */
    }
    airThreadBarrierWait(task->pctx->iterBarrierB);
  }

  return _task;
}

int
_pushContextCheck(pushContext *pctx) {
  char me[]="_pushContextCheck", err[BIFF_STRLEN];
  unsigned int numSingle;
  
  if (!pctx) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(PUSH, err); return 1;
  }
  if (!( pctx->thingNum >= 1 )) {
    sprintf(err, "%s: pctx->thingNum (%d) not >= 1\n", me, pctx->thingNum);
    biffAdd(PUSH, err); return 1;
  }
  if (!( AIR_IN_CL(1, pctx->threadNum, PUSH_THREAD_MAXNUM) )) {
    sprintf(err, "%s: pctx->threadNum (%d) outside valid range [1,%d]", me,
            pctx->threadNum, PUSH_THREAD_MAXNUM);
    biffAdd(PUSH, err); return 1;
  }

  if (nrrdCheck(pctx->nin)) {
    sprintf(err, "%s: got a broken input nrrd", me);
    biffMove(PUSH, err, NRRD); return 1;
  }
  if (!( (4 == pctx->nin->dim && 7 == pctx->nin->axis[0].size) )) {
    sprintf(err, "%s: input doesn't look like 3D masked tensor volume", me);
    biffAdd(PUSH, err); return 1;
  }
  numSingle = 0;
  numSingle += (1 == pctx->nin->axis[1].size);
  numSingle += (1 == pctx->nin->axis[2].size);
  numSingle += (1 == pctx->nin->axis[3].size);
  if (numSingle > 1) {
    sprintf(err, "%s: can have a single sample along at most one axis", me);
    biffAdd(PUSH, err); return 1;
  }

  if (pctx->npos) {
    if (nrrdCheck(pctx->npos)) {
      sprintf(err, "%s: got a broken position nrrd", me);
      biffMove(PUSH, err, NRRD); return 1;
    }
    if (!( 2 == pctx->npos->dim 
           && 3 == pctx->npos->axis[0].size )) {
      sprintf(err, "%s: position nrrd not 2-D 3-by-N", me);
      biffAdd(PUSH, err); return 1;
    }
  }
  if (pctx->nstn) {
    if (!pctx->npos) {
      sprintf(err, "%s: can't have start/num nrrd w/out position nrrd", me);
      biffAdd(PUSH, err); return 1;
    }
    if (nrrdCheck(pctx->nstn)) {
      sprintf(err, "%s: got a broken start/num nrrd", me);
      biffMove(PUSH, err, NRRD); return 1;
    }
    if (!( 2 == pctx->nstn->dim 
           && nrrdTypeUInt == pctx->nstn->type
           && 3 == pctx->nstn->axis[0].size )) {
      sprintf(err, "%s: start/num nrrd not 2-D 3-by-N array of %ss", me,
              airEnumStr(nrrdType, nrrdTypeUInt));
      biffAdd(PUSH, err); return 1;
    }
  }
  if (tenGageUnknown != pctx->gravItem) {
    unsigned int nii;
    if (airEnumValCheck(tenGage, pctx->gravItem)) {
      sprintf(err, "%s: gravity item %u invalid", me, pctx->gravItem);
      biffAdd(PUSH, err); return 1;
    }
    if (3 != tenGageKind->table[pctx->gravItem].answerLength) {
      sprintf(err, "%s: answer length of gravity item %s is %u, not 3", me,
              airEnumStr(tenGage, pctx->gravItem),
              tenGageKind->table[pctx->gravItem].answerLength);
      biffAdd(PUSH, err); return 1;
    }
    if (!AIR_EXISTS(pctx->gravScl)) {
      sprintf(err, "%s: gravity scaling doesn't exist", me);
      biffAdd(PUSH, err); return 1;
    }
    for (nii=0; nii<=1; nii++) {
      if (tenGageUnknown != pctx->gravNotItem[nii]) {
        if (airEnumValCheck(tenGage, pctx->gravNotItem[nii])) {
          sprintf(err, "%s: not gravity item[%u] %u invalid",
                  me, nii, pctx->gravNotItem[nii]);
          biffAdd(PUSH, err); return 1;
        }
        if (3 != tenGageKind->table[pctx->gravItem].answerLength) {
          sprintf(err, "%s: answer length of not gravity item[%u] "
                  "%s is %u, not 3", me, nii,
                  airEnumStr(tenGage, pctx->gravNotItem[nii]),
                  tenGageKind->table[pctx->gravNotItem[nii]].answerLength);
          biffAdd(PUSH, err); return 1;
        }
      }
    }
  }
  return 0;
}

int
pushStart(pushContext *pctx) {
  char me[]="pushStart", err[BIFF_STRLEN];
  unsigned int tidx;

  if (_pushContextCheck(pctx)) {
    sprintf(err, "%s: trouble", me);
    biffAdd(PUSH, err); return 1;
  }

  airSrandMT(pctx->seedRNG);
  
  /* the ordering of things below is important: gage and fiber contexts
     have to be set up before they're copied by task setup */
  if (_pushTensorFieldSetup(pctx)
      || _pushGageSetup(pctx) 
      || _pushFiberSetup(pctx)
      || _pushTaskSetup(pctx)
      || _pushBinSetup(pctx)
      || _pushThingSetup(pctx)) {
    sprintf(err, "%s: trouble setting up context", me);
    biffAdd(PUSH, err); return 1;
  }
  fprintf(stderr, "!%s: setup done-ish\n", me);

  pctx->finished = AIR_FALSE;
  if (pctx->threadNum > 1) {
    pctx->binMutex = airThreadMutexNew();
    pctx->iterBarrierA = airThreadBarrierNew(pctx->threadNum);
    pctx->iterBarrierB = airThreadBarrierNew(pctx->threadNum);
  }

  /* start threads 1 and up running; they'll all hit iterBarrierA  */
  for (tidx=1; tidx<pctx->threadNum; tidx++) {
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
** NB: this implements the body of thread 0, the master thread
*/
int
pushIterate(pushContext *pctx) {
  char me[]="pushIterate", *_err, err[BIFF_STRLEN];
  unsigned int ti, thingNum;
  double time0, time1;

  if (!pctx) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(PUSH, err); return 1;
  }
  
  if (pctx->verbose) {
    fprintf(stderr, "%s: starting iterations\n", me);
  }

  time0 = airTime();

  /* the _pushWorker checks finished after iterBarrierA */
  pctx->finished = AIR_FALSE;
  pctx->binIdx=0;
  for (ti=0; ti<pctx->threadNum; ti++) {
    pctx->task[ti]->sumVel = 0;
    pctx->task[ti]->thingNum = 0;
  }

  if (pctx->threadNum > 1) {
    airThreadBarrierWait(pctx->iterBarrierA);
  }
  if (pctx->verbose) {
    fprintf(stderr, "%s: starting iter %d\n", me, pctx->iter);
  }
  if (_pushProcess(pctx->task[0])) {
    _err = biffGetDone(PUSH);
    fprintf(stderr, "%s: task %u trouble w/ iter %d\n%s", me,
            pctx->task[0]->threadIdx, pctx->iter, _err);
    return 1;
  }
  if (pctx->threadNum > 1) {
    airThreadBarrierWait(pctx->iterBarrierB);
  }

  /* This is the code that happens between iteration barriers */
  pctx->meanVel = 0;
  thingNum = 0;
  for (ti=0; ti<pctx->threadNum; ti++) {
    pctx->meanVel += pctx->task[ti]->sumVel;
    /*
    fprintf(stderr, "!%s: task %d sumVel = %g\n", me,
            ti, pctx->task[ti]->sumVel);
    */
    thingNum += pctx->task[ti]->thingNum;
  }
  pctx->meanVel /= thingNum;
  if (pushRebin(pctx)) {
    sprintf(err, "%s: problem with new point locations", me);
    biffAdd(PUSH, err); return 1;
  }

  time1 = airTime();
  pctx->timeIteration = time1 - time0;
  pctx->iter += 1;

  return 0;
}

int
pushRun(pushContext *pctx) {
  char me[]="pushRun", err[BIFF_STRLEN],
    poutS[AIR_STRLEN_MED], toutS[AIR_STRLEN_MED], soutS[AIR_STRLEN_MED];
  Nrrd *npos, *nten, *nstn;
  double time0, time1;
  
  time0 = airTime();
  pctx->iter = 0;
  do {
    if (pushIterate(pctx)) {
      sprintf(err, "%s: trouble on iter %d", me, pctx->iter);
      biffAdd(PUSH, err); return 1;
    }
    if (pctx->snap && !(pctx->iter % pctx->snap)) {
      nten = nrrdNew();
      npos = nrrdNew();
      nstn = nrrdNew();
      sprintf(poutS, "snap.%06d.pos.nrrd", pctx->iter);
      sprintf(toutS, "snap.%06d.ten.nrrd", pctx->iter);
      sprintf(soutS, "snap.%06d.stn.nrrd", pctx->iter);
      if (pushOutputGet(npos, nten, nstn, pctx)) {
        sprintf(err, "%s: couldn't get snapshot for iter %d", me, pctx->iter);
        biffAdd(PUSH, err); return 1;
      }
      fprintf(stderr, "%s: %s, meanVel=%g, %g iter/sec\n", me,
              poutS, pctx->meanVel, pctx->iter/(airTime()-time0));
      if (nrrdSave(poutS, npos, NULL)
          || nrrdSave(toutS, nten, NULL)
          || nrrdSave(soutS, nstn, NULL)) {
        sprintf(err, "%s: couldn't save snapshot for iter %d", me, pctx->iter);
        biffMove(PUSH, err, NRRD); return 1;
      }
      nten = nrrdNuke(nten);
      npos = nrrdNuke(npos);
      nstn = nrrdNuke(nstn);
    }
    pctx->iter++;
  } while ( (pctx->meanVel > pctx->minMeanVel
             && (0 == pctx->maxIter
                 || pctx->iter < pctx->maxIter)) );
  time1 = airTime();

  pctx->timeRun = time1 - time0;

  return 0;
}

/*
** this is called *after* pushOutputGet
**
** should nix everything created by the many _push*Setup() functions
*/
int
pushFinish(pushContext *pctx) {
  char me[]="pushFinish", err[BIFF_STRLEN];
  unsigned int ii, tidx;

  if (!pctx) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(PUSH, err); return 1;
  }

  pctx->finished = AIR_TRUE;
  if (pctx->threadNum > 1) {
    if (pctx->verbose > 1) {
      fprintf(stderr, "%s: finishing workers\n", me);
    }
    airThreadBarrierWait(pctx->iterBarrierA);
  }
  /* worker threads now pass barrierA and see that finished is AIR_TRUE,
     and then bail, so now we collect them */
  for (tidx=pctx->threadNum; tidx>0; tidx--) {
    if (tidx-1) {
      airThreadJoin(pctx->task[tidx-1]->thread,
                    &(pctx->task[tidx-1]->returnPtr));
    }
    pctx->task[tidx-1]->thread = airThreadNix(pctx->task[tidx-1]->thread);
    pctx->task[tidx-1] = _pushTaskNix(pctx->task[tidx-1]);
  }
  pctx->task = (pushTask **)airFree(pctx->task);

  pctx->nten = nrrdNuke(pctx->nten);
  pctx->ninv = nrrdNuke(pctx->ninv);
  pctx->nmask = nrrdNuke(pctx->nmask);
  pctx->gctx = gageContextNix(pctx->gctx);
  pctx->fctx = tenFiberContextNix(pctx->fctx);
  for (ii=0; ii<pctx->binNum; ii++) {
    pushBinDone(pctx->bin + ii);
  }
  pctx->bin = (pushBin *)airFree(pctx->bin);
  ELL_3V_SET(pctx->binsEdge, 0, 0, 0);
  pctx->binNum = 0;

  if (pctx->threadNum > 1) {
    pctx->binMutex = airThreadMutexNix(pctx->binMutex);
    pctx->iterBarrierA = airThreadBarrierNix(pctx->iterBarrierA);
    pctx->iterBarrierB = airThreadBarrierNix(pctx->iterBarrierB);
  }

  return 0;
}
