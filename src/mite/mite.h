/*
  teem: Gordon Kindlmann's research software
  Copyright (C) 2004, 2003, 2002, 2001, 2000, 1999, 1998 University of Utah

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

#include <teem/air.h>
#include <teem/biff.h>
#include <teem/ell.h>
#include <teem/nrrd.h>
#include <teem/gage.h>
#include <teem/limn.h>
#include <teem/hoover.h>
#include <teem/ten.h>

#if defined(_WIN32) && !defined(TEEM_STATIC) && !defined(__CYGWIN__)
#define mite_export __declspec(dllimport)
#else
#define mite_export
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define MITE miteBiffKey

/*
******** mite_t
**
** the type used for representing and storing transfer function *range*
** (color, opacity, etc) information is:
** 1: float
** 0: double
*/

#if 0
typedef float mite_t;
#define mite_nt nrrdTypeFloat
#define mite_at airTypeFloat
#define limnVTOQN limnVtoQN_f
#define MITE_T_DOUBLE 0
#else
typedef double mite_t;
#define mite_nt nrrdTypeDouble
#define mite_at airTypeDouble
#define limnVTOQN limnVtoQN_d
#define MITE_T_DOUBLE 1
#endif

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
** There are some constraints on how the nrrd as transfer function is
** stored and used:
** 1) all transfer functions are lookup tables: there is no 
** interpolation other than nearest neighbor (actually, someday linear
** interpolation may be supported, but that's it)
** 2) regardless of the centerings of the axes of nxtf, the lookup table
** axes will be treated as though they were cell centered (linear 
** interpolation will always use node centering)
** 3) the logical dimension of the transfer function is always one less
** ntxf->dim, with axis 0 always for the range of the function, and axes
** 1 and onwards for the domain.  For instance, a univariate opacity map
** is 2D, with ntxf->axis[0].size == 1.
** 
** So, ntxf->dim-1 is the number of variables in the domain of the transfer
** function, and ntxf->axis[0].size is the number of variables in the range.
*/

/*
******** miteUser struct
**
** all the input parameters for mite specified by the user, as well as
** a mop for cleaning up memory used during rendering.  Currently,
** unlike gage, there is no API for setting these- you go in the
** struct and set them yourself. 
**
** Mite can currently handle scalar, 3-vector, and (symmetric) tensor
** volumes, one (at most) of each.  All these volumes must have the
** same 3-D size, because we're only using one gageContext per thread,
** and the gageContext is what stores the convolution kernel weights
** evaluated per sample.
*/
typedef struct {
  Nrrd *nsin,            /* scalar volume being rendered */
    *nvin,               /* 3-vector volume being rendered */
    *ntin,               /* tensor volume being rendered */
    **ntxf,              /* array of nrrds containing transfer functions,
			    these are never altered (in contrast to ntxf
			    in miteRender) */
    *nout;               /* output image container, for five-channel output
			    RGBAZ.  We'll nrrdMaybeAlloc all the image data
			    and put it in here, but we won't nrrdNuke(nout),
			    just like we won't nrrdNuke nsin, ntin, or any of
			    the ntxf[i] */
  int ntxfNum;           /* allocated and valid length of ntxf[] */
  char shadeStr[AIR_STRLEN_MED];  /* how to do shading */
  /* for each possible element of the txf range, what value should it
     start at prior to multiplying by the values (if any) learned from
     the txf.  Mainly needed to store non-unity values for the
     quantities not covered by a transfer function */
  mite_t rangeInit[MITE_RANGE_NUM]; 
  double refStep,        /* length of "unity" for doing opacity correction */
    rayStep,             /* distance between sampling planes */
    opacMatters,         /* minimal significant opacity, currently used to
			    assign a Z depth (really "Tw") for each rendered
			    ray */
    opacNear1;           /* opacity close enough to unity for the sake of
			    doing early ray termination */
  hooverContext *hctx;   /* context and input for all hoover-related things,
			    including camera and image parameters */
  /* local initial copies of kernels, later passed to gageKernelSet */
  NrrdKernelSpec *ksp[GAGE_KERNEL_NUM];
  gageContext *gctx0;    /* context and input for all gage-related things,
			    including all kernels.  This is gageContextCopy'd
			    for multi-threaded use (hence the 0) */
  limnLight *lit;        /* a struct for all lighting info, although 
			    currently only the ambient and *first* directional
			    light are used */
  int normalSide,        /* determines direction of gradient that is used
			    as normal for shading:
			    1: normal points to lower values
                               (higher values are more "inside"); 
                            0: "two-sided": dot-products are abs()'d
                            -1: normal points to higher values
                               (lower values more "inside") */
    verbUi, verbVi;      /* pixel coordinate for which to turn on verbosity */
  airArray *umop;        /* for things allocated which are used across
			    multiple renderings */
  /* output information from last rendering */
  double rendTime,       /* rendering time, in seconds */
    sampRate;            /* rate (KHz) at which sampler callback was called */
} miteUser;

