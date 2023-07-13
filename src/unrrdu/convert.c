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

#define INFO "Convert to another type (as if by cast, w/ optional clamp)"
static const char *_unrrdu_convertInfoL
  = (INFO ". By default this does not transform, scale, or intelligently "
          "quantize values; it just copies them from one type to another, which "
          "replicates exactly what you'd get in C when you assign from a variable "
          "of one type to another, or when you cast to a different type. However, "
          "clamping values to the representable range of the output type is possible. "
          "with \"-clamp\". "
          "See also \"unu quantize\","
          "\"unu 2op x\", and \"unu 3op clamp\".\n "
          "* Uses nrrdConvert or nrrdClampConvert");

static int
unrrdu_convertMain(int argc, const char **argv, const char *me, hestParm *hparm) {
  hestOpt *opt = NULL;
  char *out, *err;
  Nrrd *nin, *nout;
  int type, pret, E, doClamp;
  airArray *mop;

  OPT_ADD_TYPE(type, "type to convert to", NULL);
  OPT_ADD_NIN(nin, "input nrrd");
  hestOptAdd_Flag(&opt, "clamp", &doClamp,
                  "clamp input values to representable range of values of "
                  "output type, to avoid wrap-around problems");
  OPT_ADD_NOUT(out, "output nrrd");

  mop = airMopNew();
  airMopAdd(mop, opt, hestOptFree_vp, airMopAlways);

  USAGE_OR_PARSE(_unrrdu_convertInfoL);
  airMopAdd(mop, opt, (airMopper)hestParseFree, airMopAlways);

  nout = nrrdNew();
  airMopAdd(mop, nout, (airMopper)nrrdNuke, airMopAlways);

  if (doClamp) {
    E = nrrdClampConvert(nout, nin, type);
  } else {
    E = nrrdConvert(nout, nin, type);
  }
  if (E) {
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: error converting nrrd:\n%s", me, err);
    airMopError(mop);
    return 1;
  }

  SAVE(out, nout, NULL);

  airMopOkay(mop);
  return 0;
}

UNRRDU_CMD(convert, INFO);
