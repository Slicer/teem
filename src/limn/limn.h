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

#ifndef LIMN_HAS_BEEN_INCLUDED
#define LIMN_HAS_BEEN_INCLUDED

#include <stdlib.h>

#include <math.h>

#include <teem/air.h>
#include <teem/biff.h>
#include <teem/hest.h>
#include <teem/ell.h>
#include <teem/nrrd.h>

#if defined(_WIN32) && !defined(TEEM_STATIC) && !defined(__CYGWIN__)
#define limn_export __declspec(dllimport)
#else
#define limn_export
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define LIMN limnBiffKey
#define LIMN_LIGHT_NUM 16

/*
******** #define LIMN_SPLINE_Q_AVG_EPS
**
** The convergence tolerance used for Buss/Fillmore quaternion
** averaging
*/
#define LIMN_SPLINE_Q_AVG_EPS 0.00001

/*
****** limnCamera struct
**
** for all standard graphics camera parameters.  Image plane is
** spanned by U and V; N always points away from the viewer, U
** always points to the right, V can point up or down, if the
** camera is left- or right-handed, respectively.
**
** Has no dynamically allocated information or pointers.
*/
typedef struct limnCamera_t {
  double from[3],     /* location of eyepoint */
    at[3],            /* what eye is looking at */
    up[3],            /* what is up direction for eye (this is not updated
                         to the "true" up) */
    uRange[2],        /* range of U values to put on horiz. image axis */
    vRange[2],        /* range of V values to put on vert. image axis */
    neer, faar,       /* neer and far clipping plane distances
                         (misspelled for the sake of McRosopht) */
    dist;             /* distance to image plane */
  int atRelative,     /* if non-zero: given neer, faar, and dist
                         quantities indicate distance relative to the
			 _at_ point, instead of the usual (in computer
			 graphics) sense if being relative to the
			 eye point */
    orthographic,     /* no perspective projection: just orthographic */
    rightHanded;      /* if rightHanded, V = NxU (V points "downwards"),
			 otherwise, V = UxN (V points "upwards") */
  /* --------------------------------------------------------------------
     End of user-set parameters.  Things below are set by limnCameraUpdate
     -------------------------------------------------------------------- */
  double W2V[16],     /* World to view transform. The _rows_ of this
                         matrix (its 3x3 submatrix) are the U, V, N
                         vectors which form the view-space coordinate frame.
                         The column-major ordering of elements in the
                         matrix is from ell:
                         0   4   8  12
                         1   5   9  13
                         2   6  10  14
                         3   7  11  15 */
    V2W[16],          /* View to world transform */
    U[4], V[4], N[4], /* View space basis vectors (in world coords)
			 last element always zero */
    vspNeer, vspFaar, /* not usually user-set: neer, far, and image plane
			 distances, in view space */
    vspDist;
} limnCamera;

/*
******** struct limnLight
**
** information for directional lighting and the ambient light
**
** Has no dynamically allocated information or pointers
*/
typedef struct {
  float amb[3],              /* RGB ambient light color */
    _dir[LIMN_LIGHT_NUM][3], /* direction of light[i] (view or world space).
				This is what the user sets via limnLightSet */
    dir[LIMN_LIGHT_NUM][3],  /* direction of light[i] (ONLY world space) 
				Not user-set: calculated/copied from _dir[] */
    col[LIMN_LIGHT_NUM][3];  /* RGB color of light[i] */
  int on[LIMN_LIGHT_NUM],    /* light[i] is on */
    vsp[LIMN_LIGHT_NUM];     /* light[i] lives in view space */
} limnLight;

enum {
  limnDeviceUnknown,
  limnDevicePS,
  limnDeviceGL,
  limnDeviceLast
};

enum {
  limnEdgeTypeUnknown,     /* 0 */
  limnEdgeTypeBackFacet,   /* 1: back-facing non-crease */
  limnEdgeTypeBackCrease,  /* 2: back-facing crease */
  limnEdgeTypeContour,     /* 3: silhoette edge */
  limnEdgeTypeFrontCrease, /* 4: front-facing crease */
  limnEdgeTypeFrontFacet,  /* 5: front-facing non-crease */
  limnEdgeTypeBorder,      /* 6: attached to only one face */
  limnEdgeTypeLone,        /* 7: attached to no other faces */
  limnEdgeTypeLast
};
#define LIMN_EDGE_TYPE_MAX    5

typedef struct {
  float lineWidth[LIMN_EDGE_TYPE_MAX+1],
    creaseAngle,      /* difference between crease and facet, in *degrees* */
    bg[3];            /* background color */
  int showpage,       /* finish with "showpage" */
    wireFrame,        /* just render wire-frame */
    noBackground;     /* refrain from initially filling with bg[] color */
} limnOptsPS;

