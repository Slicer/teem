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

#include "coil.h"

/*
**  x ----> X
**   \   0  1  2 
**  |  \   3  4  5
**  |    Y   6  7  8
**  |
**  |    9 10 11   
**  Z      12 13 14
**           15 16 17
**  
**       18 19 20
**         21 22 23
**           24 25 26
*/

coil_t
_coilLaplacian3(coil_t *iv3, double spacing[3]) {
  
  return ((iv3[12] - 2*iv3[13] + iv3[14])/(spacing[0]*spacing[0])
	  + (iv3[10] - 2*iv3[13] + iv3[16])/(spacing[1]*spacing[1])
	  + (iv3[ 4] - 2*iv3[13] + iv3[22])/(spacing[2]*spacing[2]));
}

void
_coilKindScalarFilterTesting(coil_t *delta, coil_t *iv3, 
			     double spacing[3],
			     double parm[COIL_PARMS_NUM]) {
  delta[0] = 0;
}

void
_coilKindScalarFilterIsotropic(coil_t *delta, coil_t *iv3,
			       double spacing[3],
			       double parm[COIL_PARMS_NUM]) {
  coil_t lapl;

  lapl = _coilLaplacian3(iv3, spacing);
  delta[0] = parm[0]*lapl;
}

void
_coilKindScalarUpdate(coil_t *val, coil_t *delta) {
  
  val[0] += delta[0];
}

const coilKind
_coilKindScalar = {
  "scalar",
  1,
  {NULL,
   _coilKindScalarFilterTesting,
   _coilKindScalarFilterIsotropic,
   NULL,
   NULL,
   NULL},
  _coilKindScalarUpdate
};

const coilKind *
coilKindScalar = &_coilKindScalar;
