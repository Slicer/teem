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

#ifndef ELL_HAS_BEEN_INCLUDED
#define ELL_HAS_BEEN_INCLUDED

#include <math.h>
#include <air.h>

#include "ellMacros.h"

#if defined(_WIN32) && defined(TEEM_DLL)
#define ell_export __declspec(dllimport)
#else
#define ell_export
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define ELL "ell"

/*
******** ellCubicRoot enum
**
** return values for ellCubic
*/
enum {
  ellCubicRootUnknown,         /* 0 */
  ellCubicRootSingle,          /* 1 */
  ellCubicRootTriple,          /* 2 */
  ellCubicRootSingleDouble,    /* 3 */
  ellCubicRootThree,           /* 4 */
  ellCubicRootLast             /* 5 */
};

/* miscEll.c */
extern ell_export int ellDebug;
extern void ell3mPrint_f(FILE *f, float s[9]);
extern void ell3vPrint_f(FILE *f, float s[3]);
extern void ell3mPrint_d(FILE *f, double s[9]);
extern void ell3vPrint_d(FILE *f, double s[3]);
extern void ell4mPrint_f(FILE *f, float s[16]);
extern void ell4vPrint_f(FILE *f, float s[4]);
extern void ell4mPrint_d(FILE *f, double s[16]);
extern void ell4vPrint_d(FILE *f, double s[4]);

/* vecEll.c */
extern void ell3vPerp_f(float p[3], float v[3]);
extern void ell3vPerp_d(double p[3], double v[3]);
extern void ell3mvMul_f(float v2[3], float m[9], float v1[3]);
extern void ell3mvMul_d(double v2[3], double m[9], double v1[3]);
extern void ell4mvMul_f(float v2[4], float m[16], float v1[4]);
extern void ell4mvMul_d(double v2[4], double m[16], double v1[4]);

/* mat.c */
extern void ell3mMul_f(float m3[9], float m1[9], float m2[9]);
extern void ell3mMul_d(double m3[9], double m1[9], double m2[9]);
extern void ell3mPreMul_f(float m[9], float x[9]);
extern void ell3mPreMul_d(double m[9], double x[9]);
extern void ell3mPostMul_f(float m[9], float x[9]);
extern void ell3mPostMul_d(double m[9], double x[9]);
extern float ell3mDet_f(float m[9]);
extern double ell3mDet_d(double m[9]);
extern void ell3mInvert_f(float i[9], float m[9]);
extern void ell3mInvert_d(double i[9], double m[9]);
extern void ell4mMul_f(float m3[16], float m1[16], float m2[16]);
extern void ell4mMul_d(double m3[16], double m1[16], double m2[16]);
extern void ell4mPreMul_f(float m[16], float x[16]);
extern void ell4mPreMul_d(double m[16], double x[16]);
extern void ell4mPostMul_f(float m[16], float x[16]);
extern void ell4mPostMul_d(double m[16], double x[16]);
extern float ell4mDet_f(float m[16]);
extern double ell4mDet_d(double m[16]);
extern void ell4mInvert_f(float i[16], float m[16]);
extern void ell4mInvert_d(double i[16], double m[16]);

/* cubic.c */
extern int ellCubic(double root[3], double A, double B, double C, int newton);

/* eigen.c */
extern void ell3mNullspace1(double ans[3], double n[9]);
extern void ell3mNullspace2(double ans0[3], double ans1[3], double n[9]);
extern int ell3mEigenvalues(double eval[3], double m[9], 
			    int newton);
extern int ell3mEigensolve(double eval[3], double evec[9], double m[9],
			   int newton);

#ifdef __cplusplus
}
#endif

#endif /* ELL_HAS_BEEN_INCLUDED */
