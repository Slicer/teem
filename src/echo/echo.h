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

#include <stdio.h>
#include <math.h>

#include <air.h>
#include <biff.h>
#include <ell.h>
#include <nrrd.h>
#include <limn.h>

#if defined(WIN32) && !defined(TEEM_BUILD)
#define echo_export __declspec(dllimport)
#else
#define echo_export
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define ECHO "echo"

/* all position and transform information is kept as ...
** 1: floats
** 0: doubles
*/
#if 1
typedef float echoPos_t;
#define echoPos_nt nrrdTypeFloat
#define echoPos_at airTypeFloat
#define ell4mInvert_p ell4mInvert_f
#define ell4mPrint_p ell4mPrint_f
#define ell4mDet_p ell4mDet_f
#define ECHO_POS_MIN (-FLT_MAX)
#define ECHO_POS_MAX FLT_MAX
#else 
typedef double echoPos_t;
#define echoPos_nt nrrdTypeDouble
#define echoPos_at airTypeDouble
#define ell4mInvert_p ell4mInvert_d
#define ell4mPrint_p ell4mPrint_d
#define ell4mDet_p ell4mDet_d
#define ECHO_POS_MIN (-DBL_MAX)
#define ECHO_POS_MAX DBL_MAX
#endif

/* all color information is kept as 
** 1: floats
** 0: doubles
*/
#if 1
typedef float echoCol_t;
#define echoCol_nt nrrdTypeFloat
#else
typedef double echoCol_t;
#define echoCol_nt nrrdTypeDouble
#endif

#define ECHO_AABBOX_OBJECT_MAX 8
#define ECHO_LIST_OBJECT_INCR 8
#define ECHO_IMG_CHANNELS 5
#define ECHO_EPSILON 0.001        /* used for adjusting ray positions */
#define ECHO_NEAR0 0.004          /* used for comparing transparency to zero */
#define ECHO_LEN_SMALL_ENOUGH 16  /* to control splitting for split objects */

typedef struct {
  /* ray-tracing parmeters */
  int jitter,          /* what kind of jittering to do */
    shadow,            /* do shadowing */
    samples,           /* # samples per pixel */
    imgResU, imgResV,  /* horizontal and vertical image resolution */
    maxRecDepth,       /* max recursion depth */
    reuseJitter,       /* don't recompute jitter offsets per pixel */
    permuteJitter,     /* properly permute the various jitter arrays */
    renderLights,      /* render the area lights */
    renderBoxes,       
    seedRand;          /* call airSrand() (don't if repeatability wanted) */
  float aperture,      /* shallowness of field */
    timeGamma,         /* gamma for values in time image */
    refDistance;       /* reference distance for 1/(r*r)'ing area lights */
  echoCol_t
    mrR, mrG, mrB,     /* color used when max recursion depth is met */
    amR, amG, amB;     /* ambient light color */

  /* RGB image generation parameters */
  echoCol_t
    bgR, bgG, bgB;     /* background color */
  float gamma;         /* display device gamma */
} EchoRTParm;

typedef struct {
  double time;         /* time to render image */
} EchoGlobalState;

typedef struct {
  Nrrd *nperm,         /* ECHO_JITTER_NUM permutations (length parm->samples)
			  used to order jittering */
    *njitt;            /* 2 x ECHO_JITTER_NUM x parm->samples values of 
			  type echoPos_t in [-1/2,1/2] */
  int *permBuff;       /* temp array for creating permutations */
  echoCol_t *chanBuff; /* for storing individual sample colors */
} EchoThreadState;

/*
******** echoJitter* enum
** 
** the different jitter patterns that are supported.  This setting is
** global- you can't have different jitter patterns on the lights versus
** the pixels.
*/
enum {
  echoJitterUnknown,
  echoJitterNone,       /* 1: N samples all at the square center */
  echoJitterGrid,       /* 2: N samples exactly on a sqrt(N) x sqrt(N) grid */
  echoJitterJitter,     /* 3: N jittered samples on a sqrt(N) x sqrt(N) grid */
  echoJitterRandom,     /* 4: N samples randomly placed in square */
  echoJitterLast
};
#define ECHO_JITTER_MAX    4

/*
******** echoSample* enum
**
** the different quantities to which the jitter two-vector may be
** applied.  
*/
enum {
  echoSampleUnknown = -1,
  echoSamplePixel,      /* 0 */
  echoSampleLight,      /* 1 */
  echoSampleLens,       /* 2 */
  echoSampleNormalA,    /* 3 */
  echoSampleNormalB,    /* 4 */
  echoSampleMotionA,    /* 5 */
  echoSampleMotionB,    /* 6 */
  echoSampleLast
};
#define ECHO_SAMPLE_NUM    6

