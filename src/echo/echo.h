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

#if 0
typedef float echoPos_t;
#define echoPos_nrrdType nrrdTypeFloat
#define POS_MAX FLT_MAX
#else 
typedef double echoPos_t;
#define echoPos_nrrdType nrrdTypeDouble
#define POS_MAX DBL_MAX
#endif

#if 0
typedef float echoCol_t;
#define echoCol_nrrdType nrrdTypeFloat
#else
typedef double echoCol_t;
#define echoCol_nrrdType nrrdTypeDouble
#endif

#define ECHO_AABBOX_OBJECT_MAX 8
#define ECHO_LIST_OBJECT_INCR 8
#define ECHO_IMG_CHANNELS 5
#define ECHO_EPSILON 0.000001

typedef struct {
  /* ray-tracing parameters */
  int verbose,         /* verbosity level */
    jitter,            /* what kind of jittering to do */
    shadow,            /* do shadowing */
    samples,           /* # samples per pixel */
    imgResU, imgResV,  /* horizontal and vertical image resolution */
    recDepth,          /* max recursion depth */
    reuseJitter,       /* don't recompute jitter offsets per pixel */
    permuteJitter,     /* properly permute the various jitter arrays */
    renderLights;      /* render the area lights */
  float aperture,      /* shallowness of field */
    timeGamma,         /* gamma for values in time image */
    refDistance;       /* reference distance for 1/(r*r)'ing area lights */
  echoCol_t
    amR, amG, amB;     /* ambient light color */

  /* RGB image generation parameters */
  echoCol_t
    bgR, bgG, bgB;     /* background color */
  float gamma;         /* display device gamma */
} EchoParam;

typedef struct {
  double time;         /* time to render image */
} EchoGlobalState;

typedef struct {
  Nrrd *nperm,         /* ECHO_JITTER_NUM permutations (length param->samples)
			  used to order jittering */
    *njitt;            /* 2 x ECHO_JITTER_NUM x param->samples values of 
			  type echoPos_t in [-1/2,1/2] */
  int *permBuff;       /* temp array for creating permutations */
  echoCol_t *chanBuff; /* for storing individual sample colors */
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

enum {
  echoMatterR,          /* 0 */
  echoMatterG,          /* 1 */
  echoMatterB,          /* 2 */
  echoMatterKa,         /* 3 */
};
enum {
  echoMatterPhongKd      = 4,
  echoMatterPhongKs,    /* 5 */
  echoMatterPhongSh,    /* 6 */
  echoMatterPhongAlpha  /* 7 */
};
enum {
  echoMatterGlassFuzzy   = 4,
  echoMatterGlassIndex  /* 5 */
};
enum {
  echoMatterMetalFuzzy   = 4
};

#define ECHO_MATTER_VALUE_NUM 8

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
  echoObjectInstance,   /* 9 */
  echoObjectLast
};
#define ECHO_OBJECT_MAX    9

/* function: me, k, intx --> lit color */

#define ECHO_OBJECT_COMMON              \
  int type
#define ECHO_OBJECT_MATTER              \
  int matter;                           \
  echoCol_t mat[ECHO_MATTER_VALUE_NUM]

typedef struct {
  ECHO_OBJECT_COMMON;
  ECHO_OBJECT_MATTER;   /* ha! its not actually in every object, but in
			   those cases were we want to access it without
			   knowing object type, then it will be there ... */
} EchoObject;

typedef struct {
  ECHO_OBJECT_COMMON;
  ECHO_OBJECT_MATTER;
  echoPos_t pos[3];
  echoPos_t rad;
} EchoObjectSphere;

typedef struct {
  ECHO_OBJECT_COMMON;
  ECHO_OBJECT_MATTER;
  /* ??? */
} EchoObjectCube;

typedef struct {
  ECHO_OBJECT_COMMON;
  ECHO_OBJECT_MATTER;
  echoPos_t vert[3][3];  /* e0 = vert[1]-vert[0],
			    e1 = vert[2]-vert[0],
			    normal = e0 x e1 */
} EchoObjectTriangle;

typedef struct {
  ECHO_OBJECT_COMMON;
  ECHO_OBJECT_MATTER;
  echoPos_t origin[3], edge0[3], edge1[3];
} EchoObjectRectangle;

typedef struct {
  ECHO_OBJECT_COMMON;
  ECHO_OBJECT_MATTER;
  /* ??? */
} EchoObjectTriMesh;

