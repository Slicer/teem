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
#include "../privateEcho.h"

/* bad bad bad Gordon */
void
_dyeHSVtoRGB(float *R, float *G, float *B,
	    float  H, float  S, float  V) {
  float min, fract, vsf, mid1, mid2;
  int sextant;
  
  if (0 == S) {
    *R = *G = *B = V;
    return;
  }
  /* else there is hue */
  if (1 == H)
    H = 0;
  H *= 6;
  sextant = (int) floor(H);
  fract = H - sextant;
  vsf = V*S*fract;
  min = V*(1 - S);
  mid1 = min + vsf;
  mid2 = V - vsf;
  switch (sextant) {
  case 0: { *R = V;    *G = mid1; *B = min;  break; }
  case 1: { *R = mid2; *G = V;    *B = min;  break; }
  case 2: { *R = min;  *G = V;    *B = mid1; break; }
  case 3: { *R = min;  *G = mid2; *B = V;    break; }
  case 4: { *R = mid1; *G = min;  *B = V;    break; }
  case 5: { *R = V;    *G = min;  *B = mid2; break; }
  }
}

void
makeSceneDOF(limnCam *cam, EchoParm *parm,
	     EchoObject **sceneP, airArray **lightArrP) {
  EchoObject *scene, *rect;
  Nrrd *ntext;

  *sceneP = scene = echoNew(echoList);
  *lightArrP = echoLightArrayNew();

  ELL_3V_SET(cam->from, 5, -5, 15);
  ELL_3V_SET(cam->at,   0, 0, 0);
  ELL_3V_SET(cam->up,   0, -1, 0);
  cam->uRange[0] = -3.7;
  cam->uRange[1] = 3.7;
  cam->vRange[0] = -3.7;
  cam->vRange[1] = 3.7;
  cam->dist = 2.1;
  cam->dist = 0.0;
  cam->dist = -2.1;

  parm->jitter = echoJitterJitter;
  echoVerbose = 0;
  parm->samples = 36;
  parm->imgResU = 300;
  parm->imgResV = 300;
  parm->aperture = 1.5;
  parm->gamma = 1.0;
  parm->renderLights = AIR_FALSE;
  parm->renderBoxes = AIR_FALSE;
  parm->seedRand = AIR_FALSE;
  parm->maxRecDepth = 10;
  parm->shadow = AIR_TRUE;

  nrrdLoad(ntext = nrrdNew(), "psq.nrrd");

  rect = echoNew(echoRectangle);
  echoRectangleSet(rect,
			 -0.5, -2.5, -3,
			 3, 0, 0,
			 0, 3, 0);
  echoMatterPhongSet(rect, 1, 0.5, 0.5, 1.0,
		     1.0, 0.0, 0.0, 1);
  echoMatterTextureSet(rect, ntext);
  echoListAdd(scene, rect);

  rect = echoNew(echoRectangle);
  echoRectangleSet(rect,
			 -1.5, -1.5, 0,
			 3, 0, 0,
			 0, 3, 0);
  echoMatterPhongSet(rect, 0.5, 1, 0.5, 1.0,
		     1.0, 0.0, 0.0, 1);
  echoMatterTextureSet(rect, ntext);
  echoListAdd(scene, rect);

  rect = echoNew(echoRectangle);
  echoRectangleSet(rect,
			 -2.5, -0.5, 3,
			 3, 0, 0,
			 0, 3, 0);
  echoMatterPhongSet(rect, 0.5, 0.5, 1, 1.0,
		     1.0, 0.0, 0.0, 1);
  echoMatterTextureSet(rect, ntext);
  echoListAdd(scene, rect);

  return;
}

  
void
makeSceneAntialias(limnCam *cam, EchoParm *parm,
		   EchoObject **sceneP, airArray **lightArrP) {
  EchoObject *scene, *rect;
  Nrrd *ntext;

  *sceneP = scene = echoNew(echoList);
  *lightArrP = echoLightArrayNew();

  ELL_3V_SET(cam->from, 0, 0, 10);
  ELL_3V_SET(cam->at,   0, 0, 0);
  ELL_3V_SET(cam->up,   0, 1, 0);
  cam->uRange[0] = -3.7;
  cam->uRange[1] = 3.7;
  cam->vRange[0] = -3.7;
  cam->vRange[1] = 3.7;

  parm->jitter = echoJitterGrid;
  echoVerbose = 0;
  parm->samples = 1;
  parm->imgResU = 300;
  parm->imgResV = 300;
  parm->aperture = 0.0;
  parm->gamma = 1.0;
  parm->renderLights = AIR_FALSE;
  parm->renderBoxes = AIR_FALSE;
  parm->seedRand = AIR_FALSE;
  parm->maxRecDepth = 10;
  parm->shadow = AIR_TRUE;

  nrrdLoad(ntext = nrrdNew(), "chirp.nrrd");
  rect = echoNew(echoRectangle);
  echoRectangleSet(rect,
			 -3, -3, 0,
			 6, 0, 0,
			 0, 6, 0);
  echoMatterPhongSet(rect, 1, 1, 1, 1.0,
		     1.0, 0.0, 0.0, 1);
  echoMatterTextureSet(rect, ntext);
  echoListAdd(scene, rect);

  return;
}

