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

#include "echo.h"
#include "private.h"

int
_echoRayIntxSphere(INTX_ARGS(Sphere)) {
  echoPos_t t, A, B, C, r[3], dscr, eps;

  ELL_3V_SUB(r, from, obj->pos);
  A = ELL_3V_DOT(dir, dir);
  /*
  printf("!%s: r = (%g,%g,%g), dir = (%g,%g,%g)\n",
	 "_echoRayIntxSphere", r[0], r[1], r[2], dir[0], dir[1], dir[2]);
  */
  B = 2*ELL_3V_DOT(dir, r);
  C = ELL_3V_DOT(r, r) - obj->rad*obj->rad;
  dscr = B*B - 4*A*C;
  /*
  printf("!%s: near,far = %g,%g; dscr = %g\n",
	 "_echoRayIntxSphere", near, far, dscr);
  */
  if (dscr <= 0) {
    /* grazes or misses (most common case) */
    return AIR_FALSE;
  }
  /* else */
  dscr = sqrt(dscr);
  t = (-B - dscr)/(2*A);
  eps = -param->epsilon;
  if (!AIR_INSIDE(near, t, far)) {
    t = (-B + dscr)/(2*A);
    eps = -eps;
    if (!AIR_INSIDE(near, t, far)) {
      return AIR_FALSE;
    }
  }
  /* else one of the intxs is in [near,far] segment */
  intx->t = t + eps;
  ELL_3V_COPY(intx->pos, from);
  ELL_3V_SCALEADD(intx->pos, dir, t);
  ELL_3V_SUB(intx->norm, obj->pos, intx->pos);
  ELL_3V_SUB(intx->view, intx->pos, from);
  intx->obj = (EchoObject *)obj;
  return AIR_TRUE;
}

int
_echoRayIntxList(INTX_ARGS(List)) {
  char me[]="_echoRayIntxList";
  int i, ret;
  echoPos_t nearestFar;
  EchoObject *kid;
  
  nearestFar = far;
  ret = AIR_FALSE;
  for (i=0; i<obj->objArr->len; i++) {
    kid = obj->obj[i];
    if (_echoRayIntx[kid->type](intx,
				from, dir,
				near, nearestFar,
				param, kid)) {
      nearestFar = intx->t;
      ret = AIR_TRUE;
    }
  }
  
  return ret;
}

_echoRayIntx_t
_echoRayIntx[ECHO_OBJECT_MAX+1] = {
  NULL,
  (_echoRayIntx_t)_echoRayIntxSphere,
  (_echoRayIntx_t)NULL,
  (_echoRayIntx_t)NULL,
  (_echoRayIntx_t)NULL,
  (_echoRayIntx_t)NULL,
  (_echoRayIntx_t)NULL,
  (_echoRayIntx_t)NULL,
  (_echoRayIntx_t)_echoRayIntxList,
  (_echoRayIntx_t)NULL
};


int
echoRayIntx(EchoIntx *intx, 
	    echoPos_t from[3], echoPos_t dir[3],
	    echoPos_t near, echoPos_t far,
	    EchoParam *param, EchoObject *obj) {

  return _echoRayIntx[obj->type](intx, from, dir, near, far, param, obj);
}


	
