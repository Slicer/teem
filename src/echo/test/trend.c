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

#include "../echo.h"

void
makeSceneGlassMetal(limnCam *cam, EchoParam *param,
		    EchoObject *scene, airArray *lightArr) {
  EchoObject *sphere, *cube, *rect;
  EchoLight *light;
  
  ELL_3V_SET(cam->from, 2, 0, 20);
  ELL_3V_SET(cam->at,   0, 0, 0);
  ELL_3V_SET(cam->up,   -1, 0, 0);
  cam->uMin = -1.8;
  cam->uMax = 1.8;
  cam->vMin = -1.8;
  cam->vMax = 1.8;

  param->jitter = echoJitterJitter;
  param->verbose = 0;
  param->samples = 4;
  param->imgResU = 200;
  param->imgResV = 200;
  param->aperture = 0.0;
  param->gamma = 2.0;
  param->refDistance = 4;
  param->renderLights = AIR_TRUE;

  /* create scene */
  sphere = echoObjectNew(echoObjectSphere);
  echoObjectSphereSet(sphere, 1, 0, 0, 0.5);
  echoMatterPhongSet(sphere, 0.5, 0.5, 1, 1.0,
		     0.1, 0.6, 0.3, 40);
  echoObjectListAdd(scene, sphere);

  cube = echoObjectNew(echoObjectCube);
  echoMatterMetalSet(cube, 1, 1, 1, 0, 0);
  echoObjectListAdd(scene, cube);

  rect = echoObjectNew(echoObjectRectangle);
  echoObjectRectangleSet(rect,
			 -2, -2, -2,
			 4, 0, 0,
			 0, 4, 0);
  echoMatterMetalSet(rect, 1, 1, 1, 0, 0);
  echoObjectListAdd(scene, rect);

  rect = echoObjectNew(echoObjectRectangle);
  echoObjectRectangleSet(rect,
			 1.0, 0.2, 4,
			 0.2, 0, 0,
			 0, 0.2, 0);
  echoMatterLightSet(rect, 1, 1, 1);
  echoObjectListAdd(scene, rect);
  light = echoLightNew(echoLightArea);
  echoLightAreaSet(light, rect);
  echoLightArrayAdd(lightArr, light);

}

void
makeSceneShadow(limnCam *cam, EchoParam *param,
		EchoObject *scene, airArray *lightArr) {
  EchoObject *sphere, *rect, *tri;
  EchoLight *light;

  ELL_3V_SET(cam->from, 2, 0, 20);
  ELL_3V_SET(cam->at,   0, 0, 0);
  ELL_3V_SET(cam->up,   -1, 0, 0);
  cam->uMin = -1.8;
  cam->uMax = 1.8;
  cam->vMin = -1.8;
  cam->vMax = 1.8;

  param->jitter = echoJitterJitter;
  param->verbose = 0;
  param->samples = 4;
  param->imgResU = 200;
  param->imgResV = 200;
  param->aperture = 0.0;
  param->gamma = 2.0;
  param->refDistance = 4;
  param->renderLights = AIR_TRUE;

  /* create scene */
  sphere = echoObjectNew(echoObjectSphere);
  echoObjectSphereSet(sphere, 0, -1, -1, 0.2);
  echoMatterPhongSet(sphere, 0.5, 0.5, 1, 1.0,
		     0.1, 0.6, 0.3, 40);
  echoObjectListAdd(scene, sphere);

  sphere = echoObjectNew(echoObjectSphere);
  echoObjectSphereSet(sphere, 0, 1, -1, 0.2);
  echoMatterPhongSet(sphere, 1, 0.5, 0.5, 1.0,
		     0.1, 0.6, 0.3, 40);
  echoObjectListAdd(scene, sphere);

  sphere = echoObjectNew(echoObjectSphere);
  echoObjectSphereSet(sphere, 0, 0, 1, 0.2);
  echoMatterPhongSet(sphere, 0.5, 1, 0.5, 1.0,
		     0.1, 0.6, 0.3, 40);
  echoObjectListAdd(scene, sphere);

  tri = echoObjectNew(echoObjectTriangle);
  echoObjectTriangleSet(tri,
			0, -1, -1,
			0, 1, -1,
			0, 0, 1);
  echoMatterPhongSet(tri, 1, 1, 0, 1.0,
		     0.1, 0.6, 0.3, 40);
  echoObjectListAdd(scene, tri);
  
  /*
  sphere = echoObjectNew(echoObjectSphere);
  echoObjectSphereSet(sphere, 0, 0, -1002, 1000);
  echoMatterPhongSet(sphere, 1.0, 0.8, 1.0, 1.0,
		     0.1, 0.5, 0.5, 200);
  echoObjectListAdd(scene, sphere);
  */

  rect = echoObjectNew(echoObjectRectangle);
  echoObjectRectangleSet(rect,
			 1.7, 1.7, -2,
			 -3.4, 0, 0,
			 0, -3.4, 0);
  echoMatterPhongSet(rect, 1.0, 0.8, 1.0, 1.0,
		     0.1, 0.3, 0.7, 300);
  echoObjectListAdd(scene, rect);


  /*
  light = echoLightNew(echoLightDirectional);
  echoLightDirectionalSet(light, 1, 1, 1, 1, 0, 0);
  echoLightArrayAdd(lightArr, light);
  */

  rect = echoObjectNew(echoObjectRectangle);
  echoObjectRectangleSet(rect,
			 1.0, 0.2, 4,
			 0.2, 0, 0,
			 0, 0.2, 0);
  echoMatterLightSet(rect, 1, 1, 1);
  echoObjectListAdd(scene, rect);
  light = echoLightNew(echoLightArea);
  echoLightAreaSet(light, rect);
  echoLightArrayAdd(lightArr, light);

}