void
makeSceneSimple(limnCam *cam, EchoParm *parm,
		EchoObject **sceneP, airArray **lightArrP) {
  EchoObject *scene, *tri, *rect, *sphere;
  EchoLight_ *light;
  airArray *lightArr;

  *sceneP = scene = echoNew(echoList);
  *lightArrP = lightArr = echoLightArrayNew();

  ELL_3V_SET(cam->from, 5, -5, 9);
  ELL_3V_SET(cam->at,   0, 0, 0);
  ELL_3V_SET(cam->up,   0, 0, 1);
  cam->uRange[0] = -3.7;
  cam->uRange[1] = 3.7;
  cam->vRange[0] = -3.7;
  cam->vRange[1] = 3.7;

  parm->jitter = echoJitterJitter;
  echoVerbose = 0;
  parm->samples = 4;
  parm->imgResU = 200;
  parm->imgResV = 200;
  parm->aperture = 0.0;
  parm->gamma = 2.0;
  parm->renderLights = AIR_FALSE;
  parm->renderBoxes = AIR_FALSE;
  parm->seedRand = AIR_FALSE;
  parm->maxRecDepth = 10;
  parm->shadow = AIR_TRUE;

  rect = echoNew(echoRectangle);
  echoRectangleSet(rect,
			 -3, -3, -2,
			 6, 0, 0,
			 0, 6, 0);
  echoMatterPhongSet(rect, 1, 1, 1, 1.0,
		     0.1, 0.5, 0.9, 50);
  echoListAdd(scene, rect);

  sphere = echoNew(echoSphere);
  echoSphereSet(sphere, 0, 0, 0, 1.7);
  echoMatterPhongSet(sphere, 1, 0.7, 1, 1.0,
		     0.0, 0.5, 0.9, 90);
  echoListAdd(scene, sphere);

  tri = echoNew(echoTriangle);
  echoTriangleSet(tri,
			0.1, 0.1, 2,
			2, 2, 2,
			0, 2, 2);
  echoMatterPhongSet(tri, 1, 0.2, 0.2, 1.0,
		     0.4, 0.6, 0.0, 90);
  echoListAdd(scene, tri);

  tri = echoNew(echoTriangle);
  echoTriangleSet(tri,
			-0.1, 0.1, 2,
			-2, 2, 2,
			-2, 0, 2);
  echoMatterPhongSet(tri, 0.2, 1.0, 0.2, 1.0,
		     0.4, 0.6, 0.0, 90);
  echoListAdd(scene, tri);

  tri = echoNew(echoTriangle);
  echoTriangleSet(tri,
			-0.1, -0.1, 2,
			-2, -2, 2,
			0, -2, 2);
  echoMatterPhongSet(tri, 0.2, 0.2, 1.0, 1.0,
		     0.4, 0.6, 0.0, 90);
  echoListAdd(scene, tri);

  /*

  ELL_4M_SET_SCALE(matx, 3, 3, 3);
  trim = echoRoughSphere(80, 40, matx);
  echoMatterPhongSet(trim, 1, 1, 1, 1.0,
		     0.1, 0.5, 0.9, 50);
  echoMatterTextureSet(trim, ntext);
  echoListAdd(scene, trim);
  */

  
  rect = echoNew(echoRectangle);
  echoRectangleSet(rect,
			 -0.4, -0.4, 6,
			 0.8, 0.0, 0,
			 0.0, 0.8, 0);
  parm->refDistance = 2;
  echoMatterLightSet(rect, 0.25, 0.25, 0.25);
  echoListAdd(scene, rect);
  light = echoLightNew(echoLightArea);
  echoLightAreaSet(light, rect);
  echoLightArrayAdd(lightArr, light);
  
  return;
}

