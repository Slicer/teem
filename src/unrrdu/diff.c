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

#define INFO "Sees if two nrrds are different in any way"
static const char *_unrrdu_diffInfoL
  = (INFO ". Looks through all fields to see if two given nrrds contain the "
          "same information. Or, array meta-data can be excluded, and comparison "
          "only on the data values is done with the -od flag.\n "
          "* Uses nrrdCompare");

static int
unrrdu_diffMain(int argc, const char **argv, const char *me, hestParm *hparm) {
  hestOpt *opt = NULL;
  char *err;
  airArray *mop;
  int pret;

  Nrrd *ninA, *ninB;
  int quiet, exitstat, onlyData, differ, ret;
  double epsilon;
  char explain[AIR_STRLEN_LARGE + 1];

  mop = airMopNew();
  hestOptAdd_1_Other(&opt, NULL, "ninA", &ninA, NULL, "First input nrrd.", nrrdHestNrrd);
  hestOptAdd_1_Other(&opt, NULL, "ninB", &ninB, NULL, "Second input nrrd.",
                     nrrdHestNrrd);
  hestOptAdd_1_Double(&opt, "eps,epsilon", "eps", &epsilon, "0.0",
                      "threshold for allowable difference in values in "
                      "data values");
  hestOptAdd_Flag(&opt, "q,quiet", &quiet,
                  "be quiet (like regular diff), so that nothing is printed "
                  "if the nrrds are the same");
  hestOptAdd_Flag(&opt, "x,exit", &exitstat,
                  "use the exit status (like regular diff) to indicate if "
                  "there was a significant difference (as if it's an error)");
  hestOptAdd_Flag(&opt, "od,onlydata", &onlyData,
                  "Compare data values only, excluding array meta-data");
  airMopAdd(mop, opt, hestOptFree_vp, airMopAlways);

  USAGE_OR_PARSE(_unrrdu_diffInfoL);
  airMopAdd(mop, opt, (airMopper)hestParseFree, airMopAlways);

  if (nrrdCompare(ninA, ninB, onlyData, epsilon, &differ, explain)) {
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: error doing compare:\n%s", me, err);
    airMopError(mop);
    return 1;
  }
  if (differ) {
    printf("%s: %s differ: %s\n", me, onlyData ? "data values" : "nrrds", explain);
    ret = 1;
  } else {
    if (!quiet) {
      if (0 == epsilon) {
        printf("%s: %s are the same\n", me, onlyData ? "data values" : "nrrds");
      } else {
        printf("%s: %s are same or within %g of each other\n", me,
               onlyData ? "data values" : "nrrds", epsilon);
      }
    }
    ret = 0;
  }

  airMopOkay(mop);
  return exitstat ? ret : 0;
}

UNRRDU_CMD(diff, INFO);
