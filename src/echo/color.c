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
                          echoPos_t *distP, EchoParm *parm,            \
                          EchoLight##TYPE *light, int samp,              \
                          EchoIntx *intx, EchoThreadState *tstate

void
_echoLightColDirDirectional(COLDIR_ARGS(Directional)) {
  
  ELL_3V_COPY(lcol, light->col);
  ELL_3V_COPY(ldir, light->dir);
  *distP = ECHO_POS_MAX;
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
  ELL_3V_SCALEADD3(at, 1.0, rect->origin, x, rect->edge0, y, rect->edge1);
  ELL_3V_SUB(ldir, at, intx->pos);
  *distP = ELL_3V_LEN(ldir);
  colScale = parm->refDistance/(*distP);
  colScale *= colScale*rect->area;
  ELL_3V_SCALE(lcol, colScale, lcol);
  ELL_3V_SCALE(ldir, 1.0/(*distP), ldir);
  return;
}

typedef void (*_echoLightColDir_t)(COLDIR_ARGS( ));

_echoLightColDir_t
_echoLightColDir[ECHO_LIGHT_MAX+1] = {
  (_echoLightColDir_t)NULL,
  (_echoLightColDir_t)_echoLightColDirDirectional,
  (_echoLightColDir_t)_echoLightColDirArea,
};
  

/*
 define COLOR_ARGS echoCol_t *chan, EchoIntx *intx, int samp,       \
                   EchoParm *parm, EchoThreadState *tstate,       \
                   EchoObject *scene, airArray *lightArr
*/

void
_echoIntxColorNone(COLOR_ARGS) {
  
  fprintf(stderr, "%s: can't color material 0 !!!\n", "_echoIntxColorNone");
}

void
_echoIntxUVColor(echoCol_t *chan, EchoIntx *intx) {
  int ui, vi, su, sv;
  unsigned char *tdata;
  echoCol_t *mat;

  mat = intx->obj->mat;
  if (intx->obj->ntext) {
    _echoRayIntxUV[intx->obj->type](intx);
    su = intx->obj->ntext->axis[1].size;
    sv = intx->obj->ntext->axis[2].size;
    AIR_INDEX(0.0, intx->u, 1.0, su, ui); ui = AIR_CLAMP(0, ui, su-1);
    AIR_INDEX(0.0, intx->v, 1.0, sv, vi); vi = AIR_CLAMP(0, vi, sv-1);
    tdata = intx->obj->ntext->data;
    chan[0] = mat[echoMatterR]*(tdata[0 + 4*(ui + su*vi)]/255.0);
    chan[1] = mat[echoMatterG]*(tdata[1 + 4*(ui + su*vi)]/255.0);
    chan[2] = mat[echoMatterB]*(tdata[2 + 4*(ui + su*vi)]/255.0);
    chan[3] = 1.0;
  }
  else {
    chan[0] = mat[echoMatterR];
    chan[1] = mat[echoMatterG];
    chan[2] = mat[echoMatterB];
    chan[3] = 1.0;
  }
}

