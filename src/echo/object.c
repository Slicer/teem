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

#include "echo.h"
#include "private.h"

#define NEW_TMPL(TYPE, BODY)                                     \
EchoObject##TYPE *                                               \
_echoObject##TYPE##_new(void) {                                  \
  EchoObject##TYPE *obj;                                         \
                                                                 \
  obj = (EchoObject##TYPE *)calloc(1, sizeof(EchoObject##TYPE)); \
  obj->type = echoObject##TYPE;                                  \
  do { BODY } while (0);                                         \
  return obj;                                                    \
}

/*
  echoObjectUnknown,
  echoObjectSphere,     
  echoObjectCube,       
  echoObjectTriangle,   
  echoObjectRectangle,  
  echoObjectTriMesh,    
  echoObjectIsosurface, 
  echoObjectAABBox,     
  echoObjectSplit,      
  echoObjectList,       
  echoObjectInstance,   
  echoObjectLast
*/

NEW_TMPL(Sphere,);
NEW_TMPL(Cube,);
NEW_TMPL(Triangle,);
NEW_TMPL(Rectangle,);
NEW_TMPL(TriMesh, /* ??? */ );
NEW_TMPL(Isosurface, /* ??? */);
NEW_TMPL(AABBox,
	 obj->obj = NULL;
	 obj->objArr = airArrayNew((void**)&(obj->obj), NULL,
				   sizeof(EchoObject *),
				   ECHO_LIST_OBJECT_INCR);
	 airArrayPointerCB(obj->objArr,
			   airNull,
			   (void *(*)(void *))echoObjectNix);
	 ELL_3V_SET(obj->min, ECHO_POS_MAX, ECHO_POS_MAX, ECHO_POS_MAX);
	 ELL_3V_SET(obj->max, ECHO_POS_MIN, ECHO_POS_MIN, ECHO_POS_MIN);
	 );
NEW_TMPL(Split,
	 obj->obj0 = obj->obj1 = NULL;
	 );
NEW_TMPL(List,
	 obj->obj = NULL;
	 obj->objArr = airArrayNew((void**)&(obj->obj), NULL,
				   sizeof(EchoObject *),
				   ECHO_LIST_OBJECT_INCR);
	 airArrayPointerCB(obj->objArr,
			   airNull,
			   (void *(*)(void *))echoObjectNix);
	 );
NEW_TMPL(Instance,
	 ELL_4M_SET_IDENT(obj->matx);
	 ELL_3M_SET_IDENT(obj->mot);
	 obj->own = AIR_FALSE;
	 obj->motion = AIR_FALSE;
	 obj->obj = NULL;
	 );

EchoObject *(*
_echoObjectNew[ECHO_OBJECT_MAX+1])(void) = {
  NULL,
  (EchoObject *(*)(void))_echoObjectSphere_new,
  (EchoObject *(*)(void))_echoObjectCube_new,
  (EchoObject *(*)(void))_echoObjectTriangle_new,
  (EchoObject *(*)(void))_echoObjectRectangle_new,
  (EchoObject *(*)(void))_echoObjectTriMesh_new,
  (EchoObject *(*)(void))_echoObjectIsosurface_new,
  (EchoObject *(*)(void))_echoObjectAABBox_new,
  (EchoObject *(*)(void))_echoObjectSplit_new,
  (EchoObject *(*)(void))_echoObjectList_new,
  (EchoObject *(*)(void))_echoObjectInstance_new
};

EchoObject *
echoObjectNew(int type) {
  
  return _echoObjectNew[type]();
}

/* ---------------------------------------------------------- */
/* ---------------------------------------------------------- */
/* ---------------------------------------------------------- */
/* ---------------------------------------------------------- */
/* ---------------------------------------------------------- */
/* ---------------------------------------------------------- */

#define NIX_TMPL(TYPE, BODY)                                     \
EchoObject##TYPE *                                               \
_echoObject##TYPE##_nix(EchoObject##TYPE *obj) {                 \
                                                                 \
  do { BODY } while (0);                                         \
  free(obj);                                                     \
  return NULL;                                                   \
}

