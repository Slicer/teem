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

/*
******** #define MITE_TXF_NUM
**
** This number constrains the sum of the dimensions of all the transfer
** functions used.  For instance, if MITE_TXF_NUM is 3, then three 1-D
** transfer functions can be used, or one 1-D and one 2-D, or just
** one 3-D transfer function.  It also serves to limit the dimension
** of any single transfer function.
*/
#define MITE_TXF_NUM 3

/*
******** miteTxfInfo
**
** A transfer function (as nrrd) and information about it which
** facilitates accessing the information in it.  Currently, all
** transfer functions are lookup tables: there is no interpolation
** other than nearest neighbor.  Also regardless of the centerings of
** the axes of nxtf, the lookup table axes will be treated as though
** they were cell centered.
*/
typedef struct {
  Nrrd *ntxf;            /* nrrd containing transfer function */
  float *data;           /* shortcut to ntxf->data */
  int num,               /* number of quantities in the range which this
			    transfer function describes */
    dim;                 /* logical dimension of transfer function; may be
			    one less than ntxf->dim */
} miteTxfInfo;

/*
******** miteUserInfo struct
**
** all the input parameters for mite specified by the user, as well as
** a mop for cleaning up memory used during rendering.  Currently,
** unlike gage, there is no API for setting these- you go in the
** struct and set them yourself. 
*/
typedef struct {
  Nrrd *nin;             /* volume being rendered */
  miteTxfInfo *txf[MITE_TXF_NUM];  /* all transfer function information */
  double refStep,        /* length of "unity" for doing opacity correction */
    rayStep,             /* distance between sampling planes */
    near1;               /* close enough to unity for the sake of doing early
			    ray termination when opacity gets high */
  hooverContext *hctx;   /* context and input for all hoover-related things,
			    including camera and image parameters */
  /* local initial copies of kernels, later passed to gageKernelSet */
  NrrdKernelSpec *ksp[GAGE_KERNEL_NUM];
  gageContext *gctx0;    /* context and input for all gage-related things,
			    including all kernels.  This context is copied
			    for multi-threaded use (hence the 0) */
  limnLight *lit;        /* a struct for all lighting info */
  int justSum,           /* don't use opacity: just sum colors */
    noDirLight;          /* forget directional phong lighting, using only
			    the ambient component */
  char *outS;            /* name of output filename */
  airArray *mop;         /* mop for all resources allocated for rendering */
} miteUserInfo;

struct miteThreadInfo_t;

/*
******** miteRenderInfo
**
** non-thread-specific state relevant for mite's internal use
*/
typedef struct {
  double time0, time1;   /* rendering start and end times */
  int sx, sy;            /* ???? */

  Nrrd *nout;            /* output image nrrd */
  float *imgData;        /* ???? */

  /* as long as there's no mutex around how the miteThreadInfos are
     airMopAdded to the miteUserInfo's mop, these have to be allocated in
     mrendRenderBegin instead of mrendThreadBegin */
  struct miteThreadInfo_t *tt[HOOVER_THREAD_MAX];  
} miteRenderInfo;

/*
******** miteThreadInfo
**
** thread-specific state for mite's internal use
*/
typedef struct miteThreadInfo_t {
  gageContext *gctx;     /* per-thread context */
  gage_t *ans,           /* vector of all gage answers */
    *norm;               /* shortcut to normalized gradient answer */
  double RR, GG, BB, AA; /* color and opacity so far */
  int thrid,             /* thread ID */
    ui, vi;              /* image coords */
} miteThreadInfo;

/* defaultsMite.c */
extern mite_export double miteDefRefStep;
extern mite_export double miteDefNear1;

/* user.c */
/*
******** char miteTxfIdent[]
** R,G,B: red, green, blue color
** A: opacity
** E: emmisivity
** a,d,s: ka, kd, ks phong parameters
** p: specular exponent
*/
extern mite_export char miteTxfIdent[];
extern miteUserInfo *miteUserInfoNew();
extern miteUserInfo *miteUserInfoNix(miteUserInfo *muu);
extern int miteUserInfoValid(miteUserInfo *muu);

/* renderMite.c */
extern int miteRenderBegin(miteRenderInfo **mrrP, miteUserInfo *muu);
extern int miteRenderEnd(miteRenderInfo *mrr, miteUserInfo *muu);

/* thread.c */
extern int miteThreadBegin(miteThreadInfo **mttP, miteRenderInfo *mrr,
			   miteUserInfo *muu, int whichThread);
extern int miteThreadEnd(miteThreadInfo *mtt, miteRenderInfo *mrr,
			 miteUserInfo *muu);

/* ray.c */
extern int miteRayBegin(miteThreadInfo *mtt, miteRenderInfo *mrr,
			miteUserInfo *muu,
			int uIndex, int vIndex, 
			double rayLen,
			double rayStartWorld[3], double rayStartIndex[3],
			double rayDirWorld[3], double rayDirIndex[3]);
extern double miteSample(miteThreadInfo *mtt, miteRenderInfo *mrr,
			 miteUserInfo *muu,
			 int num, double rayT, int inside,
			 double samplePosWorld[3],
			 double samplePosIndex[3]);
extern int miteRayEnd(miteThreadInfo *mtt, miteRenderInfo *mrr,
		      miteUserInfo *muu);

#ifdef __cplusplus
}
#endif

#endif /* MITE_HAS_BEEN_INCLUDED */
