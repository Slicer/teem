/*
  teem: Gordon Kindlmann's research software
  Copyright (C) 2005  Gordon Kindlmann
  Copyright (C) 2004, 2003, 2002, 2001, 2000, 1999, 1998  University of Utah

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "unrrdu.h"
#include "privateUnrrdu.h"

#define INFO "Undo the action of \"unu tile\""
char *_unrrdu_untileInfoL =
(INFO
 ". To untile the data set you split two axes, permute the pieces "
 "to a new dimenstion.  The axes are then merged.  The net result "
 "is a nrrd with one more dimension.  To undo the tile operation "
 "the arguments can be nearly the same.  For example, if you tiled "
 "to a higher axis with tile (i.e. axis 1 to axis 0 and 2), you "
 "need to subtract 1 from that higher axis on the untile (-a 0 1 1).");

int
unrrdu_untileMain(int argc, char **argv, char *me, hestParm *hparm) {
  hestOpt *opt = NULL;
  char *out, *err;
  Nrrd *nin, *nout;
  int size[2], axes[3], pret;
  airArray *mop;

  hestOptAdd(&opt, "a", "axMerge ax0 ax1", airTypeInt, 3, 3, axes, NULL,
             "an axis is extracted from ax0 and ax1 and merged into axMerge");
  hestOptAdd(&opt, "s", "fast, slow sizes", airTypeInt, 2, 2, size, NULL,
             "fast and slow axis sizes to produce as result of splitting "
             "given axis.");
  OPT_ADD_NIN(nin, "input nrrd");
  OPT_ADD_NOUT(out, "output nrrd");

  mop = airMopNew();
  airMopAdd(mop, opt, (airMopper)hestOptFree, airMopAlways);

  USAGE(_unrrdu_untileInfoL);
  PARSE();
  airMopAdd(mop, opt, (airMopper)hestParseFree, airMopAlways);

  nout = nrrdNew();
  airMopAdd(mop, nout, (airMopper)nrrdNuke, airMopAlways);
  
  if (nrrdUntile2D(nout, nin, axes[1], axes[2], axes[0], size[0], size[1])) {
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: error tiling nrrd:\n%s", me, err);
    airMopError(mop);
    return 1;
  }

  SAVE(out, nout, NULL);

  airMopOkay(mop);
  return 0;
}

UNRRDU_CMD(untile, INFO);
