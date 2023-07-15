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

#define INFO "Generate barycentric histograms of anisotropy"
static const char *_tend_anhistInfoL
  = (INFO ".  The barycentric space used is either one of Westin's "
          "triple of spherical, linear, and planar anisotropy.  The bin "
          "counts in the histogram are weighted by the confidence value.");

static int
tend_anhistMain(int argc, const char **argv, const char *me, hestParm *hparm) {
  int pret;
  hestOpt *hopt = NULL;
  char *perr, *err;
  airArray *mop;

  int version, res, right;
  Nrrd *nin, *nout, *nwght;
  char *outS;

  hestOptAdd_1_Int(&hopt, "v", "westin version", &version, "1",
                   "Which version of Westin's anisotropy metric triple "
                   "to use, either \"1\" or \"2\"");
  hestOptAdd_1_Other(&hopt, "w", "nweight", &nwght, "",
                     "how to weigh contributions to histogram.  By default "
                     "(not using this option), the increment is one bin count per "
                     "sample, but by giving a nrrd, the value in the nrrd at the "
                     "corresponding location will be the bin count increment ",
                     nrrdHestNrrd);
  hestOptAdd_1_Int(&hopt, "r", "res", &res, NULL, "resolution of anisotropy plot");
  hestOptAdd_Flag(&hopt, "right", &right,
                  "sample a right-triangle-shaped region, instead of "
                  "a roughly equilateral triangle. ");
  hestOptAdd_1_Other(&hopt, "i", "nin", &nin, "-", "input diffusion tensor volume",
                     nrrdHestNrrd);
  hestOptAdd_1_String(&hopt, "o", "nout", &outS, "-", "output image (floating point)");

  mop = airMopNew();
  airMopAdd(mop, hopt, (airMopper)hestOptFree, airMopAlways);
  USAGE_PARSE(_tend_anhistInfoL);
  airMopAdd(mop, hopt, (airMopper)hestParseFree, airMopAlways);

  nout = nrrdNew();
  airMopAdd(mop, nout, (airMopper)nrrdNuke, airMopAlways);

  if (tenAnisoHistogram(nout, nin, nwght, right, version, res)) {
    airMopAdd(mop, err = biffGetDone(TEN), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble making histogram:\n%s\n", me, err);
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
TEND_CMD(anhist, INFO);
