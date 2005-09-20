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

enum {
  flagUnknown,        /*  0 */
  flagDefaultCenter,  /*  1 */
  flagNrrd,           /*  2 */
  flagInputDimension, /*  3 */
  flagInputCenters,   /*  4 */
  flagInputSizes,     /*  5 */
  flagKernels,        /*  6 */
  flagSamples,        /*  7 */
  flagRanges,         /*  8 */
  flagBoundary,       /*  9 */
  flagBufferAllocate, /* 10 */
  flagVectorAllocate, /* 11 */
  flagPermutation,    /* 12 */
  flagVectorFill,     /* 13 */
  flagClamp,          /* 14 */
  flagRound,          /* 15 */
  flagTypeOut,        /* 16 */
  flagPadValue,       /* 17 */
  flagRenormalize,    /* 18 */
  flagLast
};
#define FLAG_MAX         18

void
nrrdResampleContextInit(NrrdResampleContext *rsmc) {
  unsigned int axIdx, axJdx, kpIdx, flagIdx;
  NrrdResampleAxis *axis;

  if (rsmc) {
    rsmc->nin = NULL;
    rsmc->boundary = nrrdDefaultResampleBoundary;
    rsmc->typeOut = nrrdDefaultResampleType;
    rsmc->renormalize = nrrdDefaultResampleRenormalize;
    rsmc->round = nrrdDefaultResampleRound;
    rsmc->clamp = nrrdDefaultResampleClamp;
    rsmc->defaultCenter = nrrdDefaultCenter;
    rsmc->padValue = nrrdDefaultResamplePadValue;
    rsmc->dim = 0;
    rsmc->passNum = AIR_CAST(unsigned int, -1);
    rsmc->topRax = AIR_CAST(unsigned int, -1);
    rsmc->botRax = AIR_CAST(unsigned int, -1);
    for (axIdx=0; axIdx<NRRD_DIM_MAX; axIdx++) {
      rsmc->permute[axIdx] = AIR_CAST(unsigned int, -1);
      rsmc->passAxis[axIdx] = AIR_CAST(unsigned int, -1);
    }
    for (axIdx=0; axIdx<NRRD_DIM_MAX+1; axIdx++) {
      axis = rsmc->axis + axIdx;
      axis->kernel = NULL;
      axis->kparm[0] = nrrdDefaultKernelParm0;
      for (kpIdx=1; kpIdx<NRRD_KERNEL_PARMS_NUM; kpIdx++) {
        axis->kparm[kpIdx] = AIR_NAN;
      }
      axis->min = axis->max = AIR_NAN;
      axis->samples = AIR_CAST(unsigned int, -1);
      axis->center = nrrdCenterUnknown;
      axis->sizeIn = AIR_CAST(unsigned int, -1);
      for (axJdx=0; axJdx<NRRD_DIM_MAX; axJdx++) {
        axis->sizePerm[axJdx] = AIR_CAST(size_t, -1);
        axis->axisPerm[axJdx] = AIR_CAST(unsigned int, -1);
      }
      axis->ratio = AIR_NAN;
      axis->ntmp = NULL;
      axis->nbuffer = nrrdNew();
      axis->nindex = nrrdNew();
      axis->nweight = nrrdNew();
    }
    /* initialize flags to all true */
    for (flagIdx=0; flagIdx<=FLAG_MAX; flagIdx++) {
      rsmc->flag[flagIdx] = AIR_TRUE;
    }
    rsmc->time = 0.0;
  }
  return;
}

NrrdResampleContext *
nrrdResampleContextNew() {
  NrrdResampleContext *rsmc;

  rsmc = (NrrdResampleContext *)calloc(1, sizeof(NrrdResampleContext));
  if (rsmc) {
    rsmc->flag = (int*)calloc(1+FLAG_MAX, sizeof(int));
    nrrdResampleContextInit(rsmc);
  }
  return rsmc;
}

NrrdResampleContext *
nrrdResampleContextNix(NrrdResampleContext *rsmc) {
  unsigned int axIdx;

  if (rsmc) {
    for (axIdx=0; axIdx<NRRD_DIM_MAX+1; axIdx++) {
      nrrdNuke(rsmc->axis[axIdx].ntmp);  /* HEY don't nuke input */
      nrrdNuke(rsmc->axis[axIdx].nbuffer);
      nrrdNuke(rsmc->axis[axIdx].nindex);
      nrrdNuke(rsmc->axis[axIdx].nweight);
    }
    airFree(rsmc->flag);
    airFree(rsmc);
  }
  return NULL;
}

int
nrrdResampleDefaultCenterSet(NrrdResampleContext *rsmc,
                             int center) {
  char me[]="nrrdResampleDefaultCenterSet", err[AIR_STRLEN_MED];

  if (!( rsmc )) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(NRRD, err); return 1;
  }
  if (!(nrrdCenterNode == center
        || nrrdCenterCell == center)) {
    sprintf(err, "%s: got invalid center (%d)", me, center);
    biffAdd(NRRD, err); return 1;
  }

  if (center != rsmc->defaultCenter) {
    rsmc->defaultCenter = center;
    rsmc->flag[flagDefaultCenter] = AIR_TRUE;
  }

  return 0;
}

