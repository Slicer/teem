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

#ifndef TEN_HAS_BEEN_INCLUDED
#define TEN_HAS_BEEN_INCLUDED
#ifdef __cplusplus
extern "C" {
#endif

#define TEN "ten"

#include <math.h>
#include <air.h>
#include <biff.h>
#include <ell.h>
#include <nrrd.h>
#include <dye.h>
#include <limn.h>

#include "tenMacros.h"

typedef enum {
  tenAnisoUnknown,    /* 0: nobody knows */
  tenAnisoC_l,        /* 1: linear */
  tenAnisoC_p,        /* 2: planar */
  tenAnisoC_a,        /* 3: linear + planar */
  tenAnisoC_s,        /* 4: spherical */
  tenAnisoC_t,        /* 5: gk's anisotropy type */
  tenAnisoLast
} tenAniso;
#define TEN_MAX_ANISO 5
  
/* tensor.c */
extern int tenVerbose;
extern int tenValidTensor(Nrrd *nin, int wantType, int useBiff);
extern int tenEigensolve(float eval[3], float evec[9], float t[7]);

/* chan.c */
extern void tenCalcOneTensor(float tens[7], float chan[7], 
			     float thresh, float slope, float b);
extern int tenCalcTensor(Nrrd *nout, Nrrd *nin, 
			 float thresh, float slope, float b);

/* aniso.c */
extern void tenAnisotropy(float c[TEN_MAX_ANISO+1], float eval[3]);

/* glyph.c */
extern int tenGlyphGen(limnObj *obj, Nrrd *nin, float useColor,
		       float anisoThresh, float anisoType,
		       float thresh, float cscale);


#ifdef __cplusplus
}
#endif
#endif /* TEN_HAS_BEEN_INCLUDED */