void
_echoIntxColorPhong(COLOR_ARGS) {
  echoCol_t *mat,        /* pointer to object's material info */
    icol[4],             /* "intersection color" */
    oa,                  /* object opacity */
    d[3], s[3],          /* ambient + diffuse, specular */
    ka, kd, ks, sh,      /* phong parmeters */
    lcol[3];             /* light color */
  echoPos_t tmp,
    view[3],             /* unit-length view vector */
    norm[3],             /* unit-length normal */
    ldir[3],             /* unit-length light vector */
    refl[3],             /* unit-length reflection vector */
    ldist;               /* distance to light */
  int lt;
  EchoLight *light;
  EchoRay shRay;
  EchoIntx shIntx;

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
  tmp = 2*ELL_3V_DOT(view, norm);
  ELL_3V_SCALEADD(refl, -1, view, tmp, norm);

  d[0] = ka*parm->amR;
  d[1] = ka*parm->amG;
  d[2] = ka*parm->amG;
  s[0] = s[1] = s[2] = 0.0;
  ELL_3V_COPY(shRay.from, intx->pos);
  shRay.near = ECHO_EPSILON;
  for (lt=0; lt<lightArr->len; lt++) {
    light = ((EchoLight **)lightArr->data)[lt];
    _echoLightColDir[light->type](lcol, ldir, &ldist, parm,
				  light, samp, intx, tstate);
    tmp = ELL_3V_DOT(ldir, norm);
    if (tmp <= 0)
      continue;
    if (parm->shadow) {
      ELL_3V_COPY(shRay.dir, ldir);
      shRay.far = ldist;
      if (parm->verbose) {
	printf("%d: from (%g,%g,%g) to (%g,%g,%g) for [%g,%g] (scene %d)\n",
	       lt, shRay.from[0], shRay.from[1], shRay.from[2],
	       shRay.dir[0], shRay.dir[1], shRay.dir[2],
	       shRay.near, shRay.far, scene->type);
      }
      if (_echoRayIntx[scene->type](&shIntx, &shRay, parm, scene)) {
	/* the shadow ray hit something, nevermind */
	if (parm->verbose) {
	  printf("       SHADOWED\n");
	}
	continue;
      }
      if (parm->verbose) {
	printf(" I see the light\n");
      }
    }
    tmp *= kd;
    ELL_3V_SCALEADD(d, 1, d, tmp, lcol);
    /* NOTE: by including the specular stuff on this side of the
       "continue", we are disallowing specular highlights on the
       backsides of even semi-transparent surfaces. */
    if (ks) {
      tmp = ELL_3V_DOT(ldir, refl);
      if (tmp > 0) {
	tmp = ks*pow(tmp, sh);
	ELL_3V_SCALEADD(s, 1, s, tmp, lcol);
      }
    }
  }

  _echoIntxUVColor(icol, intx);
  chan[0] = icol[0]*d[0] + s[0];
  chan[1] = icol[1]*d[1] + s[1];
  chan[2] = icol[2]*d[2] + s[2];
  chan[3] = icol[3]*oa;
}

void
_echoFuzzifyNormal(echoPos_t norm[3], echoCol_t fuzz,
		   echoPos_t *jitt, echoPos_t view[3]) {
  float tmp, p0[3], p1[3], origNorm[3], j0, j1;
  int side, newside;

  /* we assume that the incoming normal is actually normalized */
  ELL_3V_COPY(origNorm, norm);
  side = (ELL_3V_DOT(view, origNorm) > 0 ? 1 : -1);
  ell3vPerp_f(p0, origNorm);
  ELL_3V_NORM(p0, p0, tmp);
  ELL_3V_CROSS(p1, p0, origNorm);
  j0 = fuzz*jitt[0];
  j1 = fuzz*jitt[1];
  ELL_3V_SCALEADD3(norm, 1.0, origNorm, j0, p0, j1, p1);
  newside = (ELL_3V_DOT(view, norm) > 0 ? 1 : -1);
  if (side != newside) {
    ELL_3V_SCALEADD3(norm, 1.0, origNorm, -j0, p0, -j1, p1);
  }
  ELL_3V_NORM(norm, norm, tmp);
}

