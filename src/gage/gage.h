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
#include <ell.h>
#include <nrrd.h>

/*
** the only extent to which gage treats different axes differently is
** the spacing between samples along the axis.  To have different
** filters for the same function, but along different axes, would be
** too messy.  Thus, gage is not very useful as the engine for
** downsampling: it can't tell that along one axis samples should be
** blurred while they should be interpolated along another.  Rather,
** it assumes that the main task of probing is *reconstruction*: of
** values, of derivatives, or lots of different quantities
*/

/*
******** gage_t
** 
** this is the very crude means by which you can control the type
** of values that gage works with: "float" or "double".  It is an
** unfortunate but greatly simplifying restriction that this type
** is used for all types of probing (scalar, vector, etc).
**
** So: choose float or double and comment/uncomment the corresponding
** set of lines below.
*/

typedef float gage_t;
#define gage_nrrdType nrrdTypeFloat
#define GAGE_TYPE_FLOAT 1

/*
typedef double gage_t;
#define gage_nrrdType nrrdTypeDouble
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
  gageKernelLast
};
#define GAGE_KERNEL_NUM    6

/*
** modifying the enums below (scalar, vector, etc query quantities)
** necesitates modifying the associated arrays in arrays.c, the
** arrays in enums.c, the fields in the associated answer struct,
** the constructor for the answer struct, and obviously the "answer"
** method itself.
*/

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
** (in the following, GT means gage_t)
*/
enum {
  gageSclUnknown=-1,  /* -1: nobody knows */
  gageSclValue,       /*  0: data value: *GT */
  gageSclGradVec,     /*  1: gradient vector, un-normalized: GT[3] */
  gageSclGradMag,     /*  2: gradient magnitude: *GT */
  gageSclNormal,      /*  3: gradient vector, normalized: GT[3] */
  gageSclHessian,     /*  4: Hessian: GT[9] (column-order) */
  gageSclLaplacian,   /*  5: Laplacian: Dxx + Dyy + Dzz: *GT */
  gageSclHessEval,    /*  6: Hessian's eigenvalues: GT[3] */
  gageSclHessEvec,    /*  7: Hessian's eigenvectors: GT[9] */
  gageScl2ndDD,       /*  8: 2nd dir.deriv. along gradient: *GT */
  gageSclGeomTens,    /*  9: symm. matrix w/ evals 0,K1,K2 and evecs grad,
			     curvature directions: GT[9] */
  gageSclCurvedness,  /* 10: L2 norm of K1, K2 (not Koen.'s "C"): *GT */
  gageSclShapeTrace,  /* 11, (K1+K2)/Curvedness: *GT */
  gageSclShapeIndex,  /* 12: Koen.'s shape index, ("S"): *GT */
  gageSclK1K2,        /* 13: principle curvature magnitudes: GT[2] */
  gageSclCurvDir,     /* 14: principle curvature directions: GT[6] */
  gageSclLast
};
#define GAGE_SCL_MAX     14
#define GAGE_SCL_TOTAL_ANS_LENGTH 51

enum {
  gageVecUnknown=-1,  /* -1: nobody knows */
  gageVecVector,      /*  0: component-wise-interpolatd (CWI) vector: GT[3] */
  gageVecLength,      /*  1: length of CWI vector: *GT */
  gageVecNormalized,  /*  2: normalized CWI vector: GT[3] */
  gageVecJacobian,    /*  3: component-wise Jacobian: GT[9]
			     0:dv_x/dx  3:dv_x/dy  6:dv_x/dz
			     1:dv_y/dx  4:dv_y/dy  7:dv_y/dz
			     2:dv_z/dx  5:dv_z/dy  8:dv_z/dz */
  gageVecDivergence,  /*  4: divergence (based on Jacobian): *GT */
  gageVecCurl,        /*  5: curl (based on Jacobian): *GT */
  gageVecLast
};
#define GAGE_VEC_MAX      5
#define GAGE_VEC_TOTAL_ANS_LENGTH 22

/*
******** gageVal... enum
**
** the different integer values/flags in a gageContext
** that can be got (via gageValGet()) or set (via gageValSet())
*/
enum {
  gageValUnknown=-1,     /* -1: nobody knows */
  gageValVerbose,        /*  0: verbosity */
  gageValRenormalize,    /*  1: make mask weights' sum = continuous integral */
  gageValCheckIntegrals, /*  2: verify integrals of kernels */
  gageValK3Pack,         /*  3: use only three kernels (00, 11, and 22) */
  gageValNeedPad,        /*  4: given kernels chosen, the padding needed */
  gageValHavePad,        /*  5: the padding of the volume used */
  gageValLast
};

