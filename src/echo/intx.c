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

/*
 define INTX_ARGS(TYPE) EchoIntx *intx, EchoRay *ray,               \
                        EchoParam *param, EchoObject##TYPE *obj
*/

#define SET_POS(intx, ray) \
  intx->pos[0] = ray->from[0] + intx->t*ray->dir[0]; \
  intx->pos[1] = ray->from[1] + intx->t*ray->dir[1]; \
  intx->pos[2] = ray->from[2] + intx->t*ray->dir[2]
  
int
_echoRayIntxSphere(INTX_ARGS(Sphere)) {
  echoPos_t t, A, B, C, r[3], dscr;

  ELL_3V_SUB(r, ray->from, obj->pos);
  A = ELL_3V_DOT(ray->dir, ray->dir);
  B = 2*ELL_3V_DOT(ray->dir, r);
  C = ELL_3V_DOT(r, r) - obj->rad*obj->rad;
  dscr = B*B - 4*A*C;
  if (dscr <= 0) {
    /* grazes or misses (most common case) */
    return AIR_FALSE;
  }
  /* else */
  dscr = sqrt(dscr);
  t = (-B - dscr)/(2*A);
  if (!AIR_INSIDE(ray->near, t, ray->far)) {
    t = (-B + dscr)/(2*A);
    if (!AIR_INSIDE(ray->near, t, ray->far)) {
      return AIR_FALSE;
    }
  }
  /* else one of the intxs is in [near,far] segment */
  intx->t = t;
  intx->pos[0] = ray->from[0] + t*ray->dir[0];
  intx->pos[1] = ray->from[1] + t*ray->dir[1];
  intx->pos[2] = ray->from[2] + t*ray->dir[2];
  ELL_3V_SUB(intx->norm, intx->pos, obj->pos);
  intx->obj = (EchoObject *)obj;
  /* set in intx:
     yes: t, norm, pos
     no: u, v, view
  */
  return AIR_TRUE;
}

void
_echoRayIntxUVSphere(EchoIntx *intx, EchoRay *ray) {
  echoPos_t len, u, v;

  ELL_3V_NORM(intx->norm, intx->norm, len);
  if (intx->norm[0] || intx->norm[1]) {
    u = atan2(intx->norm[1], intx->norm[0]);
    intx->u = AIR_AFFINE(-M_PI, u, M_PI, 0.0, 1.0);
    v = asin(intx->norm[2]);
    intx->v = AIR_AFFINE(-M_PI/2, v, M_PI/2, 0.0, 1.0);
  }
  else {
    intx->u = 0;
    intx->v = AIR_AFFINE(-1.0, intx->norm[2], 1.0, 0.0, 1.0);
  }
}

/*
** MINI_INTX
**
** given a triangle in terms of origin, edge0, edge1, this will
** begin the intersection calculation:
** - sets pvec, tvec, qvec (all of them if intx is not ruled out )
** - sets u, and rules out intx based on (u < 0.0 || u > 1.0)
** - sets v, and rules out intx based on COND
** - sets t, and rules out intx based on (t < near || t > far)
*/
#define MINI_INTX(ray, origin, edge0, edge1, pvec, qvec, tvec,               \
		  det, t, u, v, COND)                                        \
  ELL_3V_CROSS(pvec, ray->dir, edge1);                                       \
  det = ELL_3V_DOT(pvec, edge0);                                             \
  if (det > -ECHO_EPSILON && det < ECHO_EPSILON) {                           \
    return AIR_FALSE;                                                        \
  }                                                                          \
  /* now det is the reciprocal of the determinant */                         \
  det = 1.0/det;                                                             \
  ELL_3V_SUB(tvec, ray->from, origin);                                       \
  u = det * ELL_3V_DOT(pvec, tvec);                                          \
  if (u < 0.0 || u > 1.0) {                                                  \
    return AIR_FALSE;                                                        \
  }                                                                          \
  ELL_3V_CROSS(qvec, tvec, edge0);                                           \
  v = det * ELL_3V_DOT(qvec, ray->dir);                                      \
  if (COND) {                                                                \
    return AIR_FALSE;                                                        \
  }                                                                          \
  t = det * ELL_3V_DOT(qvec, edge1);                                         \
  if (t < ray->near || t > ray->far) {                                       \
    return AIR_FALSE;                                                        \
  }

