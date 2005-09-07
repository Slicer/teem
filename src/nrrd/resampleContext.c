/*
  Teem: Tools to process and visualize scientific data and images
  Copyright (C) 2005  Gordon Kindlmann
  Copyright (C) 2004, 2003, 2002, 2001, 2000, 1999, 1998  University of Utah

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public License
  (LGPL) as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  The terms of redistributing and/or modifying this software also
  include exceptions to the LGPL that facilitate static linking.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "nrrd.h"
#include "privateNrrd.h"

NrrdResampleContext *
nrrdResampleContextNew() {
  NrrdResampleContext *rsmc;
  unsigned int axIdx, kpIdx;

  rsmc = (NrrdResampleContext *)calloc(1, sizeof(NrrdResampleContext));
  if (rsmc) {
    rsmc->nin = NULL;
    for (axIdx=0; axIdx<NRRD_DIM_MAX; axIdx++) {
      rsmc->kernel[axIdx] = NULL;
      rsmc->samples[axIdx] = 0;
      rsmc->parm[axIdx][0] = nrrdDefRsmpScale;
      for (kpIdx=1; kpIdx<NRRD_KERNEL_PARMS_NUM; kpIdx++) {
        rsmc->parm[axIdx][kpIdx] = AIR_NAN;
      }
      rsmc->min[axIdx] = rsmc->max[axIdx] = AIR_NAN;
    }
    rsmc->boundary = nrrdDefRsmpBoundary;
    rsmc->type = nrrdDefRsmpType;
    rsmc->renormalize = nrrdDefRsmpRenormalize;
    rsmc->round = nrrdDefRsmpRound;
    rsmc->clamp = nrrdDefRsmpClamp;
    rsmc->padValue = nrrdDefRsmpPadValue;
  }
  return rsmc;
}

NrrdResampleContext *
nrrdResampleContextNix(NrrdResampleContext *rsmc) {

  if (rsmc) {
    airFree(rsmc);
  }
  return NULL;
}

int
nrrdResampleNrrdSet(NrrdResampleContext *rsmc, const Nrrd *nin) {
  char me[]="nrrdResampleNrrdSet", err[AIR_STRLEN_MED];

  if (!( rsmc && nin )) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(NRRD, err); return 1;
  }
  if (nrrdCheck(nin)) {
    sprintf(err, "%s: problems with given nrrd", me);
    biffAdd(NRRD, err); return 1;
  }
  if (nrrdTypeBlock == nin->type) {
    sprintf(err, "%s: can't resample from type %s", me,
            airEnumStr(nrrdType, nrrdTypeBlock));
    biffAdd(NRRD, err); return 1;
  }

  /* do it */

  return 0;
}

int
nrrdResampleKernelSet(NrrdResampleContext *rsmc, unsigned int axIdx, 
                      const NrrdKernel *kern,
                      double parm[NRRD_KERNEL_PARMS_NUM]) {
  char me[]="nrrdResampleKernelSet", err[AIR_STRLEN_MED];
  
  if (!rsmc) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(NRRD, err); return 1;
  }
  if (!rsmc->nin) {
    sprintf(err, "%s: haven't set input nrrd yet", me);
    biffAdd(NRRD, err); return 1;
  }
  if (!( axIdx < rsmc->nin->dim )) {
    sprintf(err, "%s: axis %u >= nin->dim %u\n", me, axIdx, rsmc->nin->dim);
    biffAdd(NRRD, err); return 1;
  }

  /* do it */
  AIR_UNUSED(kern);
  AIR_UNUSED(parm);

  return 0;
}

int
nrrdResampleSamplesSet(NrrdResampleContext *rsmc,
                       unsigned int axIdx, 
                       size_t samples) {
  /* char me[]="nrrdResampleSamplesSet", err[AIR_STRLEN_MED]; */

  AIR_UNUSED(rsmc);
  AIR_UNUSED(axIdx);
  AIR_UNUSED(samples);

  return 0;
}

int
nrrdResampleRangeSet(NrrdResampleContext *rsmc,
                     unsigned int axIdx,
                     double min, double max) {
  /* char me[]="nrrdResampleRangeSet", err[AIR_STRLEN_MED]; */

  AIR_UNUSED(rsmc);
  AIR_UNUSED(axIdx);
  AIR_UNUSED(min);
  AIR_UNUSED(max);

  return 0;
}

int
nrrdResampleRangeFullSet(NrrdResampleContext *rsmc,
                         unsigned int axIdx) {
  /* char me[]="nrrdResampleRangeFullSet", err[AIR_STRLEN_MED]; */

  AIR_UNUSED(rsmc);
  AIR_UNUSED(axIdx);

  return 0;
}

int
nrrdResampleBoundarySet(NrrdResampleContext *rsmc,
                        int boundary) {
  /* char me[]="nrrdResampleBoundarySet", err[AIR_STRLEN_MED]; */

  AIR_UNUSED(rsmc);
  AIR_UNUSED(boundary);

  return 0;
}

int
nrrdResamplePadValueSet(NrrdResampleContext *rsmc,
                        double padValue) {
  /* char me[]="nrrdResamplePadValueSet", err[AIR_STRLEN_MED]; */

  AIR_UNUSED(rsmc);
  AIR_UNUSED(padValue);

  return 0;
}

int
nrrdResampleTypeOutSet(NrrdResampleContext *rsmc,
                       int type) {
  /* char me[]="nrrdResampleTypeOutSet", err[AIR_STRLEN_MED]; */

  AIR_UNUSED(rsmc);
  AIR_UNUSED(type);

  return 0;
}

int
nrrdResampleRenormalizeSet(NrrdResampleContext *rsmc,
                           int renormalize) {
  /* char me[]="nrrdResampleRenormalizeSet", err[AIR_STRLEN_MED]; */

  AIR_UNUSED(rsmc);
  AIR_UNUSED(renormalize);

  return 0;
}

int
nrrdResampleRoundSet(NrrdResampleContext *rsmc,
                     int round) {
  /* char me[]="nrrdResampleRoundSet", err[AIR_STRLEN_MED]; */

  AIR_UNUSED(rsmc);
  AIR_UNUSED(round);

  return 0;
}

int
nrrdResampleClampSet(NrrdResampleContext *rsmc,
                     int clamp) {
  /* char me[]="nrrdResampleClampSet", err[AIR_STRLEN_MED]; */

  AIR_UNUSED(rsmc);
  AIR_UNUSED(clamp);

  return 0;
}

int
nrrdResampleExecute(NrrdResampleContext *rsmc, Nrrd *nout) {
  char me[]="nrrdResampleExecute", err[AIR_STRLEN_MED];

  if (!(rsmc && nout)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(NRRD, err); return 1;
  }
  return 0;
}