int
nrrdResampleNrrdSet(NrrdResampleContext *rsmc, const Nrrd *nin) {
  char me[]="nrrdResampleNrrdSet", err[AIR_STRLEN_MED];
  unsigned int axIdx, kpIdx;

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

  rsmc->nin = nin;
  rsmc->flag[flagNrrd] = AIR_TRUE;

  /* per-axis information should be invalidated at this point, because
     if we defer the invalidation to later ...Update() calls, it will
     clobber the effects of intervening calls to the likes of 
     ...KernelSet(), ...SampleSet(), and so on */
  if (rsmc->dim != nin->dim) {
    for (axIdx=0; axIdx<NRRD_DIM_MAX; axIdx++) {
      rsmc->axis[axIdx].center = nrrdCenterUnknown;
      rsmc->axis[axIdx].sizeIn = 0;
      rsmc->axis[axIdx].kernel = NULL;
      rsmc->axis[axIdx].kparm[0] = nrrdDefaultKernelParm0;
      for (kpIdx=1; kpIdx<NRRD_KERNEL_PARMS_NUM; kpIdx++) {
        rsmc->axis[axIdx].kparm[kpIdx] = AIR_NAN;
      }
      rsmc->axis[axIdx].samples = 0;
      rsmc->axis[axIdx].min = AIR_NAN;
      rsmc->axis[axIdx].max = AIR_NAN;
    }
  }

  return 0;
}

#define PER_AXIS_ERROR_CHECK \
  if (!rsmc) { \
    sprintf(err, "%s: got NULL pointer", me); \
    biffAdd(NRRD, err); return 1; \
  } \
  if (!rsmc->nin) { \
    sprintf(err, "%s: haven't set input nrrd yet", me); \
    biffAdd(NRRD, err); return 1; \
  } \
  if (!( axIdx < rsmc->nin->dim )) { \
    sprintf(err, "%s: axis %u >= nin->dim %u\n", me, axIdx, rsmc->nin->dim); \
    biffAdd(NRRD, err); return 1; \
  }

int
nrrdResampleKernelSet(NrrdResampleContext *rsmc, unsigned int axIdx, 
                      const NrrdKernel *kernel,
                      double kparm[NRRD_KERNEL_PARMS_NUM]) {
  char me[]="nrrdResampleKernelSet", err[AIR_STRLEN_MED];
  unsigned int kpIdx;
  
  PER_AXIS_ERROR_CHECK;

  rsmc->axis[axIdx].kernel = kernel;
  if (kernel) {
    for (kpIdx=0; kpIdx<kernel->numParm; kpIdx++) {
      rsmc->axis[axIdx].kparm[kpIdx] = kparm[kpIdx];
    }
  }
  rsmc->flag[flagKernels] = AIR_TRUE;

  return 0;
}

int
nrrdResampleSamplesSet(NrrdResampleContext *rsmc,
                       unsigned int axIdx, 
                       size_t samples) {
  char me[]="nrrdResampleSamplesSet", err[AIR_STRLEN_MED];

  PER_AXIS_ERROR_CHECK;

  if (rsmc->axis[axIdx].samples != samples) {
    rsmc->axis[axIdx].samples = samples;
    rsmc->flag[flagSamples] = AIR_TRUE;
  }

  return 0;
}

int
nrrdResampleRangeSet(NrrdResampleContext *rsmc,
                     unsigned int axIdx,
                     double min, double max) {
  char me[]="nrrdResampleRangeSet", err[AIR_STRLEN_MED];

  PER_AXIS_ERROR_CHECK;
  if (!(AIR_EXISTS(min) && AIR_EXISTS(max) && min != max)) {
    sprintf(err, "%s: need min != max and both to exist", me);
    biffAdd(NRRD, err); return 1;
  }
  if (!(rsmc->axis[axIdx].min == min
        && rsmc->axis[axIdx].max == max)) {
    rsmc->axis[axIdx].min = min;
    rsmc->axis[axIdx].max = max;
    rsmc->flag[flagRanges] = AIR_TRUE;
  }

  return 0;
}

int
nrrdResampleRangeFullSet(NrrdResampleContext *rsmc,
                         unsigned int axIdx) {
  char me[]="nrrdResampleRangeFullSet", err[AIR_STRLEN_MED];
  double min, max;
  int center;
  
  PER_AXIS_ERROR_CHECK;

  /* HEY trick is to figure out the axis's centering, and to
     make sure its the same code as used elsewhere */
  center = (rsmc->axis[axIdx].center
            ? rsmc->axis[axIdx].center
            : (rsmc->nin->axis[axIdx].center
               ? rsmc->nin->axis[axIdx].center
               : rsmc->defaultCenter));
  /* same here */
  if (nrrdCenterCell == center) {
    min = -0.5;
    max = rsmc->nin->axis[axIdx].size - 0.5;
  } else {
    min = 0.0;
    max = rsmc->nin->axis[axIdx].size - 1;
  }
  if (!(rsmc->axis[axIdx].min == min
        && rsmc->axis[axIdx].max == max)) {
    rsmc->axis[axIdx].min = min;
    rsmc->axis[axIdx].max = max;
    rsmc->flag[flagRanges] = AIR_TRUE;
  }

  return 0;
}

