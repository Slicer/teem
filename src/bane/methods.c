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
_baneInitMeasrParm(baneMeasrParm *axp) {
  int i;

  axp->res = -1;
  axp->measr = baneMeasrUnknown;
  axp->inc = baneIncUnknown;
  for (i=0; i<=BANE_INC_NUM_PARM-1; i++) {
    axp->incParm[i] = AIR_NAN;
  }
}

baneHVolParm *
baneHVolParmNew() {
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
      hvp->clipParm[i] = AIR_NAN;
    }
    hvp->incLimit = baneDefIncLimit;
  }
  return hvp;
}

baneHVolParm *
baneHVolParmNix(baneHVolParm *hvp) {
  
  if (hvp) {
    free(hvp);
  }
  return NULL;
}

/*
******** baneHVolParmGKMSInit()
**
** The way Gordon does it.
*/
void
baneHVolParmGKMSInit(baneHVolParm *hvp) {

  if (hvp) {
    hvp->verb = 1;
    hvp->axp[0].res = 256;
    hvp->axp[0].measr = baneMeasrGradMag_cd;
    hvp->axp[0].inc = baneIncPercentile;
    hvp->axp[0].incParm[0] = 1024;
    hvp->axp[0].incParm[1] = 0.15;
    /*
    hvp->axp[0].inc = baneIncRangeRatio;
    hvp->axp[0].incParm[0] = 1.0;
    */

    hvp->axp[1].res = 256;
    hvp->axp[1].measr = baneMeasrHess_cd;
    hvp->axp[1].inc = baneIncPercentile;
    hvp->axp[1].incParm[0] = 1024;
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
