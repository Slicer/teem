/*
  teem: Gordon Kindlmann's research software
  Copyright (C) 2004, 2003, 2002, 2001, 2000, 1999, 1998 University of Utah

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

#include "coil.h"

/*
** 
** iv3 is: diam x diam x diam x valLen
*/
void
_coilIv3Fill(coil_t *iv3, coil_t *here, int radius, int valLen,
	     int x0, int y0, int z0, int sizeX, int sizeY, int sizeZ) {
  int diam, vi,    /* value index */
    xni, yni, zni, /* neighborhood (iv3) indices */
    xvi, yvi, zvi; /* volume indices */
  
  /* this should be parameterized on both radius and valLen */
  /* this should shuffle values within iv3 when possible */
  diam = 1 + 2*radius;
  for (zni=0; zni<diam; zni++) {
    zvi = AIR_CLAMP(0, zni-radius+z0, sizeZ-1) - z0;
    for (yni=0; yni<diam; yni++) {
      yvi = AIR_CLAMP(0, yni-radius+y0, sizeY-1) - y0;
      for (xni=0; xni<diam; xni++) {
	xvi = AIR_CLAMP(0, xni-radius+x0, sizeX-1) - x0;
	for (vi=0; vi<valLen; vi++) {
	  iv3[xni + diam*(yni + diam*(zni + diam*vi))] = 
	    here[vi + valLen*(0 + 2*(xvi + sizeX*(yvi + sizeY*zvi)))];
	}
      }
    }
  }
  return;
}

void
_coilProcess(coilTask *task, int doFilter) {
  int xi, yi, zi, sizeX, sizeY, sizeZ, valLen, radius;
  coil_t *here;
  void (*filter)(coil_t *delta, coil_t *iv3, 
		 double spacing[3],
		 double parm[COIL_PARMS_NUM]);
  
  sizeX = task->cctx->size[0];
  sizeY = task->cctx->size[1];
  sizeZ = task->cctx->size[2];
  valLen = task->cctx->kind->valLen;
  radius = task->cctx->radius;
  filter = task->cctx->kind->filter[task->cctx->method->type];
  here = (coil_t*)(task->cctx->nvol->data);
  here += 2*valLen*sizeX*sizeY*task->startZ;
  if (doFilter) {
    for (zi=task->startZ; zi<=task->endZ; zi++) {
      for (yi=0; yi<sizeY; yi++) {
	for (xi=0; xi<sizeX; xi++) {
	  _coilIv3Fill(task->iv3, here + 0*valLen, radius, valLen,
		       xi, yi, zi, sizeX, sizeY, sizeZ);
	  filter(here + 1*valLen, task->iv3, task->cctx->spacing, task->cctx->parm);
	  here += 2*valLen;
	}
      }
    }
  } else {
    for (zi=task->startZ; zi<=task->endZ; zi++) {
      for (yi=0; yi<sizeY; yi++) {
	for (xi=0; xi<sizeX; xi++) {
	  task->cctx->kind->update(here + 0*valLen, here + 1*valLen);
	  here += 2*valLen;
	}
      }
    }
  }
  return;
}

coilTask *
_coilTaskNew(coilContext *cctx, int threadIdx, int sizeZ) {
  coilTask *task;
  int len, diam;

  len = cctx->kind->valLen;
  diam = 1 + 2*cctx->radius;
  task = (coilTask *)calloc(1, sizeof(coilTask));
  if (task) {
    task->cctx = cctx;
    task->thread = airThreadNew();
    task->startZ = threadIdx*sizeZ/cctx->numThreads;
    task->endZ = (threadIdx+1)*sizeZ/cctx->numThreads - 1;
    task->threadIdx = threadIdx;
    task->iv3 = (coil_t*)calloc(len*diam*diam*diam, sizeof(coil_t));
    task->returnPtr = NULL;
  }
  return task;
}

coilTask *
_coilTaskNix(coilTask *task) {

  if (task) {
    task->thread = airThreadNix(task->thread);
    task->iv3 = airFree(task->iv3);
    free(task);
  }
  return NULL;
}

void *
_coilWorker(void *_task) {
  char me[]="_coilWorker";
  coilTask *task;

  task = (coilTask *)_task;

  while (1) {
    /* wait until parent has set cctx->finished */
    if (task->cctx->verbose) {
      fprintf(stderr, "%s(%d): waiting to check finished\n",
	      me, task->threadIdx);
    }
    airThreadBarrierWait(task->cctx->finishBarrier);
    if (task->cctx->finished) {
      if (task->cctx->verbose) {
	fprintf(stderr, "%s(%d): done!\n", me, task->threadIdx);
      }
      break;
    }
    /* else there's work */

    /* first: filter */
    if (task->cctx->verbose) {
      fprintf(stderr, "%s(%d): filtering ... \n",
	      me, task->threadIdx);
    }
    _coilProcess(task, AIR_TRUE);
    airThreadBarrierWait(task->cctx->filterBarrier);

    /* second: update */
    if (task->cctx->verbose) {
      fprintf(stderr, "%s(%d): updating ... \n",
	      me, task->threadIdx);
    }
    _coilProcess(task, AIR_FALSE);
    airThreadBarrierWait(task->cctx->updateBarrier);
  }

  return _task;
}

