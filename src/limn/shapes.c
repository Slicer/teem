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

limnObj *
limnNewCube() {
  char me[] = "limnNewCube", err[128];
  limnObj *o;
  int v[4];
  float x, y, z;

  o = limnNewObj();
  if (!o) {
    sprintf(err, "%s: couldn't allocate object\n", me);
    biffAdd(LIMN, err); return NULL;
  }

  x = y = z = 1;
  limnNewPoint(o, -x, -y, -z);
  limnNewPoint(o,  x, -y, -z);
  limnNewPoint(o,  x,  y, -z);
  limnNewPoint(o, -x,  y, -z);
  limnNewPoint(o, -x, -y,  z);
  limnNewPoint(o,  x, -y,  z);
  limnNewPoint(o,  x,  y,  z);
  limnNewPoint(o, -x,  y,  z);
  LINEAL_4SET(v, 3, 2, 1, 0);  limnNewFace(o, v, 4, NULL);
  LINEAL_4SET(v, 1, 5, 4, 0);  limnNewFace(o, v, 4, NULL);
  LINEAL_4SET(v, 2, 6, 5, 1);  limnNewFace(o, v, 4, NULL);
  LINEAL_4SET(v, 3, 7, 6, 2);  limnNewFace(o, v, 4, NULL);
  LINEAL_4SET(v, 0, 4, 7, 3);  limnNewFace(o, v, 4, NULL);
  LINEAL_4SET(v, 5, 6, 7, 4);  limnNewFace(o, v, 4, NULL);

  return o;
}

limnObj *
limnNewSphere(int thetaRes, int phiRes) {
  char me[] = "limnNewSphere", err[128];
  limnObj *o;
  int nti, ti, pi, v[4], last;
  float x, y, z, t, p;
  
  o = limnNewObj();
  if (!o) {
    sprintf(err, "%s: couldn't allocate object", me);
    biffAdd(LIMN, err); return NULL;
  }
  if (thetaRes < 3) {
    sprintf(err, "%s: need thetaRes at least 3 (not %d)", me, thetaRes);
    biffAdd(LIMN, err); return NULL;
  }
  if (phiRes < 2) {
    sprintf(err, "%s: need phiRes at least 2 (not %d)", me, phiRes);
    biffAdd(LIMN, err); return NULL;
  }
  
  limnNewPoint(o, 0, 0, 1);
  for (pi=1; pi<=phiRes-1; pi++) {
    p = AIR_AFFINE(0, pi, phiRes, 0, M_PI);
    for (ti=0; ti<=thetaRes-1; ti++) {
      t = AIR_AFFINE(0, ti, thetaRes, 0, 2*M_PI);
      x = cos(t)*sin(p);
      y = sin(t)*sin(p);
      z = cos(p);
      limnNewPoint(o, x, y, z);
    }
  }
  last = limnNewPoint(o, 0, 0, -1);
  for (ti=1; ti<=thetaRes; ti++) {
    nti = ti < thetaRes ? ti+1 : 1;
    LINEAL_3SET(v, ti, nti, 0);
    limnNewFace(o, v, 3, NULL);
  }
  for (pi=0; pi<=phiRes-3; pi++) {
    for (ti=1; ti<=thetaRes; ti++) {
      nti = ti < thetaRes ? ti+1 : 1;
      LINEAL_4SET(v, pi*thetaRes + ti, (pi+1)*thetaRes + ti,
		  (pi+1)*thetaRes + nti, pi*thetaRes + nti);
      limnNewFace(o, v, 4, NULL);
    }  
  }
  for (ti=1; ti<=thetaRes; ti++) {
    nti = ti < thetaRes ? ti+1 : 1;
    LINEAL_3SET(v, pi*thetaRes + ti, last, pi*thetaRes + nti);
    limnNewFace(o, v, 3, NULL);
  }

  return o;
}

limnObj *
limnNewCylinder(int res) {
  char me[] = "limnNewCylinder", err[128];
  limnObj *o;
  float x, y, th;
  int *v, i, j;
  
  o = limnNewObj();
  if (!o) {
    sprintf(err, "%s: couldn't allocate object", me);
    biffAdd(LIMN, err); return NULL;
  }
  if (res < 3) {
    sprintf(err, "%s: need resolution at least 3 (not %d)", me, res);
    biffAdd(LIMN, err); return NULL;
  }
  v = (int *)calloc(res, sizeof(int));
  if (!v) {
    sprintf(err, "%s: couldn't calloc %d ints", me, res);
    biffAdd(LIMN, err); return NULL;
  }

  for (i=0; i<=res-1; i++) {
    th = AIR_AFFINE(0, i, res, 0, 2*M_PI);
    x = cos(th);
    y = sin(th);
    limnNewPoint(o, x, y, 1);
    limnNewPoint(o, x, y, -1);
  }
  for (i=0; i<=res-1; i++) {
    j = (i+1) % res;
    LINEAL_4SET(v, 2*i, 2*i + 1, 2*j + 1, 2*j);
    limnNewFace(o, v, 4, NULL);
  }
  for (i=0; i<=res-1; i++) {
    v[i] = 2*i;
  }
  limnNewFace(o, v, res, NULL);
  for (i=0; i<=res-1; i++) {
    v[i] = 2*(res-1-i) + 1;
  }
  limnNewFace(o, v, res, NULL);
  
  free(v);
  return o;
}

