/*
  teem: Gordon Kindlmann's research software
  Copyright (C) 2003, 2002, 2001, 2000, 1999, 1998 University of Utah

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
******** limnCameraUpdate()
**
** sets in cam: W2V matrix, vspNeer, vspFaar, vspDist
**
** This does use biff to describe problems with camera settings
*/
int
limnCameraUpdate(limnCamera *cam) {
  char me[] = "limnCameraUpdate", err[129];
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
  if (cam->atRel) {
    /* ctx->cam->{neer,dist} are "at" relative */
    cam->vspNeer = cam->neer + len;
    cam->vspFaar = cam->faar + len;
    cam->vspDist = cam->dist + len;
  }
  else {
    /* ctx->cam->{neer,dist} are eye relative */
    cam->vspNeer = cam->neer;
    cam->vspFaar = cam->faar;
    cam->vspDist = cam->dist;
  }
  if (!(cam->vspNeer > 0 && cam->vspDist > 0 && cam->vspFaar > 0)) {
    sprintf(err, "%s: eye-relative near (%g), dist (%g), or far (%g) <= 0\n",
	    me, cam->vspNeer, cam->vspDist, cam->vspFaar);
    biffAdd(LIMN, err); return 1;
  }
  if (!(cam->vspNeer <= cam->vspFaar)) {
    sprintf(err, "%s: eye-relative near (%g) further than far (%g)\n",
	    me, cam->vspNeer, cam->vspFaar);
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

  if (cam->rightHanded) {
    ELL_3V_CROSS(v, n, u);
  }
  else {
    ELL_3V_CROSS(v, u, n);
  }

  ELL_4V_COPY(cam->U, u);
  ELL_4V_COPY(cam->V, v);
  ELL_4V_COPY(cam->N, n);
  ELL_4M_TRANSLATE_SET(T, -cam->from[0], -cam->from[1], -cam->from[2]);
  ELL_4M_ROWS_SET(R, u, v, n, l);
  ELL_4M_MUL(cam->W2V, R, T);
  ELL_4M_COPY(T, cam->W2V);
  ell_4m_inv_d(cam->V2W, T);

  return 0;
}
