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

#include "gage.h"
#include "private.h"

char gageErrStr[AIR_STRLEN_LARGE];
int gageErrNum;

/* --------------------------- scl ------------------------- */

gage_t gageSclZeroNormal[3] = {1,0,0};

/*
******** gageSclAnsLength[]
**
** the number of gage_t used for each answer
*/
int
gageSclAnsLength[GAGE_SCL_MAX+1] = {
  1,  3,  1,  3,  9,  1,  3,  9,  1,  9,  2,  6,  1,  1
};

/*
******** gageSclAnsOffset[]
**
** the index into the answer array of the first element of the answer
*/
int
gageSclAnsOffset[GAGE_SCL_MAX+1] = {
  0,  1,  4,  5,  8, 17, 18, 21, 30, 31, 40, 42, 48, 49  /* 50 */
};

/*
** _gageSclNeedDeriv[]
**
** each value is a BIT FLAG representing the different value/derivatives
** that are needed to calculate the quantity.  
*/
int
_gageSclNeedDeriv[GAGE_SCL_MAX+1] = {
  1,  2,  2,  2,  4,  4,  4,  4,  6,  4,  4,  4,  4,  4
};


/*
** _gageSclPrereq[]
** 
** this records the measurements which are needed as ingredients for any
** given measurement, but it is not necessarily the recursive expansion of
** that requirement (that role is performed by gageSclSetQuery())
*/
unsigned int
_gageSclPrereq[GAGE_SCL_MAX+1] = {
  /* gageSclValue */
  0,

  /* gageSclGradVec */
  0,

  /* gageSclGradMag */
  (1<<gageSclGradVec),

  /* gageSclNormal */
  (1<<gageSclGradVec) | (1<<gageSclGradMag),

  /* gageSclHessian */
  0,

  /* gageSclLaplacian */
  (1<<gageSclHessian),   /* not really true, but this is simpler */

  /* gageSclHessEval */
  (1<<gageSclHessian),

  /* gageSclHessEvec */
  (1<<gageSclHessian) | (1<<gageSclHessEval),

  /* gageScl2ndDD */
  (1<<gageSclHessian) | (1<<gageSclNormal),

  /* gageSclGeomTens */
  (1<<gageSclHessian) | (1<<gageSclNormal) | (1<<gageSclGradMag),
  
  /* gageSclK1K2 */
  (1<<gageSclGeomTens),

  /* gageSclCurvDir */
  (1<<gageSclGeomTens) | (1<<gageSclNormal),
  
  /* gageSclShapeIndex */
  (1<<gageSclK1K2),

  /* gageSclCurvedness */
  (1<<gageSclK1K2)
};

/* --------------------------- vec ------------------------- */

/*
******** gageVecAnsLength[]
**
** the number of gage_t used for each answer
*/
int
gageVecAnsLength[GAGE_VEC_MAX+1] = {
  3,  1,  3,  9,  3,  3
};

/*
******** gageVecAnsOffset[]
**
** the index into the answer array of the first element of the answer
*/
int
gageVecAnsOffset[GAGE_VEC_MAX+1] = {
  0,  3,  4,  7, 16, 19 /* 22 */
};

/*
** _gageVecNeedDeriv[]
**
** each value is a BIT FLAG representing the different value/derivatives
** that are needed to calculate the quantity.  
*/
int
_gageVecNeedDeriv[GAGE_VEC_MAX+1] = {
  1,  1,  1,  2,  2,  2
};

/*
** _gageVecPrereq[]
** 
** this records the measurements which are needed as ingredients for any
** given measurement, but it is not necessarily the recursive expansion of
** that requirement.
*/
unsigned int
_gageVecPrereq[GAGE_VEC_MAX+1] = {
  /* gageVecVector */
  0,

  /* gageVecLength */
  (1<<gageVecVector),

  /* gageVecNormalized */
  (1<<gageVecVector) | (1<<gageVecLength),

  /* gageVecJacobian */
  0,

  /* gageVecDivergence */
  (1<<gageVecJacobian),

  /* gageVecCurl */
  (1<<gageVecJacobian)
};