limnObj *
limnNewCone(int res) {
  float th, x, y;
  char me[]="limnNewCone", err[128];
  limnObj *o;
  int i, j, *v;

  o = limnNewObj();
  if (!o) {
    sprintf(err, "%s: couldn't allocate object", me);
    biffAdd(LIMN, err); return NULL;
  }
  if (res < 3) {
    sprintf(err, "%s: need resolution at least 3 (not %d)", me, res);
    biffAdd(LIMN, err); return NULL;
  }
  v = (int *)calloc(res, sizeof(int));
  if (!v) {
    sprintf(err, "%s: couldn't calloc %d ints", me, res);
    biffAdd(LIMN, err); return NULL;
  }

  for (i=0; i<=res-1; i++) {
    th = AIR_AFFINE(0, i, res, 0, 2*M_PI);
    x = cos(th);
    y = sin(th);
    limnNewPoint(o, x, y, 0);
  }
  limnNewPoint(o, 0, 0, 1);
  for (i=0; i<=res-1; i++) {
    j = (i+1) % res;
    LINEAL_3SET(v, i, j, res);
    limnNewFace(o, v, 3, NULL);
  }
  for (i=0; i<=res-1; i++) {
    v[i] = res-1-i;
  }
  limnNewFace(o, v, res, NULL);
  
  free(v);
  return o;

}

limnObj *
limnNewArrow(int res, float L, float r, float R, float hasp,
	     float pos[3], float vec[3]) {
  limnObj *c, *h;
  float s[16], t[16], z[3], headLen, fullLen;

  headLen = R*hasp;
  fullLen = L*LINEAL_3LEN(vec);
  res = AIR_MAX(3, res);
  
  /* make the shaft */
  c = limnNewCylinder(res);
  limnMatxTranslate(t, 0, 0, 1);
  limnMatxScale(s, r, r, (fullLen-headLen)/2);
  limnMatxMult(s, t);
  limnMatxApply(c, s);

  /* make the head */
  h = limnNewCone(res);
  limnMatxScale(s, R, R, headLen);
  limnMatxTranslate(t, 0, 0, fullLen-headLen);
  limnMatxMult(t, s);
  limnMatxApply(h, t);

  /* merge them */
  limnObjMerge(c, h);
  limnNixObj(h);

  /* rotate and translate arrow */
  LINEAL_3SET(z, 0, 0, 1);
  limnMatxRotateUtoV(s, z, vec);
  limnMatxTranslate(t, pos[0], pos[1], pos[2]);
  limnMatxMult(t, s);
  limnMatxApply(c, t);
  
  return(c);
}

limnObj *
limnNewAxes(int res, float radius) {
  limnObj *cx, *cy, *cz, *b1, *b2, *b3;
  float r[16], s[16], t[16];

  res = AIR_MAX(3, res);

  cx = limnNewCylinder(res);
  limnMatxScale(s, radius, radius, 0.5);
  limnMatxApply(cx, s);
  limnMatxTranslate(t, 0, 0, 0.5);
  limnMatxApply(cx, t);
  
  cy = limnObjCopy(cx);
  limnMatxRotateX(r, -M_PI/2);
  limnMatxApply(cy, r);
  cz = limnObjCopy(cx);
  limnMatxRotateY(r, +M_PI/2);
  limnMatxApply(cz, r);

  limnObjMerge(cx, cy);
  limnObjMerge(cx, cz);
  limnNixObj(cy);
  limnNixObj(cz);

  b1 = limnNewSphere(res, AIR_MAX(2, res/2));
  limnMatxScale(s, radius, radius, radius);
  limnMatxApply(b1, s);
  b2 = limnObjCopy(b1);
  b3 = limnObjCopy(b1);

  limnMatxTranslate(t, 0, 1 + 2*radius, 0);
  limnMatxApply(b1, t);
  limnMatxTranslate(t, 0, 0, 1 + 2*radius);
  limnMatxApply(b2, t);
  limnMatxTranslate(t, 0, 0, 1 + 5*radius);
  limnMatxApply(b3, t);

  limnObjMerge(cx, b1);
  limnObjMerge(cx, b2);
  limnObjMerge(cx, b3);
  limnNixObj(b1);
  limnNixObj(b2);
  limnNixObj(b3);

  /*
  b = limnNewCube();  limnMatxApply(b, s);
  limnMatxTranslate(t, 0, 0, 1);
  limnMatxApply(b, t);
  limnObjMerge(a, b);
  limnNixObj(b);
  */
  
  return(cx);
}
