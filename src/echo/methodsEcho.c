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

EchoRTParm *
echoRTParmNew(void) {
  EchoRTParm *parm;
  
  parm = (EchoRTParm *)calloc(1, sizeof(EchoRTParm));
  parm->jitter = echoJitterNone;
  parm->shadow = AIR_TRUE;
  parm->samples = 1;
  parm->imgResU = parm->imgResV = 256;
  parm->maxRecDepth = 5;
  parm->reuseJitter = AIR_FALSE;
  parm->permuteJitter = AIR_TRUE;
  parm->renderLights = AIR_TRUE;
  parm->renderBoxes = AIR_FALSE;
  parm->seedRand = AIR_TRUE;
  parm->aperture = 0.0;     /* pinhole camera by default */
  parm->timeGamma = 6.0;
  parm->refDistance = 1.0;
  parm->mrR = 1.0;
  parm->mrG = 0.0;
  parm->mrB = 1.0;
  parm->amR = 1.0;
  parm->amG = 1.0;
  parm->amB = 1.0;
  parm->bgR = 0.0;
  parm->bgG = 0.0;
  parm->bgB = 0.0;
  parm->gamma = 2.2;
  return parm;
}

EchoRTParm *
echoRTParmNix(EchoRTParm *parm) {

  free(parm);

  return NULL;
}

EchoGlobalState *
echoGlobalStateNew(void) {
  EchoGlobalState *state;
  
  state = (EchoGlobalState *)calloc(1, sizeof(EchoGlobalState));
  state->time = 0;
  return state;
}

EchoGlobalState *
echoGlobalStateNix(EchoGlobalState *state) {

  if (state) {
    airFree(state);
  }
  return NULL;
}

EchoThreadState *
echoThreadStateNew(void) {
  EchoThreadState *state;
  
  state = (EchoThreadState *)calloc(1, sizeof(EchoThreadState));
  state->njitt = nrrdNew();
  state->nperm = nrrdNew();
  state->permBuff = NULL;
  state->chanBuff = NULL;
  return state;
}

EchoThreadState *
echoThreadStateNix(EchoThreadState *state) {

  if (state) {
    nrrdNuke(state->njitt);
    nrrdNuke(state->nperm);
    airFree(state->permBuff);
    airFree(state->chanBuff);
    airFree(state);
  }
  return NULL;
}

limnCam *
echoLimnCamNew(void) {
  limnCam *cam;

  cam = limnCamNew();
  cam->atRel = AIR_TRUE;
  cam->rightHanded = AIR_TRUE;

  return cam;
}
