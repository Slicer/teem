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


#include "limn.h"

limnCam *
limnCamNew(void) {
  limnCam *cam;

  cam = (limnCam *)calloc(1, sizeof(limnCam));
  cam->eyeRel = 1;
  cam->leftHanded = 0;
  return(cam);
}

limnCam *
limnCamNix(limnCam *cam) {

  if (cam) {
    free(cam);
  }
  return NULL;
}

void
_limnOptsPSDefaults(limnOptsPS *ps) {

  ps->edgeWidth[0] = 0.0;
  ps->edgeWidth[1] = 0.00;
  ps->edgeWidth[2] = 2;
  ps->edgeWidth[3] = 0.5;
  ps->edgeWidth[4] = 2;
  ps->creaseAngle = 80;
  ps->bgGray = 0.9;
}

limnWin *
limnWinNew(int device) {
  limnWin *win;

  win = (limnWin *)calloc(1, sizeof(limnWin));
  win->device = device;
  switch(device) {
  case limnDevicePS:
    win->zFlip = 1;
    _limnOptsPSDefaults(&(win->ps));
    break;
  }
  win->scale = 400;
  win->file = NULL;
  return win;
}

limnWin *
limnWinNix(limnWin *win) {

  if (win) {
    free(win);
  }
  return NULL;
}
