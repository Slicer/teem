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

#include <stdio.h>
#include <biff.h>
#include <air.h>
#include <lineal.h>
#include <math.h>

#define LIMN "limn"

#define LIMN_MAXLIT 10

/*
******** struct limnLighting
**
** information for directional lighting and the ambient light
**
** Has no dynamically allocated information or pointers
*/
typedef struct {
  float ambR, ambG, ambB, /* ambient light color */
    _dir[LIMN_MAXLIT][3], /* direction of light[i] (only world) 
			     This is calculated/copied from dir[] */
    dir[LIMN_MAXLIT][3],  /* direction of light[i] (either view or world)
			     This is what the user sets */
    col[LIMN_MAXLIT][3];  /* RGB color of light[i] */
  int numLights,          /* number of lights for which information is set
			     (not all of which are necessarily "on") */
    on[LIMN_MAXLIT],      /* light[i] is on */
    vsp[LIMN_MAXLIT];     /* light[i] lives in view space */
} limnLight;

/*
****** struct limnCam
**
** for all standard graphics camera parameters
**
** Has no dynamically allocated information or pointers
*/
typedef struct {
  float from[3],     /* location of eyepoint */
    at[3],           /* what eye is looking at */
    up[3],           /* what is up direction for eye */
    uMin, uMax,      /* range of U values to put on horiz. image axis */
    vMin, vMax,      /* range of V values to put on vert. image axis */
    near, far,       /* near and far clipping plane distances */
    dist;            /* distance to view plane */
  int eyeRel;        /* if true: near, far, and dist quantities
			measure distance from the eyepoint towards the
			at point.  if false: near, far, and dist
			quantities measure distance from the _at_
			point, but with the same sense (sign) as above */

  float uvn[9],      /* not usually user-set: the world to view transform */
    eNear, eFar,     /* not usually user-set: near and far dist from eye */
    eDist;           /* not usually user-set: view plane dist from eye */
} limnCam;

/*
******** struct limnPoint
**
** all the information you might want for a point
**
** Has no dynamically allocated information or pointers
*/
typedef struct {
  float w[4],        /* world coordinates (homogeneous) */
    v[3],            /* view coordinates */
    s[3],            /* screen coordinates */
    n[3];            /* vertex normal (world coords only) */
} limnPoint;

/*
******** struct limnFace
**
** all the information about a face
**
** Has no dynamically allocated information or pointers
*/
typedef struct {
  float rgba[4],     /* color of this face */
    wn[3],           /* normal in world space */
    sn[3];           /* normal in screen space (post-perspective-divide) */
  int sides,         /* number of sides (vertices) */
    vidx;            /* index of 1st vertex in parent obj's big vertex list */
} limnFace;

/*
******** struct limnObj
**
** the beast used to represent objects
**
** Relies on many dynamically allocated arrays
*/
typedef struct {
  float ka, kd, ks,  /* amounts of ambient, diffuse, shading desired */
    shine;           /* shininess exponent */
  
  limnPoint *p;      /* array of point structs (not pointers) */
  int numP;          /* # of points, length of "p" */
  airArray *pArr;    /* airArray around "p" */

  int *v;            /* big 1-D array of vertex indices for all faces */
  int numV;          /* total # of verts across all faces, length of "v" */
  airArray *vArr;    /* airArray around "v" */

  limnFace *f;       /* array of face structs (not pointers) */
  int numF;          /* # of faces, length of "f" */
  airArray *fArr;    /* airArray around "f" */
} limnObj;

/* methods.c */
extern limnCam *limnNewCam();
extern void limnNixCam(limnCam *cam);
extern limnLight *limnNewLight();
extern void limnNixLight(limnLight *lit);

/* obj.c */
extern limnObj *limnNewObj();
extern int limnNixObj(limnObj *obj);
extern int limnNewPoint(limnObj *obj, float x, float y, float z);
extern int limnNewFace(limnObj *obj, int *vert, int numVert, float rgba[4]);
extern int limnObjMerge(limnObj *o1, limnObj *o2);
extern limnObj *limnObjCopy(limnObj *o);

/* light.c */
/* view space coordinates are left handed: 
   +X points to the right, +Y upwards, and +Z away from you */
extern int limnUpdateLights(limnLight *lit, limnCam *cam);
extern int limnSetLight(limnLight *lit, int vsp,
			float r, float g, float b,
			float x, float y, float z);
extern int limnSetAmbient(limnLight *lit, float r, float g, float b);
extern int limnTurnLight(limnLight *lit, int idx, int on);

/* cam.c */
extern int limnSetUVN(limnCam *cam);

/* io.c */
extern int limnWriteAsOBJ(FILE *file, limnObj *obj);

/* qn.c */
extern void limn16QNtoV(float *vec, unsigned short qn, int doNorm);
extern unsigned short limnVto16QN(float *vec);
extern void limn16QN1PBtoV(float *vec, unsigned short qn, int doNorm);
extern unsigned short limnVto16QN1PB(float *vec);
extern void limn15QNtoV(float *vec, unsigned short qn, int doNorm);
extern unsigned short limnVto15QN(float *vec);

/* shapes.c */
extern limnObj *limnNewCube();
extern limnObj *limnNewSphere(int thetaRes, int phiRes);
extern limnObj *limnNewCylinder(int res);
extern limnObj *limnNewAxes(int res, float radius);
extern limnObj *limnNewCone(int res);
extern limnObj *limnNewArrow(int res, float L, float r, float R, float hasp,
			     float pos[3], float vec[3]);

/* matx.c */
extern void limnMatxScale(float *m, float x, float y, float z);
extern void limnMatxTranslate(float *m, float x, float y, float z);
extern void limnMatxRotateX(float *m, float th);
extern void limnMatxRotateY(float *m, float th);
extern void limnMatxRotateZ(float *m, float th);
extern void limnMatxRotateUtoV(float *m, float *u, float *v);
extern int limnMatxApply(limnObj *o, float *m);
extern void limnMatxMult(float *m, float *_n);
extern int limnNormHC(limnObj *o);
extern void limnMatx9to16(float s[16], float n[9]);
extern void limnMatx16to9(float n[9], float s[16]);
extern void limnMatx9Print(FILE *f, float n[9]);
extern void limnMatx16Print(FILE *f, float s[16]);

#ifdef __cplusplus
}
#endif
#endif /* LIMN_HAS_BEEN_INCLUDED */
