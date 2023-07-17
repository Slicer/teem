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

#include "dye.h"

const int dyePresent = 42;

const char *const dyeBiffKey = "dye";

/* clang-format off */
const char *const
dyeSpaceToStr[DYE_MAX_SPACE+1] = {
  "(unknown)",
  "HSV",
  "HSL",
  "RGB",
  "XYZ",
  "LAB",
  "LUV",
  "LCH"
};

/* NB: the creation of dye in 2001 predates the creation of the airEnum in 2002.
GLK forgot that chronology when the dyeSpace airEnum was added belatedly in 2015,
and was bewildered why it wasn't there already. That chronology explains why this
airEnum isn't used more widely within dye. */

static const char *
_dyeSpaceDesc[DYE_MAX_SPACE+1] = {
  "unknown colorspace",
  "single hexcone",
  "double hexcone",
  "traditional device primaries",
  "CIE 1931 XYZ space",
  "CIE L*a*b*",
  "CIE 1976 L*u*v*",
  "polar coord(L*a*b*)"
};

static const airEnum
_dyeSpace = {
  "colorspace",
  DYE_MAX_SPACE,
  /* that we need this cast is a sign that the airEnum could perhaps to have more
    const-correctness, i.e. instead of the "const char **str" field maybe we should
    have "const char *const *str" or "const char *const *const str"?  Hmmm */
  AIR_CAST(const char **, dyeSpaceToStr),
  NULL,
  _dyeSpaceDesc,
  NULL, NULL,
  AIR_FALSE
};
const airEnum *const
dyeSpace = &_dyeSpace;
/* clang-format on */

/*
** this function predates the dyeSpace airEnum, so we'll keep it.
*/
int /* Biff: nope */
dyeStrToSpace(char *_str) {
  int spc;
  char *str;

  spc = dyeSpaceUnknown;
  if ((str = airStrdup(_str))) {
    airToUpper(str);
    for (spc = 0; spc < dyeSpaceLast; spc++) {
      if (!strcmp(str, dyeSpaceToStr[spc])) {
        break;
      }
    }
    if (dyeSpaceLast == spc) {
      spc = dyeSpaceUnknown;
    }
    str = (char *)airFree(str);
  }
  return spc;
}

dyeColor * /* Biff: nope */
dyeColorInit(dyeColor *col) {

  if (col) {
    ELL_3V_SET(col->val[0], AIR_NAN, AIR_NAN, AIR_NAN);
    ELL_3V_SET(col->val[1], AIR_NAN, AIR_NAN, AIR_NAN);
    col->xWhite = col->yWhite = AIR_NAN;
    col->spc[0] = dyeSpaceUnknown;
    col->spc[1] = dyeSpaceUnknown;
    col->ii = 0;
  }
  return col;
}

dyeColor * /* Biff: nope */
dyeColorSet(dyeColor *col, int space, float v0, float v1, float v2) {

  if (col && DYE_VALID_SPACE(space)) {
    col->ii = AIR_CLAMP(0, col->ii, 1);

    /* We switch to the other one if the current one seems to be used,
       but we don't switch if new and current colorspaces are the same.
       If the other one is being used too, oh well.  */
    if (dyeSpaceUnknown != col->spc[col->ii] && AIR_EXISTS(col->val[col->ii][0])
        && col->spc[col->ii] != space) {
      col->ii = 1 - col->ii;
    }

    ELL_3V_SET(col->val[col->ii], v0, v1, v2);
    col->spc[col->ii] = space;
  }
  return col;
}

int /* Biff: nope */
dyeColorGet(float *v0P, float *v1P, float *v2P, dyeColor *col) {
  int spc;

  spc = dyeSpaceUnknown;
  if (v0P && v1P && v2P && col) {
    col->ii = AIR_CLAMP(0, col->ii, 1);
    spc = col->spc[col->ii];
    ELL_3V_GET(*v0P, *v1P, *v2P, col->val[col->ii]);
  }
  return spc;
}

int /* Biff: nope */
dyeColorGetAs(float *v0P, float *v1P, float *v2P, dyeColor *colIn, int space) {
  dyeColor _col, *col;

  col = &_col;
  dyeColorCopy(col, colIn);
  /* hope for no error */
  dyeConvert(col, space);
  return dyeColorGet(v0P, v1P, v2P, col);
}

dyeColor * /* Biff: nope */
dyeColorNew() {
  dyeColor *col;

  col = (dyeColor *)calloc(1, sizeof(dyeColor));
  col = dyeColorInit(col);
  return col;
}

dyeColor * /* Biff: nope */
dyeColorCopy(dyeColor *c1, dyeColor *c0) {

  if (c1 && c0) {
    memcpy(c1, c0, sizeof(dyeColor));
  }
  return c1;
}

dyeColor * /* Biff: nope */
dyeColorNix(dyeColor *col) {

  if (col) {
    col = (dyeColor *)airFree(col);
  }
  return NULL;
}

int /* Biff: 1 */
dyeColorParse(dyeColor *col, char *_str) {
  static const char me[] = "dyeColorParse";
  char *str;
  char *colon, *valS;
  float v0, v1, v2;
  int spc;

  if (!(col && _str)) {
    biffAddf(DYE, "%s: got NULL pointer", me);
    return 1;
  }
  if (!(str = airStrdup(_str))) {
    biffAddf(DYE, "%s: couldn't strdup!", me);
    return 1;
  }
  if (!(colon = strchr(str, ':'))) {
    biffAddf(DYE, "%s: given string \"%s\" didn't contain colon", me, str);
    return 1;
  }
  *colon = '\0';
  valS = colon + 1;
  if (3 != sscanf(valS, "%g,%g,%g", &v0, &v1, &v2)) {
    biffAddf(DYE, "%s: couldn't parse three floats from \"%s\"", me, valS);
    return 1;
  }
  spc = dyeStrToSpace(str);
  if (dyeSpaceUnknown == spc) {
    biffAddf(DYE, "%s: couldn't parse colorspace from \"%s\"", me, str);
    return 1;
  }
  str = (char *)airFree(str);

  dyeColorSet(col, spc, v0, v1, v2);
  return 0;
}

char * /* Biff: nope */
dyeColorSprintf(char *str, dyeColor *col) {

  if (str && col) {
    col->ii = AIR_CLAMP(0, col->ii, 1);
    sprintf(str, "%s:%g,%g,%g", dyeSpaceToStr[col->spc[col->ii]], col->val[col->ii][0],
            col->val[col->ii][1], col->val[col->ii][2]);
  }
  return str;
}
