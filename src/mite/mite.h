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
******** miteUserInfo struct
**
** all the input parameters for mite specified by the user, as well as
** a mop for cleaning up memory used during rendering
*/
typedef struct {
  Nrrd *nin, *ntf;
  double refStep, rayStep, near1;
  NrrdKernelSpec *ksp00, *ksp11, *ksp22;
  hoovContext *ctx;
  limnLight *lit;
  int renorm, sum;
  char *outS;

  airArray *mop;
} miteUserInfo;

/*
******** miteRenderInfo
**
** non-thread-specific state relevant for mite's internal use
*/
typedef struct {
  gageContext *gtx;
  gageSclAnswer *san;
  double time0, time1;
  int sx, sy;

  Nrrd *nout;
  float *imgData;
} miteRenderInfo;

/*
******** miteThreadInfo
**
** thread-specific state for mite's internal use
*/
typedef struct {
  double R, G, B, A;
  int thrid, /* thread ID */
    ui, vi;  /* image coords */
} miteThreadInfo;

/* defaultsMite.c */
extern mite_export double miteDefRefStep;
extern mite_export int miteDefRenorm;
extern mite_export double miteDefNear1;

/* user.c */
extern miteUserInfo *miteUserInfoNew(hoovContext *ctx);
extern miteUserInfo *miteUserInfoNix(miteUserInfo *muu);

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
extern int miteRayEnd(miteThreadInfo *mtt, miteRenderInfo *mrr,
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
