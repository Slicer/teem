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

#ifndef ECHO_HAS_BEEN_INCLUDED
#define ECHO_HAS_BEEN_INCLUDED

#define ECHO "echo"

#include <stdio.h>
#include <math.h>

#include <air.h>
#include <biff.h>
#include <ell.h>
#include <limn.h>
#include <nrrd.h>
#include <dye.h>

#if 1
typedef float echoPos_t;
#define echoPos_nrrdType nrrdTypeFloat
#define POS_MAX FLT_MAX
#else 
typedef double echoPos_t;
#define echoPos_nrrdType nrrdTypeDouble
#define POS_MAX DBL_MAX
#endif

#if 1
typedef float echoCol_t;
#define echoCol_nrrdType nrrdTypeFloat
#else
typedef double echoCol_t;
#define echoCol_nrrdType nrrdTypeDouble
#endif

#define ECHO_AABBOX_OBJECT_MAX 8
#define ECHO_LIST_OBJECT_INCR 8
#define ECHO_MATTER_VALUE_NUM 8

typedef struct {
  /* ray-tracing parameters */
  int verbose,         /* verbosity level */
    jitter,            /* what kind of jittering to do */
    samples,           /* # samples per pixel */
    imgResU, imgResV,  /* horizontal and vertical image resolution */
    recDepth,          /* max recursion depth */
    reuseJitter,       /* don't recompute jitter offsets per pixel */
    permuteJitter;     /* properly permute the various jitter arrays */
  float epsilon,       /* somewhat bigger than zero */
    aperture;          /* shallowness of field */

  /* RGB image generation parameters */
  echoCol_t
    bgR, bgG, bgB;     /* background color */
  float gamma;         /* display device gamma */
} EchoParam;

typedef struct {
  double time0, time1; /* start and end time for whole image rendering */
} EchoGlobalState;

typedef struct {
  Nrrd *nperm,         /* ECHO_JITTER_NUM permutations (length param->samples)
			  used to order jittering */
    *njitt;            /* 2 x ECHO_JITTER_NUM x param->samples values of 
			  type echoPos_t in [-1/2,1/2] */
  int *permBuff;       /* temp array for creating permutations */
} EchoThreadState;

enum {
  echoJitterUnknown,
  echoJitterNone,       /* 1: N samples all at the square center */
  echoJitterGrid,       /* 2: N samples exactly on a sqrt(N) x sqrt(n) grid */
  echoJitterJitter,     /* 3: N jittered samples on a sqrt(N) x sqrt(n) grid */
  echoJitterRandom,     /* 4: N samples randomly placed in square */
  echoJitterLast
};
#define ECHO_JITTER_MAX    4

enum {
  echoSampleUnknown = -1,
  echoSamplePixel,      /* 0 */
  echoSampleAreaLight,  /* 1 */
  echoSampleLens,       /* 2 */
  echoSampleNormal,     /* 3 */
  echoSampleLast
};
#define ECHO_SAMPLE_NUM    4

enum {
  echoMatterUnknown,
  echoMatterPhong,      /* 1 */
  echoMatterGlass,      /* 2 */
  echoMatterMetal,      /* 3 */
  echoMatterLight,      /* 4 */
  echoMatterLast
};
#define ECHO_MATTER_MAX    4

/* enum.c ------------------------------------------ */
extern airEnum echoJitter;
extern airEnum echoObject;

/* object.c ---------------------------------------- */

enum {
  echoObjectUnknown,
  echoObjectSphere,     /* 1 */
  echoObjectCube,       /* 2 */
  echoObjectTriangle,   /* 3 */
  echoObjectRectangle,  /* 4 */
  echoObjectTriMesh,    /* 5: only triangles in the mesh */
  echoObjectIsosurface, /* 6 */
  echoObjectAABBox,     /* 7 */
  echoObjectList,       /* 8 */
  echoObjectLast
};
#define ECHO_OBJECT_MAX    8

/* function: me, k, intx --> lit color */

#define ECHO_OBJECT_COMMON \
  int type
#define ECHO_OBJECT_MATERIAL \
  echoCol_t mat[ECHO_MATTER_VALUE_NUM]

