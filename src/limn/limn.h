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

#ifndef LIMN_HAS_BEEN_INCLUDED
#define LIMN_HAS_BEEN_INCLUDED

#include <stdlib.h>

#include <math.h>
#include <air.h>
#include <biff.h>
#include <hest.h>
#include <ell.h>
#include <nrrd.h>

#if defined(_WIN32) && !defined(TEEM_BUILD)
#define limn_export __declspec(dllimport)
#else
#define limn_export
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define LIMN "limn"
#define LIMN_LITE_NUM 16

/*
****** limnCam struct
**
** for all standard graphics camera parameters.  Image plane is
** spanned by U and V; N always points away from the viewer.  V can
** point up or down, if the camera is left- or right-handed,
** respectively.
**
** Has no dynamically allocated information or pointers.
*/
typedef struct limnCam_t {
  double from[3],     /* location of eyepoint */
    at[3],            /* what eye is looking at */
    up[3],            /* what is up direction for eye (this is not updated
                         to the "true" up) */
    uRange[2],        /* range of U values to put on horiz. image axis */
    vRange[2],        /* range of V values to put on vert. image axis */
    neer, faar,       /* neer and far clipping plane distances
                         (misspelled for the sake of McRosopht) */
    dist;             /* distance to image plane */
  int atRel,          /* if non-zero: given neer, faar, and dist
                         quantities indicate distance relative to the
			 _at_ point, instead of the usual (in computer
			 graphics) sense if being relative to the
			 eye point */
    ortho,            /* no perspective projection: just orthographic */
    rightHanded;      /* if rightHanded, V = NxU (V points "downwards"),
			 otherwise, V = UxN (V points "upwards") */
  /* --------------------------------------------------------------------
     End of user-set parameters.  Things below are set by limnCamUpdate
     -------------------------------------------------------------------- */
  double W2V[16],     /* World to view transform. The _rows_ of this
                         matrix (its 3x3 submatrix) are the U, V, N
                         vectors which form the view-space coordinate frame.
                         The column-major ordering of elements into the
                         matrix is from ell:
                         0   4   8  12
                         1   5   9  13
                         2   6  10  14
                         3   7  11  15 */
    V2W[16],          /* View to world transform */
    U[4], V[4], N[4], /* View space basis vectors (in world coords)
			 last element always zero */
    vspNeer, vspFaar, /* not usually user-set: neer, far dist (view space) */
    vspDist;
} limnCam;

/*
******** struct limnLight
**
** information for directional lighting and the ambient light
**
** Has no dynamically allocated information or pointers
*/
typedef struct {
  float amb[3],             /* RGB ambient light color */
    _dir[LIMN_LITE_NUM][3], /* direction of light[i] (view or world space).
			       This is what the user sets via limnLightSet */
    dir[LIMN_LITE_NUM][3],  /* direction of light[i] (ONLY world space) 
			       Not user-set: calculated/copied from _dir[] */
    col[LIMN_LITE_NUM][3];  /* RGB color of light[i] */
  int on[LIMN_LITE_NUM],    /* light[i] is on */
    vsp[LIMN_LITE_NUM];     /* light[i] lives in view space */
} limnLight;

enum {
  limnDeviceUnknown,
  limnDevicePS,
  limnDeviceOpenGL,
  limnDeviceLast
};

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

enum {
  limnSpaceUnknown,
  limnSpaceWorld,
  limnSpaceView,
  limnSpaceScreen,
  limnSpaceDevice,
  limnSpaceLast
};

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

/*
******** limnQN enum
**
** the different quantized normal schemes currently supported
*/
enum {
  limnQN_Unknown,     /* 0 */
  limnQN_16checker,   /* 1 */
  limnQN_16simple,    /* 2 */
  limnQN_16border1,   /* 3 */
  limnQN_15checker,   /* 4 */
  limnQN_Last
};
#define LIMN_QN_MAX      4

/* defaultsLimn.c */
extern limn_export int limnDefCamAtRel;
extern limn_export int limnDefCamOrtho;
extern limn_export int limnDefCamRightHanded;

/* qn.c */
extern limn_export int limnQNBytes[LIMN_QN_MAX+1];
extern limn_export void (*limnQNtoV[LIMN_QN_MAX+1])(float *vec, int qn, int doNorm);
extern limn_export int (*limnVtoQN[LIMN_QN_MAX+1])(int *qnP, float *vec);

/* light.c */
typedef void (*limnEnvMapCB)(float rgb[3], float vec[3], void *data);
extern int limnEnvMapFill(Nrrd *envMap, limnEnvMapCB cb, 
			  void *data, int qnMethod);
extern void limnLightSet(limnLight *lit, int which, int vsp,
			 float r, float g, float b,
			 float x, float y, float z);
extern void limnLightSetAmbient(limnLight *lit, float r, float g, float b);
extern void limnLightSwitch(limnLight *lit, int which, int on);
extern void limnLightReset(limnLight *lit);
extern int limnLightUpdate(limnLight *lit, limnCam *cam);
extern void limnLightDiffuseCB(float rgb[3], float vec[3], void *_lit);

/* methodsLimn.c */
extern limnLight *limnLightNew(void);
extern limnLight *limnLightNix(limnLight *);
extern limnCam *limnCamNew(void);
extern limnCam *limnCamNix(limnCam *cam);
extern limnWin *limnWinNew(int device);
extern limnWin *limnWinNix(limnWin *win);

/* hestLimn.c */
extern void limnHestCamOptAdd(hestOpt **hoptP, limnCam *cam,
			      char *frDef, char *atDef, char *upDef,
			      char *dnDef, char *diDef, char *dfDef,
			      char *urDef, char *vrDef);

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
extern int limnObjSquareAdd(limnObj *obj, int sp);
extern int limnObjLoneEdgeAdd(limnObj *obj, int sp);
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

/* renderLimn.c */
extern int limnObjPSRender(limnObj *obj, limnCam *cam, 
			   Nrrd *envMap, limnWin *win);

#ifdef __cplusplus
}
#endif

#endif /* LIMN_HAS_BEEN_INCLUDED */
