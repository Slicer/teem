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


#include "limn.h"

limnLight *
limnLightNew(void) {
  limnLight *lit;

  lit = (limnLight *)calloc(1, sizeof(limnLight));
  return lit;
}

limnLight *
limnLightNix(limnLight *lit) {
  
  if (lit)
    free(lit);
  return NULL;
}

limnCam *
limnCamNew(void) {
  limnCam *cam;

  cam = (limnCam *)calloc(1, sizeof(limnCam));
  if (cam) {
    cam->atRel = limnDefCamAtRel;
    cam->ortho = limnDefCamOrtho;
    cam->rightHanded = limnDefCamRightHanded;
  }
  return cam;
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
  ps->edgeWidth[3] = 0;
  ps->edgeWidth[4] = 0;
  ps->creaseAngle = 80;
  ps->bgGray = 0.9;
}

limnWin *
limnWinNew(int device) {
  limnWin *win;

  win = (limnWin *)calloc(1, sizeof(limnWin));
  if (win) {
    win->device = device;
    switch(device) {
    case limnDevicePS:
      win->yFlip = 1;
      _limnOptsPSDefaults(&(win->ps));
      break;
    }
    win->scale = 72;
    win->file = NULL;
  }
  return win;
}

limnWin *
limnWinNix(limnWin *win) {

  if (win) {
    free(win);
  }
  return NULL;
}
