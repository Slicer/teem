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

/* parse, Parser, PARSEST: may this be the final implementation of hestParse */

#include <assert.h>
#include <sys/errno.h>

int
histPop(hestInputStack *hist, const hestParm *hparm) {
  if (!(hist && hparm)) {
    biffAddf(HEST, "%s: got NULL pointer (hist %p hparm %p)", __func__, AIR_VOIDP(hist),
             AIR_VOIDP(hparm));
    return 1;
  }
  if (!(hist->len)) {
    biffAddf(HEST, "%s: cannot pop from input stack height 0", __func__);
    return 1;
  }
  hestInput *topHin = hist->hin + hist->len - 1;
  if (topHin->dashBraceComment) {
    biffAddf(HEST,
             "%s: %u start comment marker%s \"-{\" not balanced by equal later \"}-\"",
             __func__, topHin->dashBraceComment,
             topHin->dashBraceComment > 1 ? "s" : "");
    return 1;
  }
  if (hparm->verbosity) {
    printf("%s: changing stack height: %u --> %u; popping %s source\n", __func__,
           hist->len, hist->len - 1,
           airEnumStr(hestSource, hist->hin[hist->len - 1].source));
  }
  airArrayLenIncr(hist->hinArr, -1);
  return 0;
}

// possible *statP values set by histProcNextArg
enum {
  procStatUnknown,  // 0: don't know
  procStatEmpty,    // 1: we have no arg to give
  procStatTryAgain, // 2: inconclusive because had to manage things
  procStatBehold    // 3: have produced an arg, here it is
};

/*
histProcNextArgSub draws on whatever the top input source is, to produce another arg.
It sets in *statP the status of the arg production process. Allowing procStatTryAgain
is a way to acknowledge the fact that we are called iteratively by a loop we don't
control, and managing the machinery of arg production is itself a multi-step process that
doesn't always produce an arg. We should need to do look-ahead to make progress, even if
we don't produce an arg.
*/
static int
histProcNextArgSub(int *statP, hestArg *tharg, hestInputStack *hist,
                   const hestParm *hparm) {
  if (!(statP && tharg && hist && hparm)) {
    biffAddf(HEST, "%s: got NULL pointer (statP %p tharg %p hist %p hparm %p)", __func__,
             AIR_VOIDP(statP), AIR_VOIDP(tharg), AIR_VOIDP(hist), AIR_VOIDP(hparm));
    return 1;
  }
  // printf("!%s: hello hist->len %u\n", __func__, hist->len);
  hestArgReset(tharg);
  *statP = procStatUnknown;
  if (!hist->len) {
    // the stack is empty; say so
    *statP = procStatEmpty;
    return 0;
  }
  uint hinIdx = hist->len - 1;
  hestInput *hin = hist->hin + hinIdx;
  // printf("!%s: source %s\n", __func__, airEnumStr(hestSource, hin->source));
  if (hestSourceCommandLine == hin->source) {
    // argv[]  0   1    2    3   (argc=4)
    //        cmd arg1 arg2 arg3
    uint argi = hin->argIdx;
    // printf("!%s: argi %u vs argc %u\n", __func__, argi, hin->argc);
    if (argi < hin->argc) {
      // there are args left to parse
      hestArgAddString(tharg, hin->argv[argi]);
      // printf("!%s: now tharg->str=|%s|\n", __func__, tharg->str);
      *statP = procStatBehold;
      hin->argIdx++;
    } else {
      // we have gotten to the end of the given argv array, pop it */
      if (histPop(hist, hparm)) {
        biffAddf(HEST, "%s: trouble popping", __func__);
        return 1;
      }
      *statP = procStatTryAgain;
    }
  } else {
    // source is default string or response file
    biffAddf(HEST, "%s: source %s not yet implemented", __func__,
             airEnumStr(hestSource, hin->source));
    return 1;
  }
  return 0;
}