int
nrrdResampleBoundarySet(NrrdResampleContext *rsmc,
                        int boundary) {
  char me[]="nrrdResampleBoundarySet", err[AIR_STRLEN_MED];

  if (!rsmc) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(NRRD, err); return 1;
  }
  if (airEnumValCheck(nrrdBoundary, boundary)) {
    sprintf(err, "%s: invalid boundary %d", me, boundary);
    biffAdd(NRRD, err); return 1;
  }

  if (rsmc->boundary != boundary) {
    rsmc->boundary = boundary;
    rsmc->flag[flagBoundary] = AIR_TRUE;
  }

  return 0;
}

int
nrrdResamplePadValueSet(NrrdResampleContext *rsmc,
                        double padValue) {
  char me[]="nrrdResamplePadValueSet", err[AIR_STRLEN_MED];

  if (!rsmc) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(NRRD, err); return 1;
  }

  if (rsmc->padValue != padValue) {
    rsmc->padValue = padValue;
    rsmc->flag[flagPadValue] = AIR_TRUE;
  }

  return 0;
}

int
nrrdResampleTypeOutSet(NrrdResampleContext *rsmc,
                       int type) {
  char me[]="nrrdResampleTypeOutSet", err[AIR_STRLEN_MED];

  if (!rsmc) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(NRRD, err); return 1;
  }
  if (nrrdTypeDefault != type && airEnumValCheck(nrrdType, type)) {
    sprintf(err, "%s: invalid type %d", me, type);
    biffAdd(NRRD, err); return 1;
  }
  if (nrrdTypeBlock == type) {
    sprintf(err, "%s: can't output %s type", me,
            airEnumStr(nrrdType, nrrdTypeBlock));
    biffAdd(NRRD, err); return 1;
  }

  if (rsmc->typeOut != type) {
    rsmc->typeOut = type;
    rsmc->flag[flagTypeOut] = AIR_TRUE;
  }
  
  return 0;
}

int
nrrdResampleRenormalizeSet(NrrdResampleContext *rsmc,
                           int renormalize) {
  char me[]="nrrdResampleRenormalizeSet", err[AIR_STRLEN_MED];

  if (!rsmc) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(NRRD, err); return 1;
  }

  if (rsmc->renormalize != renormalize) {
    rsmc->renormalize = renormalize;
    rsmc->flag[flagRenormalize] = AIR_TRUE;
  }
  
  return 0;
}

int
nrrdResampleRoundSet(NrrdResampleContext *rsmc,
                     int round) {
  char me[]="nrrdResampleRoundSet", err[AIR_STRLEN_MED];

  if (!rsmc) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(NRRD, err); return 1;
  }

  if (rsmc->round != round) {
    rsmc->round = round;
    rsmc->flag[flagRound] = AIR_TRUE;
  }

  return 0;
}

int
nrrdResampleClampSet(NrrdResampleContext *rsmc,
                     int clamp) {
  char me[]="nrrdResampleClampSet", err[AIR_STRLEN_MED];

  if (!rsmc) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(NRRD, err); return 1;
  }

  if (rsmc->clamp != clamp) {
    rsmc->clamp = clamp;
    rsmc->flag[flagClamp] = AIR_TRUE;
  }

  return 0;
}

int
_nrrdResampleInputDimensionUpdate(NrrdResampleContext *rsmc) {
  
  if (rsmc->flag[flagNrrd]) {
    if (rsmc->dim != rsmc->nin->dim) {
      rsmc->dim = rsmc->nin->dim;
      rsmc->flag[flagInputDimension] = AIR_TRUE;
    }
  }
  return 0;
}

int
_nrrdResampleInputCentersUpdate(NrrdResampleContext *rsmc) {
  unsigned int axIdx;
  int center;

  if (rsmc->flag[flagDefaultCenter]
      || rsmc->flag[flagInputDimension]
      || rsmc->flag[flagNrrd]) {
    for (axIdx=0; axIdx<NRRD_DIM_MAX; axIdx++) {
      center = (rsmc->axis[axIdx].center
                ? rsmc->axis[axIdx].center
                : (rsmc->nin->axis[axIdx].center
                   ? rsmc->nin->axis[axIdx].center
                   : rsmc->defaultCenter));
      if (rsmc->axis[axIdx].center != center) {
        rsmc->axis[axIdx].center = center;
        rsmc->flag[flagInputCenters] = AIR_TRUE;
      }
    }  
    rsmc->flag[flagDefaultCenter] = AIR_FALSE;
  }

  return 0;
}

