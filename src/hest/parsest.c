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

/* parse, Parser, PARSEST: please let this be the final implementation of hestParse */

#include <assert.h>
#include <sys/errno.h>

/* variable name conventions:
harg  = hestArg
havec = hestArgVec
hin   = hestInput
hist  = hestInputStack
*/

static int
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

// possible *nastP values to indicate histProc_N_ext_A_rg _st_atus
enum {
  nastUnknown,  // 0: don't know
  nastEmpty,    // 1: we have no arg to give and we have given it
  nastTryAgain, // 2: inconclusive because we had to futz around
  nastBehold    // 3: have produced an arg, here it is
};
static const airEnum _nast_ae = {.name = "next-arg-status",
                                 .M = 3,
                                 .str = (const char *[]){"(unknown_status)", // 0
                                                         "empty",            // 1
                                                         "try-again",        // 2
                                                         "behold"},          // 3
                                 .val = NULL,
                                 .desc = NULL,
                                 .strEqv = NULL,
                                 .valEqv = NULL,
                                 .sense = AIR_FALSE};
static const airEnum *const nast_ae = &_nast_ae;

/*
Properly parsing a character sequence into "arguments", like the shell does to convert
the typed command-line into the argv[] array, is more than just calling strtok(), but the
old hest code only did that, which is why it had to be re-written.  Instead, handling "
vs ' quotes, character escaping, and the implicit concatenation of adjacent args,
requires a little deterministic finite automaton (DFA) to be done correctly. Here is the
POSIX info about tokenizing:
https://pubs.opengroup.org/onlinepubs/9699919799/utilities/V3_chap02.html#tag_18_03
(although hest does not do rule 5 about parameter expansion, command substitution,
or arithmetic expansion), and here are the details about quoting:
https://pubs.opengroup.org/onlinepubs/9699919799/utilities/V3_chap02.html#tag_18_02
Here is instructive example code https://github.com/nyuichi/dash.git ; in src/parser.c
see the readtoken1() function.
*/
/* the DFA states of our arg tokenizer */
enum {
  argstUnknown,  // 0: don't know
  argstStart,    // 1: skipping whitespace to start arg
  argstInside,   // 2: currently inside a token/arg
  argstSingleQ,  // 3: inside single quoting
  argstDoubleQ,  // 4: inside double quoting
  argstEscapeIn, // 5: got \ escape from within (unquoted) word
  argstEscapeDQ, // 6: Escape Dairy Queen!
  argstComment,  // 7: #-initiated comment
};
// and airEnum for this
static const airEnum _argst_ae = {.name = "tokenizer-state",
                                  .M = 6,
                                  .str = (const char *[]){"(unknown_state)", // 0
                                                          "start",           // 1
                                                          "inside",          // 2
                                                          "singleQ",         // 3
                                                          "doubleQ",         // 4
                                                          "escapeIn",        // 5
                                                          "escapeDQ",        // 6
                                                          "#comment"},       // 7
                                  .val = NULL,
                                  .desc = NULL,
                                  .strEqv = NULL,
                                  .valEqv = NULL,
                                  .sense = AIR_FALSE};
static const airEnum *const argst_ae = &_argst_ae;

