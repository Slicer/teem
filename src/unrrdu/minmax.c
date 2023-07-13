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

#include <unistd.h> /* for isatty() and STDIN_FILENO */

#define INFO "Print out min and max values in one or more nrrds"
static const char *_unrrdu_minmaxInfoL
  = (INFO ". Unlike other commands, this doesn't produce a nrrd.  It only "
          "prints to standard out the min and max values found in the input nrrd(s), "
          "and it also indicates if there are non-existent values.\n "
          "* Uses nrrdRangeNewSet");

static int /* Biff: 1 */
unrrdu_minmaxDoit(const char *me, char *inS, int blind8BitRange, int singleLine,
                  FILE *fout) {
  Nrrd *nrrd;
  NrrdRange *range;
  airArray *mop;

  if (!strcmp("-", inS) && isatty(STDIN_FILENO)) {
    biffAddf(me, "declining to try reading Nrrd from stdin as tty (terminal)");
    return 1;
  }

  mop = airMopNew();
  airMopAdd(mop, nrrd = nrrdNew(), (airMopper)nrrdNuke, airMopAlways);
  if (nrrdLoad(nrrd, inS, NULL)) {
    biffMovef(me, NRRD, "%s: trouble loading \"%s\"", me, inS);
    airMopError(mop);
    return 1;
  }

  range = nrrdRangeNewSet(nrrd, blind8BitRange);
  airMopAdd(mop, range, (airMopper)nrrdRangeNix, airMopAlways);
  if (singleLine) {
    char minStr[128], maxStr[128], nexStr[128];
    airSinglePrintf(NULL, minStr, "%.17g", range->min);
    airSinglePrintf(NULL, maxStr, "%.17g", range->max);
    if (range->hasNonExist) {
      strcpy(nexStr, " non-existent");
    } else {
      strcpy(nexStr, "");
    }
    fprintf(fout, "%s %s%s\n", minStr, maxStr, nexStr);
  } else {
    airSinglePrintf(fout, NULL, "min: %.17g\n", range->min);
    airSinglePrintf(fout, NULL, "max: %.17g\n", range->max);
    if (range->min == range->max) {
      if (0 == range->min) {
        fprintf(fout, "# min == max == 0.0 exactly\n");
      } else {
        fprintf(fout, "# min == max\n");
      }
    }
    if (range->hasNonExist) {
      fprintf(fout, "# has non-existent values\n");
    }
  }

  airMopOkay(mop);
  return 0;
}

static int
unrrdu_minmaxMain(int argc, const char **argv, const char *me, hestParm *hparm) {
  hestOpt *opt = NULL;
  char *err, **inS;
  airArray *mop;
  int pret, blind8BitRange, singleLine, okay;
  unsigned int ni, ninLen;
#define B8DEF "false"

  mop = airMopNew();
  hparm->noArgsIsNoProblem = AIR_TRUE;
  hestOptAdd_1_Bool(&opt, "blind8", "bool", &blind8BitRange,
                    B8DEF, /* NOTE: not using nrrdStateBlind8BitRange here
                              for consistency with previous behavior */
                    "whether to blindly assert the range of 8-bit data, "
                    "without actually going through the data values, i.e. "
                    "uchar is always [0,255], signed char is [-128,127]. "
                    "Note that even if you do not use this option, the default "
                    "(" B8DEF ") is potentialy over-riding the effect of "
                    "environment variable NRRD_STATE_BLIND_8_BIT_RANGE; "
                    "see \"unu env\"");
  hestOptAdd_Flag(
    &opt, "sl", &singleLine,
    "Without this option, output is on multiple lines (for min, for max, "
    "and then maybe more lines about non-existent values or min, max "
    "conditions). With \"-sl\", output is a single line containing just min "
    "and max, possibly followed by the single word \"non-existent\" if and "
    "only if there were non-existent values. If there are multiple inputs, "
    "the input filename is printed first on the per-input single line.");
  hestOptAdd_Nv_String(&opt, NULL, "nin1", 1, -1, &inS, "-", "input nrrd(s)", &ninLen);
  airMopAdd(mop, opt, hestOptFree_vp, airMopAlways);

  USAGE_OR_PARSE(_unrrdu_minmaxInfoL);
  airMopAdd(mop, opt, (airMopper)hestParseFree, airMopAlways);

  /* HEY "okay" logic copied from head.c */
  okay = AIR_FALSE;
  for (ni = 0; ni < ninLen; ni++) {
    if (ninLen > 1) {
      if (singleLine) {
        fprintf(stdout, "%s ", inS[ni]);
      } else {
        fprintf(stdout, "==> %s <==\n", inS[ni]);
      }
    }
    if (unrrdu_minmaxDoit(me, inS[ni], blind8BitRange, singleLine, stdout)) {
      airMopAdd(mop, err = biffGetDone(me), airFree, airMopAlways);
      fprintf(stderr, "%s: trouble with \"%s\":\n%s", me, inS[ni], err);
      /* continue working on the remaining files */
    } else {
      /* processed at least one file ok */
      okay = AIR_TRUE;
    }
    if (ninLen > 1 && ni < ninLen - 1 && !singleLine) {
      fprintf(stdout, "\n");
    }
  }
  if (!okay) {
    /* none of the given files could be read; something is wrong */
    if (ninLen > 1) {
      fprintf(stderr, "\n%s: Unable to read data from any file\n", me);
    }
    hestUsage(stderr, opt, me, hparm);
    fprintf(stderr, "\nFor more info: \"%s --help\"\n", me);
  }

  airMopOkay(mop);
  return 0;
}

UNRRDU_CMD(minmax, INFO);
