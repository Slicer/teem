#include "bane.h"

void
_baneInitMeasrParm(baneMeasrParm *axp) {
  int i;

  axp->res = -1;
  axp->measr = baneMeasrUnknown;
  axp->inc = baneIncUnknown;
  for (i=0; i<=BANE_INC_NUM_PARM-1; i++) {
    axp->incParm[i] = airNanf();
  }
}

baneHVolParm *
baneNewHVolParm() {
  baneHVolParm *hvp;
  int i;
  
  hvp = calloc(1, sizeof(baneHVolParm));
  if (hvp) {
    hvp->verb = 1;
    _baneInitMeasrParm(hvp->axp + 0);
    _baneInitMeasrParm(hvp->axp + 1);
    _baneInitMeasrParm(hvp->axp + 2);
    hvp->clip = baneClipUnknown;
    for (i=0; i<=BANE_CLIP_NUM_PARM-1; i++) {
      hvp->clipParm[i] = airNanf();
    }
    hvp->incLimit = BANE_DEF_INCLIMIT;
  }
  return hvp;
}

void
baneNixHVolParm(baneHVolParm *hvp) {
  
  if (hvp) {
    free(hvp);
  }
}

void
baneGKMSInitHVolParm(baneHVolParm *hvp) {

  if (hvp) {
    hvp->verb = 1;
    hvp->axp[0].res = 256;
    hvp->axp[0].measr = baneMeasrGradMag_cd;
    hvp->axp[0].inc = baneIncPercentile;
    hvp->axp[0].incParm[0] = 512;
    hvp->axp[0].incParm[1] = 0.15;
    /*
    hvp->axp[0].inc = baneIncRangeRatio;
    hvp->axp[0].incParm[0] = 1.0;
    */
    hvp->axp[1].res = 256;
    hvp->axp[1].measr = baneMeasrHess_cd;
    hvp->axp[1].inc = baneIncPercentile;
    hvp->axp[1].incParm[0] = 512;
    hvp->axp[1].incParm[1] = 0.15;
    /*
    hvp->axp[1].inc = baneIncRangeRatio;
    hvp->axp[1].incParm[0] = 1.0;
    */
    hvp->axp[2].res = 256;
    hvp->axp[2].measr = baneMeasrVal;
    hvp->axp[2].inc = baneIncRangeRatio;
    hvp->axp[2].incParm[0] = 1.0;
    hvp->clip = baneClipAbsolute;
    hvp->clipParm[0] = 256;
  }
}
