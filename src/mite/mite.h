/*
  teem: Gordon Kindlmann's research software
  Copyright (C) 2002, 2001, 2000, 1999, 1998 University of Utah

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

#ifndef MITE_HAS_BEEN_INCLUDED
#define MITE_HAS_BEEN_INCLUDED

#include <air.h>
#include <biff.h>
#include <nrrd.h>
#include <gage.h>
#include <limn.h>
#include <hoover.h>

#if defined(WIN32) && !defined(TEEM_BUILD)
#define mite_export __declspec(dllimport)
#else
#define mite_export
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define MITE "mite"

enum {
  miteRangeUnknown = -1,
  miteRangeAlpha,        /* 0: "A", opacity */
  miteRangeRed,          /* 1: "R" */
  miteRangeGreen,        /* 2: "G" */
  miteRangeBlue,         /* 3: "B" */
  miteRangeEmissivity,   /* 4: "E" */
  miteRangeKa,           /* 5: "a", phong ambient term */
  miteRangeKd,           /* 6: "d", phong diffuse */
  miteRangeKs,           /* 7: "s", phong specular */
  miteRangeSP,           /* 8: "p", phong specular power */
  miteRangeLast
};
#define MITE_RANGE_NUM      9

/*
******** #define MITE_TXF_NUM
**
** This number constrains the sum of the dimensions of all the transfer
** functions used.  For instance, if MITE_TXF_NUM is 3, then three 1-D
** transfer functions can be used, or one 1-D and one 2-D, or just
** one 3-D transfer function.  It also serves to limit the dimension
** of any single transfer function.
*/
#define MITE_TXF_NUM 4

/*
******** miteTxf
**
** A transfer function (as nrrd) and information about it which
** facilitates accessing the information in it.  There are some
** constraints on how the transfer function is stored and used:
** 1) all transfer functions are lookup tables: there is no 
** interpolation other than nearest neighbor. 
** 2) regardless of the centerings of the axes of nxtf, the lookup table
** axes will be treated as though they were cell centered.
** 3) the logical dimension of the transfer function is always one less
** ntxf->dim, with axis 0 always for the range of the function, and axes
** 1 and onwards for the domain.  For instance, a univariate opacity map
** is 2D, with ntxf->axis[0].size == 1.
** 
** So, ntxf->dim-1 is the number of variables in the domain of the transfer
** function, and ntxf->axis[0].size is the number of variables in the range.
*/
typedef struct {
  Nrrd *ntxf;                    /* nrrd containing transfer function */
  int *rangeIdx,                 /* indices of the ntxf->axis[0].size
				    transfer function range quantities */
    domainIdx[MITE_TXF_NUM];     /* integer indentifiers for the ntxf->dim-1
				    transfer function domain variables */
} miteTxf;

/*
******** miteUser struct
**
** all the input parameters for mite specified by the user, as well as
** a mop for cleaning up memory used during rendering.  Currently,
** unlike gage, there is no API for setting these- you go in the
** struct and set them yourself. 
*/
typedef struct {
  Nrrd *nin,             /* volume being rendered */
    *ntxf[MITE_TXF_NUM]; /* nrrds containing transfer functions */
  int ntxfNum;           /* number of nrrds set in ntxf[] */
  /* for each possible element of the txf range, what value should it
     start at prior to multiplying by the values (if any) learned from
     the txf.  Mainly needed to store non-unity values for the
     quantities not covered by a transfer function */
  float rangeInit[MITE_RANGE_NUM]; 
  double refStep,        /* length of "unity" for doing opacity correction */
    rayStep,             /* distance between sampling planes */
    near1;               /* close enough to unity for the sake of doing early
			    ray termination when opacity gets high */
  hooverContext *hctx;   /* context and input for all hoover-related things,
			    including camera and image parameters */
  /* local initial copies of kernels, later passed to gageKernelSet */
  NrrdKernelSpec *ksp[GAGE_KERNEL_NUM];
  gageContext *gctx0;    /* context and input for all gage-related things,
			    including all kernels.  This is gageContextCopy'd
			    for multi-threaded use (hence the 0) */
  limnLight *lit;        /* a struct for all lighting info */
  int justSum,           /* don't use opacity: just sum colors */
    noDirLight;          /* forget directional phong lighting, using only
			    the ambient component */
  char *outS;            /* name of output file */
  airArray *mop;         /* mop for all resources allocated for rendering */
} miteUser;