int
coilStart(coilContext *cctx) {
  char me[]="coilStart", err[AIR_STRLEN_MED];
  int tidx, elIdx, valIdx, valLen;
  coil_t (*lup)(const void*, size_t), *val;

  if (!cctx) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(COIL, err); return 1;
  }
  cctx->task = (coilTask **)calloc(cctx->numThreads, sizeof(coilTask *));
  if (!(cctx->task)) {
    sprintf(err, "%s: couldn't allocate array of tasks", me);
    biffAdd(COIL, err); return 1;
  }
  
  /* we create tasks for ALL threads, including me, thread 0 */
  cctx->task[0] = NULL;
  for (tidx=0; tidx<cctx->numThreads; tidx++) {
    cctx->task[tidx] = _coilTaskNew(cctx, tidx, cctx->size[2]);
    if (!(cctx->task[tidx])) {
      sprintf(err, "%s: couldn't allocate task %d", me, tidx);
      biffAdd(COIL, err); return 1;
    }
  }
  
  cctx->finished = AIR_FALSE;
  if (cctx->numThreads > 1) {
    cctx->finishBarrier = airThreadBarrierNew(cctx->numThreads);
    cctx->filterBarrier = airThreadBarrierNew(cctx->numThreads);
    cctx->updateBarrier = airThreadBarrierNew(cctx->numThreads);
  }

  /* start threads 1 and up running (they won't get far) */
  if (cctx->verbose) {
    fprintf(stderr, "%s: parent doing slices Z=[%d,%d]\n", me, 
	    cctx->task[0]->startZ, cctx->task[0]->endZ);
  }
  for (tidx=1; tidx<cctx->numThreads; tidx++) {
    if (cctx->verbose) {
      fprintf(stderr, "%s: spawning thread %d (Z=[%d,%d])\n", me, tidx,
	      cctx->task[tidx]->startZ, cctx->task[tidx]->endZ);
    }
    airThreadStart(cctx->task[tidx]->thread, _coilWorker,
		   (void *)(cctx->task[tidx]));
  }

  /* initialize the values in cctx->nvol */
  val = (coil_t*)(cctx->nvol->data);
  valLen = cctx->kind->valLen;
#if COIL_TYPE_FLOAT
  lup = nrrdFLookup[cctx->nin->type];
#else
  lup = nrrdDLookup[cctx->nin->type];
#endif
  for (elIdx=0; elIdx<cctx->size[0]*cctx->size[1]*cctx->size[2]; elIdx++) {
    for (valIdx=0; valIdx<valLen; valIdx++) {
      val[valIdx + 0*valLen] = lup(cctx->nin->data, valIdx + valLen*elIdx);
      val[valIdx + 1*valLen] = 0;
    }
    val += 2*valLen;
  }
  
  return 0;
}

int
_coilPause(int huge) {
  int i, j, num;
  
  num = 0;
  for (i=1; i<huge; i++) {
    for (j=0; j<huge; j++) {
      num *= i;
    }
  }
  return num;
}

/*
******** coilIterate
**
** (documentation)
**
** NB: this implements the body of thread 0
*/
int
coilIterate(coilContext *cctx, int numIterations) {
  char me[]="coilIterate", err[AIR_STRLEN_MED];
  int iter;

  if (!cctx) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(COIL, err); return 1;
  }
  
  for (iter=0; iter<numIterations; iter++) {
    if (cctx->verbose) {
      fprintf(stderr, "%s: starting iter %d\n", me, iter);
    }
    cctx->finished = AIR_FALSE;
    if (cctx->numThreads > 1) {
      airThreadBarrierWait(cctx->finishBarrier);
    }
    
    /* first: filter */
    if (cctx->verbose) {
      fprintf(stderr, "%s: filtering ... \n", me);
    }
    _coilProcess(cctx->task[0], AIR_TRUE);
    if (cctx->numThreads > 1) {
      airThreadBarrierWait(cctx->filterBarrier);
    }

    /* second: update */
    if (cctx->verbose) {
      fprintf(stderr, "%s: updating ... \n", me);
    }
    _coilProcess(cctx->task[0], AIR_FALSE);
    if (cctx->numThreads > 1) {
      airThreadBarrierWait(cctx->updateBarrier);
    }
  }
  return 0;
}

int
coilFinish(coilContext *cctx) {
  char me[]="coilFinish", err[AIR_STRLEN_MED];
  int tidx;

  if (!cctx) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(COIL, err); return 1;
  }

  if (cctx->verbose) {
    fprintf(stderr, "%s: finishing workers\n", me);
  }
  cctx->finished = AIR_TRUE;
  if (cctx->numThreads > 1) {
    airThreadBarrierWait(cctx->finishBarrier);
  }
  for (tidx=1; tidx<cctx->numThreads; tidx++) {
    airThreadJoin(cctx->task[tidx]->thread, &(cctx->task[tidx]->returnPtr));
    cctx->task[tidx]->thread = airThreadNix(cctx->task[tidx]->thread);
    cctx->task[tidx] = _coilTaskNix(cctx->task[tidx]);
  }
  cctx->task[0]->thread = airThreadNix(cctx->task[0]->thread);
  cctx->task[0] = _coilTaskNix(cctx->task[0]);
  cctx->task = airFree(cctx->task);

  return 0;
}
