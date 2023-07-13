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

#include "unrrdu.h"
#include "privateUnrrdu.h"

#define INFO "Make image orientation be axis-aligned"
static const char *_unrrdu_unorientInfoL = (INFO ". Does various tricks.\n "
                                                 "* Uses nrrdOrientationReduce");

static int
unrrdu_unorientMain(int argc, const char **argv, const char *me, hestParm *hparm) {
  hestOpt *opt = NULL;
  char *out, *err;
  Nrrd *nin, *nout;
  int pret;
  int setMinsFromOrigin;
  airArray *mop;

  /*
   * with new nrrdHestNrrdNoTTY, can let "-" be default input as normal; no more need
   * for this awkwardness:
   *
   * / * if we gave a default for this, then there it would fine to have
   * no command-line arguments whatsoever, and then the user would not
   * know how to get the basic usage information * /
   * hestOptAdd(&opt, "i,input", "nin", airTypeOther, 1, 1, &nin, NULL,
   *            "input nrrd "
   *            "(sorry, can't use usual default of \"-\" for stdin "
   *            "because of hest quirk)",
   *            NULL, NULL, nrrdHestNrrd);
   */
  hparm->noArgsIsNoProblem = AIR_TRUE;
  hestOptAdd_1_Other(&opt, "i,input", "nin", &nin, "-",
                     "input nrrd. By default reads from stdin", nrrdHestNrrdNoTTY);
  hestOptAdd_Flag(&opt, "smfo", &setMinsFromOrigin,
                  "set some axis mins based on space origin (hack)");
  OPT_ADD_NOUT(out, "output nrrd");

  mop = airMopNew();
  airMopAdd(mop, opt, hestOptFree_vp, airMopAlways);

  USAGE_OR_PARSE(_unrrdu_unorientInfoL);
  airMopAdd(mop, opt, (airMopper)hestParseFree, airMopAlways);

  nout = nrrdNew();
  airMopAdd(mop, nout, (airMopper)nrrdNuke, airMopAlways);

  if (nrrdOrientationReduce(nout, nin, setMinsFromOrigin)) {
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: error unorienting nrrd:\n%s", me, err);
    airMopError(mop);
    return 1;
  }

  SAVE(out, nout, NULL);

  airMopOkay(mop);
  return 0;
}

UNRRDU_CMD(unorient, INFO);
