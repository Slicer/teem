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

#ifdef __cplusplus
extern "C" {
#endif


#ifndef GAGE_HAS_BEEN_INCLUDED
#define GAGE_HAS_BEEN_INCLUDED

#define GAGE "gage"

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>

#include <air.h>
#include <biff.h>
#include <nrrd.h>
#include <ell.h>

/*
** general notes:
**
** the only extent to which gage treats different axes differently is
** the spacing between samples along the axis.  To have different
** filters for the same function, but along different axes, would be
** too messy.  Thus, gage is not very useful as the engine for
** downsampling: it can't tell that along one axis samples should be
** blurred while they should be interpolated along another.  Rather,
** it assumes that the main task of probing is *reconstruction*: of
** values, of derivatives, of lots of different quantities
**
** controlling 3pack vs 6pack is done solely by which set of kernels
** is set- there is no direct control for that.  Currently, if you
** want to do both kinds of probing, then you need two contexts.
 */

/*
******** gage_t
** 
** this is the very crude means by which you can control the type
** of values that gage works with: "float" or "double".  It is an
** unfortunate but greatly simplifying restriction that this type
** is used for all types of probing (scalar, vector, etc).
*/
/* chose this: ... */

typedef float gage_t;
#define GAGE_TYPE_FLOAT 1

/* ... or this: */
/*
typedef double gage_t;
#define GAGE_TYPE_FLOAT 0
*/

/*
******** gageKernel... enum
**
** these are the different kernels that might be used in gage, regardless
** of what kind of volume is being probed.
*/
enum {
  gageKernelUnknown=-1, /* -1: nobody knows */
  gageKernel00,         /* 0: reconstructing values */
  gageKernel10,         /* 1: reconstructing 1st derivatives */
  gageKernel11,         /* 2: measuring 1st derivatives */
  gageKernel20,         /* 3: reconstructing 1st partials and 2nd deriv.s */
  gageKernel21,         /* 4: measuring 1st partials for a 2nd derivative */
  gageKernel22,         /* 5: measuring 2nd derivatives */
  gageKernelLast,
  gageKernel
};
#define GAGE_KERNEL_NUM    6

/*
******** gageScl... enum
**
** all the things that gage can measure in a scalar volume.  The query is
** formed by a bitwise-or of left-shifts of 1 by these values:
**   (1<<gageSclVal)|(1<<gageSclGradMag)|(1<<gageScl2ndDD)
** queries for the value, gradient magnitude, and 2nd directional derivative.
**
** NOTE: although it is currently listed that way, it is not necessary
** that prerequisite measurements are listed before the other measurements
** which need them
**
** (in the following, GST means gageScl_t)
*/
enum {
  gageSclUnknown=-1,  /* -1: nobody knows */
  gageSclValue,       /*  0: data value (*GST) */
  gageSclGradVec,     /*  1: gradient vector, un-normalized (GST[3])*/
  gageSclGradMag,     /*  2: gradient magnitude (*GST) */
  gageSclNormal,      /*  3: gradient vector, normalized (GST[3]) */
  gageSclHessian,     /*  4: Hessian (GST[9]) */
  gageSclLaplacian,   /*  5: Laplacian: Dxx + Dyy + Dzz (*GST) */
  gageSclHessEval,    /*  6: Hessian's eigenvalues (GST[3]) */
  gageSclHessEvec,    /*  7: Hessian's eigenvectors (GST[9]) */
  gageScl2ndDD,       /*  8: 2nd dir.deriv. along gradient (*GST) */
  gageSclGeomTens,    /*  9: symm. matrix w/ evals 0,K1,K2 and evecs grad,
			     curvature directions (GST[9]) */
  gageSclK1K2,        /* 10: principle curvature magnitudes (GST[2]) */
  gageSclCurvDir,     /* 11: principle curvature directions (GST[6]) */
  gageSclShapeIndex,  /* 12: Koen.'s shape index, ("S") (*GST) */
  gageSclCurvedness,  /* 13: L2 norm of K1, K2 (not Koen.'s "C") (*GST) */
  gageSclLast
};
#define GAGE_SCL_MAX     13
#define GAGE_SCL_TOTAL_ANS_LENGTH 50

/*
******** gageContext struct
**
** this is for everything that is common to any volume probing, be it
** scalar, vector, whatever.  The current intent is that, for the sake
** of simplicity, this context is used in association with only one
** volume, even though in principle the information stored here could
** be shared across, say, a scalar and a vector volume of the same
** dimensions and same padding.  On the off chance that that usage 
** becomes supported, I have refrained from putting the Nrrd* inside
** the gageContext.
**
** Note: these are never created or accessed by the user- all the
** functionality relating to these is declared in private.h.  The
** reason to make the gageContext struct declaration public is because
** hiding it would be too clever, and would complicate setting and
** accessing these fields for normal use and/or debugging.
*/
typedef struct {
  /*  --------------------------------------- Input parameters */
  int verbose;                /* verbosity */
  NrrdKernel *k[GAGE_KERNEL_NUM];
                              /* interp, 1st, 2nd deriv. kernels */
  double kparm[GAGE_KERNEL_NUM][NRRD_KERNEL_PARMS_NUM];
                              /* kernel parameters */
  int renormalize;            /* hack to make sure that sum of
				 discrete value reconstruction weights
				 is same as kernel's continuous
				 integral, and that the 1nd and 2nd
				 deriv weights really sum to 0.0 */
  /*  --------------------------------------- Internal state */
  /*  ------------ kernel-dependent */
  int needPad;                /* amount of boundary margin required
				 for current kernels (irrespective of
				 query, which is perhaps foolish) */
  int fr, fd;                 /* max filter radius and diameter */
  gage_t *fsl;                /* filter sample locations (all axes) */
  int *off;                   /* offsets to other fd^3 samples needed
				 to fill 3D intermediate value
				 cache. Allocated size is dependent on
				 kernels (hence consideration as
				 kernel-dependent), values inside are
				 dependent on the dimensions of the
				 volume */
  /*  ------------ volume-dependent */
  int havePad;                /* amount of boundary margin associated
				 with current volume (may be greater
				 than needPad) */
  int sx, sy, sz;             /* dimensions of padded nrrd (ctx->npad) */
  gage_t xs, ys, zs,          /* spacings for each axis */
    fwScl[GAGE_KERNEL_NUM][3];/* how to rescale weights for each of the
				 kernels according to non-unity-ness of
				 sample spacing (0=X, 1=Y, 2=Z) */
  /* ------------- query-dependent */
  gage_t *fw[GAGE_KERNEL_NUM];/* the needed filter weights (all axes):
				 whether or not these are allocated or
				 left NULL prior to probing is
				 determined by _gageUpdate(). The size
				 that they are allocated to (3*fd) is
				 dependent on the kernel set */
  /*  ------------ probe-location-dependent */
  gage_t xf, yf, zf;          /* fractional voxel location of last
				 query, used to short-circuit
				 calculation of filter sample
				 locations and weights */
  int bidx;                   /* base-index: lowest (linear) index of
				 samples currrently in 3D value cache */
} gageContext;

/*
******** gageSclContext struct
**
** The input, state, and output needed for probing in scalar volumes
*/
typedef struct {
  gageContext c;              /* the general context */
  /*  --------------------------------------- Input parameters */
  Nrrd *npad;                 /* user-padded nrrd.  This is not "owned" by
				 the context- no nrrdNuke() or nrrdNix() call
				 is made by gageSclContextNix() */
  unsigned int query;         /* the query */
  gage_t epsilon;             /* gradient magnitude can never be smaller
				 than this */
  /*  --------------------------------------- Internal state */
  /*  ------------ kernel-dependent */
  int k3pack;                 /* non-zero (true) iff we have no
				 kernels for gageKernelIJ with I != J.
				 So, we use the value reconstruction
				 kernel (gageKernel00) for 1st and 2nd
				 derivative reconstruction, and so on.
				 This is faster because we can re-use
				 results from low-order convolutions. */
  gage_t *iv3, *iv2, *iv1;    /* 3D, 2D, 1D, value caches */
  /*  ------------ volume-dependent */
  gage_t (*lup)(void *ptr, nrrdBigInt I); 
                              /* nrrd{F,D}Lookup[] element, according to
				 npad and gage_t */
  /* ------------- query-dependent */
  int doV, doD1, doD2;        /* which derivatives need to be calculated
				 (more immediately useful for 3pack) */
  int needK[GAGE_KERNEL_NUM]; /* which kernels are needed */
  /*  --------------------------------------- Output */
  gage_t
    ans[GAGE_SCL_TOTAL_ANS_LENGTH],
                              /* where all the answers are held */
    *val, *gvec,              /* convenience pointers into ans[] */
    *gmag, *norm,      
    *hess, *lapl, *heval, *hevec, *scnd,
    *gten, *k1k2, *cdir, *S, *C;
} gageSclContext;

/* defaults.c */
extern int gageDefVerbose;
extern int gageDefRenormalize;
extern double gageDefSclEpsilon;

/* enums.c */
extern airEnum gageScl;

/* arrays.c */
extern char gageErrStr[AIR_STRLEN_LARGE];
extern int gageErrNum;
extern gage_t gageSclZeroNormal[3];
extern int gageSclAnsLength[GAGE_SCL_MAX+1];
extern int gageSclAnsOffset[GAGE_SCL_MAX+1];
extern int gageSclAnsLen[GAGE_SCL_MAX+1];

/* sclmethods.c */
extern gageSclContext *gageSclContextNew();
extern gageSclContext *gageSclContextNix(gageSclContext *sctx);
extern int gageSclKernelSet(gageSclContext *sctx,
			    int which, NrrdKernel *k, double *kparm);
extern int gageSclNeedPadGet(gageSclContext *sctx);
extern int gageSclVolumeSet(gageSclContext *sctx, int pad, Nrrd *npad);
extern int gageSclQuerySet(gageSclContext *sctx, unsigned int query);
extern int gageSclUpdate(gageSclContext *sctx);
extern gageSclContext *gageSclContextCopy(gageSclContext *sctx);

/* scl.c */
extern int gageSclProbe(gageSclContext *sctx, gage_t x, gage_t y, gage_t z);

#endif /* GAGE_HAS_BEEN_INCLUDED */

#ifdef __cplusplus
}
#endif
