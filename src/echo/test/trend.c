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
  Nrrd *nraw, *nimg, *nppm;
  limnCam *cam;
  EchoParam *param;
  EchoGlobalState *state;
  EchoObject *scene;
  airArray *lightArr, *mop;
  EchoLight **light;
  char *me, *err;
  int E;
  
  me = argv[0];

  mop = airMopInit();

  cam = echoLimnCamNew();
  airMopAdd(mop, cam, (airMopper)limnCamNix, airMopAlways);
  ELL_3V_SET(cam->from, 0, 0, 10);
  ELL_3V_SET(cam->at,   0, 0, 0);
  ELL_3V_SET(cam->up,   0, 1, 0);
  cam->uMin = -0.5;
  cam->uMax = 0.5;
  cam->vMin = -0.5;
  cam->vMax = 0.5;
  cam->near = -2;
  cam->dist = 0;
  cam->far = 2;

  param = echoParamNew();
  airMopAdd(mop, param, (airMopper)echoParamNix, airMopAlways);
  param->jitter = echoJitterNone;
  param->verbose = 3;
  param->samples = 1;
  param->imgResU = 256;
  param->imgResV = 256;
  param->epsilon = 0.000001;
  param->aperture = 0.0;

  state = echoGlobalStateNew();
  airMopAdd(mop, state, (airMopper)echoGlobalStateNix, airMopAlways);

  scene = echoObjectNew(echoObjectAABox);
  airMopAdd(mop, scene, (airMopper)echoObjectNix, airMopAlways);
  
  lightArr = airArrayNew((void**)&light, NULL, sizeof(EchoLight *), 1);
  airMopAdd(mop, lightArr, (airMopper)airArrayNuke, airMopAlways);

  nraw = nrrdNew();
  nimg = nrrdNew();
  nppm = nrrdNew();
  airMopAdd(mop, nraw, (airMopper)nrrdNuke, airMopAlways);
  airMopAdd(mop, nimg, (airMopper)nrrdNuke, airMopAlways);
  airMopAdd(mop, nppm, (airMopper)nrrdNuke, airMopAlways);

  E = 0;
  if (!E) E |= echoRender(nraw, cam, param, state, scene, lightArr);
  if (!E) E |= echoComposite(nimg, nraw, param);
  if (!E) E |= echoPPM(nppm, nimg, param);
  if (E) {
    airMopAdd(mop, err = biffGetDone(ECHO), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble:\n%s\n", me, err);
    airMopError(mop); return 1;
  }
  if (!E) E |= nrrdSave("out.ppm", nppm, NULL);
  if (E) {
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble:\n%s\n", me, err);
    airMopError(mop); return 1;
  }

  airMopOkay(mop);
  return 0;
}