void
_echoIntxColorMetal(COLOR_ARGS) {
  echoCol_t *mat,        /* pointer to object's material info */
    RS, RD,
    d[3],
    fuzz,
    lcol[3],
    rfCol[5];         /* color from reflected ray */
  echoPos_t tmp,
    view[3],          /* unit-length view vector */
    norm[3];          /* unit-length normal */
  echoPos_t *jitt,    /* fuzzy reflection jittering */
    ldir[3], ldist;
  double c;
  EchoRay rfRay, shRay;
  EchoIntx shIntx;
  int lt;
  EchoLight *light;

  mat = intx->obj->mat;

  if (0 && parm->verbose) {
    printf("depth = %d, t = %g\n", intx->depth, intx->t);
  }
  ELL_3V_NORM(intx->norm, intx->norm, tmp);
  ELL_3V_NORM(view, intx->view, tmp);
  ELL_3V_COPY(norm, intx->norm);
  ELL_3V_COPY(rfRay.from, intx->pos);
  rfRay.near = ECHO_EPSILON;
  rfRay.far = ECHO_POS_MAX;
  rfRay.depth = intx->depth + 1;
  rfRay.shadow = AIR_FALSE;
  fuzz = mat[echoMatterMetalFuzzy];
  if (fuzz) {
    jitt = ((echoPos_t *)tstate->njitt->data
	    + 2*(samp*ECHO_SAMPLE_NUM + echoSampleNormalA));
    _echoFuzzifyNormal(norm, fuzz, jitt, view);
  }
  tmp = 2*ELL_3V_DOT(view, norm);
  ELL_3V_SCALEADD(rfRay.dir, -1.0, view, tmp, norm);
  echoRayColor(rfCol, samp, &rfRay, parm, tstate, scene, lightArr);
  
  c = 1 - ELL_3V_DOT(norm, view);
  c = c*c*c*c*c;
  RS = mat[echoMatterMetalR0];
  RS = RS + (1 - RS)*c;
  RD = mat[echoMatterMetalKd]*(1-RS);
  d[0] = d[1] = d[2] = 0.0;
  if (RD) {
    /* compute the diffuse component */
    ELL_3V_COPY(shRay.from, intx->pos);
    shRay.near = ECHO_EPSILON;
    for (lt=0; lt<lightArr->len; lt++) {
      light = ((EchoLight **)lightArr->data)[lt];
      _echoLightColDir[light->type](lcol, ldir, &ldist, parm,
				    light, samp, intx, tstate);
      tmp = ELL_3V_DOT(ldir, norm);
      if (tmp <= 0)
	continue;
      if (0 && parm->verbose) {
	printf("light %d: dot = %g\n", lt, tmp);
      }
      if (parm->shadow) {
	ELL_3V_COPY(shRay.dir, ldir);
	shRay.far = ldist;
	if (_echoRayIntx[scene->type](&shIntx, &shRay, parm, scene)) {
	  /* the shadow ray hit something, nevermind */
	  continue;
	}
      }
      ELL_3V_SCALEADD(d, 1, d, tmp, lcol);
    }
  }
  chan[0] = mat[echoMatterR]*(RD*d[0] + RS*rfCol[0]);
  chan[1] = mat[echoMatterG]*(RD*d[1] + RS*rfCol[1]);
  chan[2] = mat[echoMatterB]*(RD*d[2] + RS*rfCol[2]);
  chan[3] = 1.0;

  return;
}

/*
** n == 1, n_t == index 
*/
int
_echoRefract(echoPos_t T[3], echoPos_t V[3],
	     echoPos_t N[3], echoCol_t index) {
  echoPos_t cosTh, cosPh, sinPhsq, rad, tmp1, tmp2;

  cosTh = ELL_3V_DOT(V, N);
  sinPhsq = (1 - cosTh*cosTh)/(index*index);
  rad = 1 - sinPhsq;
  if (rad < 0) 
    return AIR_FALSE;
  /* else we do not have total internal reflection */
  cosPh = sqrt(rad);
  tmp1 = -1.0/index; tmp2 = cosTh/index - cosPh; 
  ELL_3V_SCALEADD(T, tmp1, V, tmp2, N);
  return AIR_TRUE;
}