/*
******** gageContext struct
**
** The information here is specific to the dimensions, scalings, and
** padding of a volume, but not to kind of volume (all kind-specific
** information is in the gagePerVolume).  One context can be used in
** conjuction with probing two different kinds of volumes.
*/
typedef struct {
  /*  --------------------------------------- Input parameters */
  int verbose;                /* verbosity */
  gage_t gradMagMin;          /* gradient vector lengths can never be
				 smaller than this */
  double integralNearZero;    /* tolerance with checkIntegrals */
  NrrdKernel *k[GAGE_KERNEL_NUM];
                              /* interp, 1st, 2nd deriv. kernels */
  double kparm[GAGE_KERNEL_NUM][NRRD_KERNEL_PARMS_NUM];
                              /* kernel parameters */
  int renormalize;            /* hack to make sure that sum of
				 discrete value reconstruction weights
				 is same as kernel's continuous
				 integral, and that the 1nd and 2nd
				 deriv weights really sum to 0.0 */
  int checkIntegrals;         /* call the "integral" method of the
				 kernel to verify that it is
				 appropriate for the task for which
				 the kernel is being set:
				 reconstruction: 1.0, derivatives: 0.0 */
  int k3pack;                 /* non-zero (true) iff we do not use
				 kernels for gageKernelIJ with I != J.
				 So, we use the value reconstruction
				 kernel (gageKernel00) for 1st and 2nd
				 derivative reconstruction, and so on.
				 This is faster because we can re-use
				 results from low-order convolutions. */
  /*  --------------------------------------- Internal state */
  int haveVolume;             /* non-zero iff gageVolumeSet has been called
				 (used to ensure that all volumes associated
				 with this context are consistent */
  /*  ------------ kernel-dependent */
  int needPad;                /* amount of boundary margin required
				 for current kernels (irrespective of
				 query, which is perhaps foolish) */
  int fr, fd;                 /* max filter radius and diameter */
  gage_t *fsl,                /* filter sample locations (all axes) */
    *fw;                      /* filter weights (all axes, all kernels):
				 logically a fd x 3 x GAGE_KERNEL_NUM array */
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
  /*  ------------ query-dependent: actually, dependent on every query
      associated with all pervolumes used with this context */
  int needK[GAGE_KERNEL_NUM]; /* which kernels are needed */
  /*  ------------ probe-location-dependent */
  gage_t xf, yf, zf;          /* fractional voxel location of last
				 query, used to short-circuit
				 calculation of filter sample
				 locations and weights */
  int bidx;                   /* base-index: lowest (linear) index, in
				 the PADDED volume, of the samples
				 currrently in 3D value cache */
} gageContext;

/*
******** gagePerVolume
**
** information that is specific to one volume, and to one kind of
** volume.
*/
typedef struct {
  int verbose;
  unsigned int query;         /* the query (recursively expanded) */
  Nrrd *npad;                 /* user-padded nrrd, not "owned" by gage,
				 nrrdNuke() and nrrdNix() not called */
  /*  --------------------------------------- Internal state */
  /*  ------------ kernel-dependent */
  gage_t *iv3, *iv2, *iv1;    /* 3D, 2D, 1D, value caches.  Exactly how
				 values are arranged in iv3 (non-scalar
				 volumes can have the component axis
				 be the slowest or the fastest) is not
				 strictly speaking gage's concern, as
				 filling iv3 is up to iv3Fill in the
				 gageKind struct */
  /*  ------------ volume-dependent */
  struct gageKind_t *kind;
  gage_t (*lup)(void *ptr, nrrdBigInt I); 
                              /* nrrd{F,D}Lookup[] element, according to
				 npad and gage_t */
  /*  ------------ query-dependent: these represent the needs of
      the one query associated with this pervolume */
  int doV, doD1, doD2;        /* which derivatives need to be calculated
				 (more immediately useful for 3pack) */
  int needK[GAGE_KERNEL_NUM]; /* which kernels are needed */
  /*  --------------------------------------- Output */
  void *ans;                  /* an answer struct, such as gageSclAnswer */
} gagePerVolume;

/*
******** gageKind struct
**
** all the information and functions that are needed to handle one
** kind of volume (such as scalar, vector, etc.)
*/
typedef struct gageKind_t {
  char str[AIR_STRLEN_SMALL];       /* short identifying string for kind */
  int baseDim,                      /* dimension that x,y,z axes start on */
    valLen,                         /* number of scalars per data point */
    queryMax,                       /* such as GAGE_SCL_MAX */
    totalAnsLen,                    /* such as GAGE_SCL_TOTAL_ANS_LENGTH */
    *needDeriv;                     /* such as _gageSclNeedDeriv */
  unsigned int *queryPrereq;        /* such as _gageSclPrereq; */
  void (*queryPrint)(unsigned int), /* such as _gageSclPrint_query() */
    *(*ansNew)(void),               /* such as _gageSclAnswerNew() */
    *(*ansNix)(void *),             /* such as _gageSclAnswerNix() */
    (*iv3Fill)(gageContext *,       /* such as _gageSclIv3Fill() */
	       gagePerVolume *,
	       void *),
    (*iv3Print)(gageContext *,      /* such as _gageSclIv3Print() */
		gagePerVolume *),
    (*filter)(gageContext *,        /* such as _gageSclFilter() */
	      gagePerVolume *),
    (*answer)(gageContext *,        /* such as _gageSclAnswer() */
	      gagePerVolume *);
} gageKind;

