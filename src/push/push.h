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

/*
** 0: push_t == float
** 1: push_t == double
*/
#if 0
typedef double push_t;
#define push_nrrdType nrrdTypeDouble
#define PUSH_TYPE_FLOAT 0
#else
typedef float push_t;
#define push_nrrdType nrrdTypeFloat
#define PUSH_TYPE_FLOAT 1
#endif

#define PUSH pushBiffKey
#define PUSH_STAGE_MAXNUM 4
#define PUSH_STAGE_PARM_MAXNUM 1
#define PUSH_FORCE_PARM_MAXNUM 3
#define PUSH_THREAD_MAXNUM 512

typedef struct pushPoint_t {
  push_t pos[3],               /* position in world space */
    vel[3],                    /* velocity */
    frc[3],                    /* force accumulator for current iteration */
    ten[7],                    /* tensor here */
    inv[7],                    /* inverse of tensor */
    cnt[3],                    /* mask's containment gradient */
    dir[3];                    /* for tractlets, unit direction through me */
} pushPoint;

typedef struct pushThing_t {
  short numPoint,              /* 1 for point, more for tractlets */
    seed;                      /* which of the following points is the seed */
  pushPoint *point;            /* dyn. alloc. array of points (not pointers) */
} pushThing;

typedef struct pushBin_t {
  int numThing;                /* # of things in this bin */
  pushThing **thing;           /* dyn. alloc. array of thing pointers */
  airArray *thingArr;          /* airArray around thingPtr and numThing */
  struct pushBin_t **neighbor; /* pre-computed NULL-terminated list of all
                                  neighboring bins, including myself */
} pushBin;

/* increment for airArrays */
#define PUSH_ARRAY_INCR 64

typedef struct pushTask_t {
  struct pushContext_t *pctx;  /* parent's context */
  gageContext *gctx;           /* result of gageContextCopy(pctx->gctx) */
  gage_t *tenAns, *cntAns;     /* results of gage probing */
  airThread *thread;           /* my thread */
  int threadIdx,               /* which thread am I */
    numThing;                  /* # things I let live this iteration */
  double sumVel;               /* sum of velocities of pts in my batches */
  void *returnPtr;             /* for airThreadJoin */
} pushTask;

typedef void (*pushProcess)(pushTask *task, int bin,
                            const push_t parm[PUSH_STAGE_PARM_MAXNUM]);

typedef struct {
  push_t (*func)(push_t haveDist, push_t restDist, push_t scale,
                 const push_t parm[PUSH_FORCE_PARM_MAXNUM]);
  push_t (*maxDist)(push_t maxEval, push_t scale,
                    const push_t parm[PUSH_FORCE_PARM_MAXNUM]);
  push_t parm[PUSH_FORCE_PARM_MAXNUM];
} pushForce;

typedef struct pushContext_t {
  /* INPUT ----------------------------- */
  Nrrd *nin,                       /* image of 2D or 3D masked tensors */
    *npos,                         /* positions to start with
                                      (overrides numPoint) */
    *nstn;                         /* start/nums for tractlets in npos */
  double drag,                     /* to slow fast things down */
    preDrag,                       /* different drag pre-min-iter */
    step,                          /* time step in integration */
    mass,                          /* mass of particles */
    scale,                         /* scaling from tensor to glyph size */
    nudge,                         /* scaling of nudging towards center */
    wall,                          /* spring constant of walls */
    margin,                        /* space allowed around [-1,1]^3 for pnts */
    minMeanVel;                    /* stop if mean velocity drops below this */
  int seed,                        /* seed value for airSrand48 */
    numThing,                      /* number things to start simulation w/ */
    numThread,                     /* number of threads to use */
    numStage,                      /* number of stages */
    minIter,                       /* if non-zero, min number of iterations */
    maxIter,                       /* if non-zero, max number of iterations */
    snap,                          /* if non-zero, interval between iterations
                                      at which output snapshots are saved */
    singleBin,                     /* disable binning (for debugging) */
    driftCorrect,                  /* prevent sliding near anisotropy edges */
    verbose;                       /* blah blah blah */
  pushForce *force;                /* force function to use */
  NrrdKernelSpec *ksp00,           /* for sampling tensor field */
    *ksp11;                        /* for gradient of mask */
  push_t stageParm[PUSH_STAGE_MAXNUM][PUSH_STAGE_PARM_MAXNUM];
  pushProcess process[PUSH_STAGE_MAXNUM]; /* the function for each stage */
  /* INTERNAL -------------------------- */
  Nrrd *nten,                      /* 3D image of 3D masked tensors */
    *nmask;                        /* mask image from nten */
  gageContext *gctx;               /* gage context around nten */
  int dimIn,                       /* dimension (2 or 3) of input */
    binsEdge,                      /* # bins along edge of grid */
    numBin,                        /* total # bins in grid */
    finished,                      /* used to signal all threads to return */
    stageIdx,                      /* stage currently undergoing processing */
    binIdx;                        /* *next* bin of points needing to be
                                      processed.  Stage is done when
                                      binIdx == numBin */
  pushBin **bin;                   /* volume of bins (see binsEdge, numBin) */
  double maxDist,                  /* max distance btween interacting points */
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

/* miscPush.c */
TEEM_API void pushTenInv(pushContext *pctx, push_t *inv, push_t *ten);

/* methodsPush.c */
TEEM_API pushThing *pushThingNew(int numPoint);
TEEM_API pushThing *pushThingNix(pushThing *thg);
TEEM_API pushBin *pushBinNew(void);
TEEM_API pushBin *pushBinNix(pushBin *bin);
TEEM_API pushContext *pushContextNew(void);
TEEM_API pushContext *pushContextNix(pushContext *pctx);

/* forces.c */
TEEM_API pushForce *pushForceParse(const char *str);
TEEM_API pushForce *pushForceNix(pushForce *force);
TEEM_API hestCB *pushHestForce;

/* binning.c */
TEEM_API int pushBinAdd(pushContext *pctx, pushThing *thing);
TEEM_API int pushRebin(pushContext *pctx);

/* corePush.c */
TEEM_API int pushStart(pushContext *pctx);
TEEM_API int pushIterate(pushContext *pctx);
TEEM_API int pushFinish(pushContext *pctx);

/* action.c */
TEEM_API int pushOutputGet(Nrrd *nPosOut, Nrrd *nTenOut, Nrrd *nStnOut, 
                           pushContext *pctx);
TEEM_API int pushRun(pushContext *pctx);

#ifdef __cplusplus
}
#endif

#endif /* PUSH_HAS_BEEN_INCLUDED */

