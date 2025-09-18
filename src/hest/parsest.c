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

/* parse, parser, parsest: this aims to be the final implmentation of hestParse */

/* A little trickery for error reporting.  For many of the functions here, if they hit an
error and hparm->verbosity is set, then we should reveal the current function name (set
by convention in `me`). But without verbosity, we hide that function name, so it appears
that the error is coming from the caller (probably identified as argv[0]).  However, that
means that functions using this `ME` macro should (in defiance of convention) set to `me`
to `functionname: ` (NOTE the `: `) so that all of that goes away without verbosity. And,
that means that error message generation here should also defy convention and instead of
being "%s: what happened" it should just be "%swhat happened" */
// how to const-correctly use either given (const) _hparm or own (non-const) hparm
#define HPARM (_hparm ? _hparm : hparm)
#define HMME  (HPARM->verbosity ? me : "")
#define ME    ((hparm && hparm->verbosity) ? me : "")

static int
histProc(hestArgVec *havec, int *helpWantedP, hestInputStack *hist, char *err,
         const hestParm *hparm) {
  static const char me[] = "histProc: ";
  int ret = 0;
  int popAtEnd = AIR_FALSE;
  *helpWantedP = AIR_FALSE; // may over-write later
  hestInput *hinTop = hist->hin + (hist->len - 1);
  switch (hinTop->source) {
  case hestSourceDefault: // ---------------------------------
    sprintf(err, "%ssorry hestSourceDefault not implemented\n", ME);
    ret = 1;
    break;
  case hestSourceCommandLine: // ---------------------------------
    /* argv[] 0   1     2    3  (argc=4) */
    /*       cmd arg1 arg2 arg3 */
    if (hparm->verbosity > 1) {
      printf("%shist->len=%u -> hinTop=%p\n", me, hist->len, AIR_VOIDP(hinTop));
    }
    if (hinTop->argIdx < hinTop->argc) {
      /* there are args left to parse */
      const char *thisArgv = hinTop->argv[hinTop->argIdx];
      if (hparm->verbosity > 1) {
        printf("%slooking at argv[%u] |%s|\n", me, hinTop->argIdx, thisArgv);
      }
      hinTop->argIdx++;
      if (hparm->respectDashBraceComments && !strcmp("-{", thisArgv)) {
        // start of -{ }- commenting (or increase in nesting level)
        hinTop->dashBraceComment += 1;
      }
      if (!hinTop->dashBraceComment) {
        hestArgVecAppendString(havec, thisArgv);
      }
      if (hparm->respectDashBraceComments && !strcmp("}-", thisArgv)) {
        if (hinTop->dashBraceComment) {
          hinTop->dashBraceComment -= 1;
        } else {
          sprintf(err, "%send comment arg \"}-\" not balanced by prior \"-{\"", ME);
          ret = 1;
        }
      }
    }
    if (hinTop->argIdx == hinTop->argc) {
      // we have gotten to the end of the given argv array */
      if (hinTop->dashBraceComment) {
        sprintf(err, "%sstart comment arg \"-{\" not balanced by later \"}-\"", ME);
        ret = 1;
      } else {
        popAtEnd = AIR_TRUE;
        // but don't pop now because we need to check for --help
      }
    }
    break;
  case hestSourceResponseFile: // ---------------------------------
    sprintf(err, "%ssorry hestSourceResponseFile not implemented\n", ME);
    ret = 1;
    break;
  }
  /* when processing command-line or response file, check for --help
     (it makes no sense for --help to appear in a default string) */
  if (hestSourceResponseFile == hinTop->source
      || hestSourceCommandLine == hinTop->source) {
    const hestArg *hlast;
    if (hparm->respectDashDashHelp                          // watching for "--help"
        && havec->len                                       // have at least one arg
        && (hlast = havec->harg + havec->len - 1)->finished // latest arg is finished
        && !strcmp("--help", hlast->str)) {                 // and it equals "--help"
      *helpWantedP = AIR_TRUE;
    }
  }
  if (popAtEnd) {
    if (hestInputStackPop(hist, err, hparm)) {
      ret = 1;
    }
  }
  return ret;
}

int
hestParse2(hestOpt *opt, int argc, const char **argv, char **_errP,
           const hestParm *_hparm) {
  /* see note on HMME (at top) for why me[] ends with ": " */
  static const char me[] = "hestParse2: ";

  // -------- initialize the mop
  airArray *mop = airMopNew();

  // -------- make exactly one of (given) _hparm and (our) hparm non-NULL
  hestParm *hparm = NULL;
  if (!_hparm) {
    hparm = hestParmNew();
    airMopAdd(mop, hparm, (airMopper)hestParmFree, airMopAlways);
  }
  if (HPARM->verbosity > 1) {
    printf("%shparm->verbosity %d\n", HMME, HPARM->verbosity);
  }

  // -------- allocate the err string. We do it a dumb way for now.
  // TODO: make this smarter
  uint eslen = 2 * AIR_STRLEN_HUGE;
  char *err = AIR_CALLOC(eslen + 1, char);
  assert(err);
  if (_errP) {
    // they care about the error string, so mop it only when there is _not_ an error
    *_errP = err;
    airMopAdd(mop, _errP, (airMopper)airSetNull, airMopOnOkay);
    airMopAdd(mop, err, airFree, airMopOnOkay);
  } else {
    /* otherwise, we're making the error string just for our own convenience,
       so we always clean it up on exit */
    airMopAdd(mop, err, airFree, airMopAlways);
  }
  if (HPARM->verbosity > 1) {
    printf("%serr %p\n", HMME, AIR_VOIDP(err));
  }

  // -------- check on validity of the hestOpt array
  if (_hestOptCheck(opt, err, HPARM)) {
    // error message has been sprinted into err
    airMopError(mop);
    return 1;
  }
  if (HPARM->verbosity > 1) {
    printf("%s_hestOptCheck passed\n", HMME);
  }

  // -------- allocate the state we use during parsing
  hestInputStack *hist = hestInputStackNew();
  airMopAdd(mop, hist, (airMopper)hestInputStackNix, airMopAlways);
  hestArgVec *havec = hestArgVecNew();
  airMopAdd(mop, havec, (airMopper)hestArgVecNix, airMopAlways);
  if (HPARM->verbosity > 1) {
    printf("%shavec and hist allocated\n", HMME);
  }

  // -------- initialize input stack w/ given argc,argv, then process it
  if (hestInputStackPushCommandLine(hist, argc, argv, err, HPARM)) {
    airMopError(mop);
    return 1;
  }
  do {
    /* Every iteration of this will work on one argv[] element, or, one character of a
       response file. As long as we avoid giving ourselves infinite work, eventually,
       bird by bird, we will finish.  */
    if (histProc(havec, &(opt->helpWanted), hist, err, HPARM)) {
      // error message in err
      airMopError(mop);
      return 1;
    }
    // keep going while there's something on stack and no calls for help have been seen
  } while (hist->len && !(opt->helpWanted));

  hestArgVecPrint(__func__, havec);

  airMopOkay(mop);
  return 0;
}
