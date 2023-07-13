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

/*
******** limnHestCameraOptAdd()
**
** calls hestOptAdd a bunch of times to set up command-line options
** useful for specifying a limnCamera.  The flags used are as follows:
** fr: cam->from
** at: cam->at
** up: cam->up
** rh: cam->rightHanded
** or: cam->orthographic
** dn: cam->neer
** di: cam->dist
** df: cam->faar
** ar: cam->atRelative
** ur: cam->uRange
** vr: cam->vRange
** fv: cam->fov
*/
void
limnHestCameraOptAdd(hestOpt **hoptP, limnCamera *cam, const char *frDef,
                     const char *atDef, const char *upDef, const char *dnDef,
                     const char *diDef, const char *dfDef, const char *urDef,
                     const char *vrDef, const char *fvDef) {
  hestOpt *hopt;

  hopt = *hoptP;
  hestOptAdd_3_Double(&hopt, "fr", "eye pos", cam->from, frDef, "camera eye point");
  hestOptAdd_3_Double(&hopt, "at", "at pos", cam->at, atDef, "camera look-at point");
  hestOptAdd_3_Double(&hopt, "up", "up dir", cam->up, upDef, "camera pseudo-up vector");
  hestOptAdd_Flag(&hopt, "rh", &(cam->rightHanded),
                  "use a right-handed UVN frame (V points down)");
  hestOptAdd_Flag(&hopt, "or", &(cam->orthographic),
                  "orthogonal (not perspective) projection");
  hestOptAdd_1_Double(&hopt, "dn", "near", &(cam->neer), dnDef,
                      "distance to near clipping plane");
  hestOptAdd_1_Double(&hopt, "di", "image", &(cam->dist), diDef,
                      "distance to image plane");
  hestOptAdd_1_Double(&hopt, "df", "far", &(cam->faar), dfDef,
                      "distance to far clipping plane");
  hestOptAdd_Flag(&hopt, "ar", &(cam->atRelative),
                  "near, image, and far plane distances are relative to "
                  "the *at* point, instead of the eye point");
  hestOptAdd_2_Double(&hopt, "ur", "uMin uMax", cam->uRange, urDef,
                      "range in U direction of image plane");
  hestOptAdd_2_Double(&hopt, "vr", "vMin vMax", cam->vRange, vrDef,
                      "range in V direction of image plane");
  hestOptAdd_1_Double(&hopt, "fv", "field of view", &(cam->fov), fvDef,
                      "angle (in degrees) vertically subtended by view window");
  *hoptP = hopt;
  return;
}