typedef struct {
  limnOptsPS ps;
  int device;
  float scale, bbox[4];
  int yFlip;
  FILE *file;
} limnWindow;

enum {
  limnSpaceUnknown,
  limnSpaceWorld,
  limnSpaceView,
  limnSpaceScreen,
  limnSpaceDevice,
  limnSpaceLast
};

/*
******** struct limnLook
**
** surface properties: pretty much anything having to do with 
** appearance, for points, edges, faces, etc.
*/
typedef struct {
  float rgba[4];
  float kads[3],   /* phong: ka, kd, ks */
    spow;          /* specular power */
} limnLook;

/*
******** struct limnVertex
**
** all the information you might want for a point
**
** Has no dynamically allocated information or pointers
*/
typedef struct {
  float world[4],        /* world coordinates (homogeneous) */
    view[4],            /* view coordinates */
    screen[3],            /* screen coordinates (device independant) */
    device[2],            /* device coordinates */
    worldNormal[3];        /* vertex normal (world coords only) */
  int lookIdx,            /* index into parent's look array */
    partIdx;
} limnVertex;

/*
******** struct limnEdge
**
** all the information about an edge
**
** Has no dynamically allocated information or pointers
*/
typedef struct limnEdge_t {
  int vertIdxIdx[2], faceIdxIdx[2],
    lookIdx,         /* index into parent's look array */
    partIdx;
  int type,          /* from the limnEdgeType enum */
    once;            /* flag used for certain kinds of rendering */
} limnEdge;

/*
******** struct limnFace
**
** all the information about a face
**
** Has no dynamically allocated information or pointers
*/
typedef struct limnFace_t {
  float worldNormal[3],
    screenNormal[3];
  int *vertIdxIdx,   /* normal array (not airArray) of indices (in part's
			vertIdx) vertex indices (in object's vert) */
    *edgeIdxIdx,     /* likewise for edges */
    sideNum;      /* number of sides (allocated length of {vert,edge}IdxIdx */
  int lookIdx,
    partIdx;
  int visible;         /* is face currently visible (AIR_TRUE or AIR_FALSE) */
  float depth;
} limnFace;

/*
******** struct limnPart
**
** one connected part of an object
*/
typedef struct limnPart_t {
  int *vertIdx, vertIdxNum,
    *edgeIdx, edgeIdxNum,
    *faceIdx, faceIdxNum;
  airArray *vertIdxArr, *edgeIdxArr, *faceIdxArr;
  int lookIdx;
  float depth;
} limnPart;

/*
******** struct limnObject
**
** the beast used to represent polygonal objects
**
** Relies on many dynamically allocated arrays
*/
typedef struct {
  limnVertex *vert; int vertNum;
  airArray *vertArr;

  limnEdge *edge; int edgeNum;
  airArray *edgeArr;

  limnFace *face; int faceNum;
  airArray *faceArr;
  int *faceSort;     /* indices into "face", sorted by depth */
  
  limnPart *part; int partNum;
  airArray *partArr;
  
  limnLook *look; int lookNum;
  airArray *lookArr;

  int doEdges,       /* if non-zero, build edges as faces are added */
    incr;            /* increment to use with airArrays */
} limnObject;

/*
******** limnQN enum
**
** the different quantized normal schemes currently supported
*/
enum {
  limnQNUnknown,     /* 0 */
  limnQN16checker,   /* 1 */
  limnQN16simple,    /* 2 */
  limnQN16border1,   /* 3 */
  limnQN15checker,   /* 4 */
  limnQN14checker,   /* 5 */
  limnQN12checker,   /* 6 */
  limnQNLast
};
#define LIMN_QN_MAX      6

enum {
  limnSplineTypeUnknown,     /* 0 */
  limnSplineTypeLinear,      /* 1 */
  limnSplineTypeTimeWarp,    /* 2 */
  limnSplineTypeHermite,     /* 3 */
  limnSplineTypeCubicBezier, /* 4 */
  limnSplineTypeBC,          /* 5 */
  limnSplineTypeLast
};
#define LIMN_SPLINE_TYPE_MAX    5

enum {
  limnSplineInfoUnknown,    /* 0 */
  limnSplineInfoScalar,     /* 1 */
  limnSplineInfo2Vector,    /* 2 */
  limnSplineInfo3Vector,    /* 3 */
  limnSplineInfoNormal,     /* 4 */
  limnSplineInfo4Vector,    /* 5 */
  limnSplineInfoQuaternion, /* 6 */
  limnSplineInfoLast
};
#define LIMN_SPLINE_INFO_MAX   6

