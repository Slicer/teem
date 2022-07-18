/*
  Teem: Tools to process and visualize scientific data and images             .
  Copyright (C) 2009--2022  University of Chicago
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

#include "unrrdu.h"
#include "privateUnrrdu.h"
#include <teem/moss.h>

#define INFO "(I)mage (L)inear Trans(X-->K)forms."
static const char *_unrrdu_ilkInfoL
  = (INFO ". Applies linear (homogenous coordinate) transforms to a given *2D* "
          "(possibly multi-channel) image, using the given kernel for resampling. "
          "This started as the \"ilk\" stand-alone tool, but was moved into unu "
          "to simplify getting its functionality to more people more easily. "
          "This is the only unu command that relies on the \"moss\" Teem library. "
          "Unfortunately the moss library *currently* knows nothing about world-space; "
          "so this tool only knows about and computes sampling locations in index space "
          "(and the output image has no meaningful world-space).");

/*
  NOTE: should be "const Nrrd *nin" but hacky code currently sets per-axis min,max as
  needed. double **matList would also benefit from some const-ness, as could min and max.
 */
static int
ilkGo(airArray *mop, Nrrd *nout, Nrrd *nin, const NrrdKernelSpec *ksp, const int *debug,
      int bound, const double *_bkg, unsigned int _bkgLen, int bkgSource,
      double *min, double *max,
      double **matList, unsigned int matListLen, const double *scale,
      const double *origInfo, unsigned int avgNum) {
  static const char me[] = "ilkGo";
  mossSampler *msp;
  const double *bkg;
  double origMat[6], origInvMat[6], mat[6];
  unsigned int ii;
  int ax0, size[2] /* HEY size[] should be size_t, no? */;

  msp = mossSamplerNew();
  airMopAdd(mop, msp, (airMopper)mossSamplerNix, airMopAlways);
  if (mossSamplerKernelSet(msp, ksp)) {
    biffAddf(UNRRDU, "%s: trouble with setting kernel", me);
    return 1;
  }
  msp->verbPixel[0] = debug[0];
  msp->verbPixel[1] = debug[1];
  if (nrrdBoundaryPad == bound) {
    if (_bkgLen != MOSS_CHAN_NUM(nin)) {
      char stmp[AIR_STRLEN_SMALL];
      biffAddf(UNRRDU, "%s: got length %u background, but image has %s channels\n", me,
               _bkgLen, airSprintSize_t(stmp, MOSS_CHAN_NUM(nin)));
      return 1;
    } else {
      bkg = _bkg;
    }
  } else {
    if (hestSourceUser == bkgSource) {
      fprintf(stderr,
              "%s: WARNING: got %u background colors, but with boundary %s, "
              "they will not be used\n",
              me, _bkgLen, airEnumStr(nrrdBoundary, bound));
    }
    bkg = NULL;
  }

  ax0 = MOSS_AXIS0(nin);
  if (!(AIR_EXISTS(nin->axis[ax0 + 0].min) && AIR_EXISTS(nin->axis[ax0 + 0].max))) {
    nrrdAxisInfoMinMaxSet(nin, ax0 + 0, mossDefCenter);
  }
  if (!(AIR_EXISTS(nin->axis[ax0 + 1].min) && AIR_EXISTS(nin->axis[ax0 + 1].max))) {
    nrrdAxisInfoMinMaxSet(nin, ax0 + 1, mossDefCenter);
  }
  min[0] = AIR_EXISTS(min[0]) ? min[0] : nin->axis[ax0 + 0].min;
  max[0] = AIR_EXISTS(max[0]) ? max[0] : nin->axis[ax0 + 0].max;
  min[1] = AIR_EXISTS(min[1]) ? min[1] : nin->axis[ax0 + 1].min;
  max[1] = AIR_EXISTS(max[1]) ? max[1] : nin->axis[ax0 + 1].max;

  for (ii = 0; ii < 2; ii++) {
    switch (AIR_INT(scale[0 + 2 * ii])) {
    case unrrduScaleNothing:
      /* same number of samples as input */
      size[ii] = AIR_INT(nin->axis[ax0 + ii].size);
      break;
    case unrrduScaleMultiply:
      /* scaling of input # samples */
      size[ii] = AIR_ROUNDUP(nin->axis[ax0 + ii].size * scale[1 + 2 * ii]);
      break;
    case unrrduScaleDivide:
      /* scaling of input # samples */
      size[ii] = AIR_ROUNDUP(nin->axis[ax0 + ii].size / scale[1 + 2 * ii]);
      break;
    case unrrduScaleExact:
      /* explicit # of samples */
      size[ii] = AIR_INT(scale[1 + 2 * ii]);
      break;
    default:
      /* error */
      biffAddf(UNRRDU, "%s: scale[0 + 2*%d] == %d not handled\n", me, ii,
               AIR_INT(scale[0 + 2 * ii]));
      return 1;
    }
  }

  /* find origin-based pre- and post- translate */
  if (0 == origInfo[0]) {
    /* absolute pixel position */
    mossMatTranslateSet(origMat, -origInfo[1], -origInfo[2]);
  } else {
    double ox, oy;
    /* in unit box [0,1]x[0,1] */
    ox = AIR_AFFINE(0.0, origInfo[1], 1.0, nin->axis[ax0 + 0].min,
                    nin->axis[ax0 + 0].max);
    oy = AIR_AFFINE(0.0, origInfo[2], 1.0, nin->axis[ax0 + 1].min,
                    nin->axis[ax0 + 1].max);
    mossMatTranslateSet(origMat, -ox, -oy);
  }
  mossMatInvert(origInvMat, origMat);

  mossMatIdentitySet(mat);
  mossMatLeftMultiply(mat, origMat);
  for (ii = 0; ii < matListLen; ii++) {
    mossMatLeftMultiply(mat, matList[ii]);
  }
  mossMatLeftMultiply(mat, origInvMat);

  if (!AIR_EXISTS(nin->axis[ax0 + 0].min) || !AIR_EXISTS(nin->axis[ax0 + 0].max)) {
    nrrdAxisInfoMinMaxSet(nin, ax0 + 0, mossDefCenter);
  }
  if (!AIR_EXISTS(nin->axis[ax0 + 1].min) || !AIR_EXISTS(nin->axis[ax0 + 1].max)) {
    nrrdAxisInfoMinMaxSet(nin, ax0 + 1, mossDefCenter);
  }
  if (avgNum > 1) {
    /* GLK is not sure what the original purpose of this was: if transform is a single
     * rotation this divides that rotation into avgNum steps, and applies and then
     * averages all the sub-rotation increments. This seems like a kind of motion blur,
     * but if that's the case why make it specific to rotation? */
    unsigned int ai;
    double angleMax, angle, mrot[6];
    Nrrd *ntmp, *nacc;
    NrrdIter *itA, *itB;
    int E;

    ntmp = nrrdNew();
    airMopAdd(mop, ntmp, (airMopper)nrrdNuke, airMopAlways);
    nacc = nrrdNew();
    airMopAdd(mop, nacc, (airMopper)nrrdNuke, airMopAlways);
    itA = nrrdIterNew();
    airMopAdd(mop, itA, (airMopper)nrrdIterNix, airMopAlways);
    itB = nrrdIterNew();
    airMopAdd(mop, itB, (airMopper)nrrdIterNix, airMopAlways);
    E = 0;
    angleMax = atan2(mat[3], mat[0]);
    fprintf(stderr, "%s: %u angles ", me, avgNum);
    for (ai = 0; ai < avgNum; ai++) {
      fprintf(stderr, ".");
      fflush(stderr);
      angle = (180 / AIR_PI) * AIR_AFFINE(0, ai, avgNum - 1, angleMax, -angleMax);
      mossMatIdentitySet(mat);
      mossMatLeftMultiply(mat, origMat);
      mossMatRotateSet(mrot, angle);
      mossMatLeftMultiply(mat, mrot);
      mossMatLeftMultiply(mat, origInvMat);
      if (mossLinearTransform(ntmp, nin, bound, bkg, mat, msp, min[0], max[0], min[1],
                              max[1], size[0], size[1])) {
        biffAddf(UNRRDU, "%s: problem doing transform", me);
        return 1;
      }
      if (!ai) {
        if (!E) E |= nrrdConvert(nacc, ntmp, nrrdTypeFloat);
      } else {
        if (!E) E |= nrrdArithBinaryOp(nacc, nrrdBinaryOpAdd, nacc, ntmp);
      }
      if (E) {
        break;
      }
    }
    fprintf(stderr, "\n");
    nrrdIterSetNrrd(itA, nacc);
    nrrdIterSetValue(itB, avgNum);
    if (!E) E |= nrrdArithIterBinaryOp(ntmp, nrrdBinaryOpDivide, itA, itB);
    if (!E)
      E |= nrrdCastClampRound(nout, ntmp, nin->type, AIR_TRUE /* clamp */,
                              0 /* round dir */);
    if (E) {
      biffAddf(UNRRDU, "%s: problem making output", me);
      return 1;
    }
  } else {
    if (mossLinearTransform(nout, nin, bound, bkg, mat, msp, min[0], max[0], min[1],
                            max[1], size[0], size[1])) {
      biffMovef(UNRRDU, MOSS, "%s: problem doing transform", me);
      return 1;
    }
  }

  return 0;
}

