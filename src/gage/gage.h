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
******** GAGE_TYPE, GAGE_TYPE_FLOAT
** 
** this is the very crude means by which you can control the type
** of values that the probe works with: "float" or "double"
*/
/* chose this: ... */
/*
#define GAGE_TYPE double
#define GAGE_TYPE_FLOAT 0
*/
/* ... or this: */

#define GAGE_TYPE float
#define GAGE_TYPE_FLOAT 1


/*
******** gageScl enum
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
** (in the following, GT means GAGE_TYPE)
*/
enum {
  gageSclUnknown=-1,  /* -1: nobody knows */
  gageSclValue,       /*  0: data value (*GT) */
  gageSclGradVec,     /*  1: gradient vector, un-normalized (GT[3])*/
  gageSclGradMag,     /*  2: gradient magnitude (*GT) */
  gageSclNormal,      /*  3: gradient vector, normalized (GT[3]) */
  gageSclHess,        /*  4: Hessian (GT[9]) */
  gageSclHessEval,    /*  5: Hessian's eigenvalues (GT[3]) */
  gageSclHessEvec,    /*  6: Hessian's eigenvectors (GT[9]) */
  gageScl2ndDD,       /*  7: 2nd dir.deriv. along gradient (*GT) */
  gageSclGeomTens,    /*  8: symm. matrix w/ evals 0,K1,K2 and evecs grad,
			     curvature directions (GT[9]) */
  gageSclK1K2,        /*  9: principle curvature magnitudes (GT[2]) */
  gageSclCurvDir,     /* 10: principle curvature directions (GT[6]) */
  gageSclShapeIndex,  /* 11: Koen.'s shape index, ("S") (*GT) */
  gageSclCurvedness,  /* 12: L2 norm of K1, K2 (not Koen.'s "C") (*GT) */
  gageSclLast
};
#define GAGE_SCL_MAX     12
#define GAGE_SCL_TOTAL_ANS_LENGTH 49

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
#define GAGE_KERNEL_MAX    5

typedef struct {
  /*  --------------------------------------- Input parameters */
  unsigned int query;               /* the query */
  nrrdKernel *k[GAGE_KERNEL_MAX+1]; /* interp, 1st, 2nd deriv. kernels */
  double kparam[GAGE_KERNEL_MAX+1][NRRD_KERNEL_PARAMS_MAX];
                                    /* kernel parameters */
  Nrrd *npad;                       /* user-padded nrrd */
  int verbose;                      /* verbosity */
  double epsilon;                   /* gradient magnitude can never be smaller
				       than this */
  int renormalize;                  /* hack to make sure that sum of discrete
				       value reconstruction weights is same as
				       kernel's continuous integral, and that
				       the 1nd and 2nd deriv weights really
				       sum to 0.0 */
  /*  --------------------------------------- Internal state */
  /*  ------------ query-dependent */
  int maxDeriv;                     /* maximum derivitive order needed for
				       current query */
  /*  ------------ kernel-dependent */
  int needPad;                      /* amount of boundary margin required for
				       current kernels (irrespective of query,
				       which is perhaps foolish) */
  int k3pack;                       /* non-zero iff we have no kernels for
				       gageKernelIJ with I != J.  So, we use
				       the value reconstruction kernel
				       (gageKernel00) for 1st and 2nd
				       derivative reconstruction, and so on.
				       This is faster because we can re-use
				       results from low-order
				       convolutions. Dependent on current set
				       of kernels. */
  int fr, fd;                       /* max filter radius and diameter */
  GAGE_TYPE *iv3, *iv2, *iv1,       /* 3D, 2D, 1D, value caches */
    *fsl,                           /* filter sample locations */
    *fw00, *fw10, *fw11,            /* filter weights for all kernels */
    *fw20, *fw21, *fw22;
  /*  ------------ volume-dependent */
  int havePad;                      /* amount of boundary margin associated
				       with current volume (may be greater
				       than needPad) */
  GAGE_TYPE (*lup)(void *ptr, nrrdBigInt I); 
                                    /* nrrd{F,D}Lookup[] element, according to
				       nrrd and GAGE_TYPE */
  int sx, sy, sz;                   /* dimensions of padded nrrd (ctx->npad) */
  float xs, ys, zs;                 /* spacings for each axis */
  int bidx;                         /* base-index: lowest (linear) index of
				       samples in currently cache */
  int *off;                         /* for current sx, sy values, offsets to
				       other fd^3 samples needed to fill iv3
				       value cache. Allocated size is dependent
				       on kernel, values are dependent on the
				       dimensions of the volume */
  /*  ------------ probe-location-dependent */
  double xf, yf, zf;                /* fractional voxel location of last query,
				       used to short-circuit calculation of
				       filter sample locations and weights */
  /*  --------------------------------------- Output */
  GAGE_TYPE
    ans[GAGE_SCL_TOTAL_ANS_LENGTH], /* where all the answers are held */
    *val, *gvec, *gmag, *norm,      /* convenience pointers into ans[] */
    *hess, *heval, *hevec, *scnd,
    *gten, *k1k2, *cdir, *S, *C;
} gageSclContext;

/* defaults.c */
extern int gageDefVerbose;
extern double gageDefEpsilon;
extern int gageDefRenormalize;

/* enums.c */
extern airEnum gageScl;

/* arrays.c */
extern int gageSclAnsLength[GAGE_SCL_MAX+1];
extern int gageSclAnsOffset[GAGE_SCL_MAX+1];
extern int gageSclAnsLen[GAGE_SCL_MAX+1];

/* sclmethods.c */
extern gageSclContext *gageSclContextNew();
extern gageSclContext *gageSclContextNix(gageSclContext *ctx);
extern int gageSclSetQuery(gageSclContext *ctx, unsigned int query);
extern void gageSclResetKernels(gageSclContext *ctx);
extern int gageSclSetKernel(gageSclContext *ctx, int which,
			    nrrdKernel *k, double *param);
extern int gageSclGetNeedPad(gageSclContext *ctx);
extern int gageSclSetPaddedVolume(gageSclContext *ctx, int pad, Nrrd *npad);
extern int gageSclUpdate(gageSclContext *ctx);
extern gageSclContext *gageSclContextCopy(gageSclContext *ctx);

/* scl.c */
extern void gageSclProbe(gageSclContext *ctx, float x, float y, float z);

#endif /* GAGE_HAS_BEEN_INCLUDED */