int
_nrrdResampleInputSizesUpdate(NrrdResampleContext *rsmc) {
  unsigned int axIdx;

  if (rsmc->flag[flagInputDimension]
      || rsmc->flag[flagNrrd]) {
    for (axIdx=0; axIdx<rsmc->dim; axIdx++) {
      if (rsmc->axis[axIdx].sizeIn != rsmc->nin->axis[axIdx].size) {
        rsmc->axis[axIdx].sizeIn = rsmc->nin->axis[axIdx].size;
        rsmc->flag[flagInputSizes] = AIR_TRUE;
      }
    }
    rsmc->flag[flagInputDimension] = AIR_FALSE;
  }

  return 0;
}

int
_nrrdResampleBufferAllocateUpdate(NrrdResampleContext *rsmc) {
  char me[]="_nrrdResampleBufferAllocateUpdate", err[AIR_STRLEN_MED];
  unsigned int axIdx;
  NrrdResampleAxis *axis;

  if (rsmc->flag[flagInputSizes]
      || rsmc->flag[flagKernels]) {
    for (axIdx=0; axIdx<rsmc->dim; axIdx++) {
      axis = rsmc->axis + axIdx;
      if (axis->kernel) {
        nrrdEmpty(axis->nbuffer);
      } else {
        if (nrrdMaybeAlloc(axis->nbuffer, nrrdResample_nt, 1,
                           1 + axis->sizeIn)) {
          sprintf(err, "%s: couldn't allocate scanline buffer", me);
          biffAdd(NRRD, err); return 1;
        }
      }
    }
    rsmc->flag[flagInputSizes] = AIR_FALSE;
    rsmc->flag[flagBufferAllocate] = AIR_TRUE;
  }
  return 0;
}

int
_nrrdResampleVectorAllocateUpdate(NrrdResampleContext *rsmc) {
  char me[]="_nrrdResampleVectorAllocateUpdate", err[AIR_STRLEN_MED];
  unsigned int axIdx, kpIdx, dotLen, minSamples;
  nrrdResample_t spacingOut, support;
  NrrdResampleAxis *axis;

  if (rsmc->flag[flagKernels]
      || rsmc->flag[flagSamples]
      || rsmc->flag[flagRanges]) {
    for (axIdx=0; axIdx<rsmc->dim; axIdx++) {
      axis = rsmc->axis + axIdx;
      if (!axis->kernel) {
        /* no resampling on this axis */
        continue;
      }
      /* check user-set parameters */
      if (!( AIR_EXISTS(axis->min) && AIR_EXISTS(axis->max) )) {
        sprintf(err, "%s: don't have min, max set on axis %u\n", me, axIdx);
        biffAdd(NRRD, err); return 1;
      }
      for (kpIdx=0; kpIdx<axis->kernel->numParm; kpIdx++) {
        if (!AIR_EXISTS(axis->kparm[kpIdx])) {
          sprintf(err, "%s: didn't set kernel parm %u on axis %u\n",
                  me, kpIdx, axIdx);
          biffAdd(NRRD, err); return 1;
        }
      }
      minSamples = (nrrdCenterCell == axis->center ? 1 : 2);
      if (!( axis->samples >= minSamples )) {
        sprintf(err, "%s: need at last %u samples for %s-centered sampling "
                "along axis %u\n", me, minSamples,
                airEnumStr(nrrdCenter, axis->center), axIdx);
        biffAdd(NRRD, err); return 1;
      }
      /* compute support (spacingIn == 1.0 by definition) */
      spacingOut = ((axis->max - axis->min) / (nrrdCenterCell == axis->center
                                               ? axis->samples
                                               : axis->samples - 1));
      axis->ratio = 1.0/spacingOut;
      support = axis->kernel->support(axis->kparm);
      if (axis->ratio > 1) {
        /* if upsampling, we need only as many samples as needed for
           interpolation with the given kernel */
        dotLen = (int)(2*ceil(support));
      } else {
        /* if downsampling, we need to use all the samples covered by
           the stretched out version of the kernel */
        dotLen = (int)(2*ceil(support/axis->ratio));
      }
      if (nrrdMaybeAlloc(axis->nweight, nrrdResample_nt, 2,
                         dotLen, axis->samples)
          || nrrdMaybeAlloc(axis->nindex, nrrdTypeInt, 2,
                            dotLen, axis->samples)) {
        sprintf(err, "%s: trouble allocating index and weighting vectors", me);
        biffAdd(NRRD, err); return 1;
      }
    }
    rsmc->flag[flagSamples] = AIR_FALSE;
    rsmc->flag[flagRanges] = AIR_FALSE;
    rsmc->flag[flagVectorAllocate] = AIR_TRUE;
  }

  return 0;
}