void
makeSceneTexture(limnCam *cam, EchoParm *parm,
		  EchoObject **sceneP, airArray **lightArrP) {
  EchoObject *scene, /* *trim, */ *rect, /* *inst, */ *sphere;
  EchoLight_ *light;
  airArray *lightArr;
  Nrrd *ntext;

  *sceneP = scene = echoNew(echoList);
  *lightArrP = lightArr = echoLightArrayNew();

  ELL_3V_SET(cam->from, 9, 9, 11);
  ELL_3V_SET(cam->at,   0, 0, 0);
  ELL_3V_SET(cam->up,   0, 0, 1);
  cam->uRange[0] = -4;
  cam->uRange[1] = 4;
  cam->vRange[0] = -4;
  cam->vRange[1] = 4;

  parm->jitter = echoJitterNone;
  echoVerbose = 0;
  parm->samples = 1;
  parm->imgResU = 300;
  parm->imgResV = 300;
  parm->aperture = 0.0;
  parm->gamma = 2.0;
  parm->renderLights = AIR_FALSE;
  parm->renderBoxes = AIR_FALSE;
  parm->seedRand = AIR_FALSE;
  parm->maxRecDepth = 10;
  parm->shadow = AIR_TRUE;


  rect = echoNew(echoRectangle);
  echoRectangleSet(rect,
			 -2, -2, 0,
			 4, 0, 0,
			 0, 4, 0);
  echoMatterPhongSet(rect, 1, 1, 1, 1.0,
		     0.1, 0.5, 0.9, 50);
  echoListAdd(scene, rect);

  nrrdLoad(ntext=nrrdNew(), "home.nrrd");
  echoMatterTextureSet(rect, ntext);

  sphere = echoNew(echoSphere);
  echoSphereSet(sphere, 0, 0, 0, 3);
  echoMatterPhongSet(sphere, 1, 1, 1, 1.0,
		     0.1, 0.5, 0.9, 50);
  echoMatterTextureSet(sphere, ntext);
  echoListAdd(scene, sphere);

  /*

  ELL_4M_SET_SCALE(matx, 3, 3, 3);
  trim = echoRoughSphere(80, 40, matx);
  echoMatterPhongSet(trim, 1, 1, 1, 1.0,
		     0.1, 0.5, 0.9, 50);
  echoMatterTextureSet(trim, ntext);
  echoListAdd(scene, trim);
  */

  
  rect = echoNew(echoRectangle);
  echoRectangleSet(rect,
			 0, 0, 6,
			 0, 2, 0,
			 0, 0, 2);
  parm->refDistance = 0.5;
  echoMatterLightSet(rect, 1, 1, 1);
  echoListAdd(scene, rect);
  light = echoLightNew(echoLightArea);
  echoLightAreaSet(light, rect);
  echoLightArrayAdd(lightArr, light);
  
  return;
}

