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


#include "limn.h"

#define LIMN_INCR 64

limnObj *
limnNewObj() {
  char me[] = "limnNewObj", err[128];
  limnObj *obj;

  obj = (limnObj *)calloc(1, sizeof(limnObj));
  if (obj) {
    obj->pArr = airArrayNew((void**)&(obj->p), &(obj->numP), 
			    sizeof(limnPoint), LIMN_INCR);
    obj->vArr = airArrayNew((void**)&(obj->v), &(obj->numV),
			    sizeof(int), LIMN_INCR);
    obj->fArr = airArrayNew((void**)&(obj->f), &(obj->numF),
			    sizeof(limnFace), LIMN_INCR);
    if (!(obj->pArr && obj->vArr && obj->fArr)) {
      sprintf(err, "%s: couldn't allocate airArrays", me);
      biffSet(LIMN, err); return NULL;
    }
  }
  return obj;
}

int
limnNixObj(limnObj *obj) {
  char me[] = "limnNixObj", err[128];

  if (!obj) {
    sprintf(err, "%s: got NULL pointer\n", me);
    biffSet(LIMN, err); return 1;
  }
  airArrayNix(obj->pArr);
  airArrayNix(obj->vArr);
  airArrayNix(obj->fArr);
  free(obj);
  return 0;
}

int
limnNewPoint(limnObj *obj, float x, float y, float z) {
  char me[] = "limnNewPoint", err[512];
  int idx;

  if (!obj) {
    sprintf(err, BIFF_NULL, me); biffSet(LIMN, err); return(-1);
  }
  idx = obj->numP;
  airArrayIncrLen(obj->pArr, 1);
  obj->p[idx].w[0] = x;
  obj->p[idx].w[1] = y;
  obj->p[idx].w[2] = z;
  obj->p[idx].w[3] = 1;
  return(idx);
}

int
limnNewFace(limnObj *obj, int *vert, int numVert, float rgba[4]) {
  char me[] = "limnNewFace", err[512];
  int i, fidx, vidx;

  if (!(obj && vert)) {
    sprintf(err, BIFF_NULL, me); biffSet(LIMN, err); return(-1);
  }
  for (i=0; i<=numVert-1; i++) {
    if (!AIR_INSIDE(0, vert[i], obj->numP-1)) {
      sprintf(err, "%s: given vert[%d] = %d not in range [0, %d]\n",
	      me, i, vert[i], obj->numP-1);
      biffSet(LIMN, err); return(-1);
    }
  }  

  fidx = obj->numF;
  airArrayIncrLen(obj->fArr, 1);
  vidx = obj->numV;
  airArrayIncrLen(obj->vArr, numVert);
  if (rgba) {
    LINEAL_4COPY(obj->f[fidx].rgba, rgba);
  }
  else {
    LINEAL_4SET(obj->f[fidx].rgba, 1, 1, 1, 1);
  }
  obj->f[fidx].sides = numVert;
  obj->f[fidx].vidx = vidx;
  for (i=0; i<=numVert-1; i++) {
    obj->v[vidx + i] = vert[i];
  }

  return(fidx);
}

int
limnObjMerge(limnObj *o1, limnObj *o2) {
  char me[] = "limnObjMerge", err[512];
  int oldP, oldV, oldF, i;

  if (!(o1 && o2)) {
    sprintf(err, BIFF_NULL, me); biffSet(LIMN, err); return 1;
  }
  if (o1 == o2) {
    sprintf(err, "%s: can't merge an object with itself", me);
    biffSet(LIMN, err); return 1;
  }

  /* comments shmomments */
  oldP = o1->numP;
  airArrayIncrLen(o1->pArr, o2->numP);
  memcpy(o1->p + oldP, o2->p, (o2->numP)*sizeof(limnPoint));
  oldV = o1->numV;
  airArrayIncrLen(o1->vArr, o2->numV);
  memcpy(o1->v + oldV, o2->v, (o2->numV)*sizeof(int));
  for (i=oldV; i<=o1->numV-1; i++) {
    o1->v[i] += oldP;
  }
  oldF = o1->numF;
  airArrayIncrLen(o1->fArr, o2->numF);
  memcpy(o1->f + oldF, o2->f, (o2->numF)*sizeof(limnFace));
  for (i=oldF; i<=o1->numF-1; i++) {
    o1->f[i].vidx += oldV;
  }
  return 0;
}

limnObj *
limnObjCopy(limnObj *o) {
  char me[] = "limnObjCopy", err[128];
  limnObj *c;

  c = limnNewObj();
  if (!c) {
    sprintf(err, "%s: couldn't allocate object\n", me);
    biffAdd(LIMN, err); return NULL;
  }

  airArrayIncrLen(c->pArr, o->numP);
  memcpy(c->p, o->p, (o->numP)*sizeof(limnPoint));
  airArrayIncrLen(c->vArr, o->numV);
  memcpy(c->v, o->v, (o->numV)*sizeof(int));
  airArrayIncrLen(c->fArr, o->numF);
  memcpy(c->f, o->f, (o->numF)*sizeof(limnFace));
  c->ka = o->ka;
  c->kd = o->kd;
  c->ks = o->ks;
  c->shine = o->shine;
  
  return(c);
}
