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

#define INFO "Calculate structure tensors from a scalar field"
static const char *_tend_stenInfoL
  = (INFO ".  Not a diffusion tensor, but it is symmetric and positive-definate.");

static int
tend_stenMain(int argc, const char **argv, const char *me, hestParm *hparm) {
  int pret;
  hestOpt *hopt = NULL;
  char *perr, *err;
  airArray *mop;

  int iScale, dScale, dsmp;
  Nrrd *nin, *nout;
  char *outS;

  hestOptAdd_1_Int(&hopt, "ds", "diff. scale", &dScale, "1",
                   "differentiation scale, in pixels: the radius of the "
                   "kernel used for differentation to compute gradient vectors");
  hestOptAdd_1_Int(&hopt, "is", "int. scale", &iScale, "2",
                   "integration scale, in pixels: the radius of the "
                   "kernel used for blurring outer products of gradients "
                   "in order compute structure tensors");
  hestOptAdd_1_Int(&hopt, "df", "downsample factor", &dsmp, "1",
                   "the factor by which to downsample when creating volume of "
                   "structure tensors");
  hestOptAdd_1_Other(&hopt, "i", "nin", &nin, "-", "input scalar volume", nrrdHestNrrd);
  hestOptAdd_1_String(&hopt, "o", "nout", &outS, "-", "output filename");

  mop = airMopNew();
  airMopAdd(mop, hopt, (airMopper)hestOptFree, airMopAlways);
  USAGE_PARSE(_tend_stenInfoL);
  airMopAdd(mop, hopt, (airMopper)hestParseFree, airMopAlways);

  nout = nrrdNew();
  airMopAdd(mop, nout, (airMopper)nrrdNuke, airMopAlways);

  if (gageStructureTensor(nout, nin, dScale, iScale, dsmp)) {
    airMopAdd(mop, err = biffGetDone(GAGE), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble calculating structure tensors:\n%s\n", me, err);
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
TEND_CMD(sten, INFO);
