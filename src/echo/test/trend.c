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

int
main(int argc, char **argv) {
  Nrrd *nraw, *nimg, *nppm, *ntmp, *npgm;
  limnCam *cam;
  EchoParam *param;
  EchoGlobalState *state;
  EchoObject *scene, *sphere;
  airArray *lightArr, *mop;
  EchoLight *light;
  char *me, *err;
  int E;
  
  me = argv[0];

  mop = airMopInit();

  cam = echoLimnCamNew();
  airMopAdd(mop, cam, (airMopper)limnCamNix, airMopAlways);
  ELL_3V_SET(cam->from, 10, 0, 0);
  ELL_3V_SET(cam->at,   0, 0, 0);
  ELL_3V_SET(cam->up,   0, 0, 1);
  cam->uMin = -2;
  cam->uMax = 2;
  cam->vMin = -2;
  cam->vMax = 2;
  cam->near = 0;
  cam->dist = 0;
  cam->far = 0;
  cam->eyeRel = AIR_FALSE;

  param = echoParamNew();
  airMopAdd(mop, param, (airMopper)echoParamNix, airMopAlways);
  param->jitter = echoJitterJitter;
  param->verbose = 3;
  param->samples = 4;
  param->imgResU = 64;
  param->imgResV = 64;
  param->epsilon = 0.000001;
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

  /* create scene */
  sphere = echoObjectNew(echoObjectSphere);
  ELL_3V_SET(((EchoObjectSphere*)sphere)->pos, 0, 0, 0);
  ((EchoObjectSphere*)sphere)->rad = 2;
  echoMatterPhongSet(sphere, 1, 0.5, 0, 0.5,
		     0.2, 1.0, 1.0, 20);
  echoObjectListAdd(scene, sphere);

  sphere = echoObjectNew(echoObjectSphere);
  echoObjectSphereSet(sphere, 3, 0, 0, 0.5);
  echoMatterPhongSet(sphere, 0, 0.5, 1, 0.5,
		     0.2, 1.0, 1.0, 20);
  echoObjectListAdd(scene, sphere);

  light = echoLightNew(echoLightDirectional);
  echoLightDirectionalSet(light, 1, 1, 1, 0, 0, 1);
  echoLightArrayAdd(lightArr, light);

  E = 0;
  if (!E) E |= echoRender(nraw, cam, param, state, scene, lightArr);
  if (!E) E |= echoComposite(nimg, nraw, param);
  if (!E) E |= echoPPM(nppm, nimg, param);
  if (E) {
    airMopAdd(mop, err = biffGetDone(ECHO), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble:\n%s\n", me, err);
    airMopError(mop); return 1;
  }
  if (!E) E |= nrrdSave("raw.nrrd", nraw, NULL);
  if (!E) E |= nrrdSave("out.ppm", nppm, NULL);
  if (!E) E |= nrrdSlice(ntmp, nraw, 0, 3);
  if (!E) E |= nrrdQuantize(npgm, ntmp, 8);
  if (!E) E |= nrrdSave("alpha.pgm", npgm, NULL);
  if (!E) E |= nrrdSlice(ntmp, nraw, 0, 4);
  nrrdMinMaxSet(ntmp);
  if (!E) E |= nrrdArithGamma(ntmp, ntmp,
			      param->timeGamma, ntmp->min, ntmp->max);
  if (!E) E |= nrrdHistoEq(ntmp, ntmp, NULL, 1024, 1);
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