void
makeSceneInstance(limnCam *cam, EchoParm *parm,
		  EchoObject **sceneP, airArray **lightArrP) {
  EchoObject *scene, *trim, *rect, *inst;
  EchoLight_ *light;
  airArray *lightArr;
  echoPos_t matx[16], A[16], B[16];
  
  *sceneP = scene = echoNew(echoList);
  *lightArrP = lightArr = echoLightArrayNew();

  ELL_3V_SET(cam->from, 9*1.3, 9*1.3, 11*1.3);
  ELL_3V_SET(cam->at,   0, 0, 0);
  ELL_3V_SET(cam->up,   0, 0, 1);
  cam->uRange[0] = -5;
  cam->uRange[1] = 5;
  cam->vRange[0] = -5;
  cam->vRange[1] = 5;

  parm->jitter = echoJitterNone;
  echoVerbose = 0;
  parm->samples = 1;
  parm->imgResU = 300;
  parm->imgResV = 300;
  parm->aperture = 0.0;
  parm->gamma = 2.0;
  parm->refDistance = 1;
  parm->renderLights = AIR_TRUE;
  parm->renderBoxes = AIR_FALSE;
  parm->seedRand = AIR_FALSE;
  parm->maxRecDepth = 10;
  parm->shadow = AIR_TRUE;
  
  ELL_4M_SET_IDENTITY(matx);
  ELL_4M_SET_SCALE(B, 2.5, 1.5, 0.8);
  ELL_4M_MUL(A, B, matx); ELL_4M_COPY(matx, A);
  ELL_4M_SET_ROTATE_X(B, 0.2);
  ELL_4M_MUL(A, B, matx); ELL_4M_COPY(matx, A);
  ELL_4M_SET_ROTATE_Y(B, 0.2);
  ELL_4M_MUL(A, B, matx); ELL_4M_COPY(matx, A);
  ELL_4M_SET_ROTATE_Y(B, 0.2);
  ELL_4M_MUL(A, B, matx); ELL_4M_COPY(matx, A);
  ELL_4M_SET_TRANSLATE(B, 0, 0, 1);
  ELL_4M_MUL(A, B, matx); ELL_4M_COPY(matx, A);


  /* trim = echoRoughSphere(50, 25, matx); */
  /*
  trim = echoRoughSphere(8, 4, matx);
  echoMatterGlassSet(trim, 0.8, 0.8, 0.8,
		     1.3, 0.0, 0.0);
  echoMatterPhongSet(trim, 1, 1, 1, 1.0,
		     0.1, 0.5, 0.9, 50);
  echoListAdd(scene, trim);
  */

  trim = echoNew(echoSphere);
  echoSphereSet(trim, 0, 0, 0, 1);
  echoMatterGlassSet(trim, 0.8, 0.8, 0.8,
		     1.3, 0.0, 0.0);
  echoMatterPhongSet(trim, 1, 1, 1, 1.0,
		     0.1, 0.5, 0.9, 50);
  inst = echoNew(echoInstance);
  echoInstanceSet(inst, matx, trim, AIR_TRUE);
  echoListAdd(scene, inst);

  rect = echoNew(echoRectangle);
  echoRectangleSet(rect,
			 -3.5, -3.5, -3.5,
			 7, 0, 0,
			 0, 7, 0);
  echoMatterPhongSet(rect, 1.0, 1.0, 1.0, 1.0,
		     0.1, 0.5, 0.9, 50);
  echoListAdd(scene, rect);
  rect = echoNew(echoRectangle);
  echoRectangleSet(rect,
			 -3.5, -3.5, -3.5,
			 0, 7, 0,
			 0, 0, 7);
  echoMatterPhongSet(rect, 1.0, 1.0, 1.0, 1.0,
		     0.1, 0.5, 0.9, 50);
  echoListAdd(scene, rect);
  /*
  rect = echoNew(echoRectangle);
  echoRectangleSet(rect,
			 -3.5, -3.5, -3.5,
			 0, 0, 7,
			 7, 0, 0);
  */
  rect = echoNew(echoSphere);
  echoSphereSet(rect, 0, 0, 0, 1);
  echoMatterPhongSet(rect, 1.0, 1.0, 1.0, 1.0,
		     0.1, 0.5, 0.9, 50);
  inst = echoNew(echoInstance);
  ELL_4M_SET_SCALE(A, 20, 20, 20);
  ELL_4M_SET_TRANSLATE(B, 0, -(20+3.5), 0);
  ELL_4M_MUL(matx, B, A);
  echoInstanceSet(inst, matx, rect, AIR_TRUE);
  echoListAdd(scene, inst);
  

  light = echoLightNew(echoLightDirectional);
  echoLightDirectionalSet(light, 1, 0, 0, 1, 0.001, 0.001);
  echoLightArrayAdd(lightArr, light);
  light = echoLightNew(echoLightDirectional);
  echoLightDirectionalSet(light, 0, 1, 0, 0.001, 1, 0.001);
  echoLightArrayAdd(lightArr, light);
  light = echoLightNew(echoLightDirectional);
  echoLightDirectionalSet(light, 0, 0, 1, 0.001, 0.001, 1);
  echoLightArrayAdd(lightArr, light);

  return;
}


void
makeSceneBVH(limnCam *cam, EchoParm *parm,
	     EchoObject **sceneP, airArray **lightArrP) {
  EchoObject *sphere;
  int i, N;
  float r, g, b;
  EchoObject *scene;
  double time0, time1;
  
  *sceneP = scene = echoNew(echoList);
  *lightArrP = echoLightArrayNew();

  ELL_3V_SET(cam->from, 9, 6, 0);
  ELL_3V_SET(cam->at,   0, 0, 0);
  ELL_3V_SET(cam->up,   0, 0, 1);
  cam->uRange[0] = -3;
  cam->uRange[1] = 3;
  cam->vRange[0] = -3;
  cam->vRange[1] = 3;

  parm->jitter = echoJitterNone;
  echoVerbose = 0;
  parm->samples = 1;
  parm->imgResU = 500;
  parm->imgResV = 500;
  parm->aperture = 0.0;
  parm->gamma = 2.0;
  parm->refDistance = 1;
  parm->renderLights = AIR_TRUE;
  parm->renderBoxes = AIR_FALSE;
  parm->seedRand = AIR_FALSE;
  parm->maxRecDepth = 10;
  parm->shadow = AIR_FALSE;

  N = 1000000;
  /* airSrand(); */
  airArraySetLen(LIST(scene)->objArr, N);
  for (i=0; i<N; i++) {
    sphere = echoNew(echoSphere);
    echoSphereSet(sphere,
			4*airRand()-2, 4*airRand()-2, 4*airRand()-2, 0.005);
    _dyeHSVtoRGB(&r, &g, &b, AIR_AFFINE(0, i, N, 0.0, 1.0), 1.0, 1.0);
    echoMatterPhongSet(sphere, r, g, b, 1.0,
		       1.0, 0.0, 0.0, 50);
    LIST(scene)->obj[i] = sphere;
  }

  time0 = airTime();
  *sceneP = scene = echoListSplit3(scene, 8);
  time1 = airTime();
  printf("BVH build time = %g seconds\n", time1 - time0);
}