/* parse an arg argument and save into `tharg`, one character `cc` at a time */
static int
argstGo(int *nastP, hestArg *tharg, int *stateP, int icc, int vrbo) {
  char cc = (char)icc;
  if (vrbo) {
    printf("%s: hello: getting %d=|%c| in state=%s\n", __func__, icc, cc,
           airEnumStr(argst_ae, *stateP));
  }
  // hitting end of input is special enough to handle separately
  if (EOF == icc) {
    int ret;
    switch (*stateP) {
    case argstStart:
    case argstComment:
      // oh well, we didn't get to start an arg
      *nastP = nastTryAgain; // input will be popped, try again with next stack element
      ret = 0;
      break;
    case argstInside:
      // the EOF ends the arg and that's ok
      *nastP = nastBehold;
      ret = 0;
      break;
    case argstSingleQ:
      biffAddf(HEST, "%s: hit input end inside single-quoted string", __func__);
      ret = 1;
      break;
    case argstDoubleQ:
      biffAddf(HEST, "%s: hit input end inside double-quoted string", __func__);
      ret = 1;
      break;
    case argstEscapeIn:
      biffAddf(HEST, "%s: hit input end after \\ escape from arg", __func__);
      ret = 1;
      break;
    case argstEscapeDQ:
      biffAddf(HEST, "%s: hit input end after \\ escape from double-quoted string",
               __func__);
      ret = 1;
      break;
    default:
      biffAddf(HEST, "%s: hit input end in unknown state %d", __func__, *stateP);
      ret = 1;
    }
    return ret;
  }
  // else not at input end, use nastUnknown as default state of "still working"
  *nastP = nastUnknown;
  switch (*stateP) {
  case argstStart:
    if (!isspace(cc)) { // if is space, we stay in argstStart
      if ('\'' == cc) {
        *stateP = argstSingleQ;
      } else if ('"' == cc) {
        *stateP = argstDoubleQ;
      } else if ('\\' == cc) {
        *stateP = argstEscapeIn;
      } else if ('#' == cc) {
        *stateP = argstComment;
      } else { // start building up new arg
        hestArgAddChar(tharg, cc);
        *stateP = argstInside;
      }
    } // if !isspace(cc)
    break;
  case argstInside:
    if (isspace(cc)) { // this is the natural end to an arg
      *nastP = nastBehold;
      *stateP = argstStart; // prepare for next arg
    } else if ('\'' == cc) {
      *stateP = argstSingleQ;
    } else if ('"' == cc) {
      *stateP = argstDoubleQ;
    } else if ('\\' == cc) {
      *stateP = argstEscapeIn;
    } else { // add to existing arg EVEN IF IT is a '#'
      hestArgAddChar(tharg, cc);
      // state stays at argstInside
    } // else !isspace(cc)
    break;
  case argstSingleQ:
    if ('\'' == cc) {
      // ending quoted string, back to arg interior
      *stateP = argstInside;
    } else {
      // add characters (including #) to arg without any other interpretation
      hestArgAddChar(tharg, cc);
    }
    break;
  case argstDoubleQ:
    if ('"' == cc) {
      // ending quoted string, back to arg interior
      *stateP = argstInside;
    } else if ('\\' == cc) {
      *stateP = argstEscapeDQ;
    } else {
      // add non-escaped characters (including #) to arg
      hestArgAddChar(tharg, cc);
    }
    break;
  case argstEscapeIn:
  case argstEscapeDQ:
    if ('\n' == cc) {
      // line continuation; ignore \ and \n
    } else {
      // add escaped characters (including #) to arg
      hestArgAddChar(tharg, cc);
    }
    // either way, back to whatever we were in pre-escape
    *stateP = (argstEscapeIn == *stateP) ? argstInside : argstDoubleQ;
    break;
  case argstComment:
    if ('\n' == cc) {
      // the newline has ended the comment, prepare for next arg
      *stateP = argstStart;
    }
    // else we skip the commented-out character
    break;
  } // switch (*stateP)
  return 0;
}

