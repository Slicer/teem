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

/*
******** echoComposite
**
**
*/
int
echoComposite(Nrrd *nimg, Nrrd *nraw, EchoParam *param) {
  char me[]="echoComposite", err[AIR_STRLEN_MED];
  echoCol_t *raw, *img, R, G, B, A;
  int i, N;

  if (!(nimg && nraw && param)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(ECHO, err); return 1;
  }
  if (nrrdMaybeAlloc(nimg, echoCol_nrrdType, 3,
		     3, nraw->axis[1].size, nraw->axis[2].size)) {
    sprintf(err, "%s:", me);
    biffMove(ECHO, err, NRRD); return 1;
  }
  
  raw = nraw->data;
  img = nimg->data;
  N = nraw->axis[1].size * nraw->axis[2].size;
  for (i=0; i<N; i++) {
    R = raw[0 + 5*i];
    G = raw[1 + 5*i];
    B = raw[2 + 5*i];
    A = raw[3 + 5*i];
    img[0 + 3*i] = A*R + (1-A)*param->bgR;
    img[1 + 3*i] = A*G + (1-A)*param->bgG;
    img[2 + 3*i] = A*B + (1-A)*param->bgB;
  }

  return 0;
}

/*
******** echoPPM
**
**
*/
int
echoPPM(Nrrd *nppm, Nrrd *nimg, EchoParam *param) {
  char me[]="echoPPM", err[AIR_STRLEN_MED];
  echoCol_t val, *img;
  unsigned char *ppm;
  int i, c, v, N;

  if (!(nppm && nimg && param)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(ECHO, err); return 1;
  }
  if (nrrdMaybeAlloc(nppm, nrrdTypeUChar, 3,
		     3, nimg->axis[1].size, nimg->axis[2].size)) {
    sprintf(err, "%s:", me);
    biffMove(ECHO, err, NRRD); return 1;
  }
  
  img = nimg->data;
  ppm = nppm->data;
  N = nimg->axis[1].size * nimg->axis[2].size;
  for (i=0; i<N; i++) {
    for (c=0; c<3; c++) {
      val = AIR_CLAMP(0.0, img[c + 3*i], 1.0);
      AIR_INDEX(0.0, val, 1.0, 256, v); 
      ppm[c + 3*i] = v;
    }
  }

  return 0;
}

/*
******** echoCheck
**
** does all the error checking required of echoRender and
** everything that it calls
*/
int
echoCheck(Nrrd *nraw, limnCam *cam, 
	  EchoParam *param, EchoState *state,
	  EchoObject *scene, airArray *lightArr) {
  char me[]="echoCheck", err[AIR_STRLEN_MED];
  int tmp;

  if (!(nraw && cam && param && scene && lightArr)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(ECHO, err); return 1;
  }
  if (limnCamUpdate(cam)) {
    sprintf(err, "%s: camera trouble", me);
    biffMove(ECHO, err, LIMN); return 1;
  }
  if (!airEnumValidVal(echoJitter, param->jitter)) {
    sprintf(err, "%s: jitter method (%d) invalid", me, param->jitter);
    biffAdd(ECHO, err); return 1;
  }
  if (!(param->samples > 0)) {
    sprintf(err, "%s: # samples (%d) invalid", me, param->samples);
    biffAdd(ECHO, err); return 1;
  }
  if (!(param->imgResU > 0 && param->imgResV)) {
    sprintf(err, "%s: image dimensions (%dx%d) invalid", me,
	    param->imgResU, param->imgResV);
    biffAdd(ECHO, err); return 1;
  }
  if (!(AIR_EXISTS(param->epsilon) && AIR_EXISTS(param->aperture))) {
    sprintf(err, "%s: epsilon or aperture doesn't exist", me);
    biffAdd(ECHO, err); return 1;
  }
  if (echoObjectAABox != scene->type) {
    sprintf(err, "%s: can only handle %s top-level object", me,
	    airEnumStr(echoObject, echoObjectAABox));
    biffAdd(ECHO, err); return 1;
  }

  switch (param->jitter) {
  case echoJitterNone:
  case echoJitterRandom:
    break;
  case echoJitterGrid:
  case echoJitterJitter:
    tmp = sqrt(param->samples);
    if (tmp*tmp != param->samples) {
      sprintf(err, "%s: need a square # samples for %s jitter method (not %d)",
	      me, airEnumStr(echoJitter, param->jitter), param->samples);
      biffAdd(ECHO, err); return 1;
    }
    break;
  }
  
  /* all is well */
  return 0;
}

