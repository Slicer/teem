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

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>

#include <air.h>
#include <biff.h>
#include <ell.h>
#include <nrrd.h>

#ifdef __cplusplus
extern "C" {
#endif

#define GAGE "gage"

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
******** GAGE_PERVOLUME_NUM
**
** max number of pervolumes that can be associated with a context.
** Since this is so often just 1, it makes no sense to adopt a more
** general mechanism to allow an unlimited number of pervolumes.
*/
#define GAGE_PERVOLUME_NUM 4

/*
******** the unnamed enum of boolean-ish flags
**
** these are passed to gageSet
*/
enum {
  gageVerbose,             /* this isn't a boolean */
  gageRenormalize,
  gageCheckIntegrals,
  gageNoRepadWhenSmaller,
  gageK3Pack
};

/*
******** gageFlag... enum
**
** organizes all the dependendies within a context and between a
** context and pervolumes.  Basically, all of this complexity is to
** the handle the fact that different kernels have diferrent supports,
** which requires different amounts of padding around volumes.  The
** user should not have to be concerned about any of this; it should
** be useful only to gageUpdate().  The flags pertaining to state 
** which is specific to a kind is indicated as such.
*/
enum {
  gageFlagUnknown=-1,   /* -1: nobody knows */
  gageFlagNeedD,        /*  0: (kind-specific) required derivatives */
  gageFlagK3Pack,       /*  1: whether to use 3 or 6 kernels */
  gageFlagNeedK,        /*  2: which of the kernels will actually be used */
  gageFlagKernel,       /*  3: any one of the kernels or its parameters */
  gageFlagNRWS,         /*  4: change in ctx->noRepadWhenSmaller */
  gageFlagNeedPad,      /*  5: required padding based on required kernels */
  gageFlagPadder,       /*  6: (kind-specific) how to pad volumes */
  gageFlagNin,          /*  7: (kind-specific) original unpadded volume */
  gageFlagHavePad,      /*  8: padding we'll actually use (may not want to
			       repad when going to a smaller padding) */
  gageFlagNpad,         /*  9: (kind-specific) padded volume */
  gageFlagLast
};
#define GAGE_FLAG_NUM      10

/*
******** gageKernel... enum
**
** these are the different kernels that might be used in gage, regardless
** of what kind of volume is being probed.
*/
enum {
  gageKernelUnknown=-1, /* -1: nobody knows */
  gageKernel00,         /*  0: measuring values */
  gageKernel10,         /*  1: reconstructing 1st derivatives */
  gageKernel11,         /*  2: measuring 1st derivatives */
  gageKernel20,         /*  3: reconstructing 1st partials and 2nd deriv.s */
  gageKernel21,         /*  4: measuring 1st partials for a 2nd derivative */
  gageKernel22,         /*  5: measuring 2nd derivatives */
  gageKernelLast
};
#define GAGE_KERNEL_NUM     6

/*
** modifying the enums below (scalar, vector, etc query quantities)
** necesitates modifying the associated arrays in arrays.c, the
** arrays in enums.c, the fields in the associated answer struct,
** the constructor for the answer struct, and obviously the "answer"
** method itself.
*/

/*
******** gageScl* enum
**
** all the things that gage can measure in a scalar volume.  The query is
** formed by a bitwise-or of left-shifts of 1 by these values:
**   (1<<gageSclValue)|(1<<gageSclGradMag)|(1<<gageScl2ndDD)
** queries for the value, gradient magnitude, and 2nd directional derivative.
** The un-bit-shifted values are required for gage to index arrays like
** gageSclAnsOffset[], _gageSclNeedDeriv[], _gageSclPrereq[], etc, and
** for the gageScl airEnum.
**
** NOTE: although it is currently listed that way, it is not necessary
** that prerequisite measurements are listed before the other measurements
** which need them (that is represented by _gageSclPrereq)
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

/*
******** GAGE_SCL_*_BIT #defines
** already bit-shifted for you, so that query:
**   (1<<gageSclValue)|(1<<gageSclGradMag)|(1<<gageScl2ndDD)
** is same as:
**   GAGE_SCL_VALUE_BIT | GAGE_SCL_GRADMAG_BIT | GAGE_SCL_2NDDD_BIT
*/
#define GAGE_SCL_VALUE_BIT      (1<<0)
#define GAGE_SCL_GRADVEC_BIT    (1<<1)
#define GAGE_SCL_GRADMAG_BIT    (1<<2)
#define GAGE_SCL_NORMAL_BIT     (1<<3)
#define GAGE_SCL_HESSIAN_BIT    (1<<4)
#define GAGE_SCL_LAPLACIAN_BIT  (1<<5)
#define GAGE_SCL_HESSEVAL_BIT   (1<<6)
#define GAGE_SCL_HESSEVEC_BIT   (1<<7)
#define GAGE_SCL_2NDDD_BIT      (1<<8)
#define GAGE_SCL_GEOMTENS_BIT   (1<<9)
#define GAGE_SCL_CURVEDNESS_BIT (1<<10)
#define GAGE_SCL_SHAPETRACE_BIT (1<<11)
#define GAGE_SCL_SHAPEINDEX_BIT (1<<12)
#define GAGE_SCL_K1K2_BIT       (1<<13)
#define GAGE_SCL_CURVDIR_BIT    (1<<14)

/*
******** gageVec* enum
**
** all the things that gage knows how to measure in a 3-vector volume
*/
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
#define GAGE_VEC_TOTAL_ANS_LENGTH 20

#define GAGE_VEC_VECTOR_BIT     (1<<0)
#define GAGE_VEC_LENGTH_BIT     (1<<1)
#define GAGE_VEC_NORMALIZED_BIT (1<<2)
#define GAGE_VEC_JACOBIAN_BIT   (1<<3)
#define GAGE_VEC_DIVERGENCE_BIT (1<<4)
#define GAGE_VEC_CURL_BIT       (1<<5)

struct gageKind_t;  /* dumb forward declaraction, ignore */

/*
******** gagePadder_t, gageNixer_t
**
** type of functions used to pad volumes and to remove padded volumes.
** Chances are, no one has to worry about these, since the default
** padder (_gageStandardPadder) and nixer (_gageStandardNixer) probably
** do exactly what you want.
*/
typedef int (gagePadder_t)(Nrrd *npad, Nrrd *nin, struct gageKind_t *kind,
			   int padding, void *padInfo);
typedef void (gageNixer_t)(Nrrd *npad, struct gageKind_t *kind,
			   void *padInfo);

/*
******** gageContext struct
**
** The information here is specific to the dimensions, scalings, and
** padding of a volume, but not to kind of volume (all kind-specific
** information is in the gagePerVolume).  One context can be used in
** conjuction with probing multiple volumes.
*/
typedef struct gageContext_t {
  /*  --------------------------------------- Input parameters */
  int verbose;                /* verbosity */
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
  int noRepadWhenSmaller;     /* if a change in parameters leads a newer and
				 smaller amount of padding, don't generate a
				 new padded volume, use the somewhat overly
				 padded volume */
  int k3pack;                 /* non-zero (true) iff we do not use
				 kernels for gageKernelIJ with I != J.
				 So, we use the value reconstruction
				 kernel (gageKernel00) for 1st and 2nd
				 derivative reconstruction, and so on.
				 This is faster because we can re-use
				 results from low-order convolutions. */
  gage_t gradMagMin;          /* pre-normalized vector lengths can't be
				 smaller than this */
  double integralNearZero;    /* tolerance with checkIntegrals on derivative
				 kernels */
  NrrdKernel *k[GAGE_KERNEL_NUM];
                              /* all the kernels we might ever need */
  double kparm[GAGE_KERNEL_NUM][NRRD_KERNEL_PARMS_NUM];
                              /* all the kernel parameters */
  struct gagePerVolume_t *pvl[GAGE_PERVOLUME_NUM];
                              /* the pervolumes attached to this context */
  int numPvl;                 /* number of pervolumes currently attached */
  gagePadder_t *padder;       /* how to pad nin to produce npad; use NULL
				 to signify no-op */  
  gageNixer_t *nixer;         /* for when npad is to be replaced or removed;
				 use NULL to signify no-op */
  /*  --------------------------------------- Internal state */
  int flag[GAGE_FLAG_NUM];    /* all the flags used by gageUpdate() used to
				 describe what changed in this context */
  int pvlFlag[GAGE_FLAG_NUM]; /* for the kind-specific flags, these mean that
				 something changed in one of the pvl's, which
				 may or may not mean that the same flag will be
				 raised in the context's main flag[] array */
  int thisIsACopy;            /* I am the result of gageContextCopy */
  int needPad;                /* amount of boundary margin required for current
				 queries (in all pervolumes), kernels, and
				 the value of k3pack */
  int havePad;                /* amount of padding currently in pervolumes */
  int fr, fd;                 /* max filter radius and diameter (among the
				 required kernels) */
  gage_t *fsl,                /* filter sample locations (all axes):
				 logically a fd x 3 array */
    *fw;                      /* filter weights (all axes, all kernels):
				 logically a fd x 3 x GAGE_KERNEL_NUM array */
  unsigned int *off;          /* offsets to other fd^3 samples needed to fill
				 3D intermediate value cache. Allocated size is
				 dependent on kernels, values inside are
				 dependent on the dimensions of the volume. It
				 may be more correct to be using size_t
				 instead of uint, but the X and Y dimensions of
				 the volume would have to be super-outrageous
				 for that to be a problem */
  int sx, sy, sz;             /* dimensions of PADDED volume */
  gage_t xs, ys, zs,          /* spacings for each axis */
    fwScale[GAGE_KERNEL_NUM][3];
                              /* how to rescale weights for each of the
				 kernels according to non-unity-ness of
				 sample spacing (0=X, 1=Y, 2=Z) */
  int needD[3];               /* which value/derivatives need to be calculated
				 for all pervolumes (doV, doD1, doD2) */
  int needK[GAGE_KERNEL_NUM]; /* which kernels are needed for all pervolumes */
  gage_t xf, yf, zf;          /* fractional voxel location of last query, used
				 to short-circuit calculation of filter sample
				 locations and weights */
  int bidx;                   /* base-index: lowest (linear) index, in the
				 PADDED volume, of the samples currrently in
				 3D value cache */
} gageContext;

/*
******** gagePerVolume
**
** information that is specific to one volume, and to one kind of
** volume.
*/
typedef struct gagePerVolume_t {
  struct gageKind_t *kind;    /* what kind of volume is this pervolume for */
  unsigned int query;         /* the query, recursively expanded */
  Nrrd *nin;                  /* the original, unpadded volume, passed to the
				 padder below, but never freed or passed
				 to anything else */
  void *padInfo;              /* supplemental information for padder and nixer,
				 but only used when they are different than the
				 usual/default ones.  Setting (and freeing)
				 this is currently none of gage's business, and
				 no other callbacks facilitate handling this */
  Nrrd *npad;                 /* the padded nrrd which is probed */
  /*  --------------------------------------- Internal state */
  int thisIsACopy;            /* I'm a copy */
  int flagNin;                /* this nin has been set/changed */
  gage_t *iv3, *iv2, *iv1;    /* 3D, 2D, 1D, value caches.  Exactly how values
				 are arranged in iv3 (non-scalar volumes can
				 have the component axis be the slowest or the
				 fastest) is not strictly speaking gage's
				 concern, as filling iv3 is up to iv3Fill in
				 the gageKind struct.  Use of iv2 and iv1 is
				 entirely up the kind's filter method. */
  gage_t (*lup)(void *ptr, size_t I); 
                              /* nrrd{F,D}Lookup[] element, according to
				 npad->type and gage_t */
  int needD[3];               /* which derivatives need to be calculated for
				 this pervolumes */
  /*  --------------------------------------- Output */
  void *ansStruct;            /* an answer struct, such as gageSclAnswer */
} gagePerVolume;

/*
******** gageKind struct
**
** all the information and functions that are needed to handle one
** kind of volume (such as scalar, vector, etc.)
*/
typedef struct gageKind_t {
  char name[AIR_STRLEN_SMALL];      /* short identifying string for kind */
  airEnum *enm;                     /* such as gageScl.  NB: the "unknown"
				       value in the enum MUST be -1 (since
				       queries are formed as bitflags) */
  int baseDim,                      /* dimension that x,y,z axes start on
				       (0 for scalars, 1 for vectors) */
    valLen,                         /* number of scalars per data point */
    queryMax,                       /* such as GAGE_SCL_MAX */
    *ansLength,                     /* such as gageSclAnsLength */
    *ansOffset,                     /* such as gageSclAnsOffset */
    totalAnsLen,                    /* such as GAGE_SCL_TOTAL_ANS_LENGTH */
    *needDeriv;                     /* such as _gageSclNeedDeriv */
  unsigned int *queryPrereq;        /* such as _gageSclPrereq; */
  void (*queryPrint)(FILE *,        /* such as _gageSclPrint_query() */
		     unsigned int), 
    *(*ansNew)(void),               /* such as _gageSclAnswerNew() */
    *(*ansNix)(void *),             /* such as _gageSclAnswerNix() */
    (*iv3Fill)(gageContext *,       /* such as _gageSclIv3Fill() */
	       gagePerVolume *,
	       void *),
    (*iv3Print)(FILE *,             /* such as _gageSclIv3Print() */
		gageContext *,
		gagePerVolume *),
    (*filter)(gageContext *,        /* such as _gageSclFilter() */
	      gagePerVolume *),
    (*answer)(gageContext *,        /* such as _gageSclAnswer() */
	      gagePerVolume *);
} gageKind;

/*
** NB: All "answer" structs MUST have the main answer vector "ans"
** as the first element so the main vector of any kind's answer can be
** obtained by casting to gageSclAnswer.
*/

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

/* defaultsGage.c */
extern int gageDefVerbose;
extern gage_t gageDefGradMagMin;
extern int gageDefRenormalize;
extern int gageDefCheckIntegrals;
extern int gageDefNoRepadWhenSmaller;
extern int gageDefK3Pack;
extern double gageDefIntegralNearZero;

/* miscGage.c */
/* gageErrStr and gageErrNum are for describing errors that happen in
   gageProbe(): using biff is too heavy-weight for this, and the idea is
   that no ill should occur if the error is repeatedly ignored */
extern char gageErrStr[AIR_STRLEN_LARGE];
extern int gageErrNum;
extern gage_t gageZeroNormal[3];
extern airEnum *gageKernel;

/* scl.c */
extern int gageSclAnsLength[GAGE_SCL_MAX+1];
extern int gageSclAnsOffset[GAGE_SCL_MAX+1];
extern airEnum *gageScl;
extern gageKind *gageKindScl;

/* vecGage.c (together with vecprint.c, these contain everything to
   implement the "vec" kind, and could be used as examples of what it
   takes to create a new gageKind) */
extern int gageVecAnsLength[GAGE_VEC_MAX+1];
extern int gageVecAnsOffset[GAGE_VEC_MAX+1];
extern airEnum *gageVec;
extern gageKind *gageKindVec;

/* methodsGage.c */
extern gageContext *gageContextNew();
extern gageContext *gageContextCopy(gageContext *ctx);
extern gageContext *gageContextNix(gageContext *ctx);
extern gagePerVolume *gagePerVolumeNew(gageKind *kind);
extern gagePerVolume *gagePerVolumeNix(gageContext *ctx, gagePerVolume *pvl);

/* general.c */
extern void gageSet(gageContext *ctx, int which, int val);
extern int gagePerVolumeAttach(gageContext *ctx, gagePerVolume *pvl);
extern int gagePerVolumeDetach(gageContext *ctx, gagePerVolume *pvl);
extern gage_t *gageAnswerPointer(gagePerVolume *pvl, int measure);
extern void gagePadderSet(gageContext *ctx, gagePadder_t *padder);
extern void gageNixerSet(gageContext *ctx, gageNixer_t *nixer);
extern int gageKernelSet(gageContext *ctx,
			 int which, NrrdKernel *k, double *kparm);
extern void gageKernelReset(gageContext *ctx);
extern int gageQuerySet(gageContext *ctx, gagePerVolume *pvl,
			unsigned int query);
extern int gageVolumeSet(gageContext *ctx, gagePerVolume *pvl,
			 Nrrd *nin);
extern int gageProbe(gageContext *ctx, gage_t x, gage_t y, gage_t z);

/* update.c */
extern int gageUpdate(gageContext *ctx);

#ifdef __cplusplus
}
#endif

#endif /* GAGE_HAS_BEEN_INCLUDED */
