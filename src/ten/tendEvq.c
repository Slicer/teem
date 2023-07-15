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

#define INFO "Quantize directions of diffusion"
static const char *_tend_evqInfoL
  = (INFO
     ". Because VTK doesn't do multi-dimensional colormaps (circa year 2000), we "
     "have to quantize directions of diffusion (usually just the principal eigenvector) "
     "in order to create the usual XYZ<->RGB coloring.  Because eigenvector directions "
     "are poorly defined in regions of low anisotropy, the length of the vector "
     "(pre-quantization) is modulated by anisotropy, requiring the selection of some "
     "anisotropy metric.");

static int
tend_evqMain(int argc, const char **argv, const char *me, hestParm *hparm) {
  int pret;
  hestOpt *hopt = NULL;
  char *perr, *err;
  airArray *mop;

  int aniso, dontScaleByAniso;
  unsigned int which;
  Nrrd *nin, *nout;
  char *outS;

  hestOptAdd_1_UInt(&hopt, "c", "evec index", &which, "0",
                    "Which eigenvector should be quantized: \"0\" for the "
                    "direction of fastest diffusion (eigenvector associated "
                    "with largest eigenvalue), \"1\" or \"2\" for other two "
                    "eigenvectors (associated with middle and smallest eigenvalue)");
  hestOptAdd_1_Enum(&hopt, "a", "aniso", &aniso, NULL,
                    "Scale the eigenvector with this anisotropy metric. " TEN_ANISO_DESC,
                    tenAniso);
  hestOptAdd_Flag(&hopt, "ns", &dontScaleByAniso,
                  "Don't attenuate the color by anisotropy.  By default (not "
                  "using this option), regions with low or no anisotropy are "
                  "very dark colors or black");
  hestOptAdd_1_Other(&hopt, "i", "nin", &nin, "-", "input diffusion tensor volume",
                     nrrdHestNrrd);
  hestOptAdd_1_String(&hopt, "o", "nout", &outS, "-", "output image (floating point)");

  mop = airMopNew();
  airMopAdd(mop, hopt, (airMopper)hestOptFree, airMopAlways);
  USAGE_PARSE(_tend_evqInfoL);
  airMopAdd(mop, hopt, (airMopper)hestParseFree, airMopAlways);

  nout = nrrdNew();
  airMopAdd(mop, nout, (airMopper)nrrdNuke, airMopAlways);
  if (tenEvqVolume(nout, nin, which, aniso, !dontScaleByAniso)) {
    airMopAdd(mop, err = biffGetDone(TEN), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble quantizing eigenvectors:\n%s\n", me, err);
    airMopError(mop);
    return 1;
  }

  if (nrrdSave(outS, nout, NULL)) {
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble writing:\n%s\n", me, err);
    airMopError(mop);
    return 1;
  }

  airMopOkay(mop);
  return 0;
}
TEND_CMD(evq, INFO);
