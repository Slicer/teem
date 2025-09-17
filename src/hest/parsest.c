/*
  Teem: Tools to process and visualize scientific data and images
  Copyright (C) 2009--2025  University of Chicago
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
  You should have received a copy of the GNU Lesser General Public License
  along with this library; if not, see <https://www.gnu.org/licenses/>.
*/

#include "hest.h"
#include "privateHest.h"

/* (parse, parser, parsest) */

/* A little trickery for error reporting.  For many of the functions here, if they hit an
error and hparm->verbosity is set, then we should reveal the current function name (set
by convention in `me`). But without verbosity, we hide that function name, so it appears
that the error is coming from the caller (probably identified as argv[0]).  However, that
means that functions using this `ME` macro should (in defiance of convention) set to `me`
to `functionname: ` (NOTE the `: `) so that all of that goes away without verbosity. And,
that means that error message generation here should also defy convention and instead of
being "%s: what happened" it should just be "%swhat happened" */
#define ME ((hparm && hparm->verbosity) ? me : "")

int
hestParse2(hestOpt *opt, int argc, const char **argv, char **_errP,
           const hestParm *_hparm) {
  /* see note on ME (at top) for why me[] ends with ": " */
  // static const char me[] = "hestParse2: ";

  /* -------- initialize the mop */
  airArray *mop = airMopNew();

  /* -------- exactly one of (given) _hparm and (our) hparm is non-NULL */
  hestParm *hparm = NULL;
  if (!_hparm) {
    hparm = hestParmNew();
    airMopAdd(mop, hparm, (airMopper)hestParmFree, airMopAlways);
  }
  /* how to const-correctly use hparm or _hparm in an expression */
#define HPARM (_hparm ? _hparm : hparm)

  /* -------- allocate the err string. We do it a dumb way for now.
     TODO: make this smarter */
  uint eslen = 2 * AIR_STRLEN_HUGE;
  char *err = AIR_CALLOC(eslen + 1, char);
  assert(err);
  if (_errP) {
    /* they care about the error string, so mop it only when there is _not_ an error */
    *_errP = err;
    airMopAdd(mop, _errP, (airMopper)airSetNull, airMopOnOkay);
    airMopAdd(mop, err, airFree, airMopOnOkay);
  } else {
    /* otherwise, we're making the error string just for our own convenience,
       so we always clean it up on exit */
    airMopAdd(mop, err, airFree, airMopAlways);
  }

  /* -------- check on validity of the hestOpt array */
  if (_hestOptCheck(opt, err, HPARM)) {
    airMopError(mop);
    return 1;
  }

  /* -------- allocate the state we use during parsing */
  hestInputStack *hist = hestInputStackNew();
  airMopAdd(mop, hist, (airMopper)hestInputStackNix, airMopAlways);
  hestArgVec *havec = hestArgVecNew();
  airMopAdd(mop, havec, (airMopper)hestArgVecNix, airMopAlways);
  hestInputStackPushCommandLine(hist, argc, argv);
  hestInputStackProcess(havec, hist);
  hestArgVecPrint(__func__, havec);

  airMopOkay(mop);
  return 0;
}
