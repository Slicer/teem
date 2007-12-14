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

/*
** this is the core of the worker threads: as long as there are bins
** left to process, get the next one, and process it
*/
int
_pullProcess(pullTask *task) {
  char me[]="_pullProcess", err[BIFF_STRLEN];
  unsigned int binIdx;
  
  while (task->pctx->binNextIdx < task->pctx->binNum) {
    /* get the index of the next bin to process */
    if (task->pctx->threadNum > 1) {
      airThreadMutexLock(task->pctx->binMutex);
    }
    /* note that we entirely skip bins with no points */
    do {
      binIdx = task->pctx->binNextIdx;
      if (task->pctx->binNextIdx < task->pctx->binNum) {
        task->pctx->binNextIdx++;
      }
    } while (binIdx < task->pctx->binNum
             && 0 == task->pctx->bin[binIdx].pointNum);
    if (task->pctx->threadNum > 1) {
      airThreadMutexUnlock(task->pctx->binMutex);
    }
    if (binIdx == task->pctx->binNum) {
      /* no more bins to process! */
      break;
    }
    if (pullBinProcess(task, binIdx)) {
      sprintf(err, "%s(%u): had trouble on bin %u", me,
              task->threadIdx, binIdx);
      biffAdd(PULL, err); return 1;
    }

  }
  return 0;
}

/* the main loop for each worker thread */
void *
_pullWorker(void *_task) {
  char me[]="_pushWorker", err[BIFF_STRLEN];
  pullTask *task;
  
  task = (pullTask *)_task;

  while (1) {
    if (task->pctx->verbose > 1) {
      fprintf(stderr, "%s(%u): waiting on barrier A\n",
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
    if (_pullProcess(task)) {
      /* HEY clearly not threadsafe to have errors ... */
      sprintf(err, "%s: thread %u trouble", me, task->threadIdx);
      biffAdd(PULL, err); 
      task->pctx->finished = AIR_TRUE;
    }
    if (task->pctx->verbose > 1) {
      fprintf(stderr, "%s(%u): waiting on barrier B\n",
              me, task->threadIdx);
    }
    airThreadBarrierWait(task->pctx->iterBarrierB);
  }

  return _task;
}

int
pullStart(pullContext *pctx) {
  char me[]="pullStart", err[BIFF_STRLEN];
  unsigned int tidx;

  /* the ordering of steps below is important! e.g. gage context has
     to be set up (_pullVolumeSetup) by before its copied (_pullTaskSetup) */
  if (_pullContextCheck(pctx)
      || _pullVolumeSetup(pctx)
      || _pullInfoSetup(pctx) 
      || _pullTaskSetup(pctx)
      || _pullBinSetup(pctx)
      || _pullPointSetup(pctx)) {
    sprintf(err, "%s: trouble setting up context", me);
    biffAdd(PULL, err); return 1;
  }
  fprintf(stderr, "!%s: setup done-ish\n", me);

  if (pctx->threadNum > 1) {
    pctx->binMutex = airThreadMutexNew();
    pctx->iterBarrierA = airThreadBarrierNew(pctx->threadNum);
    pctx->iterBarrierB = airThreadBarrierNew(pctx->threadNum);
    /* start threads 1 and up running; they'll all hit iterBarrierA  */
    for (tidx=1; tidx<pctx->threadNum; tidx++) {
      if (pctx->verbose > 1) {
        fprintf(stderr, "%s: spawning thread %d\n", me, tidx);
      }
      airThreadStart(pctx->task[tidx]->thread, _pullWorker,
                     (void *)(pctx->task[tidx]));
    }
  } else {
    pctx->binMutex = NULL;
    pctx->iterBarrierA = NULL;
    pctx->iterBarrierB = NULL;
  }

  pctx->iter = 0;
  pctx->timeIteration = 0;
  pctx->timeRun = 0;

  return 0;
}

/*
** this is called *after* pullOutputGet
**
** should nix everything created by the many _pull*Setup() functions
*/
int
pullFinish(pullContext *pctx) {
  char me[]="pullFinish", err[BIFF_STRLEN];
  unsigned int tidx;

  if (!pctx) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(PULL, err); return 1;
  }

  pctx->finished = AIR_TRUE;
  if (pctx->threadNum > 1) {
    if (pctx->verbose > 1) {
      fprintf(stderr, "%s: finishing workers\n", me);
    }
    airThreadBarrierWait(pctx->iterBarrierA);
    /* worker threads now pass barrierA and see that finished is AIR_TRUE,
       and then bail, so now we collect them */
    for (tidx=pctx->threadNum; tidx>0; tidx--) {
      if (tidx-1) {
        airThreadJoin(pctx->task[tidx-1]->thread,
                      &(pctx->task[tidx-1]->returnPtr));
      }
    }
    pctx->binMutex = airThreadMutexNix(pctx->binMutex);
    pctx->iterBarrierA = airThreadBarrierNix(pctx->iterBarrierA);
    pctx->iterBarrierB = airThreadBarrierNix(pctx->iterBarrierB);
  }

  /* no need for _pullVolumeFinish(pctx), at least not now */
  /* no need for _pullInfoFinish(pctx), at least not now */
  _pullTaskFinish(pctx);
  _pullBinFinish(pctx);
  /* no need for _pullPointFinish(pctx): nixed bins deleted pnts inside */

  return 0;
}
/*
** _pullIterate
**
** (documentation)
**
** NB: this implements the body of thread 0, the master thread
*/
int
_pullIterate(pullContext *pctx) {
  char me[]="_pullIterate", err[BIFF_STRLEN];
  double time0, time1;
  int myError;

  if (!pctx) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(PULL, err); return 1;
  }
  
  if (pctx->verbose) {
    fprintf(stderr, "%s: starting iter %d w/ %u threads\n",
            me, pctx->iter, pctx->threadNum);
  }

  time0 = airTime();

  /* the _pullWorker checks finished after iterBarrierA */
  pctx->finished = AIR_FALSE;

  /* initialize index of next bin to be doled out to threads */
  pctx->binNextIdx=0;

  if (pctx->threadNum > 1) {
    airThreadBarrierWait(pctx->iterBarrierA);
  }
  myError = AIR_FALSE;
  if (_pullProcess(pctx->task[0])) {
    sprintf(err, "%s: master thread trouble w/ iter %u", me, pctx->iter);
    biffAdd(PULL, err);
    pctx->finished = AIR_TRUE;
    myError = AIR_TRUE;
  }
  if (pctx->threadNum > 1) {
    airThreadBarrierWait(pctx->iterBarrierB);
  }
  if (pctx->finished) {
    if (!myError) {
      /* we didn't set finished- one of the workers must have */
      sprintf(err, "%s: worker error on iter %u", me, pctx->iter);
      biffAdd(PULL, err); 
    }
    return 1;
  }
  if (pullRebin(pctx)) {
    sprintf(err, "%s: problem with new point locations", me);
    biffAdd(PULL, err); return 1;
  }

  time1 = airTime();
  pctx->timeIteration = time1 - time0;
  return 0;
}