NIX_TMPL(TriMesh, /* ??? */);
NIX_TMPL(Isosurface, /* ??? */);
NIX_TMPL(AABBox,
	 /* unset callbacks */
	 airArrayPointerCB(obj->objArr, NULL, NULL);
	 airArrayNuke(obj->objArr);
	 );
NIX_TMPL(Split,
	 echoObjectNix(obj->obj0);
	 echoObjectNix(obj->obj1);
	 );
NIX_TMPL(List,
	 /* unset callbacks */
	 airArrayPointerCB(obj->objArr, NULL, NULL);
	 airArrayNuke(obj->objArr);
	 );
NIX_TMPL(Instance, /* ??? */);

EchoObject *(*
_echoObjectNix[ECHO_OBJECT_MAX+1])(EchoObject *) = {
  NULL,
  (EchoObject *(*)(EchoObject *))airFree,
  (EchoObject *(*)(EchoObject *))airFree,
  (EchoObject *(*)(EchoObject *))airFree,
  (EchoObject *(*)(EchoObject *))airFree,
  (EchoObject *(*)(EchoObject *))_echoObjectTriMesh_nix,
  (EchoObject *(*)(EchoObject *))_echoObjectIsosurface_nix,
  (EchoObject *(*)(EchoObject *))_echoObjectAABBox_nix,
  (EchoObject *(*)(EchoObject *))_echoObjectSplit_nix,
  (EchoObject *(*)(EchoObject *))_echoObjectList_nix,
  (EchoObject *(*)(EchoObject *))_echoObjectInstance_nix
};

EchoObject *
echoObjectNix(EchoObject *obj) {

  return _echoObjectNix[obj->type](obj);
}


/* ---------------------------------------------------------- */
/* ---------------------------------------------------------- */
/* ---------------------------------------------------------- */
/* ---------------------------------------------------------- */
/* ---------------------------------------------------------- */
/* ---------------------------------------------------------- */

#define NUK_TMPL(TYPE, BODY)                                     \
EchoObject##TYPE *                                               \
_echoObject##TYPE##_nuke(EchoObject##TYPE *obj) {                \
                                                                 \
  do { BODY } while (0);                                         \
  free(obj);                                                     \
  return NULL;                                                   \
}

NUK_TMPL(TriMesh, /* ??? */);
NUK_TMPL(Isosurface, /* ??? */);
NUK_TMPL(AABBox,
	 /* due to airArray callbacks, this will nuke all kids */
	 airArrayNuke(obj->objArr);
	 );
NUK_TMPL(Split,
	 echoObjectNuke(obj->obj0);
	 echoObjectNuke(obj->obj1);
	 );
NUK_TMPL(List,
	 /* due to airArray callbacks, this will nuke all kids */
	 airArrayNuke(obj->objArr);
	 );
NUK_TMPL(Instance,
	 if (obj->own) {
	   echoObjectNix(obj->obj);
	 }
	 );

EchoObject *(*
_echoObjectNuke[ECHO_OBJECT_MAX+1])(EchoObject *) = {
  NULL,
  (EchoObject *(*)(EchoObject *))airFree,
  (EchoObject *(*)(EchoObject *))airFree,
  (EchoObject *(*)(EchoObject *))airFree,
  (EchoObject *(*)(EchoObject *))airFree,
  (EchoObject *(*)(EchoObject *))_echoObjectTriMesh_nuke,
  (EchoObject *(*)(EchoObject *))_echoObjectIsosurface_nuke,
  (EchoObject *(*)(EchoObject *))_echoObjectAABBox_nuke,
  (EchoObject *(*)(EchoObject *))_echoObjectSplit_nuke,
  (EchoObject *(*)(EchoObject *))_echoObjectList_nuke,
  (EchoObject *(*)(EchoObject *))_echoObjectInstance_nuke
};

EchoObject *
echoObjectNuke(EchoObject *obj) {

  return _echoObjectNuke[obj->type](obj);
}


