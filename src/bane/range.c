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