enum {
  limnCameraPathTrackUnknown, /* 0 */
  limnCameraPathTrackFrom,    /* 1: 3-D spline for *from* points, quaternion
				 spline for camera directions towards at */
  limnCameraPathTrackAt,      /* 2: 3-D spline for *at* points, quaternion 
				 spline for directions back to camera */
  limnCameraPathTrackBoth,    /* 3: 2 3-D splines: one for from, one for at */
  limnCameraPathTrackLast
};
#define LIMN_CAMERA_PATH_TRACK_MAX 3

/*
******** limnSpline
**
** the ncpt nrrd stores control point information in a 3-D nrrd, with
** sizes C by 3 by N, where C is the number of values needed for each 
** point (3 for 3Vecs, 1 for scalars), and N is the number of control
** points.  The 3 things per control point are 0) the pre-point info 
** (either inward tangent or an internal control point), 1) the control
** point itself, 2) the post-point info (e.g., outward tangent).
**
** NOTE: for the sake of simplicity, the ncpt nrrd is always "owned"
** by the limnSpline, that is, it is COPIED from the one given in 
** limnSplineNew() (and is converted to type double along the way),
** and it will is deleted with limnSplineNix.
*/
typedef struct limnSpline_t {
  int type,          /* from limnSplineType* enum */
    info,            /* from limnSplineInfo* enum */
    loop;            /* the last (implicit) control point is the first */
  double B, C;       /* B,C values for BC-splines */
  Nrrd *ncpt;        /* the control point info, ALWAYS a 3-D nrrd */
  double *time;      /* ascending times for non-uniform control points.
			Currently, only used for limnSplineTypeTimeWarp */
} limnSpline;

typedef struct limnSplineTypeSpec_t {
  int type;          /* from limnSplineType* enum */
  double B, C;       /* B,C values for BC-splines */
} limnSplineTypeSpec;

/* defaultsLimn.c */
extern limn_export const char *limnBiffKey;
extern limn_export int limnDefCameraAtRelative;
extern limn_export int limnDefCameraOrthographic;
extern limn_export int limnDefCameraRightHanded;

/* qn.c */
extern limn_export int limnQNBins[LIMN_QN_MAX+1];
extern limn_export void (*limnQNtoV_f[LIMN_QN_MAX+1])(float *vec, int qn);
extern limn_export void (*limnQNtoV_d[LIMN_QN_MAX+1])(double *vec, int qn);
extern limn_export int (*limnVtoQN_f[LIMN_QN_MAX+1])(float *vec);
extern limn_export int (*limnVtoQN_d[LIMN_QN_MAX+1])(double *vec);


/* light.c */
extern void limnLightSet(limnLight *lit, int which, int vsp,
			 float r, float g, float b,
			 float x, float y, float z);
extern void limnLightSetAmbient(limnLight *lit, float r, float g, float b);
extern void limnLightSwitch(limnLight *lit, int which, int on);
extern void limnLightReset(limnLight *lit);
extern int limnLightUpdate(limnLight *lit, limnCamera *cam);

/* env.c */
typedef void (*limnEnvMapCB)(float rgb[3], float vec[3], void *data);
extern int limnEnvMapFill(Nrrd *envMap, limnEnvMapCB cb, 
			  int qnMethod, void *data);
extern void limnLightDiffuseCB(float rgb[3], float vec[3], void *_lit);
extern int limnEnvMapCheck(Nrrd *envMap);

/* methodsLimn.c */
extern limnLight *limnLightNew(void);
extern void limnCameraInit(limnCamera *cam);
extern limnLight *limnLightNix(limnLight *);
extern limnCamera *limnCameraNew(void);
extern limnCamera *limnCameraNix(limnCamera *cam);
extern limnWindow *limnWindowNew(int device);
extern limnWindow *limnWindowNix(limnWindow *win);

/* hestLimn.c */
extern void limnHestCameraOptAdd(hestOpt **hoptP, limnCamera *cam,
				 char *frDef, char *atDef, char *upDef,
				 char *dnDef, char *diDef, char *dfDef,
				 char *urDef, char *vrDef);

/* cam.c */
extern int limnCameraUpdate(limnCamera *cam);
extern int limnCameraPathMake(limnCamera *cam, int numFrames,
			      limnCamera *keycam, double *time,
			      int numKeys, int trackFrom, 
			      limnSplineTypeSpec *quatType,
			      limnSplineTypeSpec *posType,
			      limnSplineTypeSpec *distType,
			      limnSplineTypeSpec *uvType);

