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

#define COLDIR_ARGS(TYPE) echoCol_t lcol[3], echoPos_t ldir[3],          \
                          EchoLight##TYPE *light, int samp,              \
                          EchoIntx *intx, EchoThreadState *tstate

void
_echoLightColDirDirectional(COLDIR_ARGS(Directional)) {
  
  ELL_3V_COPY(lcol, light->col);
  ELL_3V_COPY(ldir, light->dir);
  return;
}

void
_echoLightColDirArea(COLDIR_ARGS(Area)) {
  EchoObjectRectangle *rect;
  echoPos_t at[3], *jitt, tmp;

  /* we assume light->obj is a EchoObjectRectangle with echoMatterLight */
  rect = (EchoObjectRectangle *)light->obj;
  ELL_3V_COPY(lcol, rect->mat);
  jitt = ((echoPos_t *)tstate->njitt->data) + samp*2*ECHO_SAMPLE_NUM;
  ELL_3V_COPY(at, rect->origin);
  tmp = jitt[0 + 2*echoSampleAreaLight] + 0.5;
  ELL_3V_SCALEADD(at, rect->edge0, tmp);
  tmp = jitt[1 + 2*echoSampleAreaLight] + 0.5;
  ELL_3V_SCALEADD(at, rect->edge1, tmp);
  ELL_3V_SUB(ldir, at, intx->pos);
  ELL_3V_SUB(ldir, intx->pos, at);
  ELL_3V_NORM(ldir, ldir, tmp);
  return;
}

typedef void (*_echoLightColDir_t)(COLDIR_ARGS());

_echoLightColDir_t
_echoLightColDir[ECHO_LIGHT_MAX+1] = {
  (_echoLightColDir_t)NULL,
  (_echoLightColDir_t)_echoLightColDirDirectional,
  (_echoLightColDir_t)_echoLightColDirArea,
};
  

/*
 define COLOR_ARGS echoCol_t *chan, EchoIntx *intx, int samp,       \
                   EchoParam *param, EchoThreadState *tstate,       \
                   EchoObject *obj, airArray *lightArr
*/

void
_echoIntxColorNone(COLOR_ARGS) {
  
  fprintf(stderr, "%s: can't color material 0 !!!\n", "_echoIntxColorNone");
}

void
_echoIntxColorPhong(COLOR_ARGS) {
  echoCol_t *mat,        /* pointer to object's material info */
    or, og, ob, oa,      /* object color and opacity */
    dr, dg, db,          /* ambient + diffuse contribution */
    sr, sg, sb,          /* specular contribution */
    ka, kd, ks, sh,      /* phong parameters */
    lcol[3];             /* light color */
  echoPos_t tmp,
    view[3],             /* unit-length view vector */
    norm[3],             /* unit-length normal */
    ldir[3],             /* unit-length light vector */
    refl[3];             /* unit-length reflection vector */
  int lt;
  EchoLight *light;

  mat = intx->obj->mat;
  oa = mat[echoMatterPhongAlpha];
  ka = mat[echoMatterKa];
  kd = mat[echoMatterPhongKd];
  ks = mat[echoMatterPhongKs];
  sh = mat[echoMatterPhongSh];

  ELL_3V_NORM(norm, intx->norm, tmp);
  ELL_3V_NORM(view, intx->view, tmp);
  ELL_3V_SCALE(refl, view, -1);
  tmp = 2*ELL_3V_DOT(view, norm);
  ELL_3V_SCALEADD(refl, norm, tmp);

  dr = ka*param->amR;
  dg = ka*param->amG;
  db = ka*param->amG;
  sr = sg = sb = 0.0;
  for (lt=0; lt<lightArr->len; lt++) {
    light = ((EchoLight **)lightArr->data)[lt];
    _echoLightColDir[light->type](lcol, ldir, light, samp, intx, tstate);
    tmp = kd*ELL_3V_DOT(ldir, norm);
    if (tmp > 0) {
      dr += lcol[0]*tmp;
      dg += lcol[1]*tmp;
      db += lcol[2]*tmp;
    }
    if (ks) {
      tmp = ELL_3V_DOT(ldir, refl);
      if (tmp > 0) {
	tmp = ks*pow(tmp, sh);
	sr += lcol[0]*tmp;
	sg += lcol[1]*tmp;
	sb += lcol[2]*tmp;
      }
    }
  }
  
  chan[0] = mat[echoMatterR]*dr + sr;
  chan[1] = mat[echoMatterG]*dg + sg;
  chan[2] = mat[echoMatterB]*db + sb;
  chan[3] = oa;
}


_echoIntxColor_t
_echoIntxColor[ECHO_MATTER_MAX+1] = {
  _echoIntxColorNone,
  _echoIntxColorPhong,
  NULL, /* glass */
  NULL, /* metal */
  NULL  /* light */
};

void
echoMatterPhongSet(EchoObject *obj,
		   echoCol_t r, echoCol_t g, echoCol_t b, echoCol_t a, 
		   echoCol_t ka, echoCol_t kd, echoCol_t ks, echoCol_t sh) {
  
  obj->matter = echoMatterPhong;
  obj->mat[echoMatterR] = r;
  obj->mat[echoMatterG] = g;
  obj->mat[echoMatterB] = b;
  obj->mat[echoMatterPhongAlpha] = a;
  obj->mat[echoMatterKa] = ka;
  obj->mat[echoMatterPhongKd] = kd;
  obj->mat[echoMatterPhongKs] = ks;
  obj->mat[echoMatterPhongSh] = sh;
}
		   
void
echoMatterGlassSet(EchoObject *obj,
		   echoCol_t r, echoCol_t g, echoCol_t b,
		   echoCol_t fuzzy, echoCol_t index, echoCol_t ka) {

  obj->matter = echoMatterGlass;
  obj->mat[echoMatterR] = r;
  obj->mat[echoMatterG] = g;
  obj->mat[echoMatterB] = b;
  obj->mat[echoMatterKa] = ka;
  obj->mat[echoMatterGlassFuzzy] = fuzzy;
  obj->mat[echoMatterGlassIndex] = index;
}

void
echoMatterMetalSet(EchoObject *obj,
		   echoCol_t r, echoCol_t g, echoCol_t b,
		   echoCol_t fuzzy, echoCol_t ka) {

  obj->matter = echoMatterMetal;
  obj->mat[echoMatterR] = r;
  obj->mat[echoMatterG] = g;
  obj->mat[echoMatterB] = b;
  obj->mat[echoMatterKa] = ka;
  obj->mat[echoMatterMetalFuzzy] = fuzzy;
}

void
echoMatterLightSet(EchoObject *obj,
		   echoCol_t r, echoCol_t g, echoCol_t b) {

  obj->matter = echoMatterLight;
  obj->mat[echoMatterR] = r;
  obj->mat[echoMatterG] = g;
  obj->mat[echoMatterB] = b;
}
		   
