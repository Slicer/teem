/*
  Teem: Tools to process and visualize scientific data and images              
  Copyright (C) 2008, 2007, 2006, 2005  Gordon Kindlmann
  Copyright (C) 2004, 2003, 2002, 2001, 2000, 1999, 1998  University of Utah

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public License
  (LGPL) as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  The terms of redistributing and/or modifying this software also
  include exceptions to the LGPL that facilitate static linking.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with this library; if not, write to Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "meet.h"

const char *
meetBiffKey = "meet";

/*
******** meetAirEnumAll
**
** ALLOCATES and returns a NULL-terminated array of 
** pointers to all the airEnums in Teem 
**
** It would be better if this array could be created at compile-time,
** but efforts at doing this resulted in lots of "initializer is not const"
** errors...
*/
const airEnum **
meetAirEnumAll() {
  airArray *arr;
  const airEnum **enm;

  arr = airArrayNew(AIR_CAST(void **, &enm), NULL, sizeof(airEnum *), 2);
  enm[airArrayLenIncr(arr, 1)] = airEndian;
  enm[airArrayLenIncr(arr, 1)] = airBool;
  /* hest: no airEnums */
  /* biff: no airEnums */
  enm[airArrayLenIncr(arr, 1)] = nrrdFormatType;
  enm[airArrayLenIncr(arr, 1)] = nrrdType;
  enm[airArrayLenIncr(arr, 1)] = nrrdEncodingType;
  enm[airArrayLenIncr(arr, 1)] = nrrdCenter;
  enm[airArrayLenIncr(arr, 1)] = nrrdKind;
  enm[airArrayLenIncr(arr, 1)] = nrrdField;
  enm[airArrayLenIncr(arr, 1)] = nrrdSpace;
  enm[airArrayLenIncr(arr, 1)] = nrrdSpacingStatus;
  enm[airArrayLenIncr(arr, 1)] = nrrdBoundary;
  enm[airArrayLenIncr(arr, 1)] = nrrdMeasure;
  enm[airArrayLenIncr(arr, 1)] = nrrdUnaryOp;
  enm[airArrayLenIncr(arr, 1)] = nrrdBinaryOp;
  enm[airArrayLenIncr(arr, 1)] = nrrdTernaryOp;
  enm[airArrayLenIncr(arr, 1)] = ell_cubic_root;
  /* unrrdu: no airEnums */
  /* dye: no airEnums */
  /* moss: no airEnums */
  enm[airArrayLenIncr(arr, 1)] = alanStop;
  enm[airArrayLenIncr(arr, 1)] = gageErr;
  enm[airArrayLenIncr(arr, 1)] = gageKernel;
  enm[airArrayLenIncr(arr, 1)] = gageScl;
  enm[airArrayLenIncr(arr, 1)] = gageVec;
  enm[airArrayLenIncr(arr, 1)] = baneGkmsMeasr;
  enm[airArrayLenIncr(arr, 1)] = limnSpace;
  enm[airArrayLenIncr(arr, 1)] = limnPolyDataInfo;
  enm[airArrayLenIncr(arr, 1)] = limnCameraPathTrack;
  enm[airArrayLenIncr(arr, 1)] = limnPrimitive;
  enm[airArrayLenIncr(arr, 1)] = limnSplineType;
  enm[airArrayLenIncr(arr, 1)] = limnSplineInfo;
  enm[airArrayLenIncr(arr, 1)] = seekType;
  enm[airArrayLenIncr(arr, 1)] = hooverErr;
  enm[airArrayLenIncr(arr, 1)] = echoJitter;
  enm[airArrayLenIncr(arr, 1)] = echoType;
  enm[airArrayLenIncr(arr, 1)] = echoMatter;
  enm[airArrayLenIncr(arr, 1)] = tenAniso;
  enm[airArrayLenIncr(arr, 1)] = tenInterpType;
  enm[airArrayLenIncr(arr, 1)] = tenGage;
  enm[airArrayLenIncr(arr, 1)] = tenFiberType;
  enm[airArrayLenIncr(arr, 1)] = tenDwiFiberType;
  enm[airArrayLenIncr(arr, 1)] = tenFiberStop;
  enm[airArrayLenIncr(arr, 1)] = tenFiberIntg;
  enm[airArrayLenIncr(arr, 1)] = tenGlyphType;
  enm[airArrayLenIncr(arr, 1)] = tenEstimate1Method;
  enm[airArrayLenIncr(arr, 1)] = tenEstimate2Method;
  enm[airArrayLenIncr(arr, 1)] = tenTripleType;
  enm[airArrayLenIncr(arr, 1)] = miteVal;
  enm[airArrayLenIncr(arr, 1)] = miteStageOp;
  enm[airArrayLenIncr(arr, 1)] = coilMethodType;
  enm[airArrayLenIncr(arr, 1)] = coilKindType;
  enm[airArrayLenIncr(arr, 1)] = pushEnergyType;
  enm[airArrayLenIncr(arr, 1)] = pullInterType;
  enm[airArrayLenIncr(arr, 1)] = pullEnergyType;
  enm[airArrayLenIncr(arr, 1)] = pullInfo;
  enm[airArrayLenIncr(arr, 1)] = pullProcessMode;
  enm[airArrayLenIncr(arr, 1)] = NULL;
  airArrayNix(arr);
  return enm;
};

void
meetAirEnumPrintAll(FILE *file) {
  const airEnum **enm, *ee;
  unsigned int ei;

  if (!file) {
    return;
  }
  enm = meetAirEnumAll();
  ei = 0;
  while ((ee = enm[ei])) {
    airEnumPrint(file, ee);
    fprintf(file, "\n");
    ei++;
  }
  free(enm);
  return;
}
