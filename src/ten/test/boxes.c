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
  cam->uRange[0] = -(cam->uRange[1] = 85);
  cam->vRange[0] = -(cam->vRange[1] = 85);
  cam->neer = -5;
  cam->dist = 0;
  cam->faar =  5;
  cam->atRel = AIR_TRUE;
  
  limnLightSetAmbient(lit, 0.5, 0.5, 0.5);
  limnLightSet(lit, 0, AIR_TRUE, 1, 1, 1, 0, 0, -1);
  limnLightUpdate(lit, cam);
  limnEnvMapFill(map=nrrdNew(), limnLightDiffuseCB, lit,
		 limnQN_16checker);

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
