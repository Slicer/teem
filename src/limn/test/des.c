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
  limnEnvMapFill(map=nrrdNew(), limnLightDiffuseCB, lit, limnQN_16checker);
  
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
