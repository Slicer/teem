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


#include "nrrd.h"

int
nrrdArithGamma(Nrrd *nout, Nrrd *nin, double gamma, int minmax) {
  char me[]="nrrdArithGamma", err[NRRD_MED_STRLEN];
  double min, max, val;
  NRRD_BIG_INT I;
  double (*lup)(void *, NRRD_BIG_INT);
  double (*ins)(void *, NRRD_BIG_INT, double);

  if (!(nout && nin)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffSet(NRRD, err); return 1;
  }
  if (nout != nin) {
    if (nrrdCopy(nout, nin)) {
      sprintf(err, "%s: couldn't initialize by copy to output", me);
      biffSet(NRRD, err); return 1;
    }
  }
  if (!AIR_BETWEEN(nrrdMinMaxUnknown, minmax, nrrdMinMaxLast)) {
    sprintf(err, "%s: minmax behavior (%d) invalid", me, minmax);
    biffSet(NRRD, err); return 1;
  }
  if (nrrdMinMaxDo(&min, &max, nin, AIR_NAN, AIR_NAN, minmax)) {
    sprintf(err, "%s: trouble setting min, max", me);
    biffAdd(NRRD, err); return 1;
  }

  lup = nrrdDLookup[nin->type];
  ins = nrrdDInsert[nout->type];
  gamma = 1/gamma;
  if (gamma < 0.0) {
    gamma = -gamma;
    for (I=0; I<=nin->num-1; I++) {
      val = lup(nin->data, I);
      val = AIR_AFFINE(min, val, max, 0.0, 1.0);
      val = pow(val, gamma);
      val = AIR_AFFINE(1.0, val, 0.0, min, max);
      ins(nout->data, I, val);
    }
  }
  else {
    for (I=0; I<=nin->num-1; I++) {
      val = lup(nin->data, I);
      val = AIR_AFFINE(min, val, max, 0.0, 1.0);
      val = pow(val, gamma);
      val = AIR_AFFINE(0.0, val, 1.0, min, max);
      ins(nout->data, I, val);
    }
  }
  
  return 0;
}
