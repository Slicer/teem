/*
  teem: Gordon Kindlmann's research software
  Copyright (C) 2003, 2002, 2001, 2000, 1999, 1998 University of Utah

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

/*
** learned: valgrind is sometimes far off when reporting the line
** number for an invalid read- have to comment out various lines in
** order to find the real offending line
*/

#include "alan.h"

int
_alanCheck(alanContext *actx) {
  char me[]="alanCheck", err[AIR_STRLEN_MED];

  if (!actx) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(ALAN, err); return 1;
  }
  if (0 == actx->dim) {
    sprintf(err, "%s: dimension of texture not set", me);
    biffAdd(ALAN, err); return 1;
  }
  if (alanTextureTypeUnknown == actx->textureType) {
    sprintf(err, "%s: texture type not set", me);
    biffAdd(ALAN, err); return 1;
  }
  if (!( actx->size[0] > 0 && actx->size[1] > 0
	 && (2 == actx->dim || actx->size[2] > 0) )) {
    sprintf(err, "%s: texture sizes invalid", me);
    biffAdd(ALAN, err); return 1;
  }
  if (0 == actx->speed) {
    sprintf(err, "%s: speed == 0", me);
    biffAdd(ALAN, err); return 1;
  }

  return 0;
}

int
alanUpdate(alanContext *actx) {
  char me[]="alanUpdate", err[AIR_STRLEN_MED];
  int ret;

  if (_alanCheck(actx)) {
    sprintf(err, "%s: ", me);
    biffAdd(ALAN, err); return 1;
  }
  if (actx->nlev[0] || actx->nlev[0]) {
    sprintf(err, "%s: confusion: nlev[0,1] already allocated?", me);
    biffAdd(ALAN, err); return 1;
  }
  actx->nlev[0] = nrrdNew();
  actx->nlev[1] = nrrdNew();
  actx->nparm = nrrdNew();
  if (2 == actx->dim) {
    ret = (nrrdMaybeAlloc(actx->nlev[0], alan_nt, 3,
			  2, actx->size[0], actx->size[1])
	   || nrrdCopy(actx->nlev[1], actx->nlev[0])
	   || nrrdMaybeAlloc(actx->nparm, alan_nt, 3,
			     3, actx->size[0], actx->size[1]));
  } else {
    ret = (nrrdMaybeAlloc(actx->nlev[0], alan_nt, 4,
			  2, actx->size[0], actx->size[1], actx->size[2])
	   || nrrdCopy(actx->nlev[1], actx->nlev[0])
	   || nrrdMaybeAlloc(actx->nparm, alan_nt, 4,
			     3, actx->size[0], actx->size[1], actx->size[2]));
  }
  if (ret) {
    sprintf(err, "%s: trouble allocating buffers", me);
    biffMove(ALAN, err, NRRD); return 1;
  }
  
  return 0;
}

int 
alanInit(alanContext *actx, const Nrrd *nlevInit, const Nrrd *nparmInit) {
  char me[]="alanInit", err[AIR_STRLEN_MED];
  alan_t *levInit=NULL, *lev0, *parmInit=NULL, *parm;
  size_t I, N;

  if (_alanCheck(actx)) {
    sprintf(err, "%s: ", me);
    biffAdd(ALAN, err); return 1;
  }
  if (!( actx->nlev[0] && actx->nlev[0] && actx->nparm )) {
    sprintf(err, "%s: nlev[0,1] not allocated: call alanUpdate", me);
    biffAdd(ALAN, err); return 1;
  }
  
  if (nlevInit) {
    if (nrrdCheck(nlevInit)) {
      sprintf(err, "%s: given nlevInit has problems", me);
      biffMove(ALAN, err, NRRD); return 1;
    }
    if (!( alan_nt == nlevInit->type 
	   && nlevInit->dim == 1 + actx->dim
	   && actx->nlev[0]->axis[0].size == nlevInit->axis[0].size
	   && actx->size[0] == nlevInit->axis[1].size
	   && actx->size[1] == nlevInit->axis[2].size 
	   && (2 == actx->dim || actx->size[2] == nlevInit->axis[3].size) )) {
      sprintf(err, "%s: type/size mismatch with given nlevInit", me);
      biffAdd(ALAN, err); return 1;
    }
    levInit = (alan_t*)(nlevInit->data);
  }
  if (nparmInit) {
    if (nrrdCheck(nparmInit)) {
      sprintf(err, "%s: given nparmInit has problems", me);
      biffMove(ALAN, err, NRRD); return 1;
    }
    if (!( alan_nt == nparmInit->type 
	   && nparmInit->dim == 1 + actx->dim
	   && 3 == nparmInit->axis[0].size
	   && actx->size[0] == nparmInit->axis[1].size
	   && actx->size[1] == nparmInit->axis[2].size 
	   && (2 == actx->dim || actx->size[2] == nparmInit->axis[3].size) )) {
      sprintf(err, "%s: type/size mismatch with given nparmInit", me);
      biffAdd(ALAN, err); return 1;
    }
    parmInit = (alan_t*)(nparmInit->data);
  }

#define RAND AIR_AFFINE(0, airRand(), 1, -actx->randRange, actx->randRange)

  N = nrrdElementNumber(actx->nlev[0])/actx->nlev[0]->axis[0].size;
  lev0 = (alan_t*)(actx->nlev[0]->data);
  parm = (alan_t*)(actx->nparm->data);
  for (I=0; I<N; I++) {
    if (levInit) {
      lev0[0 + 2*I] = levInit[0 + 2*I];
      lev0[1 + 2*I] = levInit[1 + 2*I];
    } else {
      lev0[0 + 2*I] = actx->initA + RAND;
      lev0[1 + 2*I] = actx->initB + RAND;
    }
    if (parmInit) {
      parm[0 + 3*I] = parmInit[0 + 3*I];
      parm[1 + 3*I] = parmInit[1 + 3*I];
      parm[2 + 3*I] = parmInit[2 + 3*I];
    } else {
      parm[0 + 3*I] = actx->speed;
      parm[1 + 3*I] = actx->alpha;
      parm[2 + 3*I] = actx->beta;
    }
  }
  return 0;
}