int
pullRun(pullContext *pctx) {
  char me[]="pullRun", err[BIFF_STRLEN],
    poutS[AIR_STRLEN_MED];
  Nrrd *npos;
  double time0, time1, enrLast,
    enrNew=AIR_NAN, enrImprov=AIR_NAN, enrImprovAvg=AIR_NAN;
  int stopIter, stopConverged;
  unsigned firstIter;
  
  if (pctx->verbose) {
    fprintf(stderr, "%s: hello\n", me);
  }
  enrLast = _pullEnergyAverage(pctx);
  fprintf(stderr, "!%s: starting system energy = %g\n", me, enrLast);
  time0 = airTime();
  firstIter = pctx->iter;
  enrImprovAvg = 0;
  do {
    if (pctx->snap && !(pctx->iter % pctx->snap)) {
      npos = nrrdNew();
      sprintf(poutS, "snap.%06d.pos.nrrd", pctx->iter);
      if (pullOutputGet(npos, NULL, NULL, nrrdTypeDouble, AIR_TRUE, pctx)) {
        sprintf(err, "%s: couldn't get snapshot for iter %d", me, pctx->iter);
        biffAdd(PULL, err); return 1;
      }
      if (nrrdSave(poutS, npos, NULL)) {
        sprintf(err, "%s: couldn't save snapshot for iter %d", me, pctx->iter);
        biffMove(PULL, err, NRRD); return 1;
      }
      npos = nrrdNuke(npos);
    }

    if (_pullIterate(pctx)) {
      sprintf(err, "%s: trouble on iter %d", me, pctx->iter);
      biffAdd(PULL, err); return 1;
    }
    pctx->iter += 1;
    enrNew = _pullEnergyAverage(pctx);
    if (firstIter + 1 == pctx->iter) {
      enrImprovAvg = enrImprov = 1;
    } else {
      enrImprov = _PULL_IMPROV(enrLast, enrNew);
      enrImprovAvg = (enrImprovAvg + enrImprov)/2;
    }
    if (pctx->verbose > 1) {
      fprintf(stderr, "%s: iter %u: e=%g,%g, de=%g,%g, s=%g,%g\n",
              me, pctx->iter, enrLast, enrNew, enrImprov, enrImprovAvg,
              _pullStepInterAverage(pctx), _pullStepConstrAverage(pctx));
    }
    enrLast = enrNew;
    stopConverged = (enrImprovAvg < pctx->energyImprovMin);
    if (stopConverged && pctx->verbose) {
      fprintf(stderr, "%s: %g < %g: converged!!\n", me, 
              enrImprovAvg, pctx->energyImprovMin);
    }
    stopIter = (pctx->iter == pctx->maxIter);
    _pullPointStepScale(pctx, pctx->opporStepScale);
    /*
    if (50 == pctx->iter) {
      _pullPointStepScale(pctx, 100);
    }
    */
  } while (!( stopIter || stopConverged ));
  fprintf(stderr, "%s: done (%d,%d) at iter %u; enr = %g, enrImprov = %g,%g\n", 
          me, stopIter, stopConverged, 
          pctx->iter, enrNew, enrImprov, enrImprovAvg);
  time1 = airTime();

  pctx->timeRun += time1 - time0;
  pctx->energy = enrNew;

  return 0;
}