void
makeSceneRainLights(limnCam *cam, EchoParam *param,
		    EchoObject *scene, airArray *lightArr) {
  EchoObject *sphere, *rect;
  EchoLight *light;
  int i, N;
  echoPos_t w;
  float r, g, b;

  ELL_3V_SET(cam->from, 2, 0, 4);
  ELL_3V_SET(cam->at,   0, 0, 0);
  ELL_3V_SET(cam->up,   0, 0, 1);
  cam->uMin = -1.8;
  cam->uMax = 1.8;
  cam->vMin = -1.8;
  cam->vMax = 1.8;

  param->jitter = echoJitterJitter;
  param->verbose = 0;
  param->samples = 36;
  param->imgResU = 1000;
  param->imgResV = 1000;
  param->aperture = 0.0;
  param->gamma = 2.0;
  param->renderLights = AIR_TRUE;
  param->shadow = AIR_FALSE;
  param->refDistance = 0.7;
  param->bgR = 0.1;
  param->bgG = 0.1;
  param->bgB = 0.1;

  /* create scene */
  sphere = echoObjectNew(echoObjectSphere);
  echoObjectSphereSet(sphere, 0, 0, 0, 1.0);
  echoMatterPhongSet(sphere, 1.0, 1.0, 1.0, 1.0,
		     0.02, 0.2, 1.0, 400);
  echoObjectListAdd(scene, sphere);

  N = 8;
  w = 1.7/N;

  for (i=0; i<N; i++) {
    rect = echoObjectNew(echoObjectRectangle);
    echoObjectRectangleSet(rect,
			   w/2, AIR_AFFINE(0, i, N-1, -1-w/2, 1-w/2), 1.5,
			   0, w, 0,
			   w, 0, 0);
    dyeHSVtoRGB(&r, &g, &b, AIR_AFFINE(0, i, N, 0.0, 1.0), 1.0, 1.0);
    echoMatterLightSet(rect, r, g, b);
    echoObjectListAdd(scene, rect);
    light = echoLightNew(echoLightArea);
    echoLightAreaSet(light, rect);
    echoLightArrayAdd(lightArr, light);
  }

}

int
main(int argc, char **argv) {
  Nrrd *nraw, *nimg, *nppm, *ntmp, *npgm;
  limnCam *cam;
  EchoParam *param;
  EchoGlobalState *state;
  EchoObject *scene;
  airArray *lightArr, *mop;
  char *me, *err;
  int E;
  
  me = argv[0];

  mop = airMopInit();

  cam = echoLimnCamNew();
  airMopAdd(mop, cam, (airMopper)limnCamNix, airMopAlways);
  cam->near = 0;
  cam->dist = 0;
  cam->far = 0;
  cam->eyeRel = AIR_FALSE;

  param = echoParamNew();
  airMopAdd(mop, param, (airMopper)echoParamNix, airMopAlways);
  param->jitter = echoJitterJitter;
  param->verbose = 3;
  param->samples = 9;
  param->imgResU = 100;
  param->imgResV = 100;
  param->aperture = 0.0;
  param->gamma = 1.0;

  state = echoGlobalStateNew();
  airMopAdd(mop, state, (airMopper)echoGlobalStateNix, airMopAlways);

  scene = echoObjectNew(echoObjectList);
  airMopAdd(mop, scene, (airMopper)echoObjectNix, airMopAlways);
  
  lightArr = echoLightArrayNew();
  airMopAdd(mop, lightArr, (airMopper)echoLightArrayNix, airMopAlways);

  nraw = nrrdNew();
  nimg = nrrdNew();
  nppm = nrrdNew();
  ntmp = nrrdNew();
  npgm = nrrdNew();
  airMopAdd(mop, nraw, (airMopper)nrrdNuke, airMopAlways);
  airMopAdd(mop, nimg, (airMopper)nrrdNuke, airMopAlways);
  airMopAdd(mop, nppm, (airMopper)nrrdNuke, airMopAlways);
  airMopAdd(mop, ntmp, (airMopper)nrrdNuke, airMopAlways);
  airMopAdd(mop, npgm, (airMopper)nrrdNuke, airMopAlways);

  /* makeSceneShadow(cam, param, scene, lightArr); */
  makeSceneGlassMetal(cam, param, scene, lightArr);

  E = 0;
  if (!E) E |= echoRender(nraw, cam, param, state, scene, lightArr);
  if (!E) E |= echoComposite(nimg, nraw, param);
  if (!E) E |= echoPPM(nppm, nimg, param);
  if (E) {
    airMopAdd(mop, err = biffGetDone(ECHO), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble:\n%s\n", me, err);
    airMopError(mop); return 1;
  }
  printf("render time = %g seconds (%g fps)\n",
	 state->time, 1.0/state->time);
  if (!E) E |= nrrdSave("raw.nrrd", nraw, NULL);
  if (!E) E |= nrrdSave("out.ppm", nppm, NULL);
  if (!E) E |= nrrdSlice(ntmp, nraw, 0, 3);
  ntmp->min = 0.0; ntmp->max = 1.0;
  if (!E) E |= nrrdQuantize(npgm, ntmp, 8);
  if (!E) E |= nrrdSave("alpha.pgm", npgm, NULL);
  if (!E) E |= nrrdSlice(ntmp, nraw, 0, 4);
  if (!E) E |= nrrdHistoEq(ntmp, ntmp, NULL, 2048, 2);
  if (!E) E |= nrrdQuantize(npgm, ntmp, 8);
  if (!E) E |= nrrdSave("time.pgm", npgm, NULL);
  if (E) {
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble:\n%s\n", me, err);
    airMopError(mop); return 1;
  }

  airMopOkay(mop);
  return 0;
}
