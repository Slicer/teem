/*
  Teem: Tools to process and visualize scientific data and images
  Copyright (C) 2009--2023  University of Chicago
  Copyright (C) 2005--2008  Gordon Kindlmann
  Copyright (C) 1998--2004  University of Utah

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

#include <teem/unrrdu.h>
#include <teem/moss.h>

static const char *ilkInfo
  = ("(I)mage (L)inear Trans(X-->K)forms. Applies linear (homogenous coordinate) "
     "transforms to a given image, using the given kernel for resampling. "
     "Unfortunately the moss library that this tool is built on *currently* knows "
     "nothing about world-space; so this tool only knows about index space. "
     "\n "
     "\n "
     "NOTE: ********* \n "
     "NOTE: ********* \n "
     "NOTE: *** this stand-alone tool is deprecated; use \"unu ilk\" instead!\n "
     "NOTE: ********* \n "
     "NOTE: ********* \n ");

int
main(int argc, const char *argv[]) {
  const char *me;
  char *errS, *outS;
  hestOpt *hopt = NULL;
  hestParm *hparm;
  airArray *mop;
  Nrrd *nin, *nout;
  NrrdKernelSpec *ksp;
  mossSampler *msp;
  double mat[6], **matList, *origInfo, origMat[6], origInvMat[6], ox, oy, min[2], max[2],
    *_bkg, *bkg;
  int debug[2], d, bound, ax0, size[2]; /* HEY size[] should be size_t */
  unsigned int matListLen, _bkgLen, i, avgNum, bkgIdx;
  double scale[4];

  me = argv[0];
  mop = airMopNew();
  hparm = hestParmNew();
  airMopAdd(mop, hparm, (airMopper)hestParmFree, airMopAlways);
  hparm->elideSingleEnumType = AIR_TRUE;
  hparm->elideSingleOtherType = AIR_TRUE;
  hparm->elideSingleOtherDefault = AIR_FALSE;
  hparm->elideMultipleNonExistFloatDefault = AIR_TRUE;
  hparm->respFileEnable = AIR_TRUE;
  hestParmColumnsIoctl(hparm, hestDefaultColumns);

  hestOptAdd_1_Other(&hopt, "i", "image", &nin, "-", "input image", nrrdHestNrrd);
  hestOptAdd_1_Other(&hopt, "0", "origin", &origInfo, "p:0,0",
                     "where to location (0,0) prior to applying transforms.\n "
                     "\b\bo \"u:<float>,<float>\" locate origin in a unit box "
                     "[0,1]x[0,1] which covers the original image\n "
                     "\b\bo \"p:<float>,<float>\" locate origin at a particular "
                     "pixel location, in the index space of the image",
                     mossHestOrigin);
  hestOptAdd_Nv_Other(&hopt, "t", "xform0", 1, -1, &matList, NULL,
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
                      &matListLen, mossHestTransform);
  hestOptAdd_1_Other(&hopt, "k", "kernel", &ksp, "cubic:0,0.5", "reconstruction kernel",
                     nrrdHestKernelSpec);
  hestOptAdd_2_Double(&hopt, "min", "xMin yMin", min, "nan nan",
                      "lower bounding corner of output image. Default (by not "
                      "using this option) is the lower corner of input image. ");
  hestOptAdd_2_Double(&hopt, "max", "xMax yMax", max, "nan nan",
                      "upper bounding corner of output image. Default (by not "
                      "using this option) is the upper corner of input image. ");
  hestOptAdd_1_Enum(&hopt, "b", "boundary", &bound, "bleed",
                    "what to do when sampling outside original image.\n "
                    "\b\bo \"bleed\": copy values at image border outward\n "
                    "\b\bo \"wrap\": do wrap-around on image locations\n "
                    "\b\bo \"pad\": use a given background value (via \"-bg\")",
                    nrrdBoundary);
  bkgIdx
    = hestOptAdd_Nv_Double(&hopt, "bg", "bg0 bg1", 1, -1, &_bkg, "nan",
                           "background color to use with boundary behavior \"pad\". "
                           "Defaults to all zeroes.",
                           &_bkgLen);
  hestOptAdd_2_Other(&hopt, "s", "xSize ySize", scale, "x1 x1",
                     "For each axis, information about how many samples in output:\n "
                     "\b\bo \"x<float>\": number of output samples is some scaling of "
                     " the number input of samples; multiplied by <float>\n "
                     "\b\bo \"<int>\": specify exact number of samples",
                     &unrrduHestScaleCB);
  hestOptAdd_1_UInt(&hopt, "a", "avg #", &avgNum, "0",
                    "number of averages (if there there is only one "
                    "rotation as transform)");
  hestOptAdd_2_Int(&hopt, "db", "x y", debug, "-1 -1",
                   "if both non-negative, turn on verbose debugging for this output "
                   "image pixel");
  hestOptAdd_1_String(&hopt, "o", "filename", &outS, "-",
                      "file to write output nrrd to");
  hestParseOrDie(hopt, argc - 1, argv + 1, hparm, me, ilkInfo, AIR_TRUE, AIR_TRUE,
                 AIR_TRUE);
  airMopAdd(mop, hopt, (airMopper)hestOptFree, airMopAlways);
  airMopAdd(mop, hopt, (airMopper)hestParseFree, airMopAlways);

  fprintf(stderr,
          "NOTE: *********\n"
          "NOTE: *********\n"
          "NOTE: *** this stand-alone tool is deprecated; use \"unu ilk\" instead!\n"
          "NOTE: *********\n"
          "NOTE: *********\n");

  nout = nrrdNew();
  airMopAdd(mop, nout, (airMopper)nrrdNuke, airMopAlways);
  msp = mossSamplerNew();
  airMopAdd(mop, msp, (airMopper)mossSamplerNix, airMopAlways);
  if (mossSamplerKernelSet(msp, ksp)) {
    fprintf(stderr, "%s: trouble with setting kernel:\n%s\n", me,
            errS = biffGetDone(MOSS));
    free(errS);
    airMopError(mop);
    return 1;
  }
  ELL_2V_COPY(msp->verbPixel, debug);
  if (nrrdBoundaryPad == bound) {
    if (_bkgLen != MOSS_CHAN_NUM(nin)) {
      char stmp[AIR_STRLEN_SMALL];
      fprintf(stderr, "%s: got length %u background, image has %s channels\n", me,
              _bkgLen, airSprintSize_t(stmp, MOSS_CHAN_NUM(nin)));
      airMopError(mop);
      return 1;
    } else {
      bkg = _bkg;
    }
  } else {
    if (hestSourceUser == hopt[bkgIdx].source) {
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

  for (d = 0; d < 2; d++) {
    switch (AIR_INT(scale[0 + 2 * d])) {
    case unrrduScaleNothing:
      /* same number of samples as input */
      size[d] = AIR_INT(nin->axis[ax0 + d].size);
      break;
    case unrrduScaleMultiply:
      /* scaling of input # samples */
      size[d] = AIR_ROUNDUP(nin->axis[ax0 + d].size * scale[1 + 2 * d]);
      break;
    case unrrduScaleDivide:
      /* scaling of input # samples */
      size[d] = AIR_ROUNDUP(nin->axis[ax0 + d].size / scale[1 + 2 * d]);
      break;
    case unrrduScaleExact:
      /* explicit # of samples */
      size[d] = AIR_INT(scale[1 + 2 * d]);
      break;
    default:
      /* error */
      fprintf(stderr, "%s: scale[0 + 2*%d] == %d unexpected\n", me, d,
              AIR_INT(scale[0 + 2 * d]));
      airMopError(mop);
      return 1;
    }
  }

  /* find origin-based pre- and post- translate */
  if (0 == origInfo[0]) {
    /* absolute pixel position */
    mossMatTranslateSet(origMat, -origInfo[1], -origInfo[2]);
  } else {
    /* in unit box [0,1]x[0,1] */
    ox = AIR_AFFINE(0.0, origInfo[1], 1.0, nin->axis[ax0 + 0].min,
                    nin->axis[ax0 + 0].max);
    oy = AIR_AFFINE(0.0, origInfo[2], 1.0, nin->axis[ax0 + 1].min,
                    nin->axis[ax0 + 1].max);
    mossMatTranslateSet(origMat, -ox, -oy);
  }
  mossMatInvert(origInvMat, origMat);

  /* form complete transform */
  mossMatIdentitySet(mat);
  mossMatLeftMultiply(mat, origMat);
  for (i = 0; i < matListLen; i++) {
    mossMatLeftMultiply(mat, matList[i]);
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
        fprintf(stderr, "%s: problem doing transform:\n%s\n", me,
                errS = biffGetDone(MOSS));
        free(errS);
        airMopError(mop);
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
      fprintf(stderr, "%s: problem making output:\n%s\n", me, errS = biffGetDone(NRRD));
      free(errS);
      airMopError(mop);
      return 1;
    }
  } else {
    if (mossLinearTransform(nout, nin, bound, bkg, mat, msp, min[0], max[0], min[1],
                            max[1], size[0], size[1])) {
      fprintf(stderr, "%s: problem doing transform:\n%s\n", me,
              errS = biffGetDone(MOSS));
      free(errS);
      airMopError(mop);
      return 1;
    }
  }

  if (nrrdSave(outS, nout, NULL)) {
    fprintf(stderr, "%s: problem saving output:\n%s\n", me, errS = biffGetDone(NRRD));
    free(errS);
    airMopError(mop);
    return 1;
  }

  airMopOkay(mop);
  exit(0);
}
