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

EchoParam *
echoParamNew(void) {
  EchoParam *param;
  
  param = (EchoParam *)calloc(1, sizeof(EchoParam));
  param->verbose = 1;
  param->jitter = echoJitterNone;
  param->samples = 1;
  param->recDepth = 1;
  param->reuseJitter = AIR_FALSE;
  param->bgR = 0.0;
  param->bgG = 0.0;
  param->bgB = 0.0;

  /* these will have to be user-set */
  param->imgResU = param->imgResV = 0;
  param->epsilon = AIR_NAN;
  param->aperture = AIR_NAN;

  return param;
}

EchoParam *
echoParamNix(EchoParam *param) {

  free(param);

  return NULL;
}

EchoState *
echoStateNew(void) {
  EchoState *state;
  
  state = (EchoState *)calloc(1, sizeof(EchoState));
  state->njitt = nrrdNew();
  state->time0 = AIR_NAN;
  state->time1 = AIR_NAN;
  
  return state;
}

EchoState *
echoStateNix(EchoState *state) {

  nrrdNuke(state->njitt);
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
