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

#include "ten.h"
#include "tenPrivate.h"

#define INFO "Register diffusion-weighted echo-planar images"
char *_tend_epiregInfoL =
  (INFO
   ".  Registration is based on moments of thresholded images. The "
   "images may be optionally blurred, prior to thresholding, but this is "
   "only for the purposes of calculating the transform; the output data "
   "is not blurred. This tool is only useful for distortions described by "
   "a scale, shear, and translate along the phase "
   "encoding direction, which is assumed to be the Y (second) axis of the "
   "image data. The output registered DWIs are resampled with the "
   "chosen kernel, with the seperate DWIs stacked along axis 0.");

int
tend_epiregMain(int argc, char **argv, char *me, hestParm *hparm) {
  int pret;
  hestOpt *hopt = NULL;
  char *perr, *err;
  airArray *mop;
  char *outS;

  NrrdKernelSpec *ksp;
  Nrrd **nin, *nout, *ngrad;
  int ref, ninLen, ccsize[2], noverbose, progress;
  float bw[2], thr[2];
  
  hestOptAdd(&hopt, "i", "b0 dwi1 dwi2", airTypeOther, 3, -1, &nin, NULL,
	     "all the DWIs, as seperate nrrds",
	     &ninLen, NULL, nrrdHestNrrd);
  hestOptAdd(&hopt, "g", "grads", airTypeOther, 1, 1, &ngrad, NULL,
	     "array of gradient directions", NULL, NULL, nrrdHestNrrd);
  hestOptAdd(&hopt, "r", "reference", airTypeInt, 1, 1, &ref, NULL,
	     "which of the DW volumes (one-based numbering) should be used "
	     "as the standard, to which all other images are transformed. ");
  hestOptAdd(&hopt, "nv", NULL, airTypeInt, 0, 0, &noverbose, NULL,
	     "turn OFF verbose mode, "
	     "have no idea what stage processing is at");
  hestOptAdd(&hopt, "p", NULL, airTypeInt, 0, 0, &progress, NULL,
	     "save out intermediate steps of processing");
  hestOptAdd(&hopt, "bw", "x, y blur width", airTypeFloat, 2, 2, bw, "0.0 2.0",
	     "standard devs in X and Y directions of gaussian filter used "
	     "to blur the DWIs prior to doing registration. "
	     "Use 0.0 to say \"no blurring\"");
  hestOptAdd(&hopt, "t", "B0, DWI threshold", airTypeFloat, 2, 2, &thr, NULL,
	     "Threshold values to use on B0 and DW images, respectively, "
	     "to seperate brain and non-brain");
  hestOptAdd(&hopt, "cc", "dark, bright size", airTypeInt, 2, 2, ccsize,
	     "400 20",
	     "when doing connected component (CC) analysis, dark CCs are "
	     "merged with bright surround if their size is less than the "
	     "first number, and bright CCs are merged with dark surround "
	     "if their size is less than the second number.  The former "
	     "includes regions of CSF and highly diffusion-weighted white "
	     "mater; the latter includes noise and scalp.  Or, use \"0 0\" "
	     "to bypass CC analysis.");
  hestOptAdd(&hopt, "k", "kernel", airTypeOther, 1, 1, &ksp, "cubic:0,0.5",
	     "kernel for resampling DWI during registration",
	     NULL, NULL, nrrdHestKernelSpec);
  hestOptAdd(&hopt, "o", "nout", airTypeString, 1, 1, &outS, "-",
	     "single output volume containing all registered DWIs");

  mop = airMopNew();
  airMopAdd(mop, hopt, (airMopper)hestOptFree, airMopAlways);
  USAGE(_tend_epiregInfoL);
  PARSE();
  airMopAdd(mop, hopt, (airMopper)hestParseFree, airMopAlways);

  nout = nrrdNew();
  airMopAdd(mop, nout, (airMopper)nrrdNuke, airMopAlways);
  if (tenEpiRegister(nout, nin, ninLen, ngrad,
		     ref,
		     bw[0], bw[1], thr[0], thr[1], ccsize[0], ccsize[1],
		     ksp->kernel, ksp->parm,
		     progress, !noverbose)) {
    airMopAdd(mop, err = biffGetDone(TEN), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble:\n%s\n", me, err);
    airMopError(mop); exit(1);
  }
  if (nrrdSave(outS, nout, NULL)) {
    airMopAdd(mop, err=biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble writing:\n%s\n", me, err);
    airMopError(mop); return 1;
  }
  
  airMopOkay(mop);
  return 0;
}
/* TEND_CMD(epireg, INFO); */
unrrduCmd tend_epiregCmd = { "epireg", INFO, tend_epiregMain };
