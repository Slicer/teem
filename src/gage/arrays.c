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

/*
******** gageSclAnsLength[]
**
** the number of GPTs used for each answer
*/
int
gageSclAnsLength[GAGE_SCL_MAX+1] = {
  1,  3,  1,  3,  9,  3,  9,  1,  9,  2,  6,  1,  1
};

/*
******** gageSclAnsOffset[]
**
** the index into the answer array of the first element of the answer
*/
int
gageSclAnsOffset[GAGE_SCL_MAX+1] = {
  0,  1,  4,  5,  8, 17, 20, 29, 30, 39, 41, 47, 48
};

/*
** _gageSclNeedDeriv[]
**
** highest order derivative needed for each different query
*/
int
_gageSclNeedDeriv[GAGE_SCL_MAX+1] = {
  0,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,  2,  2
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
  /*  0: gageSclValue */
  0,

  /*  1: gageSclGradVec */
  0,

  /*  2: gageSclGradMag */
  (1<<gageSclGradVec),

  /*  3: gageSclNormal */
  (1<<gageSclGradVec) | (1<<gageSclGradMag),

  /*  4: gageScalarHess */
  0,

  /*  5: gageSclHessEval */
  (1<<gageSclHess),

  /*  6: gageSclHessEvec */
  (1<<gageSclHess) | (1<<gageSclHessEval),

  /*  7: gageScl2ndDD */
  (1<<gageSclHess) | (1<<gageSclNormal),

  /*  8: gageSclGeomTens */
  (1<<gageSclHess) | (1<<gageSclNormal) | (1<<gageSclGradMag),
  
  /*  9: gageSclK1K2 */
  (1<<gageSclGeomTens),

  /* 10: gageSclCurvDir */
  (1<<gageSclGeomTens) | (1<<gageSclNormal),
  
  /* 11: gageSclShapeIndex */
  (1<<gageSclK1K2),

  /* 12: gageSclCurvedness */
  (1<<gageSclK1K2)
};
