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

#define LIMN_INCR 50

limnObj *
limnObjNew(int edges) {
  limnObj *obj;

  obj = (limnObj *)calloc(1, sizeof(limnObj));

  /* create all various airArrays */
  obj->pA = airArrayNew((void**)&(obj->p), NULL, 
			sizeof(limnPoint), LIMN_INCR);
  obj->vA = airArrayNew((void**)&(obj->v), NULL,
			sizeof(int), LIMN_INCR);
  obj->eA = airArrayNew((void**)&(obj->e), NULL,
			sizeof(limnEdge), LIMN_INCR);
  obj->fA = airArrayNew((void**)&(obj->f), NULL,
			sizeof(limnFace), LIMN_INCR);
  obj->rA = airArrayNew((void**)&(obj->r), NULL,
			sizeof(limnPart), LIMN_INCR);
  obj->sA = airArrayNew((void**)&(obj->s), NULL,
			sizeof(limnSP), LIMN_INCR);
  obj->rCurr = NULL;

  /* initialize three sp's, for points, edges, and faces */
  airArrayIncrLen(obj->sA, 3);

  obj->edges = edges;
    
  return obj;
}

limnObj *
limnObjNuke(limnObj *obj) {

  airArrayNuke(obj->pA);
  airArrayNuke(obj->vA);
  airArrayNuke(obj->eA);
  airArrayNuke(obj->fA);
  airArrayNuke(obj->rA);
  airArrayNuke(obj->sA);
  free(obj);
  return NULL;
}

int
limnObjPartBegin(limnObj *obj) {
  int rBase;
  limnPart *r;

  rBase = airArrayIncrLen(obj->rA, 1);
  r = &(obj->r[rBase]);
  r->fBase = obj->fA->len;  r->fNum = 0;
  r->eBase = obj->eA->len;  r->eNum = 0;
  r->pBase = obj->pA->len;  r->pNum = 0;
  r->origIdx = rBase;
  obj->rCurr = r;

  return rBase;
}

int
limnObjPointAdd(limnObj *obj, int sp, float x, float y, float z) {
  limnPoint *p;
  int pBase;

  pBase = airArrayIncrLen(obj->pA, 1);
  p = &(obj->p[pBase]);
  ELL_4V_SET(p->w, x, y, z, 1);
  ELL_3V_SET(p->v, AIR_NAN, AIR_NAN, AIR_NAN);
  ELL_3V_SET(p->s, AIR_NAN, AIR_NAN, AIR_NAN);
  ELL_3V_SET(p->n, AIR_NAN, AIR_NAN, AIR_NAN);
  p->sp = sp;
  obj->rCurr->pNum++;

  return pBase;
}

void
_limnEdgeInit(limnEdge *e, int sp, int face, int v0, int v1) {

  e->v0 = v0;
  e->v1 = v1;
  e->f0 = face;
  e->f1 = -1;
  e->sp = sp;
  e->visib = 0;
}

int
limnObjEdgeAdd(limnObj *obj, int sp, int face, int v0, int v1) {
  int ret, t, i, eNum, eBase;
  limnEdge *e;
  
  eBase = obj->rCurr->eBase;
  eNum = obj->rCurr->eNum;
  
  if (v0 > v1) {
    ELL_SWAP2(v0, v1, t);
  }
  
  /* do a linear search through this part's edges */
  for (i=0; i<=eNum-1; i++) {
    e = &(obj->e[eBase+i]);
    if (e->v0 == v0 && e->v1 == v1) {
      break;
    }
  }
  if (i == eNum) {
    /* edge not found */
    eBase = airArrayIncrLen(obj->eA, 1);
    e = &(obj->e[eBase]);
    _limnEdgeInit(e, sp, face, v0, v1);
    ret = eBase;
    obj->rCurr->eNum++;
  }
  else {
    /* edge already exists */
    e->f1 = face;
    ret = eBase+i;
  }

  return ret;
}

int
limnObjFaceAdd(limnObj *obj, int sp, int numVert, int *vert) {
  int i, vBase, fBase;
  limnFace *f;

  fBase = airArrayIncrLen(obj->fA, 1);
  vBase = airArrayIncrLen(obj->vA, numVert);
  
  f = &(obj->f[fBase]);
  f->vBase = vBase;
  f->vNum = numVert;
  for (i=0; i<=numVert-1; i++) {
    obj->v[vBase + i] = vert[i];
    if (obj->edges) {
      limnObjEdgeAdd(obj, 1, fBase, vert[i], vert[AIR_MOD(i+1, numVert)]);
    }
  }
  f->sp = sp;
  obj->rCurr->fNum++;
  
  return fBase;
}

int
limnObjPartEnd(limnObj *obj) {
  
  obj->rCurr = NULL;
  
  return 0;
}

int
limnObjSPAdd(limnObj *obj) {

  return airArrayIncrLen(obj->sA, 1);
}

