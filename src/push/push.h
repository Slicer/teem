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

#ifndef PUSH_HAS_BEEN_INCLUDED
#define PUSH_HAS_BEEN_INCLUDED

#include <math.h>

#include <teem/air.h>
#include <teem/biff.h>
#include <teem/ell.h>
#include <teem/nrrd.h>
#include <teem/gage.h>
#include <teem/ten.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PUSH pushBiffKey

#define PUSH_STAGE_MAX 4
#define PUSH_STAGE_PARM_MAX 1
#define PUSH_THREAD_MAX 512

#ifdef TEEM_PUSH_TYPE_DOUBLE
typedef double push_t;
#define push_nrrdType nrrdTypeDouble
#define PUSH_TYPE_FLOAT 0
#else
typedef float push_t;
#define push_nrrdType nrrdTypeFloat
#define PUSH_TYPE_FLOAT 1
#endif

typedef struct pushTask_t {
  struct pushContext_t *pctx;      /* parent's context */
  gageContext *gctx;               /* result of gageContextCopy(pctx->gctx) */
  gage_t *tenAns;                  /* result of gage probing */
  airThread *thread;               /* my thread */
  int threadIdx;                   /* which thread am I */
  double sumVel;                   /* sum of velocities for points in my batches */
  void *returnPtr;                 /* for airThreadJoin */
} pushTask;

typedef void (*pushProcess)(struct pushTask_t *task, int batch,
                            double parm[PUSH_STAGE_PARM_MAX]);

typedef struct pushContext_t {
  /* INPUT ----------------------------- */
  Nrrd *nin;                       /* image of 2D or 3D masked tensors */
  double drag,                     /* magnitude of viscosity term */
    step,                          /* time step in integration */
    minMeanVel;                    /* stop if mean velocity drops below this */
  int pointsPerBatch,              /* number points per "batch" */
    numBatch,                      /* total number of batches in simulation */
    numThread,                     /* number of threads to use */
    numStage,                      /* number of stages */
    maxIter,                       /* if non-zero, max number of iterations */
    snap,                          /* if non-zero, interval between iterations
                                      at which output snapshots are saved */
    verbose;                       /* blah blah blah */
  const NrrdKernel *kernel;
  double kparm[NRRD_KERNEL_PARMS_NUM];
  double stageParm[PUSH_STAGE_MAX][PUSH_STAGE_PARM_MAX]; /* parms for stages */
  pushProcess process[PUSH_STAGE_MAX]; /* the function for each stage */
  /* INTERNAL -------------------------- */
  Nrrd *nten,                      /* always 3D masked tensors */
    *nPosVel,
    *nVelAcc;
  gageContext *gctx;               /* gage context around nten */
  gage_t *tenAns;                  /* result of gage probing */
  int dimIn,                       /* dimension (2 or 3) of input */
    finished,                      /* used to signal all threads to return */
    stage,                         /* stage currently undergoing processing */
    batch;                         /* *next* batch of points needing to be
                                      processed, either in filter or in update
                                      stage.  Stage is done when
                                      batch == numBatch */
  double minPos[3],                /* lower corner of world position */
    maxPos[3],                     /* upper corner of world position */
    meanVel,                       /* latest mean velocity of particles */
    time0, time1;                  /* time at start and stop of run */
  pushTask **task;                 /* dynamically allocated array of tasks */
  airThreadMutex *batchMutex;      /* mutex around batch (and stage) */
  airThreadBarrier *stageBarrierA, /* barriers between stages */
    *stageBarrierB;
  /* OUTPUT ---------------------------- */
  double time;                     /* how long it took to run */
  Nrrd *noutPos,                   /* list of 2D or 3D positions */
    *noutTen;                      /* list of 2D or 3D masked tensors */
} pushContext;

/* defaultsPush.c */
TEEM_API const char *pushBiffKey;

/* methodsPush.c */
TEEM_API pushContext *pushContextNew();
TEEM_API pushContext *pushContextNix(pushContext *pctx);

/* corePush.c */
TEEM_API int pushStart(pushContext *pctx);
TEEM_API int pushIterate(pushContext *pctx);
TEEM_API int pushFinish(pushContext *pctx);

/* action.c */
TEEM_API int pushOutputGet(Nrrd *nPosOut, Nrrd *nTenOut, pushContext *pctx);
TEEM_API int pushRun(pushContext *pctx);

#ifdef __cplusplus
}
#endif

#endif /* PUSH_HAS_BEEN_INCLUDED */

