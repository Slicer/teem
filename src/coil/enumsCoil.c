/*
  teem: Gordon Kindlmann's research software
  Copyright (C) 2004, 2003, 2002, 2001, 2000, 1999, 1998 University of Utah

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

#include "coil.h"

char
_coilMethodTypeStr[COIL_METHOD_TYPE_MAX+1][AIR_STRLEN_SMALL] = {
  "(unknown_method)",
  "testing",
  "isotropic",
  "anisotropic",
  "modified curvature",
  "curvature flow"
};

char
_coilMethodTypeDesc[COIL_METHOD_TYPE_MAX+1][AIR_STRLEN_MED] = {
  "unknown_method",
  "nothing, actually, just here for testing",
  "isotropic diffusion (Gaussian blurring)",
  "anisotropic diffusion (Perona-Malik)",
  "modified curvature diffusion",
  "curvature flow"
};

char
_coilMethodTypeStrEqv[][AIR_STRLEN_SMALL] = {
  "test", "testing",
  "iso", "isotropic",
  "aniso", "anisotropic",
  "mcde",
  "flow", 
  ""
};

int
_coilMethodTypeValEqv[] = {
  coilMethodTypeTesting, coilMethodTypeTesting,
  coilMethodTypeIsotropic, coilMethodTypeIsotropic,
  coilMethodTypeAnisotropic, coilMethodTypeAnisotropic,
  coilMethodTypeModifiedCurvature,
  coilMethodTypeCurvatureFlow
};

airEnum
_coilMethodType = {
  "method",
  COIL_METHOD_TYPE_MAX,
  _coilMethodTypeStr,  NULL,
  _coilMethodTypeDesc,
  _coilMethodTypeStrEqv, _coilMethodTypeValEqv,
  AIR_FALSE
};
airEnum *
coilMethodType = &_coilMethodType;

/* -------------------------------------------------- */

char
_coilKindTypeStr[COIL_KIND_TYPE_MAX+1][AIR_STRLEN_SMALL] = {
  "(unknown_kind)",
  "scalar",
  "3color",
  "7tensor"
};

char
_coilKindTypeDesc[COIL_KIND_TYPE_MAX+1][AIR_STRLEN_MED] = {
  "unknown_kind",
  "plain old scalar quantities",
  "3-component color",
  "ten-style 7-valued tensor"
};

char
_coilKindTypeStrEqv[][AIR_STRLEN_SMALL] = {
  "scalar",
  "3color",
  "7tensor",
  ""
};

int
_coilKindTypeValEqv[] = {
  coilKindTypeScalar,
  coilKindType3Color,
  coilKindType7Tensor
};

airEnum
_coilKindType = {
  "kind",
  COIL_KIND_TYPE_MAX,
  _coilKindTypeStr,  NULL,
  _coilKindTypeDesc,
  _coilKindTypeStrEqv, _coilKindTypeValEqv,
  AIR_FALSE
};
airEnum *
coilKindType = &_coilKindType;
