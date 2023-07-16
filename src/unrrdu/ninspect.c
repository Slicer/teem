/*
  Teem: Tools to process and visualize scientific data and images
  Copyright (C) 2009--2022  University of Chicago
  Copyright (C) 2008, 2007, 2006, 2005  Gordon Kindlmann
  Copyright (C) 2004, 2003, 2002, 2001, 2000, 1999, 1998  University of Utah

  This library is free software; you can redistribute it and/or modify it under the terms
  of the GNU Lesser General Public License (LGPL) as published by the Free Software
  Foundation; either version 2.1 of the License, or (at your option) any later version.
  The terms of redistributing and/or modifying this software also include exceptions to
  the LGPL that facilitate static linking.

  This library is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
  PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License along with
  this library; if not, write to Free Software Foundation, Inc., 51 Franklin Street,
  Fifth Floor, Boston, MA 02110-1301 USA
*/

#include "unrrdu.h"
#include "privateUnrrdu.h"
/* should be okay to include this, since now unu depends on moss depends on ell */
#include <teem/ellMacros.h>

#define INFO "Makes 2D color image to inspect 3D scalar volume"
static const char *_unrrdu_infoL
  = (INFO " without lots of parameter fiddling; useful for making gallery of "
          "large set of volumes. A color image "
          "of three axis-aligned projections is composed of histogram-"
          "equalized and quantized images of the summation (red), "
          "variance (green), and maximum (blue) intensity projections. "
          "If volume is orientation in RAS or LPS space, then a standard "
          "orientation is used for projections and projections are "
          "upsampled (with box kernel) to have isotropic pixels.\n "
          "\n "
          "(The \"ninspect\" name is not especially meaningful, but it is the "
          "name of what used to be a stand-alone Teem command-line utility, "
          "peer to unu).");

