/*
  teem: Gordon Kindlmann's research software
  Copyright (C) 2003, 2002, 2001, 2000, 1999, 1998 University of Utah

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

#define INFO "Print out min and max values in a nrrd"
char *_unrrdu_minmaxInfoL =
(INFO ". Unlike other commands, this doesn't produce a nrrd.  It only "
 "prints to standard error the min and max values found in the input nrrd, "
 "and it also indicates if there are non-existant values.");

int
unrrdu_minmaxMain(int argc, char **argv, char *me, hestParm *hparm) {
  hestOpt *opt = NULL;
  char *err;
  Nrrd *nin;
  NrrdRange *range;
  airArray *mop;
  int pret;

  mop = airMopNew();
  hestOptAdd(&opt, NULL, "nin", airTypeOther, 1, 1, &nin, NULL,
	     "input nrrd",
	     NULL, NULL, nrrdHestNrrd);
  airMopAdd(mop, opt, (airMopper)hestOptFree, airMopAlways);

  USAGE(_unrrdu_minmaxInfoL);
  PARSE();
  airMopAdd(mop, opt, (airMopper)hestParseFree, airMopAlways);

  range = nrrdRangeNewSet(nin, nrrdBlind8BitRangeFalse);
  airSinglePrintf(stderr, NULL, "min: %f\n", range->min);
  airSinglePrintf(stderr, NULL, "max: %f\n", range->max);
  if (range->hasNonExist) {
    fprintf(stderr, "# has non-existent values\n");
  }

  airMopOkay(mop);
  return 0;
}

UNRRDU_CMD(minmax, INFO);