int
_nrrdResampleVectorFillUpdate(NrrdResampleContext *rsmc) {
  char me[]="_nrrdResampleVectorFillUpdate", err[AIR_STRLEN_MED];
  unsigned int axIdx, dotIdx, dotLen, halfLen, smpIdx, kpIdx;
  int *indexData, tmp, base, rawIdx;
  nrrdResample_t *weightData, idx, integral;
  NrrdResampleAxis *axis;
  double kparm[NRRD_KERNEL_PARMS_NUM];

  if (rsmc->flag[flagRenormalize]
      || rsmc->flag[flagPadValue]
      || rsmc->flag[flagBoundary]
      || rsmc->flag[flagInputCenters]
      || rsmc->flag[flagBufferAllocate]
      || rsmc->flag[flagVectorAllocate]) {
    for (axIdx=0; axIdx<rsmc->dim; axIdx++) {
      axis = rsmc->axis + axIdx;
      if (!axis->kernel) {
        /* no resampling on this axis */
        continue;
      }

      /* calculate sample locations and do first pass on indices */
      indexData = (int *)axis->nindex->data;
      weightData = (nrrdResample_t *)axis->nweight->data;
      dotLen = axis->nweight->axis[0].size;
      halfLen = dotLen/2;
      for (smpIdx=0; smpIdx<axis->samples; smpIdx++) {
        idx = (nrrdCenterCell == axis->center
               ? AIR_AFFINE(-0.5, smpIdx, axis->samples-0.5,
                            axis->min, axis->max)
               : AIR_AFFINE(0.0, smpIdx, axis->samples-1.0,
                            axis->min, axis->max));
        base = (int)floor(idx) - halfLen + 1;
        for (dotIdx=0; dotIdx<dotLen; dotIdx++) {
          tmp = indexData[dotIdx + dotLen*smpIdx] = base + dotIdx;
          weightData[dotIdx + dotLen*smpIdx] = idx - tmp;
        }
        if (rsmc->verbose) {
          if (!smpIdx) {
            fprintf(stderr, "%s: smpIdx=%u -> idx=%g -> base=%d\n", me,
                    smpIdx, idx, base);
            fprintf(stderr, "%s: sample locations:\n", me);
          }
          fprintf(stderr, "%s: %d (sample locations)\n        ", me, smpIdx);
          for (dotIdx=0; dotIdx<dotLen; dotIdx++) {
            fprintf(stderr, "%d/%g ",
                    indexData[dotIdx + dotLen*smpIdx],
                    weightData[dotIdx + dotLen*smpIdx]);
          }
          fprintf(stderr, "\n");
        }
      }

      /* figure out what to do with the out-of-range indices */
      for (dotIdx=0; dotIdx<dotLen*axis->samples; dotIdx++) {
        rawIdx = indexData[dotIdx];
        if (!AIR_IN_CL(0, rawIdx, AIR_CAST(int, axis->sizeIn)-1)) {
          switch(rsmc->boundary) {
          case nrrdBoundaryPad:
          case nrrdBoundaryWeight:  /* this will be further handled later */
            rawIdx = axis->sizeIn;
            break;
          case nrrdBoundaryBleed:
            rawIdx = AIR_CLAMP(0, rawIdx, AIR_CAST(int, axis->sizeIn)-1);
            break;
          case nrrdBoundaryWrap:
            rawIdx = AIR_MOD(rawIdx, AIR_CAST(int, axis->sizeIn));
            break;
          default:
            sprintf(err, "%s: boundary behavior %d unknown/unimplemented", 
                    me, rsmc->boundary);
            biffAdd(NRRD, err); return 0;
          }
          indexData[dotIdx] = rawIdx;
        }
      }
      
      /* run the sample locations through the chosen kernel.  We play a 
         sneaky trick on the kernel parameter 0 in case of downsampling
         to create the blurring of the old index space */
      kparm[0] = (axis->ratio < 1
                  ? axis->kparm[0] / axis->ratio
                  : axis->kparm[0]);
      for (kpIdx=1; kpIdx<NRRD_KERNEL_PARMS_NUM; kpIdx++) {
        kparm[kpIdx] = axis->kparm[kpIdx];
      }
      axis->kernel->EVALN(weightData, weightData, dotLen*axis->samples, kparm);
      
      /* special handling of "cheap" kernel */
      if (nrrdKernelCheap == axis->kernel) {
        for (smpIdx=0; smpIdx<axis->samples; smpIdx++) {
          nrrdResample_t dist, minDist;
          int minIdx, minSet;
          minIdx = indexData[0 + dotLen*smpIdx];
          minDist = weightData[0 + dotLen*smpIdx];
          /* find sample closest to origin */
          for (dotIdx=1; dotIdx<dotLen; dotIdx++) {
            dist = weightData[dotIdx + dotLen*smpIdx];
            if (dist < minDist) {
              minDist = dist;
              minIdx = indexData[dotIdx + dotLen*smpIdx];
            }
          }
          /* set kernel weights to select sample closest to origin */
          minSet = AIR_FALSE;
          for (dotIdx=0; dotIdx<dotLen; dotIdx++) {
            if (minIdx == indexData[dotIdx + dotLen*smpIdx] && !minSet) {
              weightData[dotIdx + dotLen*smpIdx] = 1.0;
              minSet = AIR_TRUE;
            } else {
              weightData[dotIdx + dotLen*smpIdx] = 0.0;
            }
          }
        }
      }

      if (rsmc->verbose) {
        fprintf(stderr, "%s: axis %u sample weights:\n", me, axIdx);
        for (smpIdx=0; smpIdx<axis->samples; smpIdx++) {
          fprintf(stderr, "%s: %d (sample weights)\n        ", me, smpIdx);
          for (dotIdx=0; dotIdx<dotLen; dotIdx++) {
            fprintf(stderr, "%d/%g ", indexData[dotIdx + dotLen*smpIdx],
                    weightData[dotIdx + dotLen*smpIdx]);
          }
          fprintf(stderr, "\n");
        }
      }

      /* final fixes on weighting values */
      integral = axis->kernel->integral(axis->kparm);
      if (nrrdBoundaryWeight == rsmc->boundary) {
        if (integral) {
          /* above, we set to axis->sizeIn all the indices that were out of 
             range.  We now use that to determine the sum of the weights
             for the indices that were in-range */
          for (smpIdx=0; smpIdx<axis->samples; smpIdx++) {
            nrrdResample_t wght = 0;
            for (dotIdx=0; dotIdx<dotLen; dotIdx++) {
              if (AIR_CAST(int, axis->sizeIn) 
                  != indexData[dotIdx + dotLen*smpIdx]) {
                wght += weightData[dotIdx + dotLen*smpIdx];
              }
            }
            for (dotIdx=0; dotIdx<dotLen; dotIdx++) {
              if (AIR_CAST(int, axis->sizeIn)
                  != indexData[dotIdx + dotLen*smpIdx]) {
                weightData[dotIdx + dotLen*smpIdx] *= integral/wght;
              } else {
                weightData[dotIdx + dotLen*smpIdx] = 0;
              }
            }
          }
        }
      } else {
        /* try to remove ripple/grating on downsampling, and errors in
           weighting on upsampling when using kernels that are not
           first-order accurate */
        if (rsmc->renormalize && integral) {
          for (smpIdx=0; smpIdx<axis->samples; smpIdx++) {
            nrrdResample_t wght = 0;
            for (dotIdx=0; dotIdx<dotLen; dotIdx++) {
              wght += weightData[dotIdx + dotLen*smpIdx];
            }
            if (wght) {
              for (dotIdx=0; dotIdx<dotLen; dotIdx++) {
                /* this used to normalize the weights so that they summed
                   to integral ("*= integral/wght"), which meant that if
                   you use a very truncated Gaussian, then your over-all
                   image brightness goes down.  This seems very contrary
                   to the whole point of renormalization. */
                weightData[dotIdx + dotLen*smpIdx] *= 1.0/wght;
              }
            }
          }
        }
      }

      if (rsmc->verbose) {
        fprintf(stderr, "%s: axis %u post-correction sample weights:\n",
                me, axIdx);
        for (smpIdx=0; smpIdx<axis->samples; smpIdx++) {
          fprintf(stderr, "%s: %d (sample weights)\n        ", me, smpIdx);
          for (dotIdx=0; dotIdx<dotLen; dotIdx++) {
            fprintf(stderr, "%d/%g ", indexData[dotIdx + dotLen*smpIdx],
                    weightData[dotIdx + dotLen*smpIdx]);
          }
          fprintf(stderr, "\n");
        }
      }
    }    
    rsmc->flag[flagRenormalize] = AIR_FALSE;
    rsmc->flag[flagPadValue] = AIR_FALSE;
    rsmc->flag[flagBoundary] = AIR_FALSE;
    rsmc->flag[flagInputCenters] = AIR_FALSE;
    rsmc->flag[flagBufferAllocate] = AIR_FALSE;
    rsmc->flag[flagVectorAllocate] = AIR_FALSE;
    rsmc->flag[flagVectorFill] = AIR_TRUE;
  }

  return 0;
}

