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
  ellCubicRootUnknown,
  ellCubicRootSingle,
  ellCubicRootTriple,
  ellCubicRootSingleDouble,
  ellCubicRootThree,
  ellCubicRootLast
} ellCubicRoot;

/* misc.c */
extern int ellDebug;

/* cubic.c */
extern int ellCubic(double A, double B, double C, double *root, int polish);

/* eigen.c */
extern void ell3mNullspace1(double n[9], double ans[3]);
extern void ell3mNullspace2(double n[9], double ans0[3], double ans1[3]);
extern int ell3mEigenvalues(double m[9], double eval[3], 
			    int polish);
extern int ell3mEigensolve(double m[9], double eval[3], double evec[3][3],
			   int polish);



#ifdef __cplusplus
}
#endif
#endif /* ELL_HAS_BEEN_INCLUDED */
