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
#include "private.h"

/* ----------------- baneUnknown -------------------- */

void
_baneRangeUnknown_Ans(double *ominP, double *omaxP,
		      double imin, double imax) {
  char me[]="_baneRangeUnknown_Ans";
  fprintf(stderr, "%s: a baneRange is unset somewhere ...\n", me);
}

baneRange
_baneRangeUnknown = {
  "unknown",
  baneRangeUnknown_e,
  _baneRangeUnknown_Ans
};
baneRange *
baneRangeUnknown = &_baneRangeUnknown;

/* ----------------- baneRangePos -------------------- */

void
_baneRangePos_Ans(double *ominP, double *omaxP,
		  double imin, double imax) {
  *ominP = 0;
  *omaxP = imax;
}

baneRange
_baneRangePos = {
  "positive",
  baneRangePos_e,
  _baneRangePos_Ans
};
baneRange *
baneRangePos = &_baneRangePos;

/* ----------------- baneRangeNeg -------------------- */

void
_baneRangeNeg_Ans(double *ominP, double *omaxP,
		  double imin, double imax) {
  
  *ominP = imin;
  *omaxP = 0;
}

baneRange
_baneRangeNeg = {
  "negative",
  baneRangeNeg_e,
  _baneRangeNeg_Ans
};
baneRange *
baneRangeNeg = &_baneRangeNeg;

/* ----------------- baneRangeCent -------------------- */

/*
** _baneRangeZeroCent_Ans
**
** Unlike the last version of this function, this is conservative: we
** choose the smallest zero-centered range that includes the original
** min and max.  Previously the average of the min and max magnitude
** were used.
*/
void
_baneRangeZeroCent_Ans(double *ominP, double *omaxP,
		       double imin, double imax) {

  imin = AIR_MIN(imin, 0);
  imax = AIR_MAX(imax, 0);
  /* now the signs of imin and imax aren't wrong */
  *ominP = AIR_MIN(-imax, imin);
  *omaxP = AIR_MAX(imax, -imin);
}

baneRange
_baneRangeZeroCent = {
  "zero-center",
  baneRangeZeroCent_e,
  _baneRangeZeroCent_Ans
};
baneRange *
baneRangeZeroCent = &_baneRangeZeroCent;

/* ----------------- baneRangeFloat -------------------- */

void
_baneRangeFloat_Ans(double *ominP, double *omaxP,
		    double imin, double imax) {
  *ominP = imin;
  *omaxP = imax;
}

baneRange
_baneRangeFloat = {
  "float",
  baneRangeFloat_e,
  _baneRangeFloat_Ans
};
baneRange *
baneRangeFloat = &_baneRangeFloat;

/* ----------------------------------------------------- */

baneRange *
baneRangeArray[BANE_RANGE_MAX+1] = {
  &_baneRangeUnknown,
  &_baneRangePos,
  &_baneRangeNeg,
  &_baneRangeZeroCent,
  &_baneRangeFloat
};