/*
******** echoMatter* enum
**
** the different materials that are supported.  This setting determines
** the interpretation of the vector of floats/doubles that constitutes
** the material information.  The Light material is only supported on
** rectangles.
*/
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
  echoMatterPhongKa,    /* 0 */
  echoMatterPhongKd,    /* 1 */
  echoMatterPhongKs,    /* 2 */
  echoMatterPhongSh,    /* 3 */
};
enum {
  echoMatterGlassIndex, /* 0 */
  echoMatterGlassKd,    /* 1 */
  echoMatterGlassFuzzy  /* 2 */
};
enum {
  echoMatterMetalR0,    /* 0 */
  echoMatterMetalKd,    /* 1 */
  echoMatterMetalFuzzy  /* 2 */
};
enum {
  echoMatterLightPower
};

#define ECHO_MATTER_VALUE_NUM 4

/* enumsEcho.c ------------------------------------------ */
extern echo_export airEnum *echoJitter_ae;
extern echo_export airEnum *echoObject_ae;

/* object.c ---------------------------------------- */

/* the enum values start with lower case e, the EchoObjects start with E,
   the airEnum ends with _ae */

enum {
  echoObjectUnknown,
  echoSphere,     /*  1 */
  echoCube,       /*  2 */
  echoTriangle,   /*  3 */
  echoRectangle,  /*  4 */
  echoTriMesh,    /*  5: only triangles in the mesh */
  echoIsosurface, /*  6 */
  echoAABBox,     /*  7 */
  echoSplit,      /*  8 */
  echoList,       /*  9 */
  echoInstance,   /* 10 */
  echoObjectLast
};

#define ECHO_OBJECT_MAX    10

/* function: me, k, intx --> lit color */

#define ECHO_OBJECT_COMMON              \
  unsigned char type

#define ECHO_OBJECT_MATTER              \
  unsigned char matter;                 \
  echoCol_t rgba[4];                    \
  echoCol_t mat[ECHO_MATTER_VALUE_NUM]; \
  Nrrd *ntext

typedef struct {
  ECHO_OBJECT_COMMON;
  ECHO_OBJECT_MATTER;   /* ha! its not actually in every object, but in
			   those cases were we want to access it without
			   knowing object type, then it will be there ... */
} EchoObject;

typedef struct {
  ECHO_OBJECT_COMMON;
  ECHO_OBJECT_MATTER;
  echoPos_t pos[3], rad;
} EchoSphere;

typedef struct {
  ECHO_OBJECT_COMMON;
  ECHO_OBJECT_MATTER;
} EchoCube;

typedef struct {
  ECHO_OBJECT_COMMON;
  ECHO_OBJECT_MATTER;
  echoPos_t vert[3][3];  /* e0 = vert[1]-vert[0],
			    e1 = vert[2]-vert[0],
			    normal = e0 x e1 */
} EchoTriangle;

typedef struct {
  ECHO_OBJECT_COMMON;
  ECHO_OBJECT_MATTER;
  echoPos_t origin[3], edge0[3], edge1[3], area;
} EchoRectangle;

typedef struct {
  ECHO_OBJECT_COMMON;
  ECHO_OBJECT_MATTER;
  echoPos_t origin[3], min[3], max[3];
  int numV, numF;
  echoPos_t *pos;
  int *vert;
} EchoTriMesh;

typedef struct {
  ECHO_OBJECT_COMMON;
  ECHO_OBJECT_MATTER;
  Nrrd *volume;
  float value;
} EchoIsosurface;

typedef struct {
  ECHO_OBJECT_COMMON;
  EchoObject **obj;
  airArray *objArr;
  echoPos_t min[3], max[3];
} EchoAABBox;

typedef struct {
  ECHO_OBJECT_COMMON;
  int axis;                   /* which axis was split */
  echoPos_t min0[3], max0[3],
    min1[3], max1[3];         /* bboxes of two children */
  EchoObject *obj0, *obj1;    /* two splits, or two aabboxes */
} EchoSplit;

typedef struct {
  ECHO_OBJECT_COMMON;
  EchoObject **obj;         /* it is important that this match the
			       ordering in the struct of the aabbox */
  airArray *objArr;
} EchoList;  

typedef struct {
  ECHO_OBJECT_COMMON;
  echoPos_t Mi[16], M[16];
  int own;
  EchoObject *obj;
} EchoInstance;

