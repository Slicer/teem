#include "bane.h"

int
_baneClipUnknown(Nrrd *rhv, double *parm) {
  char me[]="_baneClipUnknown", err[128];

  sprintf(err, "%s: Need To Specify a Clipping Method !!!\n", me);
  biffSet(BANE, err);
  return -1;
}

int
_baneClipAbsolute(Nrrd *rhv, double *parm) {

  return parm[0];
}

int
_baneClipPeakRatio(Nrrd *rhv, double *parm) {
  int *hits, maxhits;
  NRRD_BIG_INT idx;
  
  hits = rhv->data;
  maxhits = 0;
  for (idx=0; idx<=rhv->num-1; idx++) {
    maxhits = AIR_MAX(maxhits, hits[idx]);
  }
  return maxhits*parm[0];
}

int
_baneClipCompare(const void *a, const void *b) {
  return((*(int*)a < *(int*)b 
	  ? -1 
	  : (*(int*)a > *(int*)b 
	     ? 1 
	     : 0)));
}

int
_baneClipPercentile(Nrrd *rhv, double *parm) {
  char me[]="_baneClipPercentile", err[128];
  Nrrd *copy;
  int *hits, clip;
  NRRD_BIG_INT sum, out, outsum, hi;
  
  if (!(copy = nrrdNewCopy(rhv))) {
    sprintf(err, "%s: couldn't create copy of histovol", me);
    biffMove(BANE, err, NRRD); return -1;
  }
  hits = copy->data;
  qsort(hits, copy->num, sizeof(int), &_baneClipCompare);
  sum = 0;
  for (hi=0; hi<=copy->num-1; hi++) {
    sum += hits[hi];
  }
  out = sum*parm[0]/100;
  outsum = 0;
  for (hi=0; hi<=copy->num-1; hi++) {
    if (outsum >= out)
      break;
    outsum += hits[hi];
  }
  clip = hits[hi];
  nrrdNuke(copy);
  return clip;
}

int
_baneClipTopN(Nrrd *rhv, double *parm) {
  char me[]="_baneClipTopN", err[128];
  Nrrd *copy;
  int *hits, clip;

  if (!(copy = nrrdNewCopy(rhv))) {
    sprintf(err, "%s: couldn't create copy of histovol", me);
    biffMove(BANE, err, NRRD); return -1;
  }
  hits = copy->data;
  qsort(hits, copy->num, sizeof(int), &_baneClipCompare);
  clip = hits[(int)parm[0]];
  nrrdNuke(copy);
  return clip;
}

baneClipType
baneClip[BANE_CLIP_MAX+1] = {
  _baneClipUnknown,
  _baneClipAbsolute,
  _baneClipPeakRatio,
  _baneClipPercentile,
  _baneClipTopN
};