typedef struct {
  ECHO_OBJECT_COMMON;
} EchoObject;

typedef struct {
  ECHO_OBJECT_COMMON;
  ECHO_OBJECT_MATERIAL;
  echoPos_t pos[3];
  echoPos_t rad;
} EchoObjectSphere;

typedef struct {
  ECHO_OBJECT_COMMON;
  ECHO_OBJECT_MATERIAL;
  /* ??? */
} EchoObjectCube;

typedef struct {
  ECHO_OBJECT_COMMON;
  ECHO_OBJECT_MATERIAL;
  echoPos_t vert[3][3];
} EchoObjectTriangle;

typedef struct {
  ECHO_OBJECT_COMMON;
  ECHO_OBJECT_MATERIAL;
  echoPos_t origin[3], edge0[3], edge1[3];
} EchoObjectRectangle;

typedef struct {
  ECHO_OBJECT_COMMON;
  ECHO_OBJECT_MATERIAL;
  /* ??? */
} EchoObjectTriMesh;

typedef struct {
  ECHO_OBJECT_COMMON;
  ECHO_OBJECT_MATERIAL;
  Nrrd *volume;
  float value;
} EchoObjectIsosurface;

typedef struct {
  ECHO_OBJECT_COMMON;
  echoPos_t min[3], max[3];
  int len;
  EchoObject *obj[ECHO_AABBOX_OBJECT_MAX];
} EchoObjectAABBox;

typedef struct {
  ECHO_OBJECT_COMMON;
  EchoObject **obj;
  airArray *objArr;
} EchoObjectList;  

extern EchoObject *echoObjectNew(int type);
extern EchoObject *echoObjectNix(EchoObject *obj);
extern int echoObjectIsContainer(EchoObject *obj);
extern void echoObjectAdd(EchoObject *parent, EchoObject *child);

/* light.c ---------------------------------------- */

enum {
  echoLightUnknown,
  echoLightAmbient,     /* 1 */
  echoLightDirectional, /* 2 */
  echoLightArea,        /* 3 */
  echoLightLast
};
#define ECHO_LIGHT_MAX     3

#define ECHO_LIGHT_COMMON \
  int type                \

typedef struct {
  ECHO_LIGHT_COMMON;
} EchoLight;

typedef struct {
  ECHO_LIGHT_COMMON;
  echoCol_t col[3];
} EchoLightAmbient;

typedef struct {
  ECHO_LIGHT_COMMON;
  echoCol_t col[3];
  echoPos_t dir[3];
} EchoLightDirectional;

typedef struct {
  ECHO_LIGHT_COMMON;
  EchoObject *obj;
  /* ??? */
} EchoLightArea;

extern EchoLight *echoLightNew(int type);
extern EchoLight *echoLightNix(EchoLight *light);

/* methods.c --------------------------------------- */
extern EchoParam *echoParamNew();
extern EchoParam *echoParamNix(EchoParam *param);
extern EchoGlobalState *echoGlobalStateNew();
extern EchoGlobalState *echoGlobalStateNix(EchoGlobalState *state);
extern EchoThreadState *echoThreadStateNew();
extern EchoThreadState *echoThreadStateNix(EchoThreadState *state);
extern limnCam *echoLimnCamNew();

/* render.c ---------------------------------------- */

typedef struct {
  echoPos_t pos[3];
  echoPos_t dir[3];
} EchoRay;

typedef struct {
  echoPos_t t;
  echoPos_t norm[3], pos[3];
  EchoObject *obj;
  /* ??? */
} EchoIntx;

extern int echoComposite(Nrrd *nimg, Nrrd *nraw, EchoParam *param);
extern int echoPPM(Nrrd *nppm, Nrrd *nimg, EchoParam *param);
extern int echoThreadStateInit(EchoThreadState *tstate,
			       EchoParam *param, EchoGlobalState *gstate);
extern void echoJitterSet(EchoParam *param, EchoThreadState *state);
extern int echoRender(Nrrd *nraw, limnCam *cam,
		      EchoParam *param, EchoGlobalState *state,
		      EchoObject *scene, airArray *lightArr);


#endif /* ECHO_HAS_BEEN_INCLUDED */