/* ---------------------------------------------------------- */
/* ---------------------------------------------------------- */
/* ---------------------------------------------------------- */
/* ---------------------------------------------------------- */
/* ---------------------------------------------------------- */
/* ---------------------------------------------------------- */

/*
  echoObjectUnknown,
  echoObjectSphere,     
  echoObjectCube,       
  echoObjectTriangle,   
  echoObjectRectangle,  
  echoObjectTriMesh,    
  echoObjectIsosurface, 
  echoObjectAABBox,     
  echoObjectSplit,      
  echoObjectList,       
  echoObjectInstance,   
  echoObjectLast
*/

/*
 define BNDS_ARGS(TYPE) echoPos_t lo[3], echoPos_t hi[3], \
                        EchoObject##TYPE *obj

 typedef int (*_echoObjectBounds_t)(BNDS_ARGS( ));
*/

#define BNDS_TMPL(TYPE)                       \
void                                          \
_echoObject##TYPE##_bounds(BNDS_ARGS(TYPE))

#define BNDS_FINISH \
  lo[0] -= ECHO_EPSILON; \
  lo[1] -= ECHO_EPSILON; \
  lo[2] -= ECHO_EPSILON; \
  hi[0] += ECHO_EPSILON; \
  hi[1] += ECHO_EPSILON; \
  hi[2] += ECHO_EPSILON

BNDS_TMPL(Sphere) {
  lo[0] = obj->pos[0] - obj->rad;
  lo[1] = obj->pos[1] - obj->rad;
  lo[2] = obj->pos[2] - obj->rad;
  hi[0] = obj->pos[0] + obj->rad;
  hi[1] = obj->pos[1] + obj->rad;
  hi[2] = obj->pos[2] + obj->rad;
  BNDS_FINISH;
}

BNDS_TMPL(Cube) {
  ELL_3V_SET(lo, -0.5-ECHO_EPSILON, -0.5-ECHO_EPSILON, -0.5-ECHO_EPSILON);
  ELL_3V_SET(hi,  0.5+ECHO_EPSILON,  0.5+ECHO_EPSILON,  0.5+ECHO_EPSILON);
  BNDS_FINISH;
}

BNDS_TMPL(Triangle) {
  ELL_3V_COPY(lo, obj->vert[0]);
  ELL_3V_MIN(lo, lo, obj->vert[1]);
  ELL_3V_MIN(lo, lo, obj->vert[2]);
  ELL_3V_COPY(hi, obj->vert[0]);
  ELL_3V_MIN(hi, hi, obj->vert[1]);
  ELL_3V_MIN(hi, hi, obj->vert[2]);
  BNDS_FINISH;
}

BNDS_TMPL(Rectangle) {
  echoPos_t v1[2], v2[3], v3[3];

  ELL_3V_COPY(lo, obj->origin);
  ELL_3V_ADD(v1, lo, obj->edge0);
  ELL_3V_ADD(v2, lo, obj->edge1);
  ELL_3V_ADD(v3, v1, obj->edge1);
  ELL_3V_MIN(lo, lo, v1);
  ELL_3V_MIN(lo, lo, v2);
  ELL_3V_MIN(lo, lo, v3);
  ELL_3V_COPY(hi, obj->origin);
  ELL_3V_MAX(hi, hi, v1);
  ELL_3V_MAX(hi, hi, v2);
  ELL_3V_MAX(hi, hi, v3);
  BNDS_FINISH;
}
BNDS_TMPL(AABBox) {
  ELL_3V_COPY(lo, obj->min);
  ELL_3V_COPY(hi, obj->max);
  BNDS_FINISH;
}
BNDS_TMPL(Split) {
  fprintf(stderr, "_echoObjectSplit_bounds: unimplemented!\n");
  BNDS_FINISH;
}
BNDS_TMPL(List) {
  int i;
  echoPos_t l[3], h[3];
  EchoObject *o;

  ELL_3V_SET(lo, ECHO_POS_MAX, ECHO_POS_MAX, ECHO_POS_MAX);
  ELL_3V_SET(hi, ECHO_POS_MIN, ECHO_POS_MIN, ECHO_POS_MIN);
  for (i=0; i<obj->objArr->len; i++) {
    o = obj->obj[i];
    _echoObjectBounds[o->type](l, h, o);
    ELL_3V_MIN(lo, lo, l);
    ELL_3V_MAX(hi, hi, h);
  }
  BNDS_FINISH;
}
BNDS_TMPL(Instance) {
  fprintf(stderr, "_echoObjectInstance_bounds: unimplemented!\n");
  BNDS_FINISH;
}
	  
