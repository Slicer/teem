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

#define INFO "Create image of 1-D value histogram"
static const char *_unrrdu_dhistoInfoL
  = (INFO
     ". With \"-nolog\", this becomes a quick & dirty way of plotting a function.\n "
     "* Uses nrrdHistoDraw");

static int
unrrdu_dhistoMain(int argc, const char **argv, const char *me, hestParm *hparm) {
  hestOpt *opt = NULL;
  char *out, *err;
  Nrrd *nin, *nout;
  int pret, nolog, notick;
  unsigned int size;
  airArray *mop;
  double max;

  hestOptAdd_1_UInt(&opt, "h,height", "height", &size, NULL,
                    "height of output image (horizontal size is determined by "
                    "number of bins in input histogram).");
  hestOptAdd_Flag(&opt, "nolog", &nolog,
                  "do not show the log-scaled histogram with decade tick-marks");
  hestOptAdd_Flag(&opt, "notick", &notick, "do not draw the log decade tick marks");
  hestOptAdd_1_Double(&opt, "max,maximum", "max # hits", &max, "nan",
                      "constrain the top of the drawn histogram to be at this "
                      "number of hits.  This will either scale the drawn histogram "
                      "downward or clip its top, depending on whether the given max "
                      "is higher or lower than the actual maximum bin count.  By "
                      "not using this option (the default), the actual maximum bin "
                      "count is used");
  OPT_ADD_NIN(nin, "input nrrd");
  OPT_ADD_NOUT(out, "output nrrd");

  mop = airMopNew();
  airMopAdd(mop, opt, hestOptFree_vp, airMopAlways);

  USAGE_OR_PARSE(_unrrdu_dhistoInfoL);
  airMopAdd(mop, opt, (airMopper)hestParseFree, airMopAlways);

  nout = nrrdNew();
  airMopAdd(mop, nout, (airMopper)nrrdNuke, airMopAlways);

  if (nrrdHistoDraw(nout, nin, size, nolog ? AIR_FALSE : (notick ? 2 : AIR_TRUE), max)) {
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: error drawing histogram nrrd:\n%s", me, err);
    airMopError(mop);
    return 1;
  }

  SAVE(out, nout, NULL);

  airMopOkay(mop);
  return 0;
}

UNRRDU_CMD(dhisto, INFO);