struct miteThread_t;

/*
******** miteShadeMethod* enum
**
** the different ways that shading can be done
*/
enum {
  miteShadeMethodUnknown,
  miteShadeMethodNone,        /* 1: no direction shading based on anything
				 in the miteShadeSpec: just ambient 
				 (though still using over operator) */
  miteShadeMethodPhong,       /* 2: what mite has been doing all along */
  miteShadeMethodLitTen,      /* 3: (tensor-based) lit-tensors */
  miteShadeMethodLast
};

/*
******** miteShadeSpec struct
**
** describes how to do shading.  With more and more generality in the
** expressions that are evaluated for transfer function application,
** there is less need for this "shading" per se (phong shading can be
** expressed with multiplicative and additive transfer functions).
** But its here for the time being...
*/
typedef struct {
  int shadeMethod;            /* from miteShadeMethod* enum */
  gageQuerySpec *vec0, *vec1,
    *scl0, *scl1;             /* things to use for shading.  How these are
				 interpreted is determined by shadeMethod:
				 phong: vec0 is used as normal
				 litten: lit-tensors based on vec0 and vec1,
				 as weighted by scl0, scl1 */
} miteShadeSpec;

/*
******** miteRender
**
** rendering-parameter-set-specific (but non-thread-specific) state,
** used internally by mite
*/
typedef struct {
  Nrrd **ntxf;                /* array of transfer function nrrds.  The 
				 difference from those in miteUser is that 
				 opacity correction (based on rayStep and 
				 refStep) has been applied to these, and
				 these have been converted/unquantized to
				 type mite_t */
  int ntxfNum;                /* allocated and valid length of ntxf[] */
  miteShadeSpec *shpec;       /* information based on muu->shadeStr */
  double time0;               /* rendering start time */

  /* as long as there's no mutex around how the miteThreads are
     airMopAdded to the miteUser's mop, these have to be _allocated_ in
     mrendRenderBegin, instead of mrendThreadBegin, which still has the
     role of initializing them */
  struct miteThread_t *tt[HOOVER_THREAD_MAX];  
  airArray *rmop;             /* for things allocated which are rendering
				 (or rendering parameter) specific */
} miteRender;

/*
******** miteStageOp* enum
**
** the kinds of things we can do per txf to modify the range variables.
** previously mite only supported seperable transfer functions (i.e.,
** multiplication only)
*/
enum {
  miteStageOpUnknown,
  miteStageOpAdd,
  miteStageOpMax,
  miteStageOpMultiply,
  miteStageOpLast
};

typedef struct {
  gage_t *val;                  /* the txf axis variable, computed either by
				   gage or by mite.  This points into the 
				   answer vector in one of the thread's
				   pervolumes */
  int size,                     /* number of entries along this txf axis */
    op,                         /* from miteStageOp* enum.  Note that this
				   operation applies to ALL the range variables
				   adjusted by this txf (can't add color while
				   multiplying opacity) */
    (*qn)(gage_t *);            /* callback for doing vector quantization of
				   vector-valued txf domain variables, or
				   NULL if this is a scalar variable */
  double min, max;              /* min, max (copied from nrrd axis) */
  mite_t *data;                 /* pointer to txf data.  If non-NULL, the
				   rest of the variables are meaningful */
  int rangeIdx[MITE_RANGE_NUM], /* indices into miteThread's range */
    rangeNum;                   /* number of range variables set by the txf
				   == number of pointers in range[] to use */
} miteStage;

