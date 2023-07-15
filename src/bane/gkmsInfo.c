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

#include "bane.h"
#include "privateBane.h"

#define INFO_INFO "Project histogram volume for opacity function generation"
static const char *_baneGkms_infoInfoL
  = (INFO_INFO ". This distills the histogram volume down to the information required "
               "to create either 1-D or 2-D opacity functions.");

static int /* Biff: 1 */
baneGkms_infoMain(int argc, const char **argv, const char *me, hestParm *hparm) {
  hestOpt *opt = NULL;
  char *outS, *perr;
  Nrrd *hvol, *nout;
  airArray *mop;
  int pret, one, measr;

  hestOptAdd_1_Enum(&opt, "m", "measr", &measr, "mean",
                    "How to project along the 2nd derivative axis.  Possibilities "
                    "include:\n "
                    "\b\bo \"mean\": average value\n "
                    "\b\bo \"median\": value at 50th percentile\n "
                    "\b\bo \"mode\": most common value\n "
                    "\b\bo \"min\", \"max\": probably not useful",
                    baneGkmsMeasr);
  hestOptAdd_Flag(&opt, "one", &one,
                  "Create 1-dimensional info file; default is 2-dimensional");
  hestOptAdd_1_Other(&opt, "i", "hvolIn", &hvol, NULL,
                     "input histogram volume (from \"gkms hvol\")", nrrdHestNrrd);
  hestOptAdd_1_String(&opt, "o", "infoOut", &outS, NULL,
                      "output info file, used by \"gkms pvg\" and \"gkms opac\"");

  mop = airMopNew();
  airMopAdd(mop, opt, (airMopper)hestOptFree, airMopAlways);
  USAGE_PARSE(_baneGkms_infoInfoL);
  airMopAdd(mop, opt, (airMopper)hestParseFree, airMopAlways);
  nout = nrrdNew();
  airMopAdd(mop, nout, (airMopper)nrrdNuke, airMopAlways);

  if (baneOpacInfo(nout, hvol, one ? 1 : 2, measr)) {
    biffAddf(BANE, "%s: trouble distilling histogram info", me);
    airMopError(mop);
    return 1;
  }

  if (nrrdSave(outS, nout, NULL)) {
    biffMovef(BANE, NRRD, "%s: trouble saving info file", me);
    airMopError(mop);
    return 1;
  }

  airMopOkay(mop);
  return 0;
}
BANE_GKMS_CMD(info, INFO_INFO);