static int
fixproj(Nrrd *nproj[3], const Nrrd *nvol) {
  static const char me[] = "fixproj";
  airArray *mop;
  Nrrd *ntmp[3], *nt;
  int ii, jj, map[3], h[3], E, mi;
  size_t rsz[3][3];
  double vec[3][3], dot[3], sp[3], parm[NRRD_KERNEL_PARMS_NUM];
  unsigned int sz[3];

  mop = airMopNew();
  fprintf(stderr, "%s: fixing projections\n", me);
  if (!(ELL_3V_EXISTS(nvol->axis[0].spaceDirection)
        && ELL_3V_EXISTS(nvol->axis[1].spaceDirection)
        && ELL_3V_EXISTS(nvol->axis[2].spaceDirection))) {
    biffAddf(UNRRDU, "%s: space directions don't exist for all 3 axes", me);
    airMopError(mop);
    return 1;
  }

  airMopAdd(mop, nt = nrrdNew(), (airMopper)nrrdNuke, airMopAlways);
  airMopAdd(mop, ntmp[0] = nrrdNew(), (airMopper)nrrdNuke, airMopAlways);
  airMopAdd(mop, ntmp[1] = nrrdNew(), (airMopper)nrrdNuke, airMopAlways);
  airMopAdd(mop, ntmp[2] = nrrdNew(), (airMopper)nrrdNuke, airMopAlways);

  /*                 RL  AP  SI */
  ELL_3V_SET(vec[0], 1, 0, 0);
  ELL_3V_SET(vec[1], 0, 1, 0);
  ELL_3V_SET(vec[2], 0, 0, 1);
  for (ii = 0; ii < 3; ii++) {
    dot[0] = ELL_3V_DOT(vec[ii], nvol->axis[0].spaceDirection);
    dot[1] = ELL_3V_DOT(vec[ii], nvol->axis[1].spaceDirection);
    dot[2] = ELL_3V_DOT(vec[ii], nvol->axis[2].spaceDirection);
    dot[0] = AIR_ABS(dot[0]);
    dot[1] = AIR_ABS(dot[1]);
    dot[2] = AIR_ABS(dot[2]);
    map[ii] = ELL_MAX3_IDX(dot[0], dot[1], dot[2]);
  }
  ELL_3V_SET(h, 1, 0, 0);
  E = 0;
  for (ii = 0; ii < 3; ii++) {
    if (h[map[ii]] != map[h[ii]]) {
      if (!E) E |= nrrdAxesSwap(ntmp[ii], nproj[map[ii]], 1, 2);
    } else {
      if (!E) E |= nrrdCopy(ntmp[ii], nproj[map[ii]]);
    }
  }
  if (E) {
    biffMovef(UNRRDU, NRRD, "%s: trouble with nrrd operations", me);
    airMopError(mop);
    return 1;
  }
  E = 0;
  if (nvol->axis[map[0]].spaceDirection[0] > 0) {
    if (!E) E |= nrrdFlip(nt, ntmp[1], 0 + 1);
    if (!E) E |= nrrdCopy(ntmp[1], nt);
    if (!E) E |= nrrdFlip(nt, ntmp[2], 0 + 1);
    if (!E) E |= nrrdCopy(ntmp[2], nt);
  }
  if (nvol->axis[map[1]].spaceDirection[1] > 0) {
    if (!E) E |= nrrdFlip(nt, ntmp[0], 0 + 1);
    if (!E) E |= nrrdCopy(ntmp[0], nt);
    if (!E) E |= nrrdFlip(nt, ntmp[2], 1 + 1);
    if (!E) E |= nrrdCopy(ntmp[2], nt);
  }
  if (nvol->axis[map[2]].spaceDirection[2] > 0) {
    if (!E) E |= nrrdFlip(nt, ntmp[0], 1 + 1);
    if (!E) E |= nrrdCopy(ntmp[0], nt);
    if (!E) E |= nrrdFlip(nt, ntmp[1], 1 + 1);
    if (!E) E |= nrrdCopy(ntmp[1], nt);
  }
  if (E) {
    biffMovef(UNRRDU, NRRD, "%s: trouble with nrrd operations", me);
    airMopError(mop);
    return 1;
  }

  for (ii = 0; ii < 3; ii++) {
    sz[ii] = AIR_UINT(nvol->axis[map[ii]].size);
    sp[ii] = ELL_3V_LEN(nvol->axis[map[ii]].spaceDirection);
  }
  mi = ELL_MIN3_IDX(sp[0], sp[1], sp[2]);
  sz[0] = AIR_UINT(sz[0] * sp[0] / sp[mi]);
  sz[1] = AIR_UINT(sz[1] * sp[1] / sp[mi]);
  sz[2] = AIR_UINT(sz[2] * sp[2] / sp[mi]);

  parm[0] = 1;
  ELL_3V_SET(rsz[0], 3, sz[1], sz[2]);
  ELL_3V_SET(rsz[1], 3, sz[0], sz[2]);
  ELL_3V_SET(rsz[2], 3, sz[0], sz[1]);
  for (ii = 0; ii < 3; ii++) {
    for (jj = 0; jj < 3; jj++) {
      /* we own these projections, and our use of nrrdSimpleResample is to
         simplify things (like not resample the color axis) that might be done
         more carefully in other settings. onward. */
      ntmp[ii]->axis[jj].center = nrrdCenterCell;
      ntmp[ii]->axis[jj].min = 0;
      ntmp[ii]->axis[jj].max = ntmp[ii]->axis[jj].size;
      /* sanity check: cancel crazy upsampling */
      if (rsz[ii][jj] > 5 * ntmp[ii]->axis[jj].size) {
        rsz[ii][jj] = ntmp[ii]->axis[jj].size;
      }
    }
    printf("%s: resampling proj %d : (%u,%u,%u) -> (%u,%u,%u)\n", me, ii,
           (unsigned int)ntmp[ii]->axis[0].size, (unsigned int)ntmp[ii]->axis[1].size,
           (unsigned int)ntmp[ii]->axis[2].size, (unsigned int)rsz[ii][0],
           (unsigned int)rsz[ii][1], (unsigned int)rsz[ii][2]);
    if (nrrdSimpleResample(nproj[ii], ntmp[ii], nrrdKernelBox, parm, rsz[ii], NULL)) {
      biffMovef(UNRRDU, NRRD, "%s: trouble resampling projection %d", me, ii);
      airMopError(mop);
      return 1;
    }
  }

  airMopOkay(mop);
  return 0;
}

