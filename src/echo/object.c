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

NEW_TMPL(Sphere,
	 obj->ntext = NULL;
	 );
NEW_TMPL(Cube,
	 obj->ntext = NULL;
	 );
NEW_TMPL(Triangle,
	 obj->ntext = NULL;
	 );
NEW_TMPL(Rectangle,
	 obj->ntext = NULL;
	 );
NEW_TMPL(TriMesh,
	 ELL_3V_SET(obj->min, ECHO_POS_MAX, ECHO_POS_MAX, ECHO_POS_MAX);
	 ELL_3V_SET(obj->max, ECHO_POS_MIN, ECHO_POS_MIN, ECHO_POS_MIN);
	 obj->ntext = NULL;
	 obj->numV = obj->numF = 0;
	 obj->pos = NULL;
	 obj->vert = NULL;
	 );
NEW_TMPL(Isosurface,
	 obj->volume = NULL;
	 obj->value = 0.0;
	 /* ??? */
	 );
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
	 ELL_4M_SET_IDENTITY(obj->M);
	 ELL_4M_SET_IDENTITY(obj->Mi);
	 obj->own = AIR_FALSE;
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

NIX_TMPL(TriMesh,
	 free(obj->pos);
	 free(obj->vert);
	 );
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

NUK_TMPL(TriMesh,
	 free(obj->pos);
	 free(obj->vert);
	 );
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
  echoPos_t a[8][4], b[8][4], l[3], h[3];

  _echoObjectBounds[obj->obj->type](l, h, obj->obj);
  ELL_4V_SET(a[0], l[0], l[1], l[2], 1);
  ELL_4V_SET(a[1], h[0], l[1], l[2], 1);
  ELL_4V_SET(a[2], l[0], h[1], l[2], 1);
  ELL_4V_SET(a[3], h[0], h[1], l[2], 1);
  ELL_4V_SET(a[4], l[0], l[1], h[2], 1);
  ELL_4V_SET(a[5], h[0], l[1], h[2], 1);
  ELL_4V_SET(a[6], l[0], h[1], h[2], 1);
  ELL_4V_SET(a[7], h[0], h[1], h[2], 1);
  /*
  printf(" ---- corners in local space\n");
  printf(" %g %g %g %g\n", a[0][0], a[0][1], a[0][2], a[0][3]);
  printf(" %g %g %g %g\n", a[1][0], a[1][1], a[1][2], a[1][3]);
  printf(" %g %g %g %g\n", a[2][0], a[2][1], a[2][2], a[2][3]);
  printf(" %g %g %g %g\n", a[3][0], a[3][1], a[3][2], a[3][3]);
  printf(" %g %g %g %g\n", a[4][0], a[4][1], a[4][2], a[4][3]);
  printf(" %g %g %g %g\n", a[5][0], a[5][1], a[5][2], a[5][3]);
  printf(" %g %g %g %g\n", a[6][0], a[6][1], a[6][2], a[6][3]);
  printf(" %g %g %g %g\n", a[7][0], a[7][1], a[7][2], a[7][3]);
  */
  ELL_4MV_MUL(b[0], obj->M, a[0]); ELL_4V_HOMOG(b[0], b[0]);
  ELL_4MV_MUL(b[1], obj->M, a[1]); ELL_4V_HOMOG(b[1], b[1]);
  ELL_4MV_MUL(b[2], obj->M, a[2]); ELL_4V_HOMOG(b[2], b[2]);
  ELL_4MV_MUL(b[3], obj->M, a[3]); ELL_4V_HOMOG(b[3], b[3]);
  ELL_4MV_MUL(b[4], obj->M, a[4]); ELL_4V_HOMOG(b[4], b[4]);
  ELL_4MV_MUL(b[5], obj->M, a[5]); ELL_4V_HOMOG(b[5], b[5]);
  ELL_4MV_MUL(b[6], obj->M, a[6]); ELL_4V_HOMOG(b[6], b[6]);
  ELL_4MV_MUL(b[7], obj->M, a[7]); ELL_4V_HOMOG(b[7], b[7]);
  /*
  printf(" ---- corners in global space\n");
  printf(" %g %g %g %g\n", b[0][0], b[0][1], b[0][2], b[0][3]);
  printf(" %g %g %g %g\n", b[1][0], b[1][1], b[1][2], b[1][3]);
  printf(" %g %g %g %g\n", b[2][0], b[2][1], b[2][2], b[2][3]);
  printf(" %g %g %g %g\n", b[3][0], b[3][1], b[3][2], b[3][3]);
  printf(" %g %g %g %g\n", b[4][0], b[4][1], b[4][2], b[4][3]);
  printf(" %g %g %g %g\n", b[5][0], b[5][1], b[5][2], b[5][3]);
  printf(" %g %g %g %g\n", b[6][0], b[6][1], b[6][2], b[6][3]);
  printf(" %g %g %g %g\n", b[7][0], b[7][1], b[7][2], b[7][3]);
  */
  ELL_3V_MIN(lo, b[0], b[1]);
  ELL_3V_MIN(lo, lo, b[2]); ELL_3V_MIN(lo, lo, b[3]);
  ELL_3V_MIN(lo, lo, b[4]); ELL_3V_MIN(lo, lo, b[5]);
  ELL_3V_MIN(lo, lo, b[6]); ELL_3V_MIN(lo, lo, b[7]);
  ELL_3V_MAX(hi, b[0], b[1]);
  ELL_3V_MAX(hi, hi, b[2]); ELL_3V_MAX(hi, hi, b[3]);
  ELL_3V_MAX(hi, hi, b[4]); ELL_3V_MAX(hi, hi, b[5]);
  ELL_3V_MAX(hi, hi, b[6]); ELL_3V_MAX(hi, hi, b[7]);
  /*
  printf(" --- new corners:\n");
  printf(" %g %g %g\n", lo[0], lo[1], lo[2]);
  printf(" %g %g %g\n", hi[0], hi[1], hi[2]);
  */
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
echoObjectListAdd(EchoObject *list, EchoObject *child) {
  int idx;
  
  if (!( list && child &&
	 (echoObjectList == list->type ||
	  echoObjectAABBox == list->type) ))
    return;

  idx = airArrayIncrLen(LIST(list)->objArr, 1);
  LIST(list)->obj[idx] = child;

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
  EchoObject *o, *split, *list0, *list1;
  int i, splitIdx, len;

  if (!( echoObjectList == list->type ||
	 echoObjectAABBox == list->type )) {
    return list;
  }

  len = LIST(list)->objArr->len;
  if (len <= ECHO_LEN_SMALL_ENOUGH) {
    /* there is nothing or only one object */
    return list;
  }

  split = echoObjectNew(echoObjectSplit);
  list0 = echoObjectNew(echoObjectList);
  list1 = echoObjectNew(echoObjectList);
  SPLIT(split)->axis = axis;
  SPLIT(split)->obj0 = list0;
  SPLIT(split)->obj1 = list1;

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
  /* printf("splitIdx = %d\n", splitIdx); */
  ELL_3V_SET(loest0, ECHO_POS_MAX, ECHO_POS_MAX, ECHO_POS_MAX);
  ELL_3V_SET(loest1, ECHO_POS_MAX, ECHO_POS_MAX, ECHO_POS_MAX);
  ELL_3V_SET(hiest0, ECHO_POS_MIN, ECHO_POS_MIN, ECHO_POS_MIN);
  ELL_3V_SET(hiest1, ECHO_POS_MIN, ECHO_POS_MIN, ECHO_POS_MIN);
  airArraySetLen(LIST(list0)->objArr, splitIdx);
  for (i=0; i<splitIdx; i++) {
    o = LIST(list)->obj[*((unsigned int *)(mids + 1 + 2*i))];
    LIST(list0)->obj[i] = o;
    _echoObjectBounds[o->type](lo, hi, o);
    /*
    printf("000 lo = (%g,%g,%g), hi = (%g,%g,%g)\n",
	   lo[0], lo[1], lo[2], hi[0], hi[1], hi[2]);
    */
    ELL_3V_MIN(loest0, loest0, lo);
    ELL_3V_MAX(hiest0, hiest0, hi);
  }
  airArraySetLen(LIST(list1)->objArr, len-splitIdx);
  for (i=splitIdx; i<len; i++) {
    o = LIST(list)->obj[*((unsigned int *)(mids + 1 + 2*i))];
    LIST(list1)->obj[i-splitIdx] = o;
    _echoObjectBounds[o->type](lo, hi, o);
    /*
    printf("111 lo = (%g,%g,%g), hi = (%g,%g,%g)\n",
	   lo[0], lo[1], lo[2], hi[0], hi[1], hi[2]);
    */
    ELL_3V_MIN(loest1, loest1, lo);
    ELL_3V_MAX(hiest1, hiest1, hi);
  }
  /*
  printf("0: loest = (%g,%g,%g); hiest = (%g,%g,%g)\n",
	 loest0[0], loest0[1], loest0[2], 
	 hiest0[0], hiest0[1], hiest0[2]);
  printf("1: loest = (%g,%g,%g); hiest = (%g,%g,%g)\n",
	 loest1[0], loest1[1], loest1[2], 
	 hiest1[0], hiest1[1], hiest1[2]);
  */
  ELL_3V_COPY(SPLIT(split)->min0, loest0);
  ELL_3V_COPY(SPLIT(split)->max0, hiest0);
  ELL_3V_COPY(SPLIT(split)->min1, loest1);
  ELL_3V_COPY(SPLIT(split)->max1, hiest1);

  echoObjectNix(list);
  free(mids);
  return split;
}

