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

#ifndef HOOV_HAS_BEEN_INCLUDED
#define HOOV_HAS_BEEN_INCLUDED

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <air.h>
#include <biff.h>
#include <ell.h>
#include <limn.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
** On what is shorted to "hoov" and what stays "hoover":
** The library name stays libhoover.a
** The include file stays hoover.h
** Source files which require the library name as suffix use "hoover".
** The biff key is HOOVER --> "hoover"
** Everything else is shortened: prefix on:
** #defines (including #include guards), functions and global variables,
** struct type names, enum values, typedefs
*/

#define HOOVER "hoover"

#define HOOV_THREAD_MAX 128

/* 
******** the mess of typedefs for callbacks used below
*/
typedef int (hoovRenderBegin_t)(void **renderInfoP,
				void *userInfo);
typedef int (hoovThreadBegin_t)(void **threadInfoP,
				void *renderInfo,
				void *userInfo,
				int whichThread);
typedef int (hoovRayBegin_t)(void *threadInfo,
			     void *renderInfo,
			     void *userInfo,
			     int uIndex,    /* img coords of current ray */
			     int vIndex, 
			     double rayLen, /* length of ray segment between
					       near and far planes,  */
			     double rayStartWorld[3],
			     double rayStartIndex[3],
			     double rayDirWorld[3],
			     double rayDirIndex[3]);
typedef double (hoovSample_t)(void *threadInfo,
			      void *renderInfo,
			      void *userInfo,
			      int num,     /* which sample this is, 0-based */
			      double rayT, /* position along ray */
			      int inside,  /* sample is inside the volume */
			      double samplePosWorld[3],
			      double samplePosIndex[3]);
typedef int (hoovRayEnd_t)(void *threadInfo,
			   void *renderInfo,
			   void *userInfo);
typedef int (hoovThreadEnd_t)(void *threadInfo,
			      void *renderInfo,
			      void *userInfo);
typedef int (hoovRenderEnd_t)(void *rendInfo, void *userInfo);

