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


#include "../dye.h"

char *me;

void
usage() {
  /*                      0       1       2         3         4    (5) */
  fprintf(stderr, "usage: %s <spaceIn> <imgIn> <spaceOut> <imgOut>\n", me);
  exit(1);
}

int
main(int argc, char *argv[]) {
  char *inFN, *outFN, *inSpcS, *otSpcS, *err;
  int inSpc, otSpc, hack3d;
  airArray *mop;
  Nrrd *nin, *nout;
  float *id, *od;
  unsigned int ii, nn;
  dyeColor *col;

  me = argv[0];
  if (5 != argc)
    usage();
  inSpcS = argv[1];
  inFN = argv[2];
  otSpcS = argv[3];
  outFN = argv[4];

  inSpc = dyeStrToSpace(inSpcS);
  if (dyeSpaceUnknown == inSpc) {
    fprintf(stderr, "%s: couldn't parse \"%s\" as colorspace\n", me, inSpcS);
    exit(1);
  }
  otSpc = dyeStrToSpace(otSpcS);
  if (dyeSpaceUnknown == otSpc) {
    fprintf(stderr, "%s: couldn't parse \"%s\" as colorspace\n", me, otSpcS);
    exit(1);
  }

  mop = airMopNew();
  nin = nrrdNew();
  airMopAdd(mop, nin, (airMopper)nrrdNuke, airMopAlways);
  nout = nrrdNew();
  airMopAdd(mop, nout, (airMopper)nrrdNuke, airMopAlways);
  if (nrrdLoad(nin, inFN, NULL)
      || nrrdCopy(nout, nin)) {
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble:\n%s", me, err);
    airMopError(mop);
    exit(1);
  }
  if (nrrdTypeFloat != nin->type) {
    fprintf(stderr, "%s: sorry, require type %s (not %s)\n", me,
            airEnumStr(nrrdType, nrrdTypeFloat),
            airEnumStr(nrrdType, nin->type));
    airMopError(mop);
    exit(1);
  }
  if (2 == nin->dim && 3 == nin->axis[0].size) {
    /* as a big hack, promote to 3D */
    nin->dim = 3;
    nin->axis[2].size = 1;
    hack3d = AIR_TRUE;
  } else {
    hack3d = AIR_FALSE;
  }
  if (!( 3 == nin->dim && 3 == nin->axis[0].size )) {
    fprintf(stderr, "%s: sorry, need 3D 3-by-X-by-Y array (not %u-D %u-by)\n", me,
            nin->dim, AIR_UINT(nin->axis[0].size));
    airMopError(mop);
    exit(1);
  }
  id = AIR_CAST(float *, nin->data);
  od = AIR_CAST(float *, nout->data);

  col = dyeColorNew();
  airMopAdd(mop, col, (airMopper)dyeColorNix, airMopAlways);

  nn = AIR_UINT(nin->axis[1].size * nin->axis[2].size);
  for (ii=0; ii<nn; ii++) {
    dyeColorSet(col, inSpc, id[0 + 3*ii], id[1 + 3*ii], id[2 + 3*ii]);
    dyeConvert(col, otSpc);
    dyeColorGet(od + 0 + 3*ii, od + 1 + 3*ii, od + 2 + 3*ii, col);
  }

  if (hack3d) {
    nout->dim = 2;
  }
  if (nrrdSave(outFN, nout, NULL)) {
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble:\n%s", me, err);
    airMopError(mop);
    exit(1);
  }

  exit(0);
}
