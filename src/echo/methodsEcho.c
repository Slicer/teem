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

#include "echo.h"

echoRTParm *
echoRTParmNew(void) {
  echoRTParm *parm;
  
  parm = (echoRTParm *)calloc(1, sizeof(echoRTParm));
  if (parm) {
    parm->jitterType = echoJitterNone;
    parm->reuseJitter = AIR_FALSE;
    parm->permuteJitter = AIR_TRUE;
    parm->doShadows = AIR_TRUE;
    parm->numSamples = 1;
    parm->imgResU = parm->imgResV = 256;
    parm->maxRecDepth = 5;
    parm->renderLights = AIR_TRUE;
    parm->renderBoxes = AIR_FALSE;
    parm->seedRand = AIR_TRUE;
    parm->aperture = 0.0;     /* pinhole camera by default */
    parm->timeGamma = 6.0;
    parm->refDistance = 1.0;
    ELL_3V_SET(parm->mr, 1.0, 0.0, 1.0);
  }
  return parm;
}

echoRTParm *
echoRTParmNix(echoRTParm *parm) {

  AIR_FREE(parm);

  return NULL;
}

echoGlobalState *
echoGlobalStateNew(void) {
  echoGlobalState *state;
  
  state = (echoGlobalState *)calloc(1, sizeof(echoGlobalState));
  if (state) {
    state->time = 0;
  }
  return state;
}

echoGlobalState *
echoGlobalStateNix(echoGlobalState *state) {

  AIR_FREE(state);
  return NULL;
}

echoThreadState *
echoThreadStateNew(void) {
  echoThreadState *state;
  
  state = (echoThreadState *)calloc(1, sizeof(echoThreadState));
  if (state) {
    state->verbose = 0;
    state->njitt = nrrdNew();
    state->nperm = nrrdNew();
    state->permBuff = NULL;
    state->chanBuff = NULL;
  }
  return state;
}

echoThreadState *
echoThreadStateNix(echoThreadState *state) {

  if (state) {
    nrrdNuke(state->njitt);
    nrrdNuke(state->nperm);
    AIR_FREE(state->permBuff);
    AIR_FREE(state->chanBuff);
    AIR_FREE(state);
  }
  return NULL;
}

echoScene *
echoSceneNew(void) {
  echoScene *ret;
  
  ret = (echoScene *)calloc(1, sizeof(echoScene));
  if (ret) {
    ret->obj = NULL;
    ret->objArr = airArrayNew((void**)&(ret->obj), NULL,
			      sizeof(echoObject *),
			      ECHO_LIST_OBJECT_INCR);
    airArrayPointerCB(ret->objArr,
		      airNull,
		      (void *(*)(void *))echoObjectNix);
    ret->lit = NULL;
    ret->litArr = airArrayNew((void**)&(ret->lit), NULL,
			      sizeof(echoObject *),
			      ECHO_LIST_OBJECT_INCR);
    /* no pointers set; objects are nixed on delete by above */
    ret->nrrd = NULL;
    ret->nrrdArr = airArrayNew((void**)&(ret->nrrd), NULL,
			       sizeof(Nrrd *),
			       ECHO_LIST_OBJECT_INCR);
    airArrayPointerCB(ret->nrrdArr,
		      airNull,
		      (void *(*)(void *))nrrdNuke);
    ELL_3V_SET(ret->am, 1.0, 1.0, 1.0);
    ELL_3V_SET(ret->bg, 0.0, 0.0, 0.0);
  }
  return ret;
}

echoScene *
echoSceneNix(echoScene *scene) {
  
  if (scene) {
    airArrayNuke(scene->objArr);
    airArrayNuke(scene->litArr);
    airArrayNuke(scene->nrrdArr);
    AIR_FREE(scene);
  }
  return NULL;
}
