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

coil_t
_coilLaplacian3(coil_t **iv3, double spacing[3]) {
  
  return (  (iv3[0][4] - 2*iv3[1][4] + iv3[2][4])/(spacing[0]*spacing[0])
	  + (iv3[1][3] - 2*iv3[1][4] + iv3[1][5])/(spacing[1]*spacing[1])
	  + (iv3[1][1] - 2*iv3[1][4] + iv3[1][7])/(spacing[2]*spacing[2]));
}

void
_coilKindScalarFilterTesting(coil_t *delta, coil_t **iv3, 
			     double spacing[3],
			     double parm[COIL_PARMS_NUM]) {
  delta[0] = 0;
}

void
_coilKindScalarFilterHomogeneous(coil_t *delta, coil_t **iv3,
				 double spacing[3],
				 double parm[COIL_PARMS_NUM]) {
  
  delta[0] = parm[0]*_coilLaplacian3(iv3, spacing);
}

/*
**  x ----> X
**   \   [0][0]  [1][0]  [2][0]
**  |  \   [0][1]  [1][1]  [2][1]
**  |    Y   [0][2]  [1][2]  [2][2]
**  |
**  |    [0][3]  [1][3]  [2][3]
**  Z      [0][4]  [1][4]  [2][4]
**           [0][5]  [1][5]  [2][5]
**  
**       [0][6]  [1][6]  [2][6]
**         [0][7]  [1][7]  [2][7]
**           [0][8]  [1][8]  [2][8]
*/

void
_coilKindScalarFilterPeronaMalik(coil_t *delta, coil_t **i,
				 double spacing[3],
				 double parm[COIL_PARMS_NUM]) {
  coil_t forwX[3], backX[3], forwY[3], backY[3], forwZ[3], backZ[3], 
    KK, rspX, rspY, rspZ;

  /* reciprocals of spacings in X, Y, and Z */
  rspX = 1.0/spacing[0];
  rspY = 1.0/spacing[1];
  rspZ = 1.0/spacing[2];

  /* gradients at forward and backward X */
  forwX[0] = rspX*(i[2][4] - i[1][4]);
  forwX[1] = rspY*(i[1][5] + i[2][5] - i[1][3] - i[2][3])/2;
  forwX[2] = rspZ*(i[1][7] + i[2][7] - i[1][1] - i[2][1])/2;
  backX[0] = rspX*(i[1][4] - i[0][4]);
  backX[1] = rspY*(i[0][5] + i[1][5] - i[0][3] - i[1][3])/2;
  backX[2] = rspZ*(i[0][7] + i[1][7] - i[0][1] - i[1][1])/2;

  /* gradients at forward and backward Y */
  forwY[0] = rspX*(i[2][4] + i[2][5] - i[0][4] - i[0][5])/2;
  forwY[1] = rspY*(i[1][5] - i[1][4]);
  forwY[2] = rspZ*(i[1][7] + i[1][8] - i[1][1] - i[1][2])/2;
  backY[0] = rspX*(i[2][3] + i[2][4] - i[0][3] - i[0][4])/2;
  backY[1] = rspY*(i[1][4] - i[1][3]);
  backY[2] = rspZ*(i[1][6] + i[1][7] - i[1][0] - i[1][1])/2;

  /* gradients at forward and backward Z */
  forwZ[0] = rspX*(i[2][4] + i[2][7] - i[0][4] - i[0][7])/2;
  forwZ[1] = rspY*(i[1][5] + i[1][8] - i[1][3] - i[1][6])/2;
  forwZ[2] = rspZ*(i[1][7] - i[1][4]);
  backZ[0] = rspX*(i[2][1] + i[2][4] - i[0][1] - i[0][4])/2;
  backZ[1] = rspY*(i[1][2] + i[1][5] - i[1][0] - i[1][3])/2;
  backZ[2] = rspZ*(i[1][4] - i[1][1]);

  /* compute fluxes */
  KK = parm[1]*parm[1];
  /*
  forwX[0] *= 1.0/(1.0 + ELL_3V_DOT(forwX, forwX)/KK);
  forwY[1] *= 1.0/(1.0 + ELL_3V_DOT(forwY, forwY)/KK);
  forwZ[2] *= 1.0/(1.0 + ELL_3V_DOT(forwZ, forwZ)/KK);
  backX[0] *= 1.0/(1.0 + ELL_3V_DOT(backX, backX)/KK);
  backY[1] *= 1.0/(1.0 + ELL_3V_DOT(backY, backY)/KK);
  backZ[2] *= 1.0/(1.0 + ELL_3V_DOT(backZ, backZ)/KK);
  */
  forwX[0] *= exp(-0.5*ELL_3V_DOT(forwX, forwX)/KK);
  forwY[1] *= exp(-0.5*ELL_3V_DOT(forwY, forwY)/KK);
  forwZ[2] *= exp(-0.5*ELL_3V_DOT(forwZ, forwZ)/KK);
  backX[0] *= exp(-0.5*ELL_3V_DOT(backX, backX)/KK);
  backY[1] *= exp(-0.5*ELL_3V_DOT(backY, backY)/KK);
  backZ[2] *= exp(-0.5*ELL_3V_DOT(backZ, backZ)/KK);

  delta[0] = parm[0]*(rspX*(forwX[0] - backX[0])
		      + rspY*(forwY[1] - backY[1])
		      + rspZ*(forwZ[2] - backZ[2]));
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
   _coilKindScalarFilterHomogeneous,
   _coilKindScalarFilterPeronaMalik,
   NULL,
   NULL},
  _coilKindScalarUpdate
};

const coilKind *
coilKindScalar = &_coilKindScalar;
