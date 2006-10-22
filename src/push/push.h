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

#ifndef PUSH_HAS_BEEN_INCLUDED
#define PUSH_HAS_BEEN_INCLUDED

#include <math.h>

#include <teem/air.h>
#include <teem/biff.h>
#include <teem/ell.h>
#include <teem/nrrd.h>
#include <teem/gage.h>
#include <teem/ten.h>

#if defined(_WIN32) && !defined(__CYGWIN__) && !defined(TEEM_STATIC)
#  if defined(TEEM_BUILD) || defined(push_EXPORTS) || defined(teem_EXPORTS)
#    define PUSH_EXPORT extern __declspec(dllexport)
#  else
#    define PUSH_EXPORT extern __declspec(dllimport)
#  endif
#else /* TEEM_STATIC || UNIX */
#  define PUSH_EXPORT extern
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define PUSH pushBiffKey
#define PUSH_THREAD_MAXNUM 512

/*
******** pointPoint
**
** information about a point in the simulation.  There are really two
** kinds of information here: "pos", "vel", and "frc" pertain to the simulation
** of point dynamics, while "ten", "aniso", "inv", and "cnt" are properties of
** the field sampled at the point.  "tan" and "nor" are only meaningful in
** tractlets.
**
** HEY: with the addition of grav, gravNot, seedThresh, GK wonders why so much
** information has to be stored per point, and not in the task ...
**
** NB: there is no constructor for this, nor does there really need to be
*/
typedef struct pushPoint_t {
  double pos[3],               /* position in world space */
    vel[3],                    /* velocity */
    frc[3],                    /* force accumulator for current iteration */
    ten[7],                    /* tensor here */
    aniso,                     /* value of Cl1 (Westin ISMRM '97) */
    inv[7],                    /* inverse of tensor */
    cnt[3],                    /* mask's containment gradient */
    tan[3],                    /* tangent: unit direction through me */
    nor[3],                    /* change in tangent, normalized */
    grav[3], gravNot[2][3],    /* gravity stuff */
    seedThresh;                /* seed thresh */
} pushPoint;

/*
******** pushThing struct
**
** Represents both single points, and tractlets, as follows:
**
** for single points: "point" tells the whole story of the point,
** but point.{tan,nor} is meaningless.  For the sake of easily computing all
** pair-wise point interactions between things, "vertNum" is 1, and
** "vert" points to "point".  "len" is 0.
**
** for tractlets: the "pos", "vel", "frc" fields of "point" summarize the
** the dynamics of the entire tractlet, while the field attributes
** ("ten", "inv", "cnt") pertain exactly to the seed point.  For example,
** a particular tensor anisotropy in "point.ten" may have resulted in this
** thing turning from a point into a tractlet, or vice versa.  "vertNum" is
** the number of tractlet vertices; "vert" is the array of them.  The
** only field of the vertex points that is not meaningful is "vel": the
** tractlet velocity is "point.vel"
*/
typedef struct pushThing_t {
  int ttaagg;
  pushPoint point;             /* information about single point, or a
                                  seed point, hard to say exactly */
  unsigned int vertNum;        /* 1 for single point, else length of vert[] */
  pushPoint *vert;             /* dyn. alloc. array of tractlet vertices
                                  (*not* pointers to pushPoints), or, just
                                  the address of "point" for single point */
  unsigned int seedIdx;        /* which of the vertices is the seed point */
  double len;                  /* 0 for point, else (world-space) length of
                                  tractlet */
} pushThing;

/*
******** pushBin
**
** the data structure for doing spatial binning.  This really serves two
** related purposes- to bin *all* the points in the simulation-- both 
** single points as well as vertices of tractlets-- and also to bin the
** all "things".  The binning of points is used for the force calculation
** stage, and the binning of things is used for the update stage.
**
** The bins are the *only* way to access the things in the simulation, so
** bins own the things they contain.  However, because things point to
** points, bins do not own the points they contain.
*/
typedef struct pushBin_t {
  unsigned int pointNum;       /* # of points in this bin */
  pushPoint **point;           /* dyn. alloc. array of point pointers */
  airArray *pointArr;          /* airArray around point and pointNum */
  unsigned int thingNum;       /* # of things in this bin */
  pushThing **thing;           /* dyn. alloc. array of thing pointers */
  airArray *thingArr;          /* airArray around thing and thingNum */
  struct pushBin_t **neighbor; /* pre-computed NULL-terminated list of all
                                  neighboring bins, including myself */
} pushBin;