_echoObjectBounds_t
_echoObjectBounds[ECHO_OBJECT_MAX+1] = {
  NULL,
  (_echoObjectBounds_t)_echoObjectSphere_bounds,
  (_echoObjectBounds_t)_echoObjectCube_bounds,
  (_echoObjectBounds_t)_echoObjectTriangle_bounds,
  (_echoObjectBounds_t)_echoObjectRectangle_bounds,
  NULL,
  NULL,
  (_echoObjectBounds_t)_echoObjectAABBox_bounds,
  (_echoObjectBounds_t)_echoObjectSplit_bounds,
  (_echoObjectBounds_t)_echoObjectList_bounds,
  (_echoObjectBounds_t)_echoObjectInstance_bounds,
};

void
echoObjectBounds(echoPos_t *lo, echoPos_t *hi, EchoObject *obj) {
  _echoObjectBounds[obj->type](lo, hi, obj);
}

/*
  echoObjectUnknown,
  echoObjectSphere,     
  echoObjectCube,       
  echoObjectTriangle,
  echoObjectRectangle,
  echoObjectTriMesh,
  echoObjectIsosurface,
  echoObjectAABBox,
  echoObjectSplit,      
  echoObjectList,       
  echoObjectInstance,   
  echoObjectLast
*/

/* ---------------------------------------------------------- */
/* ---------------------------------------------------------- */
/* ---------------------------------------------------------- */
/* ---------------------------------------------------------- */
/* ---------------------------------------------------------- */
/* ---------------------------------------------------------- */

int
echoObjectIsContainer(EchoObject *obj) {
  
  if (!obj)
    return AIR_FALSE;

  if (echoObjectAABBox == obj->type ||
      echoObjectInstance == obj->type ||
      echoObjectSplit == obj->type ||
      echoObjectList == obj->type) {
    return AIR_TRUE;
  }
  else {
    return AIR_FALSE;
  }
}

void
echoObjectListAdd(EchoObject *parent, EchoObject *child) {
  int idx;
  
  if (!( parent && child &&
	 (echoObjectList == parent->type ||
	  echoObjectAABBox == parent->type) ))
    return;

  idx = airArrayIncrLen(LIST(parent)->objArr, 1);
  LIST(parent)->obj[idx] = child;

  return;
}

int
_echoPosCompare(double *A, double *B) {
  
  return *A < *B ? -1 : (*A > *B ? 1 : 0);
}

