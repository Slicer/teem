/*
  Teem: Tools to process and visualize scientific data and images
  Copyright (C) 2007, 2006, 2005  Gordon Kindlmann
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

#ifndef PULL_HAS_BEEN_INCLUDED
#define PULL_HAS_BEEN_INCLUDED

#include <teem/air.h>
#include <teem/biff.h>
#include <teem/ell.h>
#include <teem/nrrd.h>
#include <teem/gage.h>
#include <teem/ten.h>

#if defined(_WIN32) && !defined(__CYGWIN__) && !defined(TEEM_STATIC)
#  if defined(TEEM_BUILD) || defined(pull_EXPORTS) || defined(teem_EXPORTS)
#    define PULL_EXPORT extern __declspec(dllexport)
#  else
#    define PULL_EXPORT extern __declspec(dllimport)
#  endif
#else /* TEEM_STATIC || UNIX */
#  define PULL_EXPORT extern
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define PULL pullBiffKey
#define PULL_THREAD_MAXNUM 512
#define PULL_VOLUME_MAXNUM 4
#define PULL_POINT_NEIGH_INCR 16

/*
******** pullInfo enum
**
** all the things that might be learned via gage from some kind, that
** can be used to control particle dynamics
*/
enum {
  pullInfoUnknown,            /*  0 */
  pullInfoTensor,             /*  1: [7] tensor here */
  pullInfoTensorInverse,      /*  2: [7] inverse of tensor here */
  pullInfoHessian,            /*  3: [9] hessian used for force distortion */
  pullInfoInside,             /*  4: [1] containment scalar */
  pullInfoInsideGradient,     /*  5: [3] containment vector */
  pullInfoHeight,             /*  6: [1] for gravity and crease detection */
  pullInfoHeightGradient,     /*  7: [3] */
  pullInfoHeightHessian,      /*  8: [9] */
  pullInfoSeedThresh,         /*  9: [1] scalar for thresholding seeding */
  pullInfoTangent1,           /* 10: [3] first tangent to constraint surf */
  pullInfoTangent2,           /* 11: [3] second tangent to constraint surf */
  pullInfoTangentMode,        /* 12: [1] for morphing between co-dim 1 and 2 */
  pullInfoIsosurfaceValue,    /* 13: [1] for isosurface extraction */
  pullInfoIsosurfaceGradient, /* 14: [3] */
  pullInfoIsosurfaceHessian,  /* 15: [9] */
  pullInfoStrength,           /* 16: [1] */
  pullInfoLast
};
#define PULL_INFO_MAX            16

/* 
** how the gageItem for a pullInfo is specified
*/
typedef struct pullInfoSpec_t {
  /* ------ INPUT ------ */
  int info;                     /* from the pullInfo* enum */
  char *volName;                /* volume name */
  char *itemName;               /* item name (kind is known by volume) */
  double scaling,               /* scaling factor (including sign) */
    zero;                       /* for height and inside: where is zero,
                                   for seedThresh, threshold value */
  /* ------ INTERNAL ------ */
  unsigned int volIdx;          /* which volume */
  int item;                     /* which item */
} pullInfoSpec;

/*
******** pullPoint
**
*/
typedef struct pullPoint_t {
  unsigned int idtag;         /* unique point ID */
  struct pullPoint_t **neigh; /* list of neighboring points */
  unsigned int neighNum;
  airArray *neighArr;
  double pos[4],              /* position in space and scale */
    energy,                   /* energy accumulator for this iteration */
    force[4];                 /* force accumulator for this iteration */
  double info[1];             /* actually, sneakily, allocated for *more*,
                                 depending on pullInfo needs, so has to be
                                 last field */
} pullPoint;

/*
******** pullBin
**
** the data structure for doing spatial binning.
*/
typedef struct pullBin_t {
  pullPoint **point;         /* dyn. alloc. array of point pointers */
  unsigned int pointNum;     /* # of points in this bin */
  airArray *pointArr;        /* airArray around point and pointNum */
  struct pullBin_t **neigh;  /* pre-computed NULL-terminated list of all
                                neighboring bins, including myself */
} pullBin;