static int
histProcNextArg(int *statP, hestArg *tharg, hestInputStack *hist,
                const hestParm *hparm) {
  // printf("!%s: hello hist->len %u\n", __func__, hist->len);
  do {
    if (histProcNextArgSub(statP, tharg, hist, hparm)) {
      biffAddf(HEST, "%s: trouble getting next arg", __func__);
      return 1;
    }
    if (hparm->verbosity > 1) {
      printf("%s: histProcNextArgSub set *statP = %d\n", __func__, *statP);
    }
  } while (*statP == procStatTryAgain);
  return 0;
}

static int
histPushCommandLine(hestInputStack *hist, int argc, const char **argv,
                    const hestParm *hparm) {
  if (!(hist && argv && hparm)) {
    biffAddf(HEST, "%s: got NULL pointer (hist %p, argv %p, hparm %p)", __func__,
             AIR_VOIDP(hist), AIR_VOIDP(argv), AIR_VOIDP(hparm));
    return 1;
  }
  if (HIST_DEPTH_MAX == hist->len) {
    biffAddf(HEST, "%s: input stack depth already at max %u", __func__, HIST_DEPTH_MAX);
    return 1;
  }
  if (hparm->verbosity) {
    printf("%s: changing stack height: %u --> %u with argc=%d,argv=%p; "
           "setting argIdx to 0\n",
           __func__, hist->hinArr->len, hist->hinArr->len + 1, argc, AIR_VOIDP(argv));
  }
  uint idx = airArrayLenIncr(hist->hinArr, 1);
  if (hparm->verbosity > 1) {
    printf("%s: new hinTop = %p\n", __func__, AIR_VOIDP(hist->hin + idx));
  }
  hist->hin[idx].source = hestSourceCommandLine;
  hist->hin[idx].argc = argc;
  hist->hin[idx].argv = argv;
  hist->hin[idx].argIdx = 0;
  return 0;
}

static int
histPushResponseFile(hestInputStack *hist, const char *rfname, const hestParm *hparm) {
  if (!(hist && rfname && hparm)) {
    biffAddf(HEST, "%s: got NULL pointer (hist %p, rfname %p, hparm %p)", __func__,
             AIR_VOIDP(hist), AIR_VOIDP(rfname), AIR_VOIDP(hparm));
    return 1;
  }
  if (HIST_DEPTH_MAX == hist->len) {
    // HEY test this error
    biffAddf(HEST, "%s: input stack depth already at max %u", __func__, HIST_DEPTH_MAX);
    return 1;
  }
  if (!strlen(rfname)) {
    // HEY test this error
    biffAddf(HEST,
             "%s: saw arg start with response file flag \"%c\" "
             "but no filename followed",
             __func__, RESPONSE_FILE_FLAG);
    return 1;
  }
  // have we seen rfname before?
  if (hist->len) {
    uint topHinIdx = hist->len - 1;
    for (uint hidx = 0; hidx < topHinIdx; hidx++) {
      hestInput *oldHin = hist->hin + hidx;
      if (hestSourceResponseFile == oldHin->source //
          && !strcmp(oldHin->rfname, rfname)) {
        // HEY test this error
        biffAddf(HEST,
                 "%s: already reading \"%s\" as response file; "
                 "cannot recursively read it again",
                 __func__, rfname);
        return 1;
      }
    }
  }
  // are we trying to read stdin twice?
  if (!strcmp("-", rfname) && hist->stdinRead) {
    // HEY test this error
    biffAddf(HEST, "%s: response filename \"%s\" but previously read stdin", __func__,
             rfname);
    return 1;
  }
  // try to open response file
  FILE *rfile = airFopen(rfname, stdin, "r");
  if (!(rfile)) {
    biffAddf(HEST, "%s: couldn't fopen(\"%s\",\"r\"): %s", __func__, rfname,
             strerror(errno));
    return 1;
  }
  // okay, we actually opened the response file; put it on the stack
  uint idx = airArrayLenIncr(hist->hinArr, 1);
  if (hparm->verbosity > 1) {
    printf("%s: (hist depth %u) new hinTop = %p\n", __func__, hist->len,
           AIR_VOIDP(hist->hin + idx));
  }
  hist->hin[idx].source = hestSourceResponseFile;
  hist->hin[idx].rfname = rfname;
  hist->hin[idx].rfile = rfile;
  return 0;
}

