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

EchoParm *
echoParmNew(void) {
  EchoParm *parm;
  
  parm = (EchoParm *)calloc(1, sizeof(EchoParm));
  parm->verbose = 1;
  parm->jitter = echoJitterNone;
  parm->samples = 1;
  parm->shadow = AIR_TRUE;
  parm->maxRecDepth = 5;
  parm->reuseJitter = AIR_FALSE;
  parm->permuteJitter = AIR_TRUE;
  parm->amR = 1.0;
  parm->amG = 1.0;
  parm->amB = 1.0;
  parm->mrR = 0.0;
  parm->mrG = 0.0;
  parm->mrB = 0.0;
  parm->bgR = 0.0;
  parm->bgG = 0.0;
  parm->bgB = 0.0;
  parm->gamma = 2.2;
  parm->timeGamma = 6.0;
  parm->renderLights = AIR_TRUE;
  parm->renderBoxes = AIR_FALSE;
  parm->refDistance = 1.0;
  parm->seedRand = AIR_TRUE;

  /* these will have to be user-set */
  parm->imgResU = parm->imgResV = 0;
  parm->aperture = AIR_NAN;

  return parm;
}

EchoParm *
echoParmNix(EchoParm *parm) {

  free(parm);

  return NULL;
}

EchoGlobalState *
echoGlobalStateNew(void) {
  EchoGlobalState *state;
  
  state = (EchoGlobalState *)calloc(1, sizeof(EchoGlobalState));
  
  return state;
}

EchoGlobalState *
echoGlobalStateNix(EchoGlobalState *state) {

  free(state);
  return NULL;
}

EchoThreadState *
echoThreadStateNew(void) {
  EchoThreadState *state;
  
  state = (EchoThreadState *)calloc(1, sizeof(EchoThreadState));
  state->njitt = nrrdNew();
  state->nperm = nrrdNew();
  
  return state;
}

EchoThreadState *
echoThreadStateNix(EchoThreadState *state) {

  nrrdNuke(state->njitt);
  nrrdNuke(state->nperm);
  free(state->permBuff);
  free(state->chanBuff);
  free(state);
  return NULL;
}

limnCam *
echoLimnCamNew(void) {
  limnCam *cam;

  cam = limnCamNew();
  cam->eyeRel = AIR_FALSE;
  cam->leftHanded = AIR_FALSE;

  return cam;
}
