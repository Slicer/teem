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
  airArray *mop;
  Nrrd *nin, *nout;
  NrrdKernelSpec *ksp;
  mossSampler *msp;
  double mat[6], **matList, *origInfo, origMat[6], origInvMat[6], ox, oy;
  int bound, matListLen, i, ax0;
  
  me = argv[0];
  mop = airMopInit();
  
  hestOptAdd(&hopt, "i", "image", airTypeOther, 1, 1, &nin, NULL,
	     "input image", NULL, NULL, nrrdHestNrrd);
  hestOptAdd(&hopt, "0", "origin", airTypeOther, 1, 1, &origInfo, "p:0,0",
	     "where to location (0,0) prior to applying transforms",
	     NULL, NULL, mossHestOrigin);
  hestOptAdd(&hopt, "t", "transform", airTypeOther, 1, -1, &matList,
	     "identity", "transform(s) to apply to image", &matListLen,
	     NULL, mossHestTransform);
  hestOptAdd(&hopt, "k", "kernel", airTypeOther, 1, 1, &ksp,
	     "tent", "reconstruction kernel",
	     NULL, NULL, nrrdHestKernelSpec);
  hestOptAdd(&hopt, "b", "boundary", airTypeEnum, 1, 1, &bound, "bleed",
	     "what to do when sampling outside original image",
	     NULL, nrrdBoundary);
  hestOptAdd(&hopt, "o", "filename", airTypeString, 1, 1, &outS, NULL,
	     "file to write output nrrd to");
  hestParseOrDie(hopt, argc-1, argv+1, NULL,
		 me, ilxInfo, AIR_TRUE, AIR_TRUE, AIR_TRUE);
  airMopAdd(mop, hopt, (airMopper)hestOptFree, airMopAlways);
  airMopAdd(mop, hopt, (airMopper)hestParseFree, airMopAlways);

  nout = nrrdNew();
  airMopAdd(mop, nout, (airMopper)nrrdNuke, airMopAlways);
  msp = mossSamplerNew();
  airMopAdd(mop, msp, (airMopper)mossSamplerNix, airMopAlways);
  if (mossSamplerKernelSet(msp, ksp->kernel, ksp->parm)) {
    fprintf(stderr, "%s: trouble with sampler:\n%s\n",
	    me, errS = biffGetDone(MOSS)); free(errS);
    airMopError(mop); return 1;
  }

  if (0 == origInfo[0]) {
    /* absolute pixel position */
    mossMatTranslateSet(origMat, -origInfo[1], -origInfo[2]);
  } else {
    /* in unit box [0,1]x[0,1] */
    ox = AIR_AFFINE(0.0, origInfo[1], 1.0, 0.0, MOSS_SX(nin)-1);
    oy = AIR_AFFINE(0.0, origInfo[2], 1.0, 0.0, MOSS_SY(nin)-1);
    mossMatTranslateSet(origMat, -ox, -oy);
  }
  mossMatInvert(origInvMat, origMat);

  /* form complete transform */
  mossMatIdentitySet(mat);
  mossMatPostMultiply(mat, origMat);
  for (i=0; i<matListLen; i++)
    mossMatPostMultiply(mat, matList[i]);
  mossMatPostMultiply(mat, origInvMat);

  ax0 = MOSS_AXIS0(nin);
  if (!AIR_EXISTS(nin->axis[ax0+0].min) || !AIR_EXISTS(nin->axis[ax0+0].max)) {
    nrrdAxisMinMaxSet(nin, ax0+0, mossDefCenter);
  }
  if (!AIR_EXISTS(nin->axis[ax0+1].min) || !AIR_EXISTS(nin->axis[ax0+1].max)) {
    nrrdAxisMinMaxSet(nin, ax0+1, mossDefCenter);
  }
  if (mossLinearTransform(nout, nin, mat, msp,
			  0, 0, 0, 0, MOSS_SX(nin), MOSS_SY(nin))) {
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