/*
histProcNextArgTry draws on whatever the top input source is, to (try to) produce
another arg. It sets in *nastP the status of the next arg production process. Allowing
nastTryAgain is a way to acknowledge that we are called iteratively by a loop we
don't control, and managing the machinery of arg production is itself a multi-step
process that doesn't always produce an arg.  We should NOT need to do look-ahead to
make progress, even if we don't produce an arg: when we see that the input source at
the top of the stack is exhausted, then we pop that source, but we shouldn't have to
then immediately test if the next input source is also exhausted (instead we say "try
again").
*/
static int
histProcNextArgTry(int *nastP, hestArg *tharg, hestInputStack *hist,
                   const hestParm *hparm) {
  if (!(nastP && tharg && hist && hparm)) {
    biffAddf(HEST, "%s: got NULL pointer (nastP %p tharg %p hist %p hparm %p)", __func__,
             AIR_VOIDP(nastP), AIR_VOIDP(tharg), AIR_VOIDP(hist), AIR_VOIDP(hparm));
    return 1;
  }
  // printf("!%s: hello hist->len %u\n", __func__, hist->len);
  hestArgReset(tharg);
  *nastP = nastUnknown;
  if (!hist->len) {
    // the stack is empty; say so
    *nastP = nastEmpty;
    return 0;
  }
  uint hinIdx = hist->len - 1;
  hestInput *hin = hist->hin + hinIdx;
  // printf("!%s: source %s\n", __func__, airEnumStr(hestSource, hin->source));
  // printf("!%s: hin(%p)->rfname = |%s|\n", __func__, AIR_VOIDP(hin), hin->rfname);
  if (hestSourceCommandLine == hin->source) {
    // argv[]  0   1    2    3   (argc=4)
    //        cmd arg1 arg2 arg3
    uint argi = hin->argIdx;
    // printf("!%s: argi %u vs argc %u\n", __func__, argi, hin->argc);
    if (argi < hin->argc) {
      // there are args left to parse
      hestArgAddString(tharg, hin->argv[argi]);
      // printf("!%s: now tharg->str=|%s|\n", __func__, tharg->str);
      *nastP = nastBehold;
      hin->argIdx++;
    } else {
      // we have gotten to the end of the given argv array, pop it as input source */
      if (histPop(hist, hparm)) {
        biffAddf(HEST, "%s: trouble popping", __func__);
        return 1;
      }
      *nastP = nastTryAgain;
    }
  } else {
    // hin->source is hestSourceResponseFile or hestSourceDefault
    int icc; // the next character we read as int
    int state = argstStart;
    do {
      // get next character `icc` from input
      if (hestSourceDefault == hin->source) {
        if (hin->carIdx < hin->dfltLen) {
          icc = hin->dfltStr[hin->carIdx];
        } else {
          icc = EOF;
        }
      } else {
        icc = fgetc(hin->rfile); // may be EOF
      }
      if (argstGo(nastP, tharg, &state, icc, hparm->verbosity > 1)) {
        if (hestSourceResponseFile == hin->source) {
          biffAddf(HEST, "%s: trouble at character %u of %s \"%s\"", __func__,
                   hin->carIdx, airEnumStr(hestSource, hin->source), hin->rfname);
        } else {
          biffAddf(HEST, "%s: trouble at character %u of %s |%s|", __func__, hin->carIdx,
                   airEnumStr(hestSource, hin->source), hin->dfltStr);
        }
        return 1;
      }
      if (EOF != icc) {
        hin->carIdx++;
      } else {
        // we're at end; pop input; *nastP already set to nastTryAgain by argstGo()
        if (histPop(hist, hparm)) {
          biffAddf(HEST, "%s: trouble popping", __func__);
          return 1;
        }
      }
    } while (nastUnknown == *nastP);
  }
  return 0;
}

static int
histProcNextArg(int *nastP, hestArg *tharg, hestInputStack *hist,
                const hestParm *hparm) {
  // printf("!%s: hello hist->len %u\n", __func__, hist->len);
  do {
    if (histProcNextArgTry(nastP, tharg, hist, hparm)) {
      biffAddf(HEST, "%s: trouble getting next arg", __func__);
      return 1;
    }
    if (hparm->verbosity > 1) {
      printf("%s: histProcNextArgSub set *nastP = %s\n", __func__,
             airEnumStr(nast_ae, *nastP));
    }
  } while (*nastP == nastTryAgain);
  return 0;
}

