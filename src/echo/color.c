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
                          echoPos_t *distP, EchoParam *param,            \
                          EchoLight##TYPE *light, int samp,              \
                          EchoIntx *intx, EchoThreadState *tstate

void
_echoLightColDirDirectional(COLDIR_ARGS(Directional)) {
  
  ELL_3V_COPY(lcol, light->col);
  ELL_3V_COPY(ldir, light->dir);
  *distP = POS_MAX;
  return;
}

void
_echoLightColDirArea(COLDIR_ARGS(Area)) {
  EchoObjectRectangle *rect;
  echoPos_t at[3], *jitt, x, y;
  float colScale;

  /* we assume light->obj is a EchoObjectRectangle with echoMatterLight */
  rect = (EchoObjectRectangle *)light->obj;
  ELL_3V_COPY(lcol, rect->mat);
  jitt = ((echoPos_t *)tstate->njitt->data) + samp*2*ECHO_SAMPLE_NUM;
  x = jitt[0 + 2*echoSampleAreaLight] + 0.5;
  y = jitt[1 + 2*echoSampleAreaLight] + 0.5;
  at[0] = rect->origin[0] + x*rect->edge0[0] + y*rect->edge1[0];
  at[1] = rect->origin[1] + x*rect->edge0[1] + y*rect->edge1[1];
  at[2] = rect->origin[2] + x*rect->edge0[2] + y*rect->edge1[2];
  ELL_3V_SUB(ldir, at, intx->pos);
  *distP = ELL_3V_LEN(ldir);
  colScale = param->refDistance/(*distP);
  colScale *= colScale;
  ELL_3V_SCALE(lcol, lcol, colScale);
  ELL_3V_SCALE(ldir, ldir, 1.0/(*distP));
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
                   EchoObject *scene, airArray *lightArr
*/

void
_echoIntxColorNone(COLOR_ARGS) {
  
  fprintf(stderr, "%s: can't color material 0 !!!\n", "_echoIntxColorNone");
}

void
_echoIntxColorPhong(COLOR_ARGS) {
  echoCol_t *mat,        /* pointer to object's material info */
    oa,                  /* object opacity */
    dr, dg, db,          /* ambient + diffuse contribution */
    sr, sg, sb,          /* specular contribution */
    ka, kd, ks, sh,      /* phong parameters */
    lcol[3];             /* light color */
  echoPos_t tmp,
    view[3],             /* unit-length view vector */
    norm[3],             /* unit-length normal */
    ldir[3],             /* unit-length light vector */
    refl[3],             /* unit-length reflection vector */
    ldist;               /* distance to light */
  int lt;
  EchoLight *light;
  EchoRay shRay;         /* (not a pointer) */
  EchoIntx shIntx;       /* (not a pointer) */

  shRay.depth = 0;
  shRay.shadow = AIR_TRUE;

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
    _echoLightColDir[light->type](lcol, ldir, &ldist, param,
				  light, samp, intx, tstate);
    tmp = ELL_3V_DOT(ldir, norm);
    /*
    if (param->verbose) {
      printf("lcol = (%g,%g,%g), ldir = (%g,%g,%g)\n",
	     lcol[0], lcol[1], lcol[2], ldir[0], ldir[1], ldir[2]);
      printf("norm = (%g,%g,%g); dr,dg,db = (%g,%g,%g)\n", 
	     norm[0], norm[1], norm[2], dr, dg, db);
      printf("pos = (%g,%g,%g)\n", intx->pos[0], intx->pos[1], intx->pos[2]);
      printf("ldir . norm = %g\n", tmp);
    }
    */
    if (tmp <= 0)
      continue;
    if (param->shadow) {
      shRay.from[0] = intx->pos[0] + ECHO_EPSILON*norm[0];
      shRay.from[1] = intx->pos[1] + ECHO_EPSILON*norm[1];
      shRay.from[2] = intx->pos[2] + ECHO_EPSILON*norm[2];
      ELL_3V_COPY(shRay.dir, ldir);
      shRay.near = 0.0;
      shRay.far = ldist;
      if (_echoRayIntx[scene->type](&shIntx, &shRay, param, scene)) {
	/* the shadow ray hit something, nevermind */
	/*
	if (param->verbose) {
	  printf("shadow ray hit obj of type %d\n", shIntx.obj->type);
	  printf("hit obj %p, shading obj %p\n", shIntx.obj, intx->obj);
	}
	*/
	continue;
      }
      /*
      else {
	if (param->verbose) {
	  printf("shadow ray hit NOTHING\n");
	}
      }
      */
    }
    tmp *= kd;
    dr += lcol[0]*tmp;
    dg += lcol[1]*tmp;
    db += lcol[2]*tmp;
    /* NOTE: by including the specular stuff on this side of the
       "continue", we are disallowing specular highlights on the
       backsides of even semi-transparent surfaces. */
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
  /*
  if (param->verbose) {
    printf("mat[0,1,2] = %g,%g,%g ---> chan[0,1,2] = %g,%g,%g\n",
	   mat[echoMatterR], mat[echoMatterG], mat[echoMatterB],
	   chan[0], chan[1], chan[2]);
  }
  */
  chan[3] = oa;
}

void
_echoIntxColorLight(COLOR_ARGS) {
  echoCol_t *mat;        /* pointer to object's material info */
  
  mat = intx->obj->mat;
  chan[0] = mat[echoMatterR];
  chan[1] = mat[echoMatterG];
  chan[2] = mat[echoMatterB];
  chan[3] = 1.0;
}

_echoIntxColor_t
_echoIntxColor[ECHO_MATTER_MAX+1] = {
  _echoIntxColorNone,
  _echoIntxColorPhong,
  NULL, /* glass */
  NULL, /* metal */
  _echoIntxColorLight,
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
		   
