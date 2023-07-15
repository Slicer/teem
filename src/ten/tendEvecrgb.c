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

#define INFO "Make an RGB volume from an eigenvector and an anisotropy"
static const char *_tend_evecrgbInfoL = (INFO ". ");

static int
tend_evecrgbMain(int argc, const char **argv, const char *me, hestParm *hparm) {
  int pret;
  hestOpt *hopt = NULL;
  char *perr, *err;
  airArray *mop;

  tenEvecRGBParm *rgbp;
  Nrrd *nin, *nout;
  char *outS;

  mop = airMopNew();
  airMopAdd(mop, hopt, (airMopper)hestOptFree, airMopAlways);

  rgbp = tenEvecRGBParmNew();
  airMopAdd(mop, rgbp, AIR_CAST(airMopper, tenEvecRGBParmNix), airMopAlways);

  hestOptAdd_1_UInt(&hopt, "c", "evec index", &(rgbp->which), NULL,
                    "which eigenvector will be colored. \"0\" for the "
                    "principal, \"1\" for the middle, \"2\" for the minor");
  hestOptAdd_1_Enum(&hopt, "a", "aniso", &(rgbp->aniso), NULL,
                    "Which anisotropy to use for modulating the saturation "
                    "of the colors.  " TEN_ANISO_DESC,
                    tenAniso);
  hestOptAdd_1_Double(&hopt, "t", "thresh", &(rgbp->confThresh), "0.5",
                      "confidence threshold");
  hestOptAdd_1_Double(&hopt, "bg", "background", &(rgbp->bgGray), "0",
                      "gray level to use for voxels who's confidence is zero ");
  hestOptAdd_1_Double(&hopt, "gr", "gray", &(rgbp->isoGray), "0",
                      "the gray level to desaturate towards as anisotropy "
                      "decreases (while confidence remains 1.0)");
  hestOptAdd_1_Double(&hopt, "gam", "gamma", &(rgbp->gamma), "1",
                      "gamma to use on color components");
  hestOptAdd_1_Other(&hopt, "i", "nin", &nin, "-", "input diffusion tensor volume",
                     nrrdHestNrrd);
  hestOptAdd_1_String(&hopt, "o", "nout", &outS, "-", "output image (floating point)");

  airMopAdd(mop, hopt, (airMopper)hestOptFree, airMopAlways);
  USAGE_PARSE(_tend_evecrgbInfoL);
  airMopAdd(mop, hopt, (airMopper)hestParseFree, airMopAlways);

  nout = nrrdNew();
  airMopAdd(mop, nout, (airMopper)nrrdNuke, airMopAlways);
  if (tenEvecRGB(nout, nin, rgbp)) {
    airMopAdd(mop, err = biffGetDone(TEN), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble doing colormapping:\n%s\n", me, err);
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
TEND_CMD(evecrgb, INFO);