#if 0 // not used yet
static int
histPushDefault(hestInputStack *hist, const char *dflt, const hestParm *hparm) {
  if (!(hist && dflt && hparm)) {
    biffAddf(HEST, "%s: got NULL pointer (hist %p, dflt %p, hparm %p)", __func__,
             AIR_VOIDP(hist), AIR_VOIDP(dflt), AIR_VOIDP(hparm));
    return 1;
  }
  if (HIST_DEPTH_MAX == hist->len) {
    biffAddf(HEST, "%s: input stack depth already at max %u", __func__, HIST_DEPTH_MAX);
    return 1;
  }
  if (hparm->verbosity) {
    printf("%s: changing stack height: %u --> %u with dflt=|%s|; "
           "dfltLen %u, dfltIdx 0\n",
           __func__, hist->hinArr->len, hist->hinArr->len + 1, dflt,
           AIR_UINT(strlen(dflt)));
  }
  uint idx = airArrayLenIncr(hist->hinArr, 1);
  hist->hin[idx].source = hestSourceDefault;
  hist->hin[idx].dfltStr = dflt;
  hist->hin[idx].dfltLen = strlen(dflt);
  hist->hin[idx].carIdx = 0;
  return 0;
}
#endif

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
  /* We need our own copy of this filename for debugging and error messages;
     the rfname we were passed was probably the `str` of some tmp hestArg */
  hist->hin[idx].rfname = airStrdup(rfname);
  hist->hin[idx].rfile = rfile;
  hist->hin[idx].carIdx = 0;
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
Upon seeing a request for a response file, we push it to the input stack.  This
function never pops from the input stack (that is the responsibility of
histProcNextArg).

This function takes no ownership of anything so avoids any mopping responsibility, not
even for the tmp arg holder `tharg`; that is passed in here and cleaned up caller.
*/
static int
histProcess(hestArgVec *havec, int *helpWantedP, hestArg *tharg, hestInputStack *hist,
            const hestParm *hparm) {
  *helpWantedP = AIR_FALSE;
  int nast = nastUnknown;
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
    /* printf("!%s: (iters %u) topHin(%p)->rfname = |%s|\n", __func__, iters,
           AIR_VOIDP(topHin), topHin->rfname); */
    const char *srcstr = airEnumStr(hestSource, topHin->source);
    // read next arg into tharg
    if (histProcNextArg(&nast, tharg, hist, hparm)) {
      biffAddf(HEST, "%s: (arg %u of %s) unable to get next arg", __func__, iters,
               srcstr);
      return 1;
    }
    if (nastEmpty == nast) {
      // the stack has no more tokens to give, stop looped requests for mre
      if (hparm->verbosity) {
        printf("%s: (arg %u of %s) empty!\n", __func__, iters, srcstr);
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
                 "%s: (arg %u of %s) end comment marker \"}-\" not "
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
        printf("%s: (arg %u of %s) skipping commented-out |%s|\n", __func__, iters,
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
        biffAddf(HEST, "%s: (arg %u of %s) \"--help\" not expected here", __func__,
                 iters, srcstr);
        return 1;
      }
    }
    if (hparm->verbosity > 1) {
      printf("%s: (arg %u of %s) looking at latest tharg |%s|\n", __func__, iters,
             srcstr, tharg->str);
    }
    if (hparm->responseFileEnable && tharg->str[0] == RESPONSE_FILE_FLAG) {
      // tharg->str is asking to open a response file; try pushing it
      if (histPushResponseFile(hist, tharg->str + 1, hparm)) {
        biffAddf(HEST, "%s: (arg %u of %s) unable to process response file %s", __func__,
                 iters, srcstr, tharg->str);
        return 1;
      }
      // have just added response file to stack, next iter will read from it
      continue;
    }
    // this arg is not specially handled by us; add it to the arg vec
    hestArgVecAppendString(havec, tharg->str);
    if (hparm->verbosity > 1) {
      printf("%s: (arg %u of %s) added |%s| to havec, now len %u\n", __func__, iters,
             srcstr, tharg->str, havec->len);
    }
  }
  if (hist->len && nast == nastEmpty) {
    biffAddf(HEST, "%s: non-empty stack (depth %u) can't generate args???", __func__,
             hist->len);
    return 1;
  }
  return 0;
}

/*
hestParse(2): parse the `argc`,`argv` commandline according to the hestOpt array `opt`.

*/
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
