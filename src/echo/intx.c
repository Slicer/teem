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

int
_echoRayIntxCubeTest(echoPos_t *tP, int *axP, int *dirP,
		     echoPos_t xmin, echoPos_t xmax,
		     echoPos_t ymin, echoPos_t ymax,
		     echoPos_t zmin, echoPos_t zmax,
		     EchoRay *ray) {
  echoPos_t txmin, tymin, tzmin, txmax, tymax, tzmax,
    dx, dy, dz, ox, oy, oz, tmin, tmax;
  int axmin, axmax;

  ELL_3V_GET(dx, dy, dz, ray->dir);
  ELL_3V_GET(ox, oy, oz, ray->from);
  if (dx > 0) { txmin = (xmin - ox)/dx; txmax = (xmax - ox)/dx; }
  else {        txmin = (xmax - ox)/dx; txmax = (xmin - ox)/dx; }
  if (dy > 0) { tymin = (ymin - oy)/dy; tymax = (ymax - oy)/dy; }
  else {        tymin = (ymax - oy)/dy; tymax = (ymin - oy)/dy; }
  if (dz > 0) { tzmin = (zmin - oz)/dz; tzmax = (zmax - oz)/dz; }
  else {        tzmin = (zmax - oz)/dz; tzmax = (zmin - oz)/dz; }
  if (txmin > tymin) { tmin = txmin; axmin = 0; }
  else {               tmin = tymin; axmin = 1; }
  if (tzmin > tmin) {  tmin = tzmin; axmin = 2; }
  if (txmax < tymax) { tmax = txmax; axmax = 0; }
  else {               tmax = tymax; axmax = 1; }
  if (tzmax < tmax) {  tmax = tzmax; axmax = 2; }
  if (tmin >= tmax)
    return AIR_FALSE;
  *tP = tmin;
  *axP = axmin;
  *dirP = 1;
  if (!AIR_INSIDE(ray->near, *tP, ray->far)) {
    *tP = tmax;
    *axP = axmax;
    *dirP = -1;
    if (!AIR_INSIDE(ray->near, *tP, ray->far)) {
      return AIR_FALSE;
    }
  }
  return AIR_TRUE;
}

int
_echoRayIntxCube(INTX_ARGS(Cube)) {
  echoPos_t t;
  int ax, dir;

  if (!_echoRayIntxCubeTest(&t, &ax, &dir,
			    -0.5, 0.5,
			    -0.5, 0.5,
			    -0.5, 0.5, ray)) 
    return AIR_FALSE;
  intx->obj = (EchoObject *)obj;
  intx->t = t;
  switch(ax) {
  case 0: ELL_3V_SET(intx->norm, dir, 0, 0); break;
  case 1: ELL_3V_SET(intx->norm, 0, dir, 0); break;
  case 2: ELL_3V_SET(intx->norm, 0, 0, dir); break;
  }
  intx->face = ax;
  if (0 && param->verbose) {
    printf("%s: ax = %d --> norm = (%g,%g,%g)\n",
	   "_echoRayIntxCube", ax,
	   intx->norm[0], intx->norm[1], intx->norm[2]);
  }
  /* set in intx:
     yes: t, norm, face
     no: u, v, view, pos
  */
  return AIR_TRUE;
}

void
_echoRayIntxUVCube(EchoIntx *intx, EchoRay *ray) {
  echoPos_t x, y, z;

  ELL_3V_GET(x, y, z, intx->pos);
  switch(intx->face) {
  case 0:
    intx->u = AIR_AFFINE(-0.5, y, 0.5, 0.0, 1.0);
    intx->v = AIR_AFFINE(-0.5, z, 0.5, 0.0, 1.0);
    break;
  case 1:
    intx->u = AIR_AFFINE(-0.5, x, 0.5, 0.0, 1.0);
    intx->v = AIR_AFFINE(-0.5, z, 0.5, 0.0, 1.0);
    break;
  case 2:
    intx->u = AIR_AFFINE(-0.5, x, 0.5, 0.0, 1.0);
    intx->v = AIR_AFFINE(-0.5, y, 0.5, 0.0, 1.0);
    break;
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
_echoRayIntxTriMesh(INTX_ARGS(TriMesh)) {
  echoPos_t t, edge0[3], edge1[3];
  EchoObjectTriMesh *trim;
  int ax, dir;

  trim = TRIMESH(obj);
  if (!_echoRayIntxCubeTest(&t, &ax, &dir,
			    trim->min[0], trim->max[0],
			    trim->min[1], trim->max[1],
			    trim->min[2], trim->max[2], ray)) {
    return AIR_FALSE;
  }
  
}

int
_echoRayIntxSplit(INTX_ARGS(Split)) {
  EchoObject *a, *b;
  echoPos_t *mina, *minb, *maxa, *maxb, t;
  int ret, ax, dir;

  if (ray->dir[obj->axis] > 0) {
    a = obj->obj0;
    mina = obj->min0;
    maxa = obj->max0;
    b = obj->obj1;
    minb = obj->min1;
    maxb = obj->max1;
  }
  else {
    a = obj->obj1;
    mina = obj->min1;
    maxa = obj->max1;
    b = obj->obj0;
    minb = obj->min0;
    maxb = obj->max0;
  }

  ret = AIR_FALSE;
  if (_echoRayIntxCubeTest(&t, &ax, &dir,
			   mina[0], maxa[0],
			   mina[1], maxa[1],
			   mina[2], maxa[2], ray)) {
    intx->boxhits++;
    if (_echoRayIntx[a->type](intx, ray, param, a)) {
      if (ray->shadow) {
	return AIR_TRUE;
      }
      ray->far = intx->t;
      ret = AIR_TRUE;
    }
  }
  if (_echoRayIntxCubeTest(&t, &ax, &dir,
			   minb[0], maxb[0],
			   minb[1], maxb[1],
			   minb[2], maxb[2], ray)) {
    intx->boxhits++;
    if (_echoRayIntx[b->type](intx, ray, param, b)) {
      ray->far = intx->t;
      ret = AIR_TRUE;
    }
  }
  return ret;
}

int
_echoRayIntxList(INTX_ARGS(List)) {
  int i, ret, ax, dir;
  EchoObject *kid;
  EchoObjectAABBox *box;
  echoPos_t t;

  if (echoObjectAABBox == obj->type) {
    box = AABBOX(obj);
    if (!_echoRayIntxCubeTest(&t, &ax, &dir,
			      box->min[0], box->max[0],
			      box->min[1], box->max[1],
			      box->min[2], box->max[2], ray)) {
      return AIR_FALSE;
    }
    intx->boxhits++;
  }
  
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
  (_echoRayIntx_t)_echoRayIntxCube,
  (_echoRayIntx_t)_echoRayIntxTriangle,
  (_echoRayIntx_t)_echoRayIntxRectangle,
  (_echoRayIntx_t)_echoRayIntxTriMesh,
  (_echoRayIntx_t)NULL,
  (_echoRayIntx_t)_echoRayIntxList,  /* trickery */
  (_echoRayIntx_t)_echoRayIntxSplit,
  (_echoRayIntx_t)_echoRayIntxList,
  (_echoRayIntx_t)NULL
};

_echoRayIntxUV_t
_echoRayIntxUV[ECHO_OBJECT_MAX+1] = {
  NULL,
  _echoRayIntxUVSphere,
  _echoRayIntxUVCube,
  _echoRayIntxUVNoop,
  _echoRayIntxUVNoop,
  NULL,
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
