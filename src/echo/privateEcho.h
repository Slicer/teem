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

#ifndef ECHO_PRIVATE_HAS_BEEN_INCLUDED
#define ECHO_PRIVATE_HAS_BEEN_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#define OBJECT(obj)    ((echoObject*)obj)
#define SPLIT(obj)     ((echoSplit*)obj)
#define LIST(obj)      ((echoList*)obj)
#define SPHERE(obj)    ((echoSphere*)obj)
#define CYLINDER(obj)  ((echoCylinder*)obj)
#define SUPERQUAD(obj) ((echoSuperquad*)obj)
#define RECTANGLE(obj) ((echoRectangle*)obj)
#define AABBOX(obj)    ((echoAABBox*)obj)
#define TRIMESH(obj)   ((echoTriMesh*)obj)
#define TRIANGLE(obj)  ((echoTriangle*)obj)
#define INSTANCE(obj)  ((echoInstance*)obj)

#define ECHO_REFLECT(refl, norm, view, tmp) \
  (tmp) = 2*ELL_3V_DOT((view), (norm)); \
  ELL_3V_SCALEADD((refl), -1.0, (view), (tmp), (norm))

#define ECHO_NEW(TYPE) \
  (echoObject##TYPE *)echoNew(echoObject##Type)

/* methodsEcho.c */
extern void _echoSceneLightAdd(echoScene *scene, echoObject *obj);
extern void _echoSceneNrrdAdd(echoScene *scene, Nrrd *nrrd);

/* intx.c */
#define RAYINTX_ARGS(TYPE) echoIntx *intx, echoRay *ray, \
                           echo##TYPE *obj, echoRTParm *parm,  \
                           echoThreadState *tstate
typedef int (*_echoRayIntx_t)(RAYINTX_ARGS(Object));
extern _echoRayIntx_t _echoRayIntx[/* object type idx */];
typedef void (*_echoRayIntxUV_t)(echoIntx *intx);
extern _echoRayIntxUV_t _echoRayIntxUV[/* object type idx */];

/* color.c */
#define INTXCOLOR_ARGS echoCol_t rgba[4], echoIntx *intx,  \
                       echoScene *scene, echoRTParm *parm, \
                       echoThreadState *tstate
typedef void (*_echoIntxColor_t)(INTXCOLOR_ARGS);
extern _echoIntxColor_t _echoIntxColor[/* matter idx */];

#ifdef __cplusplus
}
#endif

#endif /*  ECHO_PRIVATE_HAS_BEEN_INCLUDED */