void
makeSceneGlass(limnCam *cam, EchoParm *parm,
	       EchoObject **sceneP, airArray **lightArrP) {
  EchoObject *cube, *rect;
  EchoLight_ *light;
  EchoObject *scene; airArray *lightArr;
  Nrrd *ntext;
  
  *sceneP = scene = echoNew(echoList);
  *lightArrP = lightArr = echoLightArrayNew();

  ELL_3V_SET(cam->from, 2, -3, 8);
  ELL_3V_SET(cam->at,   0, 0, 0);
  ELL_3V_SET(cam->up,   0, 0, 1);
  cam->uRange[0] = -1.0;
  cam->uRange[1] = 1.0;
  cam->vRange[0] = -1.0;
  cam->vRange[1] = 1.0;

  parm->jitter = echoJitterNone;
  echoVerbose = 0;
  parm->samples = 1;
  parm->imgResU = 300;
  parm->imgResV = 300;
  parm->aperture = 0.0;
  parm->gamma = 2.0;
  parm->refDistance = 4;
  parm->renderLights = AIR_FALSE;
  parm->shadow = AIR_FALSE;
  parm->seedRand = AIR_FALSE;
  parm->maxRecDepth = 10;
  parm->mrR = 1.0;
  parm->mrG = 0.0;
  parm->mrB = 1.0;

  cube = echoNew(echoCube);
  printf("cube = %p\n", cube);
  echoMatterGlassSet(cube,
		     1.0, 1.0, 1.0,
		     1.5, 0.0, 0.0);
  echoListAdd(scene, cube);

  nrrdLoad(ntext=nrrdNew(), "psq.nrrd");
  
  rect = echoNew(echoRectangle);
  printf("rect = %p\n", rect);
  echoRectangleSet(rect,
			 -1, -1, -0.51,
			 2, 0, 0,
			 0, 2, 0);
  echoMatterPhongSet(rect, 1.0, 1.0, 1.0, 1.0,
		     0.1, 0.6, 0.3, 40);
  echoMatterTextureSet(rect, ntext);
  echoListAdd(scene, rect);

  light = echoLightNew(echoLightDirectional);
  echoLightDirectionalSet(light, 1, 1, 1, 0, 0, 1);
  echoLightArrayAdd(lightArr, light);
}

void
makeSceneGlass2(limnCam *cam, EchoParm *parm,
		EchoObject **sceneP, airArray **lightArrP) {
  EchoObject *cube, *rect;
  EchoLight_ *light;
  EchoObject *scene; airArray *lightArr;
  Nrrd *ntext;
  echoPos_t matx[16];
  
  *sceneP = scene = echoNew(echoList);
  *lightArrP = lightArr = echoLightArrayNew();

  ELL_3V_SET(cam->from, 0, 0, 100);
  ELL_3V_SET(cam->at,   0, 0, 0);
  ELL_3V_SET(cam->up,   0, 1, 0);
  cam->uRange[0] = -1.0;
  cam->uRange[1] = 1.0;
  cam->vRange[0] = -1.0;
  cam->vRange[1] = 1.0;

  parm->jitter = echoJitterNone;
  echoVerbose = 0;
  parm->samples = 1;
  parm->imgResU = 300;
  parm->imgResV = 300;
  parm->aperture = 0.0;
  parm->gamma = 2.0;
  parm->refDistance = 4;
  parm->renderLights = AIR_FALSE;
  parm->shadow = AIR_FALSE;
  parm->seedRand = AIR_FALSE;
  parm->maxRecDepth = 10;
  parm->mrR = 1.0;
  parm->mrG = 0.0;
  parm->mrB = 1.0;

  ELL_4M_SET_SCALE(matx, 0.5, 0.5, 0.5);
  cube = echoRoughSphere(80, 40, matx);
  /*
  cube = echoNew(echoSphere);
  echoSphereSet(cube, 0, 0, 0, 0.5);
  */
  echoMatterGlassSet(cube,
		     1.0, 1.0, 1.0,
		     1.33333, 0.0, 0.0);
  echoListAdd(scene, cube);

  nrrdLoad(ntext=nrrdNew(), "check.nrrd");
  
  rect = echoNew(echoRectangle);
  printf("rect = %p\n", rect);
  echoRectangleSet(rect,
			 -1, -1, -0.51,
			 2, 0, 0,
			 0, 2, 0);
  echoMatterPhongSet(rect, 1.0, 1.0, 1.0, 1.0,
		     0.0, 1.0, 0.0, 40);
  echoMatterTextureSet(rect, ntext);
  echoListAdd(scene, rect);

  light = echoLightNew(echoLightDirectional);
  echoLightDirectionalSet(light, 1, 1, 1, 0, 0, 1);
  echoLightArrayAdd(lightArr, light);
}

