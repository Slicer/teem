#include "bane.h"

/*
**
**     obviously, it is extremely important 
**     that all these be kept in sync.
**
*/

char 
baneRangeStr[BANE_RANGE_MAX+1][BANE_SMALL_STRLEN] = {
  "unknown",
  "positive",
  "negative",
  "zero-centered",
  "anywhere"
};

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

  

