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

/*
******** limnUpdateLights()
**
** copies information from the dir vectors to the _dir vectors
** This needs to be called even if there are no viewspace lights.
*/
int
limnUpdateLights(limnLight *lit, limnCam *cam) {
  char me[] = "limnUpdateLights", err[512];
  float dir[3], _dir[3];
  int i;
  
  if (!(lit && cam)) {
    sprintf(err, "%s: got NULL pointer\n", me);
    biffSet(LIMN, err); return 1;
  }
  if (limnSetUVN(cam)) {
    sprintf(err, "%s: trouble setting view matrix\n", me);
    return 1;
  }
  for (i=0; i<=lit->numLights-1; i++) {
    LINEAL_3COPY(dir, lit->dir[i]);
    if (lit->vsp[i]) {
      LINEAL_3MATMUL(_dir, cam->uvn, dir);
    }
    else {
      LINEAL_3COPY(_dir, dir);
    }
    lineal3Norm(_dir);
    LINEAL_3COPY(lit->_dir[i], _dir);
    printf("%s: light %d (%s): (%g,%g,%g) -> (%g,%g,%g)\n",
	   me, i, lit->on[i] ? "on" : "off",
	   lit->dir[i][0], 
	   lit->dir[i][1], 
	   lit->dir[i][2], 
	   lit->_dir[i][0], 
	   lit->_dir[i][1], 
	   lit->_dir[i][2]);
  }
  return 0;
}

/*
******** limnSetLight()
** 
** turns on another light
**
** returns -1 on error, index of new light (>=0) if okay
*/
int
limnSetLight(limnLight *lit, int vsp,
	     float r, float g, float b,
	     float x, float y, float z) {
  char me[]="limnSetLight", err[512];
  int i;

  if (!lit) {
    sprintf(err, "%s: got NULL pointer", me);
    biffSet(LIMN, err); return -1;
  }
  i = lit->numLights;
  if (i == LIMN_MAXLIT) {
    sprintf(err, "%s: have reached max of %d lights", me, i);
    biffSet(LIMN, err); return -1;
  }
  lit->on[i] = 1;
  lit->vsp[i] = vsp;
  LINEAL_3SET(lit->col[i], r, g, b);
  LINEAL_3SET(lit->dir[i], x, y, z);
  lit->numLights++;
  
  return i;
}

/*
******** limnSetAmbient()
**
** sets the ambient light color
**
** returns 1 on error, 0 if okay
*/
int
limnSetAmbient(limnLight *lit, float r, float g, float b) {
  char me[]="limnSetAmbient", err[512];
  
  if (!lit) {
    sprintf(err, "%s: got NULL pointer", me);
    biffSet(LIMN, err); return 1;
  }
  lit->ambR = r;
  lit->ambG = g;
  lit->ambB = b;

  return 0;
}

/*
******** limnTurnLight
**
** can toggle a light on/off
**
** returns 1 on error, 0 if okay
*/
int
limnTurnLight(limnLight *lit, int idx, int on) {
  char me[]="limnTurnLight", err[512];

  if (!lit) {
    sprintf(err, "%s: got NULL pointer", me);
    biffSet(LIMN, err); return 1;
  }
  if (!AIR_INSIDE(0, idx, lit->numLights-1)) {
    sprintf(err, "%s: light index %d out of range (0 to %d)", 
	    me, idx, lit->numLights-1);
    biffSet(LIMN, err); return 1;
  }
  lit->on[idx] = on;
  return 0;
}
