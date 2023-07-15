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

#define INFO "Condense axis-0 scanlines into \"blocks\""
static const char *_unrrdu_blockInfoL
  = (INFO ". Output nrrd will be of type \"block\": the type "
          "for an opaque chunk of "
          "memory.  Block samples can be sliced, cropped, shuffled, "
          "permuted, etc., but there is no scalar value associated "
          "with them, so they can not be histogrammed, quantized, "
          "resampled, converted, etc.  The output nrrd will have "
          "one less dimension than input; axis N information will "
          "be shifted down to axis N-1.  Underlying data "
          "is unchanged.");

static int
unrrdu_blockMain(int argc, const char **argv, const char *me, hestParm *hparm) {
  hestOpt *opt = NULL;
  char *out, *err;
  Nrrd *nin, *nout;
  airArray *mop;
  int pret;

  /* nrrdHestNrrdNoTTY simplifies this, just like unquantize and unorient */
  hparm->noArgsIsNoProblem = AIR_TRUE;
  hestOptAdd_1_Other(&opt, "i", "nin", &nin, "-", "input nrrd", nrrdHestNrrdNoTTY);
  OPT_ADD_NOUT(out, "output nrrd");

  mop = airMopNew();
  airMopAdd(mop, opt, hestOptFree_vp, airMopAlways);

  USAGE_OR_PARSE(_unrrdu_blockInfoL);
  airMopAdd(mop, opt, (airMopper)hestParseFree, airMopAlways);

  nout = nrrdNew();
  airMopAdd(mop, nout, (airMopper)nrrdNuke, airMopAlways);

  if (nrrdBlock(nout, nin)) {
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: error blocking nrrd:\n%s", me, err);
    airMopError(mop);
    return 1;
  }

  SAVE(out, nout, NULL);

  airMopOkay(mop);
  return 0;
}

UNRRDU_CMD_HIDE(block, INFO);
