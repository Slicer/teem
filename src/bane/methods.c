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

baneProbeK3Pack *
baneProbeK3PackNew() {
  baneProbeK3Pack *pack;
  int i;

  pack = (baneProbeK3Pack *)calloc(1, sizeof(baneProbeK3Pack));
  if (pack) {
    pack->k0 = pack->k1 = pack->k2 = nrrdKernelZero;
    pack->param0[0] = pack->param1[0] = pack->param2[0] = 1.0;
    for (i=1; i<=NRRD_MAX_KERNEL_PARAMS-1; i++) {
      pack->param0[i] = pack->param1[i] = pack->param2[i] = 0;
    }
  }
  return pack;
}

baneProbeK3Pack *
baneProbeK3PackNix(baneProbeK3Pack *pack) {

  if (pack) {
    free(pack);
  }
  return NULL;
}