/*
******** pushTask
**
** The information specific for a thread.  
*/
typedef struct pushTask_t {
  struct pushContext_t *pctx;  /* parent's context */
  gageContext *gctx;           /* result of gageContextCopy(pctx->gctx) */
  const double *tenAns, *invAns, *cntAns,
    *gravAns, *gravNotAns[2],  /* results of gage probing */
    *seedThreshAns;            /* seed threshold answer */
  tenFiberContext *fctx;       /* result of tenFiberContextCopy(pctx->fctx) */
  airThread *thread;           /* my thread */
  unsigned int threadIdx,      /* which thread am I */
    thingNum;                  /* # things I let live this iteration */
  double sumVel,               /* sum of velocities of things in my bins */
    *vertBuff;                 /* buffer for tractlet vertices */
  void *returnPtr;             /* for airThreadJoin */
} pushTask;

/*
******** pushEnergyType* enum
**
** the different shapes of potential energy profiles that can be used
*/
enum {
  pushEnergyTypeUnknown,       /* 0 */
  pushEnergyTypeSpring,        /* 1 */
  pushEnergyTypeGauss,         /* 2 */
  pushEnergyTypeCoulomb,       /* 3 */
  pushEnergyTypeCotan,         /* 4 */
  pushEnergyTypeZero,          /* 5 */
  pushEnergyTypeLast
};
#define PUSH_ENERGY_TYPE_MAX      5
#define PUSH_ENERGY_PARM_NUM 3

/*
******** pushEnergy
**
** the functions which determine inter-point forces
*/
typedef struct {
  char name[AIR_STRLEN_SMALL];
  unsigned int parmNum;
  double (*energy)(double dist, const double parm[PUSH_ENERGY_PARM_NUM]);
  double (*force)(double dist, const double parm[PUSH_ENERGY_PARM_NUM]);
  double (*support)(const double parm[PUSH_ENERGY_PARM_NUM]);
} pushEnergy;

typedef struct {
  const pushEnergy *energy;
  double parm[PUSH_ENERGY_PARM_NUM];
} pushEnergySpec;

/*
******** pushContext
**
** everything for doing one simulation computation
*/
typedef struct pushContext_t {
  /* INPUT ----------------------------- */
  Nrrd *nin,                       /* 3D image of 3D masked tensors, though
                                      it may only be a single slice */
    *npos,                         /* positions to start with
                                      (overrides thingNum) */
    *nstn;                         /* start/nums for tractlets in npos */
  double step,                     /* time step in integration */
    scale,                         /* scaling from tensor to glyph size */
    wall,                          /* spring constant of walls */
    cntScl,                        /* magnitude of containment gradient */
    bigTrace,                      /* a last minute hack */
    minMeanVel;                    /* stop if mean velocity drops below this */
  int detReject,                   /* determinant-based rejection at init */
    midPntSmp,                     /* sample midpoint btw part.s for physics */
    verbose;                       /* blah blah blah */
  unsigned int seedRNG,            /* seed value for random number generator */
    thingNum,                      /* number things to start simulation w/ */
    threadNum,                     /* number of threads to use */
    maxIter,                       /* if non-zero, max number of iterations */
    snap;                          /* if non-zero, interval between iterations
                                      at which output snapshots are saved */
  int gravItem,                    /* tenGage item (vector) for gravity */
    gravNotItem[2];                /* for constraining gravity */
  double gravScl;                  /* sign and magnitude of gravity's effect */

  int seedThreshItem,              /* item for constraining random seeding */
    seedThreshSign;                /* +1: need val > thresh; -1: opposite */
  double seedThresh;               /* threshold for seed constraint */

  const pushEnergySpec *ensp;      /* potential energy function to use */

  int tltUse,                      /* enable tractlets */
    tltFrenet;                     /* use Frenet frames for tractlet forces */
  unsigned int tltStepNum;         /* max # points on each tractlet half */
  double tltThresh,
    tltSoft, tltStep;              /* tractlet formation parameters */

  int binSingle;                   /* disable binning (for debugging) */
  unsigned int binIncr;            /* increment for per-bin thing airArray */

  NrrdKernelSpec *ksp00,           /* for sampling tensor field */
    *ksp11,                        /* for gradient of mask */
    *ksp22;                        /* 2nd deriv, probably for gravity */

  /* INTERNAL -------------------------- */

  Nrrd *nten,                      /* 3D image of 3D masked tensors */
    *ninv,                         /* pre-computed inverse of nten */
    *nmask;                        /* mask image from nten */
  gageContext *gctx;               /* gage context around nten, ninv, nmask */
  gagePerVolume *tpvl, *ipvl;      /* gage pervolumes around nten and ninv */
  tenFiberContext *fctx;           /* tenFiber context around nten */
  int finished;                    /* used to signal all threads to return */
  unsigned int dimIn,              /* dim (2 or 3) of input, meaning whether
                                      it was a single slice or a full volume */
    sliceAxis;                     /* got a single 3-D slice, which axis had
                                      only a single sample */

  pushBin *bin;                    /* volume of bins (see binsEdge, binNum) */
  unsigned int binsEdge[3],        /* # bins along each volume edge,
                                      determined by maxEval and scale */
    binNum,                        /* total # bins in grid */
    binIdx;                        /* *next* bin of points needing to be
                                      processed.  Stage is done when
                                      binIdx == binNum */
  airThreadMutex *binMutex;        /* mutex around bin */

  double maxDist,                  /* max distance btween interacting points */
    maxEval, meanEval,             /* max and mean principal eval in field */
    maxDet,
    meanVel;                       /* latest mean velocity of particles */
  pushTask **task;                 /* dynamically allocated array of tasks */
  airThreadBarrier *iterBarrierA;  /* barriers between iterations */
  airThreadBarrier *iterBarrierB;  /* barriers between iterations */

  /* OUTPUT ---------------------------- */

  double timeIteration,            /* time needed for last (single) iter */
    timeRun;                       /* total time spent in computation */
  unsigned int iter;               /* how many iterations were needed */
  Nrrd *noutPos,                   /* list of 2D or 3D positions */
    *noutTen;                      /* list of 2D or 3D masked tensors */
} pushContext;