int
_nrrdResamplePermutationUpdate(NrrdResampleContext *rsmc) {
  char me[]="_nrrdResamplePermutationUpdate";
  unsigned int axIdx, passIdx, currTop, lastTop, fromTop, toTop;
  int bi;

  if (rsmc->flag[flagInputSizes]
      || rsmc->flag[flagKernels]) {

    rsmc->topRax = rsmc->botRax = AIR_CAST(unsigned int, -1);
    for (axIdx=0; axIdx<rsmc->dim; axIdx++) {
      if (rsmc->axis[axIdx].kernel) {
        if (AIR_CAST(unsigned int, -1) == rsmc->topRax) {
          rsmc->topRax = axIdx;
        }
        rsmc->botRax = axIdx;
      }
    }
    if (rsmc->verbose) {
      fprintf(stderr, "%s: topRax = %u; botRax = %u\n", me,
              rsmc->topRax, rsmc->botRax);
    }
    
    /* figure out total number of passes needed, and construct the
       permute[] array.  permute[i] = j means that the axis in position
       i of the old array will be in position j of the new one
       (permute[] answers "where do I put this", not "what got put here").
    */
    rsmc->passNum = 0;
    bi = 0;
    for (axIdx=0; axIdx<rsmc->dim; axIdx++) {
      if (rsmc->axis[axIdx].kernel) {
        do {
          bi = AIR_MOD(bi+1, AIR_CAST(int, rsmc->dim));
        } while (!rsmc->axis[bi].kernel);
        rsmc->permute[bi] = axIdx;
        rsmc->passNum += 1;
      } else {
        rsmc->permute[axIdx] = axIdx;
        bi += bi == AIR_CAST(int, axIdx);
      }
    }
    rsmc->permute[rsmc->dim] = rsmc->dim;  /* HEY: what is this for? */

    toTop = AIR_CAST(unsigned int, -1);
    for (axIdx=0; axIdx<rsmc->dim; axIdx++) {
      /* this will always "break" somewhere */
      if (rsmc->topRax == rsmc->permute[axIdx]) {
        toTop = axIdx;
        break;
      }
    }
    fromTop = rsmc->permute[rsmc->topRax];

    if (rsmc->verbose) {
      fprintf(stderr, "%s: passNum = %u; permute =\n     ",
              me, rsmc->passNum);
      for (axIdx=0; axIdx<rsmc->dim; axIdx++) {
        fprintf(stderr, "%u ", rsmc->permute[axIdx]);
      }
      fprintf(stderr, "\n");
      fprintf(stderr, "%s: toTop = %u; fromTop = %u\n", me, toTop, fromTop);
    }

    if (rsmc->passNum) {
      /* create array of how the axes will be arranged in each pass ("ax"), 
         and create array of how big each axes is in each pass ("sz").
         The input to pass i will have axis layout described in ax[i] and
         axis sizes described in sz[i] */
      passIdx = 0;
      currTop = rsmc->topRax;
      rsmc->passAxis[passIdx] = currTop;
      rsmc->axis[currTop].passIdx = passIdx;
      for (axIdx=0; axIdx<rsmc->dim; axIdx++) {
        rsmc->axis[currTop].axisPerm[axIdx] = axIdx;
        rsmc->axis[currTop].sizePerm[axIdx] = rsmc->axis[axIdx].sizeIn;
      }
      for (passIdx=1; passIdx<rsmc->passNum+1; passIdx++) {
        lastTop = currTop;
        currTop = (passIdx<rsmc->passNum
                   ? rsmc->axis[currTop].axisPerm[toTop]
                   : NRRD_DIM_MAX);
        rsmc->passAxis[passIdx] = currTop;
        rsmc->axis[currTop].passIdx = passIdx;
        for (axIdx=0; axIdx<rsmc->dim; axIdx++) {
          rsmc->axis[currTop].axisPerm[rsmc->permute[axIdx]] 
            = rsmc->axis[lastTop].axisPerm[axIdx];
          rsmc->axis[currTop].sizePerm[rsmc->permute[axIdx]] 
            = rsmc->axis[lastTop].sizePerm[axIdx];
          /* modify the one size corresponding to the resampled axis */
          rsmc->axis[currTop].sizePerm[fromTop] = rsmc->axis[lastTop].samples;
        }
      }
      if (rsmc->verbose) {
        NrrdResampleAxis *axis;
        fprintf(stderr, "%s: axis and size permutations:\n", me);
        for (passIdx=0; passIdx<rsmc->passNum+1; passIdx++) {
          axis = rsmc->axis + rsmc->passAxis[passIdx];
          fprintf(stderr, "----- pass[%u=?=%u] @ %u %s:\n", passIdx,
                  axis->passIdx, rsmc->passAxis[passIdx],
                  (passIdx<rsmc->passNum ? "" : "(output of final pass)"));
          if (!passIdx) {
            fprintf(stderr, "resampling: ");
            for (axIdx=0; axIdx<rsmc->dim; axIdx++) {
              fprintf(stderr, "%s ", rsmc->axis[axIdx].kernel ? " XX" : "   ");
            }
            fprintf(stderr, "\n");
          }
          fprintf(stderr, "      axes: ");
          for (axIdx=0; axIdx<rsmc->dim; axIdx++) {
            fprintf(stderr, "%3u ", axis->axisPerm[axIdx]);
          }
          fprintf(stderr, "\n");
          fprintf(stderr, "     sizes: ");
          for (axIdx=0; axIdx<rsmc->dim; axIdx++) {
            fprintf(stderr, "%3u ",
                    AIR_CAST(unsigned int, axis->sizePerm[axIdx]));
          }
          fprintf(stderr, "\n");
        }
        fprintf(stderr, "\n");
      }
    }

    rsmc->flag[flagInputSizes] = AIR_FALSE;
    rsmc->flag[flagKernels] = AIR_FALSE;
    rsmc->flag[flagPermutation] = AIR_TRUE;
  }

  return 0;
}

