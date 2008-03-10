/*
  Teem: Tools to process and visualize scientific data and images
  Copyright (C) 2006, 2005  Gordon Kindlmann
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

#include "gage.h"
#include "privateGage.h"

/*
** little helper function to do pre-blurring of a given nrrd 
** of the sort that might be useful for scale-space gage use
**
** nblur[] has to already be allocated for "num" Nrrd*s, 
** AND, they all have to point to valid Nrrds
*/
int
gageStackBlur(Nrrd *const nblur[], unsigned int blnum,
              const Nrrd *nin, unsigned int baseDim,
              const NrrdKernelSpec *_kspec,
              double rangeMin, double rangeMax,
              int boundary, int renormalize, int verbose,
              const char *savePath) {
  char me[]="gageStackBlur", err[BIFF_STRLEN];
  unsigned int blidx, axi;
  NrrdResampleContext *rsmc;
  NrrdKernelSpec *kspec;
  airArray *mop;
  int E;

  if (!(nblur && nin && _kspec)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(GAGE, err); return 1;
  }
  if (!blnum) {
    sprintf(err, "%s: need non-zero num", me);
    biffAdd(GAGE, err); return 1;
  }
  if (3 + baseDim != nin->dim) {
    sprintf(err, "%s: need nin->dim %u (not %u) with baseDim %u", me,
            3 + baseDim, nin->dim, baseDim);
    biffAdd(GAGE, err); return 1;
  }
  if (!( AIR_EXISTS(rangeMin) && AIR_EXISTS(rangeMax) )) {
    sprintf(err, "%s: range min (%g) and max (%g) don't both exist", me,
            rangeMin, rangeMax);
    biffAdd(GAGE, err); return 1;
  }
  if (airEnumValCheck(nrrdBoundary, boundary)) {
    sprintf(err, "%s: %d not a valid %s value", me,
            boundary, nrrdBoundary->name);
    biffAdd(GAGE, err); return 1;
  }
  
  mop = airMopNew();
  kspec = nrrdKernelSpecCopy(_kspec);
  if (!kspec) {
    sprintf(err, "%s: problem copying kernel spec", me);
    biffAdd(GAGE, err); airMopError(mop); return 1;
  }
  airMopAdd(mop, kspec, (airMopper)nrrdKernelSpecNix, airMopAlways);
  /* pre-allocate output Nrrds in case not already there */
  for (blidx=0; blidx<blnum; blidx++) {
    if (!nblur[blidx]) {
      sprintf(err, "%s: got NULL nblur[%u]", me, blidx);
      biffAdd(GAGE, err); airMopError(mop); return 1;
    }
  }
  rsmc = nrrdResampleContextNew();
  airMopAdd(mop, rsmc, (airMopper)nrrdResampleContextNix, airMopAlways);

  E = 0;
  if (!E) E |= nrrdResampleDefaultCenterSet(rsmc, nrrdDefaultCenter);
  if (!E) E |= nrrdResampleNrrdSet(rsmc, nin);
  if (baseDim) {
    unsigned int bai;
    for (bai=0; bai<baseDim; bai++) {
      if (!E) E |= nrrdResampleKernelSet(rsmc, bai, NULL, NULL);
    }
  }
  for (axi=0; axi<3; axi++) {
    if (!E) E |= nrrdResampleSamplesSet(rsmc, baseDim + axi,
                                        nin->axis[baseDim + axi].size);
    if (!E) E |= nrrdResampleRangeFullSet(rsmc, baseDim + axi);
  }
  if (!E) E |= nrrdResampleBoundarySet(rsmc, boundary);
  if (!E) E |= nrrdResampleTypeOutSet(rsmc, nrrdTypeDefault);
  if (!E) E |= nrrdResampleRenormalizeSet(rsmc, renormalize);
  if (E) {
    fprintf(stderr, "%s: trouble setting up resampling\n", me);
    biffAdd(GAGE, err); airMopError(mop); return 1;
  }
  for (blidx=0; blidx<blnum; blidx++) {
    char fileName[AIR_STRLEN_HUGE], tmpName[AIR_STRLEN_SMALL];
    kspec->parm[0] = AIR_AFFINE(0, blidx, blnum-1, rangeMin, rangeMax);
    for (axi=0; axi<3; axi++) {
      if (!E) E |= nrrdResampleKernelSet(rsmc, baseDim + axi,
                                         kspec->kernel, kspec->parm);
    }
    if (verbose) {
      fprintf(stderr, "%s: resampling %u/%u ... ", me, blidx, blnum);
      fflush(stderr);
    }
    if (!E) E |= nrrdResampleExecute(rsmc, nblur[blidx]);
    if (E) {
      if (verbose) {
        fprintf(stderr, "problem!\n");
      }
      sprintf(err, "%s: trouble setting resampling %u of %u", me, blidx, blnum);
      biffAdd(GAGE, err); airMopError(mop); return 1;
    }
    if (verbose) {
      fprintf(stderr, "done.\n");
    }
    if (savePath || verbose > 4) {
      sprintf(tmpName, "blur%02u.nrrd", blidx);
      strcpy(fileName, "");
      if (savePath) {
        strcat(fileName, savePath);
        strcat(fileName, "/");
      }
      strcat(fileName, tmpName);
      if (nrrdSave(fileName, nblur[blidx], NULL)) {
        sprintf(err, "%s: trouble saving blur[%u] to %s", me,
                blidx, tmpName);
        biffAdd(GAGE, err); airMopError(mop); return 1;
      }
    }
  }

  airMopOkay(mop);
  return 0;
}