/*
******** miteVal* enum
** 
** the quantities not measured by gage (but often reliant on gage-based
** measurements) which can appear in the transfer function domain.
** In many respects, these behave like gage queries, and these are
** associated with a gageKind (miteValGageKind), but it is hardly a 
** real, bona fide, gageKind. The answers for these are stored in
** the miteThread, in lieu of a gagePerVolume
*/
enum {
  miteValUnknown=-1,    /* -1: nobody knows */
  miteValXw,            /*  0: "Xw", X position, world space (*gage_t) */
  miteValXi,            /*  1: "Xi", X     "   , index   "   (*gage_t) */
  miteValYw,            /*  2: "Yw", Y     "   , world   "   (*gage_t) */
  miteValYi,            /*  3: "Yi", Y     "   , index   "   (*gage_t) */
  miteValZw,            /*  4: "Zw", Z     "   , world   "   (*gage_t) */
  miteValZi,            /*  5: "Zi", Z     "   , index   "   (*gage_t) */
  miteValTw,            /*  6: "Tw", ray position (*gage_t) */
  miteValTi,            /*  7: "Ti", ray index (ray sample #) (*gage_t) */
  miteValNdotV,         /*  8: "NdotV", surface normal dotted w/ view vector
			        (towards eye) (*gage_t) */
  miteValNdotL,         /*  9: "NdotL", surface normal dotted w/ light vector
			        (towards the light source) (*gage_t) */
  miteValGTdotV,        /* 10: "GTdotV", normal curvature in view direction 
			        (*gage_t) */
  miteValVrefN,         /* 11: "VrefN", view vector (towards eye) reflected
			       across surface normal (gage_t[3]) */
  miteValVdefT,         /* 12: "VdefT", view direction, deflected by tensor,
			       then normalized (gage_t[3]) */
  miteValVdefTdotV,     /* 13: "VdefTdotV", VdefT dotted back with V, not the
			       same as the tensor contraction along V,
			       (*gage_t) */
  miteValLast
};
#define MITE_VAL_MAX       13
#define MITE_VAL_TOTAL_ANS_LENGTH   18

/*
******** miteThread
**
** thread-specific state for mite's internal use
*/
typedef struct miteThread_t {
  gageContext *gctx;            /* per-thread context */
  gage_t *ansScl,               /* shortcut to scalar answer vector */
    *ansVec,                    /* shortcut to vector answer vector */
    *ansTen,                    /* shortcut to tensor answer vector */
    *vec0, *vec1, *scl0, *scl1, /* shortcuts vectors/scalars used for 
				   shading; explained with miteShadeSpec */
    ansMiteVal[MITE_VAL_TOTAL_ANS_LENGTH];  /* room for all the miteVal */
  int verbose,                  /* blah blah blah */
    thrid,                      /* thread ID */
    ui, vi,                     /* image coords */
    samples;                    /* number of samples handled so far by
				   this thread */
  miteStage *stage;             /* array of stages for txf computation */
  int stageNum;                 /* number of stages == length of stage[] */
  mite_t range[MITE_RANGE_NUM], /* rendering variables, which are either
				   copied from miteUser's rangeInit[], or
				   over-written by txf evaluation */
    rayStep,                    /* per-ray step (may need to be different for
				   each ray to enable sampling on planes) */
    V[3],                       /* per-ray view direction */
    RR, GG, BB, TT;             /* per-ray composited values */
} miteThread;

/* defaultsMite.c */
extern mite_export const char *miteBiffKey;
extern mite_export double miteDefRefStep;
extern mite_export int miteDefRenorm;
extern mite_export int miteDefNormalSide;
extern mite_export double miteDefOpacNear1;
extern mite_export double miteDefOpacMatters;

/* kindnot.c */
extern mite_export airEnum *miteVal;
extern mite_export gageKind *miteValGageKind;

/* txf.c */
extern mite_export char miteRangeChar[MITE_RANGE_NUM];
extern void miteVariablePrint(char *buff, const gageQuerySpec *qsp);
extern int miteVariableParse(gageQuerySpec *qsp, const char *label);
extern int miteNtxfCheck(const Nrrd *ntxf);

/* user.c */
extern miteUser *miteUserNew();
extern miteUser *miteUserNix(miteUser *muu);

/* renderMite.c */
extern miteShadeSpec *miteShadeSpecNew();
extern miteShadeSpec *miteShadeSpecNix(miteShadeSpec *);
extern int miteShadeParse(miteShadeSpec *shpec, char *shadeStr);
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
