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

#define INFO "Find connected components (CCs)"
static const char *_unrrdu_ccfindInfoL
  = (INFO ". This works on 1-byte and 2-byte integral values, as well as "
          "4-byte ints.\n "
          "* Uses nrrdCCFind");

static int
unrrdu_ccfindMain(int argc, const char **argv, const char *me, hestParm *hparm) {
  hestOpt *opt = NULL;
  char *out, *err, *valS;
  Nrrd *nin, *nout, *nval = NULL;
  airArray *mop;
  int type, pret;
  unsigned int conny;

  hestOptAdd_1_String(&opt, "v,values", "filename", &valS, "",
                      "Giving a filename here allows you to save out the values "
                      "associated with each connect component.  This can be used "
                      "later with \"ccmerge -d\".  By default, no record of the "
                      "original CC values is kept.");
  hestOptAdd_1_Other(&opt, "t,type", "type", &type, "default",
                     "type to use for output, to store the CC ID values.  By default "
                     "(not using this option), the type used will be the smallest of "
                     "uchar, ushort, or int, that can represent all the CC ID values. "
                     "Using this option allows one to specify the integral type to "
                     "be used.",
                     &unrrduHestMaybeTypeCB);
  hestOptAdd_1_UInt(&opt, "c,connect", "connectivity", &conny, NULL,
                    "what kind of connectivity to use: the number of coordinates "
                    "that vary in order to traverse the neighborhood of a given "
                    "sample.  In 2D: \"1\": 4-connected, \"2\": 8-connected");
  OPT_ADD_NIN(nin, "input nrrd");
  OPT_ADD_NOUT(out, "output nrrd");

  mop = airMopNew();
  airMopAdd(mop, opt, hestOptFree_vp, airMopAlways);

  USAGE_OR_PARSE(_unrrdu_ccfindInfoL);
  airMopAdd(mop, opt, (airMopper)hestParseFree, airMopAlways);

  nout = nrrdNew();
  airMopAdd(mop, nout, (airMopper)nrrdNuke, airMopAlways);

  if (nrrdCCFind(nout, airStrlen(valS) ? &nval : NULL, nin, type, conny)) {
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: error doing connected components:\n%s", me, err);
    airMopError(mop);
    return 1;
  }
  if (nval) {
    airMopAdd(mop, nval, (airMopper)nrrdNuke, airMopAlways);
  }

  if (airStrlen(valS)) {
    SAVE(valS, nval, NULL);
  }
  SAVE(out, nout, NULL);

  airMopOkay(mop);
  return 0;
}

UNRRDU_CMD(ccfind, INFO);