/*
**  0 1 2
**  3 4 5
**  6 7 8
*/

int
_alanPerIteration(alanContext *actx, int iter) {
  char me[]="_alanPerIteration", fname[AIR_STRLEN_MED];
  Nrrd *nslc, *nimg;

  if (actx->saveInterval && !(iter % actx->saveInterval)) {
    fprintf(stderr, "%s: iter = %d, averageChange = %g\n",
	    me, iter, actx->averageChange);
    sprintf(fname, "%06d.nrrd", iter);
    nrrdSave(fname, actx->nlev[iter % 2], NULL);
  }
  if (actx->frameInterval && !(iter % actx->frameInterval)) {
    nrrdSlice(nslc=nrrdNew(), actx->nlev[iter % 2], 0, 0);
    nrrdQuantize(nimg=nrrdNew(), nslc, NULL, 8);
    sprintf(fname, "%06d.png", iter);
    nrrdSave(fname, nimg, NULL);
    nrrdNuke(nslc);
    nrrdNuke(nimg);
  }
  return 0;
}


typedef struct {
  /* these two are genuine input to each worker thread */
  alanContext *actx;  
  int idx;
  /* this is just a convenient place to put airThread (so that alanRun()
     doesn't have to make multiple arrays of per-thread items) */
  airThread thread;
  /* pointless: a pointer to this is passed to airThreadJoin */
  void *me;
} alanTask;

