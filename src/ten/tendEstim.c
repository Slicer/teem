/*
  teem: Gordon Kindlmann's research software
  Copyright (C) 2003, 2002, 2001, 2000, 1999, 1998 University of Utah

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
   "output of \"tend epireg\").  The tensor coefficient weightings associated "
   "with "
   "each of the DWIs, the B matrix, is given as a seperate array, "
   "see \"tend bmat\" usage "
   "info for details.  A \"confidence\" value is computed with the tensor, "
   "based on a soft thresholding of the sum of all the DWIs, according to "
   "the threshold and softness parameters. ");

int
tend_estimMain(int argc, char **argv, char *me, hestParm *hparm) {
  int pret;
  hestOpt *hopt = NULL;
  char *perr, *err, *terrS;
  airArray *mop;

  Nrrd *ndwi, *nbmat, *nterr=NULL, *nout;
  char *outS;
  float thresh, soft, b;

  hestOptAdd(&hopt, "e", "filename", airTypeString, 1, 1, &terrS, "",
	     "Giving a filename here allows you to save out the tensor "
	     "error: a value which measures how much error there is between "
	     "the tensor model and the given diffusion weighted measurements "
	     "for each sample.  By default, no such error calculation is "
	     "saved.");
  hestOptAdd(&hopt, "t", "thresh", airTypeFloat, 1, 1, &thresh, "nan",
	     "value at which to threshold the mean DWI value per pixel "
	     "in order to generate the \"confidence\" mask.  By default, "
	     "the threshold value is calculated automatically, based on "
	     "histogram analysis.");
  hestOptAdd(&hopt, "s", "soft", airTypeFloat, 1, 1, &soft, "0",
	     "how fuzzy the confidence boundary should be.  By default, "
	     "confidence boundary is perfectly sharp");
  hestOptAdd(&hopt, "B", "B matrix", airTypeOther, 1, 1, &nbmat, NULL,
	     "B matrix characterizing how tensor components are weighted "
	     "with each gradient direction.  \"tend bmat\" is one source "
	     "for such a matrix.", NULL, NULL, nrrdHestNrrd);
  hestOptAdd(&hopt, "b", "b", airTypeFloat, 1, 1, &b, "1",
	     "additional b scaling factor ");
  hestOptAdd(&hopt, "i", "dwi", airTypeOther, 1, 1, &ndwi, "-",
	     "4-D volume of all DWIs, stacked along axis 0, starting with "
	     "the B=0 image", NULL, NULL, nrrdHestNrrd);
  hestOptAdd(&hopt, "o", "nout", airTypeString, 1, 1, &outS, "-",
	     "output image (floating point)");

  mop = airMopNew();
  airMopAdd(mop, hopt, (airMopper)hestOptFree, airMopAlways);
  USAGE(_tend_estimInfoL);
  PARSE();
  airMopAdd(mop, hopt, (airMopper)hestParseFree, airMopAlways);

  nout = nrrdNew();
  airMopAdd(mop, nout, (airMopper)nrrdNuke, airMopAlways);

  if (tenEstimate(nout, airStrlen(terrS) ? &nterr : NULL, 
		  ndwi, nbmat, thresh, soft, b)) {
    airMopAdd(mop, err=biffGetDone(TEN), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble making tensor volume:\n%s\n", me, err);
    airMopError(mop); return 1;
  }
  if (nterr) {
    if (nrrdSave(terrS, nterr, NULL)) {
      airMopAdd(mop, err=biffGetDone(NRRD), airFree, airMopAlways);
      fprintf(stderr, "%s: trouble writing:\n%s\n", me, err);
      airMopError(mop); return 1;
    }
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