/*
** this is a little messy: the pvl array is allocated and filled here
** because that's the most idiomatic extension of the way that 
** gagePerVolumeNew() is both the allocator and the initializer. sigh.
*/
int
gageStackPerVolumeNew(gageContext *ctx,
                      gagePerVolume ***pvlP,
                      const Nrrd *const *nblur, unsigned int blnum,
                      const gageKind *kind) {
  char me[]="gageStackPerVolumeNew", err[BIFF_STRLEN];
  gagePerVolume **pvl;
  airArray *mop;
  unsigned int blidx;

  if (!( ctx && pvlP && nblur && kind )) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(GAGE, err); return 1;
  }
  if (!blnum) {
    sprintf(err, "%s: need non-zero num", me);
    biffAdd(GAGE, err); return 1;
  }

  mop = airMopNew();
  pvl = *pvlP = AIR_CAST(gagePerVolume **,
                         calloc(blnum, sizeof(gagePerVolume *)));
  if (!pvl) {
    sprintf(err, "%s: couldn't allocated %u pvl pointers", me, blnum);
    biffAdd(GAGE, err); airMopError(mop); return 1;
  }
  airMopAdd(mop, pvlP, (airMopper)airSetNull, airMopOnError);
  airMopAdd(mop, pvl, (airMopper)airFree, airMopOnError);
  for (blidx=0; blidx<blnum; blidx++) {
    if (!( pvl[blidx] = gagePerVolumeNew(ctx, nblur[blidx], kind) )) {
      sprintf(err, "%s: on pvl %u of %u", me, blidx, blnum);
      biffAdd(GAGE, err); airMopError(mop); return 1;
    }
  }

  airMopOkay(mop);
  return 0;
}

/*
** the "base" pvl is now at the very end of the ctx->pvl, 
** instead of at the beginning
*/
int
gageStackPerVolumeAttach(gageContext *ctx, gagePerVolume *pvlBase,
                         gagePerVolume **pvlStack, unsigned int blnum,
                         double rangeMin, double rangeMax) {
  char me[]="gageStackPerVolumeAttach", err[BIFF_STRLEN];
  unsigned int blidx;

  if (!(ctx && pvlBase && pvlStack)) { 
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(GAGE, err); return 1;
  }
  if (!( blnum > 1 )) {
    sprintf(err, "%s: need more than one sample along stack", me);
    biffAdd(GAGE, err); return 1;
  }
  if (!( AIR_EXISTS(rangeMin) && AIR_EXISTS(rangeMax) )) {
    sprintf(err, "%s: range min (%g) and max (%g) don't both exist", me,
            rangeMin, rangeMax);
    biffAdd(GAGE, err); return 1;
  }
  if (!( 0 <= rangeMin && rangeMin <= rangeMax )) {
    sprintf(err, "%s: need 0 <= range min (%g) <= range max (%g)", me,
            rangeMin, rangeMax);
    biffAdd(GAGE, err); return 1;
  }

  if (gagePerVolumeAttach(ctx, pvlBase)) {
    sprintf(err, "%s: on base pvl", me);
    biffAdd(GAGE, err); return 1;
  }
  for (blidx=0; blidx<blnum; blidx++) {
    if (gagePerVolumeAttach(ctx, pvlStack[blidx])) {
      sprintf(err, "%s: on pvl %u of %u", me, blidx, blnum);
      biffAdd(GAGE, err); return 1;
    }
  }
  
  ctx->stackRange[0] = rangeMin;
  ctx->stackRange[1] = rangeMax;

  return 0;
}

/*
** _gageStackIv3Fill
**
** after the individual iv3's in the stack have been filled, 
** this does the across-stack filtering to fill pvl[0]'s iv3
**
** eventually this will be the place where Hermite spline-based
** reconstruction will be placed.
*/
int
_gageStackIv3Fill(gageContext *ctx) {
  /* char me[]="_gageStackIv3Fill"; */
  unsigned int fd, ii, cacheIdx, cacheLen;
  double wght, val;

  fd = 2*ctx->radius;
  cacheLen = fd*fd*fd*ctx->pvl[0]->kind->valLen;
  /* NOTE we are treating the 4D fd*fd*fd*valLen iv3 as a big 1-D array */
  for (cacheIdx=0; cacheIdx<cacheLen; cacheIdx++) {
    val = 0;
    for (ii=0; ii<ctx->pvlNum-1; ii++) {
      wght = ctx->stackFslw[ii];
      val += (wght
              ? wght*ctx->pvl[1+ii]->iv3[cacheIdx]
              : 0);
    }
    ctx->pvl[0]->iv3[cacheIdx] = val;
  }
  return 0;
}

/*
******** gageStackProbe()
*/
int
gageStackProbe(gageContext *ctx,
               double xi, double yi, double zi, double stackIdx) {
  char me[]="gageStackProbe";

  if (!ctx) {
    return 1;
  }
  if (!ctx->parm.stackUse) {
    sprintf(ctx->errStr, "%s: can't probe stack without parm.stackUse", me);
    ctx->errNum = 1;
    return 1;
  }
  return _gageProbe(ctx, xi, yi, zi, stackIdx);
}

int
gageStackProbeSpace(gageContext *ctx,
                    double xx, double yy, double zz, double ss,
                    int indexSpace, int clamp) {
  char me[]="gageStackProbeSpace";

  if (!ctx) {
    return 1;
  }
  if (!ctx->parm.stackUse) {
    sprintf(ctx->errStr, "%s: can't probe stack without parm.stackUse", me);
    ctx->errNum = 1;
    return 1;
  }
  return _gageProbeSpace(ctx, xx, yy, zz, ss, indexSpace, clamp);;
}