static int
ninspect_proj(Nrrd *nout, const Nrrd *nin, int axis, int smart, float amount) {
  static const char me[] = "ninspect_proj";
  airArray *mop;
  Nrrd *ntmpA, *ntmpB, **nrgb;
  int bins;

  if (!(nout && nin)) {
    biffAddf(UNRRDU, "%s: got NULL pointer", me);
    return 1;
  }
  if (!(AIR_IN_CL(0, axis, 2))) {
    biffAddf(UNRRDU, "%s: given axis %d outside valid range [0,1,2]", me, axis);
    return 1;
  }

  /* allocate a bunch of nrrds to use as basically temp variables */
  mop = airMopNew();
  airMopAdd(mop, ntmpA = nrrdNew(), (airMopper)nrrdNuke, airMopAlways);
  airMopAdd(mop, ntmpB = nrrdNew(), (airMopper)nrrdNuke, airMopAlways);
  /* HEY: this used to be nrgb[3], but its passing to nrrdJoin caused
     "dereferencing type-punned pointer might break strict-aliasing rules"
     warning; GLK not sure how else to fix it */
  nrgb = AIR_CALLOC(3, Nrrd *);
  airMopAdd(mop, nrgb, airFree, airMopAlways);
  airMopAdd(mop, nrgb[0] = nrrdNew(), (airMopper)nrrdNuke, airMopAlways);
  airMopAdd(mop, nrgb[1] = nrrdNew(), (airMopper)nrrdNuke, airMopAlways);
  airMopAdd(mop, nrgb[2] = nrrdNew(), (airMopper)nrrdNuke, airMopAlways);

  /* these arguments to nrrdHistoEq will control its behavior */
  bins = 3000; /* equalization will use a histogram with this many bins */

  /* the following idiom is one way of handling the fact that any
     non-trivial nrrd call can fail, and if it does, then any subsequent
     nrrd calls should be avoided (to be perfectly safe), so that you can
     get the error message from biff.  Because of the left-to-right ordering
     ensured for logical expressions, this will all be called in sequence
     until one of them has a non-zero return.  If he had exception handling,
     we'd put all the nrrd calls in one "try" block.  */
  if (nrrdProject(ntmpA, nin, axis, nrrdMeasureSum, nrrdTypeDefault)
      || nrrdHistoEq(ntmpB, ntmpA, NULL, bins, smart, amount)
      || nrrdQuantize(nrgb[0], ntmpB, NULL, 8)
      || nrrdProject(ntmpA, nin, axis, nrrdMeasureVariance, nrrdTypeDefault)
      || nrrdHistoEq(ntmpB, ntmpA, NULL, bins, smart, amount)
      || nrrdQuantize(nrgb[1], ntmpB, NULL, 8)
      || nrrdProject(ntmpA, nin, axis, nrrdMeasureMax, nrrdTypeDefault)
      || nrrdQuantize(nrgb[2], ntmpA, NULL, 8)
      || nrrdJoin(nout, (const Nrrd *const *)nrgb, 3, 0, AIR_TRUE)) {
    biffMovef(UNRRDU, NRRD, "%s: trouble with nrrd operations", me);
    airMopError(mop);
    return 1;
  }

  airMopOkay(mop);
  return 0;
}

static int
doit(Nrrd *nout, const Nrrd *nin, int smart, float amount, unsigned int margin,
     const unsigned char *back) {
  static const char me[] = "doit";
  Nrrd *nproj[3];
  airArray *mop;
  int E, which;
  unsigned int axis, srl, sap, ssi;
  size_t min[3], ii, nn;
  unsigned char *out;

  if (!(nout && nin && back)) {
    biffAddf(UNRRDU, "%s: got NULL pointer", me);
    return 1;
  }
  if (!(3 == nin->dim)) {
    biffAddf(UNRRDU, "%s: given nrrd has dimension %d, not 3\n", me, nin->dim);
    return 1;
  }

  mop = airMopNew();
  airMopAdd(mop, nproj[0] = nrrdNew(), (airMopper)nrrdNuke, airMopAlways);
  airMopAdd(mop, nproj[1] = nrrdNew(), (airMopper)nrrdNuke, airMopAlways);
  airMopAdd(mop, nproj[2] = nrrdNew(), (airMopper)nrrdNuke, airMopAlways);

  /* do projections for each axis, with some progress indication to sterr */
  for (axis = 0; axis <= 2; axis++) {
    fprintf(stderr, "%s: doing axis %d projections ... ", me, axis);
    fflush(stderr);
    if (ninspect_proj(nproj[axis], nin, axis, smart, amount)) {
      fprintf(stderr, "ERROR\n");
      biffAddf(UNRRDU, "%s: trouble doing projections for axis %d", me, axis);
      airMopError(mop);
      return 1;
    }
    fprintf(stderr, "done\n");
  }

  if ((nrrdSpaceRightAnteriorSuperior == nin->space
       || nrrdSpaceLeftPosteriorSuperior == nin->space)) {
    double ejl[3], thresh = 0.001;
    for (ii = 0; ii < 3; ii++) {
      ejl[ii] = ELL_3V_LEN(nin->axis[ii].spaceDirection);
    }
    if (ejl[0] > thresh && ejl[1] > thresh && ejl[2] > thresh) {
      if (fixproj(nproj, nin)) {
        fprintf(stderr, "ERROR\n");
        biffAddf(UNRRDU,
                 "%s: trouble reorienting/resampling "
                 "projections",
                 me);
        airMopError(mop);
        return 1;
      }
    } else {
      printf("%s not reorienting/resampling projections with edge "
             "lens %g,%g,%g\n",
             me, ejl[0], ejl[1], ejl[2]);
    }
  }
  srl = AIR_UINT(nproj[1]->axis[0 + 1].size);
  sap = AIR_UINT(nproj[0]->axis[0 + 1].size);
  ssi = AIR_UINT(nproj[1]->axis[1 + 1].size);

  /* allocate output as 8-bit color image.  We know output type is
     nrrdTypeUChar because ninspect_proj finishes each projection
     with nrrdQuantize to 8-bits */
  if (nrrdMaybeAlloc_va(nout, nrrdTypeUChar, 3, AIR_SIZE_T(3),
                        AIR_SIZE_T(srl + 3 * margin + sap),
                        AIR_SIZE_T(ssi + 3 * margin + sap))) {
    biffMovef(UNRRDU, NRRD, "%s: couldn't allocate output", me);
    airMopError(mop);
    return 1;
  }

  nn = nout->axis[1].size * nout->axis[2].size;
  out = AIR_CAST(unsigned char *, nout->data);
  for (ii = 0; ii < nn; ii++) {
    ELL_3V_COPY(out + 3 * ii, back);
  }

  min[0] = 0;
  E = 0;
  which = 0;
  if (!E) {
    min[1] = margin;
    min[2] = margin;
    which = 1;
  }
  if (!E) E |= nrrdInset(nout, nout, nproj[1], min);
  if (!E) {
    min[1] = margin;
    min[2] = 2 * margin + ssi;
    which = 2;
  }
  if (!E) E |= nrrdInset(nout, nout, nproj[2], min);
  if (!E) {
    min[1] = 2 * margin + srl;
    min[2] = margin;
    which = 3;
  }
  if (!E) E |= nrrdInset(nout, nout, nproj[0], min);
  if (E) {
    biffAddf(UNRRDU, NRRD, "%s: couldn't composite output (which = %d)", me, which);
    airMopError(mop);
    return 1;
  }

  airMopOkay(mop);
  return 0;
}