int
_nrrdResampleOutputUpdate(NrrdResampleContext *rsmc, Nrrd *nout, char *func) {
  char me[]="_nrrdResampleOutputUpdate", err[AIR_STRLEN_MED];
  unsigned int axIdx;

  if (rsmc->flag[flagClamp]
      || rsmc->flag[flagRound]
      || rsmc->flag[flagTypeOut]
      || rsmc->flag[flagVectorFill]
      || rsmc->flag[flagPermutation]
      || rsmc->flag[flagNrrd]) {

    if (0 == rsmc->passNum) {
    /* actually, no resampling was desired.  Copy input to output,
       but with the clamping that we normally do at the end of resampling */
      size_t size[NRRD_DIM_MAX], numOut, I;
      double val;

      nrrdAxisInfoGet_nva(rsmc->nin, nrrdAxisInfoSize, size);
      if (nrrdMaybeAlloc_nva(nout, rsmc->typeOut, rsmc->nin->dim, size)) {
        sprintf(err, "%s: couldn't allocate output", me);
        biffAdd(NRRD, err); return 1;
      }
      numOut = nrrdElementNumber(nout);
      for (I=0; I<numOut; I++) {
        val = nrrdDLookup[rsmc->nin->type](rsmc->nin->data, I);
        val = nrrdDClamp[rsmc->typeOut](val);
        nrrdDInsert[rsmc->typeOut](nout->data, I, val);
      }
      nrrdAxisInfoCopy(nout, rsmc->nin, NULL, NRRD_AXIS_INFO_NONE);
      /* HEY: need to create textual representation of resampling parameters */
      if (nrrdContentSet(nout, func, rsmc->nin, "")) {
        sprintf(err, "%s:", me);
        biffAdd(NRRD, err); return 1;
      }
      if (nrrdBasicInfoCopy(nout, rsmc->nin,
                            NRRD_BASIC_INFO_DATA_BIT
                            | NRRD_BASIC_INFO_TYPE_BIT
                            | NRRD_BASIC_INFO_BLOCKSIZE_BIT
                            | NRRD_BASIC_INFO_DIMENSION_BIT
                            | NRRD_BASIC_INFO_CONTENT_BIT
                            | NRRD_BASIC_INFO_COMMENTS_BIT
                            | (nrrdStateKeyValuePairsPropagate
                               ? 0
                               : NRRD_BASIC_INFO_KEYVALUEPAIRS_BIT))) {
        sprintf(err, "%s:", me);
        biffAdd(NRRD, err); return 1;
      }
    } else {
      /* there is some real resample required */
      
      

      /* We had to assume some centering when doing resampling; it would
         be rather irresponsible to not record it in the output */
      for (axIdx=0; axIdx<rsmc->dim; axIdx++) {
        if (rsmc->axis[axIdx].kernel) {
          nout->axis[axIdx].center = rsmc->axis[axIdx].center;
        }
      }
    }

    rsmc->flag[flagClamp] = AIR_FALSE;
    rsmc->flag[flagRound] = AIR_FALSE;
    rsmc->flag[flagTypeOut] = AIR_FALSE;
    rsmc->flag[flagVectorFill] = AIR_FALSE;
    rsmc->flag[flagPermutation] = AIR_FALSE;
    rsmc->flag[flagNrrd] = AIR_FALSE;
  }

  return 0;
}

