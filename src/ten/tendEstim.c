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

#define INFO "Estimate tensors from a set of DW images"
char *_tend_estimInfoL =
  (INFO
   ".  The various DWI volumes must be stacked along axis 0 (as with the "
   "output of \"tend epireg\").  The gradient directions associated with "
   "each of the DWIs are given as a seperate array, see \"tend emat\" usage "
   "info for details.  A \"confidence\" value is computed with the tensor, "
   "based on a soft thresholding of the sum of all the DWIs, according to "
   "the threshold and softness parameters. ");

int
tend_estimMain(int argc, char **argv, char *me, hestParm *hparm) {
  int pret;
  hestOpt *hopt = NULL;
  char *perr, *err;
  airArray *mop;

  Nrrd *ndwi, *nemat, *ngrad, *nout;
  char *outS;
  float thresh, soft, b;

  hestOptAdd(&hopt, "t", "thresh", airTypeFloat, 1, 1, &thresh, NULL,
	     "confidence threshold");
  hestOptAdd(&hopt, "s", "soft", airTypeFloat, 1, 1, &soft, "0",
	     "how fuzzy confidence boundary should be.  By default, "
	     "confidence boundary is perfectly sharp");
  hestOptAdd(&hopt, "b", "b", airTypeFloat, 1, 1, &b, "1",
	     "b value from scan");
  hestOptAdd(&hopt, "i", "dwi", airTypeOther, 1, 1, &ndwi, "-",
	     "4-D volume of all DWIs, stacked along axis 0, starting with "
	     "the B=0 image", NULL, NULL, nrrdHestNrrd);
  hestOptAdd(&hopt, "g", "grads", airTypeOther, 1, 1, &ngrad, NULL,
	     "array of gradient directions", NULL, NULL, nrrdHestNrrd);
  hestOptAdd(&hopt, "o", "nout", airTypeString, 1, 1, &outS, "-",
	     "output image (floating point)");

  mop = airMopNew();
  airMopAdd(mop, hopt, (airMopper)hestOptFree, airMopAlways);
  USAGE(_tend_estimInfoL);
  PARSE();
  airMopAdd(mop, hopt, (airMopper)hestParseFree, airMopAlways);

  nout = nrrdNew();
  airMopAdd(mop, nout, (airMopper)nrrdNuke, airMopAlways);
  nemat = nrrdNew();
  airMopAdd(mop, nemat, (airMopper)nrrdNuke, airMopAlways);

  if (tenEstimationMatrix(nemat, ngrad)
      || tenEstimate(nout, ndwi, nemat, thresh, soft, b)) {
    airMopAdd(mop, err=biffGetDone(TEN), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble making tensor volume:\n%s\n", me, err);
    airMopError(mop); return 1;
  }

  if (nrrdSave(outS, nout, NULL)) {
    airMopAdd(mop, err=biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble writing:\n%s\n", me, err);
    airMopError(mop); return 1;
  }

  airMopOkay(mop);
  return 0;
}
/* TEND_CMD(estim, INFO); */
unrrduCmd tend_estimCmd = { "estim", INFO, tend_estimMain };