/* defaultsPush.c */
PUSH_EXPORT const char *pushBiffKey;

/* methodsPush.c */
PUSH_EXPORT pushThing *pushThingNew(unsigned int vertNum);
PUSH_EXPORT pushThing *pushThingNix(pushThing *thg);
PUSH_EXPORT pushContext *pushContextNew(void);
PUSH_EXPORT pushContext *pushContextNix(pushContext *pctx);

/* forces.c (legacy name for info about (derivatives of) energy functions) */
PUSH_EXPORT airEnum *pushEnergyType;
PUSH_EXPORT const pushEnergy *const pushEnergyUnknown;
PUSH_EXPORT const pushEnergy *const pushEnergySpring;
PUSH_EXPORT const pushEnergy *const pushEnergyGauss;
PUSH_EXPORT const pushEnergy *const pushEnergyCoulomb;
PUSH_EXPORT const pushEnergy *const pushEnergyCotan;
PUSH_EXPORT const pushEnergy *const pushEnergyZero;
PUSH_EXPORT const pushEnergy *const pushEnergyAll[PUSH_ENERGY_TYPE_MAX+1];
PUSH_EXPORT pushEnergySpec *pushEnergySpecNew();
PUSH_EXPORT pushEnergySpec *pushEnergySpecNix(pushEnergySpec *ensp);
PUSH_EXPORT int pushEnergySpecParse(pushEnergySpec *ensp, const char *str);
PUSH_EXPORT hestCB *pushHestEnergySpec;

/* corePush.c */
PUSH_EXPORT int pushStart(pushContext *pctx);
PUSH_EXPORT int pushIterate(pushContext *pctx);
PUSH_EXPORT int pushRun(pushContext *pctx);
PUSH_EXPORT int pushFinish(pushContext *pctx);

/* binning.c */
PUSH_EXPORT void pushBinInit(pushBin *bin, unsigned int incr);
PUSH_EXPORT void pushBinDone(pushBin *bin);
PUSH_EXPORT int pushBinThingAdd(pushContext *pctx, pushThing *thing);
PUSH_EXPORT int pushBinPointAdd(pushContext *pctx, pushPoint *point);
PUSH_EXPORT void pushBinAllNeighborSet(pushContext *pctx);
PUSH_EXPORT int pushRebin(pushContext *pctx);

/* setup.c */
PUSH_EXPORT int pushTaskFiberReSetup(pushContext *pctx,
                                     double tltThresh,
                                     double tltSoft,
                                     double tltStep,
                                     unsigned int tltStepNum);

/* action.c */
PUSH_EXPORT int pushBinProcess(pushTask *task, unsigned int myBinIdx);
PUSH_EXPORT int pushOutputGet(Nrrd *nPosOut, Nrrd *nTenOut, Nrrd *nStnOut, 
                              pushContext *pctx);

#ifdef __cplusplus
}
#endif

#endif /* PUSH_HAS_BEEN_INCLUDED */