/*
******** echoObjectListSplit()
**
** returns a EchoObjectSplit to point to the same things as pointed
** to by the given EchoObjectList
*/
EchoObject *
echoObjectListSplit(EchoObject *list, int axis) {
  echoPos_t lo[3], hi[3], loest0[3], hiest0[3],
    loest1[3], hiest1[3];
  double *mids;
  EchoObject *o, *split, *box0, *box1;
  int i, splitIdx, len;

  if (!( echoObjectList == list->type ||
	 echoObjectAABBox == list->type )) 
    return NULL;

  split = echoObjectNew(echoObjectSplit);
  box0 = echoObjectNew(echoObjectAABBox);
  box1 = echoObjectNew(echoObjectAABBox);
  SPLIT(split)->axis = axis;
  SPLIT(split)->obj0 = box0;
  SPLIT(split)->obj1 = box1;

  len = LIST(list)->objArr->len;
  if (!len) {
    echoObjectNix(list);
    return split;
  }

  mids = malloc(2 * len * sizeof(double));
  for (i=0; i<len; i++) {
    o = LIST(list)->obj[i];
    _echoObjectBounds[o->type](lo, hi, o);
    mids[0 + 2*i] = (lo[axis] + hi[axis])/2;
    *((unsigned int *)(mids + 1 + 2*i)) = i;
  }
  /* overkill, I know, I know */
  qsort(mids, len, 2*sizeof(double),
	(int (*)(const void *, const void *))_echoPosCompare);
  /*
  for (i=0; i<len; i++) {
    printf("%d -> %g\n", i, mids[0 + 2*i]);
  }
  */
  
  splitIdx = len/2;
  printf("splitIdx = %d\n", splitIdx);
  ELL_3V_SET(loest0, ECHO_POS_MAX, ECHO_POS_MAX, ECHO_POS_MAX);
  ELL_3V_SET(loest1, ECHO_POS_MAX, ECHO_POS_MAX, ECHO_POS_MAX);
  ELL_3V_SET(hiest0, ECHO_POS_MIN, ECHO_POS_MIN, ECHO_POS_MIN);
  ELL_3V_SET(hiest1, ECHO_POS_MIN, ECHO_POS_MIN, ECHO_POS_MIN);
  for (i=0; i<splitIdx; i++) {
    o = LIST(list)->obj[*((unsigned int *)(mids + 1 + 2*i))];
    echoObjectListAdd(box0, o);
    _echoObjectBounds[o->type](lo, hi, o);
    /*
    printf("000 lo = (%g,%g,%g), hi = (%g,%g,%g)\n",
	   lo[0], lo[1], lo[2], hi[0], hi[1], hi[2]);
    */
    ELL_3V_MIN(loest0, loest0, lo);
    ELL_3V_MAX(hiest0, hiest0, hi);
  }
  for (i=splitIdx; i<len; i++) {
    o = LIST(list)->obj[*((unsigned int *)(mids + 1 + 2*i))];
    echoObjectListAdd(box1, o);
    _echoObjectBounds[o->type](lo, hi, o);
    /*
    printf("111 lo = (%g,%g,%g), hi = (%g,%g,%g)\n",
	   lo[0], lo[1], lo[2], hi[0], hi[1], hi[2]);
    */
    ELL_3V_MIN(loest1, loest1, lo);
    ELL_3V_MAX(hiest1, hiest1, hi);
  }

  printf("0: loest = (%g,%g,%g); hiest = (%g,%g,%g)\n",
	 loest0[0], loest0[1], loest0[2], 
	 hiest0[0], hiest0[1], hiest0[2]);
  printf("1: loest = (%g,%g,%g); hiest = (%g,%g,%g)\n",
	 loest1[0], loest1[1], loest1[2], 
	 hiest1[0], hiest1[1], hiest1[2]);

  ELL_3V_COPY(AABBOX(box0)->min, loest0);
  ELL_3V_COPY(AABBOX(box0)->max, hiest0);
  ELL_3V_COPY(AABBOX(box1)->min, loest1);
  ELL_3V_COPY(AABBOX(box1)->max, hiest1);

  echoObjectNix(list);
  free(mids);
  return split;
}

