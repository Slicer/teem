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

#ifndef DYE_HAS_BEEN_INCLUDED
#define DYE_HAS_BEEN_INCLUDED

#include <stdio.h>
#include <math.h>
#include <ctype.h>

#include <teem/air.h>
#include <teem/biff.h>
#include <teem/ell.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DYE dyeBiffKey

enum {
  dyeSpaceUnknown,        /* 0: nobody knows */
  dyeSpaceHSV,            /* 1: single hexcone */
  dyeSpaceHSL,            /* 2: double hexcone */
  dyeSpaceRGB,            /* 3: obscure, deprecated */
  dyeSpaceXYZ,            /* 4: perceptual primaries */
  dyeSpaceLAB,            /* 5: 1976 CIE (L*a*b*) (based on Munsell) */
  dyeSpaceLUV,            /* 6: 1976 CIE (L*u*v*) */
  dyeSpaceLast
};
#define DYE_MAX_SPACE 6

#define DYE_VALID_SPACE(spc) \
  (AIR_IN_OP(dyeSpaceUnknown, (spc), dyeSpaceLast))

typedef struct {
  float val[2][3];        /* room for two colors: two triples of floats */
  float xWhite, yWhite;   /* chromaticity for white point */
  signed char spc[2],     /* the spaces the two colors belong to */
    ii;                   /* which (0 or 1) of the two values is current */
} dyeColor;

/* methodsDye.c */
TEEM_API const char *dyeBiffKey;
TEEM_API char dyeSpaceToStr[][AIR_STRLEN_SMALL];
TEEM_API int dyeStrToSpace(char *str);
TEEM_API dyeColor *dyeColorInit(dyeColor *col);
TEEM_API dyeColor *dyeColorSet(dyeColor *col, int space, 
                             float v0, float v1, float v2);
TEEM_API int dyeColorGet(float *v0P, float *v1P, float *v2P, dyeColor *col);
TEEM_API int dyeColorGetAs(float *v0P, float *v1P, float *v2P, 
                         dyeColor *col, int space);
TEEM_API dyeColor *dyeColorNew();
TEEM_API dyeColor *dyeColorCopy(dyeColor *c1, dyeColor *c0);
TEEM_API dyeColor *dyeColorNix(dyeColor *col);
TEEM_API int dyeColorParse(dyeColor *col, char *str);
TEEM_API char *dyeColorSprintf(char *str, dyeColor *col);

/* convertDye.c */
typedef void (*dyeConverter)(float*, float*, float*, float, float, float);
TEEM_API void dyeRGBtoHSV(float *H, float *S, float *V,
                          float  R, float  G, float  B);
TEEM_API void dyeHSVtoRGB(float *R, float *G, float *B,
                          float  H, float  S, float  V);
TEEM_API void dyeRGBtoHSL(float *H, float *S, float *L,
                          float  R, float  G, float  B);
TEEM_API void dyeHSLtoRGB(float *R, float *G, float *B,
                          float  H, float  S, float  L);
TEEM_API void dyeRGBtoXYZ(float *X, float *Y, float *Z,
                          float  R, float  G, float  B);
TEEM_API void dyeXYZtoRGB(float *R, float *G, float *B,
                          float  X, float  Y, float  Z);
TEEM_API void dyeXYZtoLAB(float *L, float *A, float *B,
                          float  X, float  Y, float  Z);
TEEM_API void dyeXYZtoLUV(float *L, float *U, float *V,
                          float  X, float  Y, float  Z);
TEEM_API void dyeLABtoXYZ(float *X, float *Y, float *Z,
                          float  L, float  A, float  B);
TEEM_API void dyeLUVtoXYZ(float *X, float *Y, float *Z,
                          float  L, float  U, float  V);
TEEM_API dyeConverter 
dyeSimpleConvert[DYE_MAX_SPACE+1][DYE_MAX_SPACE+1];
TEEM_API int dyeConvert(dyeColor *col, int space);

#ifdef __cplusplus
}
#endif

#endif /* DYE_HAS_BEEN_INCLUDED */
