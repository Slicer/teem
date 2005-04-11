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

/*
**  0: pos = position,                         0 + 3 =
**  3: vel = velocity,                         3 + 3 =
**  6: ten = tensor,                           6 + 7 =
** 13: cnt = containment by gradient of mask  13 + 3 = 16.
*/
#define PUSH_POS  0
#define PUSH_VEL  3
#define PUSH_TEN  6
#define PUSH_CNT 13
#define PUSH_ATTR_LEN 16

/* increment for airArrays in bins */
#define PUSH_PIDX_INCR 32

typedef struct pushTask_t {
  struct pushContext_t *pctx;      /* parent's context */
  gageContext *gctx;               /* result of gageContextCopy(pctx->gctx) */
  gage_t *tenAns, *cntAns;         /* results of gage probing */
  airThread *thread;               /* my thread */
  int threadIdx;                   /* which thread am I */
  double sumVel;                   /* sum of velocities of pts in my batches */
  void *returnPtr;                 /* for airThreadJoin */
} pushTask;

typedef void (*pushProcess)(struct pushTask_t *task, int batch,
                            double parm[PUSH_STAGE_PARM_MAX]);

typedef struct pushContext_t {
  /* INPUT ----------------------------- */
  Nrrd *nin,                       /* image of 2D or 3D masked tensors */
    *npos;                         /* positions to start with
                                      (overrides numPoint) */
  double drag,                     /* to slow fast things down */
    preDrag,                       /* different drag pre-min-iter */
    step,                          /* time step in integration */
    mass,                          /* mass of particles */
    scale,                         /* scaling from tensor to glyph size */
    pull,                          /* region in which there is attraction */
    stiff,                         /* spring constant on glyph surface */
    preStiff,                      /* different stiff pre-min-iter */
    nudge,                         /* scaling of nudging towards center */
    wall,                          /* spring constant of walls */
    margin,                        /* space allowed around [-1,1]^3 for pnts */
    minMeanVel;                    /* stop if mean velocity drops below this */
  int seed,                        /* seed value for airSrand48 */
    numPoint,                      /* number of points to simulate */
    numThread,                     /* number of threads to use */
    numStage,                      /* number of stages */
    minIter,                       /* if non-zero, min number of iterations */
    maxIter,                       /* if non-zero, max number of iterations */
    snap,                          /* if non-zero, interval between iterations
                                      at which output snapshots are saved */
    singleBin,                     /* disable binning (for debugging) */
    driftCorrect,                  /* prevent sliding near anisotropy edges */
    verbose;                       /* blah blah blah */
  NrrdKernelSpec *ksp00,           /* for sampling tensor field */
    *ksp11;                        /* for gradient of mask */
  double stageParm[PUSH_STAGE_MAX][PUSH_STAGE_PARM_MAX]; /* parms for stages */
  pushProcess process[PUSH_STAGE_MAX]; /* the function for each stage */
  /* INTERNAL -------------------------- */
  Nrrd *nten,                      /* 3D image of 3D masked tensors */
    *nmask,                        /* mask image from nten */
    *nPointAttr,                   /* 1-vector of point attributes */
    *nVelAcc;                      /* for implementing integration */
  gageContext *gctx;               /* gage context around nten */
  gage_t *tenAns,                  /* gage probe to learn tensor */
    *cntAns;                       /* gage probe to learn containment vector */
  int dimIn,                       /* dimension (2 or 3) of input */
    binsEdge,                      /* # bins along edge of grid */
    numBin,                        /* total # bins in grid */
    finished,                      /* used to signal all threads to return */
    stage,                         /* stage currently undergoing processing */
    bin;                           /* *next* bin of points needing to be
                                      processed.  Stage is done when
                                      bin == numBin */
  int **pidx;                      /* image/volume of point index arrays */
  airArray **pidxArr;              /* all airArrays around pidx[] */
  double maxEval,                  /* maximum eigenvalue in input field */
    minPos[3],                     /* lower corner of world position */
    maxPos[3],                     /* upper corner of world position */
    meanVel,                       /* latest mean velocity of particles */
    time0, time1;                  /* time at start and stop of run */
  pushTask **task;                 /* dynamically allocated array of tasks */
  airThreadMutex *binMutex;        /* mutex around bin */
  airThreadBarrier *stageBarrierA, /* barriers between stages */
    *stageBarrierB;
  /* OUTPUT ---------------------------- */
  double time;                     /* how long it took to run */
  int iter;                        /* how many iterations were needed */
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

