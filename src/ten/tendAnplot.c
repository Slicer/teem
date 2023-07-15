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

#define INFO "Graph anisotropy metric in barycentric coords"
static const char *_tend_anplotInfoL
  = (INFO ".  The metrics all vary from 0.0 to 1.0, and will be sampled "
          "in the lower right half of the image.  The plane on which they are "
          "sampled is a surface of constant trace.  You may want to use "
          "\"unu resample -s = x0.57735 -k tent\" to transform the triangle into "
          "a 30-60-90 triangle, and \"ilk -t 1,-0.5,0,0,0.866,0 -k tent "
          "-0 u:0,1 -b pad -bg 0\" (possibly followed by "
          "teem/src/limntest/triimg) to transform the domain into an equilateral "
          "triangle.");

static int
tend_anplotMain(int argc, const char **argv, const char *me, hestParm *hparm) {
  int pret;
  hestOpt *hopt = NULL;
  char *perr, *err;
  airArray *mop;

  int aniso, whole, nanout, hflip;
  unsigned int res;
  Nrrd *nout;
  char *outS;

  hestOptAdd_1_UInt(&hopt, "r", "res", &res, "256", "resolution of anisotropy plot");
  hestOptAdd_Flag(&hopt, "w", &whole,
                  "sample the whole triangle of constant trace, "
                  "instead of just the "
                  "sixth of it in which the eigenvalues have the "
                  "traditional sorted order. ");
  hestOptAdd_Flag(&hopt, "hflip", &hflip,
                  "flip the two bottom corners (swapping the place of "
                  "linear and planar)");
  hestOptAdd_Flag(&hopt, "nan", &nanout,
                  "set the pixel values outside the triangle to be NaN, "
                  "instead of 0");
  hestOptAdd_1_Enum(&hopt, "a", "aniso", &aniso, NULL,
                    "Which anisotropy metric to plot.  " TEN_ANISO_DESC, tenAniso);
  hestOptAdd_1_String(&hopt, "o", "nout", &outS, "-", "output image (floating point)");

  mop = airMopNew();
  airMopAdd(mop, hopt, (airMopper)hestOptFree, airMopAlways);
  USAGE_JUSTPARSE(_tend_anplotInfoL);
  airMopAdd(mop, hopt, (airMopper)hestParseFree, airMopAlways);

  nout = nrrdNew();
  airMopAdd(mop, nout, (airMopper)nrrdNuke, airMopAlways);
  if (tenAnisoPlot(nout, aniso, res, hflip, whole, nanout)) {
    airMopAdd(mop, err = biffGetDone(TEN), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble making plot:\n%s\n", me, err);
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
TEND_CMD(anplot, INFO);
