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

/* variable name conventions:
harg  = hestArg
havec = hestArgVec
hin   = hestInput
hist  = hestInputStack
*/

// histPopt pops one hestInput from the `hist` stack
static int
histPop(hestInputStack *hist, const hestParm *hparm) {
  if (!(hist && hparm)) {
    biffAddf(HEST, "%s%sgot NULL pointer (hist %p hparm %p)", _ME_, AIR_VOIDP(hist),
             AIR_VOIDP(hparm));
    return 1;
  }
  if (!(hist->len)) {
    biffAddf(HEST, "%s%scannot pop from input stack height 0", _ME_);
    return 1;
  }
  hestInput *topHin = hist->hin + hist->len - 1;
  if (topHin->dashBraceComment) {
    biffAddf(HEST,
             "%s%s%u start comment marker%s \"-{\" not balanced by equal later \"}-\"",
             _ME_, topHin->dashBraceComment, topHin->dashBraceComment > 1 ? "s" : "");
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

// possible *nastP values to indicate histProcNextArg status
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

/* argst* enum values
Properly parsing a character sequence into "arguments", like the shell does to convert
the typed command-line into the argv[] array, is more than just calling strtok(), but the
old hest code only did that, which is why it had to be re-written.  Instead, handling
" vs ' quotes, character escaping, and the implicit concatenation of adjacent args,
requires a little deterministic finite automaton (DFA) to be done correctly. Here is the
POSIX info about tokenizing:
https://pubs.opengroup.org/onlinepubs/9699919799/utilities/V3_chap02.html#tag_18_03
(although hest does not do rule 5 about parameter expansion, command substitution,
or arithmetic expansion), and here are the details about quoting:
https://pubs.opengroup.org/onlinepubs/9699919799/utilities/V3_chap02.html#tag_18_02
Here is instructive example code https://github.com/nyuichi/dash.git
  in src/parser.c see the readtoken1() function and the DFA there.
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

/* argstGo
Implements the DFA that tokenizes a character sequence into arguments: the thing that the
shell does for you to convert the command-line you type into an array argv[] of args.
This gets one character `icc` at a time, and a pointer to the current DFA state
`*stateP`, and builds up the output token in `tharg`.
The Next Arg STatus pointer `nastP` is used to signal when a token is finished. */
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
      biffAddf(HEST, "%s%shit input end inside single-quoted string", _MEV_(vrbo));
      ret = 1;
      break;
    case argstDoubleQ:
      biffAddf(HEST, "%s%shit input end inside double-quoted string", _MEV_(vrbo));
      ret = 1;
      break;
    case argstEscapeIn:
      biffAddf(HEST, "%s%shit input end after \\ escape from arg", _MEV_(vrbo));
      ret = 1;
      break;
    case argstEscapeDQ:
      biffAddf(HEST, "%s%shit input end after \\ escape from double-quoted string",
               _MEV_(vrbo));
      ret = 1;
      break;
    default:
      biffAddf(HEST, "%s%shit input end in unknown state %d", _MEV_(vrbo), *stateP);
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
    if ('\n' == cc) {
      // line continuation; ignore \ and \n
    } else {
      // add escaped character (including #) to arg
      hestArgAddChar(tharg, cc);
    }
    // back to unescaped input
    *stateP = argstInside;
    break;
  case argstEscapeDQ:
    if ('\n' == cc) {
      // like above: line continuation; ignore \ and \n
    } else if ('$' == cc || '\'' == cc || '\"' == cc || '\\' == cc) {
      // add escaped character to arg
      hestArgAddChar(tharg, cc);
    } else {
      // other character (needlessly) escaped, put in both \ and char
      hestArgAddChar(tharg, '\\');
      hestArgAddChar(tharg, cc);
    }
    // back to unescaped input
    *stateP = argstDoubleQ;
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

/* histProcNextArgTry
Draws on whatever the top input source is, to (try to) produce another arg. It sets in
*nastP the status of the next arg production process. Allowing nastTryAgain is a way to
acknowledge that we are called iteratively by a loop we don't control, and managing the
machinery of arg production is itself a multi-step process that doesn't always produce an
arg.  We should NOT need to do look-ahead to make progress, even if we don't produce an
arg: when we see that the input source at the top of the stack is exhausted, then we pop
that source, but we shouldn't have to then immediately test if the next input source is
also exhausted (instead we say "try again").
*/
static int
histProcNextArgTry(int *nastP, hestArg *tharg, hestInputStack *hist,
                   const hestParm *hparm) {
  if (!(nastP && tharg && hist && hparm)) {
    biffAddf(HEST, "%s%sgot NULL pointer (nastP %p tharg %p hist %p hparm %p)", _ME_,
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
      hestArgSetString(tharg, hin->argv[argi]);
      // printf("!%s: now tharg->str=|%s|\n", __func__, tharg->str);
      *nastP = nastBehold;
      hin->argIdx++;
    } else {
      // we have gotten to the end of the given argv array, pop it as input source */
      if (histPop(hist, hparm)) {
        biffAddf(HEST, "%s%strouble popping %s", _ME_,
                 airEnumStr(hestSource, hestSourceCommandLine));
        return 1;
      }
      *nastP = nastTryAgain;
    }
  } else if (hestSourceResponseFile == hin->source || hestSourceDefault == hin->source) {
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
      } else if (hestSourceResponseFile == hin->source) {
        icc = fgetc(hin->rfile); // may be EOF
      } else {
        biffAddf(HEST, "%s%sconfused by input source %d", _ME_, hin->source);
        return 1;
      }
      if (argstGo(nastP, tharg, &state, icc, hparm->verbosity > 3)) {
        if (hestSourceResponseFile == hin->source) {
          biffAddf(HEST, "%s%strouble at character %u of %s \"%s\"", _ME_, hin->carIdx,
                   airEnumStr(hestSource, hin->source), hin->rfname);
        } else {
          biffAddf(HEST, "%s%strouble at character %u of %s |%s|", _ME_, hin->carIdx,
                   airEnumStr(hestSource, hin->source), hin->dfltStr);
        }
        return 1;
      }
      if (EOF != icc) {
        hin->carIdx++;
      } else {
        // we're at end; pop input; *nastP already set to nastTryAgain by argstGo()
        if (histPop(hist, hparm)) {
          if (hestSourceResponseFile == hin->source) {
            biffAddf(HEST, "%s%strouble popping %s \"%s\"", _ME_,
                     airEnumStr(hestSource, hin->source), hin->rfname);
          } else {
            biffAddf(HEST, "%s%strouble popping %s |%s|", _ME_,
                     airEnumStr(hestSource, hin->source), hin->dfltStr);
          }
          return 1;
        }
      }
    } while (nastUnknown == *nastP);
  } else {
    biffAddf(HEST, "%s%sconfused about hin->source %d", _ME_, hin->source);
    return 1;
  }
  return 0;
}