/*
******** pullEnergyType* enum
**
** the different shapes of potential energy profiles that can be used
*/
enum {
  pullEnergyTypeUnknown,       /* 0 */
  pullEnergyTypeSpring,        /* 1 */
  pullEnergyTypeGauss,         /* 2 */
  pullEnergyTypeCoulomb,       /* 3 */
  pullEnergyTypeCotan,         /* 4 */
  pullEnergyTypeZero,          /* 5 */
  pullEnergyTypeLast
};
#define PULL_ENERGY_TYPE_MAX      5
#define PULL_ENERGY_PARM_NUM 3

/*
******** pullEnergy
**
** the functions which determine inter-point forces
**
** NOTE: the eval() function probably does NOT check to see it was passed
** non-NULL pointers into which to store energy and force
*/
typedef struct {
  char name[AIR_STRLEN_SMALL];
  unsigned int parmNum;
  void (*eval)(double *energy, double *force,
               double dist, const double parm[PULL_ENERGY_PARM_NUM]);
  double (*support)(const double parm[PULL_ENERGY_PARM_NUM]);
} pullEnergy;

typedef struct {
  const pullEnergy *energy;
  double parm[PULL_ENERGY_PARM_NUM];
} pullEnergySpec;

/*
** In the interests of simplicity (rather than avoiding redundancy), 
** this is going to copied per-task, which is why it contains the gageContext
** The idea is that the first of these is somehow set up by the user
** or something, and the rest of them are created within pull per-task.
*/
typedef struct {
  char *name;                  /* how the volume will be identified
                                  (like its a variable name) */
  const gageKind *kind;
  const Nrrd *ninSingle;       /* don't own */
  const Nrrd *const *ninScale; /* don't own;
                                  NOTE: only one of ninSingle and ninScale
                                  can be non-NULL */
  unsigned int scaleNum;       /* length of volScale[] */
  double scaleMin, scaleMax;
  NrrdKernelSpec *ksp00,       /* for sampling tensor field */
    *ksp11,                    /* for gradient of mask, other 1st derivs */
    *ksp22,                    /* for 2nd derivatives */
    *kspSS;                    /* for reconstructing from scale-space
                                  samples */
  gageContext *gctx;           /* do own, and set based on info here */
  gagePerVolume *gpvl;         /* stupid gage API ... */
} pullVolume;

/*
******** pullTask
**
** The information specific for a thread.  
*/
typedef struct pullTask_t {
  struct pullContext_t
    *pctx;                      /* parent's context; not const because the
                                   tasks assign themselves bins to do work */
  pullVolume
    *vol[PULL_VOLUME_MAXNUM];   /* volumes copied from parent */
  const double
    *ans[PULL_INFO_MAX+1];      /* answer *pointers* for all possible infos,
                                   pointing into per-task per-volume gctxs,
                                   or: NULL if that info is not being used */
  unsigned int 
    infoOffset[PULL_INFO_MAX+1];/* offset of answer within pullPoint->info */
  airThread *thread;            /* my thread */
  unsigned int threadIdx;       /* which thread am I */
  unsigned int
    pointNum;                   /* # points I let live this iteration */
  double energySum,             /* sum of energies of points I processed */
    deltaFracSum;               /* contribution to pctx->deltaFrac */
  airRandMTState *rng;          /* state for my RNG */
  void *returnPtr;              /* for airThreadJoin */
} pullTask;

