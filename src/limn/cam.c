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
******** limnCamUpdate()
**
** sets in cam: W2V matrix, vspNear, vspFar, vspDist
**
** This does use biff to describe problems with camera settings
*/
int
limnCamUpdate(limnCam *cam) {
  char me[] = "limnCamUpdate", err[129];
  float len, l[4], u[4], v[4], n[4], T[16], R[16];

  ELL_4V_SET(u, 0, 0, 0, 0);
  ELL_4V_SET(v, 0, 0, 0, 0);
  ELL_4V_SET(n, 0, 0, 0, 0);
  ELL_4V_SET(l, 0, 0, 0, 1);

  ELL_3V_SUB(n, cam->at, cam->from);
  len = ELL_3V_LEN(n);
  if (!len) {
    sprintf(err, "%s: cam->at (%g,%g,%g) == cam->from (%g,%g,%g)\n", me,
	    cam->at[0], cam->at[1], cam->at[2], 
	    cam->from[0], cam->from[1], cam->from[2]);
    biffAdd(LIMN, err); return 1;
  }
  if (cam->eyeRel) {
    /* ctx->cam->{near,dist} are eye relative */
    cam->vspNear = cam->near;
    cam->vspFar = cam->far;
    cam->vspDist = cam->dist;
  }
  else {
    /* ctx->cam->{near,dist} are "at" relative */
    cam->vspNear = cam->near + len;
    cam->vspFar = cam->far + len;
    cam->vspDist = cam->dist +len;
  }
  if (!(cam->vspNear >= 0 && cam->vspDist >= 0 && cam->vspFar >= 0)) {
    sprintf(err, "%s: eye-relative near (%g), dist (%g), or far (%g) < 0\n",
	    me, cam->vspNear, cam->vspDist, cam->vspFar);
    biffAdd(LIMN, err); return 1;
  }
  if (!(cam->vspNear <= cam->vspFar)) {
    sprintf(err, "%s: eye-relative near (%g) further than far (%g)\n",
	    me, cam->vspNear, cam->vspFar);
    biffAdd(LIMN, err); return 1 ;
  }
  ELL_3V_SCALE(n, n, 1.0/len);
  ELL_3V_CROSS(u, n, cam->up);
  len = ELL_3V_LEN(u);
  if (!len) {
    sprintf(err, "%s: cam->up is co-linear with view direction\n", me);
    biffAdd(LIMN, err); return 1 ;
  }
  ELL_3V_SCALE(u, u, 1.0/len);

  if (cam->leftHanded) {
    ELL_3V_CROSS(v, u, n);
  }
  else {
    ELL_3V_CROSS(v, n, u);
  }

  ELL_4M_SET_TRANSLATE(T, -cam->from[0], -cam->from[1], -cam->from[2]);
  ELL_4M_SET_ROWS(R, u, v, n, l);
  ELL_4M_MUL(cam->W2V, R, T);

  return 0;
}

