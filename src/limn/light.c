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
limnEnvMapFill(Nrrd *map, limnEnvMapCB cb, void *data, int qnMethod) {
  char me[]="limnEnvMapFill", err[128];
  int sx, sy;
  int qn;
  float vec[3], *mapData;

  if (!(map && cb)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(LIMN, err); return 1;
  }
  if (!AIR_BETWEEN(limnQN_Unknown, qnMethod, limnQN_Last)) {
    sprintf(err, "%s: QN method %d invalid", me, qnMethod);
    biffAdd(LIMN, err); return 1;
  }
  switch(qnMethod) {
  case limnQN_16checker:
    sx = sy = 256;
    if (nrrdMaybeAlloc_va(map, nrrdTypeFloat, 3, 3, sx, sy)) {
      sprintf(err, "%s: couldn't alloc output", me);
      biffMove(LIMN, err, NRRD); return 1;
    }
    mapData = map->data;
    for (qn=0; qn<=sx*sy-1; qn++) {
      limnQNtoV[limnQN_16checker](vec, qn, AIR_TRUE);
      cb(mapData + 3*qn, vec, data);
    }
    break;
  default:
    sprintf("%s: sorry, QN method %d not implemented", me, qnMethod);
    biffAdd(LIMN, err); return 1;
  }

  return 0;
}

/*
******** limnLightUpdate()
**
** copies information from the dir vectors to the _dir vectors
** This needs to be called even if there are no viewspace lights,
** in which case "cam" can be passed as NULL
*/
int
limnLightUpdate(limnLight *lit, limnCam *cam) {
  float dir[3], _dir[3], uvn[9], norm;
  int i;
  
  if (cam) {
    limnCamUpdate(cam);
    ELL_34M_EXTRACT(uvn, cam->W2V);
  }
  for (i=0; i<=lit->lNum-1; i++) {
    ELL_3V_COPY(dir, lit->dir[i]);
    if (cam && lit->vsp[i]) {
      ELL_3MV_TMUL(_dir, uvn, dir);
    }
    else {
      ELL_3V_COPY(_dir, dir);
    }
    ELL_3V_NORM(_dir, _dir, norm);
    ELL_3V_COPY(lit->_dir[i], _dir);
  }
  return 0;
}

void
limnLightDiffuseCB(float rgb[3], float vec[3], void *_lit) {
  float dot, r, g, b, norm;
  limnLight *lit;
  int i;

  lit = _lit;
  ELL_3V_NORM(vec, vec, norm);
  r = lit->amb[0];
  g = lit->amb[1];
  b = lit->amb[2];
  for (i=0; i<lit->lNum; i++) {
    dot = ELL_3V_DOT(vec, lit->_dir[i]);
    dot = AIR_MAX(0, dot);
    r += dot*lit->col[i][0];
    g += dot*lit->col[i][1];
    b += dot*lit->col[i][2];
  }
  rgb[0] = AIR_CLAMP(0, r, 1);
  rgb[1] = AIR_CLAMP(0, g, 1);
  rgb[2] = AIR_CLAMP(0, b, 1);
}


/*
******** limnLightSet()
** 
** turns on another light
**
** returns -1 on error, index of new light (>=0) if okay
*/
int
limnLightSet(limnLight *lit, int vsp,
	     float r, float g, float b,
	     float x, float y, float z) {
  int i;

  i = lit->lNum;
  if (i == LIMN_MAXLIT) {
    fprintf(stderr, "limnLightSet: reached max of %d lights!", i);
    return -1;
  }
  lit->on[i] = 1;
  lit->vsp[i] = vsp;
  ELL_3V_SET(lit->col[i], r, g, b);
  ELL_3V_SET(lit->dir[i], x, y, z);
  lit->lNum++;
  
  return i;
}

/*
******** limnLightSetAmbient()
**
** sets the ambient light color
**
** returns 1 on error, 0 if okay
*/
int
limnLightSetAmbient(limnLight *lit, float r, float g, float b) {
  
  ELL_3V_SET(lit->amb, r, g, b);

  return 0;
}

/*
******** limnLightToggle
**
** can toggle a light on/off
**
** returns 1 on error, 0 if okay
*/
int
limnLightToggle(limnLight *lit, int idx, int on) {

  if (!AIR_INSIDE(0, idx, lit->lNum-1)) {
    fprintf(stderr, "limnLightToggle: light index %d out of range (0 to %d)", 
	    idx, lit->lNum-1);
  }
  lit->on[idx] = on;

  return 0;
}
