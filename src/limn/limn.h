/*
  The contents of this file are subject to the University of Utah Public
  License (the "License"); you may not use this file except in
  compliance with the License.
  
  Software distributed under the License is distributed on an "AS IS"
  basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See
  the License for the specific language governing rights and limitations
  under the License.

  The Original Source Code is "teem", released March 23, 2001.
  
  The Original Source Code was developed by the University of Utah.
  Portions created by UNIVERSITY are Copyright (C) 2001, 1998 University
  of Utah. All Rights Reserved.
*/


#ifndef LIMN_HAS_BEEN_INCLUDED
#define LIMN_HAS_BEEN_INCLUDED
#ifdef __cplusplus
extern "C" {
#endif

#define LIMN "limn"

#include <stdlib.h>

#include <math.h>
#include <air.h>
#include <biff.h>
#include <ell.h>
#include <nrrd.h>

#define LIMN_MAXLIT 20

/*
******** limnQN enum
**
** the different quantized normal schemes currently supported
*/
typedef enum {
  limnQNUnknown,
  limnQN16,
  limnQN16PB1,
  limnQN15,
  limnQNLast
} limnQN;

/*
****** struct limnCam
**
** for all standard graphics camera parameters
**
** Has no dynamically allocated information or pointers
*/
typedef struct limnCam_t {
  float from[3],     /* location of eyepoint */
    at[3],           /* what eye is looking at */
    up[3],           /* what is up direction for eye */
    uMin, uMax,      /* range of U values to put on horiz. image axis */
    vMin, vMax,      /* range of V values to put on vert. image axis */
    near, far,       /* near and far clipping plane distances */
    dist;            /* distance to view plane */
  int eyeRel,        /* if true: near, far, and dist quantities
			measure distance from the eyepoint towards the
			at point.  if false: near, far, and dist
			quantities measure distance from the _at_
			point, but with the same sense (sign) as above */
    leftHanded;
  
  float W2V[16],     /* not usually user-set: the world to view transform */
    vspNear, vspFar,
    vspDist;         /* not usually user-set: near and far dist (view space) */
} limnCam;

typedef enum {
  limnDeviceUnknown,
  limnDevicePS,
  limnDeviceOpenGL,
  limnDeviceLast
} limnDevice;

typedef struct {
  float edgeWidth[5],
    creaseAngle,
    bgGray;
} limnOptsPS;

typedef struct limnWin_t {
  limnOptsPS ps;
  int device;
  float scale, bbox[4];
  int yFlip;
  FILE *file;
} limnWin;

/*
******** struct limnLight
**
** information for directional lighting and the ambient light
**
** Has no dynamically allocated information or pointers
*/
typedef struct {
  float amb[3],           /* RGB ambient light color */
    _dir[LIMN_MAXLIT][3], /* direction of light[i] (only world space) 
			     This is calculated/copied from dir[] */
    dir[LIMN_MAXLIT][3],  /* direction of light[i] (either view or world space)
			     This is what the user sets */
    col[LIMN_MAXLIT][3];  /* RGB color of light[i] */
  int lNum,               /* number of lights for which information is set
			     (not all of which are necessarily "on") */
    on[LIMN_MAXLIT],      /* light[i] is on */
    vsp[LIMN_MAXLIT];     /* light[i] lives in view space */
} limnLight;

typedef enum {
  limnSpaceUnknown,
  limnSpaceWorld,
  limnSpaceView,
  limnSpaceScreen,
  limnSpaceDevice,
  limnSpaceLast
} limnSpace;

/*
******** struct limnPoint
**
** all the information you might want for a point
**
** Has no dynamically allocated information or pointers
*/
typedef struct limnPoint_t {
  float w[4],        /* world coordinates (homogeneous) */
    v[4],            /* view coordinates */
    s[3],            /* screen coordinates (device independant) */
    d[2],            /* device coordinates */
    n[3];            /* vertex normal (world coords only) */
  int sp;            /* index into parent's SP list */
} limnPoint;

/*
******** struct limnEdge
**
** all the information about an edge
**
** Has no dynamically allocated information or pointers
*/
typedef struct limnEdge_t {
  int v0, v1,        /* two point indices (in parent's point list) */
    f0, f1,          /* two face indices (in parent's face list) */
    sp;              /* index into parent's SP list */
  
  int visib;         /* is edge currently visible (or active) */
} limnEdge;

/*
******** struct limnFace
**
** all the information about a face
**
** Has no dynamically allocated information or pointers
*/
typedef struct limnFace_t {
  float wn[3],       /* normal in world space */
    sn[3];           /* normal in screen space (post-perspective-divide) */
  int vBase, vNum,   /* start and length in parent's vertex array, "v" */
    sp;              /* index into parent's SP list */
  
  int visib;         /* is face currently visible */
  float z;           /* for depth ordering */
} limnFace;

/*
******** struct limnPart
**
** one connected part of an object
*/
typedef struct limnPart_t {
  int fNum, fBase,   /* start and length in parent's limnFace array, "f" */
    eNum, eBase,     /* start and length in parent's limnEdge array, "e" */
    pNum, pBase,     /* start and length in parent's limnPoint array, "p" */
    origIdx;         /* initial part index of this part */

  float z;           /* assuming that the occlusion graph between parts is
			acyclic, one depth value is good enough for
			painter's algorithm ordering of drawing */
  unsigned char rgba[4];
} limnPart;

/*
******** struct limnSP
**
** "surface" properties: pretty much anything having to do with 
** appearance, for points, edges, faces, etc.
*/
typedef struct limnSP_t {
  float rgba[4], thick;
  float k[3], spec;
} limnSP;

/*
******** struct limnObj
**
** the beast used to represent polygonal objects
**
** Relies on many dynamically allocated arrays
*/
typedef struct limnObj_t {
  limnPoint *p;      /* array of point structs */
  airArray *pA;      /* airArray around "p" */

  int *v;            /* array of vertex indices for all faces */
  airArray *vA;      /* airArray around "v" */

  limnEdge *e;       /* array of edge structs */
  airArray *eA;      /* airArray around "e" */

  limnFace *f;       /* array of face structs */
  airArray *fA;      /* airArray around "f" */
  
  limnPart *r;       /* array of part structs */
  airArray *rA;      /* arrArray around "r" */

  limnSP *s;         /* array of surface properties */
  airArray *sA;      /* airArray around "s" */

  limnPart *rCurr;   /* pointer to part under construction */

  int edges;         /* if non-zero, build edges as faces are added */
} limnObj;

/* qn.c */
extern float *limnQN16toV(float *vec, unsigned short qn, 
			  int zeroZero, int doNorm);
extern unsigned short limnQNVto16(float *vec, int zeroZero);
extern float *limnQN16PB1toV(float *vec, unsigned short qn, int doNorm);
extern unsigned short limnQNVto16PB1(float *vec);
extern float *limnQN15toV(float *vec, unsigned short qn, int doNorm);
extern unsigned short limnQNVto15(float *vec);

/* light.c */
typedef void (*limnEnvMapCB)(float rgb[3], float vec[3], void *data);
extern int limnEnvMapFill(Nrrd *envMap, limnEnvMapCB cb, 
			  void *data, int qnMethod);
extern int limnLightSet(limnLight *lit, int vsp,
			float r, float g, float b,
			float x, float y, float z);
extern int limnLightSetAmbient(limnLight *lit, float r, float g, float b);
extern int limnLightToggle(limnLight *lit, int idx, int on);
extern void limnLightDiffuseCB(float rgb[3], float vec[3], void *_lit);
extern int limnLightUpdate(limnLight *lit, limnCam *cam);

/* methods.c */
extern limnLight *limnLightNew(void);
extern limnLight *limnLightNix(limnLight *);
extern limnCam *limnCamNew(void);
extern limnCam *limnCamNix(limnCam *cam);
extern limnWin *limnWinNew(int device);
extern limnWin *limnWinNix(limnWin *win);

/* cam.c */
extern int limnCamUpdate(limnCam *cam);

/* obj.c */
extern limnObj *limnObjNew(int incr, int edges);
extern limnObj *limnObjNuke(limnObj *obj);
extern int limnObjPointAdd(limnObj *obj, int sp, float x, float y, float z);
extern int limnObjFaceAdd(limnObj *obj, int sp, int numVert, int *vert);
extern int limnObjSPAdd(limnObj *obj);
extern int limnObjPartStart(limnObj *obj, int sp);
extern int limnObjPartFinish(limnObj *obj);

/* io.c */
extern int limnObjDescribe(FILE *file, limnObj *obj);

/* shapes.c */
extern int limnObjCubeAdd(limnObj *obj, int sp);
extern int limnObjCylinderAdd(limnObj *obj, int sp, int res);
extern int limnObjPolarSphereAdd(limnObj *obj, int sp, 
				 int thetaRes, int phiRes);
extern int limnObjConeAdd(limnObj *obj, int sp, int res);

/* transform.c */
extern int limnObjHomog(limnObj *obj, int space);
extern int limnObjNormals(limnObj *obj, int space);
extern int limnObjSpaceTransform(limnObj *obj, limnCam *cam, limnWin *win,
				 int space);
extern int limnObjPartTransform(limnObj *obj, int ri, float tx[16]);

/* render.c */
extern int limnObjPSRender(limnObj *obj, limnCam *cam, 
			   Nrrd *envMap, limnWin *win);


#ifdef __cplusplus
}
#endif
#endif /* LIMN_HAS_BEEN_INCLUDED */