/*
******** pullContext
**
** everything for doing one computation
**
** NOTE: right now there is no API for setting the input fields (as there
** is in gage and tenFiber) eventually there will be...
*/
typedef struct pullContext_t {
  /* INPUT ----------------------------- */
  int verbose;                     /* blah blah blah */
  unsigned int pointNum;           /* number points to start simulation w/ */
  Nrrd *npos;                      /* positions (4xN array) to start with
                                      (overrides pointNum) */
  pullVolume 
    *vol[PULL_VOLUME_MAXNUM];      /* the volumes we analyze (we DO OWN) */
  unsigned int volNum;             /* actual length of vol[] used */

  pullInfoSpec
    *ispec[PULL_INFO_MAX+1];       /* info ii is in effect if ispec[ii] is
                                      non-NULL (and we DO OWN ispec[ii]) */
  
  double stepInitial,              /* initial time step in integration 
                                      (which will be reduced as the system
                                      converges) */
    interScl,                      /* scaling on inter-particle interactions */
    wallScl;                       /* spring constant of walls */

  /* concerning the probability-based optimizations */
  double
    neighborTrueProb,              /* probability that we find the true
                                      neighbors of the particle, as opposed to
                                      using a cached list */
    probeProb;                     /* probability that we gageProbe() to find
                                      the real local values, instead of
                                      re-using last value */

  /* how the time-step is adaptively varied as the system converges */
  double 
    deltaLimit,                    /* speed limit on particles' motion, as a
                                      fraction of nominal radius along
                                      direction of motion */
    deltaFracMin,                  /* lowest value of deltaFrac (see below)
                                      that is allowed without decreasing 
                                      step size */
    energyStepFrac,                /* when energy goes up instead of down, the
                                      fraction by which to scale step size */
    deltaFracStepFrac,             /* when deltaFrac goes below deltaFracMin,
                                      fraction by which to scale step size */
    energyImprovMin;               /* convergence threshold: stop when
                                      fracional improvement (decrease) in
                                      energy dips below this */
  unsigned int seedRNG,            /* seed value for random number generator */
    threadNum,                     /* number of threads to use */
    maxIter,                       /* if non-zero, max number of iterations */
    snap;                          /* if non-zero, interval between iterations
                                      at which output snapshots are saved */
  
  pullEnergySpec *ensp;            /* potential energy function to use */
  
  int binSingle;                   /* disable binning (for debugging) */
  unsigned int binIncr;            /* increment for per-bin airArray */

  /* INTERNAL -------------------------- */

  double bboxMin[3], bboxMax[3];   /* bounding box of all volumes, the region
                                      over which the binning is defined */
  unsigned int infoTotalLen;       /* total length of the info buffers needed,
                                      which determines size of allocated
                                      binPoint */
  unsigned int idtagNext;          /* next per-point igtag value */
  int finished;                    /* used to signal all threads to return */

  pullBin *bin;                    /* volume of bins (see binsEdge, binNum) */
  unsigned int binsEdge[3],        /* # bins along each volume edge,
                                      determined by maxEval and scale */
    binNum,                        /* total # bins in grid */
    binIdx;                        /* *next* bin of points needing to be
                                      processed.  Stage is done when
                                      binIdx == binNum */
  airThreadMutex *binMutex;        /* mutex around bin, needed because bins
                                      are the unit of work for the tasks */

  double step,                     /* current working step size */
    maxDist,                       /* max distance btween interacting points */
    energySum;                     /* potential energy of entire system */
  pullTask **task;                 /* dynamically allocated array of tasks */
  airThreadBarrier *iterBarrierA;  /* barriers between iterations */
  airThreadBarrier *iterBarrierB;  /* barriers between iterations */
  double deltaFrac;                /* mean (over all particles in last 
                                      iteration) of fraction of distance 
                                      actually travelled to distance that it
                                      wanted to travel (due to speed limit) */

  /* OUTPUT ---------------------------- */

  double timeIteration,            /* time needed for last (single) iter */
    timeRun;                       /* total time spent in computation */
  unsigned int iter;               /* how many iterations were needed */
  Nrrd *noutPos;                   /* list of 4D positions */
} pullContext;

/* defaultsPull.c */
PULL_EXPORT const char *pullBiffKey;

