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

void
_coilKind7TensorFilterTesting(coil_t *delta, coil_t **iv3, 
			      double spacing[3],
			      double parm[COIL_PARMS_NUM]) {
  delta[0] = 0;
  delta[1] = 0;
  delta[2] = 0;
  delta[3] = 0;
  delta[4] = 0;
  delta[5] = 0;
  delta[6] = 0;
}

void
_coilKind7TensorFilterHomogeneous(coil_t *delta, coil_t **iv3, 
				  double spacing[3],
				  double parm[COIL_PARMS_NUM]) {
  
}

void
_coilKindTensorUpdate(coil_t *val, coil_t *delta) {
  
  val[0] += delta[0]; /* WARNING: this could change confidence! */
  val[1] += delta[1];
  val[2] += delta[2];
  val[3] += delta[3];
  val[4] += delta[4];
  val[5] += delta[5];
  val[6] += delta[6];
}

const coilKind
_coilKindTensor = {
  "tensor",
  7,
  {NULL,
   _coilKind7TensorFilterTesting,
   _coilKind7TensorFilterHomogeneous,
   NULL,
   NULL,
   NULL},
  _coilKindTensorUpdate
};

const coilKind *
coilKindTensor = &_coilKindTensor;