extern EchoObject *echoNew(unsigned char type);
extern EchoObject *echoNix(EchoObject *obj);
extern EchoObject *echoNuke(EchoObject *obj);
extern void echoBoundsGet(echoPos_t *lo, echoPos_t *hi, EchoObject *obj);
extern int echoIsContainer(EchoObject *obj);
extern void echoListAdd(EchoObject *parent, EchoObject *child);
extern EchoObject *echoListSplit(EchoObject *list, int axis);
extern EchoObject *echoListSplit3(EchoObject *list, int depth);
extern void echoSphereSet(EchoObject *sphere,
			  echoPos_t x, echoPos_t y,
			  echoPos_t z, echoPos_t rad);
extern void echoRectangleSet(EchoObject *rect,
			     echoPos_t ogx, echoPos_t ogy, echoPos_t ogz,
			     echoPos_t x0, echoPos_t y0, echoPos_t z0,
			     echoPos_t x1, echoPos_t y1, echoPos_t z1);
extern void echoTriangleSet(EchoObject *tri,
			    echoPos_t x0, echoPos_t y0, echoPos_t z0, 
			    echoPos_t x1, echoPos_t y1, echoPos_t z1, 
			    echoPos_t x2, echoPos_t y2, echoPos_t z2);
extern void echoTriMeshSet(EchoObject *trim,
			   int numV, echoPos_t *pos,
			   int numF, int *vert);
extern void echoInstanceSet(EchoObject *inst,
			    echoPos_t *M, EchoObject *obj, int own);
extern EchoObject *echoRoughSphere(int theRes, int phiRes,
				   echoPos_t *matx);

/* methodsEcho.c --------------------------------------- */
extern EchoRTParm *echoRTParmNew();
extern EchoRTParm *echoRTParmNix(EchoRTParm *parm);
extern EchoGlobalState *echoGlobalStateNew();
extern EchoGlobalState *echoGlobalStateNix(EchoGlobalState *state);
extern EchoThreadState *echoThreadStateNew();
extern EchoThreadState *echoThreadStateNix(EchoThreadState *state);
extern limnCam *echoLimnCamNew();   /* set some fields after limnCamNew() */

/* renderEcho.c ---------------------------------------- */

extern echo_export int echoVerbose;

typedef struct {
  echoPos_t from[3],    /* ray comes from this point */
    dir[3],             /* ray goes in this (not normalized) direction */
    neer, faar;         /* look for intx in this interval */
    int depth,          /* recursion depth */
    shadow;             /* this is a shadow ray */
  echoCol_t transp;     /* for shadow rays, the transparency so far; starts
			   at 1.0, goes down to 0.0 */
} EchoRay;

typedef struct {
  EchoObject *obj;      /* computed with every intersection */
  echoPos_t t,          /* computed with every intersection */
    u, v;               /* sometimes needed for texturing */
  echoPos_t norm[3],    /* computed with every intersection */
    view[3],            /* always used with coloring */
    pos[3];             /* always used with coloring (and perhaps texturing) */
  int depth,            /* the depth of the ray that generated this intx */
    face,
    boxhits;            /* how many bounding boxes we hit */
} EchoIntx;

extern int echoComposite(Nrrd *nimg, Nrrd *nraw, EchoRTParm *parm);
extern int echoPPM(Nrrd *nppm, Nrrd *nimg, EchoRTParm *parm);
extern int echoThreadStateInit(EchoThreadState *tstate,
			       EchoRTParm *parm, EchoGlobalState *gstate);
extern void echoJitterCompute(EchoRTParm *parm, EchoThreadState *state);
extern int echoRTRender(Nrrd *nraw, limnCam *cam,
			EchoRTParm *parm, EchoGlobalState *gstate,
			EchoObject *scene, airArray *lightArr);

/* intx.c ------------------------------------------- */
extern void echoRayColor(echoCol_t *chan, int samp, EchoRay *ray,
			 EchoRTParm *parm, EchoThreadState *tstate,
			 EchoObject *scene, airArray *lightArr);

/* color.c ------------------------------------------ */
extern void echoMatterColorSet(EchoObject *obj,
			       echoCol_t R, echoCol_t G, echoCol_t B,
			       echoCol_t A);
extern void echoMatterPhongSet(EchoObject *obj,
			       echoCol_t ka, echoCol_t kd,
			       echoCol_t ks, echoCol_t sh);
extern void echoMatterGlassSet(EchoObject *obj,
			       echoCol_t index, echoCol_t kd, echoCol_t fuzzy);
extern void echoMatterMetalSet(EchoObject *obj,
			       echoCol_t R0, echoCol_t kd, echoCol_t fuzzy);
extern void echoMatterLightSet(EchoObject *obj,
			       echoCol_t power);
extern void echoMatterTextureSet(EchoObject *obj, Nrrd *ntext);

#ifdef __cplusplus
}
#endif

#endif /* ECHO_HAS_BEEN_INCLUDED */