static int
unrrdu_ninspectMain(int argc, const char **argv, const char *me, hestParm *hparm) {
  int pret;
  hestOpt *opt = NULL;
  airArray *mop;
  char *outS, *err;
  Nrrd *nin, *nout;
  NrrdIoState *nio;
  unsigned int margin;
  float heqamount;
  unsigned int back[3];
  unsigned char backUC[3];

  mop = airMopNew();

  hestOptAdd_1_Other(&opt, "i", "nin", &nin, "-",
                     "input nrrd to project.  Must be three dimensional.", nrrdHestNrrd);
  hestOptAdd_1_Float(&opt, "amt", "heq", &heqamount, "0.5",
                     "how much to apply histogram equalization to projection images");
  hestOptAdd_1_UInt(
    &opt, "m", "margin", &margin, "6",
    "pixel size of margin on boundary, and space between the projections");
  hestOptAdd_3_UInt(&opt, "b", "background", back, "0 0 0",
                    "background color (8-bit RGB)");
  hestOptAdd_1_String(&opt, "o", "img out", &outS, NULL,
                      "output image to save to.  Will try to use whatever "
                      "format is implied by extension, but will fall back to PPM.");
  USAGE_OR_PARSE(_unrrdu_infoL);
  airMopAdd(mop, opt, (airMopper)hestParseFree, airMopAlways);

  nout = nrrdNew();
  airMopAdd(mop, nout, (airMopper)nrrdNuke, airMopAlways);
  nio = nrrdIoStateNew();
  airMopAdd(mop, nio, (airMopper)nrrdIoStateNix, airMopAlways);

  nrrdStateDisableContent = AIR_TRUE;

  ELL_3V_COPY_TT(backUC, unsigned char, back);
  if (doit(nout, nin, 1, heqamount, margin, backUC)) {
    err = biffGetDone(UNRRDU);
    airMopAdd(mop, err, airFree, airMopAlways);
    fprintf(stderr, "%s: trouble creating output:\n%s", me, err);
    airMopError(mop);
    return 1;
  }

  if (nrrdFormatPNG->nameLooksLike(outS) && !nrrdFormatPNG->available()) {
    fprintf(stderr, "(%s: using PPM format for output)\n", me);
    nio->format = nrrdFormatPNM;
  }
  if (nrrdSave(outS, nout, nio)) {
    err = biffGetDone(NRRD);
    airMopAdd(mop, err, airFree, airMopAlways);
    fprintf(stderr, "%s: trouble saving output image \"%s\":\n%s", me, outS, err);
    airMopError(mop);
    return 1;
  }

  airMopOkay(mop);
  return 0;
}

UNRRDU_CMD(ninspect, INFO);
