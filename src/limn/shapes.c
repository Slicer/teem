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


#include "limn.h"

int
limnObjCubeAdd(limnObj *obj, int sp) {
  int pb, v[4], ret;
  float x, y, z;

  x = y = z = 0.5;
  ret = limnObjPartStart(obj);
  /*
                                     7     6

                  z               4     5
                  |    y
                  |   /              3     2
                  |  /
                  | /             0     1
                    ------ x
  */
  pb = limnObjPointAdd(obj, 0, -x, -y, -z);
  limnObjPointAdd(obj, 0,  x, -y, -z);
  limnObjPointAdd(obj, 0,  x,  y, -z);
  limnObjPointAdd(obj, 0, -x,  y, -z);
  limnObjPointAdd(obj, 0, -x, -y,  z);
  limnObjPointAdd(obj, 0,  x, -y,  z);
  limnObjPointAdd(obj, 0,  x,  y,  z);
  limnObjPointAdd(obj, 0, -x,  y,  z);
  ELL_4V_SET(v, pb+3, pb+2, pb+1, pb+0);  limnObjFaceAdd(obj, sp, 4, v);
  ELL_4V_SET(v, pb+1, pb+5, pb+4, pb+0);  limnObjFaceAdd(obj, sp, 4, v);
  ELL_4V_SET(v, pb+2, pb+6, pb+5, pb+1);  limnObjFaceAdd(obj, sp, 4, v);
  ELL_4V_SET(v, pb+3, pb+7, pb+6, pb+2);  limnObjFaceAdd(obj, sp, 4, v);
  ELL_4V_SET(v, pb+0, pb+4, pb+7, pb+3);  limnObjFaceAdd(obj, sp, 4, v);
  ELL_4V_SET(v, pb+5, pb+6, pb+7, pb+4);  limnObjFaceAdd(obj, sp, 4, v);
  limnObjPartFinish(obj);

  return ret;
}

int
limnObjSquareAdd(limnObj *obj, int sp) {
  int pb, v[4], ret;
  float x, y;

  x = y = 0.5;
  ret = limnObjPartStart(obj);
  pb = limnObjPointAdd(obj, 0, -x, -y, 0);
  limnObjPointAdd(obj,      0,  x, -y, 0);
  limnObjPointAdd(obj,      0,  x,  y, 0);
  limnObjPointAdd(obj,      0, -x,  y, 0);
  ELL_4V_SET(v, pb+0, pb+1, pb+2, pb+3);  limnObjFaceAdd(obj, 2, 4, v);
  limnObjPartFinish(obj);

  return ret;
}

int
limnObjLoneEdgeAdd(limnObj *obj, int sp) {
  int pb, ret;
  float x;

  x = 0.5;
  ret = limnObjPartStart(obj);
  pb = limnObjPointAdd(obj, 0, -x, 0, 0);
  limnObjPointAdd(obj,      0,  x, 0, 0);
  limnObjEdgeAdd(obj, 1, -1, pb+0, pb+1);
  limnObjPartFinish(obj);

  return ret;
}

int
limnObjCylinderAdd(limnObj *obj, int sp, int res) {
  float x, y, th;
  int i, j, t, pb=-1, ret, *v;
  
  ret = limnObjPartStart(obj);
  v = (int *)calloc(res, sizeof(int));

  for (i=0; i<=res-1; i++) {
    th = AIR_AFFINE(0, i, res, 0, 2*M_PI);
    x = cos(th);
    y = sin(th);
    t = limnObjPointAdd(obj, 0, x, y, 1);
    if (!i)
      pb = t;
    limnObjPointAdd(obj, 0, x, y, -1);
  }
  for (i=0; i<=res-1; i++) {
    j = (i+1) % res;
    ELL_4V_SET(v, pb + 2*i, pb + 2*i + 1, pb + 2*j + 1, pb + 2*j);
    limnObjFaceAdd(obj, sp, 4, v);
  }
  for (i=0; i<=res-1; i++) {
    v[i] = pb + 2*i;
  }
  limnObjFaceAdd(obj, sp, res, v);
  for (i=0; i<=res-1; i++) {
    v[i] = pb + 2*(res-1-i) + 1;
  }
  limnObjFaceAdd(obj, sp, res, v);
  limnObjPartFinish(obj);
  
  free(v);
  return ret;
}


int
limnObjPolarSphereAdd(limnObj *obj, int sp, int thetaRes, int phiRes) {
  int ret, pb, nti, ti, pi, v[4], pl;
  float x, y, z, t, p;
  
  thetaRes = AIR_MAX(thetaRes, 3);
  phiRes = AIR_MAX(phiRes, 2);
  
  ret = limnObjPartStart(obj);
  pb = limnObjPointAdd(obj, 0, 0, 0, 1);
  for (pi=1; pi<=phiRes-1; pi++) {
    p = AIR_AFFINE(0, pi, phiRes, 0, M_PI);
    for (ti=0; ti<=thetaRes-1; ti++) {
      t = AIR_AFFINE(0, ti, thetaRes, 0, 2*M_PI);
      x = cos(t)*sin(p);
      y = sin(t)*sin(p);
      z = cos(p);
      limnObjPointAdd(obj, 0, x, y, z);
    }
  }
  pl = limnObjPointAdd(obj, 0, 0, 0, -1);
  for (ti=1; ti<=thetaRes; ti++) {
    nti = ti < thetaRes ? ti+1 : 1;
    ELL_3V_SET(v, pb+ti, pb+nti, pb+0);
    limnObjFaceAdd(obj, sp, 3, v);
  }
  for (pi=0; pi<=phiRes-3; pi++) {
    for (ti=1; ti<=thetaRes; ti++) {
      nti = ti < thetaRes ? ti+1 : 1;
      ELL_4V_SET(v, pb+pi*thetaRes + ti, pb+(pi+1)*thetaRes + ti,
		 pb+(pi+1)*thetaRes + nti, pb+pi*thetaRes + nti);
      limnObjFaceAdd(obj, sp, 4, v);
    }  
  }
  for (ti=1; ti<=thetaRes; ti++) {
    nti = ti < thetaRes ? ti+1 : 1;
    ELL_3V_SET(v, pb+pi*thetaRes + ti, pl, pb+pi*thetaRes + nti);
    limnObjFaceAdd(obj, sp, 3, v);
  }
  limnObjPartFinish(obj);

  return ret;
}

int
limnObjConeAdd(limnObj *obj, int sp, int res) {
  float th, x, y;
  int ret, t, pb=-1, i, j, *v;

  v = (int *)calloc(res, sizeof(int));

  ret = limnObjPartStart(obj);
  for (i=0; i<=res-1; i++) {
    th = AIR_AFFINE(0, i, res, 0, 2*M_PI);
    x = cos(th);
    y = sin(th);
    t = limnObjPointAdd(obj, 0, x, y, 0);
    if (!i)
      pb = t;
  }
  limnObjPointAdd(obj, 0, 0, 0, 1);
  for (i=0; i<=res-1; i++) {
    j = (i+1) % res;
    ELL_3V_SET(v, pb+i, pb+j, pb+res);
    limnObjFaceAdd(obj, sp, 3, v);
  }
  for (i=0; i<=res-1; i++) {
    v[i] = pb+res-1-i;
  }
  limnObjFaceAdd(obj, sp, res, v);
  limnObjPartFinish(obj);
  
  free(v);
  return ret;
}