EchoObject *
echoObjectListSplit3(EchoObject *list, int depth) {
  EchoObject *tmp, *ret, **ptr;

  if (!( echoObjectList == list->type ||
	 echoObjectAABBox == list->type )) 
    return NULL;
  
  if (0 == depth)
    return list;
  
  ret = tmp = echoObjectListSplit(list, 0);
  SPLIT(tmp)->obj0 = echoObjectListSplit(SPLIT(tmp)->obj0, 1);
  SPLIT(tmp)->obj1 = echoObjectListSplit(SPLIT(tmp)->obj1, 1);

  tmp = SPLIT(ret)->obj0;
  SPLIT(tmp)->obj0 = echoObjectListSplit(SPLIT(tmp)->obj0, 2);
  SPLIT(tmp)->obj1 = echoObjectListSplit(SPLIT(tmp)->obj1, 2);
  tmp = SPLIT(ret)->obj1;
  SPLIT(tmp)->obj0 = echoObjectListSplit(SPLIT(tmp)->obj0, 2);
  SPLIT(tmp)->obj1 = echoObjectListSplit(SPLIT(tmp)->obj1, 2);

  ptr = &(SPLIT(SPLIT(SPLIT(ret)->obj0)->obj0)->obj0);
  *ptr = echoObjectListSplit3(*ptr, depth-1);
  ptr = &(SPLIT(SPLIT(SPLIT(ret)->obj0)->obj0)->obj1);
  *ptr = echoObjectListSplit3(*ptr, depth-1);
  ptr = &(SPLIT(SPLIT(SPLIT(ret)->obj0)->obj1)->obj0);
  *ptr = echoObjectListSplit3(*ptr, depth-1);
  ptr = &(SPLIT(SPLIT(SPLIT(ret)->obj0)->obj1)->obj1);
  *ptr = echoObjectListSplit3(*ptr, depth-1);
  ptr = &(SPLIT(SPLIT(SPLIT(ret)->obj1)->obj0)->obj0);
  *ptr = echoObjectListSplit3(*ptr, depth-1);
  ptr = &(SPLIT(SPLIT(SPLIT(ret)->obj1)->obj0)->obj1);
  *ptr = echoObjectListSplit3(*ptr, depth-1);
  ptr = &(SPLIT(SPLIT(SPLIT(ret)->obj1)->obj1)->obj0);
  *ptr = echoObjectListSplit3(*ptr, depth-1);
  ptr = &(SPLIT(SPLIT(SPLIT(ret)->obj1)->obj1)->obj1);
  *ptr = echoObjectListSplit3(*ptr, depth-1);
  return ret;
}


void
echoObjectSphereSet(EchoObject *_sphere,
		    echoPos_t x, echoPos_t y, echoPos_t z, echoPos_t rad) {
  EchoObjectSphere *sphere;

  if (_sphere && echoObjectSphere == _sphere->type) {
    sphere = (EchoObjectSphere *)_sphere;
    ELL_3V_SET(sphere->pos, x, y, z);
    sphere->rad = rad;
  }
  return;
}

void
echoObjectRectangleSet(EchoObject *_rect,
		       echoPos_t ogx, echoPos_t ogy, echoPos_t ogz,
		       echoPos_t e0x, echoPos_t e0y, echoPos_t e0z,
		       echoPos_t e1x, echoPos_t e1y, echoPos_t e1z) {
  EchoObjectRectangle *rect;

  if (_rect && echoObjectRectangle == _rect->type) {
    rect = (EchoObjectRectangle *)_rect;
    ELL_3V_SET(rect->origin, ogx, ogy, ogz);
    ELL_3V_SET(rect->edge0, e0x, e0y, e0z);
    ELL_3V_SET(rect->edge1, e1x, e1y, e1z);
    rect->area = ELL_3V_LEN(rect->edge0) * ELL_3V_LEN(rect->edge1);
  }
  return;
}
		       
void
echoObjectTriangleSet(EchoObject *_tri,
		      echoPos_t x0, echoPos_t y0, echoPos_t z0, 
		      echoPos_t x1, echoPos_t y1, echoPos_t z1, 
		      echoPos_t x2, echoPos_t y2, echoPos_t z2) {
  EchoObjectTriangle *tri;

  if (_tri && echoObjectTriangle == _tri->type) {
    tri = (EchoObjectTriangle *)_tri;
    ELL_3V_SET(tri->vert[0], x0, y0, z0);
    ELL_3V_SET(tri->vert[1], x1, y1, z1);
    ELL_3V_SET(tri->vert[2], x2, y2, z2);
  }
  return;
}

/*		       
  echoObjectUnknown,
  echoObjectSphere,   
  echoObjectCube,     
  echoObjectTriangle, 
  echoObjectRectangle,
  echoObjectTriMesh,   
  echoObjectIsosurface,
  echoObjectAABBox,    
  echoObjectList,      
  echoObjectInstance,  
  echoObjectLast
*/