void
_echoIntxColorGlass(COLOR_ARGS) {
  echoCol_t *mat,     /* pointer to object's material info */
    RS, R0,
    k[3], r, g, b,
    fuzz, index,
    rfCol[5],         /* color from reflected ray */
    trCol[5];         /* color from transmitted ray */
  echoPos_t           /* since I don't want to branch to call ell3vPerp_? */
    tmp,
    refl[3], tran[3],
    view[3],          /* unit-length view vector */
    norm[3];          /* unit-length normal */
  echoPos_t *jitt;    /* fuzzy reflection jittering */
  double c;
  EchoRay trRay, rfRay;

  mat = intx->obj->mat;

  ELL_3V_NORM(intx->norm, intx->norm, tmp);
  ELL_3V_COPY(norm, intx->norm);
  ELL_3V_NORM(view, intx->view, tmp);
  ELL_3V_COPY(trRay.from, intx->pos);
  ELL_3V_COPY(rfRay.from, intx->pos);
  trRay.near = rfRay.near = ECHO_EPSILON;
  trRay.far = rfRay.far = ECHO_POS_MAX;
  trRay.depth = rfRay.depth = intx->depth + 1;
  trRay.shadow = rfRay.shadow = AIR_FALSE;
  fuzz = mat[echoMatterGlassFuzzy];
  index = mat[echoMatterGlassIndex];
  tmp = ELL_3V_DOT(view, norm);
  if (fuzz) {
    jitt = (echoPos_t *)tstate->njitt->data+ 2*samp*ECHO_SAMPLE_NUM;
    jitt += 2*(echoSampleNormalA + (tmp > 0));
    _echoFuzzifyNormal(norm, fuzz, jitt, view);
  }
  ELL_3V_SCALEADD(refl, -1, view, 2*tmp, norm);
  if (0 && parm->verbose) {
    printf("(glass; depth = %d)\n", intx->depth);
    printf("view . norm = %g\n", tmp);
  }
  
  if (tmp > 0) {
    /* "d.n < 0": we're bouncing off the outside */
    _echoRefract(tran, view, norm, index);
    c = tmp;
    if (0 && parm->verbose) {
      printf("view = (%g,%g,%g)\n",
	     view[0], view[1], view[2]);
      printf("tran = (%g,%g,%g)\n",
	     tran[0], tran[1], tran[2]);
      printf("c = %g\n", c);
    }
    ELL_3V_SET(k, 1, 1, 1);
  }
  else {
    /* we're bouncing off the inside */
    r = mat[echoMatterR];
    g = mat[echoMatterG];
    b = mat[echoMatterB];
    k[0] = r*exp((r-1)*intx->t);
    k[1] = g*exp((g-1)*intx->t);
    k[2] = b*exp((b-1)*intx->t);
    ELL_3V_SCALE(norm, -1, norm);
    if (_echoRefract(tran, view, norm, 1/index)) {
      c = -ELL_3V_DOT(tran, norm);
    }
    else {
      /* holy moly, its total internal reflection time */
      ELL_3V_COPY(rfRay.dir, refl);
      echoRayColor(chan, samp, &rfRay, parm, tstate, scene, lightArr);
      chan[0] *= k[0];
      chan[1] *= k[1];
      chan[2] *= k[2];
      return;
    }
  }
  R0 = (index - 1)/(index + 1);
  R0 *= R0;
  c = 1 - c;
  c = c*c*c*c*c;
  RS = R0 + (1-R0)*c;
  if (0 && parm->verbose) {
    printf("index = %g, R0 = %g, RS = %g\n", index, R0, RS);
    printf("k = %g %g %g\n", k[0], k[1], k[2]);
  }
  ELL_3V_COPY(rfRay.dir, refl);
  echoRayColor(rfCol, samp, &rfRay, parm, tstate, scene, lightArr);
  ELL_3V_COPY(trRay.dir, tran);
  echoRayColor(trCol, samp, &trRay, parm, tstate, scene, lightArr);
  ELL_3V_SCALEADD(chan, RS, rfCol, 1-RS, trCol);
  chan[0] *= k[0];
  chan[1] *= k[1];
  chan[2] *= k[2];
  chan[3] = 1.0;
  return;
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
  _echoIntxColorGlass,
  _echoIntxColorMetal,
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
		   echoCol_t index, echoCol_t kd, echoCol_t fuzzy) {

  obj->matter = echoMatterGlass;
  obj->mat[echoMatterR] = r;
  obj->mat[echoMatterG] = g;
  obj->mat[echoMatterB] = b;
  obj->mat[echoMatterKa] = 0.0;
  obj->mat[echoMatterGlassIndex] = index;
  obj->mat[echoMatterGlassKd] = kd;
  obj->mat[echoMatterGlassFuzzy] = fuzzy;
}

void
echoMatterMetalSet(EchoObject *obj,
		   echoCol_t r, echoCol_t g, echoCol_t b,
		   echoCol_t R0, echoCol_t kd, echoCol_t fuzzy) {

  obj->matter = echoMatterMetal;
  obj->mat[echoMatterR] = r;
  obj->mat[echoMatterG] = g;
  obj->mat[echoMatterB] = b;
  obj->mat[echoMatterKa] = 0.0;
  obj->mat[echoMatterMetalR0] = R0;
  obj->mat[echoMatterMetalKd] = kd;
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

void
echoMatterTextureSet(EchoObject *obj, Nrrd *ntext) {
  
  if (obj && ntext && 
      3 == ntext->dim && 
      nrrdTypeUChar == ntext->type &&
      4 == ntext->axis[0].size) {
    obj->ntext = ntext;
  }
}
