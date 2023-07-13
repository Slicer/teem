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

#define INFO "Perform histogram equalization"
static const char *_unrrdu_heqInfoL
  = (INFO ". If this seems to be doing nothing, try increasing the "
          "number of histograms bins by an order of magnitude or "
          "two (or more).  Or, use \"unu gamma\" to warp the values "
          "in the direction you know they need to go.  Either of "
          "these might work because extremely tall and narrow peaks "
          "in the equalization histogram will produce poor results.\n "
          "* Uses nrrdHistoEq");

static int
unrrdu_heqMain(int argc, const char **argv, const char *me, hestParm *hparm) {
  hestOpt *opt = NULL;
  char *out, *err, *mapS;
  Nrrd *nin, *nout, *nmap;
  int pret;
  unsigned int bins, smart;
  airArray *mop;
  float amount;

  /* we want to facilitate saving out the mapping as a text file,
     but with the domain included */
  /* this is commented out with the 8 Aug 2003 advent of nrrdDefGetenv
  nrrdDefWriteBareTable = AIR_FALSE;
  */

  hestOptAdd_1_UInt(&opt, "b,bin", "bins", &bins, NULL,
                    "# bins to use in histogram that is created in order to "
                    "calculate the mapping that achieves the equalization.");
  hestOptAdd_1v_UInt(&opt, "s,smart", "bins", &smart, "0",
                     "# bins in value histogram to ignore in calculating the mapping. "
                     "Bins are ignored when they get more hits than other bins, and "
                     "when the values that fall in them are constant.  This is an "
                     "effective way to prevent large regions of background value "
                     "from distorting the equalization mapping.");
  hestOptAdd_1_Float(&opt, "a,amount", "amount", &amount, "1.0",
                     "extent to which the histogram equalizing mapping should be "
                     "applied; 0.0: no change, 1.0: full equalization");
  hestOptAdd_1_String(&opt, "m,map", "filename", &mapS, "",
                      "The value mapping used to achieve histogram equalization is "
                      "represented by a univariate regular map.  By giving a filename "
                      "here, that map can be saved out and applied to other nrrds "
                      "with \"unu rmap\"");
  OPT_ADD_NIN(nin, "input nrrd");
  OPT_ADD_NOUT(out, "output nrrd");

  mop = airMopNew();
  airMopAdd(mop, opt, hestOptFree_vp, airMopAlways);

  USAGE_OR_PARSE(_unrrdu_heqInfoL);
  airMopAdd(mop, opt, (airMopper)hestParseFree, airMopAlways);

  nout = nrrdNew();
  airMopAdd(mop, nout, (airMopper)nrrdNuke, airMopAlways);

  if (nrrdHistoEq(nout, nin, airStrlen(mapS) ? &nmap : NULL, bins, smart, amount)) {
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble histogram equalizing:\n%s", me, err);
    airMopError(mop);
    return 1;
  }

  if (airStrlen(mapS)) {
    SAVE(mapS, nmap, NULL);
  }
  SAVE(out, nout, NULL);

  airMopOkay(mop);
  return 0;
}

UNRRDU_CMD(heq, INFO);