void
echoJitterSet(EchoParam *param, EchoState *state) {
  echoPos_t *jitt, w;
  int i, xi, yi, n, N;

  jitt = (echoPos_t *)state->njitt->data;
  N = param->samples;
  n = sqrt(N);
  w = 1.0/n;
  for (i=0; i<ECHO_SAMPLE_NUM*N; i++) {
    xi = i % n; yi = i / n;
    switch(param->jitter) {
    case echoJitterNone:
      jitt[0 + 2*i] = 0.0;
      jitt[1 + 2*i] = 0.0;
      break;
    case echoJitterGrid:
      jitt[0 + 2*i] = NRRD_AXIS_POS(nrrdCenterCell, -0.5, 0.5, n, xi);
      jitt[1 + 2*i] = NRRD_AXIS_POS(nrrdCenterCell, -0.5, 0.5, n, yi);
      break;
    case echoJitterJitter:
      jitt[0 + 2*i] = NRRD_AXIS_POS(nrrdCenterCell, -0.5, 0.5, n, xi);
      jitt[0 + 2*i] += AIR_AFFINE(0.0, airRand(), 1.0, -w/2, w/2);
      jitt[1 + 2*i] = NRRD_AXIS_POS(nrrdCenterCell, -0.5, 0.5, n, yi);
      jitt[1 + 2*i] += AIR_AFFINE(0.0, airRand(), 1.0, -w/2, w/2);
      break;
    case echoJitterRandom:
      jitt[0 + 2*i] = airRand() - 0.5;
      jitt[1 + 2*i] = airRand() - 0.5;
      break;
    }
  }

  return;
}

/*
******** echoRender
**
** top-level call to accomplish all rendering.  As much error checking
** as possible should be done here and not in the lower-level functions.
*/
int
echoRender(Nrrd *nraw, limnCam *cam,
	   EchoParam *param, EchoState *state,
	   EchoObject *scene, airArray *lightArr) {
  char me[]="echoRender", err[AIR_STRLEN_MED];
  int imgUi, imgVi,     /* integral pixel indices */
    samp;               /* which sample are we doing */
  echoPos_t tmp,
    eye[3],             /* eye center before jittering */
    from[3],            /* ray origination (eye after jittering */
    at[3],              /* ray destination (pixel center after jittering) */
    U[4], V[4], N[4],   /* view space basis (only first 3 elements used) */
    pixUsz, pixVsz,     /* U and V dimensions of a pixel */
    imgU, imgV,         /* floating point pixel center locations */
    imgOrig[3],         /* image origin */
    *jitt;              /* current scanline of master jitter array */

  if (echoCheck(nraw, cam, param, state, scene, lightArr)) {
    sprintf(err, "%s: problem with input", me);
    biffAdd(ECHO, err); return 1;
  }
  if (nrrdMaybeAlloc(nraw, echoCol_nrrdType, 3,
		     5, param->imgResU, param->imgResV)) {
    sprintf(err, "%s: couldn't allocate output image", me);
    biffMove(ECHO, err, NRRD); return 1;
  }
  if (nrrdMaybeAlloc(state->njitt, echoPos_nrrdType, 3,
		     2, ECHO_SAMPLE_NUM, param->samples)) {
    sprintf(err, "%s: couldn't allocate jitter array", me);
    biffMove(ECHO, err, NRRD); return 1;
  }

  state->time0 = airTime();
  airSrand();
  echoJitterSet(param, state);

  /* set eye, U, V, N, imgOrig */
  ELL_3V_COPY(eye, cam->from);
  ELL_4MV_GET_ROW0(U, cam->W2V);
  ELL_4MV_GET_ROW1(V, cam->W2V);
  ELL_4MV_GET_ROW2(N, cam->W2V);
  ELL_3V_COPY(imgOrig, eye);
  ELL_3V_SCALEADD(imgOrig, N, cam->vspDist);

  /* determine pixel dimensions */
  pixUsz = (cam->uMax - cam->uMin)/(param->imgResU);
  pixVsz = (cam->vMax - cam->vMin)/(param->imgResV);

  for (imgVi=0; imgVi<param->imgResV; imgVi++) {
    imgV = NRRD_AXIS_POS(nrrdCenterCell, cam->vMin, cam->vMax,
			 param->imgResV, imgVi);
    for (imgUi=0; imgUi<param->imgResU; imgUi++) {
      imgU = NRRD_AXIS_POS(nrrdCenterCell, cam->uMin, cam->uMax,
			   param->imgResU, imgUi);
      /* initialize jitt on first "scanline" */
      jitt = (echoPos_t *)state->njitt->data;

      /* go through samples */
      for (samp=0; samp<=param->samples; samp++) {
	/* set from[] */
	ELL_3V_COPY(from, eye);
	tmp = param->aperture*jitt[0 + 2*echoSampleLens];
	ELL_3V_SCALEADD(from, U, tmp);
	tmp = param->aperture*jitt[1 + 2*echoSampleLens];
	ELL_3V_SCALEADD(from, V, tmp);

	/* set at[] */
	ELL_3V_COPY(at, imgOrig);
	tmp = imgU + pixUsz*jitt[0 + 2*echoSamplePixel];
	ELL_3V_SCALEADD(at, U, tmp);
	tmp = imgV + pixVsz*jitt[1 + 2*echoSamplePixel];
	ELL_3V_SCALEADD(at, V, tmp);

	

	/* move jitt to next "scanline" */
	jitt += 2*ECHO_SAMPLE_NUM;
      }
      if (!param->reuseJitter) 
	echoJitterSet(param, state);
    }
  }
  state->time1 = airTime();
  

  return 0;
}
