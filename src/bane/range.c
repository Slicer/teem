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


#include "bane.h"

void
_baneRangeUnknown(double *nminP, double *nmaxP, double min, double max) {
  char me[]="_baneRangeUnknown";

  fprintf(stderr, "%s: Need To Specify A Range Method !!!\n", me);
}

void
_baneRangePos(double *nminP, double *nmaxP, double min, double max) {
  
  *nminP = 0;
  *nmaxP = max;
}

void
_baneRangeNeg(double *nminP, double *nmaxP, double min, double max) {
  
  *nminP = min;
  *nmaxP = 0;
}

void
_baneRangeZeroCent(double *nminP, double *nmaxP, double min, double max) {
  double mag;
  
  mag = (AIR_ABS(min) + AIR_ABS(max))/2;
  *nminP = -mag;
  *nmaxP = mag;
}

void
_baneRangeFloat(double *nminP, double *nmaxP, double min, double max) {
  
  *nminP = min;
  *nmaxP = max;
}

baneRangeType
baneRange[BANE_RANGE_MAX+1] = {
  _baneRangeUnknown,
  _baneRangePos,
  _baneRangeNeg,
  _baneRangeZeroCent,
  _baneRangeFloat
};


