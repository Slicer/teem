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
    if (nrrdMaybeAlloc(map, nrrdTypeFloat, 3, 3, sx, sy)) {
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
  for (i=0; i<LIMN_LITE_NUM; i++) {
    if (!lit->on[i])
      continue;
    dot = ELL_3V_DOT(vec, lit->dir[i]);
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
** turns on a light
**
*/
void
limnLightSet(limnLight *lit, int which, int vsp,
	     float r, float g, float b,
	     float x, float y, float z) {
  
  if (lit && AIR_INSIDE(0, which, LIMN_LITE_NUM-1)) {
    lit->on[which] = 1;
    lit->vsp[which] = vsp;
    ELL_3V_SET(lit->col[which], r, g, b);
    ELL_3V_SET(lit->_dir[which], x, y, z);
  }
}

/*
******** limnLightSetAmbient()
**
** sets the ambient light color
*/
void
limnLightSetAmbient(limnLight *lit, float r, float g, float b) {
  
  if (lit) {
    ELL_3V_SET(lit->amb, r, g, b);
  }
}

/*
******** limnLightUpdate()
**
** copies information from the _dir vectors to the dir vectors. This
** needs to be called even if there are no viewspace lights, so that
** the dir vectors are set and normalized, in which case "cam" can be
** passed as NULL.
** 
** returns 1 if there was a problem in the camera, otherwise 0.
*/
int
limnLightUpdate(limnLight *lit, limnCam *cam) {
  char me[]="limnLightUpdate", err[AIR_STRLEN_MED];
  float dir[3], _dir[3], uvn[9], norm;
  int i;
  
  if (cam) {
    if (limnCamUpdate(cam)) {
      sprintf(err, "%s: trouble in camera", me);
      biffAdd(LIMN, err); return 1;
    }
    ELL_34M_EXTRACT(uvn, cam->V2W);
  }
  for (i=0; i<LIMN_LITE_NUM; i++) {
    ELL_3V_COPY(_dir, lit->_dir[i]);
    if (cam && lit->vsp[i]) {
      ELL_3MV_MUL(dir, uvn, _dir);
    }
    else {
      ELL_3V_COPY(dir, _dir);
    }
    ELL_3V_NORM(dir, dir, norm);
    ELL_3V_COPY(lit->dir[i], dir);
  }
  return 0;
}

/*
******** limnLightToggle
**
** can toggle a light on/off
**
** returns 1 on error, 0 if okay
*/
void
limnLightSwitch(limnLight *lit, int which, int on) {

  if (lit && AIR_INSIDE(0, which, LIMN_LITE_NUM-1)) {
    lit->on[which] = on;
  }
}

void
limnLightReset(limnLight *lit) {
  int i;

  if (lit) {
    ELL_3V_SET(lit->amb, 0, 0, 0);
    for (i=0; i<LIMN_LITE_NUM; i++) {
      ELL_3V_SET(lit->_dir[i], 0, 0, 0);
      ELL_3V_SET(lit->dir[i], 0, 0, 0);
      ELL_3V_SET(lit->col[i], 0, 0, 0);
      lit->on[i] = AIR_FALSE;
      lit->vsp[i] = AIR_FALSE;
    }
  }
}
