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

#ifndef TEN_HAS_BEEN_INCLUDED
#define TEN_HAS_BEEN_INCLUDED

#include <math.h>
#include <air.h>
#include <biff.h>
#include <ell.h>
#include <nrrd.h>
#include <dye.h>
#include <limn.h>

#include "tenMacros.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TEN "ten"

enum {
  tenAnisoUnknown,    /* 0: nobody knows */
  tenAniso_Cl,        /* 1: Westin's linear */
  tenAniso_Cp,        /* 2: Westin's planar */
  tenAniso_Ca,        /* 3: Westin's linear + planar */
  tenAniso_Cs,        /* 4: Westin's spherical */
  tenAniso_Ct,        /* 5: gk's anisotropy type */
  tenAniso_RA,        /* 6: Bass+Pier's relative anisotropy */
  tenAniso_FA,        /* 7: (Bass+Pier's fractional anisotropy)/sqrt(2) */
  tenAniso_VF,        /* 8: volume fraction = 1-(Bass+Pier's volume ratio) */
  tenAnisoLast
};
#define TEN_ANISO_MAX    8

typedef struct {
  Nrrd *vThreshVol;
  float anisoType, anisoThresh;
  float dwiThresh, vThresh, useColor;
  float thresh, cscale;
  float sumFloor, sumCeil;
  float fakeSat;
  int dim;
} tenGlyphParm;

/* arraysTen.c */
extern airEnum *tenAniso;

/* methodsTen.c */
extern tenGlyphParm *tenGlyphParmNew();
extern tenGlyphParm *tenGlyphParmNix(tenGlyphParm *parm);

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
extern void tenAnisoCalc(float c[TEN_ANISO_MAX+1], float eval[3]);
extern int tenAnisoVolume(Nrrd *nout, Nrrd *nin, float anis);

/* glyph.c */
extern int tenGlyphGen(limnObj *obj, Nrrd *nin, tenGlyphParm *parm);

#ifdef __cplusplus
}
#endif

#endif /* TEN_HAS_BEEN_INCLUDED */
