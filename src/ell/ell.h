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

#ifndef ELL_HAS_BEEN_INCLUDED
#define ELL_HAS_BEEN_INCLUDED
#ifdef __cplusplus
extern "C" {
#endif

#define ELL "ell"

#include <math.h>
#include <air.h>

#include "ellMacros.h"

/*
******** ellCubicRoot enum
**
** return values for ellCubic
*/
typedef enum {
  ellCubicRootUnknown,         /* 0 */
  ellCubicRootSingle,          /* 1 */
  ellCubicRootTriple,          /* 2 */
  ellCubicRootSingleDouble,    /* 3 */
  ellCubicRootThree,           /* 4 */
  ellCubicRootLast             /* 5 */
} ellCubicRoot;

/* misc.c */
extern int ellDebug;
extern void ell3mPrint_f(FILE *f, float s[9]);
extern void ell3vPrint_f(FILE *f, float s[3]);
extern void ell3mPrint_d(FILE *f, double s[9]);
extern void ell3vPrint_d(FILE *f, double s[3]);
extern void ell4mPrint_f(FILE *f, float s[16]);
extern void ell4vPrint_f(FILE *f, float s[3]);
extern void ell4mPrint_d(FILE *f, double s[16]);
extern void ell4vPrint_d(FILE *f, double s[4]);

/* vec.c */
extern void ell3vPerp_f(float p[3], float v[3]);
extern void ell3vPerp_d(double p[3], double v[3]);

/* cubic.c */
extern int ellCubic(double root[3], double A, double B, double C, int polish);

/* eigen.c */
extern void ell3mNullspace1(double ans[3], double n[9]);
extern void ell3mNullspace2(double ans0[3], double ans1[3], double n[9]);
extern int ell3mEigenvalues(double eval[3], double m[9], 
			    int polish);
extern int ell3mEigensolve(double eval[3], double evec[9], double m[9],
			   int polish);



#ifdef __cplusplus
}
#endif
#endif /* ELL_HAS_BEEN_INCLUDED */
