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
  echoPos_t t, A, B, C, r[3], dscr;
     
  ELL_3V_SUB(r, from, obj->pos);
  A = ELL_3V_DOT(dir, dir);
  B = 2*ELL_3V_DOT(dir, r);
  C = ELL_3V_DOT(r, r) - obj->rad*obj->rad;
  dscr = B*B - 4*A*C;
  if (dscr <= 0) {
    /* grazes or misses (most common case) */
    return AIR_FALSE;
  }
  /* else */
  dscr = sqrt(dscr);
  t = (-B - dscr)/(2*A);
  if (!AIR_INSIDE(near, t, far)) {
    t = (-B + dscr)/(2*A);
    if (!AIR_INSIDE(near, t, far)) {
      return AIR_FALSE;
    }
  }
  /* else one of the intxs is in [near,far] segment */
  intx->t = t;
  ELL_3V_COPY(intx->pos, from);
  ELL_3V_SCALEADD(intx->pos, dir, t);
  ELL_3V_SUB(intx->norm, obj->pos, intx->pos);
  intx->obj = (EchoObject *)obj;
  return AIR_TRUE;
}

int
_echoRayIntxList(INTX_ARGS(List)) {
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

void
echoRayColor(echoCol_t *chan, int samp,
	     echoPos_t from[3], echoPos_t dir[3],
	     echoPos_t near, echoPos_t far,
	     EchoParam *param, EchoThreadState *tstate,
	     EchoObject *scene, airArray *lightArr) {
  
  EchoIntx intx;  /* NOT a pointer */

  chan[4] = airTime();
  if (_echoRayIntx[scene->type](&intx, from, dir, near, far, param, scene)) {
    chan[0] = 1.0;
    chan[1] = 0.0;
    chan[2] = 0.0;
    chan[3] = 1.0;
  }
  else {
    /* ray hits nothing in scene */
    chan[0] = 0.0;
    chan[1] = 0.0;
    chan[2] = 0.0;
    chan[3] = 0.0;
  }
  chan[4] = airTime() - chan[4];

  return;
}

	
