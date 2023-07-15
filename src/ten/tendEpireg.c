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

#include "ten.h"
#include "privateTen.h"

#define INFO "Register diffusion-weighted echo-planar images"
static const char *_tend_epiregInfoL
  = (INFO ". This registration corrects the shear, scale, and translate along "
          "the phase encoding direction (assumed to be the Y (second) axis of "
          "the image) caused by eddy currents from the diffusion-encoding "
          "gradients with echo-planar imaging.  The method is based on calculating "
          "moments of segmented images, where the segmentation is a simple "
          "procedure based on blurring (optional), thresholding and "
          "connected component analysis. "
          "The registered DWIs are resampled with the "
          "chosen kernel, with the separate DWIs stacked along axis 0.");

static int
tend_epiregMain(int argc, const char **argv, const char *me, hestParm *hparm) {
  int pret, rret;
  hestOpt *hopt = NULL;
  char *perr, *err;
  airArray *mop;
  char *outS, *buff;

  char *gradS;
  NrrdKernelSpec *ksp;
  Nrrd **nin, **nout3D, *nout4D, *ngrad, *ngradKVP, *nbmatKVP;
  unsigned int ni, ninLen, *skip, skipNum, baseNum;
  int ref, noverbose, progress, nocc;
  float bw[2], thr, fitFrac;
  double bvalue;

  hestOptAdd_Nv_Other(&hopt, "i", "dwi0 dwi1", 1, -1, &nin, NULL,
                      "all the diffusion-weighted images (DWIs), as separate 3D nrrds, "
                      "**OR**: one 4D nrrd of all DWIs stacked along axis 0",
                      &ninLen, nrrdHestNrrd);
  hestOptAdd_1_String(&hopt, "g", "grads", &gradS, NULL,
                      "array of gradient directions, in the same order as the "
                      "associated DWIs were given to \"-i\", "
                      "**OR** \"-g kvp\" signifies that gradient directions should "
                      "be read from the key/value pairs of the DWI");
  hestOptAdd_1_Int(&hopt, "r", "reference", &ref, "-1",
                   "which of the DW volumes (zero-based numbering) should be used "
                   "as the standard, to which all other images are transformed. "
                   "Using -1 (the default) means that 9 intrinsic parameters "
                   "governing the relationship between the gradient direction "
                   "and the resulting distortion are estimated and fitted, "
                   "ensuring good registration with the non-diffusion-weighted "
                   "T2 image (which is never explicitly used in registration). "
                   "Otherwise, by picking a specific DWI, no distortion parameter "
                   "estimation is done. ");
  hestOptAdd_Flag(&hopt, "nv", &noverbose,
                  "turn OFF verbose mode, and "
                  "have no idea what stage processing is at.");
  hestOptAdd_Flag(&hopt, "p", &progress, "save out intermediate steps of processing");
  hestOptAdd_2_Float(&hopt, "bw", "x,y blur", bw, "1.0 2.0",
                     "standard devs in X and Y directions of gaussian filter used "
                     "to blur the DWIs prior to doing segmentation. This blurring "
                     "does not effect the final resampling of registered DWIs. "
                     "Use \"0.0 0.0\" to say \"no blurring\"");
  hestOptAdd_1_Float(&hopt, "t", "DWI thresh", &thr, "nan",
                     "Threshold value to use on DWIs, "
                     "to do initial separation of brain and non-brain.  By default, "
                     "the threshold is determined automatically by histogram "
                     "analysis.");
  hestOptAdd_Flag(&hopt, "ncc", &nocc,
                  "do *NOT* do connected component (CC) analysis, after "
                  "thresholding and before moment calculation.  Doing CC analysis "
                  "usually gives better results because it converts the "
                  "thresholding output into something much closer to a "
                  "real segmentation");
  hestOptAdd_1_Float(&hopt, "f", "fit frac", &fitFrac, "0.70",
                     "(only meaningful with \"-r -1\") When doing linear fitting "
                     "of the intrinsic distortion parameters, it is good "
                     "to ignore the slices for which the segmentation was poor.  A "
                     "heuristic is used to rank the slices according to segmentation "
                     "quality.  This option controls how many of the (best) slices "
                     "contribute to the fitting.  Use \"0\" to disable distortion "
                     "parameter fitting. ");
  hestOptAdd_1_Other(&hopt, "k", "kernel", &ksp, "cubic:0,0.5",
                     "kernel for resampling DWIs along the phase-encoding "
                     "direction during final registration stage",
                     nrrdHestKernelSpec);
  hestOptAdd_1_UInt(&hopt, "s", "start #", &baseNum, "1",
                    "first number to use in numbered sequence of output files.");
  hestOptAdd_1_String(&hopt, "o", "output/prefix", &outS, "-",
                      "For separate 3D DWI volume inputs: prefix for output filenames; "
                      "will save out one (registered) "
                      "DWI for each input DWI, using the same type as the input. "
                      "**OR**: For single 4D DWI input: output file name. ");

  mop = airMopNew();
  airMopAdd(mop, hopt, (airMopper)hestOptFree, airMopAlways);
  USAGE_JUSTPARSE(_tend_epiregInfoL);
  airMopAdd(mop, hopt, (airMopper)hestParseFree, airMopAlways);

  if (strcmp("kvp", gradS)) {
    /* they're NOT coming from key/value pairs */
    if (nrrdLoad(ngrad = nrrdNew(), gradS, NULL)) {
      airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
      fprintf(stderr, "%s: trouble loading gradient list:\n%s\n", me, err);
      airMopError(mop);
      return 1;
    }
  } else {
    if (1 != ninLen) {
      fprintf(stderr, "%s: can do key/value pairs only from single nrrd", me);
      airMopError(mop);
      return 1;
    }
    /* they are coming from key/value pairs */
    if (tenDWMRIKeyValueParse(&ngradKVP, &nbmatKVP, &bvalue, &skip, &skipNum, nin[0])) {
      airMopAdd(mop, err = biffGetDone(TEN), airFree, airMopAlways);
      fprintf(stderr, "%s: trouble parsing gradient list:\n%s\n", me, err);
      airMopError(mop);
      return 1;
    }
    if (nbmatKVP) {
      fprintf(stderr, "%s: sorry, can only use gradients, not b-matrices", me);
      airMopError(mop);
      return 1;
    }
    ngrad = ngradKVP;
  }
  airMopAdd(mop, ngrad, (airMopper)nrrdNuke, airMopAlways);

  nout3D = AIR_CALLOC(ninLen, Nrrd *);
  airMopAdd(mop, nout3D, airFree, airMopAlways);
  nout4D = nrrdNew();
  airMopAdd(mop, nout4D, (airMopper)nrrdNuke, airMopAlways);
  buff = AIR_CALLOC(airStrlen(outS) + 10, char);
  airMopAdd(mop, buff, airFree, airMopAlways);
  if (!(nout3D && nout4D && buff)) {
    fprintf(stderr, "%s: couldn't allocate buffers", me);
    airMopError(mop);
    return 1;
  }
  for (ni = 0; ni < ninLen; ni++) {
    nout3D[ni] = nrrdNew();
    airMopAdd(mop, nout3D[ni], (airMopper)nrrdNuke, airMopAlways);
  }
  if (1 == ninLen) {
    rret = tenEpiRegister4D(nout4D, nin[0], ngrad, ref, bw[0], bw[1], fitFrac, thr,
                            !nocc, ksp->kernel, ksp->parm, progress, !noverbose);
  } else {
    rret = tenEpiRegister3D(nout3D, nin, ninLen, ngrad, ref, bw[0], bw[1], fitFrac, thr,
                            !nocc, ksp->kernel, ksp->parm, progress, !noverbose);
  }
  if (rret) {
    airMopAdd(mop, err = biffGetDone(TEN), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble doing epireg:\n%s\n", me, err);
    airMopError(mop);
    return 1;
  }

  if (1 == ninLen) {
    if (nrrdSave(outS, nout4D, NULL)) {
      airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
      fprintf(stderr, "%s: trouble writing \"%s\":\n%s\n", me, outS, err);
      airMopError(mop);
      return 1;
    }
  } else {
    for (ni = 0; ni < ninLen; ni++) {
      if (ninLen + baseNum > 99) {
        sprintf(buff, "%s%05u.nrrd", outS, ni + baseNum);
      } else if (ninLen + baseNum > 9) {
        sprintf(buff, "%s%02u.nrrd", outS, ni + baseNum);
      } else {
        sprintf(buff, "%s%u.nrrd", outS, ni + baseNum);
      }
      if (nrrdSave(buff, nout3D[ni], NULL)) {
        airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
        fprintf(stderr, "%s: trouble writing \"%s\":\n%s\n", me, buff, err);
        airMopError(mop);
        return 1;
      }
    }
  }

  airMopOkay(mop);
  return 0;
}
TEND_CMD(epireg, INFO);