int
_nrrdResampleContextCheck(NrrdResampleContext *rsmc) {
  char me[]="_nrrdResampleContextCheck", err[AIR_STRLEN_MED];

  if (nrrdBoundaryPad == rsmc->boundary && !AIR_EXISTS(rsmc->padValue)) {
    sprintf(err, "%s: asked for boundary padding, but no pad value set\n", me);
    biffAdd(NRRD, err); return 1;
  }

  return 0;
}


int
nrrdResampleExecute(NrrdResampleContext *rsmc, Nrrd *nout) {
  char me[]="nrrdResampleExecute", func[]="resample", err[AIR_STRLEN_MED];
  double time0;

  if (!(rsmc && nout)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(NRRD, err); return 1;
  }

  time0 = airTime();
  if (_nrrdResampleContextCheck(rsmc)
      || _nrrdResampleInputDimensionUpdate(rsmc)
      || _nrrdResampleInputCentersUpdate(rsmc)
      || _nrrdResampleInputSizesUpdate(rsmc)
      || _nrrdResampleBufferAllocateUpdate(rsmc)
      || _nrrdResampleVectorAllocateUpdate(rsmc)
      || _nrrdResampleVectorFillUpdate(rsmc)
      || _nrrdResamplePermutationUpdate(rsmc)
      || _nrrdResampleOutputUpdate(rsmc, nout, func)) {
    sprintf(err, "%s: trouble", me);
    biffAdd(NRRD, err); return 1;
  }
  rsmc->time = airTime() - time0;

  return 0;
}
