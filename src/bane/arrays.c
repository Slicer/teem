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

/*
**
**     obviously, it is extremely important 
**     that all these be kept in sync.
**
*/

/*
char 
baneRangeStr[BANE_RANGE_MAX+1][BANE_SMALL_STRLEN] = {
  "unknown",
  "positive",
  "negative",
  "zero-centered",
  "anywhere"
};
*/

char
baneMeasrStr[BANE_MEASR_MAX+1][BANE_SMALL_STRLEN] = {
  "unknown",
  "value",
  "gradient-mag_cd",
  "Laplacian_cd",
  "Hessian-2dd_cd",
  "grad-mag-grad_cd"
};
int
baneMeasrRange[BANE_MEASR_MAX+1] = {
  baneRangeUnknown,
  baneRangeFloat,
  baneRangePos,
  baneRangeZeroCent,
  baneRangeZeroCent,
  baneRangeZeroCent
};

int
baneMeasrMargin[BANE_MEASR_MAX+1] = {
  -1,
  0,
  1,
  2,
  2,
  2
};

char
baneIncStr[BANE_INC_MAX+1][BANE_SMALL_STRLEN] = {
  "unknown",
  "absolute",
  "range-ratio",
  "percentile",
  "standard-dev"
};

int
baneIncNumParm[BANE_INC_MAX+1] = {
  -1,
  2,
  1,
  2,
  2
};

char
baneClipStr[BANE_CLIP_MAX+1][BANE_SMALL_STRLEN] = {
  "unknown",
  "absolute",
  "peak-ratio",
  "percentile",
  "top-N"
};

int
baneClipNumParm[BANE_CLIP_MAX+1] = {
  -1,
  1,
  1,
  1,
  1
};

  

