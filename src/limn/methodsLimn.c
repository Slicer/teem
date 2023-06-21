/*
  Teem: Tools to process and visualize scientific data and images
  Copyright (C) 2009--2023  University of Chicago
  Copyright (C) 2005--2008  Gordon Kindlmann
  Copyright (C) 1998--2004  University of Utah

  This library is free software; you can redistribute it and/or modify it under the terms
  of the GNU Lesser General Public License (LGPL) as published by the Free Software
  Foundation; either version 2.1 of the License, or (at your option) any later version.
  The terms of redistributing and/or modifying this software also include exceptions to
  the LGPL that facilitate static linking.

  This library is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
  PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License along with
  this library; if not, write to Free Software Foundation, Inc., 51 Franklin Street,
  Fifth Floor, Boston, MA 02110-1301 USA
*/

#include "limn.h"

limnLight * /* Biff: nope */
limnLightNew(void) {
  limnLight *lit;

  lit = (limnLight *)calloc(1, sizeof(limnLight));
  return lit;
}

limnLight * /* Biff: nope */
limnLightNix(limnLight *lit) {

  if (lit) {
    free(lit);
  }
  return NULL;
}

void
limnCameraInit(limnCamera *cam) {

  if (cam) {
    cam->atRelative = limnDefCameraAtRelative;
    cam->orthographic = limnDefCameraOrthographic;
    cam->rightHanded = limnDefCameraRightHanded;
    cam->uRange[0] = cam->uRange[1] = AIR_NAN;
    cam->vRange[0] = cam->vRange[1] = AIR_NAN;
    cam->fov = AIR_NAN;
    cam->aspect = AIR_NAN;
  }
  return;
}

limnCamera * /* Biff: nope */
limnCameraNew(void) {
  limnCamera *cam;

  cam = (limnCamera *)calloc(1, sizeof(limnCamera));
  if (cam) {
    limnCameraInit(cam);
  }
  return cam;
}

limnCamera * /* Biff: nope */
limnCameraNix(limnCamera *cam) {

  if (cam) {
    free(cam);
  }
  return NULL;
}

static void
_limnOptsPSDefaults(limnOptsPS *ps) {

  ps->lineWidth[limnEdgeTypeUnknown] = AIR_NAN;
  ps->lineWidth[limnEdgeTypeBackFacet] = 0.0;
  ps->lineWidth[limnEdgeTypeBackCrease] = 0.0;
  ps->lineWidth[limnEdgeTypeContour] = 2.0;
  ps->lineWidth[limnEdgeTypeFrontCrease] = 1.0;
  ps->lineWidth[limnEdgeTypeFrontFacet] = 0.0;
  ps->lineWidth[limnEdgeTypeBorder] = 1.0;
  ps->lineWidth[limnEdgeTypeLone] = 1.0;
  ps->creaseAngle = 46;
  ps->showpage = AIR_FALSE;
  ps->wireFrame = AIR_FALSE;
  ps->noBackground = AIR_FALSE;
  ELL_3V_SET(ps->bg, 1, 1, 1);
  ELL_3V_SET(ps->edgeColor, 0, 0, 0);
}

limnWindow * /* Biff: nope */
limnWindowNew(int device) {
  limnWindow *win;

  win = (limnWindow *)calloc(1, sizeof(limnWindow));
  if (win) {
    win->device = device;
    switch (device) {
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

limnWindow * /* Biff: nope */
limnWindowNix(limnWindow *win) {

  if (win) {
    free(win);
  }
  return NULL;
}