void
makeSceneGlassMetal(limnCam *cam, EchoParm *parm,
		    EchoObject **sceneP, airArray **lightArrP) {
  EchoObject *sphere, *cube, *rect;
  EchoLight_ *light;
  EchoObject *scene; airArray *lightArr;
  
  *sceneP = scene = echoNew(echoList);
  *lightArrP = lightArr = echoLightArrayNew();

  ELL_3V_SET(cam->from, 3, 0, 6);
  ELL_3V_SET(cam->at,   0, 0, 0);
  ELL_3V_SET(cam->up,   -1, 0, 0);
  cam->uRange[0] = -0.9;
  cam->uRange[1] = 0.7;
  cam->vRange[0] = -0.3;
  cam->vRange[1] = 1.3;

  parm->jitter = echoJitterJitter;
  echoVerbose = 0;
  parm->samples = 36;
  parm->imgResU = 400;
  parm->imgResV = 400;
  parm->samples = 36;
  parm->imgResU = 400;
  parm->imgResV = 400;
  parm->aperture = 0.0;
  parm->gamma = 2.0;
  parm->refDistance = 4;
  parm->renderLights = AIR_TRUE;
  parm->seedRand = AIR_FALSE;

  /* create scene */
  sphere = echoNew(echoSphere);
  echoSphereSet(sphere, 0.70, -0.3, -0.4, 0.1);
  echoMatterPhongSet(sphere, 1, 0, 0, 1.0,
		     0.1, 0.6, 0.3, 80);
  echoListAdd(scene, sphere);

  sphere = echoNew(echoSphere);
  echoSphereSet(sphere, 0.66, 0.0, -0.4, 0.1);
  echoMatterPhongSet(sphere, 0, 1, 0, 1.0,
		     0.1, 0.6, 0.3, 80);
  echoMatterGlassSet(sphere, 0, 1, 0,
		     1.5, 0.0, 0.0);
  echoListAdd(scene, sphere);

  sphere = echoNew(echoSphere);
  echoSphereSet(sphere, 0.62, 0.3, -0.4, 0.1);
  echoMatterPhongSet(sphere, 0, 0, 1, 1.0,
		     0.1, 0.6, 0.3, 80);
  echoListAdd(scene, sphere);

  cube = echoNew(echoCube);
  echoMatterPhongSet(cube, 0.5, 1, 1, 1.0,
		     0.1, 0.6, 0.3, 40);
  echoMatterGlassSet(cube,
		     1.0, 1.0, 1.0,
		     1.5, 0.0, 0.0);
  echoMatterMetalSet(cube,
		     1.0, 1.0, 1.0,
		     0.5, 0.3, 0.11);
  echoListAdd(scene, cube);

  rect = echoNew(echoRectangle);
  echoRectangleSet(rect,
			 -2, -2, -1.5,
			 3, 0, 0,
			 0, 4, 0);
  echoMatterPhongSet(rect, 1.0, 1.0, 1.0, 1.0,
		     0.1, 0.6, 0.3, 40);
  echoListAdd(scene, rect);

  rect = echoNew(echoRectangle);
  echoRectangleSet(rect,
			 1.0, 0.2, 4,
			 0.2, 0, 0,
			 0, 0.2, 0);
  echoMatterLightSet(rect, 1, 1, 1);
  echoListAdd(scene, rect);
  light = echoLightNew(echoLightArea);
  echoLightAreaSet(light, rect);
  echoLightArrayAdd(lightArr, light);

  /*
  rect = echoNew(echoRectangle);
  echoRectangleSet(rect,
			 4.5, -1, -4,
			 0.0, 2, 0.0,
			 0.0, 0.0, 2);
  echoMatterLightSet(rect, 0.02, 0.02, 0);
  echoListAdd(scene, rect);
  light = echoLightNew(echoLightArea);
  echoLightAreaSet(light, rect);
  echoLightArrayAdd(lightArr, light);
  */

  return;
}

