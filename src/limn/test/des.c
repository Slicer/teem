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


#include <biff.h>
#include <limn.h>

char *me;

int
main(int argc, char *argv[]) {
  limnObj *obj;
  limnCam *cam;
  limnWin *win;
  limnLight *lit;
  Nrrd *map;
  float tt[16], rot[16], scal[16], tran[16];
  int i, j, k, ri;

  me = argv[0];

  cam = limnCamNew();
  ELL_3V_SET(cam->from, 30, 30, 30);
  ELL_3V_SET(cam->at, 0, 0, 0);
  ELL_3V_SET(cam->up, 0, 0, 1);
  cam->uMin = -(cam->uMax = 1.3);
  cam->vMin = -(cam->vMax = 1.3);
  cam->near = -5;
  cam->dist = 0;
  cam->far =  5;
  cam->eyeRel = AIR_FALSE;
  
  lit = limnLightNew();
  limnLightSet(lit, AIR_FALSE, 1, 1, 1, 1, 0, 0);
  /*
  limnLightSet(lit, AIR_FALSE, 0, 1, 0, 0, 1, 0);
  limnLightSet(lit, AIR_FALSE, 0, 0, 1, 0, 0, 1);
  */
  limnLightUpdate(lit, cam);
  limnEnvMapFill(map=nrrdNew(), limnLightDiffuseCB, lit, limnQN16);
  
  win = limnWinNew(limnDevicePS);
  win->file = fopen("out.ps", "w");
  win->scale = 200;

  obj = limnObjNew(256, AIR_TRUE);
  printf("hello 0\n");
  limnObjPolarSphereAdd(obj, 2, 100, 50);
  printf("hello 1\n");

  /*
  ri = limnObjCubeAdd(obj, 2);
  limnObjPartTransform(obj, ri, ELL_4M_SET_SCALE(tt, 4, 0.3, 0.3));
  limnObjPartTransform(obj, ri, ELL_4M_SET_TRANSLATE(tt, 2.4, 0, 0));
  ri = limnObjCubeAdd(obj, 2);
  limnObjPartTransform(obj, ri, ELL_4M_SET_SCALE(tt, 0.3, 3, 0.3));
  limnObjPartTransform(obj, ri, ELL_4M_SET_TRANSLATE(tt, 0, 1.9, 0));
  ri = limnObjCubeAdd(obj, 2);
  limnObjPartTransform(obj, ri, ELL_4M_SET_SCALE(tt, 0.3, 0.3, 2));
  limnObjPartTransform(obj, ri, ELL_4M_SET_TRANSLATE(tt, 0, 0, 1.4));
  */

  if (limnCamUpdate(cam)) {
    printf("%s: camera trouble:\n%s", me, biffGet(LIMN));
    exit(1);
  }

  limnObjHomog(obj, limnSpaceWorld);
  limnObjNormals(obj, limnSpaceWorld);
  limnObjSpaceTransform(obj, cam, win, limnSpaceView);
  limnObjSpaceTransform(obj, cam, win, limnSpaceScreen);
  limnObjNormals(obj, limnSpaceScreen);
  limnObjSpaceTransform(obj, cam, win, limnSpaceDevice);

  limnObjDepthSortParts(obj);
  limnObjPSRender(obj, cam, map, win);

  /* limnObjDescribe(stdout, obj); */

  fclose(win->file);
  
  obj = limnObjNuke(obj);
  return 0;
}
