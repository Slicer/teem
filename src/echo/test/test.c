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
  char *me, *err;
  echoScene *scene;
  echoObject *obj;
  Nrrd *nraw;
  limnCam *cam;
  echoRTParm *parm;
  echoGlobalState *gstate;
  airArray *mop;

  me = argv[0];
  mop = airMopNew();
  scene = echoSceneNew();
  airMopAdd(mop, scene, (airMopper)echoSceneNix, airMopAlways);
  obj = echoObjectNew(scene, echoTypeSphere);
  echoSphereSet(obj, 0, 0, 0, 0.5);
  echoColorSet(obj, 1, 0.5, 0.25, 0.25);

  nraw = nrrdNew();
  cam = limnCamNew();
  parm = echoRTParmNew();
  gstate = echoGlobalStateNew();
  airMopAdd(mop, nraw, (airMopper)nrrdNuke, airMopAlways);
  airMopAdd(mop, cam, (airMopper)limnCamNix, airMopAlways);
  airMopAdd(mop, parm, (airMopper)echoRTParmNix, airMopAlways);
  airMopAdd(mop, gstate, (airMopper)echoGlobalStateNix, airMopAlways);

  ELL_3V_SET(cam->from, 10, 0, 0);
  ELL_3V_SET(cam->at, 5, 0, 0);
  ELL_3V_SET(cam->up, 0, 0, 1);
  cam->neer = -2;
  cam->dist = 0;
  cam->faar = 2;
  cam->atRel = AIR_TRUE;
  cam->rightHanded = AIR_TRUE;
  cam->uRange[0] = -1;  cam->vRange[0] = -1;
  cam->uRange[1] =  1;  cam->vRange[1] =  1;
  parm->imgResU = parm->imgResV = 128;
  parm->numSamples = 4;
  parm->jitterType = echoJitterJitter;
  parm->aperture = 0;

  if (echoRTRender(nraw, cam, scene, parm, gstate)) {
    airMopAdd(mop, err = biffGetDone(ECHO), airFree, airMopAlways);
    fprintf(stderr, "%s: %s\n", me, err);
    airMopError(mop);
    return 1;
  }

  nrrdSave("nraw.nrrd", nraw, NULL);
  
  airMopOkay(mop);

  return 0;
}
