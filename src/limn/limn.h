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
    fov,              /* if non-NaN, and aspect is non-NaN, then {u,v}Range
			 will be set accordingly by limnCameraUpdate().
			 "fov" is the angle, in degrees, vertically subtended
			 by the view window */
    aspect,           /* the ratio of horizontal to vertical size of the 
			 view window */
    neer, faar,       /* near and far clipping plane distances
                         (misspelled for the sake of a McRosopht compiler) */
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
#define LIMN_EDGE_TYPE_MAX    7

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
  float scale, 
    bbox[4];          /* minX minY maxX maxY */
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
**
** learned: I used to have an array of limnParts inside here, and that
** array was managed by an airArray.  Inside the limnPart, are more
** airArrays, for example the faceIdxArr which internally stores the
** *address* of faceIdx.  When the array of limnParts is resized, the
** limnPart's faceIdx pointer is still valid, and faceIdxArr is still
** valid, but the faceIdxArr's internal pointer to the faceIdx pointer
** is now bogus.  Thus: the weakness of airArrays (as long as they
** aren't giving the data pointer anew for EACH ACCESS), is that you
** must not confuse the airArrays by changing the address of its user
** data pointer.  Putting user data pointers inside of a bigger
** airArray is a fine way to create such confusion.
*/
typedef struct {
  limnVertex *vert; int vertNum;
  airArray *vertArr;

  limnEdge *edge; int edgeNum;
  airArray *edgeArr;

  limnFace *face; int faceNum;
  airArray *faceArr;
  limnFace **faceSort;    /* pointers into "face", sorted by depth */
  
  limnPart **part; int partNum;  /* double indirection, see above */
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
  limnQNUnknown,     /*  0 */
  limnQN16simple,    /*  1 */
  limnQN16border1,   /*  2 */
  limnQN16checker,   /*  3 */
  limnQN16octa,      /*  4 */
  limnQN15octa,      /*  5 */
  limnQN14checker,   /*  6 */
  limnQN14octa,      /*  7 */
  limnQN13octa,      /*  8 */
  limnQN12checker,   /*  9 */
  limnQN12octa,      /* 10 */
  limnQN11octa,      /* 11 */
  limnQN10checker,   /* 12 */
  limnQN10octa,      /* 13 */
  limnQN9octa,       /* 14 */
  limnQN8checker,    /* 15 */
  limnQN8octa,       /* 16 */
  limnQNLast
};
#define LIMN_QN_MAX     16

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
  limnCameraPathTrackBoth,    /* 3: three 3-D splines: for from point, at
				 point, and the up vector */
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
TEEM_API const char *limnBiffKey;
TEEM_API int limnDefCameraAtRelative;
TEEM_API int limnDefCameraOrthographic;
TEEM_API int limnDefCameraRightHanded;

/* enumsLimn.c */
TEEM_API airEnum *limnCameraPathTrack;

/* qn.c */
TEEM_API int limnQNBins[LIMN_QN_MAX+1];
TEEM_API void (*limnQNtoV_f[LIMN_QN_MAX+1])(float *vec, int qn);
TEEM_API void (*limnQNtoV_d[LIMN_QN_MAX+1])(double *vec, int qn);
TEEM_API int (*limnVtoQN_f[LIMN_QN_MAX+1])(float *vec);
TEEM_API int (*limnVtoQN_d[LIMN_QN_MAX+1])(double *vec);

/* light.c */
TEEM_API void limnLightSet(limnLight *lit, int which, int vsp,
			   float r, float g, float b,
			   float x, float y, float z);
TEEM_API void limnLightSetAmbient(limnLight *lit, float r, float g, float b);
TEEM_API void limnLightSwitch(limnLight *lit, int which, int on);
TEEM_API void limnLightReset(limnLight *lit);
TEEM_API int limnLightUpdate(limnLight *lit, limnCamera *cam);

/* env.c */
typedef void (*limnEnvMapCB)(float rgb[3], float vec[3], void *data);
TEEM_API int limnEnvMapFill(Nrrd *envMap, limnEnvMapCB cb, 
			    int qnMethod, void *data);
TEEM_API void limnLightDiffuseCB(float rgb[3], float vec[3], void *_lit);
TEEM_API int limnEnvMapCheck(Nrrd *envMap);

/* methodsLimn.c */
TEEM_API limnLight *limnLightNew(void);
TEEM_API void limnCameraInit(limnCamera *cam);
TEEM_API limnLight *limnLightNix(limnLight *);
TEEM_API limnCamera *limnCameraNew(void);
TEEM_API limnCamera *limnCameraNix(limnCamera *cam);
TEEM_API limnWindow *limnWindowNew(int device);
TEEM_API limnWindow *limnWindowNix(limnWindow *win);

/* hestLimn.c */
TEEM_API void limnHestCameraOptAdd(hestOpt **hoptP, limnCamera *cam,
				   char *frDef, char *atDef, char *upDef,
				   char *dnDef, char *diDef, char *dfDef,
				   char *urDef, char *vrDef, char *fvDef);

/* cam.c */
TEEM_API int limnCameraAspectSet(limnCamera *cam,
				 int horz, int vert, int centering);
TEEM_API int limnCameraUpdate(limnCamera *cam);
TEEM_API int limnCameraPathMake(limnCamera *cam, int numFrames,
				limnCamera *keycam, double *time,
				int numKeys, int trackFrom, 
				limnSplineTypeSpec *quatType,
				limnSplineTypeSpec *posType,
				limnSplineTypeSpec *distType,
				limnSplineTypeSpec *viewType);

/* obj.c */
TEEM_API int limnObjectLookAdd(limnObject *obj);
TEEM_API limnObject *limnObjectNew(int incr, int doEdges);
TEEM_API limnObject *limnObjectNix(limnObject *obj);
TEEM_API int limnObjectPartAdd(limnObject *obj);
TEEM_API int limnObjectVertexAdd(limnObject *obj, int partIdx, int lookIdx,
				 float x, float y, float z);
TEEM_API int limnObjectEdgeAdd(limnObject *obj, int partIdx, int lookIdx,
			       int faceIdxIdx, int vertIdxIdx0,
			       int vertIdxIdx1);
TEEM_API int limnObjectFaceAdd(limnObject *obj, int partIdx, int lookIdx,
			       int sideNum, int *vertIdxIdx);

/* io.c */
TEEM_API int limnObjectDescribe(FILE *file, limnObject *obj);
TEEM_API int limnObjectOFFRead(limnObject *obj, FILE *file);
TEEM_API int limnObjectOFFWrite(FILE *file, limnObject *obj);

/* shapes.c */
TEEM_API int limnObjectCubeAdd(limnObject *obj, int lookIdx);
TEEM_API int limnObjectSquareAdd(limnObject *obj, int lookIdx);
TEEM_API int limnObjectLoneEdgeAdd(limnObject *obj, int lookIdx);
TEEM_API int limnObjectCylinderAdd(limnObject *obj, int lookIdx,
				   int axis,int res);
TEEM_API int limnObjectPolarSphereAdd(limnObject *obj, int lookIdx, int axis,
				      int thetaRes, int phiRes);
TEEM_API int limnObjectConeAdd(limnObject *obj, int lookIdx,
			       int axis, int res);
TEEM_API int limnObjectPolarSuperquadAdd(limnObject *obj, int lookIdx,
					 int axis, float A, float B,
					 int thetaRes, int phiRes);

/* transform.c */
TEEM_API int limnObjectHomog(limnObject *obj, int space);
TEEM_API int limnObjectNormals(limnObject *obj, int space);
TEEM_API int limnObjectSpaceTransform(limnObject *obj, limnCamera *cam,
				      limnWindow *win, int space);
TEEM_API int limnObjectPartTransform(limnObject *obj, int partIdx,
				     float tx[16]);
TEEM_API int limnObjectDepthSortParts(limnObject *obj);
TEEM_API int limnObjectDepthSortFaces(limnObject *obj);
TEEM_API int limnObjectFaceReverse(limnObject *obj);

/* renderLimn.c */
TEEM_API int limnObjectRender(limnObject *obj, limnCamera *cam,
			      limnWindow *win);
TEEM_API int limnObjectPSDraw(limnObject *obj, limnCamera *cam,
			      Nrrd *envMap, limnWindow *win);
TEEM_API int limnObjectPSDrawConcave(limnObject *obj, limnCamera *cam,
				     Nrrd *envMap, limnWindow *win);

/* splineMethods.c */
TEEM_API limnSplineTypeSpec *limnSplineTypeSpecNew(int type, ...);
TEEM_API limnSplineTypeSpec *limnSplineTypeSpecNix(limnSplineTypeSpec *spec);
TEEM_API limnSpline *limnSplineNew(Nrrd *ncpt, int info,
				   limnSplineTypeSpec *spec);
TEEM_API limnSpline *limnSplineNix(limnSpline *spline);
TEEM_API int limnSplineNrrdCleverFix(Nrrd *nout, Nrrd *nin,
				     int info, int type);
TEEM_API limnSpline *limnSplineCleverNew(Nrrd *ncpt, int info,
					 limnSplineTypeSpec *spec);
TEEM_API int limnSplineUpdate(limnSpline *spline, Nrrd *ncpt);

/* splineMisc.c */
TEEM_API airEnum *limnSplineType;
TEEM_API airEnum *limnSplineInfo;
TEEM_API limnSpline *limnSplineParse(char *str);
TEEM_API limnSplineTypeSpec *limnSplineTypeSpecParse(char *str);
TEEM_API hestCB *limnHestSpline;
TEEM_API hestCB *limnHestSplineTypeSpec;
TEEM_API int limnSplineInfoSize[LIMN_SPLINE_INFO_MAX+1];
TEEM_API int limnSplineTypeHasImplicitTangents[LIMN_SPLINE_TYPE_MAX+1];
TEEM_API int limnSplineNumPoints(limnSpline *spline);
TEEM_API double limnSplineMinT(limnSpline *spline);
TEEM_API double limnSplineMaxT(limnSpline *spline);
TEEM_API void limnSplineBCSet(limnSpline *spline, double B, double C);

/* splineEval.c */
TEEM_API void limnSplineEvaluate(double *out, limnSpline *spline, double time);
TEEM_API int limnSplineNrrdEvaluate(Nrrd *nout, limnSpline *spline, Nrrd *nin);
TEEM_API int limnSplineSample(Nrrd *nout, limnSpline *spline,
			      double minT, int M, double maxT);


#ifdef __cplusplus
}
#endif

#endif /* LIMN_HAS_BEEN_INCLUDED */
