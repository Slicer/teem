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


#include <ten.h>

char *me;

void
usage() {
  /*                      0     1       2   (3)*/
  fprintf(stderr, "usage: %s <nrrdIn> <psOut>\n", me);
  exit(1);
}


int
main(int argc, char *argv[]) {
  char *inS, *outS;
  Nrrd *nin, *map;
  limnObj *obj;
  limnCam *cam;
  limnLight *lit;
  limnWin *win;
  tenGlyphParm *parm;
  double t0, t1;

  me = argv[0];
  if (3 != argc)
    usage();

  inS = argv[1];
  outS = argv[2];

  if (nrrdLoad(nin=nrrdNew(), inS)) {
    fprintf(stderr, "%s: couldn't load \"%s\":\n%s", me, inS, biffGet(NRRD));
    exit(1);
  }
  
  win = limnWinNew(limnDevicePS);
  lit = limnLightNew();
  cam = limnCamNew();
  obj = limnObjNew(32*1024, AIR_TRUE);
 
  t0 = airTime();
  parm = tenGlyphParmNew();
  parm->vThreshVol = NULL;
  parm->vThresh = 0.3;
  parm->dwiThresh = 0.9;
  parm->useColor = 1;
  parm->cscale = 0.55;
  parm->anisoThresh = 0.24;
  parm->anisoType = 0.3;
  parm->sumFloor = 4;
  parm->sumCeil = 10;
  parm->fakeSat = 1.1;
  parm->dim = 3;
  if (tenGlyphGen(obj, nin, parm)) {
    fprintf(stderr, "%s: trouble:\n%s", me, biffGet(TEN));
    exit(1);
  }
  t1 = airTime();
  printf("%s: tenGlyphGen: %g seconds\n", me, t1-t0);
  printf("pNum: %d\n", obj->pA->len);
  printf("vNum: %d\n", obj->vA->len);
  printf("eNum: %d\n", obj->eA->len);
  printf("fNum: %d\n", obj->fA->len);
  printf("rNum: %d\n", obj->rA->len);

  /* ELL_3V_SET(cam->from, -325, 10, -10); */
  /* ELL_3V_SET(cam->from, -316, 100, -80); */
  /* ELL_3V_SET(cam->from, -300, 100, -80); */
  /* ELL_3V_SET(cam->from, -212, 212, -80); */
  /* ELL_3V_SET(cam->from, -10, 316, -80); */
  /* ELL_3V_SET(cam->from, 212, 212, -80); */
  /* ELL_3V_SET(cam->from, 300, 100, -80); */

  ELL_3V_SET(cam->from, -300, 100, -80);
  ELL_3V_SET(cam->at, 0, 0, 0);
  ELL_3V_SET(cam->up, 0, 0, -1);
  cam->uMin = -(cam->uMax = 85);
  cam->vMin = -(cam->vMax = 85);
  cam->near = -5;
  cam->dist = 0;
  cam->far =  5;
  cam->eyeRel = AIR_FALSE;
  
  limnLightSetAmbient(lit, 0.5, 0.5, 0.5);
  limnLightSet(lit, AIR_TRUE, 1, 1, 1, 0, 0, -1);
  limnLightUpdate(lit, cam);
  limnEnvMapFill(map=nrrdNew(), limnLightDiffuseCB, lit, limnQN16);

  win->file = fopen(outS, "w");
  win->scale = 10;
  ELL_5V_SET(win->ps.edgeWidth, 0, 0, 0.001, 0, 0.001);
  win->ps.bgGray = 1.0;

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


  fclose(win->file);
  
}