int
unrrdu_ilkMain(int argc, const char **argv, const char *me, hestParm *hparm) {
  hestOpt *opt = NULL;
  char *outS, *err;
  int pret;
  airArray *mop;

  Nrrd *nin, *nout;
  double *origInfo, **matList, min[2], max[2], *_bkg;
  unsigned int matListLen, _bkgLen, bkgIdx, avgNum;
  NrrdKernelSpec *ksp;
  int bound, debug[2];
  double scale[4];

  hestOptAdd(&opt, "0", "origin", airTypeOther, 1, 1, &origInfo, "p:0,0",
             "where to location (0,0) prior to applying transforms.\n "
             "\b\bo \"u:<float>,<float>\" locate origin in a unit box "
             "[0,1]x[0,1] which covers the original image\n "
             "\b\bo \"p:<float>,<float>\" locate origin at a particular "
             "pixel location, in the index space of the image",
             NULL, NULL, mossHestOrigin);
  hestOptAdd(&opt, "t", "xform0", airTypeOther, 1, -1, &matList, NULL,
             "transform(s) to apply to image.  Transforms "
             "are applied in the order in which they appear.\n "
             "\b\bo \"identity\": no geometric transform, just resampling\n "
             "\b\bo \"translate:x,y\": shift image by vector (x,y), as "
             "measured in pixels\n "
             "\b\bo \"rotate:ang\": rotate CCW by ang degrees\n "
             "\b\bo \"scale:xs,ys\": scale by xs in X, and ys in Y\n "
             "\b\bo \"shear:fix,amnt\": shear by amnt, keeping fixed "
             "the pixels along a direction <fix> degrees from the X axis\n "
             "\b\bo \"flip:ang\": flip along axis an angle <ang> degrees from "
             "the X axis\n "
             "\b\bo \"a,b,tx,c,d,ty\": specify the transform explicitly "
             "in row-major order (opposite of PostScript) ",
             &matListLen, NULL, mossHestTransform);
  hestOptAdd(&opt, "k", "kernel", airTypeOther, 1, 1, &ksp, "cubic:0,0.5",
             "reconstruction kernel", NULL, NULL, nrrdHestKernelSpec);
  hestOptAdd(&opt, "min", "xMin yMin", airTypeDouble, 2, 2, min, "nan nan",
             "lower bounding corner of output image. Default (by not "
             "using this option) is the lower corner of input image. ");
  hestOptAdd(&opt, "max", "xMax yMax", airTypeDouble, 2, 2, max, "nan nan",
             "upper bounding corner of output image. Default (by not "
             "using this option) is the upper corner of input image. ");
  hestOptAdd(&opt, "b", "boundary", airTypeEnum, 1, 1, &bound, "bleed",
             "what to do when sampling outside original image.\n "
             "\b\bo \"bleed\": copy values at image border outward\n "
             "\b\bo \"wrap\": do wrap-around on image locations\n "
             "\b\bo \"pad\": use a given background value (via \"-bg\")",
             NULL, nrrdBoundary);
  bkgIdx = hestOptAdd(&opt, "bg", "bg0 bg1", airTypeDouble, 1, -1, &_bkg, "nan",
                      "background color to use with boundary behavior \"pad\". "
                      "Defaults to all zeroes.",
                      &_bkgLen);
  hestOptAdd(&opt, "s", "xSize ySize", airTypeOther, 2, 2, scale, "x1 x1",
             "For each axis, information about how many samples in output:\n "
             "\b\bo \"x<float>\": number of output samples is some scaling of "
             " the number input of samples; multiplied by <float>\n "
             "\b\bo \"<int>\": specify exact number of samples",
             NULL, NULL, &unrrduHestScaleCB);
  hestOptAdd(&opt, "a", "avg #", airTypeUInt, 1, 1, &avgNum, "0",
             "number of averages (if there there is only one "
             "rotation as transform)");
  hestOptAdd(&opt, "db", "x y", airTypeInt, 2, 2, debug, "-1 -1",
             "if both non-negative, turn on verbose debugging for this output "
             "image pixel");
  hestOptAdd(&opt, "i", "image", airTypeOther, 1, 1, &nin, "-", "input 2D image", NULL,
             NULL, nrrdHestNrrd);
  OPT_ADD_NOUT(outS, "output image");

  mop = airMopNew();
  airMopAdd(mop, opt, (airMopper)hestOptFree, airMopAlways);

  USAGE(_unrrdu_ilkInfoL);
  PARSE();
  airMopAdd(mop, opt, (airMopper)hestParseFree, airMopAlways);

  nout = nrrdNew();
  airMopAdd(mop, nout, (airMopper)nrrdNuke, airMopAlways);

  if (ilkGo(mop, nout, nin, ksp, debug, bound, _bkg, _bkgLen, opt[bkgIdx].source,
            min, max, matList, matListLen, scale, origInfo, avgNum)) {
    airMopAdd(mop, err = biffGetDone(UNRRDU), airFree, airMopAlways);
    fprintf(stderr, "%s: error:\n%s", me, err);
    airMopError(mop);
    return 1;
  }
  SAVE(outS, nout, NULL);

  airMopOkay(mop);
  return 0;
}

UNRRDU_CMD(ilk, INFO);