EchoObject *
echoObjectListSplit3(EchoObject *list, int depth) {
  EchoObject *ret, *tmp0, *tmp1;

  if (!( echoObjectList == list->type ||
	 echoObjectAABBox == list->type )) 
    return NULL;

  if (!depth)
    return list;

  ret = echoObjectListSplit(list, 0);

#define DOIT(obj, ax) ((obj) = echoObjectListSplit((obj), (ax)))
#define MORE(obj) echoObjectSplit == (obj)->type

  if (MORE(ret)) {
    tmp0 = DOIT(SPLIT(ret)->obj0, 1);
    if (MORE(tmp0)) {
      tmp1 = DOIT(SPLIT(tmp0)->obj0, 2);
      if (MORE(tmp1)) {
	SPLIT(tmp1)->obj0 = echoObjectListSplit3(SPLIT(tmp1)->obj0, depth-1);
	SPLIT(tmp1)->obj1 = echoObjectListSplit3(SPLIT(tmp1)->obj1, depth-1);
      }
      tmp1 = DOIT(SPLIT(tmp0)->obj1, 2);
      if (MORE(tmp1)) {
	SPLIT(tmp1)->obj0 = echoObjectListSplit3(SPLIT(tmp1)->obj0, depth-1);
	SPLIT(tmp1)->obj1 = echoObjectListSplit3(SPLIT(tmp1)->obj1, depth-1);
      }
    }
    tmp0 = DOIT(SPLIT(ret)->obj1, 1);
    if (MORE(tmp0)) {
      tmp1 = DOIT(SPLIT(tmp0)->obj0, 2);
      if (MORE(tmp1)) {
	SPLIT(tmp1)->obj0 = echoObjectListSplit3(SPLIT(tmp1)->obj0, depth-1);
	SPLIT(tmp1)->obj1 = echoObjectListSplit3(SPLIT(tmp1)->obj1, depth-1);
      }
      tmp1 = DOIT(SPLIT(tmp0)->obj1, 2);
      if (MORE(tmp1)) {
	SPLIT(tmp1)->obj0 = echoObjectListSplit3(SPLIT(tmp1)->obj0, depth-1);
	SPLIT(tmp1)->obj1 = echoObjectListSplit3(SPLIT(tmp1)->obj1, depth-1);
      }
    }
  }
  return ret;
}