void
makeSceneShadow(limnCam *cam, EchoParm *parm,
		EchoObject **sceneP, airArray **lightArrP) {
  EchoObject *sphere, *rect, *tri;
  EchoLight_ *light;
  EchoObject *scene; airArray *lightArr;

  *sceneP = scene = echoNew(echoList);
  *lightArrP = lightArr = echoLightArrayNew();

  ELL_3V_SET(cam->from, 2, 0, 20);
  ELL_3V_SET(cam->at,   0, 0, 0);
  ELL_3V_SET(cam->up,   -1, 0, 0);
  cam->uRange[0] = -1.8;
  cam->uRange[1] = 1.8;
  cam->vRange[0] = -1.8;
  cam->vRange[1] = 1.8;

  parm->jitter = echoJitterJitter;
  echoVerbose = 0;
  parm->samples = 4;
  parm->imgResU = 200;
  parm->imgResV = 200;
  parm->aperture = 0.0;
  parm->gamma = 2.0;
  parm->refDistance = 4;
  parm->renderLights = AIR_TRUE;

  /* create scene */
  sphere = echoNew(echoSphere);
  echoSphereSet(sphere, 0, -1, -1, 0.2);
  echoMatterPhongSet(sphere, 0.5, 0.5, 1, 1.0,
		     0.1, 0.6, 0.3, 40);
  echoListAdd(scene, sphere);

  sphere = echoNew(echoSphere);
  echoSphereSet(sphere, 0, 1, -1, 0.2);
  echoMatterPhongSet(sphere, 1, 0.5, 0.5, 1.0,
		     0.1, 0.6, 0.3, 40);
  echoListAdd(scene, sphere);

  sphere = echoNew(echoSphere);
  echoSphereSet(sphere, 0, 0, 1, 0.2);
  echoMatterPhongSet(sphere, 0.5, 1, 0.5, 1.0,
		     0.1, 0.6, 0.3, 40);
  echoListAdd(scene, sphere);

  tri = echoNew(echoTriangle);
  echoTriangleSet(tri,
			0, -1, -1,
			0, 1, -1,
			0, 0, 1);
  echoMatterPhongSet(tri, 1, 1, 0, 1.0,
		     0.1, 0.6, 0.3, 40);
  echoListAdd(scene, tri);
  
  /*
  sphere = echoNew(echoSphere);
  echoSphereSet(sphere, 0, 0, -1002, 1000);
  echoMatterPhongSet(sphere, 1.0, 0.8, 1.0, 1.0,
		     0.1, 0.5, 0.5, 200);
  echoListAdd(scene, sphere);
  */

  rect = echoNew(echoRectangle);
  echoRectangleSet(rect,
			 1.7, 1.7, -2,
			 -3.4, 0, 0,
			 0, -3.4, 0);
  echoMatterPhongSet(rect, 1.0, 0.8, 1.0, 1.0,
		     0.1, 0.3, 0.7, 300);
  echoListAdd(scene, rect);


  /*
  light = echoLightNew(echoLightDirectional);
  echoLightDirectionalSet(light, 1, 1, 1, 1, 0, 0);
  echoLightArrayAdd(lightArr, light);
  */

  rect = echoNew(echoRectangle);
  echoRectangleSet(rect,
			 1.0, 0.2, 4,
			 0.2, 0, 0,
			 0, 0.2, 0);
  echoMatterLightSet(rect, 1, 1, 1);
  echoListAdd(scene, rect);
  light = echoLightNew(echoLightArea);
  echoLightAreaSet(light, rect);
  echoLightArrayAdd(lightArr, light);

}

void
makeSceneRainLights(limnCam *cam, EchoParm *parm,
		    EchoObject **sceneP, airArray **lightArrP) {
  EchoObject *sphere, *rect;
  EchoLight_ *light;
  int i, N;
  echoPos_t w;
  float r, g, b;
  EchoObject *scene; airArray *lightArr;

  *sceneP = scene = echoNew(echoList);
  *lightArrP = lightArr = echoLightArrayNew();

  ELL_3V_SET(cam->from, 2, 0, 4);
  ELL_3V_SET(cam->at,   0, 0, 0);
  ELL_3V_SET(cam->up,   0, 0, 1);
  cam->uRange[0] = -1.8;
  cam->uRange[1] = 1.8;
  cam->vRange[0] = -1.8;
  cam->vRange[1] = 1.8;

  parm->jitter = echoJitterJitter;
  echoVerbose = 0;
  parm->samples = 36;
  parm->imgResU = 1000;
  parm->imgResV = 1000;
  parm->samples = 36;
  parm->imgResU = 200;
  parm->imgResV = 200;
  parm->aperture = 0.0;
  parm->gamma = 2.0;
  parm->renderLights = AIR_TRUE;
  parm->shadow = AIR_FALSE;
  parm->refDistance = 0.5;
  parm->bgR = 0.1;
  parm->bgG = 0.1;
  parm->bgB = 0.1;

  /* create scene */
  sphere = echoNew(echoSphere);
  echoSphereSet(sphere, 0, 0, 0, 1.0);
  echoMatterPhongSet(sphere, 1.0, 1.0, 1.0, 1.0,
		     0.02, 0.2, 1.0, 400);
  echoListAdd(scene, sphere);

  N = 8;
  w = 1.7/N;

  for (i=0; i<N; i++) {
    rect = echoNew(echoRectangle);
    echoRectangleSet(rect,
			   w/2, AIR_AFFINE(0, i, N-1, -1-w/2, 1-w/2), 1.5,
			   0, w, 0,
			   w, 0, 0);
    _dyeHSVtoRGB(&r, &g, &b, AIR_AFFINE(0, i, N, 0.0, 1.0), 1.0, 1.0);
    echoMatterLightSet(rect, r, g, b);
    echoListAdd(scene, rect);
    light = echoLightNew(echoLightArea);
    echoLightAreaSet(light, rect);
    echoLightArrayAdd(lightArr, light);
  }

}

