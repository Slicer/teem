/*
  Teem: Tools to process and visualize scientific data and images              
  Copyright (C) 2011, 2010, 2009  University of Chicago
  Copyright (C) 2008, 2007, 2006, 2005  Gordon Kindlmann
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

  You should have received a copy of the GNU Lesser General Public License
  along with this library; if not, write to Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "nrrd.h"
#include "privateNrrd.h"

enum {
  flagUnknown,         /*  0 */
  flagInput,           /*  1 */
  flagCenter,          /*  2 */
  flagLast
};
#define FLAG_MAX           2


NrrdDeringContext *
nrrdDeringContextNew(void) {
  NrrdDeringContext *drc;

  drc = AIR_CALLOC(1, NrrdDeringContext);
  if (!drc) {
    return NULL;
  }
  drc->verbose = 0;
  drc->radLen = AIR_NAN;
  drc->nin = NULL;
  drc->center[0] = AIR_NAN;
  drc->center[1] = AIR_NAN;
  drc->thetaNum = 0;
  drc->flag = AIR_CALLOC(FLAG_MAX+1, int); /* will be set to zero=false */
  if (!(drc->flag)) {
    free(drc);
    return NULL;
  }
  
  return drc;
}

NrrdDeringContext *
nrrdDeringContextNix(NrrdDeringContext *drc) {

  if (drc) {
    airFree(drc->flag);
    free(drc);
  }
  return NULL;
}

int
nrrdDeringVerboseSet(NrrdDeringContext *drc, int verbose) {
  static const char me[]="nrrdDeringVerboseSet";

  if (!drc) {
    biffAddf(NRRD, "%s: got NULL pointer", me);
    return 1;
  }
  
  drc->verbose = verbose;
  return 0;
}

int
nrrdDeringInputSet(NrrdDeringContext *drc, const Nrrd *nin) {
  static const char me[]="nrrdDeringInputSet";
  
  if (!( drc && nin )) {
    biffAddf(NRRD, "%s: got NULL pointer", me);
    return 1;
  }
  if (nrrdCheck(nin)) {
    biffAddf(NRRD, "%s: problems with given nrrd", me);
    return 1;
  }
  if (nrrdTypeBlock == nin->type) {
    biffAddf(NRRD, "%s: can't resample from type %s", me,
             airEnumStr(nrrdType, nrrdTypeBlock));
    return 1;
  }
  if (!( 2 == nin->dim || 3 == nin->dim )) {
    biffAddf(NRRD, "%s: need 2 or 3 dim nrrd (not %u)", me, nin->dim);
    return 1;
  }

  if (drc->verbose > 2) {
    fprintf(stderr, "%s: hi\n", me);
  }
  drc->nin = nin;
  drc->cdata = AIR_CAST(const char *, nin->data);
  drc->sliceSize = (nin->axis[0].size
                    * nin->axis[1].size
                    * nrrdElementSize(nin));
  if (drc->verbose > 2) {
    fprintf(stderr, "%s: sliceSize = %u\n", me,
            AIR_CAST(unsigned int, drc->sliceSize));
  }
  drc->flag[flagInput] = AIR_TRUE;

  return 0;
}

int
nrrdDeringCenterSet(NrrdDeringContext *drc, double cx, double cy) {
  static const char me[]="nrrdDeringCenterSet";

  if (!drc) {
    biffAddf(NRRD, "%s: got NULL pointer", me);
    return 1;
  }
  if (!( AIR_EXISTS(cx) && AIR_EXISTS(cy) )) {
    biffAddf(NRRD, "%s: center (%g,%g) doesn't exist", me, cx, cy);
    return 1;
  }
  
  drc->center[0] = cx;
  drc->center[1] = cy;
  drc->flag[flagCenter] = AIR_TRUE;

  return 0;
}

int
nrrdDeringThetaNumSet(NrrdDeringContext *drc, unsigned int thetaNum) {
  static const char me[]="nrrdDeringThetaNumSet";

  if (!drc) {
    biffAddf(NRRD, "%s: got NULL pointer", me);
    return 1;
  }
  if (!thetaNum) {
    biffAddf(NRRD, "%s: need non-zero thetaNum", me);
    return 1;
  }
  
  drc->thetaNum = thetaNum;

  return 0;
}