void
_echoSetPos(echoPos_t *p3, echoPos_t *matx, echoPos_t *p4) {
  echoPos_t a[4], b[4];
  
  if (matx) {
    ELL_4V_SET(a, p4[0], p4[1], p4[2], 1);
    ELL_4MV_MUL(b, matx, a);
    ELL_34V_HOMOG(p3, b);
  }
  else {
    ELL_3V_COPY(p3, p4);
  }
}

EchoObject *
echoObjectRoughSphere(int theRes, int phiRes, echoPos_t *matx) {
  EchoObject *ret;
  EchoObjectTriMesh *trim;
  echoPos_t *_pos, *pos, tmp[3];
  int *_vert, *vert, thidx, phidx, n;
  echoPos_t th, ph;

  ret = echoObjectNew(echoObjectTriMesh);
  trim = TRIM(ret);
  trim->numV = 2 + (phiRes-1)*theRes;
  trim->numF = (2 + 2*(phiRes-2))*theRes;

  _pos = pos = calloc(3*trim->numV, sizeof(echoPos_t));
  _vert = vert = calloc(3*trim->numF, sizeof(int));

  ELL_3V_SET(tmp, 0, 0, 1); _echoSetPos(pos, matx, tmp); pos += 3;
  for (phidx=1; phidx<phiRes; phidx++) {
    ph = AIR_AFFINE(0, phidx, phiRes, 0.0, M_PI);
    for (thidx=0; thidx<theRes; thidx++) {
      th = AIR_AFFINE(0, thidx, theRes, 0.0, 2*M_PI);
      ELL_3V_SET(tmp, cos(th)*sin(ph), sin(th)*sin(ph), cos(ph));
      _echoSetPos(pos, matx, tmp); pos += 3;
    }
  }
  ELL_3V_SET(tmp, 0, 0, -1); _echoSetPos(pos, matx, tmp);

  for (thidx=0; thidx<theRes; thidx++) {
    n = AIR_MOD(thidx+1, theRes);
    ELL_3V_SET(vert, 0, 1+thidx, 1+n); vert += 3;
  }
  for (phidx=0; phidx<phiRes-2; phidx++) {
    for (thidx=0; thidx<theRes; thidx++) {
      n = AIR_MOD(thidx+1, theRes);
      ELL_3V_SET(vert, 1+phidx*theRes+thidx, 1+(1+phidx)*theRes+thidx,
		 1+phidx*theRes+n); vert += 3;
      ELL_3V_SET(vert, 1+(1+phidx)*theRes+thidx, 1+(1+phidx)*theRes+n, 
		 1+phidx*theRes+n); vert += 3;
    }
  }
  for (thidx=0; thidx<theRes; thidx++) {
    n = AIR_MOD(thidx+1, theRes);
    ELL_3V_SET(vert, 1+(phiRes-2)*theRes+thidx, trim->numV-1,
	       1+(phiRes-2)*theRes+n); 
    vert += 3;
  }

  echoObjectTriMeshSet(ret, trim->numV, _pos, trim->numF, _vert);
  
  return(ret);
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

void
echoObjectTriMeshSet(EchoObject *_trim,
		     int numV, echoPos_t *pos,
		     int numF, int *vert) {
  EchoObjectTriMesh *trim;
  int i;

  if (_trim && echoObjectTriMesh == _trim->type) {
    trim = TRIM(_trim);
    trim->numV = numV;
    trim->pos = pos;
    trim->numF = numF;
    trim->vert = vert;
    ELL_3V_SET(trim->min, ECHO_POS_MAX, ECHO_POS_MAX, ECHO_POS_MAX);
    ELL_3V_SET(trim->max, ECHO_POS_MIN, ECHO_POS_MIN, ECHO_POS_MIN);
    ELL_3V_SET(trim->origin, 0.0, 0.0, 0.0);
    for (i=0; i<numV; i++) {
      ELL_3V_MIN(trim->min, trim->min, pos + 3*i);
      ELL_3V_MAX(trim->max, trim->max, pos + 3*i);
      ELL_3V_ADD(trim->origin, trim->origin, pos + 3*i);
    }
    ELL_3V_SCALE(trim->origin, 1.0/numV, trim->origin);
  }
  return;
}

void
echoObjectInstanceSet(EchoObject *_inst,
		      echoPos_t *M, EchoObject *obj, int own) {
  EchoObjectInstance *inst;
  
  if (_inst && echoObjectInstance == _inst->type) {
    inst = INSTANCE(_inst);
    ell4mInvert_p(inst->Mi, M);
    ELL_4M_COPY(inst->M, M);
    inst->obj = obj;
    inst->own = own;
  }
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

