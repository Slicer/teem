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
#include <unrrdu.h>
#include <dye.h>
#include <limn.h>

#include "tenMacros.h"

#if defined(_WIN32) && !defined(TEEM_BUILD)
#define ten_export __declspec(dllimport)
#else
#define ten_export
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define TEN "ten"

enum {
  tenAnisoUnknown,    /*  0: nobody knows */
  tenAniso_Cl1,       /*  1: Westin's linear (first version) */
  tenAniso_Cp1,       /*  2: Westin's planar (first version) */
  tenAniso_Ca1,       /*  3: Westin's linear + planar (first version) */
  tenAniso_Cs1,       /*  4: Westin's spherical (first version) */
  tenAniso_Ct1,       /*  5: gk's anisotropy type (first version) */
  tenAniso_Cl2,       /*  6: Westin's linear (second version) */
  tenAniso_Cp2,       /*  7: Westin's planar (second version) */
  tenAniso_Ca2,       /*  8: Westin's linear + planar (second version) */
  tenAniso_Cs2,       /*  9: Westin's spherical (second version) */
  tenAniso_Ct2,       /* 10: gk's anisotropy type (second version) */
  tenAniso_RA,        /* 11: Bass+Pier's relative anisotropy */
  tenAniso_FA,        /* 12: (Bass+Pier's fractional anisotropy)/sqrt(2) */
  tenAniso_VF,        /* 13: volume fraction = 1-(Bass+Pier's volume ratio) */
  tenAniso_RR,        /* 14: based on radius of circle in cubic solve */
  tenAnisoLast
};
#define TEN_ANISO_MAX    14

typedef struct {
  Nrrd *vThreshVol;
  float anisoType, anisoThresh;
  float dwiThresh, vThresh, useColor;
  float thresh, cscale;
  float sumFloor, sumCeil;
  float fakeSat;
  int dim;
} tenGlyphParm;

#define TEN_ANISO_DESC \
  "All the Westin metrics come in two versions.  Currently supported:\n " \
  "\b\bo \"cl1\", \"cl2\": Westin's linear\n " \
  "\b\bo \"cp1\", \"cp2\": Westin's planar\n " \
  "\b\bo \"ca1\", \"ca2\": Westin's linear + planar\n " \
  "\b\bo \"cs1\", \"cs2\": Westin's spherical (1-ca)\n " \
  "\b\bo \"ct1\", \"ct2\": GK's anisotropy type (cp/ca)\n " \
  "\b\bo \"ra\": Basser/Pierpaoli relative anisotropy\n " \
  "\b\bo \"fa\": Basser/Pierpaoli fractional anisotropy/sqrt(2)\n " \
  "\b\bo \"vf\": volume fraction = 1-(Basser/Pierpaoli volume ratio)"

/* arraysTen.c */
extern ten_export airEnum *tenAniso;

/* methodsTen.c */
extern tenGlyphParm *tenGlyphParmNew();
extern tenGlyphParm *tenGlyphParmNix(tenGlyphParm *parm);

/* tensor.c */
extern ten_export int tenVerbose;
extern int tenTensorCheck(Nrrd *nin, int wantType, int useBiff);
extern int tenExpand(Nrrd *tnine, Nrrd *tseven, float scale, float thresh);
extern int tenShrink(Nrrd *tseven, Nrrd *nconf, Nrrd *tnine);
extern int tenEigensolve(float eval[3], float evec[9], float t[7]);
extern int tenTensorMake(Nrrd *nout, Nrrd *nconf, Nrrd *neval, Nrrd *nevec);

/* chan.c */
extern void tenCalcOneTensor(float tens[7], float chan[7], 
			     float thresh, float slope, float b);
extern int tenCalcTensor(Nrrd *nout, Nrrd *nin, 
			 float thresh, float slope, float b);

/* aniso.c */
extern ten_export float tenAnisoSigma;  /* added to denominator
					   in Westin anisos */
extern void tenAnisoCalc(float c[TEN_ANISO_MAX+1], float eval[3]);
extern int tenAnisoPlot(Nrrd *nout, int aniso, int res);
extern int tenAnisoVolume(Nrrd *nout, Nrrd *nin, int aniso, float thresh);
extern int tenAnisoHistogram(Nrrd *nout, Nrrd *nin,
			     int version, int resolution);

/* miscTen.c */
extern short tenEvqOne(float vec[3], float scl);
extern int tenEvqVolume(Nrrd *nout, Nrrd *nin, int which, int aniso);

/* glyph.c */
extern int tenGlyphGen(limnObj *obj, Nrrd *nin, tenGlyphParm *parm);

/* tend{Flotsam,Anplot,Anvol,Evec,Eval,Expand}.c */
#define TEND_DECLARE(C) extern ten_export unrrduCmd tend_##C##Cmd;
#define TEND_LIST(C) &tend_##C##Cmd,
#define TEND_MAP(F) \
F(make) \
F(calc) \
F(anplot) \
F(anvol) \
F(anhist) \
F(point) \
F(eval) \
F(evec) \
F(evq) \
F(expand) \
F(shrink) \
F(satin)
TEND_MAP(TEND_DECLARE)
extern ten_export unrrduCmd *tendCmdList[]; 
extern void tendUsage(char *me, hestParm *hparm);


#ifdef __cplusplus
}
#endif

#endif /* TEN_HAS_BEEN_INCLUDED */
