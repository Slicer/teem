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

/*
******** limnCamUpdate()
**
** sets in cam: W2V matrix, vspNear, vspFaar, vspDist
**
** This does use biff to describe problems with camera settings
*/
int
limnCamUpdate(limnCam *cam) {
  char me[] = "limnCamUpdate", err[129];
  double len, l[4], u[4], v[4], n[4], T[16], R[16];

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
    cam->vspFaar = cam->faar;
    cam->vspDist = cam->dist;
  }
  else {
    /* ctx->cam->{near,dist} are "at" relative */
    cam->vspNear = cam->near + len;
    cam->vspFaar = cam->faar + len;
    cam->vspDist = cam->dist +len;
  }
  if (!(cam->vspNear > 0 && cam->vspDist > 0 && cam->vspFaar > 0)) {
    sprintf(err, "%s: eye-relative near (%g), dist (%g), or far (%g) <= 0\n",
	    me, cam->vspNear, cam->vspDist, cam->vspFaar);
    biffAdd(LIMN, err); return 1;
  }
  if (!(cam->vspNear <= cam->vspFaar)) {
    sprintf(err, "%s: eye-relative near (%g) further than far (%g)\n",
	    me, cam->vspNear, cam->vspFaar);
    biffAdd(LIMN, err); return 1 ;
  }
  ELL_3V_SCALE(n, 1.0/len, n);
  ELL_3V_CROSS(u, n, cam->up);
  len = ELL_3V_LEN(u);
  if (!len) {
    sprintf(err, "%s: cam->up is co-linear with view direction\n", me);
    biffAdd(LIMN, err); return 1 ;
  }
  ELL_3V_SCALE(u, 1.0/len, u);

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