void *
_alanTuring2DWorker(void *_task) {
  alan_t *lev0, *lev1, *parm, speed, alpha, beta, A, B,
    *v[9], lapA, lapB, deltaA, deltaB, diffA, diffB, change;
  int iter, stop, startY, endY, idx, px, mx, py, my, sx, sy, x, y;
  alanTask *task;

  task = (alanTask *)_task;
  sx = task->actx->size[0];
  sy = task->actx->size[1];
  parm = (alan_t*)(task->actx->nparm->data);
  diffA = task->actx->diffA/(task->actx->H*task->actx->H);
  diffB = task->actx->diffB/(task->actx->H*task->actx->H);
  startY = task->idx*sy/task->actx->numThreads;
  endY = (task->idx+1)*sy/task->actx->numThreads;

  for (iter = 0; 
       alanStopNot == task->actx->stop && iter < task->actx->maxIteration; 
       iter++) {

    lev0 = (alan_t*)(task->actx->nlev[iter % 2]->data);
    lev1 = (alan_t*)(task->actx->nlev[(iter+1) % 2]->data);
    stop = alanStopNot;
    change = 0;
    for (y = startY;
	 alanStopNot == stop && y < endY; 
	 y++) {
      if (task->actx->wrap) {
	py = AIR_MOD(y+1, sy);
	my = AIR_MOD(y-1, sy);
      } else {
	py = AIR_MIN(y+1, sy-1);
	my = AIR_MAX(y-1, 0);
      }
      for (x = 0; 
	   alanStopNot == stop && x < sx; 
	   x++) {
	if (task->actx->wrap) {
	  px = AIR_MOD(x+1, sx);
	  mx = AIR_MOD(x-1, sx);
	} else {
	  px = AIR_MIN(x+1, sx-1);
	  mx = AIR_MAX(x-1, 0);
	}
	idx = x + sx*(y);
	A = lev0[0 + 2*idx];
	B = lev0[1 + 2*idx];
	speed = parm[0 + 3*idx];
	alpha = parm[1 + 3*idx];
	beta = parm[2 + 3*idx];
	v[0] = lev0 + 2*(mx + sx*(my));
	v[1] = lev0 + 2*( x + sx*(my));
	v[2] = lev0 + 2*(px + sx*(my));
	v[3] = lev0 + 2*(mx + sx*( y));
	v[5] = lev0 + 2*(px + sx*( y));
	v[6] = lev0 + 2*(mx + sx*(py));
	v[7] = lev0 + 2*( x + sx*(py));
	v[8] = lev0 + 2*(px + sx*(py));
	lapA = v[1][0] + v[3][0] + v[5][0] + v[7][0] - 4*A;
	lapB = v[1][1] + v[3][1] + v[5][1] + v[7][1] - 4*B;
	
	deltaA = task->actx->K*(alpha - A*B) + diffA*lapA;
	if (AIR_ABS(deltaA) > task->actx->maxPixelChange) {
	  stop = alanStopDiverged;
	  break;
	}
	change += AIR_ABS(deltaA);
	deltaB = task->actx->K*(A*B - B - beta) + diffB*lapB;
	if (!( AIR_EXISTS(deltaA) && AIR_EXISTS(deltaB) )) {
	  stop = alanStopNonExist;
	  break;
	}
	
	A += speed*deltaA;
	B += speed*deltaB;  
	B = AIR_MAX(0, B);
	lev1[0 + 2*idx] = A;
	lev1[1 + 2*idx] = B; 
      }
    }

    /* add change to global sum in a threadsafe way */
    airThreadMutexLock(&(task->actx->changeMutex));
    task->actx->averageChange += change*(endY - startY)/(sx*sy);
    task->actx->changeCount += 1;
    if (task->actx->changeCount == task->actx->numThreads) {
      /* I must be the last thread to reach this point; all 
	 others must have passed the mutex unlock, and are
	 sitting at the barrier */
      if (task->actx->averageChange < task->actx->minAverageChange) {
	/* we converged */
	task->actx->stop = alanStopConverged;
      } else if (alanStopNot != stop) {
	/* we stopped for some reason */
	task->actx->stop = stop;
      } else {
	/* we keep going */
	_alanPerIteration(task->actx, iter);
	if (task->actx->perIteration) {
	  task->actx->perIteration(task->actx, iter);
	}
      }
      task->actx->averageChange = 0;
      task->actx->changeCount = 0;
    }
    airThreadMutexUnlock(&(task->actx->changeMutex));

    /* force all threads to line up here, once per iteration */
    airThreadBarrierWait(&(task->actx->iterBarrier));
  }
  
  if (iter == task->actx->maxIteration) {
    /* HEY: all threads will agree on this, right? */
    task->actx->stop = alanStopMaxIteration;
  }
  /* else: we bailed because of !(task->actx->keepWorking); some thread
     should have set task->actx->stop as part of killing things */
  return _task;
}

int
alanRun(alanContext *actx) {
  char me[]="alanRun", err[AIR_STRLEN_MED];
  int tid;
  alanTask task[ALAN_THREAD_MAX];

  if (_alanCheck(actx)) {
    sprintf(err, "%s: ", me);
    biffAdd(ALAN, err); return 1;
  }
  if (!( actx->nlev[0] && actx->nlev[0] )) {
    sprintf(err, "%s: nlev[0,1] not allocated: "
	    "call alanUpdate + alanInit", me);
    biffAdd(ALAN, err); return 1;
  }
  

  airThreadMutexInit(&(actx->changeMutex));
  airThreadBarrierInit(&(actx->iterBarrier), actx->numThreads);
  task->actx->averageChange = 0;
  task->actx->changeCount = 0;
  for (tid=0; tid<actx->numThreads; tid++) {
    task[tid].actx = actx;
    task[tid].idx = tid;
    airThreadCreate(&(task[tid].thread), _alanTuring2DWorker,
		    (void *)&(task[tid]));
  }
  for (tid=0; tid<actx->numThreads; tid++) {
    airThreadJoin(&(task[tid].thread), &(task[tid].me));
  }
  airThreadBarrierDone(&(actx->iterBarrier));
  airThreadMutexDone(&(actx->changeMutex));

  /* we assume that someone set actx->stop */
  return 0;
}