/* energy.c */
PULL_EXPORT airEnum *pullEnergyType;
PULL_EXPORT const pullEnergy *const pullEnergyUnknown;
PULL_EXPORT const pullEnergy *const pullEnergySpring;
PULL_EXPORT const pullEnergy *const pullEnergyGauss;
PULL_EXPORT const pullEnergy *const pullEnergyCoulomb;
PULL_EXPORT const pullEnergy *const pullEnergyCotan;
PULL_EXPORT const pullEnergy *const pullEnergyZero;
PULL_EXPORT const pullEnergy *const pullEnergyAll[PULL_ENERGY_TYPE_MAX+1];
PULL_EXPORT pullEnergySpec *pullEnergySpecNew();
PULL_EXPORT void pullEnergySpecSet(pullEnergySpec *ensp,
                                   const pullEnergy *energy,
                                   const double parm[PULL_ENERGY_PARM_NUM]);
PULL_EXPORT pullEnergySpec *pullEnergySpecNix(pullEnergySpec *ensp);
PULL_EXPORT int pullEnergySpecParse(pullEnergySpec *ensp, const char *str);
PULL_EXPORT hestCB *pullHestEnergySpec;

/* volumePull.c */
PULL_EXPORT gageKind *pullGageKindParse(const char *str);
PULL_EXPORT pullVolume *pullVolumeNew();
PULL_EXPORT pullVolume *pullVolumeNix(pullVolume *vol);
PULL_EXPORT int pullVolumeSingleAdd(pullContext *pctx, pullVolume *vol,
                                    char *name, const Nrrd *nin,
                                    const gageKind *kind, 
                                    const NrrdKernelSpec *ksp00,
                                    const NrrdKernelSpec *ksp11,
                                    const NrrdKernelSpec *ksp22);
PULL_EXPORT int pullVolumeStackAdd(pullContext *pctx, pullVolume *vol,
                                   char *name, const Nrrd *const *nin,
                                   unsigned int ninNum,
                                   const gageKind *kind, 
                                   double scaleMin, double scaleMax,
                                   const NrrdKernelSpec *ksp00,
                                   const NrrdKernelSpec *ksp11,
                                   const NrrdKernelSpec *ksp22,
                                   const NrrdKernelSpec *kspSS);

/* infoPull.c */
PULL_EXPORT airEnum *const pullInfo;
PULL_EXPORT unsigned int pullInfoAnswerLen(int info);
PULL_EXPORT pullInfoSpec *pullInfoSpecNew();
PULL_EXPORT pullInfoSpec *pullInfoSpecNix(pullInfoSpec *ispec);
PULL_EXPORT int pullInfoSpecAdd(pullContext *pctx, pullInfoSpec *ispec,
                                int info, const char *volName,
                                const char *itemName);

/* contextPull.c */
PULL_EXPORT pullContext *pullContextNew(void);
PULL_EXPORT pullContext *pullContextNix(pullContext *pctx);
PULL_EXPORT int pullOutputGet(Nrrd *nPos, Nrrd *nEnr, pullContext *pctx);

/* pointPull.c */
PULL_EXPORT pullPoint *pullPointNew(pullContext *pctx);
PULL_EXPORT pullPoint *pullPointNix(pullPoint *pnt);

/* binningPull.c */
PULL_EXPORT void pullBinInit(pullBin *bin, unsigned int incr);
PULL_EXPORT void pullBinDone(pullBin *bin);
PULL_EXPORT int pullBinPointAdd(pullContext *pctx, pullPoint *point);
PULL_EXPORT void pullBinAllNeighborSet(pullContext *pctx);
PULL_EXPORT int pullBinPointAdd(pullContext *pctx, pullPoint *point);
PULL_EXPORT int pullRebin(pullContext *pctx);

/* actionPull.c */
PULL_EXPORT int pullBinProcess(pullTask *task, unsigned int myBinIdx);

/* corePull.c */
PULL_EXPORT int pullStart(pullContext *pctx);
PULL_EXPORT int pullIterate(pullContext *pctx);
PULL_EXPORT int pullRun(pullContext *pctx);
PULL_EXPORT int pullFinish(pullContext *pctx);

#ifdef __cplusplus
}
#endif

#endif /* PULL_HAS_BEEN_INCLUDED */