int
_echoRayIntxRectangle(INTX_ARGS(Rectangle)) {
  echoPos_t pvec[3], qvec[3], tvec[3], det, t, u, v, *edge0, *edge1;
  
  if (echoMatterLight == obj->matter
      && (ray->shadow || !param->renderLights))
    return AIR_FALSE;
  edge0 = obj->edge0;
  edge1 = obj->edge1;
  MINI_INTX(ray, obj->origin, edge0, edge1,
	    pvec, qvec, tvec, det, t, u, v,
	    (v < 0.0 || v > 1.0));
  intx->t = t;
  intx->u = u;
  intx->v = v;
  ELL_3V_CROSS(intx->norm, edge0, edge1);
  intx->obj = (EchoObject *)obj;
  /* set in intx:
     yes: t, u, v, norm
     no: pos, view
  */
  return AIR_TRUE;
}

int
_echoRayIntxTriangle(INTX_ARGS(Triangle)) {
  echoPos_t pvec[3], qvec[3], tvec[3], det, t, u, v, edge0[3], edge1[3];
  
  ELL_3V_SUB(edge0, obj->vert[1], obj->vert[0]);
  ELL_3V_SUB(edge1, obj->vert[2], obj->vert[0]);
  MINI_INTX(ray, obj->vert[0], edge0, edge1,
	    pvec, qvec, tvec, det, t, u, v,
	    (v < 0.0 || u + v > 1.0));
  intx->t = t;
  intx->u = u;
  intx->v = v;
  ELL_3V_CROSS(intx->norm, edge0, edge1);
  intx->obj = (EchoObject *)obj;
  /* set in intx:
     yes: t, u, v, norm
     no: pos, view
  */
  return AIR_TRUE;
}

int
_echoRayIntxList(INTX_ARGS(List)) {
  int i, ret;
  EchoObject *kid;
  
  ret = AIR_FALSE;
  for (i=0; i<obj->objArr->len; i++) {
    kid = obj->obj[i];
    if (_echoRayIntx[kid->type](intx, ray,
				param, kid)) {
      if (ray->shadow) {
	/* we don't care where this hit is, or whether it is
	   the closest one; we hit something, so we're in shadow */
	return AIR_TRUE;
      }
      ray->far = intx->t;
      ret = AIR_TRUE;
    }
  }
  
  return ret;
}

void
_echoRayIntxUVNoop(EchoIntx *intx, EchoRay *ray) {

}
	      

_echoRayIntx_t
_echoRayIntx[ECHO_OBJECT_MAX+1] = {
  NULL,
  (_echoRayIntx_t)_echoRayIntxSphere,
  (_echoRayIntx_t)NULL,
  (_echoRayIntx_t)_echoRayIntxTriangle,
  (_echoRayIntx_t)_echoRayIntxRectangle,
  (_echoRayIntx_t)NULL,
  (_echoRayIntx_t)NULL,
  (_echoRayIntx_t)NULL,
  (_echoRayIntx_t)_echoRayIntxList,
  (_echoRayIntx_t)NULL
};

_echoRayIntxUV_t
_echoRayIntxUV[ECHO_OBJECT_MAX+1] = {
  NULL,
  _echoRayIntxUVSphere,
  NULL,
  _echoRayIntxUVNoop,
  _echoRayIntxUVNoop,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL
};


int
echoRayIntx(EchoIntx *intx, EchoRay *ray,
	    EchoParam *param, EchoObject *obj) {

  return _echoRayIntx[obj->type](intx, ray, param, obj);
}
