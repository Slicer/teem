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

NrrdDeringContext *
nrrdDeringContextNew(void) {
  NrrdDeringContext *drc;

  drc = AIR_CALLOC(1, NrrdDeringContext);
  if (!drc) {
    return NULL;
  }
  drc->verbose = 0;
  drc->linearInterp = AIR_FALSE;
  drc->nin = NULL;
  drc->center[0] = AIR_NAN;
  drc->center[1] = AIR_NAN;
  drc->radiusScale = 1.0;
  drc->thetaNum = 0;
  
  return drc;
}

NrrdDeringContext *
nrrdDeringContextNix(NrrdDeringContext *drc) {

  if (drc) {
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
nrrdDeringLinearInterpSet(NrrdDeringContext *drc, int linterp) {
  static const char me[]="nrrdDeringLinearInterpSet";

  if (!drc) {
    biffAddf(NRRD, "%s: got NULL pointer", me);
    return 1;
  }
  
  drc->linearInterp = linterp;
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

  return 0;
}

int
nrrdDeringRadiusScaleSet(NrrdDeringContext *drc, double rsc) {
  static const char me[]="nrrdDeringRadiusScaleSet";

  if (!drc) {
    biffAddf(NRRD, "%s: got NULL pointer", me);
    return 1;
  }
  if (!( AIR_EXISTS(rsc) && rsc > 0.0 )) {
    biffAddf(NRRD, "%s: need finite positive radius scale, not %g", me, rsc);
    return 1;
  }

  drc->radiusScale = rsc;
  
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

/*
** per-thread state for deringing
*/
typedef struct {
  double radMax;
  size_t radNum;
  airArray *mop;
  Nrrd *nsliceOrig,         /* wrapped slice of nin, sneakily non-const */
    *nslice,                /* slice of nin, converted to double */
    *nptxf,
    *nwght;
  float *slice, *ptxf, *wght;
} deringBag;

deringBag *
deringBagNew(NrrdDeringContext *drc, double radMax) {
  deringBag *dbg;

  dbg = AIR_CALLOC(1, deringBag);
  dbg->radMax = radMax;
  dbg->radNum = AIR_ROUNDUP(drc->radiusScale*radMax);

  dbg->mop = airMopNew();
  dbg->nsliceOrig = nrrdNew();
  airMopAdd(dbg->mop, dbg->nsliceOrig, (airMopper)nrrdNix /* not Nuke! */,
            airMopAlways);
  dbg->nslice = nrrdNew();
  airMopAdd(dbg->mop, dbg->nslice, (airMopper)nrrdNuke, airMopAlways);
  dbg->nptxf = nrrdNew();
  airMopAdd(dbg->mop, dbg->nptxf, (airMopper)nrrdNuke, airMopAlways);
  dbg->nwght = nrrdNew();
  airMopAdd(dbg->mop, dbg->nwght, (airMopper)nrrdNuke, airMopAlways);
  
  return dbg;
}

deringBag *
deringBagNix(deringBag *dbg) {

  airMopOkay(dbg->mop);
  airFree(dbg);
  return NULL;
}

int
deringArraySetup(NrrdDeringContext *drc, deringBag *dbg, unsigned int zi) {
  static const char me[]="deringAlloc";

  /* slice setup */
  if (nrrdWrap_va(dbg->nsliceOrig,
                  /* HEY: sneaky bypass of const-ness of drc->cdata */
                  AIR_CAST(void *, drc->cdata + zi*(drc->sliceSize)),
                  drc->nin->type, 2, 
                  drc->nin->axis[0].size,
                  drc->nin->axis[1].size)
      || (nrrdTypeFloat == drc->nin->type
          ? nrrdCopy(dbg->nslice, dbg->nsliceOrig)
          : nrrdConvert(dbg->nslice, dbg->nsliceOrig, nrrdTypeFloat))) {
    biffAddf(NRRD, "%s: slice setup trouble", me);
    return 1;
  }
  dbg->slice = AIR_CAST(float *, dbg->nslice->data);
  
  /* polar transform setup */
  if (nrrdMaybeAlloc_va(dbg->nptxf, nrrdTypeFloat, 2,
                        dbg->radNum,
                        AIR_CAST(size_t, drc->thetaNum))
      || nrrdMaybeAlloc_va(dbg->nwght, nrrdTypeFloat, 2,
                           dbg->radNum,
                           AIR_CAST(size_t, drc->thetaNum))) {
    biffAddf(NRRD, "%s: polar transform allocation problem", me);
    return 1;
  }
  dbg->ptxf = AIR_CAST(float *, dbg->nptxf->data);
  dbg->wght = AIR_CAST(float *, dbg->nwght->data);
  
  return 0;
}

int
deringPolarTxf(NrrdDeringContext *drc, deringBag *dbg, unsigned int zi) {
  unsigned int sx, sy, xi, yi, rrIdx, thIdx;

  sx = AIR_CAST(unsigned int, drc->nin->axis[0].size);
  sy = AIR_CAST(unsigned int, drc->nin->axis[1].size);
  for (yi=0; yi<sy; yi++) {
    for (xi=0; xi<sx; xi++) {
      double dx, dy, rr, th;

      dx = xi - drc->center[0];
      dy = yi - drc->center[1];
      rr = sqrt(dx*dx + dy*dy);
      th = atan2(-dx, dy);
      if (drc->linearInterp) {
        float rrFrac, thFrac, val;
        unsigned int bidx;
        val = dbg->slice[xi + sx*yi];
        rr *= drc->radiusScale;
        rr = AIR_MAX(0, rr-0.5);
        rrIdx = AIR_CAST(unsigned int, rr);
        rrIdx = AIR_MIN(rrIdx, dbg->radNum-2);
        rrFrac = rr - rrIdx;
        th = (drc->thetaNum)*(1 + th/AIR_PI)/2;
        th = AIR_MAX(0, th-0.5);
        thIdx = AIR_CAST(unsigned int, th);
        thIdx = AIR_MIN(thIdx, drc->thetaNum-2);
        thFrac = th - thIdx;
        bidx = rrIdx + dbg->radNum*thIdx;
        dbg->ptxf[bidx                  ] += (1-rrFrac)*(1-thFrac)*val;
        dbg->ptxf[bidx + 1              ] +=     rrFrac*(1-thFrac)*val;
        dbg->ptxf[bidx     + dbg->radNum] += (1-rrFrac)*thFrac*val;
        dbg->ptxf[bidx + 1 + dbg->radNum] +=     rrFrac*thFrac*val;
        dbg->wght[bidx                  ] += (1-rrFrac)*(1-thFrac);
        dbg->wght[bidx + 1              ] +=     rrFrac*(1-thFrac);
        dbg->wght[bidx     + dbg->radNum] += (1-rrFrac)*thFrac;
        dbg->wght[bidx + 1 + dbg->radNum] +=     rrFrac*thFrac;
      } else {
        rrIdx = airIndexClamp(0, rr, dbg->radMax, dbg->radNum);
        thIdx = airIndexClamp(-AIR_PI, th, AIR_PI, drc->thetaNum);
        dbg->ptxf[rrIdx + dbg->radNum*thIdx] += dbg->slice[xi + sx*yi];
        dbg->wght[rrIdx + dbg->radNum*thIdx] += 1;
      }
    }
  }
  for (thIdx=0; thIdx<drc->thetaNum; thIdx++) {
    for (rrIdx=0; rrIdx<dbg->radNum; rrIdx++) {
      double tmpW;
      tmpW = dbg->wght[rrIdx + dbg->radNum*thIdx];
      if (tmpW) {
        dbg->ptxf[rrIdx + dbg->radNum*thIdx] /= tmpW;
      } else {
        dbg->ptxf[rrIdx + dbg->radNum*thIdx] = AIR_NAN;
      }
    }
  }
  if (0) {
    char fname[AIR_STRLEN_SMALL];
    sprintf(fname, "ptxf-%02u.nrrd", zi);
    nrrdSave(fname, dbg->nptxf, NULL);
  }
  return 0;
}

int
deringDo(NrrdDeringContext *drc, deringBag *dbg,
         Nrrd *nout, unsigned int zi) {
  static const char me[]="deringDo";

  if (deringArraySetup(drc, dbg, zi)
      || deringPolarTxf(drc, dbg, zi)) {
    biffAddf(NRRD, "%s: trouble", me);
    return 1;
  }
  /* filter polar transform */
  /* dering nslice in-place */
  /* convert/copy nslice to output slice */
  AIR_UNUSED(nout);

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
  double dx, dy, radLen, len;
  deringBag *dbg;
  airArray *mop;
  
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
  /* set radLen: radial length of polar transform of data */
  radLen = 0;
  sx = AIR_CAST(unsigned int, drc->nin->axis[0].size);
  sy = AIR_CAST(unsigned int, drc->nin->axis[1].size);
  dx = 0 - drc->center[0];
  dy = 0 - drc->center[1];
  len = sqrt(dx*dx + dy*dy);
  radLen = AIR_MAX(radLen, len);
  dx = sx-1 - drc->center[0];
  dy = 0 - drc->center[1];
  len = sqrt(dx*dx + dy*dy);
  radLen = AIR_MAX(radLen, len);
  dx = sx-1 - drc->center[0];
  dy = sy-1 - drc->center[1];
  len = sqrt(dx*dx + dy*dy);
  radLen = AIR_MAX(radLen, len);
  dx = 0 - drc->center[0];
  dy = sy-1 - drc->center[1];
  len = sqrt(dx*dx + dy*dy);
  radLen = AIR_MAX(radLen, len);
  if (drc->verbose) {
    fprintf(stderr, "%s: radLen = %g\n", me, radLen);
  }

  /* create deringBag(s) */
  mop = airMopNew();
  dbg = deringBagNew(drc, radLen);
  airMopAdd(mop, dbg, (airMopper)deringBagNix, airMopAlways);
  
  sz = (2 == drc->nin->dim
        ? 1
        : AIR_CAST(unsigned int, drc->nin->axis[2].size));
  for (zi=0; zi<sz; zi++) {
    if (drc->verbose) {
      fprintf(stderr, "%s: slice %u of %u ...\n", me, zi, sz);
    }
    if (deringDo(drc, dbg, nout, zi)) {
      biffAddf(NRRD, "%s: trouble on slice %u", me, zi);
      return 1;
    }
    if (drc->verbose) {
      fprintf(stderr, "%s: ... %u done\n", me, zi);
    }
  }

  airMopOkay(mop);
  return 0;
}