// histProcNextArg wraps around histProcNextArgTry, hiding occurances of nastTryAgain
static int
histProcNextArg(int *nastP, hestArg *tharg, hestInputStack *hist,
                const hestParm *hparm) {
  // printf("!%s: hello hist->len %u\n", __func__, hist->len);
  do {
    if (histProcNextArgTry(nastP, tharg, hist, hparm)) {
      biffAddf(HEST, "%s%strouble getting next arg", _ME_);
      return 1;
    }
    if (hparm->verbosity > 1) {
      printf("%s: histProcNextArgSub set *nastP = %s\n", __func__,
             airEnumStr(nast_ae, *nastP));
    }
  } while (*nastP == nastTryAgain);
  return 0;
}

static int
histPushCommandLine(hestInputStack *hist, int argc, const char **argv,
                    const hestParm *hparm) {
  if (!(hist && argv && hparm)) {
    biffAddf(HEST, "%s%sgot NULL pointer (hist %p, argv %p, hparm %p)", _ME_,
             AIR_VOIDP(hist), AIR_VOIDP(argv), AIR_VOIDP(hparm));
    return 1;
  }
  if (HIST_DEPTH_MAX == hist->len) {
    biffAddf(HEST, "%s%sinput stack depth already at max %u", _ME_, HIST_DEPTH_MAX);
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
    biffAddf(HEST, "%s%sgot NULL pointer (hist %p, rfname %p, hparm %p)", _ME_,
             AIR_VOIDP(hist), AIR_VOIDP(rfname), AIR_VOIDP(hparm));
    return 1;
  }
  if (HIST_DEPTH_MAX == hist->len) {
    // HEY test this error
    biffAddf(HEST, "%s%sinput stack depth already at max %u", _ME_, HIST_DEPTH_MAX);
    return 1;
  }
  if (!strlen(rfname)) {
    // HEY test this error
    biffAddf(HEST,
             "%s%ssaw arg start with response file flag \"%c\" "
             "but no filename followed",
             _ME_, RESPONSE_FILE_FLAG);
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
                 "%s%salready reading \"%s\" as response file; "
                 "cannot recursively read it again",
                 _ME_, rfname);
        return 1;
      }
    }
  }
  // are we trying to read stdin twice?
  if (!strcmp("-", rfname) && hist->stdinRead) {
    // HEY test this error
    biffAddf(HEST, "%s%sresponse filename \"%s\" but previously read stdin", _ME_,
             rfname);
    return 1;
  }
  // try to open response file
  FILE *rfile = airFopen(rfname, stdin, "r");
  if (!(rfile)) {
    biffAddf(HEST, "%s%scouldn't fopen(\"%s\",\"r\"): %s", _ME_, rfname,
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

static int
histPushDefault(hestInputStack *hist, const char *dflt, const hestParm *hparm) {
  if (!(hist && dflt && hparm)) {
    biffAddf(HEST, "%s%sgot NULL pointer (hist %p, dflt %p, hparm %p)", _ME_,
             AIR_VOIDP(hist), AIR_VOIDP(dflt), AIR_VOIDP(hparm));
    return 1;
  }
  if (HIST_DEPTH_MAX == hist->len) {
    biffAddf(HEST, "%s%sinput stack depth already at max %u", _ME_, HIST_DEPTH_MAX);
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

/* histProcess
Consumes args (tokens) from the stack `hist`, mostly just copying them into `havec`,
but this does interpret the tokens just enough to implement:
   (what)                     (allowed sources)
 - commenting with -{ , }-    all (even a default string, may regret this)
 - ask for --help             command-line
 - response files             command-line, response file (not default string)
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
  if (!hist->len) {
    biffAddf(HEST, "%s%scannot process zero-height stack", _ME_);
    return 1;
  }
  if (helpWantedP) *helpWantedP = AIR_FALSE;
  int nast = nastUnknown;
  uint iters = 0;
  // printf("!%s: hello hist->len %u\n", __func__, hist->len);
  // initialize destination havec
  airArrayLenSet(havec->hargArr, 0);
  /* We `return` directly from this loop ONLY when we MUST stop processing the stack,
     because of an error, or because of user asking for help.
     Otherwise, we loop again. */
  while (1) {
    iters += 1;
    // learn ways to describe current input source
    hestInput *topHin = hist->hin + hist->len - 1;
    int srcval = topHin->source;
    const char *srcstr = airEnumStr(hestSource, srcval);
    // read next arg into tharg
    if (histProcNextArg(&nast, tharg, hist, hparm)) {
      biffAddf(HEST, "%s%s(iter %u, on %s) unable to get next arg", _ME_, iters, srcstr);
      return 1;
    }
    if (nastEmpty == nast) {
      // the stack has no more tokens to give, stop looped requests for more
      if (hparm->verbosity) {
        printf("%s: (iter %u, on %s) empty!\n", __func__, iters, srcstr);
      }
      break;
    }
    // annoyingly, we may get here with an empty stack (HEY fix this?)
    topHin = (hist->len //
                ? hist->hin + hist->len - 1
                : NULL);
    // printf("!%s: nast = %s, |stack| = %u, topHin = %p\n", __func__,
    //        airEnumStr(nast_ae, nast), hist->len, AIR_VOIDP(topHin));
    //  we have a token, is it turning off commenting?
    if (hparm->respectDashBraceComments && !strcmp("}-", tharg->str)) {
      if (!topHin) {
        biffAddf(HEST, "%s%s(iter %u, on %s) unexpected empty stack (0)", _ME_, iters,
                 srcstr);
        return 1;
      }
      if (topHin->dashBraceComment) {
        topHin->dashBraceComment -= 1;
        if (hparm->verbosity) {
          printf("%s: topHin->dashBraceComment now %u\n", __func__,
                 topHin->dashBraceComment);
        }
        continue; // since }- does not belong in havec
      } else {
        biffAddf(HEST,
                 "%s%s(iter %u, on %s) end comment marker \"}-\" not "
                 "balanced by prior \"-{\"",
                 _ME_, iters, srcstr);
        return 1;
      }
    }
    // not ending comment, are we starting (or deepening) one?
    if (hparm->respectDashBraceComments && !strcmp("-{", tharg->str)) {
      if (!topHin) {
        biffAddf(HEST, "%s%s(iter %u, on %s) unexpected empty stack (1)", _ME_, iters,
                 srcstr);
        return 1;
      }
      topHin->dashBraceComment += 1;
      if (hparm->verbosity) {
        printf("%s: topHin->dashBraceComment now %u\n", __func__,
               topHin->dashBraceComment);
      }
      continue;
    }
    // if in comment, move along
    if (topHin && topHin->dashBraceComment) {
      if (hparm->verbosity > 1) {
        printf("%s: (iter %u, on %s) skipping commented-out |%s|\n", __func__, iters,
               srcstr, tharg->str);
      }
      continue;
    }
    // else this arg is not in a comment and is not related to commenting
    if (hparm->respectDashDashHelp && !strcmp("--help", tharg->str)) {
      if (!topHin) {
        biffAddf(HEST, "%s%s(iter %u, on %s) unexpected empty stack (2)", _ME_, iters,
                 srcstr);
        return 1;
      }
      if (hestSourceCommandLine == topHin->source) {
        /* user asking for help halts further parsing work: user is not looking
           for parsing results nor error messages about that process */
        if (!helpWantedP) {
          biffAddf(HEST, "%s%s(iter %u, on %s) saw \"--help\" but have NULL helpWantedP",
                   _ME_, iters, srcstr);
          return 1;
        }
        *helpWantedP = AIR_TRUE;
        return 0;
      } else {
        biffAddf(HEST, "%s%s(iter %u, on %s) \"--help\" not handled in this source",
                 _ME_, iters, srcstr);
        return 1;
      }
    }
    if (hparm->verbosity > 2) {
      printf("%s: (iter %u, on %s) looking at latest tharg |%s|\n", __func__, iters,
             srcstr, tharg->str);
    }
    if (hparm->responseFileEnable && tharg->str[0] == RESPONSE_FILE_FLAG) {
      if (!topHin) {
        biffAddf(HEST, "%s%s(iter %u, on %s) unexpected empty stack (3)", _ME_, iters,
                 srcstr);
        return 1;
      }
      if (hestSourceDefault == topHin->source) {
        biffAddf(HEST,
                 "%s%s(iter %u, on %s) %s response files not handled in this source",
                 _ME_, iters, srcstr, tharg->str);
        return 1;
      } else {
        // tharg->str is asking to open a response file; try pushing it
        if (histPushResponseFile(hist, tharg->str + 1, hparm)) {
          biffAddf(HEST, "%s%s(iter %u, on %s) unable to process response file %s", _ME_,
                   iters, srcstr, tharg->str);
          return 1;
        }
        // have just added response file to stack, next iter will start reading from it
        continue;
      }
    }
    // this arg is not specially handled by us; add it to the arg vec
    hestArgVecAppendString(havec, tharg->str);
    if (hparm->verbosity > 1) {
      printf("%s: (iter %u, on %s) added |%s| to havec, now len %u\n", __func__, iters,
             srcstr, tharg->str, havec->len);
    }
    // set source in the hestArg we just appended
    havec->harg[havec->len - 1]->source = srcval;
    // bail if stack is empty
    if (!topHin) {
      break;
    }
  }
  if (hist->len && nast == nastEmpty) {
    biffAddf(HEST, "%s%snon-empty stack (depth %u) can't generate args???", _ME_,
             hist->len);
    return 1;
  }
  return 0;
}

/* whichOptFlag(): for which option (by index) is this the flag?

Given an arg string `flarg` (which may be an flag arg like "-size" or parm arg like
"512"), this finds which one, of the options in the given hestOpt array `opt` is
identified by `flarg`. Returns the index of the matching option, if there is a match.

If there is no match, returns UINT_MAX.
*/
static uint
whichOptFlag(const hestOpt *opt, const char *flarg, const hestParm *hparm) {
  uint optNum = opt->arrLen;
  if (hparm->verbosity > 3)
    printf("%s: looking for maybe-is-flag |%s| in optNum=%u options\n", __func__, flarg,
           optNum);
  for (uint optIdx = 0; optIdx < optNum; optIdx++) {
    if (hparm->verbosity > 3)
      printf("%s:      optIdx %u |%s| ?\n", __func__, optIdx,
             opt[optIdx].flag ? opt[optIdx].flag : "(nullflag)");
    const char *optFlag = opt[optIdx].flag;
    if (!optFlag) continue; // it can't be for this unflagged option
    if (strchr(optFlag, MULTI_FLAG_SEP)) {
      // look for both long and short versions
      char *buff = AIR_CALLOC(strlen("--") + strlen(flarg) + strlen(optFlag) + 1, char);
      char *ofboth = airStrdup(optFlag);
      char *sep = strchr(ofboth, MULTI_FLAG_SEP);
      *sep = '\0'; // break short and long into separate strings
      /* first try the short version */
      sprintf(buff, "-%s", ofboth);
      if (!strcmp(flarg, buff)) return (free(buff), free(ofboth), optIdx);
      /* else try the long version */
      sprintf(buff, "--%s", sep + 1);
      if (!strcmp(flarg, buff)) return (free(buff), free(ofboth), optIdx);
      free(buff);
      free(ofboth);
    } else {
      /* flag only comes in short version */
      char *buff = AIR_CALLOC(strlen("-") + strlen(optFlag) + 1, char);
      sprintf(buff, "-%s", optFlag);
      if (!strcmp(flarg, buff)) return (free(buff), optIdx);
      free(buff);
    }
  }
  if (hparm->verbosity > 3) printf("%s: no match, returning UINT_MAX\n", __func__);
  return UINT_MAX;
}

/* identStr sprints into `ident` (and returns same `ident`)
   a way to identify `opt` in error and usage messages */
static char *
identStr(char *ident, const hestOpt *opt) {
  if (opt->flag) {
    if (strchr(opt->flag, MULTI_FLAG_SEP)) {
      char *fcopy = airStrdup(opt->flag);
      char *sep = strchr(fcopy, MULTI_FLAG_SEP);
      *sep = '\0';
      sprintf(ident, "\"-%s%c--%s\" option", fcopy, MULTI_FLAG_SEP, sep + 1);
      free(fcopy);
    } else {
      sprintf(ident, "\"-%s\" option", opt->flag);
    }
  } else {
    sprintf(ident, "\"<%s>\" option", opt->name);
  }
  return ident;
}

/* havecTransfer
(if `num`) moves `num` args from `hvsrc` (starting at `srcIdx`) to `opt->havec`
This takes `hestOpt *opt` instead of `opt->havec` so that we can also take this
time to set `opt->source` according to the incoming `hvsrc->harg[]->source`. To
minimize cleverness, we set `opt->source` with every transferred argument, which
means that the per-option source remembered is the source of the *last* argument of the
option.
*/
static int
havecTransfer(hestOpt *opt, hestArgVec *hvsrc, uint srcIdx, uint num,
              const hestParm *hparm) {
  if (!(opt && hvsrc)) {
    biffAddf(HEST, "%s%sgot NULL opt %p or hvsrc %p", _ME_, AIR_VOIDP(opt),
             AIR_VOIDP(hvsrc));
    return 1;
  }
  if (num) {
    if (!(srcIdx < hvsrc->len)) {
      biffAddf(HEST, "%s%sstarting index %u in source beyond its length %u", _ME_,
               srcIdx, hvsrc->len);
      return 1;
    }
    if (!(srcIdx + num <= hvsrc->len)) {
      biffAddf(HEST, "%s%shave only %u args but want %u starting at %u", _ME_,
               hvsrc->len, num, srcIdx);
      return 1;
    }
    // okay now do the work, starting with empty destination havec
    hestArgVecReset(opt->havec);
    for (uint ai = 0; ai < num; ai++) {
      hestArg *harg = hestArgVecRemove(hvsrc, srcIdx);
      hestArgVecAppendArg(opt->havec, harg);
      opt->source = harg->source;
    }
  }
  return 0;
}

static void
optPrint(const hestOpt *opt, uint opi) {
  printf("--- opt %u:", opi);
  printf("\t%s%s", opt->flag ? "flag-" : "", opt->flag ? opt->flag : "UNflag");
  printf("\tname|%s|\t k%d (%u)--(%d) \t%s \tdflt|%s|\n",
         opt->name ? opt->name : "(null)", opt->kind, opt->min, opt->max,
         _hestTypeStr[opt->type], opt->dflt ? opt->dflt : "(null)");
  printf("    source %s\n", airEnumStr(hestSource, opt->source));
  hestArgVecPrint("", "    havec:", opt->havec);
  return;
}

static void
optAllPrint(const char *func, const char *ctx, const hestOpt *optall) {
  printf("%s: %s:\n", func, ctx);
  printf("%s: v.v.v.v.v.v.v.v.v hestOpt %p has %u options (allocated for %u):\n", func,
         AIR_VOIDP(optall), optall->arrLen, optall->arrAlloc);
  for (uint opi = 0; opi < optall->arrLen; opi++) {
    optPrint(optall + opi, opi);
  }
  printf("%s: ^'^'^'^'^'^'^'^'^\n", func);
  return;
}

/* havecExtractFlagged
Extracts the parameter args associated with all flagged options from the given
`hestArgVec *havec` (as generated by histProc()) and stores them the corresponding
opt->havec. Also sets opt->source according to where that flag arg appeared (we don't
notice if the parameters were weirdly split between the comamnd-line and a response file;
only the source of the flag arg is recorded).

In the case of variadic parameter options, this does the work of figuring out which args
belong with the option. In any case, this only extracts and preserves (in opt->havec) the
parameter args, not the flag arg that identified which option was being set.

As a result of this work, the passed `havec` is shortened: all args associated with
flagged opts are removed, so that later work can extract args for unflagged opts.

For variadic parameter options, the sawP information is not set here, since it is better
set at the final value parsing time, which happens after defaults are enstated.

This is where, thanks to the action of whichOptFlag(), "--" (and only "--" due to
VAR_PARM_STOP_FLAG) is used as a marker for the end of a flagged variadic parameter
option.  AND, the "--" marker is removed from `havec`.
*/
static int
havecExtractFlagged(hestOpt *opt, hestArgVec *havec, const hestParm *hparm) {
  char ident1[AIR_STRLEN_HUGE + 1], ident2[AIR_STRLEN_HUGE + 1];
  uint argIdx = 0;
  hestOpt *theOpt = NULL;
  while (argIdx < havec->len) { // NOTE: havec->len may decrease within an interation!
    if (hparm->verbosity > 1) {
      printf("%s: ------------- argIdx = %u (of %u) -> argv[argIdx] = |%s|\n", __func__,
             argIdx, havec->len, havec->harg[argIdx]->str);
    }
    uint optIdx = whichOptFlag(opt, havec->harg[argIdx]->str, hparm);
    if (UINT_MAX == optIdx) {
      // havec->harg[argIdx]->str is not a flag for any option, move on to next arg
      if (hparm->verbosity > 2) {
        printf("%s: |%s| not a flag arg, continue\n", __func__,
               havec->harg[argIdx]->str);
      }
      argIdx++;
      continue;
    }
    // else havec->harg[argIdx]->str is a flag for option with index optIdx aka theOpt
    theOpt = opt + optIdx;
    if (hparm->verbosity)
      printf("%s: argv[%u]=|%s| is flag of opt %u \"%s\"\n", __func__, argIdx,
             havec->harg[argIdx]->str, optIdx, theOpt->flag);
    /* see if we can associate some parameters with the flag */
    if (hparm->verbosity) printf("%s: any associated parms?\n", __func__);
    int hitEnd = AIR_FALSE;
    int varParm = (5 == opt[optIdx].kind);
    const char VPS[3] = {'-', VAR_PARM_STOP_FLAG, '\0'};
    int hitVPS = AIR_FALSE;
    uint nextOptIdx = 0, // what is index of option who's flag we hit next
      parmNum = 0,       // how many parm args have we counted up
      pai;               // tmp parm arg index
    while (              // parmNum is plausible # parms
      AIR_INT(parmNum) < _hestMax(theOpt->max)
      // and looking ahead by parmNum still gives us a valid index pai
      && !(hitEnd = !((pai = argIdx + 1 + parmNum) < havec->len))
      // and either this isn't a variadic parm opt
      && (!varParm || // or, it is a varparm opt, and we aren't looking at "--"
          !(hitVPS = !strcmp(VPS, havec->harg[pai]->str)))
      && UINT_MAX // and we aren't looking at start of another flagged option
           == (nextOptIdx = whichOptFlag(opt, havec->harg[pai]->str, hparm))) {
      if (hparm->verbosity)
        printf("%s: optIdx %d |%s|; argIdx %u < %u |%s| --> parmNum --> %d\n", __func__,
               optIdx, theOpt->flag, argIdx, pai, havec->harg[pai]->str, parmNum + 1);
      parmNum++;
    }
    /* we stopped because we got the max number of parameters, or
       we hitEnd, or
       varParm and we hitVPS, or
       we hit the start of another flagged option (indicated by nextOptIdx) */
    if (hparm->verbosity)
      printf("%s: optIdx %d |%s|: stopped w/ "
             "parmNum=%u hitEnd=%d hitVPS=%d nextOptIdx=%u\n",
             __func__, optIdx, theOpt->flag, parmNum, hitEnd, hitVPS, nextOptIdx);
    if (parmNum < theOpt->min) { // didn't get required min # parameters
      if (hitEnd) {
        biffAddf(HEST,
                 "%s%shit end of args before getting %u parameter%s "
                 "for %s (got %u)",
                 _ME_, theOpt->min, theOpt->min > 1 ? "s" : "", identStr(ident1, theOpt),
                 parmNum);
      } else if (hitVPS) {
        biffAddf(HEST,
                 "%s%shit \"-%c\" (variadic-parameter-stop flag) before getting %u "
                 "parameter%s for %s (got %u)",
                 _ME_, VAR_PARM_STOP_FLAG, theOpt->min, theOpt->min > 1 ? "s" : "",
                 identStr(ident1, theOpt), parmNum);
      } else if (UINT_MAX != nextOptIdx) {
        biffAddf(HEST, "%s%ssaw %s before getting %u parameter%s for %s (got %d)", _ME_,
                 identStr(ident2, opt + nextOptIdx), theOpt->min,
                 theOpt->min > 1 ? "s" : "", identStr(ident1, theOpt), parmNum);
      } else {
        biffAddf(HEST,
                 "%s%ssorry, confused about not getting %u "
                 "parameter%s for %s (got %d)",
                 _ME_, theOpt->min, theOpt->min > 1 ? "s" : "", identStr(ident1, theOpt),
                 parmNum);
      }
      return 1;
    }
    if (hparm->verbosity) {
      printf("%s: ________ argv[%d]=|%s|: optIdx %u |%s| followed by %u parms\n",
             __func__, argIdx, havec->harg[argIdx]->str, optIdx, theOpt->flag, parmNum);
    }
    if (hparm->verbosity > 1) {
      hestArgVecPrint(__func__, "main havec as it came", havec);
    }
    // remember from whence this flagged option came
    theOpt->source = havec->harg[argIdx]->source;
    // lose the flag argument
    hestArgNix(hestArgVecRemove(havec, argIdx));
    if (havecTransfer(theOpt, havec, argIdx, parmNum, hparm)) {
      biffAddf(HEST, "%s%strouble transferring %u args for %s", _ME_, parmNum,
               identStr(ident1, theOpt));
      return 1;
    }
    if (hitVPS) {
      // drop the variadic-parameter-stop flag
      hestArgNix(hestArgVecRemove(havec, argIdx));
    }
    if (hparm->verbosity) {
      char info[AIR_STRLEN_HUGE + 1];
      sprintf(info, "main havec after extracting optIdx %u |%s| and %u parms", optIdx,
              theOpt->flag, parmNum);
      hestArgVecPrint(__func__, info, havec);
      sprintf(info, "optIdx %u |%s|'s own havec", optIdx, theOpt->flag);
      hestArgVecPrint(__func__, info, theOpt->havec);
    }
    // do NOT increment argIdx
  }

  /* make sure that flagged options without default were given */
  uint optNum = opt->arrLen;
  for (uint opi = 0; opi < optNum; opi++) {
    theOpt = opt + opi;
    if (theOpt->flag) { // this is a flagged option we should have handled above
      int needing = (1 != theOpt->kind    // this kind of option can take a parm
                     && !(theOpt->dflt)); // and this option has no default
      if (hparm->verbosity > 1) {
        printf("%s: flagged opt %u |%s| source = %s%s\n", __func__, opi, theOpt->flag,
               airEnumStr(hestSource, theOpt->source),
               needing ? " <-- w/ parm but w/out default" : "");
      }
      // if needs to be set but hasn't been
      if (needing && hestSourceUnknown == theOpt->source) {
        biffAddf(HEST, "%s%sdidn't see required %s", _ME_, identStr(ident1, theOpt));
        return 1;
      }
    }
  }

  if (hparm->verbosity) {
    optAllPrint(__func__, "end of havecExtractFlagged", opt);
    hestArgVecPrint(__func__, "end of havecExtractFlagged", havec);
  }
  return 0;
}

/* havecExtractUnflagged()

extracts the parameter args associated with all unflagged options (of `hestOpt *opt`)
from the given `hestArgVec *havec` and (like havecExtractFlagged) extracts those args and
saves them in the corresponding opt[].havec

This is the function that has to handle the trickly logic of allowing there to be
multiple unflagged options, only one of which may have a variadic number of parms; that
one has to be extracted last.
*/
static int
havecExtractUnflagged(hestOpt *opt, hestArgVec *havec, const hestParm *hparm) {
  char ident[AIR_STRLEN_HUGE + 1];
  uint optNum = opt->arrLen; // number of options (flagged or unflagged)
  uint ufOptNum = 0;         // number of unflagged options
  for (uint opi = 0; opi < optNum; opi++) {
    if (!opt[opi].flag) {
      ufOptNum += 1;
    }
  }
  /* simplify indexing into unflagged options, forward and backward,
   with a little Nx2 array ufOpi2 of option indices */
  uint *ufOpi2 = NULL;
  if (!ufOptNum) {
    /* no unflagged options; we're ~done */
    goto finishingup;
  }
  ufOpi2 = AIR_CALLOC(2 * ufOptNum, uint);
  assert(ufOpi2);
  uint upii = 0; // index into ufOpi2
  for (uint opi = 0; opi < optNum; opi++) {
    if (!opt[opi].flag) {
      ufOpi2[2 * upii + 0] = opi;
      upii++;
    }
  }
  for (upii = 0; upii < ufOptNum; upii++) {
    // fill in backward side
    ufOpi2[2 * upii + 1] = ufOpi2[2 * (ufOptNum - 1 - upii) + 0];
  }
  if (hparm->verbosity) {
    printf("%s: ufOpi2 helper array:\n up:", __func__);
    for (upii = 0; upii < ufOptNum; upii++) {
      printf(" \t%u", ufOpi2[2 * upii + 0]);
    }
    printf("\n down:");
    for (upii = 0; upii < ufOptNum; upii++) {
      printf(" \t%u", ufOpi2[2 * upii + 1]);
    }
    printf("\n");
  }
  uint ufVarOpi = optNum; // index (if < optNum) of the unflagged variadic parm option
  for (upii = 0; upii < ufOptNum; upii++) {
    uint opi = ufOpi2[2 * upii + 0];
    if (5 == opt[opi].kind) { // (unflagged) multiple variadic parm
      ufVarOpi = opi;
      break;
    }
  }
  /* now, if there is an unflagged variadic option (NOTE that _hestOPCheck()
     ensured that there is at most one of these), then ufVarOpi is its index in opt[].
     If there is no unflagged variadic option, ufVarOpi is optNum. */
  if (hparm->verbosity) {
    printf("%s: ufVarOpi = %u %s\n", __func__, ufVarOpi,
           (ufVarOpi == optNum ? "==> there is no unflagged variadic opt"
                               : "is index of single unflagged variadic opt"));
  }

  // grab parameters for all unflagged opts before opt[ufVarOpi]
  for (upii = 0; upii < ufOptNum; upii++) {
    uint opi = ufOpi2[2 * upii + 0]; // 0: increasing index direction
    if (opi == ufVarOpi) {
      break;
    }
    if (hparm->verbosity) {
      printf("%s: looking at opi = %u kind %d\n", __func__, opi, opt[opi].kind);
    }
    /* Either we're not using the defaults because we know we have enough args,
    else we know we do not have enough args and yet we also don't have a default.
    Either way, we try extracting the args; in the later case just to generate a
    descriptive error message about the situation */
    if (opt[opi].min /* == max */ < havec->len || !opt[opi].dflt) {
      if (havecTransfer(opt + opi, havec, 0, opt[opi].min, hparm)) {
        biffAddf(HEST, "%s%strouble getting args for %sunflagged %s[%u]", _ME_,
                 !opt[opi].dflt ? "default-less " : "", identStr(ident, opt + opi), opi);
        return (free(ufOpi2), 1);
      }
    }
  }
  if (ufVarOpi == optNum) {
    // if there is no unflagged multiple variadic option, we're done-ish
    goto finishingup;
  }
  /* else we do have an unflagged multiple variadic option, so we work down to it
   from other end of arg vec */
  // HEY COPY-PASTA
  for (upii = 0; upii < ufOptNum; upii++) {
    uint opi = ufOpi2[2 * upii + 1]; // 1: decreasing index direction
    if (opi == ufVarOpi) {
      break;
    }
    if (hparm->verbosity) {
      printf("%s: looking at (later) opi = %u kind %d\n", __func__, opi, opt[opi].kind);
    }
    // same logic as above
    if (opt[opi].min /* == max */ < havec->len || !opt[opi].dflt) {
      uint idx0 = (opt[opi].min < havec->len     //
                     ? havec->len - opt[opi].min //
                     : 0);
      if (havecTransfer(opt + opi, havec, idx0, opt[opi].min, hparm)) {
        biffAddf(HEST, "%s%strouble getting args for (later) %sunflagged %s[%u]", _ME_,
                 !opt[opi].dflt ? "default-less " : "", identStr(ident, opt + opi), opi);
        return (free(ufOpi2), 1);
      }
    }
  }

  /* now, finally, we grab the parameters of the sole variadic parameter unflagged opt;
     the one with index ufVarOpi < optNum (and we're only here because it exists) */
  if (hparm->verbosity) {
    printf("%s: ufVarOpi=%u   min, have, max = %u %u %d\n", __func__, ufVarOpi,
           opt[ufVarOpi].min, havec->len, _hestMax(opt[ufVarOpi].max));
  }
  uint minArg = opt[ufVarOpi].min; /* min < max ! */
  if (minArg > havec->len && !opt[ufVarOpi].dflt) {
    biffAddf(HEST,
             "%s%shave only %u args left but need %u for "
             "(default-less) variadic unflagged %s[%u]",
             _ME_, havec->len, minArg, identStr(ident, opt + ufVarOpi), ufVarOpi);
    return (free(ufOpi2), 1);
  }
  // else minArg <= havec->len, or, minArg > havec->len and do have default
  if (minArg <= havec->len) {
    // can satisfy option from havec, no need to use default
    uint getArg = havec->len;      // want to grab as many args as possible
    if (-1 != opt[ufVarOpi].max) { // but no more than needed
      getArg = AIR_MIN(getArg, AIR_UINT(opt[ufVarOpi].max));
    }
    if (havecTransfer(opt + ufVarOpi, havec, 0, getArg, hparm)) {
      biffAddf(HEST, "%s%strouble getting args for variadic unflagged %s[%u]", _ME_,
               identStr(ident, opt + ufVarOpi), ufVarOpi);
      return (free(ufOpi2), 1);
    }
  }
  // else minArg > havec->len so can't satisfy from havec,
  // but its ok since we do have default

  /* make sure that unflagged options without default were given */
  for (upii = 0; upii < ufOptNum; upii++) {
    uint opi = ufOpi2[2 * upii + 0];
    if (!(opt[opi].dflt) && hestSourceUnknown == opt[opi].source) {
      biffAddf(HEST, "%s%sdidn't see required (unflagged) %s[%u]", _ME_,
               identStr(ident, opt + opi), opi);
      return (free(ufOpi2), 1);
    }
  }

finishingup:
  if (hparm->verbosity) {
    optAllPrint(__func__, "end of havecExtractUnflagged", opt);
    hestArgVecPrint(__func__, "end of havecExtractUnflagged", havec);
  }
  if (havec->len) {
    biffAddf(HEST,
             "%s%safter getting %u unflagged opts, have %u unexpected arg%s "
             "%s\"%s\"",
             _ME_, ufOptNum, havec->len, havec->len > 1 ? "s," : "",
             havec->len > 1 ? "starting with " : "", havec->harg[0]->str);
    return (airFree(ufOpi2), 1);
  }
  return (airFree(ufOpi2), 0);
}

/* optProcessDefaults
All the command-line arguments (and any response files invoked therein) should now be
processed (by transferring the arguments to per-option opt->havec arrays), but we need to
ensure that every option has information from which to set values. The per-option
opt->dflt string is what we look to now, to finish setting per-option opt->havec arrays
for all the options for which opt->havec have not already been set.  We use
`!opt->source` (aka hestSourceUnknown) as the indicator of not already being set.
*/
static int
optProcessDefaults(hestOpt *opt, hestArg *tharg, hestInputStack *hist,
                   const hestParm *hparm) {
  uint optNum = opt->arrLen;
  for (uint opi = 0; opi < optNum; opi++) {
    if (hparm->verbosity) {
      printf(" -> %s incoming", __func__);
      optPrint(opt + opi, opi);
    }
    if (opt[opi].source) {
      /* the source is already set (to something other than hestSourceUnknown),
         so there's no need for using the default */
      continue;
    }
    opt[opi].source = hestSourceDefault;
    if (1 == opt[opi].kind) {
      /* There is no meaningful "default" for stand-alone flags (and in fact
         opt[opi].dflt is enforced to be NULL) so there is no default string to tokenize,
         but we above set source to default for sake of completeness, and to signal that
         the flag was not given by user */
      goto nextopt;
    }
    char ident[AIR_STRLEN_HUGE + 1];
    identStr(ident, opt + opi);
    // should have already checked for this but just to make sure
    if (!opt[opi].dflt) {
      biffAddf(HEST, "%s%s %s[%u] needs default string but it is NULL", _ME_, ident,
               opi);
      return 1;
    }
    /* in some circumstances the default may be empty "", even if non-NULL, which means
    that no args will be put into opt[opi].havec, and that's okay, but that's part of why
    we set the source above to hestSourceDefault, so that we'd know the source even if it
    isn't apparent in any of the (non-existant) args. */
    if (hparm->verbosity) {
      printf("%s: looking at %s[%u] default string |%s|\n", __func__, ident, opi,
             opt[opi].dflt);
    }
    if (histPushDefault(hist, opt[opi].dflt, hparm)
        || histProcess(opt[opi].havec, NULL, tharg, hist, hparm)) {
      biffAddf(HEST, "%s%sproblem tokenizing %s[%u] default string", _ME_, ident, opi);
      return 1;
    }
    /* havecExtractFlagged and havecExtractUnflagged have done the work of ensuring that
    the minimum number of parm args have been extracted for each option. We have to do
    something analogous for args tokenized from the default strings. */
    if (opt[opi].havec->len < opt[opi].min) {
      biffAddf(
        HEST, "%s%s %s[%u] default string \"%s\" supplied %u arg%s but need at least %u",
        _ME_, ident, opi, opt[opi].dflt, opt[opi].havec->len,
        opt[opi].havec->len > 1 ? "s" : "", opt[opi].min);
      return 1;
    }
  nextopt:
    if (hparm->verbosity) {
      printf("<-  %s: outgoing", __func__);
      optPrint(opt + opi, opi);
    }
  }
  return 0;
}

/* hestParse2
Parse the `argc`,`argv` commandline according to the hestOpt array `opt`, and as
tweaked by settings in (if non-NULL) the given `hestParm *_hparm`.  If there is an
error, an error message string describing it in detail is generated and
 - if errP: *errP is set to the newly allocated error message string
   NOTE: it is the caller's responsibility to free() it later
 - if !errP: the error message is fprintf'ed to stderr

The basic phases of parsing are:

0) Error checking on given `opt` array

1) Generate internal representation of command-line that includes expanding any response
files; this all goes into the `hestArgVec *havec`.

2) From `havec`, extract the args that are attributable to flagged and unflagged options,
moving each `hestArg` out of main `havec` and into the per-hestOpt opt->havec

3) For options not user-supplied, process the opt's `dflt` string to set opt->havec

4) Now, every option should have a opt->havec set, regardless of where it came from. So
parse those per-opt args to set final values for the user to see

What is allocated as result of work here should be freed by hestParseFree
*/
int
hestParse2(hestOpt *opt, int argc, const char **argv, char **errP,
           const hestParm *_hparm) {
  airArray *mop = airMopNew(); // initialize the mop

  // make exactly one of (given) _hparm and (our) hparm non-NULL
  hestParm *hparm = NULL;
  if (!_hparm) {
    hparm = hestParmNew();
    airMopAdd(mop, hparm, (airMopper)hestParmFree, airMopAlways);
  }
  // how to const-correctly use hparm or _hparm in an expression
#define HPARM (_hparm ? _hparm : hparm)
  if (HPARM->verbosity > 1) {
    printf("%s: (%s) hparm->verbosity %d\n", __func__, _hparm ? "given" : "default",
           HPARM->verbosity);
  }

  // error string song and dance
#define DO_ERR(WUT)                                                                     \
  biffAddf(HEST, "%s%s" WUT, _MEV_(HPARM->verbosity));                                  \
  char *err = biffGetDone(HEST);                                                        \
  if (errP) {                                                                           \
    *errP = err;                                                                        \
  } else {                                                                              \
    fprintf(stderr, "%s: problem:\n%s", __func__, err);                                 \
    free(err);                                                                          \
  }

  // --0--0--0--0--0-- check on validity of the hestOpt array
  if (_hestOPCheck(opt, HPARM)) {
    DO_ERR("problem with given hestOpt array");
    airMopError(mop);
    return 1;
  }
  if (HPARM->verbosity > 1) {
    printf("%s: _hestOPCheck passed\n", __func__);
  }

  // allocate the state we use during parsing
  hestInputStack *hist = hestInputStackNew();
  airMopAdd(mop, hist, (airMopper)hestInputStackNix, airMopAlways);
  hestArgVec *havec = hestArgVecNew();
  airMopAdd(mop, havec, (airMopper)hestArgVecNix, airMopAlways);
  hestArg *tharg = hestArgNew(); // tmp hestArg
  airMopAdd(mop, tharg, (airMopper)hestArgNix, airMopAlways);
  if (HPARM->verbosity > 1) {
    printf("%s: parsing state allocated\n", __func__);
  }

  // --1--1--1--1--1-- initialize input stack w/ given argc,argv, process it
  if (histPushCommandLine(hist, argc, argv, HPARM)
      || histProcess(havec, &(opt->helpWanted), tharg, hist, HPARM)) {
    DO_ERR("problem with initial processing of command-line");
    airMopError(mop);
    return 1;
  }
  if (HPARM->verbosity > 1) {
    // have finished input stack, what argvec did it leave us with?
    hestArgVecPrint(__func__, "after histProcess", havec);
  }
  if (opt->helpWanted) {
    // once the call for help is made, we respect it: clean up and return
    airMopOkay(mop);
    return 0;
  }

  // --2--2--2--2--2-- extract args associated with flagged and unflagged opt
  if (havecExtractFlagged(opt, havec, HPARM)
      // this will detect extraneous args after unflagged opts are extracted
      || havecExtractUnflagged(opt, havec, HPARM)) {
    DO_ERR("problem extracting args for options");
    airMopError(mop);
    return 1;
  }

  /* --3--3--3--3--3-- process defaults strings for opts that weren't user-supplied
  Like havecExtract{,Un}Flagged, this builds up opt->havec, but does not parse it */
  if (optProcessDefaults(opt, tharg, hist, HPARM)) {
    DO_ERR("problem with processing defaults");
    airMopError(mop);
    return 1;
  }

#if 0
  // --4--4--4--4--4-- Finally, parse the args and set values
  if (optSetValues) {
    DO_ERR("problem with setting values");
    airMopError(mop);
    return 1;
  }
#endif
  airMopOkay(mop);
  return 0;
}
