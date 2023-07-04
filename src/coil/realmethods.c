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

#include "coil.h"
/* clang-format off */

/* ------------------------------------------ */
static const coilMethod
_coilMethodTesting = {
  "testing",
  coilMethodTypeTesting,
  0
};

/* ------------------------------------------ */
static const coilMethod
_coilMethodHomogeneous = {
  "homogeneous",
  coilMethodTypeHomogeneous,
  1
};

/* ------------------------------------------ */
static const coilMethod
_coilMethodPeronaMalik = {
  "perona-malik",
  coilMethodTypePeronaMalik,
  2
};

/* ------------------------------------------ */
static const coilMethod
_coilMethodModifiedCurvature = {
  "modified-curvature",
  coilMethodTypeModifiedCurvature,
  3
};

/* ------------------------------------------ */
static const coilMethod
_coilMethodModifiedCurvatureRings = {
  "modified-curvature-rings",
  coilMethodTypeModifiedCurvatureRings,
  6
};

/* ------------------------------------------ */
static const coilMethod
_coilMethodSelf = {
  "self",
  coilMethodTypeSelf,
  1
};

/* ------------------------------------------ */
static const coilMethod
_coilMethodFinish = {
  "finish",
  coilMethodTypeFinish,
  4
};

/* ------------------------------------------ */
const coilMethod* const
coilMethodArray[COIL_METHOD_TYPE_MAX+1] = {
  NULL,
  &_coilMethodTesting,
  &_coilMethodHomogeneous,
  &_coilMethodPeronaMalik,
  &_coilMethodModifiedCurvature,
  &_coilMethodModifiedCurvatureRings,
  NULL,
  &_coilMethodSelf,
  &_coilMethodFinish
};
/* clang-format on */
