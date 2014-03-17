/*
  Teem: Tools to process and visualize scientific data and images             .
  Copyright (C) 2013, 2012, 2011, 2010, 2009  University of Chicago
  Copyright (C) 2008, 2007, 2006, 2005  Gordon Kindlmann
  Copyright (C) 2004, 2003, 2002, 2001, 2000, 1999, 1998  University of Utah

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public License
  (LGPL) as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  The terms of redistributing and/or modifying this software also
  include exceptions to the LGPL that facilitate static linking.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with this library; if not, write to Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "unrrdu.h"
#include "privateUnrrdu.h"

#define INFO "a hack of some kind"
static const char *_unrrdu_hackInfoL =
(INFO ". This is used as a place to put whatever one-off code "
 "you want to try, with the whatever benefits come with being a "
 "unu command.\n "
 "* (not based on any particular nrrd function)");

int
unrrdu_hackMain(int argc, const char **argv, const char *me,
                hestParm *hparm) {
  hestOpt *opt = NULL;
  char *out, *err;
  Nrrd *nin, *nout;
  int pret;
  airArray *mop;

  char *what;

  hestOptAdd(&opt, NULL, "what", airTypeString, 1, 1, &what, NULL,
             "what hack to do");
  OPT_ADD_NIN(nin, "input nrrd");
  OPT_ADD_NOUT(out, "output nrrd");

  mop = airMopNew();
  airMopAdd(mop, opt, (airMopper)hestOptFree, airMopAlways);

  USAGE(_unrrdu_hackInfoL);
  PARSE();
  airMopAdd(mop, opt, (airMopper)hestParseFree, airMopAlways);

  nout = nrrdNew();
  airMopAdd(mop, nout, (airMopper)nrrdNuke, airMopAlways);

  if (!strcmp(what, "sdincr")) {
    unsigned int sdim, axi;
    sdim = nin->spaceDim;
    if (!sdim) {
      fprintf(stderr, "%s: need non-zero space dimension", me);
      airMopError(mop);
      return 1;
    }
    if (nrrdCopy(nout, nin)) {
      airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
      fprintf(stderr, "%s: error converting nrrd:\n%s", me, err);
      airMopError(mop);
      return 1;
    }
    nout->spaceDim = sdim + 1;
    nout->space = nrrdSpaceUnknown;
    nout->spaceOrigin[sdim] = 0.0;
    for (axi=0; axi<nout->dim; axi++) {
      if (nrrdSpaceVecExists(sdim, nout->axis[axi].spaceDirection)) {
        nout->axis[axi].spaceDirection[sdim] = 0.0;
      }
    }
  } else {
    fprintf(stderr, "%s: no \"%s\" hack implemented", me, what);
    airMopError(mop);
    return 1;
  }

  SAVE(out, nout, NULL);

  airMopOkay(mop);
  return 0;
}

UNRRDU_CMD_HIDE(hack, INFO);