/*
histProcess consumes args (tokens) from the stack `hist`, mostly just copying
them into `havec`, but this does interpret the tokens just enough to implement:
   (what)                     (allowed sources)
 - commenting with -{ , }-    all (even a default string, may regret this)
 - ask for --help             command-line
 - response files             command-line, response file
since these are the things that histProcNextArg does not understand (it just produces
finished tokens). On the other hand, we do NOT know anything about individual hestOpts
and their flags, which is why they weren't passed to us (--help is special).
Upon seeing a request for a response file, we push it to the input stack.  This function
never pops from the input stack (that is the responsibility of histProcNextArg).

This function takes no ownership of anything so avoids any mopping responsibility, not
even for the tmp arg holder `tharg`; that is passed in here and cleaned up caller.
*/
static int
histProcess(hestArgVec *havec, int *helpWantedP, hestArg *tharg, hestInputStack *hist,
            const hestParm *hparm) {
  *helpWantedP = AIR_FALSE;
  int stat = procStatUnknown;
  uint iters = 0;
  hestInput *topHin;
  // printf("!%s: hello hist->len %u\n", __func__, hist->len);
  /* We `return` directly from this loop ONLY when we MUST stop processing the stack,
     because of an error, or because of user asking for help.
     Otherwise, we loop again. */
  while (1) {
    iters += 1;
    /* if this loop just pushed a response file, the top hestInput is different
       from what it was when this function started, so re-learn it. */
    topHin = hist->hin + hist->len - 1;
    const char *srcstr = airEnumStr(hestSource, topHin->source);
    // read next arg into tharg
    if (histProcNextArg(&stat, tharg, hist, hparm)) {
      biffAddf(HEST, "%s: (arg %u src %s) unable to get next arg", __func__, iters,
               srcstr);
      return 1;
    }
    if (procStatEmpty == stat) {
      // the stack has no more tokens to give, stop looped requests for mre
      if (hparm->verbosity) {
        printf("%s: (arg %u src %s) empty!\n", __func__, iters, srcstr);
      }
      break;
    }
    // we have a token, is it turning off commenting?
    if (hparm->respectDashBraceComments && !strcmp("}-", tharg->str)) {
      if (topHin->dashBraceComment) {
        topHin->dashBraceComment -= 1;
        if (hparm->verbosity) {
          printf("%s: topHin->dashBraceComment now %u\n", __func__,
                 topHin->dashBraceComment);
        }
        continue; // since }- does not belong in havec
      } else {
        biffAddf(HEST,
                 "%s: (arg %u src %s) end comment marker \"}-\" not "
                 "balanced by prior \"-{\"",
                 __func__, iters, srcstr);
        return 1;
      }
    }
    // not ending comment, are we starting (or deepening) one?
    if (hparm->respectDashBraceComments && !strcmp("-{", tharg->str)) {
      topHin->dashBraceComment += 1;
      if (hparm->verbosity) {
        printf("%s: topHin->dashBraceComment now %u\n", __func__,
               topHin->dashBraceComment);
      }
      continue;
    }
    // if in comment, move along
    if (topHin->dashBraceComment) {
      if (hparm->verbosity > 1) {
        printf("%s: (arg %u src %s) skipping commented-out |%s|\n", __func__, iters,
               srcstr, tharg->str);
      }
      continue;
    }
    // else this arg is not in a comment and is not related to commenting
    if (hparm->respectDashDashHelp && !strcmp("--help", tharg->str)) {
      if (hestSourceCommandLine == topHin->source) {
        *helpWantedP = AIR_TRUE;
        /* user asking for help halts further parsing work: user is not looking
           for parsing results nor error messages about that process */
        return 0;
      } else {
        biffAddf(HEST, "%s: (arg %u src %s) \"--help\" not expected here", __func__,
                 iters, srcstr);
        return 1;
      }
    }
    if (hparm->verbosity > 1) {
      printf("%s: (arg %u src %s) looking at latest tharg |%s|\n", __func__, iters,
             srcstr, tharg->str);
    }
    if (hparm->responseFileEnable && tharg->str[0] == RESPONSE_FILE_FLAG) {
      // tharg->str is asking to open a response file; try pushing it
      if (histPushResponseFile(hist, tharg->str + 1, hparm)) {
        biffAddf(HEST, "%s: (arg %u src %s) unable to process response file %s",
                 __func__, iters, srcstr, tharg->str);
        return 1;
      }
      // have just added response file to stack, next iter will read from it
      continue;
    }
    // this arg is not specially handled by us; add it to the arg vec
    hestArgVecAppendString(havec, tharg->str);
    if (hparm->verbosity > 1) {
      printf("%s: (arg %u src %s) added |%s| to havec, now len %u\n", __func__, iters,
             srcstr, tharg->str, havec->len);
    }
  }
  if (hist->len && stat == procStatEmpty) {
    biffAddf(HEST, "%s: non-empty stack (depth %u) can't generate args???", __func__,
             hist->len);
    return 1;
  }
  return 0;
}