struct miteThread_t;

/*
******** miteRender
**
** non-thread-specific state relevant for mite's internal use
*/
typedef struct {
  miteTxf *txf[MITE_TXF_NUM]; /* transfer functions */
  int txfNum;                 /* number of transfer functions */
  double time0, time1;        /* rendering start and end times */
  int sx, sy,                 /* image dimensions */
    totalSamples;             /* total # samples used for all rays */
  Nrrd *nout;                 /* output image nrrd */
  float *imgData;             /* output image data */

  /* as long as there's no mutex around how the miteThreads are
     airMopAdded to the miteUser's mop, these have to be allocated in
     mrendRenderBegin instead of mrendThreadBegin */
  struct miteThread_t *tt[HOOVER_THREAD_MAX];  
} miteRender;

typedef struct {
  gage_t *val;            /* the gage-measured txf axis variable */
  int size;               /* number of samples */
  float min, max,         /* min, max (copied from nrrd axis) */
    *data,                /* pointer to txf data.  If non-NULL, the
			     rest of the variables are meaningful */
    *range[MITE_TXF_NUM]; /* pointers to thread-specific rendering variables
			     that will be determined by the txf */
  int num;                /* number of range variables set by the txf
			     == number of pointers in range[] to use */
} miteStage;

/*
******** miteThread
**
** thread-specific state for mite's internal use
*/
typedef struct miteThread_t {
  miteStage stage[MITE_TXF_NUM]; /* all stages for txf computation */
  gageContext *gctx;     /* per-thread context */
  gage_t *ans,           /* shortcut to gctx->pvl[0]->ans */
    *norm;               /* shortcut to ans[...normal...] */
  int thrid,             /* thread ID */
    ui, vi;              /* image coords */
} miteThread;

/* defaultsMite.c */
extern mite_export double miteDefRefStep;
extern mite_export double miteDefNear1;

/* txf.c */
extern mite_export char miteRangeChar[MITE_RANGE_NUM];
extern miteTxf *miteTxfNew(Nrrd *ntxf);
extern miteTxf *miteTxfNix(miteTxf *txf);

/* user.c */
extern miteUser *miteUserNew();
extern miteUser *miteUserNix(miteUser *muu);

/* renderMite.c */
extern int miteRenderBegin(miteRender **mrrP, miteUser *muu);
extern int miteRenderEnd(miteRender *mrr, miteUser *muu);

/* thread.c */
extern int miteThreadBegin(miteThread **mttP, miteRender *mrr, miteUser *muu,
			   int whichThread);
extern int miteThreadEnd(miteThread *mtt, miteRender *mrr, miteUser *muu);

/* ray.c */
extern int miteRayBegin(miteThread *mtt, miteRender *mrr, miteUser *muu,
			int uIndex, int vIndex, 
			double rayLen,
			double rayStartWorld[3], double rayStartIndex[3],
			double rayDirWorld[3], double rayDirIndex[3]);
extern double miteSample(miteThread *mtt, miteRender *mrr, miteUser *muu,
			 int num, double rayT, int inside,
			 double samplePosWorld[3],
			 double samplePosIndex[3]);
extern int miteRayEnd(miteThread *mtt, miteRender *mrr,
		      miteUser *muu);

#ifdef __cplusplus
}
#endif

#endif /* MITE_HAS_BEEN_INCLUDED */