/*
******** hoovContext struct
**
** Everything that hoovRender() needs to do its thing, and no more.
** This is all read-only informaiton.
** 1) camera information
** 3) volume information
** 4) image information
** 5) opaque "user information" pointer
** 6) the number of threads to spawn
** 7) the callbacks
**
** For the sake of some simplicity, the of both the volume is always
** assumed to be node-centered, and the image is always assumed to
** be cell-centered.
*/
typedef struct {

  /******** 1) camera information */
  limnCam *cam;            /* camera info */

  /******** 2) volume information: size and spacing, voxel vs. cell, scaling */
  int volSize[3];          /* X,Y,Z resolution of volume */
  double volSpacing[3];    /* distance between samples in X,Y,Z direction */
  
  /******** 3) image information: dimensions, pixels inside vs. on-edge */
  int imgSize[2];          /* # samples of image along U and V axes */
  
  /******** 4) opaque "user information" pointer */
  void *userInfo;          /* passed to all callbacks */

  /******** 5) the number of threads to spawn */
  int numThreads;          /* number of threads to spawn per rendering */
  
  /*
  ******* 6) the callbacks 
  **
  ** The conceptual ordering of these callbacks is as they are listed
  ** below.  For example, rayBegin and rayEnd are called multiple
  ** times between threadBegin and threadEnd, and so on.  All of these
  ** are initialized to one of the stub functions provided by hoover.  
  **
  ** A non-zero return of any of these indicates error. Which callback
  ** failed is represented by the return value of hoovRender(), the
  ** return value from the callback is stored in *errCodeP by
  ** hoovRender(), and the problem thread number is stored in
  ** *errThreadP.
  */

  /* 
  ** renderBegin()
  **
  ** called once at beginning of whole rendering, and
  ** *renderInfoP is passed to all following calls as "renderInfo".
  ** Any mechanisms for inter-thread communication go nicely in 
  ** the renderInfo.
  **
  ** int (*renderBegin)(void **renderInfoP, void *userInfo);
  */
  hoovRenderBegin_t *renderBegin;
  
  /* 
  ** threadBegin() 
  **
  ** called once per thread, and *threadInfoP is passed to all
  ** following calls as "threadInfo".
  **
  ** int (*threadBegin)(void **threadInfoP, void *renderInfo, void *userInfo,
  **                    int whichThread);
  */
  hoovThreadBegin_t *threadBegin;
  
  /*
  ** rayBegin()
  **
  ** called once at the beginning of each ray.  This function will be
  ** called regardless of whether the ray actually intersects the
  ** volume, but this will change in the future.
  **
  ** int (*rayBegin)(void *threadInfo, void *renderInfo, void *userInfo,
  **                 int uIndex, int vIndex, 
  **                 double rayLen,
  **                 double rayStartWorld[3], double rayStartIndex[3],
  **                 double rayDirWorld[3], double rayDirIndex[3]);
  */
  hoovRayBegin_t *rayBegin;

  /* 
  ** sample()
  **
  ** called once per sample along the ray, and the return value is
  ** used to indicate how far to increment the ray position for the
  ** next sample.  Negative values back you up.  A return of 0.0 is
  ** taken to mean a non-erroneous ray termination, a return of NaN is
  ** taken to mean an error condition.  It is the user's
  ** responsibility to store an error code or whatever they want
  ** somewhere accessible.
  **
  ** This is not a terribly flexible scheme (don't forget, this is
  ** hoover): it enforces rather rigid constraints on how
  ** multi-threading works: one thread can not render multiple rays
  ** simulatenously.  If there were more args to cbSample (like a
  ** rayInfo, or an integral rayIndex), then this would be possible,
  ** but it would mean that _hoovThreadBody() would have to implement
  ** all the smarts about which samples belong on which rays belong
  ** with which threads.
  **
  ** At some point now or in the future, an effort will be made to
  ** never call this function if the ray does not in fact intersect
  ** the volume at all.
  **
  ** double (*sample)(void *threadInfo, void *renderInfo, void *userInfo,
  **                  int num, double rayT, int inside,
  **                  double samplePosWorld[3],
  **                  double samplePosIndex[3]);
  */
  hoovSample_t *sample;

  /*
  ** rayEnd()
  ** 
  ** called at the end of the ray.  The end of a ray is:
  ** 1) sample returns 0.0, or,
  ** 2) when the sample location goes behind far plane
  **
  ** int (*rayEnd)(void *threadInfo, void *renderInfo, void *userInfo);
  */
  hoovRayEnd_t *rayEnd;

  /* 
  ** threadEnd()
  **
  ** called at end of thread
  **
  ** int (*threadEnd)(void *threadInfo, void *renderInfo, void *userInfo);
  */
  hoovThreadEnd_t *threadEnd;
  
  /* 
  ** renderEnd()
  ** 
  ** called once at end of whole rendering
  **
  ** int (*renderEnd)(void *rendInfo, void *userInfo);
  */
  hoovRenderEnd_t *renderEnd;

} hoovContext;

/*
******** hoovErr... enum
**
** possible returns from hoovRender.
** hoovErrNone: no error, all is well: 
** hoovErrInit: error detected in hoover, call biffGet(HOOVER)
** otherwise, return indicates which call-back had trouble
*/
enum {
  hoovErrNone,
  hoovErrInit,           /* call biffGet(HOOVER) */
  hoovErrRenderBegin,
  hoovErrThreadCreate,
  hoovErrThreadBegin,
  hoovErrRayBegin,
  hoovErrSample,
  hoovErrRayEnd,
  hoovErrThreadEnd,
  hoovErrThreadJoin,
  hoovErrRenderEnd,
  hoovErrLast
};
  
/* methodsHoover.c */
extern hoovContext *hoovContextNew();
extern int hoovContextCheck(hoovContext *ctx);
extern void hoovContextNix(hoovContext *ctx);

/* rays.c */
extern int hoovRender(hoovContext *ctx, int *errCodeP, int *errThreadP);

/* stub.c */
extern hoovRenderBegin_t hoovStubRenderBegin;
extern hoovThreadBegin_t hoovStubThreadBegin;
extern hoovRayBegin_t hoovStubRayBegin;
extern hoovSample_t hoovStubSample;
extern hoovRayEnd_t hoovStubRayEnd;
extern hoovThreadEnd_t hoovStubThreadEnd;
extern hoovRenderEnd_t hoovStubRenderEnd;

#ifdef __cplusplus
}
#endif

#endif /* HOOV_HAS_BEEN_INCLUDED */