/* obj.c */
extern int limnObjectLookAdd(limnObject *obj);
extern limnObject *limnObjectNew(int incr, int doEdges);
extern limnObject *limnObjectNix(limnObject *obj);
extern int limnObjectPartAdd(limnObject *obj);
extern int limnObjectVertexAdd(limnObject *obj, int partIdx, int lookIdx,
			       float x, float y, float z);
extern int limnObjectEdgeAdd(limnObject *obj, int partIdx, int lookIdx,
			     int faceIdxIdx, int vertIdxIdx0, int vertIdxIdx1);
extern int limnObjectFaceAdd(limnObject *obj, int partIdx,
			     int lookIdx, int sideNum, int *vertIdxIdx);

/* io.c */
extern int limnObjectDescribe(FILE *file, limnObject *obj);
extern int limnObjectOFFRead(limnObject *obj, FILE *file);
extern int limnObjectOFFWrite(FILE *file, limnObject *obj);

/* shapes.c */
extern int limnObjectCubeAdd(limnObject *obj, int lookIdx);
extern int limnObjectSquareAdd(limnObject *obj, int lookIdx);
extern int limnObjectLoneEdgeAdd(limnObject *obj, int lookIdx);
extern int limnObjectCylinderAdd(limnObject *obj, int lookIdx,
				 int axis,int res);
extern int limnObjectPolarSphereAdd(limnObject *obj, int lookIdx, int axis,
				    int thetaRes, int phiRes);
extern int limnObjectConeAdd(limnObject *obj, int lookIdx, int axis, int res);
extern int limnObjectPolarSuperquadAdd(limnObject *obj, int lookIdx, int axis,
				       float A, float B,
				       int thetaRes, int phiRes);

/* transform.c */
extern int limnObjectHomog(limnObject *obj, int space);
extern int limnObjectNormals(limnObject *obj, int space);
extern int limnObjectSpaceTransform(limnObject *obj, limnCamera *cam,
				    limnWindow *win, int space);
extern int limnObjectPartTransform(limnObject *obj, int partIdx, float tx[16]);
extern int limnObjectDepthSortParts(limnObject *obj);
extern int limnObjectDepthSortFaces(limnObject *obj);
extern int limnObjectFaceReverse(limnObject *obj);

/* renderLimn.c */
extern int limnObjectRender(limnObject *obj, limnCamera *cam, limnWindow *win);
extern int limnObjectPSDraw(limnObject *obj, limnCamera *cam,
			    Nrrd *envMap, limnWindow *win);
extern int limnObjectPSDrawConcave(limnObject *obj, limnCamera *cam,
				   Nrrd *envMap, limnWindow *win);

/* splineMethods.c */
extern limnSplineTypeSpec *limnSplineTypeSpecNew(int type, ...);
extern limnSplineTypeSpec *limnSplineTypeSpecNix(limnSplineTypeSpec *spec);
extern limnSpline *limnSplineNew(Nrrd *ncpt, int info,
				 limnSplineTypeSpec *spec);
extern limnSpline *limnSplineNix(limnSpline *spline);
extern int limnSplineNrrdCleverFix(Nrrd *nout, Nrrd *nin, int info, int type);
extern limnSpline *limnSplineCleverNew(Nrrd *ncpt, int info,
				       limnSplineTypeSpec *spec);
extern int limnSplineUpdate(limnSpline *spline, Nrrd *ncpt);

/* splineMisc.c */
extern limn_export airEnum *limnSplineType;
extern limn_export airEnum *limnSplineInfo;
extern limnSpline *limnSplineParse(char *str);
extern limnSplineTypeSpec *limnSplineTypeSpecParse(char *str);
extern limn_export hestCB *limnHestSpline;
extern limn_export hestCB *limnHestSplineTypeSpec;
extern limn_export int limnSplineInfoSize[LIMN_SPLINE_INFO_MAX+1];
extern limn_export
  int limnSplineTypeHasImplicitTangents[LIMN_SPLINE_TYPE_MAX+1];
extern int limnSplineNumPoints(limnSpline *spline);
extern double limnSplineMinT(limnSpline *spline);
extern double limnSplineMaxT(limnSpline *spline);
extern void limnSplineBCSet(limnSpline *spline, double B, double C);

/* splineEval.c */
extern void limnSplineEvaluate(double *out, limnSpline *spline, double time);
extern int limnSplineNrrdEvaluate(Nrrd *nout, limnSpline *spline, Nrrd *nin);
extern int limnSplineSample(Nrrd *nout, limnSpline *spline,
			    double minT, int M, double maxT);


#ifdef __cplusplus
}
#endif

#endif /* LIMN_HAS_BEEN_INCLUDED */
