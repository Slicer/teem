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

int
_limnObjWHomog(limnObj *obj) {
  int pi;
  limnPoint *p;
  float h;
  
  for (pi=0; pi<=obj->pA->len-1; pi++) {
    p = obj->p + pi;
    h = 1.0/p->w[3];
    ELL_3V_SCALE(p->w, p->w, h);
    p->w[3] = 1.0;
  }
  
  return 0;
}

int
_limnObjNormals(limnObj *obj, int space) {
  int vn, vi, vb, fi;
  limnFace *f;
  limnPoint *p0, *p1, *p2;
  float e1[3], e2[3], x[3], n[3], norm;
  
  for (fi=0; fi<=obj->fA->len-1; fi++) {
    f = obj->f + fi;
    vn = f->vNum;
    vb = f->vBase;
    ELL_3V_SET(n, 0, 0, 0);
    for (vi=0; vi<vn; vi++) {
      p0 = obj->p + obj->v[vb + vi];
      p1 = obj->p + obj->v[vb + AIR_MOD(vi+1,vn)];
      p2 = obj->p + obj->v[vb + AIR_MOD(vi-1,vn)];
      if (limnSpaceWorld == space) {
	ELL_3V_SUB(e1, p1->w, p0->w);
	ELL_3V_SUB(e2, p2->w, p0->w);
      }
      else {
	ELL_3V_SUB(e1, p1->s, p0->s);
	ELL_3V_SUB(e2, p2->s, p0->s);
      }
      ELL_3V_CROSS(x, e1, e2);
      ELL_3V_ADD(n, n, x);
    }
    if (limnSpaceWorld == space) {
      ELL_3V_NORM(f->wn, n, norm);
    }
    else {
      ELL_3V_NORM(f->sn, n, norm);
    }
  }

  return 0;
}

int
_limnObjVTransform(limnObj *obj, limnCam *cam) {
  int pi;
  limnPoint *p;
  float d;

  for (pi=0; pi<=obj->pA->len-1; pi++) {
    p = obj->p + pi;
    ELL_4MV_MUL(p->v, cam->W2V, p->w);
    d = 1.0/p->v[3];
    ELL_4V_SCALE(p->v, p->v, d);
  }
  return 0;
}

int
_limnObjSTransform(limnObj *obj, limnCam *cam) {
  int pi;
  limnPoint *p;
  float d;

  for (pi=0; pi<=obj->pA->len-1; pi++) {
    p = obj->p + pi;
    d = cam->vspDist/p->v[2];
    p->s[0] = d*p->v[0];
    p->s[1] = d*p->v[1];
    p->s[2] = p->v[2];
  }
  return 0;
}

int
_limnObjDTransform(limnObj *obj, limnCam *cam, limnWin *win) {
  int pi;
  limnPoint *p;
  float wy0, wy1, wx0, wx1, t;
  
  wx0 = 0;
  wx1 = (cam->uMax - cam->uMin)*win->scale;
  wy0 = 0;
  wy1 = (cam->vMax - cam->vMin)*win->scale;
  ELL_4V_SET(win->bbox, wx0, wy0, wx1, wy1);
  if (win->yFlip) {
    ELL_SWAP2(wy0, wy1, t);
  }
  for (pi=0; pi<=obj->pA->len-1; pi++) {
    p = obj->p + pi;
    p->d[0] = AIR_AFFINE(cam->uMin, p->s[0], cam->uMax, wx0, wx1);
    p->d[1] = AIR_AFFINE(cam->vMin, p->s[1], cam->vMax, wy0, wy1);
  }
  return 0;
}

int
limnObjHomog(limnObj *obj, int space) {
  char me[]="limnObjHomog";
  int ret;

  switch(space) {
  case limnSpaceWorld:
    ret = _limnObjWHomog(obj);
    break;
  default:
    fprintf(stderr, "%s: space %d unknown or unimplemented\n", me, space);
    ret = 1;
    break;
  }
  
  return ret;
}

int
limnObjNormals(limnObj *obj, int space) {
  char me[]="limnObjNormals";
  int ret;
  
  switch(space) {
  case limnSpaceWorld:
  case limnSpaceScreen:
    ret = _limnObjNormals(obj, space);
    break;
  default:
    fprintf(stderr, "%s: space %d unknown or unimplemented\n", me, space);
    ret = 1;
    break;
  }

  return ret;
}

int
limnObjSpaceTransform(limnObj *obj, limnCam *cam, limnWin *win, int space) {
  char me[]="limnObjSpaceTransform";
  int ret;

  switch(space) {
  case limnSpaceView:
    ret = _limnObjVTransform(obj, cam);
    break;
  case limnSpaceScreen:
    ret = _limnObjSTransform(obj, cam);
    break;
  case limnSpaceDevice:
    ret = _limnObjDTransform(obj, cam, win);
    break;
  default:
    fprintf(stderr, "%s: space %d unknown or unimplemented\n", me, space);
    ret = 1;
    break;
  }

  return ret;
}

int
limnObjPartTransform(limnObj *obj, int ri, float tx[16]) {
  int pi, pn, pb;
  limnPart *r;
  limnPoint *p;
  float tmp[4];
  
  r = obj->r + ri;
  pb = r->pBase;
  pn = r->pNum;
  for (pi=pb; pi<=pb+pn-1; pi++) {
    p = obj->p + pi;
    ELL_4MV_MUL(tmp, tx, p->w);
    ELL_4V_COPY(p->w, tmp);
  }

  return 0;
}

int
_limnPartDepthCompare(void *_a, void *_b) {
  limnPart *a;
  limnPart *b;

  a = _a;
  b = _b;
  return AIR_COMPARE(b->z, a->z);
}

int
limnObjDepthSortParts(limnObj *obj) {
  limnPart *r;
  limnPoint *p;
  int pi, ri, rNum;

  rNum = obj->rA->len;
  for (ri=0; ri<=rNum-1; ri++) {
    r = obj->r + ri;
    r->z = 0;
    for (pi=0; pi<=r->pNum-1; pi++) {
      p = obj->p + r->pBase + pi;
      r->z += p->s[2];
    }
    r->z /= r->pNum;
  }
  
  qsort(obj->r, rNum, sizeof(limnPart), _limnPartDepthCompare);

  return 0;
}