/*
******** gageSimpleStruct
**
** If all defaults are okay, and if padding by bleeding is okay,
** and if there is only one volume per context, then this is a 
** suitable basis for a simplified access to gage functionality.
*/
typedef struct {
  Nrrd *nin;                  /* the input volume to probe */
  gageKind *kind;             /* kind of volume */
  NrrdKernel *k[GAGE_KERNEL_NUM];
                              /* interp, 1st, 2nd deriv. kernels */
  double kparm[GAGE_KERNEL_NUM][NRRD_KERNEL_PARMS_NUM];
                              /* kernel parameters */
  unsigned int query;         /* the bit-flag query */
  /*  --------------------------------------- Internal state */
  Nrrd *npad;                 /* padded input */
  gageContext *ctx;
  gagePerVolume *pvl;
  /*  --------------------------------------- Output */
  void *ans;                  /* an answer struct, such as gageSclAnswer */
} gageSimple;

/*
******** gageSclAnswer struct
**
** Where answers to scalar probing are stored.
*/
typedef struct {
  gage_t
    ans[GAGE_SCL_TOTAL_ANS_LENGTH],  /* all the answers */
    *val, *gvec,                     /* convenience pointers into ans[] */
    *gmag, *norm,      
    *hess, *lapl, *heval, *hevec, *scnd,
    *gten, *C, *St, *Si, *k1k2, *cdir;
} gageSclAnswer;

/*
******** gageVecAnswer struct
**
** Where answers to vector probing are stored.
*/
typedef struct {
  gage_t
    ans[GAGE_VEC_TOTAL_ANS_LENGTH], /* all the answers */
    *vec, *len,                     /* convenience pointers into ans[] */
    *norm, *jac,      
    *div, *curl;
} gageVecAnswer;

/* defaults.c */
extern int gageDefVerbose;
extern gage_t gageDefGradMagMin;
extern int gageDefRenormalize;
extern int gageDefCheckIntegrals;
extern int gageDefK3Pack;

/* enums.c */
extern airEnum gageKernel;
extern airEnum gageScl;
extern airEnum gageVec;

/* arrays.c */
extern gage_t gageZeroNormal[3];
extern char gageErrStr[AIR_STRLEN_LARGE];
extern int gageErrNum;
extern int gageSclAnsLength[GAGE_SCL_MAX+1];
extern int gageSclAnsOffset[GAGE_SCL_MAX+1];
extern int gageVecAnsLength[GAGE_VEC_MAX+1];
extern int gageVecAnsOffset[GAGE_VEC_MAX+1];

/* kinds.c */
extern gageKind *gageKindScl;
extern gageKind *gageKindVec;

/* methods.c */
extern gageContext *gageContextNew();
extern gageContext *gageContextNix(gageContext *ctx);
extern gagePerVolume *gagePerVolumeNew(int needPad, gageKind *kind);
extern gagePerVolume *gagePerVolumeNix(gagePerVolume *pvl);
extern gageSimple *gageSimpleNew();
extern gageSimple *gageSimpleNix(gageSimple *spl);

/* general.c */
extern void gageValSet(gageContext *ctx, int which, int val);
extern int gageValGet(gageContext *ctx, int which);
extern int gageKernelSet(gageContext *ctx,
			 int which, NrrdKernel *k, double *kparm);
extern void gageKernelReset(gageContext *ctx);
extern int gageVolumeSet(gageContext *ctx, gagePerVolume *pvl,
			 Nrrd *npad, int havePad);
extern int gageQuerySet(gagePerVolume *pvl, unsigned int query);
extern int gageUpdate(gageContext *ctx, gagePerVolume *pvl);
extern int gageProbe(gageContext *ctx, gagePerVolume *pvl,
		     gage_t x, gage_t y, gage_t z);

/* simple.c */
extern int gageSimpleUpdate(gageSimple *gsl);
extern int gageSimpleKernelSet(gageSimple *gsl,
			       int which, NrrdKernel *k, double *kparm);
extern int gageSimpleProbe(gageSimple *gsl, gage_t x, gage_t y, gage_t z);

#endif /* GAGE_HAS_BEEN_INCLUDED */
#ifdef __cplusplus
}
#endif
