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


