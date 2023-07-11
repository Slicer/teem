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

#define INFO "converts from 1-D world to index position"
static const char *_unrrdu_w2iInfoL
  = (INFO ", given the centering of the data (cell vs. node), "
          "the range of positions, and the number of intervals into "
          "which position has been quantized. "
          "This is a demo/utility, which does not actually operate on any nrrds. "
          "Previously available as the stand-alone pos2idx binary.\n "
          "* Uses NRRD_IDX macro");

static int
unrrdu_w2iMain(int argc, const char **argv, const char *me, hestParm *hparm) {
  hestOpt *opt = NULL;
  airArray *mop;
  int pret;
  char *err;

  int center;
  double minPos, maxPos, pos, indx, size;

  mop = airMopNew();
  hestOptAdd_1_Enum(&opt, NULL, "center", &center, NULL,
                    "which centering applies to the quantized position.\n "
                    "Possibilities are:\n "
                    "\b\bo \"cell\": for histogram bins, quantized values, and "
                    "pixels-as-squares\n "
                    "\b\bo \"node\": for non-trivially interpolated "
                    "sample points",
                    nrrdCenter);
  hestOptAdd_1_Double(&opt, NULL, "minPos", &minPos, NULL,
                      "smallest position associated with index 0");
  hestOptAdd_1_Double(&opt, NULL, "maxPos", &maxPos, NULL,
                      "highest position associated with highest index");
  hestOptAdd_1_Double(&opt, NULL, "num", &size, NULL,
                      "number of intervals into which position has been quantized");
  hestOptAdd_1_Double(&opt, NULL, "world", &pos, NULL,
                      "the input world position, to be converted to index");
  airMopAdd(mop, opt, hestOptFree_vp, airMopAlways);
  USAGE_OR_PARSE(_unrrdu_w2iInfoL);
  airMopAdd(mop, opt, (airMopper)hestParseFree, airMopAlways);

  indx = NRRD_IDX(center, minPos, maxPos, size, pos);
  printf("%g\n", indx);

  airMopOkay(mop);
  return 0;
}

UNRRDU_CMD(w2i, INFO);
