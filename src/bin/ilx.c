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

#include <moss.h>

char *ilxInfo = ("Applies linear (homogenous coordinate) transforms "
		 "to a given image, using the given kernel for "
		 "resampling. ");

int
main(int argc, char *argv[]) {
  char *me, *errS, *outS;
  hestOpt *hopt=NULL;
  hestParm *hparm;
  airArray *mop;
  Nrrd *nin, *nout;
  NrrdKernelSpec *ksp;
  mossSampler *msp;
  double mat[6], **matList, *origInfo, origMat[6], origInvMat[6], ox, oy,
    min[2], max[2];
  int bound, matListLen, i, ax0, size[2], bgLen;
  float *bg;
  
  me = argv[0];
  mop = airMopInit();
  hparm = hestParmNew();
  airMopAdd(mop, hparm, (airMopper)hestParmFree, airMopAlways);
  hparm->elideSingleEnumType = AIR_TRUE;
  hparm->elideSingleOtherType = AIR_TRUE;
  hparm->elideSingleOtherDefault = AIR_FALSE;
  hparm->elideMultipleNonExistFloatDefault = AIR_TRUE;
  
  hestOptAdd(&hopt, "i", "image", airTypeOther, 1, 1, &nin, NULL,
	     "input image", NULL, NULL, nrrdHestNrrd);
  hestOptAdd(&hopt, "0", "origin", airTypeOther, 1, 1, &origInfo, "p:0,0",
	     "where to location (0,0) prior to applying transforms",
	     NULL, NULL, mossHestOrigin);
  hestOptAdd(&hopt, "t", "xform0", airTypeOther, 1, -1, &matList,
	     "identity", "transform(s) to apply to image", &matListLen,
	     NULL, mossHestTransform);
  hestOptAdd(&hopt, "k", "kernel", airTypeOther, 1, 1, &ksp,
	     "tent", "reconstruction kernel",
	     NULL, NULL, nrrdHestKernelSpec);
  hestOptAdd(&hopt, "min", "xMin yMin", airTypeDouble, 2, 2, min, "nan nan",
	     "lower bounding corner of output image. Default (by not "
	     "using this option) is the lower corner of input image. ");
  hestOptAdd(&hopt, "max", "xMax yMax", airTypeDouble, 2, 2, max, "nan nan",
	     "upper bounding corner of output image. Default (by not "
	     "using this option) is the upper corner of input image. ");
  hestOptAdd(&hopt, "b", "boundary", airTypeEnum, 1, 1, &bound, "bleed",
	     "what to do when sampling outside original image",
	     NULL, nrrdBoundary);
  hestOptAdd(&hopt, "bg", "bg0 bg1", airTypeFloat, 1, -1, &bg, "nan",
	     "background color to use with boundary behavior \"pad\". "
	     "Defaults to all zeroes.",
	     &bgLen);
  hestOptAdd(&hopt, "s", "xSize, ySize", airTypeInt, 2, 2, size, "0 0",
	     "dimensions of output image.  Use 0 to mean \"same size "
	     "as input image\"");
  hestOptAdd(&hopt, "o", "filename", airTypeString, 1, 1, &outS, NULL,
	     "file to write output nrrd to");
  hestParseOrDie(hopt, argc-1, argv+1, hparm,
		 me, ilxInfo, AIR_TRUE, AIR_TRUE, AIR_TRUE);
  airMopAdd(mop, hopt, (airMopper)hestOptFree, airMopAlways);
  airMopAdd(mop, hopt, (airMopper)hestParseFree, airMopAlways);

  nout = nrrdNew();
  airMopAdd(mop, nout, (airMopper)nrrdNuke, airMopAlways);
  msp = mossSamplerNew();
  airMopAdd(mop, msp, (airMopper)mossSamplerNix, airMopAlways);
  msp->boundary = bound;
  if (mossSamplerKernelSet(msp, ksp->kernel, ksp->parm)) {
    fprintf(stderr, "%s: trouble with sampler:\n%s\n",
	    me, errS = biffGetDone(MOSS)); free(errS);
    airMopError(mop); return 1;
  }
  if (AIR_EXISTS(bg[0]) && bgLen != MOSS_NCOL(nin)) {
    fprintf(stderr, "%s: got %d background colors, image has %d colors\n", 
	    me, bgLen, MOSS_NCOL(nin));
    airMopError(mop); return 1;
  }

  ax0 = MOSS_AXIS0(nin);
  if (!( AIR_EXISTS(nin->axis[ax0+0].min)
	 && AIR_EXISTS(nin->axis[ax0+0].max))) {
    nrrdAxisMinMaxSet(nin, ax0+0, mossDefCenter);
  }
  if (!( AIR_EXISTS(nin->axis[ax0+1].min)
	 && AIR_EXISTS(nin->axis[ax0+1].max))) {
    nrrdAxisMinMaxSet(nin, ax0+1, mossDefCenter);
  }
  min[0] = AIR_EXISTS(min[0]) ? min[0] : nin->axis[ax0+0].min;
  max[0] = AIR_EXISTS(max[0]) ? max[0] : nin->axis[ax0+0].max;
  min[1] = AIR_EXISTS(min[1]) ? min[1] : nin->axis[ax0+1].min;
  max[1] = AIR_EXISTS(max[1]) ? max[1] : nin->axis[ax0+1].max;
  size[0] = size[0] > 0 ? size[0] : nin->axis[ax0+0].size;
  size[1] = size[1] > 0 ? size[1] : nin->axis[ax0+1].size;

  /* find origin-based pre- and post- translate */
  if (0 == origInfo[0]) {
    /* absolute pixel position */
    mossMatTranslateSet(origMat, -origInfo[1], -origInfo[2]);
  } else {
    /* in unit box [0,1]x[0,1] */
    ox = AIR_AFFINE(0.0, origInfo[1], 1.0,
		    nin->axis[ax0+0].min, nin->axis[ax0+0].max);
    oy = AIR_AFFINE(0.0, origInfo[2], 1.0, 
		    nin->axis[ax0+1].min, nin->axis[ax0+1].max);
    mossMatTranslateSet(origMat, -ox, -oy);
  }
  mossMatInvert(origInvMat, origMat);

  /* form complete transform */
  mossMatIdentitySet(mat);
  mossMatPostMultiply(mat, origMat);
  for (i=0; i<matListLen; i++)
    mossMatPostMultiply(mat, matList[i]);
  mossMatPostMultiply(mat, origInvMat);

  if (!AIR_EXISTS(nin->axis[ax0+0].min) || !AIR_EXISTS(nin->axis[ax0+0].max)) {
    nrrdAxisMinMaxSet(nin, ax0+0, mossDefCenter);
  }
  if (!AIR_EXISTS(nin->axis[ax0+1].min) || !AIR_EXISTS(nin->axis[ax0+1].max)) {
    nrrdAxisMinMaxSet(nin, ax0+1, mossDefCenter);
  }
  if (mossLinearTransform(nout, nin, AIR_EXISTS(bg[0]) ? bg : NULL,
			  mat, msp,
			  min[0], max[0], min[1], max[1],
			  size[0], size[1])) {
    fprintf(stderr, "%s: problem doing transform:\n%s\n",
	    me, errS = biffGetDone(MOSS)); free(errS);
    airMopError(mop); return 1;
  }

  if (nrrdSave(outS, nout, NULL)) {
    fprintf(stderr, "%s: problem saving output:\n%s\n",
	    me, errS = biffGetDone(NRRD)); free(errS);
    airMopError(mop); return 1;
  }

  airMopOkay(mop);
  exit(0);
}
