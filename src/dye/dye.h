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

#ifndef DYE_HAS_BEEN_INCLUDED
#define DYE_HAS_BEEN_INCLUDED
#ifdef __cplusplus
extern "C" {
#endif

#define DYE "dye"

#include <stdio.h>
#include <math.h>
#include <ctype.h>

#include <air.h>
#include <biff.h>
#include <ell.h>

typedef enum {
  dyeSpaceUnknown,        /* 0: nobody knows */
  dyeSpaceHSV,            /* 1: single hexcone */
  dyeSpaceHSL,            /* 2: double hexcone */
  dyeSpaceRGB,            /* 3: obscure, deprecated */
  dyeSpaceXYZ,            /* 4: perceptual primaries */
  dyeSpaceLAB,            /* 5: 1976 CIE (L*a*b*) (based on Munsell) */
  dyeSpaceLUV,            /* 6: 1976 CIE (L*u*v*) */
  dyeSpaceLast
} dyeSpace;
#define DYE_MAX_SPACE 6

#define DYE_VALID_SPACE(spc) \
  (AIR_BETWEEN(dyeSpaceUnknown, spc, dyeSpaceLast))

typedef struct {
  float val[2][3];        /* room for two colors (a triple of floats) */
  float xWhite, yWhite;   /* chromaticity for white point */
  signed char spc[2],     /* the spaces the two colors belong to */
    wch;                  /* which (0 or 1) of the two values is current */
} dyeColor;


/* methods.c */
extern char dyeSpaceToStr[][AIR_SMALL_STRLEN];
extern int dyeStrToSpace(char *str);
extern dyeColor *dyeColorInit(dyeColor *col);
extern dyeColor *dyeColorSet(dyeColor *col, int space, 
			     float v0, float v1, float v2);
extern int dyeColorGet(float *v0P, float *v1P, float *v2P, dyeColor *col);
extern int dyeColorGetAs(float *v0P, float *v1P, float *v2P, 
			 dyeColor *col, int space);
extern dyeColor *dyeColorNew();
extern dyeColor *dyeColorCopy(dyeColor *c1, dyeColor *c0);
extern dyeColor *dyeColorNix(dyeColor *col);
extern int dyeColorParse(dyeColor *col, char *str);
extern char *dyeColorSprintf(char *str, dyeColor *col);

/* convert.c */
typedef void (*dyeConverter)(float*, float*, float*, float, float, float);
extern void dyeRGBtoHSV(float *H, float *S, float *V,
			float  R, float  G, float  B);
extern void dyeHSVtoRGB(float *R, float *G, float *B,
			float  H, float  S, float  V);
extern void dyeRGBtoHSL(float *H, float *S, float *L,
			float  R, float  G, float  B);
extern void dyeHSLtoRGB(float *R, float *G, float *B,
			float  H, float  S, float  L);
extern void dyeRGBtoXYZ(float *X, float *Y, float *Z,
			float  R, float  G, float  B);
extern void dyeXYZtoRGB(float *R, float *G, float *B,
			float  X, float  Y, float  Z);
extern void dyeXYZtoLAB(float *L, float *A, float *B,
			float  X, float  Y, float  Z);
extern void dyeXYZtoLUV(float *L, float *U, float *V,
			float  X, float  Y, float  Z);
extern void dyeLABtoXYZ(float *X, float *Y, float *Z,
			float  L, float  A, float  B);
extern void dyeLUVtoXYZ(float *X, float *Y, float *Z,
			float  L, float  U, float  V);
extern dyeConverter dyeSimpleConvert[DYE_MAX_SPACE+1][DYE_MAX_SPACE+1];
extern int dyeConvert(dyeColor *col, int space);


#ifdef __cplusplus
}
#endif
#endif /* DYE_HAS_BEEN_INCLUDED */
