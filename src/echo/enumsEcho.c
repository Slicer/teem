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

/* ------------------------------- jitter --------------------------- */

char
_echoJitterStr[ECHO_JITTER_MAX+1][AIR_STRLEN_SMALL] = {
  "(unknown_jitter)",
  "none",
  "grid",
  "jitter",
  "random"
};

char
_echoJitterDesc[ECHO_JITTER_MAX+1][AIR_STRLEN_MED] = {
  "unknown_jitter",
  "no jittering; samples are always at center of region",
  "samples are at regular grid vertices",
  "samples are randomly located with grid cells",
  "samples are just random"
};

char
_echoJitterStrEqv[][AIR_STRLEN_SMALL] = {
  "none",
  "grid", "regular",
  "jitter",
  "random"
};

int
_echoJitterValEqv[] = {
  echoJitterNone,
  echoJitterGrid, echoJitterGrid, 
  echoJitterJitter,
  echoJitterRandom
};

airEnum
_echoJitter_ae = {
  "jitter",
  ECHO_JITTER_MAX,
  _echoJitterStr,  NULL,
  _echoJitterDesc,
  _echoJitterStrEqv, _echoJitterValEqv,
  AIR_FALSE
};
airEnum *
echoJitter_ae = &_echoJitter_ae;

/* ------------------------------- object --------------------------- */

char
_echoObjectStr[ECHO_OBJECT_MAX+1][AIR_STRLEN_SMALL] = {
  "(unknown_object)",
  "sphere",
  "cube",
  "triangle",
  "rectangle",
  "mesh",
  "isosurface",
  "AABoundingBox",
  "split",
  "list",
  "instance"
};

char
_echoObjectDesc[ECHO_OBJECT_MAX+1][AIR_STRLEN_MED] = {
  "unknown_object",
  "sphere",
  "unit cube, centered at the origin",
  "triangle",
  "rectangle",
  "mesh of triangles",
  "isosurface of some volume dataset",
  "axis-aligned bounding box",
  "split",
  "list",
  "instance"
};

char
_echoObjectStrEqv[][AIR_STRLEN_SMALL] = {
  "sphere",
  "cube",
  "triangle", "tri",
  "rectangle", "rect",
  "mesh", "tri-mesh", "trimesh",
  "isosurface",
  "AABoundingBox",
  "split",
  "list",
  "instance"
};

int
_echoObjectValEqv[] = {
  echoSphere,
  echoCube,
  echoTriangle, echoTriangle,
  echoRectangle, echoRectangle,
  echoTriMesh, echoTriMesh, echoTriMesh,
  echoIsosurface,
  echoAABBox,
  echoSplit,
  echoList,
  echoInstance
};

airEnum
_echoObject_ae = {
  "object",
  ECHO_OBJECT_MAX,
  _echoObjectStr,  NULL,
  _echoObjectDesc,
  _echoObjectStrEqv, _echoObjectValEqv,
  AIR_FALSE
};
airEnum *
echoObject_ae = &_echoObject_ae;
