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


#include "nrrd.h"

/*
******** nrrdArithGamma()
**
** map the values in a nrrd through a power function; essentially:
** val = pow(val, gamma), but this is after the val has been normalized
** to be in the range of 0.0 to 1.0 (assuming that the given min and
** max really are the full range of the values in the nrrd).  Thus,
** the given min and max values are fixed points of this
** transformation.  Using a negative gamma means that after the pow()
** function has been applied, the value is inverted with respect to
** min and max.  */
int
nrrdArithGamma(Nrrd *nout, Nrrd *nin, double gamma, double min, double max) {
  char me[]="nrrdArithGamma", err[NRRD_STRLEN_MED];
  double val;
  nrrdBigInt I, num;
  double (*lup)(void *, nrrdBigInt);
  double (*ins)(void *, nrrdBigInt, double);

  if (!(nout && nin)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(NRRD, err); return 1;
  }
  if (nout != nin) {
    if (nrrdCopy(nout, nin)) {
      sprintf(err, "%s: couldn't initialize by copy to output", me);
      biffAdd(NRRD, err); return 1;
    }
  }
  if (!( AIR_EXISTS(gamma) && AIR_EXISTS(min) && AIR_EXISTS(max) )) {
    sprintf(err, "%s: not all of gamma, min, max exist", me);
    biffAdd(NRRD, err); return 1;
  }
  if (!( nrrdTypeBlock != nin->type && nrrdTypeBlock != nout->type )) {
    sprintf(err, "%s: can't deal with %s type", me,
	    nrrdEnumValToStr(nrrdEnumType, nrrdTypeBlock));
    biffAdd(NRRD, err); return 1;
  }

  lup = nrrdDLookup[nin->type];
  ins = nrrdDInsert[nout->type];
  gamma = 1/gamma;
  num = nrrdElementNumber(nin);
  if (gamma < 0.0) {
    gamma = -gamma;
    for (I=0; I<=num-1; I++) {
      val = lup(nin->data, I);
      val = AIR_AFFINE(min, val, max, 0.0, 1.0);
      val = pow(val, gamma);
      val = AIR_AFFINE(1.0, val, 0.0, min, max);
      ins(nout->data, I, val);
    }
  }
  else {
    for (I=0; I<=num-1; I++) {
      val = lup(nin->data, I);
      val = AIR_AFFINE(min, val, max, 0.0, 1.0);
      val = pow(val, gamma);
      val = AIR_AFFINE(0.0, val, 1.0, min, max);
      ins(nout->data, I, val);
    }
  }
  
  return 0;
}