int
hestParse2(hestOpt *opt, int argc, const char **argv, char **_errP,
           const hestParm *_hparm) {

  /* how to const-correctly use hparm or _hparm in an expression */
#define HPARM (_hparm ? _hparm : hparm)

  // -------- initialize the mop
  airArray *mop = airMopNew();

  // -------- make exactly one of (given) _hparm and (our) hparm non-NULL
  hestParm *hparm = NULL;
  if (!_hparm) {
    hparm = hestParmNew();
    airMopAdd(mop, hparm, (airMopper)hestParmFree, airMopAlways);
  }
  if (HPARM->verbosity > 1) {
    printf("%s: hparm->verbosity %d\n", __func__, HPARM->verbosity);
  }

  // -------- allocate the err string. We do it a dumb way for now.
  // TODO: make this allocation smarter
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
    printf("%s: err %p\n", __func__, AIR_VOIDP(err));
  }

  // -------- check on validity of the hestOpt array
  if (_hestOptCheck(opt, err, HPARM)) {
    // error message has been sprinted into err
    airMopError(mop);
    return 1;
  }
  if (HPARM->verbosity > 1) {
    printf("%s: _hestOptCheck passed\n", __func__);
  }

  // -------- allocate the state we use during parsing
  hestInputStack *hist = hestInputStackNew();
  airMopAdd(mop, hist, (airMopper)hestInputStackNix, airMopAlways);
  hestArgVec *havec = hestArgVecNew();
  airMopAdd(mop, havec, (airMopper)hestArgVecNix, airMopAlways);
  hestArg *tharg = hestArgNew(); // tmp hestArg
  airMopAdd(mop, tharg, (airMopper)hestArgNix, airMopAlways);
  if (HPARM->verbosity > 1) {
    printf("%s: parsing state allocated\n", __func__);
  }

  // -------- initialize input stack w/ given argc,argv, then process it
  if (histPushCommandLine(hist, argc, argv, HPARM)
      || histProcess(havec, &(opt->helpWanted), tharg, hist, HPARM)) {
    char *bferr = biffGetDone(HEST);
    airMopAdd(mop, bferr, airFree, airMopAlways);
    strcpy(err, bferr);
    airMopError(mop);
    return 1;
  }

  // (debugging) have finished input stack, what argvec did it leave us with?
  hestArgVecPrint(__func__, havec);

  if (opt->helpWanted) {
    // once the call for help is made, we respect it: clean up and return
    airMopOkay(mop);
    return 0;
  }

  // ( extract, process: make little argvec for each opt )
  // extract given flagged options
  // extract given unflagged options
  // process default strings of not-given options
  // set value(s) from per-opt argvec

  airMopOkay(mop);
  return 0;
}