static int 
deringDo(NrrdDeringContext *drc, unsigned int zi, Nrrd *nout) {
  static const char me[]="deringDo";
  Nrrd *nsliceOrig,         /* wrapped slice of nin, sneakily non-const */
    *nslice,                /* slice of nin, converted to double */
    *nptxf,
    *nwght;
  airArray *mop;
  unsigned int sx, sy, xi, yi, radNum;
  unsigned int rrIdx, thIdx;
  float *slice=NULL, *ptxf=NULL, *wght=NULL;

  mop = airMopNew();
  nsliceOrig = nrrdNew();
  airMopAdd(mop, nsliceOrig, (airMopper)nrrdNix /* not Nuke */, airMopAlways);
  nslice = nrrdNew();
  airMopAdd(mop, nslice, (airMopper)nrrdNuke, airMopAlways);
  nptxf = nrrdNew();
  airMopAdd(mop, nptxf, (airMopper)nrrdNuke, airMopAlways);
  nwght = nrrdNew();
  airMopAdd(mop, nwght, (airMopper)nrrdNuke, airMopAlways);
  
  /* allocate slices */
  if (nrrdWrap_va(nsliceOrig,
                  /* HEY: sneaky bypass of const-ness of drc->cdata */
                  AIR_CAST(void *, drc->cdata + zi*(drc->sliceSize)),
                  drc->nin->type, 2, 
                  drc->nin->axis[0].size,
                  drc->nin->axis[1].size)
      || (nrrdTypeFloat == drc->nin->type
          ? nrrdCopy(nslice, nsliceOrig)
          : nrrdConvert(nslice, nsliceOrig, nrrdTypeFloat))) {
    biffAddf(NRRD, "%s: slice setup trouble", me);
    airMopError(mop); return 1;
  }

  /* allocate polar transforms */
  radNum = AIR_ROUNDUP(drc->radLen);
  if (nrrdMaybeAlloc_va(nptxf, nrrdTypeFloat, 2,
                        AIR_CAST(size_t, radNum),
                        AIR_CAST(size_t, drc->thetaNum))
      || nrrdMaybeAlloc_va(nwght, nrrdTypeFloat, 2,
                           AIR_CAST(size_t, radNum),
                           AIR_CAST(size_t, drc->thetaNum))) {
    biffAddf(NRRD, "%s: polar transform allocation problem", me);
    airMopError(mop); return 1;
  }
  ptxf = AIR_CAST(float *, nptxf->data);
  wght = AIR_CAST(float *, nwght->data);

  /* do the polar transform */
  sx = AIR_CAST(unsigned int, drc->nin->axis[0].size);
  sy = AIR_CAST(unsigned int, drc->nin->axis[1].size);
  slice = AIR_CAST(float *, nslice->data);
  for (yi=0; yi<sy; yi++) {
    for (xi=0; xi<sx; xi++) {
      double dx, dy, rr, th;

      dx = xi - drc->center[0];
      dy = yi - drc->center[1];
      rr = sqrt(dx*dx + dy*dy);
      th = atan2(dy, dx);
      rrIdx = airIndexClamp(0, rr, drc->radLen, radNum);
      thIdx = airIndexClamp(-AIR_PI, th, AIR_PI, drc->thetaNum);
      /* fixme */
      ptxf[rrIdx + radNum*thIdx] += slice[xi + sx*yi];
      wght[rrIdx + radNum*thIdx] += 1;
    }
  }
  for (thIdx=0; thIdx<drc->thetaNum; thIdx++) {
    for (rrIdx=0; rrIdx<radNum; rrIdx++) {
      ptxf[rrIdx + radNum*thIdx] /= (wght[rrIdx + radNum*thIdx]
                                     ? wght[rrIdx + radNum*thIdx]
                                     : 1);
    }
  }
  if (0) {
    char fname[AIR_STRLEN_SMALL];
    sprintf(fname, "ptxf-%02u.nrrd", zi);
    nrrdSave(fname, nptxf, NULL);
  }

  /* filter polar transform */
  /* dering nslice in-place */
  /* convert/copy nslice to output slice */
  
  airMopOkay(mop);
  return 0;
}

static int
deringCheck(NrrdDeringContext *drc) {
  static const char me[]="deringCheck";

  if (!(drc->nin)) {
    biffAddf(NRRD, "%s: no input set", me);
    return 1;
  }
  if (!( AIR_EXISTS(drc->center[0]) && AIR_EXISTS(drc->center[1]) )) {
    biffAddf(NRRD, "%s: no center set", me);
    return 1;
  }
  if (!(drc->thetaNum)) {
    biffAddf(NRRD, "%s: no thetaNum set", me);
    return 1;
  }
  return 0;
}

int
nrrdDeringExecute(NrrdDeringContext *drc, Nrrd *nout) {
  static const char me[]="nrrdDeringExecute";
  unsigned int sx, sy, sz, zi;
  double dx, dy, len;
  
  if (!( drc && nout )) {
    biffAddf(NRRD, "%s: got NULL pointer", me);
    return 1;
  }
  if (deringCheck(drc)) {
    biffAddf(NRRD, "%s: trouble with setup", me);
    return 1;
  }

  if (nrrdCopy(nout, drc->nin)) {
    biffAddf(NRRD, "%s: trouble initializing output with input", me);
    return 1;
  }

  sx = AIR_CAST(unsigned int, drc->nin->axis[0].size);
  sy = AIR_CAST(unsigned int, drc->nin->axis[1].size);
  
  /* find radial length of polar transform of data */
  drc->radLen = 0;
  dx = 0 - drc->center[0];
  dy = 0 - drc->center[1];
  len = sqrt(dx*dx + dy*dy);
  drc->radLen = AIR_MAX(drc->radLen, len);
  dx = sx-1 - drc->center[0];
  dy = 0 - drc->center[1];
  len = sqrt(dx*dx + dy*dy);
  drc->radLen = AIR_MAX(drc->radLen, len);
  dx = sx-1 - drc->center[0];
  dy = sy-1 - drc->center[1];
  len = sqrt(dx*dx + dy*dy);
  drc->radLen = AIR_MAX(drc->radLen, len);
  dx = 0 - drc->center[0];
  dy = sy-1 - drc->center[1];
  len = sqrt(dx*dx + dy*dy);
  drc->radLen = AIR_MAX(drc->radLen, len);
  if (drc->verbose) {
    fprintf(stderr, "%s: drc->radLen = %g\n", me, drc->radLen);
  }
  
  sz = (2 == drc->nin->dim
        ? 1
        : AIR_CAST(unsigned int, drc->nin->axis[2].size));
  for (zi=0; zi<sz; zi++) {
    if (drc->verbose) {
      fprintf(stderr, "%s: slice %u of %u ...\n", me, zi, sz);
    }
    if (deringDo(drc, zi, nout)) {
      biffAddf(NRRD, "%s: trouble on slice %u", me, zi);
      return 1;
    }
    if (drc->verbose) {
      fprintf(stderr, "%s: ... %u done\n", me, zi);
    }
  }

  return 0;
}