typedef struct {
  ECHO_OBJECT_COMMON;
  ECHO_OBJECT_MATTER;
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

typedef struct {
  ECHO_OBJECT_COMMON;
  echoPos_t matx[16];
  int own;
  EchoObject *obj;
} EchoObjectInstance;  

extern EchoObject *echoObjectNew(int type);
extern EchoObject *echoObjectNix(EchoObject *obj);
extern int echoObjectIsContainer(EchoObject *obj);
extern void echoObjectListAdd(EchoObject *parent, EchoObject *child);
extern void echoObjectSphereSet(EchoObject *sphere,
				echoPos_t x, echoPos_t y,
				echoPos_t z, echoPos_t rad);
extern void echoObjectRectangleSet(EchoObject *_rect,
				   echoPos_t ogx, echoPos_t ogy, echoPos_t ogz,
				   echoPos_t x0, echoPos_t y0, echoPos_t z0,
				   echoPos_t x1, echoPos_t y1, echoPos_t z1);
extern void echoObjectTriangleSet(EchoObject *_tri,
				  echoPos_t x0, echoPos_t y0, echoPos_t z0, 
				  echoPos_t x1, echoPos_t y1, echoPos_t z1, 
				  echoPos_t x2, echoPos_t y2, echoPos_t z2);

/* light.c ---------------------------------------- */

enum {
  echoLightUnknown,
  echoLightDirectional, /* 1 */
  echoLightArea,        /* 2 */
  echoLightLast
};
#define ECHO_LIGHT_MAX     2

#define ECHO_LIGHT_COMMON \
  int type                \

typedef struct {
  ECHO_LIGHT_COMMON;
} EchoLight;

typedef struct {
  ECHO_LIGHT_COMMON;
  echoCol_t col[3];
  echoPos_t dir[3];          /* normalized by echoLightDirectionalSet */
} EchoLightDirectional;

typedef struct {
  ECHO_LIGHT_COMMON;
  EchoObject *obj;
  /* ??? */
} EchoLightArea;

extern EchoLight *echoLightNew(int type);
extern EchoLight *echoLightNix(EchoLight *light);
extern airArray *echoLightArrayNew();
extern void echoLightArrayAdd(airArray *lightArr, EchoLight *light);
extern airArray *echoLightArrayNix(airArray *lightArr);
extern void echoLightDirectionalSet(EchoLight *light,
				    echoCol_t r, echoCol_t g, echoCol_t b,
				    echoPos_t x, echoPos_t y, echoPos_t z);
extern void echoLightAreaSet(EchoLight *light, EchoObject *obj);

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
  echoPos_t from[3],    /* ray comes from this point */
    dir[3],             /* ray goes in this (not normalized) direction */
    near, far;          /* look for intx in this interval */
  int depth,            /* recursion depth */
    shadow;             /* this is a shadow ray */
} EchoRay;

typedef struct {
  EchoObject *obj;      /* computed with every intersection */
  echoPos_t t,          /* computed with every intersection */
    u, v;               /* sometimes needed for texturing */
  echoPos_t norm[3],    /* computed with every intersection */
    view[3],            /* always used with coloring */
    pos[3];             /* always used with coloring (and perhaps texturing) */
  /* ??? extra information for where in tri mesh it hit? */
} EchoIntx;

extern int echoComposite(Nrrd *nimg, Nrrd *nraw, EchoParam *param);
extern int echoPPM(Nrrd *nppm, Nrrd *nimg, EchoParam *param);
extern int echoThreadStateInit(EchoThreadState *tstate,
			       EchoParam *param, EchoGlobalState *gstate);
extern void echoJitterSet(EchoParam *param, EchoThreadState *state);
extern int echoRender(Nrrd *nraw, limnCam *cam,
		      EchoParam *param, EchoGlobalState *gstate,
		      EchoObject *scene, airArray *lightArr);

/* intx.c ------------------------------------------- */
extern void echoRayColor(echoCol_t *chan, int samp, EchoRay *ray,
			 EchoParam *param, EchoThreadState *tstate,
			 EchoObject *scene, airArray *lightArr);

/* color.c ------------------------------------------ */
extern void echoMatterPhongSet(EchoObject *obj,
			       echoCol_t r, echoCol_t g, echoCol_t b,
			       echoCol_t a, echoCol_t ka, echoCol_t kd,
			       echoCol_t ks, echoCol_t sh);
extern void echoMatterGlassSet(EchoObject *obj,
			       echoCol_t r, echoCol_t g, echoCol_t b,
			       echoCol_t fuzzy, echoCol_t index, echoCol_t ka);
extern void echoMatterMetalSet(EchoObject *obj,
			       echoCol_t r, echoCol_t g, echoCol_t b,
			       echoCol_t fuzzy, echoCol_t ka);
extern void echoMatterLightSet(EchoObject *obj,
			       echoCol_t r, echoCol_t g, echoCol_t b);


#endif /* ECHO_HAS_BEEN_INCLUDED */

