/*
  The contents of this file are subject to the University of Utah Public
  License (the "License"); you may not use this file except in
  compliance with the License.
  
  Software distributed under the License is distributed on an "AS IS"
  basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See
  the License for the specific language governing rights and limitations
  under the License.

  The Original Source Code is "teem", released March 23, 2001.
  
  The Original Source Code was developed by the University of Utah.
  Portions created by UNIVERSITY are Copyright (C) 2001, 1998 University
  of Utah. All Rights Reserved.
*/


char *me;

#include <limn.h>

int
main(int argc, char *argv[]) {
  limnLight *lit;
  limnCam *cam;
  Nrrd *map, *ppm;

  me = argv[0];

  cam = limnCamNew();
  ELL_3V_SET(cam->from, 10, 0, 0);
  ELL_3V_SET(cam->at, 0, 0, 0);
  ELL_3V_SET(cam->up, 0, 0, 1);
  cam->uMin = -(cam->uMax = 4);
  cam->vMin = -(cam->vMax = 3);
  cam->near = -5;
  cam->dist = 0;
  cam->far =  5;
  cam->eyeRel = AIR_FALSE;

  lit = limnLightNew();
  limnLightSet(lit, AIR_TRUE, 1, 0, 0, 1, 0, 0);
  limnLightSet(lit, AIR_TRUE, 0, 1, 0, 0, 1, 0);
  limnLightSet(lit, AIR_TRUE, 0, 0, 1, 0, 0, 1);
  limnLightUpdate(lit, cam);
  
  if (limnEnvMapFill(map=nrrdNew(), 
		     limnLightDiffuseCB, lit,
		     limnQN_16checker)) {
    fprintf(stderr, "%s: trouble:\n%s", me, biffGet(LIMN));
    exit(1);
  }
  map->min = 0;
  map->max = 1;
  if (nrrdQuantize(ppm=nrrdNew(), map, 8)) {
    fprintf(stderr, "%s: trouble:\n%s", me, biffGet(NRRD));
    exit(1);
  }
  if (nrrdSave("map.ppm", ppm, NULL)) {
    fprintf(stderr, "%s: trouble:\n%s", me, biffGet(NRRD));
    exit(1);
  }
  
  nrrdNuke(map);
  nrrdNuke(ppm);
  exit(0);
}