int
main(int argc, char **argv) {
  Nrrd *nraw, *nimg, *nppm, *ntmp, *ntmp2, *npgm;
  limnCam *cam;
  EchoParm *parm;
  EchoGlobalState *state;
  EchoObject *scene;
  airArray *lightArr, *mop;
  char *me, *err;
  int E;
  
  /*  
  echoPos_t T[3], V[3], N[3], tmp;
  ELL_3V_SET(V, 1, 1, 0); ELL_3V_NORM(V, V, tmp);
  ELL_3V_SET(N, 0, 1, 0); ELL_3V_NORM(N, N, tmp);
  printf("V: (%g,%g,%g); N: (%g,%g,%g)\n",
	 V[0], V[1], V[2], N[0], N[1], N[2]);
  printf("refract return = %d\n", _echoRefract(T, V, N, 1.0));
  printf("  --> T = (%g,%g,%g)\n", T[0], T[1], T[2]);
  exit(0);
  */

  me = argv[0];

  mop = airMopInit();

  cam = echoLimnCamNew();
  airMopAdd(mop, cam, (airMopper)limnCamNix, airMopAlways);
  cam->neer = 0;
  cam->dist = 0;
  cam->faar = 0;
  cam->atRel = AIR_TRUE;

  parm = echoParmNew();
  airMopAdd(mop, parm, (airMopper)echoParmNix, airMopAlways);

  state = echoGlobalStateNew();
  airMopAdd(mop, state, (airMopper)echoGlobalStateNix, airMopAlways);

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

  /* makeSceneRainLights(cam, parm, &scene, &lightArr); */
  /* makeSceneShadow(cam, parm, &scene, &lightArr); */
  /* makeSceneGlass(cam, parm, &scene, &lightArr); */
  /* makeSceneGlass2(cam, parm, &scene, &lightArr); */
  /* makeSceneGlassMetal(cam, parm, &scene, &lightArr); */
  /* makeSceneBVH(cam, parm, &scene, &lightArr); */
  /* makeSceneInstance(cam, parm, &scene, &lightArr); */
  /* makeSceneTexture(cam, parm, &scene, &lightArr); */
  makeSceneSimple(cam, parm, &scene, &lightArr);
  /* makeSceneAntialias(cam, parm, &scene, &lightArr); */
  /* makeSceneDOF(cam, parm, &scene, &lightArr); */
  airMopAdd(mop, scene, (airMopper)echoNuke, airMopAlways);
  airMopAdd(mop, lightArr, (airMopper)echoLightArrayNix, airMopAlways);

  E = 0;
  if (!E) E |= echoRender(nraw, cam, parm, state, scene, lightArr);
  if (!E) E |= echoComposite(nimg, nraw, parm);
  if (!E) E |= echoPPM(nppm, nimg, parm);
  if (E) {
    airMopAdd(mop, err = biffGetDone(ECHO), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble:\n%s\n", me, err);
    airMopError(mop); return 1;
  }
  printf("render time = %g seconds (%g fps)\n",
	 state->time, 1.0/state->time);
  if (!E) E |= nrrdSave("raw.nrrd", nraw, NULL);
  if (!E) E |= nrrdSave("o.ppm", nppm, NULL);
  if (!E) E |= nrrdSlice(ntmp, nraw, 0, 3);
  ntmp->min = 0.0; ntmp->max = 1.0;
  if (!E) E |= nrrdQuantize(npgm, ntmp, 8);
  if (!E) E |= nrrdSave("alpha.pgm", npgm, NULL);
  if (!E) E |= nrrdSlice(ntmp, nraw, 0, 4);
  if (!E) E |= nrrdHistoEq(ntmp2=nrrdNew(), ntmp, NULL, 2048, 2);
  if (!E) E |= nrrdQuantize(npgm, ntmp2, 8);
  if (!E) E |= nrrdSave("time.pgm", npgm, NULL);
  if (E) {
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble:\n%s\n", me, err);
    airMopError(mop); return 1;
  }

  airMopOkay(mop);
  return 0;
}
