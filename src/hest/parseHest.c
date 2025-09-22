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
  You should have received a copy of the GNU Lesser General Public License
  along with this library; if not, see <https://www.gnu.org/licenses/>.
*/

#include "hest.h"
#include "privateHest.h"

#include <string.h>

/* A little trickery for error reporting.  For many of the functions here, if they hit an
error and hparm->verbosity is set, then we should reveal the current function name (set
by convention in `me`). But without verbosity, we hide that function name, so it appears
that the error is coming from the caller (probably identified as argv[0]).  However, that
means that functions using this `ME` macro should (in defiance of convention) set to `me`
to `functionname: ` (NOTE the `: `) so that all of that goes away without verbosity. And,
that means that error message generation here should also defy convention and instead of
being "%s: what happened" it should just be "%swhat happened" */
#define ME ((hparm && hparm->verbosity) ? me : "")

/*
argsInResponseFiles()

returns the number of "args" (i.e. the number of AIR_WHITESPACE-separated strings) that
will be parsed from the response files. The role of this function is solely to simplify
the task of avoiding memory leaks.  By knowing exactly how many args we'll get in the
response file, then hestParse() can allocate its local argv[] for exactly as long as it
needs to be, and we can avoid using an airArray.  The drawback is that we open and read
through the response files twice.  Alas.

NOTE: Currently, this makes no effort to interpreted quoted strings "like this one" as a
single arg, and we do not do any VTAB-separating of args here.
*/
static int
argsInResponseFiles(int *argsNumP, int *respFileNumP, const char **argv,
                    const hestParm *hparm) {
  FILE *file;
  static const char me[] = "argsInResponseFiles: ";
  char line[AIR_STRLEN_HUGE + 1], *pound;
  int argIdx, len;

  *argsNumP = 0;
  *respFileNumP = 0;
  if (!hparm->responseFileEnable) {
    /* don't do response files; we're done */
    return 0;
  }

  argIdx = 0;
  while (argv /* can be NULL for testing */ && argv[argIdx]) {
    if (RESPONSE_FILE_FLAG == argv[argIdx][0]) {
      /* argv[argIdx] looks like its naming a response file */
      /* NOTE: despite the repeated temptation: "-" aka stdin cannot be a response file,
         because it is going to be read in twice: once by argsInResponseFiles, and then
         again by copyArgv */
      if (!(file = fopen(argv[argIdx] + 1, "rb"))) {
        /* can't open the indicated response file for reading */
        fprintf(stderr, "%scouldn't open \"%s\" for reading as response file", ME,
                argv[argIdx] + 1);
        *argsNumP = 0;
        *respFileNumP = 0;
        return 1;
      }
      /* read first line, and start looping over lines */
      len = airOneLine(file, line, AIR_STRLEN_HUGE + 1);
      while (len > 0) {
        /* first # (or #-alike char) is turned into line end */
        if ((pound = strchr(line, RESPONSE_FILE_COMMENT))) {
          *pound = '\0';
        }
        /* count words in line */
        airOneLinify(line);
        *argsNumP += airStrntok(line, AIR_WHITESPACE);
        /* read next line for next iter */
        len = airOneLine(file, line, AIR_STRLEN_HUGE + 1);
      }
      fclose(file); /* ok because file != stdin, see above */
      (*respFileNumP)++;
    }
    argIdx++;
  }
  return 0;
}

/* prints (for debugging) the given non-const argv array */
static void
printArgv(int argc, char **argv, const char *pfx) {
  int ai;

  printf("%sargc=%d : ", pfx ? pfx : "", argc);
  for (ai = 0; ai < argc; ai++) {
    printf("%s%s ", pfx ? pfx : "", argv[ai]);
  }
  printf("%s\n", pfx ? pfx : "");
}

/*
copyArgv()

Copies given oldArgv to newArgv, including (if they are enabled) injecting the contents
of response files.  Returns the number of AIR_WHITESPACE-separated arguments in newArgv.
Allocations of the strings in newArgv are remembered (to be airFree'd later) in the given
pmop. Returns the number of args set in newArgv. BUT: upon seeing "--help" if
parm->respectDashDashHelp, this sets *sawHelp=AIR_TRUE, and then finishes early.

NOTE: like argsInResponseFiles, this is totally naive about quoted strings that appear in
response files! So "multiple words like this" would be processed as four args, the first
one starting with '"', and the last one ending with '"'. The same naivety means that the
'#' character is considered to mark the beginning of a comment, EVEN IF THAT '#' is
inside a string.  Sorry.  (This is why hest parsing is being re-written ...)

For a brief moment in 2023, this also stopped if it saw "--", but that meant "--" is a
brick wall that hestParse could never see past. But that misunderstands the relationship
between how hestParse works and how the world uses "--".  According to POSIX guidelines:
https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/V1_chap12.html#tag_12_01
the elements of argv can be first "options" and then "operands", where "options" are
indicated by something starting with '-', and may have 0 or more "option-arguments".
Then, according to Guideline 10:
    The first -- argument that is not an option-argument should be accepted as a
    delimiter indicating the end of options. Any following arguments should be treated
    as operands, even if they begin with the '-' character.
So "--" marks the end of some "option-arguments".

But hestParse does not know or care about "operands": *every* element of the given argv
will be interpreted as the argument to some option, including an unflagged option (a
variable unflagged option is how hest would support something like "cksum *.txt").  For
hest to implement the expected behavior for "--", hest has to care about "--" only in the
context of collecting parameters to *flagged* options. But copyArgv() is upstream of that
awareness (of flagged vs unflagged), so we do not act on "--" here.

Note that there are lots of ways that hest does NOT conform to these POSIX guidelines
(such as: currently single-character flags cannot be grouped together, and options can
have their arguments be optional), but those guidelines are used here to help documenting
what "--" should mean.
*/
static int
copyArgv(int *sawHelp, char **newArgv, const char **oldArgv, const hestParm *hparm,
         airArray *pmop) {
  static const char me[] = "copyArgv";
  char line[AIR_STRLEN_HUGE + 1], *pound;
  unsigned int len, newArgc, oldArgc, incr, argIdx;
  FILE *file;

  /* count number of given ("old") args */
  oldArgc = 0;
  while (oldArgv /* might be NULL for testing */ && oldArgv[oldArgc]) {
    oldArgc++;
  }

  if (hparm->verbosity > 1) {
    printf("%s: hello, oldArgc (number of input args) = %u:\n", me, oldArgc);
    for (argIdx = 0; argIdx < oldArgc; argIdx++) {
      printf("    oldArgv[%u] == |%s|\n", argIdx, oldArgv[argIdx]);
    }
  }

  newArgc = 0;
  *sawHelp = AIR_FALSE;
  for (argIdx = 0; argIdx < oldArgc; argIdx++) {
    if (hparm->respectDashDashHelp && !strcmp("--help", oldArgv[argIdx])) {
      *sawHelp = AIR_TRUE;
      break;
    }
    /* else not a show-stopper argument */
    if (hparm->verbosity) {
      printf("%s:________ newArgc = %u, argIdx = %u -> \"%s\"\n", me, newArgc, argIdx,
             oldArgv[argIdx]);
      printArgv(newArgc, newArgv, "     ");
    }
    if (!hparm->responseFileEnable || RESPONSE_FILE_FLAG != oldArgv[argIdx][0]) {
      /* either ignoring response files, or its not a response file:
      we copy the arg, remember to free it, and increment the new arg idx */
      newArgv[newArgc] = airStrdup(oldArgv[argIdx]);
      airMopAdd(pmop, newArgv[newArgc], airFree, airMopAlways);
      newArgc += 1;
    } else {
      /* It is a response file.  Error checking on open-ability
         should have been done by argsInResponseFiles() */
      file = fopen(oldArgv[argIdx] + 1, "rb");
      /* start line-reading loop */
      len = airOneLine(file, line, AIR_STRLEN_HUGE + 1);
      while (len > 0) {
        unsigned rgi;
        if (hparm->verbosity) printf("%s: line: |%s|\n", me, line);
        /* HEY HEY too bad for you if you put # inside a string */
        if ((pound = strchr(line, RESPONSE_FILE_COMMENT))) *pound = '\0';
        if (hparm->verbosity) printf("%s: -0-> line: |%s|\n", me, line);
        airOneLinify(line);
        incr = airStrntok(line, AIR_WHITESPACE);
        if (hparm->verbosity) printf("%s: -1-> line: |%s|, incr=%d\n", me, line, incr);
        airParseStrS(newArgv + newArgc, line, AIR_WHITESPACE,
                     incr /*, greedy=AIR_FALSE */);
        for (rgi = 0; rgi < incr; rgi++) {
          /* This time, we did allocate memory.  We can use airFree and
             not airFreeP because these will not be reset before mopping */
          airMopAdd(pmop, newArgv[newArgc + rgi], airFree, airMopAlways);
        }
        len = airOneLine(file, line, AIR_STRLEN_HUGE + 1);
        newArgc += incr;
      }
      fclose(file);
    }
    if (hparm->verbosity) {
      printArgv(newArgc, newArgv, "     ");
      printf("%s: ^^^^^^^ newArgc = %d, argIdx = %d\n", me, newArgc, argIdx);
    }
  }
  /* NULL-terminate newArgv[] */
  newArgv[newArgc] = NULL;

  return newArgc;
}

/* uint _hestErrStrlen(const hestOpt *opt, int argc, const char **argv) ...
 * This was a bad idea, so has been removed for TeemV2. Now hest internally uses biff
 */

/*
identStr()
copies into ident a string for identifying an option in error and usage messages
*/
static char *
identStr(char *ident, const hestOpt *opt, const hestParm *hparm, int brief) {
  char copy[AIR_STRLEN_HUGE + 1], *sep;
  AIR_UNUSED(hparm);
  if (opt->flag && (sep = strchr(opt->flag, MULTI_FLAG_SEP))) {
    strcpy(copy, opt->flag);
    sep = strchr(copy, MULTI_FLAG_SEP);
    *sep = '\0';
    if (brief)
      sprintf(ident, "-%s%c--%s option", copy, MULTI_FLAG_SEP, sep + 1);
    else
      sprintf(ident, "-%s option", copy);
  } else {
    sprintf(ident, "%s%s%s option", opt->flag ? "\"-" : "<",
            opt->flag ? opt->flag : opt->name, opt->flag ? "\"" : ">");
  }
  return ident;
}

/*
whichOptFlag()

given a string in "flag" (with the hypen prefix) finds which of the options in the given
array of options has the matching flag. Returns the index of the matching option, or -1
if there is no match, but returns -2 if the flag is the end-of-parameters marker "--"
(and only "--", due to VAR_PARM_STOP_FLAG)
*/
static int
whichOptFlag(const hestOpt *opt, const char *flag, const hestParm *hparm) {
  static const char me[] = "whichOptFlag";
  char buff[2 * AIR_STRLEN_HUGE + 1], copy[AIR_STRLEN_HUGE + 1], *sep;
  int optIdx, optNum;

  optNum = hestOptNum(opt);
  if (hparm->verbosity)
    printf("%s: (a) looking for maybe-is-flag |%s| in optNum=%d options\n", me, flag,
           optNum);
  for (optIdx = 0; optIdx < optNum; optIdx++) {
    if (hparm->verbosity)
      printf("%s:      optIdx %d |%s| ?\n", me, optIdx,
             opt[optIdx].flag ? opt[optIdx].flag : "(nullflag)");
    if (!opt[optIdx].flag) continue;
    if (strchr(opt[optIdx].flag, MULTI_FLAG_SEP)) {
      strcpy(copy, opt[optIdx].flag);
      sep = strchr(copy, MULTI_FLAG_SEP);
      *sep = '\0';
      /* first try the short version */
      sprintf(buff, "-%s", copy);
      if (!strcmp(flag, buff)) return optIdx;
      /* then try the long version */
      sprintf(buff, "--%s", sep + 1);
      if (!strcmp(flag, buff)) return optIdx;
    } else {
      /* flag has only the short version */
      sprintf(buff, "-%s", opt[optIdx].flag);
      if (!strcmp(flag, buff)) return optIdx;
    }
  }
  if (hparm->verbosity) printf("%s: (b) optNum = %d\n", me, optNum);
  if (VAR_PARM_STOP_FLAG) {
    sprintf(buff, "-%c", VAR_PARM_STOP_FLAG);
    if (hparm->verbosity)
      printf("%s: does maybe-is-flag |%s| == -VAR_PARM_STOP_FLAG |%s| ?\n", me, flag,
             buff);
    if (!strcmp(flag, buff)) {
      if (hparm->verbosity) printf("%s: yes, it does! returning -2\n", me);
      return -2;
    }
  }
  if (hparm->verbosity) printf("%s: (c) returning -1\n", me);
  return -1;
}

/*
extractToStr: from the given *argcP,argv description of args, starting at arg index
`base`, takes *UP TO* `pnum` parameters out of argv, and puts them into a new
VTAB-separated qstring WHICH THIS FUNCTION ALLOCATES, and accordingly decreases `*argcP`.

***** This is the function where VTAB starts being used.

The number of parameters so extracted is stored in `*pnumGot` (if `pnumGot` non-NULL).
`*pnumGot` can be less than `pnum` if we hit "--" (or the equivalent)
*/
static char *
extractToStr(int *argcP, char **argv, unsigned int base, unsigned int pnum,
             unsigned int *pnumGot, const hestParm *hparm) {
  unsigned int len, pidx, true_pnum;
  char *ret;
  char stops[3] = "";

  if (!pnum) return NULL;

  if (hparm) {
    stops[0] = '-';
    stops[1] = VAR_PARM_STOP_FLAG;
    stops[2] = '\0';
  } /* else stops stays as empty string */

  /* find length of buffer we'll have to allocate */
  len = 0;
  for (pidx = 0; pidx < pnum; pidx++) {
    if (base + pidx == AIR_UINT(*argcP)) {
      /* ran up against end of argv array; game over */
      return NULL;
    }
    if (!strcmp(argv[base + pidx], stops)) {
      /* saw something like "--", so that's the end */
      break;
    }
    /* increment by strlen of current arg */
    len += AIR_UINT(strlen(argv[base + pidx]));
    /* and then increment by 2, for 2 '"'s around quoted parm */
    if (strstr(argv[base + pidx], " ")) {
      len += 2;
    }
  }
  if (pnumGot) *pnumGot = pidx;
  true_pnum = pidx;
  len += true_pnum + 1; /* for spaces between args, and final '\0' */
  ret = AIR_CALLOC(len, char);
  strcpy(ret, "");
  for (pidx = 0; pidx < true_pnum; pidx++) {
    if (strstr(argv[base + pidx], " ")) {
      /* if a single element of argv has spaces in it, someone went to the trouble of
       putting it in quotes or escaping the space, and we perpetuate the favor by quoting
       it when we concatenate all the argv elements together, so that airParseStrS will
       recover it as a single string again */
      strcat(ret, "\"");
    }
    /* HEY: if there is a '\"' character in this string, quoted or
       not, its going to totally confuse later parsing */
    strcat(ret, argv[base + pidx]);
    if (strstr(argv[base + pidx], " ")) {
      strcat(ret, "\"");
    }
    /* add space prior to anticipated next parm
     HEY this needs to be re-written to extend an argv */
    if (pidx < true_pnum - 1) strcat(ret, " ");
  }
  /* shuffle down later argv pointers */
  for (pidx = base + true_pnum; pidx <= AIR_UINT(*argcP); pidx++) {
    argv[pidx - true_pnum] = argv[pidx];
  }
  *argcP -= true_pnum;
  return ret;
}

/*
extractFlagged()

extracts the parameters associated with all flagged options from the given *argcP and
argv (wherein the flag and subsequent parms were all separate strings), and here stores
them in optParms[] as a single VTAB-separated string (VTABs originating in extractToStr),
recording the number of parameters in optParmNum[], and whether or not the flagged option
appeared in optAprd[].

The sawP information is not set here, since it is better set at value parsing time, which
happens after defaults are enstated.

This is where, thanks to the action of whichOptFlag(), "--" (and only "--" due to
VAR_PARM_STOP_FLAG) is used as a marker for the end of a *flagged* variable parameter
option.  AND, the "--" marker is removed from the argv.
*/
static int
extractFlagged(char **optParms, unsigned int *optParmNum, int *optAprd, int *argcP,
               char **argv, hestOpt *opt, const hestParm *hparm, airArray *pmop) {
  /* see note on ME (at top) for why me[] ends with ": " */
  static const char me[] = "extractFlagged: ";
  char ident1[AIR_STRLEN_HUGE + 1], ident2[AIR_STRLEN_HUGE + 1];
  int argIdx, parmNum, optIdx, nextOptIdx, optNum, op;

  if (hparm->verbosity) printf("%s: *argcP = %d\n", me, *argcP);
  argIdx = 0;
  while (argIdx <= *argcP - 1) {
    if (hparm->verbosity) {
      printf("%s----------------- argIdx = %d -> argv[argIdx] = %s\n", me, argIdx,
             argv[argIdx]);
    }
    optIdx = whichOptFlag(opt, argv[argIdx], hparm);
    if (hparm->verbosity)
      printf("%sA: argv[%d]=|%s| is flag of opt %d\n", me, argIdx, argv[argIdx], optIdx);
    if (!(0 <= optIdx)) {
      /* not a flag, move on */
      argIdx++;
      if (hparm->verbosity) printf("%s: !(0 <= %d), so: continue\n", me, optIdx);
      continue;
    }
    /* see if we can associate some parameters with the flag */
    if (hparm->verbosity)
      printf("%soptIdx %d |%s|: any parms?\n", me, optIdx, opt[optIdx].flag);
    parmNum = 0;
    nextOptIdx = 0; // what is index of option who's flag we see next
    while (parmNum < _hestMax(opt[optIdx].max)   /* */
           && argIdx + parmNum + 1 <= *argcP - 1 /* */
           && -1
                == (nextOptIdx = whichOptFlag(opt,                        /* */
                                              argv[argIdx + parmNum + 1], /* */
                                              hparm))) {
      parmNum++;
      if (hparm->verbosity)
        printf("%soptIdx %d |%s|: parmNum --> %d nextOptIdx = %d\n", me, optIdx,
               opt[optIdx].flag, parmNum, nextOptIdx);
    }
    /* we stopped because we got the max number of parameters, or
       because we hit the end of the command line, or
       because whichOptFlag() returned something other than -1,
       which means it returned -2, or a valid option index.
       If we stopped because of whichOptFlag()'s return value,
       nextOptIdx has been set to that return value */
    if (hparm->verbosity)
      printf("%sB: optIdx %d |%s|: stopped with parmNum = %d nextOptIdx = %d\n", me,
             optIdx, opt[optIdx].flag, parmNum, nextOptIdx);
    if (parmNum < (int)opt[optIdx].min) { /* HEY scrutinize casts */
      /* didn't get minimum number of parameters */
      if (!(argIdx + parmNum + 1 <= *argcP - 1)) {
        fprintf(stderr,
                "%sgot to end of line before getting %d parameter%s "
                "for %s (got %d)\n",
                ME, opt[optIdx].min, opt[optIdx].min > 1 ? "s" : "",
                identStr(ident1, opt + optIdx, hparm, AIR_TRUE), parmNum);
      } else if (-2 != nextOptIdx) {
        fprintf(stderr, "%ssaw %s before getting %d parameter%s for %s (got %d)\n", ME,
                identStr(ident1, opt + nextOptIdx, hparm, AIR_FALSE), opt[optIdx].min,
                opt[optIdx].min > 1 ? "s" : "",
                identStr(ident2, opt + optIdx, hparm, AIR_FALSE), parmNum);
      } else {
        fprintf(stderr,
                "%ssaw \"-%c\" (option-parameter-stop flag) before getting %d "
                "parameter%s for %s (got %d)\n",
                ME, VAR_PARM_STOP_FLAG, opt[optIdx].min, opt[optIdx].min > 1 ? "s" : "",
                identStr(ident2, opt + optIdx, hparm, AIR_FALSE), parmNum);
      }
      return 1;
    }
    /* record number of parameters seen for this option */
    optParmNum[optIdx] = parmNum;
    if (hparm->verbosity) {
      printf("%s________ argv[%d]=|%s|: *argcP = %d -> optIdx = %d\n", me, argIdx,
             argv[argIdx], *argcP, optIdx);
      printArgv(*argcP, argv, "     ");
    }
    /* lose the flag argument */
    free(extractToStr(argcP, argv, argIdx, 1, NULL, NULL));
    /* extract the args after the flag */
    if (optAprd[optIdx]) {
      /* oh so this is not the first time we've seen this option;
      so now forget everything triggered by having seen it already */
      airMopSub(pmop, optParms[optIdx], airFree);
      optParms[optIdx] = (char *)airFree(optParms[optIdx]);
    }
    optParms[optIdx] = extractToStr(argcP, argv, argIdx, optParmNum[optIdx], NULL, NULL);
    airMopAdd(pmop, optParms[optIdx], airFree, airMopAlways);
    optAprd[optIdx] = AIR_TRUE;
    if (-2 == nextOptIdx) {
      /* we drop the option-parameter-stop flag */
      free(extractToStr(argcP, argv, argIdx, 1, NULL, NULL));
    }
    if (hparm->verbosity) {
      printArgv(*argcP, argv, "     ");
      printf("%s^^^^^^^^ *argcP = %d\n", me, *argcP);
      printf("%soptParms[%d] = |%s|\n", me, optIdx,
             optParms[optIdx] ? optParms[optIdx] : "(null)");
    }
  }

  /* make sure that flagged options without default were given */
  optNum = hestOptNum(opt);
  for (op = 0; op < optNum; op++) {
    if (1 != opt[op].kind && opt[op].flag && !opt[op].dflt && !optAprd[op]) {
      fprintf(stderr, "%sdidn't get required %s\n", ME,
              identStr(ident1, opt + op, hparm, AIR_FALSE));
      return 1;
    }
  }

  return 0;
}

static int
nextUnflagged(int op, hestOpt *opt, int optNum) {

  for (; op <= optNum - 1; op++) {
    if (!opt[op].flag) break;
  }
  return op;
}

/*
extractUnflagged()

extracts the parameters associated with all unflagged options from the given *argcP and
argv (wherein the flag and subsequent parms were all separate strings), and here stores
them in optParms[] as a single VTAB-separated string (VTABs originating in extractToStr),
recording the number of parameters in optParmNum[].

This is the function that has to handle the trickly logic of allowing there to be
multiple unflagged options, one of which may have a variable number of parms; that
one has to be extracted last.
*/
static int
extractUnflagged(char **optParms, unsigned int *optParmNum, int *argcP, char **argv,
                 hestOpt *opt, const hestParm *hparm, airArray *pmop) {
  /* see note on ME (at top) for why me[] ends with ": " */
  static const char me[] = "extractUnflagged: ";
  char ident[AIR_STRLEN_HUGE + 1];
  int nvp, np, op, unflag1st, unflagVar, optNum;

  optNum = hestOptNum(opt);
  unflag1st = nextUnflagged(0, opt, optNum);
  if (optNum == unflag1st) {
    /* no unflagged options; we're done */
    return 0;
  }
  if (hparm->verbosity) {
    printf("%soptNum %d != unflag1st %d: have some (of %d) unflagged options\n", me,
           optNum, unflag1st, optNum);
  }

  for (unflagVar = unflag1st; unflagVar != optNum;
       unflagVar = nextUnflagged(unflagVar + 1, opt, optNum)) {
    if (AIR_INT(opt[unflagVar].min) < _hestMax(opt[unflagVar].max)) {
      break;
    }
  }
  /* now, if there is a variable parameter unflagged opt, unflagVar is its
     index in opt[], or else unflagVar is optNum */
  if (hparm->verbosity) {
    printf("%sunflagVar %d\n", me, unflagVar);
  }

  /* grab parameters for all unflagged opts before opt[unflagVar] */
  for (op = nextUnflagged(0, opt, optNum); op < unflagVar;
       op = nextUnflagged(op + 1, opt, optNum)) {
    if (hparm->verbosity) {
      printf("%sop = %d; unflagVar = %d\n", me, op, unflagVar);
    }
    np = opt[op].min; /* min == max, as implied by how unflagVar was set */
    if (!(np <= *argcP)) {
      fprintf(stderr, "%sdon't have %d parameter%s %s%s%sfor %s\n", ME, np,
              np > 1 ? "s" : "", argv[0] ? "starting at \"" : "", argv[0] ? argv[0] : "",
              argv[0] ? "\" " : "", identStr(ident, opt + op, hparm, AIR_TRUE));
      return 1;
    }
    optParms[op] = extractToStr(argcP, argv, 0, np, NULL, NULL);
    airMopAdd(pmop, optParms[op], airFree, airMopAlways);
    optParmNum[op] = np;
  }
  /* we skip over the variable parameter unflagged option,
  subtract from *argcP the number of parameters in all the opts which follow it,
  in order to get the number of parameters in the sole variable parameter option,
  store this in nvp */
  nvp = *argcP;
  for (op = nextUnflagged(unflagVar + 1, opt, optNum); op < optNum;
       op = nextUnflagged(op + 1, opt, optNum)) {
    nvp -= opt[op].min; /* min == max */
  }
  if (nvp < 0) {
    op = nextUnflagged(unflagVar + 1, opt, optNum);
    np = opt[op].min;
    fprintf(stderr, "%sdon't have %d parameter%s for %s\n", ME, np, np > 1 ? "s" : "",
            identStr(ident, opt + op, hparm, AIR_FALSE));
    return 1;
  }
  /* else we had enough args for all the unflagged options following
     the sole variable parameter unflagged option, so snarf them up */
  for (op = nextUnflagged(unflagVar + 1, opt, optNum); op < optNum;
       op = nextUnflagged(op + 1, opt, optNum)) {
    np = opt[op].min;
    optParms[op] = extractToStr(argcP, argv, nvp, np, NULL, NULL);
    airMopAdd(pmop, optParms[op], airFree, airMopAlways);
    optParmNum[op] = np;
  }

  /* now we grab the parameters of the sole variable parameter unflagged opt,
     if it exists (unflagVar < optNum) */
  if (hparm->verbosity) {
    printf("%s (still here) unflagVar %d vs optNum %d (nvp %d)\n", me, unflagVar, optNum,
           nvp);
  }
  if (unflagVar < optNum) {
    if (hparm->verbosity) {
      printf("%s unflagVar=%d: min, nvp, max = %d %d %d\n", me, unflagVar,
             opt[unflagVar].min, nvp, _hestMax(opt[unflagVar].max));
    }
    /* we'll do error checking for unexpected args later */
    nvp = AIR_MIN(nvp, _hestMax(opt[unflagVar].max));
    if (nvp) {
      unsigned int gotp = 0;
      /* pre-2023: this check used to be done regardless of nvp, but that incorrectly
      triggered this error message when there were zero given parms, but the default
      could have supplied them */
      if (nvp < AIR_INT(opt[unflagVar].min)) {
        fprintf(stderr, "%sdidn't get minimum of %d arg%s for %s (got %d)\n", ME,
                opt[unflagVar].min, opt[unflagVar].min > 1 ? "s" : "",
                identStr(ident, opt + unflagVar, hparm, AIR_TRUE), nvp);
        return 1;
      }
      optParms[unflagVar] = extractToStr(argcP, argv, 0, nvp, &gotp, hparm);
      if (hparm->verbosity) {
        printf("%s extracted %u to new string |%s| (*argcP now %d)\n", me, gotp,
               optParms[unflagVar], *argcP);
      }
      airMopAdd(pmop, optParms[unflagVar], airFree, airMopAlways);
      optParmNum[unflagVar] = gotp; /* which is < nvp in case of "--" */
    } else {
      optParms[unflagVar] = NULL;
      optParmNum[unflagVar] = 0;
    }
  }
  return 0;
}

static int
_hestDefaults(char **optParms, int *optDfltd, unsigned int *optParmNum,
              const int *optAprd, const hestOpt *opt, const hestParm *hparm,
              airArray *mop) {
  /* see note on ME (at top) for why me[] ends with ": " */
  static const char me[] = "_hestDefaults: ";
  char *tmpS, ident[AIR_STRLEN_HUGE + 1];
  int optIdx, optNum;

  optNum = hestOptNum(opt);
  for (optIdx = 0; optIdx < optNum; optIdx++) {
    if (hparm->verbosity)
      printf("%soptIdx=%d/%d (kind=%d): parms=\"%s\" optParmNum=%u, optAprd=%d\n", me,
             optIdx, optNum, opt[optIdx].kind, optParms[optIdx], optParmNum[optIdx],
             optAprd[optIdx]);
    switch (opt[optIdx].kind) {
    case 1:
      /* -------- (no-parameter) boolean flags -------- */
      /* default is indeed always ignored for the sake of setting the option's value, but
         optDfltd is used downstream to set the option's source. The info came from
         the user if the flag appears, otherwise it is from the default. */
      optDfltd[optIdx] = !optAprd[optIdx];
      break;
    case 2:
      /* -------- one required parameter -------- */
    case 3:
      /* -------- multiple required parameters -------- */
      /* we'll used defaults if the flag didn't appear */
      optDfltd[optIdx] = opt[optIdx].flag && !optAprd[optIdx];
      break;
    case 4:
      /* -------- optional single variables -------- */
      /* if the flag appeared (if there is a flag) but the parameter didn't, we'll
         "invert" the default; if the flag didn't appear (or if there isn't a flag) and
         the parameter also didn't appear, we'll use the default.  In either case,
         optParmNum[op] will be zero, and in both cases, we need to use the default
         information. */
      optDfltd[optIdx] = (0 == optParmNum[optIdx]);
      /* fprintf(stderr, "%s optParmNum[%d] = %u --> optDfltd[%d] = %d\n", me,
       *       op, optParmNum[op], op, optDfltd[op]); */
      break;
    case 5:
      /* -------- multiple optional parameters -------- */
      /* we'll use the default if there is a flag and it didn't appear. Otherwise (with a
         flagged option), if optParmNum[op] is zero, we'll use the default if user has
         given zero parameters, yet the the option requires at least one. If an unflagged
         option can have zero parms, and user has given zero parms, then we don't use the
         default */
      optDfltd[optIdx]
        = (opt[optIdx].flag
             ? !optAprd[optIdx] /* option is flagged and flag didn't appear */
             /* else: option is unflagged, and there were no given parms,
             and yet the option requires at least one parm */
             : !optParmNum[optIdx] && opt[optIdx].min >= 1);
      /* fprintf(stderr,
       *       "!%s: opt[%d].flag = %d; optAprd[op] = %d; optParmNum[op] = %d;
       * opt[op].min = %d "
       *       "--> optDfltd[op] = %d\n",
       *       me, op, !!opt[op].flag, optAprd[op], optParmNum[op], opt[op].min,
       * optDfltd[op]);
       */
      break;
    }
    /* if not using the default, we're done with this option; continue to next one */
    if (!optDfltd[optIdx]) continue;
    /* HEY NO: need to do VTAB delimiting here! */
    optParms[optIdx] = airStrdup(opt[optIdx].dflt);
    if (hparm->verbosity) {
      printf("%soptParms[%d] = |%s|\n", me, optIdx, optParms[optIdx]);
    }
    if (optParms[optIdx]) {
      airMopAdd(mop, optParms[optIdx], airFree, airMopAlways);
      airOneLinify(optParms[optIdx]);
      tmpS = airStrdup(optParms[optIdx]);
      optParmNum[optIdx] = airStrntok(tmpS, " ");
      airFree(tmpS);
    }
    /* fprintf(stderr,
     *       "!%s: after default; optParmNum[%d] = %u; varparm = %d (min %d vs max
     * %d)\n", me, op, optParmNum[op], AIR_INT(opt[op].min) < _hestMax(opt[op].max),
     *       ((int)opt[op].min), _hestMax(opt[op].max)); */
    if (AIR_INT(opt[optIdx].min) < _hestMax(opt[optIdx].max)) {
      if (!AIR_IN_CL(AIR_INT(opt[optIdx].min), AIR_INT(optParmNum[optIdx]),
                     _hestMax(opt[optIdx].max))) {
        if (-1 == opt[optIdx].max) {
          fprintf(stderr,
                  "%s# parameters (in default) for %s is %d, but need %d or more\n", ME,
                  identStr(ident, opt + optIdx, hparm, AIR_TRUE), optParmNum[optIdx],
                  opt[optIdx].min);
        } else {
          fprintf(
            stderr,
            "%s# parameters (in default) for %s is %d, but need between %d and %d\n", ME,
            identStr(ident, opt + optIdx, hparm, AIR_TRUE), optParmNum[optIdx],
            opt[optIdx].min, _hestMax(opt[optIdx].max));
        }
        return 1;
      }
    }
  }
  return 0;
}

/*
This function moved from air/miscAir;
the usage below is its only usage in Teem
*/
static int
airIStore(void *v, int t, int i) {

  switch (t) {
  case airTypeBool:
    return (*((int *)v) = !!i);
    break;
  case airTypeShort:
    return (*((short *)v) = i);
    break;
  case airTypeUShort:
    return (int)(*((unsigned short *)v) = i);
    break;
  case airTypeInt:
    return (*((int *)v) = i);
    break;
  case airTypeUInt:
    return (int)(*((unsigned int *)v) = i);
    break;
  case airTypeLong:
    return (int)(*((long int *)v) = i);
    break;
  case airTypeULong:
    return (int)(*((unsigned long int *)v) = i);
    break;
  case airTypeSize_t:
    return (int)(*((size_t *)v) = i);
    break;
  case airTypeFloat:
    return (int)(*((float *)v) = (float)(i));
    break;
  case airTypeDouble:
    return (int)(*((double *)v) = (double)(i));
    break;
  case airTypeChar:
    return (*((char *)v) = (char)(i));
    break;
  default:
    return 0;
    break;
  }
}

/*
this function moved from air/miscAir; the usage below
is its only usage in Teem
*/
static double
airDLoad(void *v, int t) {

  switch (t) {
  case airTypeBool:
    return AIR_CAST(double, *((int *)v));
    break;
  case airTypeShort:
    return AIR_CAST(double, *((short *)v));
    break;
  case airTypeUShort:
    return AIR_CAST(double, *((unsigned short *)v));
    break;
  case airTypeInt:
    return AIR_CAST(double, *((int *)v));
    break;
  case airTypeUInt:
    return AIR_CAST(double, *((unsigned int *)v));
    break;
  case airTypeLong:
    return AIR_CAST(double, *((long int *)v));
    break;
  case airTypeULong:
    return AIR_CAST(double, *((unsigned long int *)v));
    break;
  case airTypeSize_t:
    return AIR_CAST(double, *((size_t *)v));
    break;
  case airTypeFloat:
    return AIR_CAST(double, *((float *)v));
    break;
  case airTypeDouble:
    return *((double *)v);
    break;
  case airTypeChar:
    return AIR_CAST(double, *((char *)v));
    break;
  default:
    return 0;
    break;
  }
}

/* whichCase() helps figure out logic of interpreting parameters and defaults
   for kind 4 and kind 5 options. (formerly "private" _hestCase) */
static int
whichCase(hestOpt *opt, const int *optDfltd, const unsigned int *optParmNum,
          const int *optAprd, int op) {

  if (opt[op].flag && !optAprd[op]) {
    return 0;
  } else if ((4 == opt[op].kind && optDfltd[op])
             || (5 == opt[op].kind && !optParmNum[op])) {
    return 1;
  } else {
    return 2;
  }
}

static int
setValues(char **optParms, int *optDfltd, unsigned int *optParmNum, int *appr,
          hestOpt *opt, const hestParm *hparm, airArray *pmop) {
  /* see note on ME (at top) for why me[] ends with ": " */
  static const char me[] = "setValues: ";
  char ident[AIR_STRLEN_HUGE + 1], cberr[AIR_STRLEN_HUGE + 1], *tok, *last,
    *optParmsCopy;
  double tmpD;
  int op, type, optNum, p, ret;
  void *vP;
  char *cP;
  size_t size;

  optNum = hestOptNum(opt);
  for (op = 0; op < optNum; op++) {
    identStr(ident, opt + op, hparm, AIR_TRUE);
    opt[op].source = optDfltd[op] ? hestSourceDefault : hestSourceCommandLine;
    /* 2023 GLK notes that r6388 2020-05-14 GLK was asking:
        How is it that, once the command-line has been parsed, there isn't an
        easy way to see (or print, for an error message) the parameter (or
        concatenation of parameters) that was passed for a given option?
    and it turns out that adding this was as simple as adding this one following
    line. The inscrutability of the hest code (or really the self-reinforcing
    learned fear of working with the hest code) seems to have been the barrier. */
    opt[op].parmStr = airStrdup(optParms[op]);
    type = opt[op].type;
    size = (airTypeEnum == type /* */
              ? sizeof(int)
              : (airTypeOther == type /* */
                   ? opt[op].CB->size
                   : _hestTypeSize[type]));
    cP = (char *)(vP = opt[op].valueP);
    if (hparm->verbosity) {
      printf("%s %d of %d: \"%s\": |%s| --> kind=%d, type=%d, size=%u\n", me, op,
             optNum - 1, optParms[op], ident, opt[op].kind, type, (unsigned int)size);
    }
    /* we may over-write these */
    opt[op].alloc = 0;
    if (opt[op].sawP) {
      *(opt[op].sawP) = 0;
    }
    switch (opt[op].kind) {
    case 1:
      /* -------- parameter-less boolean flags -------- */
      /* the value pointer is always assumed to be an int* */
      if (vP) *((int *)vP) = appr[op];
      break;
    case 2:
      /* -------- one required parameter -------- */
      /* 2023 GLK is really curious why "if (optParms[op] && vP) {" is (​repeatedly)
      guarding all the work in these blocks, and why that wasn't factored out */
      if (optParms[op] && vP) {
        switch (type) {
        case airTypeEnum:
          if (1 != airParseStrE((int *)vP, optParms[op], " ", 1, opt[op].enm)) {
            fprintf(stderr, "%scouldn\'t parse %s\"%s\" as %s for %s\n", ME,
                    optDfltd[op] ? "(default) " : "", optParms[op], opt[op].enm->name,
                    ident);
            return 1;
          }
          break;
        case airTypeOther:
          strcpy(cberr, "");
          ret = opt[op].CB->parse(vP, optParms[op], cberr);
          if (ret) {
            if (strlen(cberr)) {
              fprintf(stderr, "%serror parsing \"%s\" as %s for %s:\n%s\n", ME,
                      optParms[op], opt[op].CB->type, ident, cberr);
            } else {
              fprintf(stderr, "%serror parsing \"%s\" as %s for %s: returned %d\n", ME,
                      optParms[op], opt[op].CB->type, ident, ret);
            }
            return ret;
          }
          if (opt[op].CB->destroy) {
            /* vP is the address of a void*, we manage the void * */
            opt[op].alloc = 1;
            airMopAdd(pmop, (void **)vP, (airMopper)airSetNull, airMopOnError);
            airMopAdd(pmop, *((void **)vP), opt[op].CB->destroy, airMopOnError);
          }
          break;
        case airTypeString:
          if (1
              != airParseStrS((char **)vP, optParms[op], " ", 1
                              /*, hparm->greedySingleString */)) {
            fprintf(stderr, "%scouldn't parse %s\"%s\" as %s for %s\n", ME,
                    optDfltd[op] ? "(default) " : "", optParms[op], _hestTypeStr[type],
                    ident);
            return 1;
          }
          /* vP is the address of a char* (a char **), but what we
             manage with airMop is the char * */
          opt[op].alloc = 1;
          airMopMem(pmop, vP, airMopOnError);
          break;
        default:
          /* type isn't string or enum, so no last arg to hestParseStr[type] */
          if (1 != _hestParseStr[type](vP, optParms[op], " ", 1)) {
            fprintf(stderr, "%scouldn't parse %s\"%s\" as %s for %s\n", ME,
                    optDfltd[op] ? "(default) " : "", optParms[op], _hestTypeStr[type],
                    ident);
            return 1;
          }
          break;
        }
      }
      break;
    case 3:
      /* -------- multiple required parameters -------- */
      if (optParms[op] && vP) {
        switch (type) {
        case airTypeEnum:
          if (opt[op].min != /* min == max */
              airParseStrE((int *)vP, optParms[op], " ", opt[op].min, opt[op].enm)) {
            fprintf(stderr, "%scouldn't parse %s\"%s\" as %d %s%s for %s\n", ME,
                    optDfltd[op] ? "(default) " : "", optParms[op], opt[op].min,
                    opt[op].enm->name, opt[op].min > 1 ? "s" : "", ident);
            return 1;
          }
          break;
        case airTypeOther:
          optParmsCopy = airStrdup(optParms[op]);
          for (p = 0; p < (int)opt[op].min; p++) { /* HEY scrutinize casts */
            tok = airStrtok(!p ? optParmsCopy : NULL, " ", &last);
            strcpy(cberr, "");
            ret = opt[op].CB->parse(cP + p * size, tok, cberr);
            if (ret) {
              if (strlen(cberr))
                fprintf(stderr,
                        "%serror parsing \"%s\" (in \"%s\") as %s "
                        "for %s:\n%s\n",
                        ME, tok, optParms[op], opt[op].CB->type, ident, cberr);
              else
                fprintf(stderr,
                        "%serror parsing \"%s\" (in \"%s\") as %s "
                        "for %s: returned %d\n",
                        ME, tok, optParms[op], opt[op].CB->type, ident, ret);
              free(optParmsCopy);
              return 1;
            }
          }
          free(optParmsCopy);
          if (opt[op].CB->destroy) {
            /* vP is an array of void*s, we manage the individual void*s */
            opt[op].alloc = 2;
            for (p = 0; p < (int)opt[op].min; p++) { /* HEY scrutinize casts */
              airMopAdd(pmop, ((void **)vP) + p, (airMopper)airSetNull, airMopOnError);
              airMopAdd(pmop, *(((void **)vP) + p), opt[op].CB->destroy, airMopOnError);
            }
          }
          break;
        case airTypeString:
          if (opt[op].min != /* min == max */
              _hestParseStr[type](vP, optParms[op], " ", opt[op].min)) {
            fprintf(stderr, "%scouldn't parse %s\"%s\" as %d %s%s for %s\n", ME,
                    optDfltd[op] ? "(default) " : "", optParms[op], opt[op].min,
                    _hestTypeStr[type], opt[op].min > 1 ? "s" : "", ident);
            return 1;
          }
          /* vP is an array of char*s, (a char**), and what we manage
             with airMop are the individual vP[p]. */
          opt[op].alloc = 2;
          for (p = 0; p < (int)opt[op].min; p++) { /* HEY scrutinize casts */
            airMopMem(pmop, &(((char **)vP)[p]), airMopOnError);
          }
          break;
        default:
          if (opt[op].min != /* min == max */
              _hestParseStr[type](vP, optParms[op], " ", opt[op].min)) {
            fprintf(stderr, "%scouldn't parse %s\"%s\" as %d %s%s for %s\n", ME,
                    optDfltd[op] ? "(default) " : "", optParms[op], opt[op].min,
                    _hestTypeStr[type], opt[op].min > 1 ? "s" : "", ident);
            return 1;
          }
          break;
        }
      }
      break;
    case 4:
      /* -------- optional single variables -------- */
      if (optParms[op] && vP) {
        int pret;
        switch (type) {
        case airTypeChar:
          /* no "inversion" for chars: using the flag with no parameter is the same as
          not using the flag i.e. we just parse from the default string */
          if (1 != _hestParseStr[type](vP, optParms[op], " ", 1)) {
            fprintf(stderr, "%scouldn't parse %s\"%s\" as %s for %s\n", ME,
                    optDfltd[op] ? "(default) " : "", optParms[op], _hestTypeStr[type],
                    ident);
            return 1;
          }
          opt[op].alloc = 0;
          break;
        case airTypeString:
          /* this is a bizarre case: optional single string, with some kind of value
          "inversion". 2023 GLK would prefer to make this like Char, Enum, and Other: for
          which there is no attempt at "inversion". But for some reason the inversion of
          a non-empty default string to a NULL string value, when the flag is used
          without a parameter, was implemented from the early days of hest.  Assuming
          that a younger GLK long ago had a reason for that, that functionality now
          persists. */
          pret = _hestParseStr[type](vP, optParms[op], " ",
                                     1 /*, hparm->greedySingleString */);
          if (1 != pret) {
            fprintf(stderr, "%scouldn't parse %s\"%s\" as %s for %s\n", ME,
                    optDfltd[op] ? "(default) " : "", optParms[op], _hestTypeStr[type],
                    ident);
            return 1;
          }
          opt[op].alloc = 1;
          if (opt[op].flag && 1 == whichCase(opt, optDfltd, optParmNum, appr, op)) {
            /* we just parsed the default, but now we want to "invert" it */
            *((char **)vP) = (char *)airFree(*((char **)vP));
            opt[op].alloc = 0;
          }
          /* vP is the address of a char* (a char**), and what we
             manage with airMop is the char * */
          airMopMem(pmop, vP, airMopOnError);
          break;
        case airTypeEnum:
          if (1 != airParseStrE((int *)vP, optParms[op], " ", 1, opt[op].enm)) {
            fprintf(stderr, "%scouldn't parse %s\"%s\" as %s for %s\n", ME,
                    optDfltd[op] ? "(default) " : "", optParms[op], opt[op].enm->name,
                    ident);
            return 1;
          }
          break;
        case airTypeOther:
          /* we're parsing an single "other".  We will not perform the special flagged
          single variable parameter games as done above, so whether this option is
          flagged or unflagged, we're going to treat it like an unflagged single variable
          parameter option: if the parameter didn't appear, we'll parse it from the
          default, if it did appear, we'll parse it from the command line.  Setting up
          optParms[op] thusly has already been done by _hestDefaults() */
          strcpy(cberr, "");
          ret = opt[op].CB->parse(vP, optParms[op], cberr);
          if (ret) {
            if (strlen(cberr))
              fprintf(stderr, "%serror parsing \"%s\" as %s for %s:\n%s\n", ME,
                      optParms[op], opt[op].CB->type, ident, cberr);
            else
              fprintf(stderr, "%serror parsing \"%s\" as %s for %s: returned %d\n", ME,
                      optParms[op], opt[op].CB->type, ident, ret);
            return 1;
          }
          if (opt[op].CB->destroy) {
            /* vP is the address of a void*, we manage the void* */
            opt[op].alloc = 1;
            airMopAdd(pmop, vP, (airMopper)airSetNull, airMopOnError);
            airMopAdd(pmop, *((void **)vP), opt[op].CB->destroy, airMopOnError);
          }
          break;
        default:
          if (1 != _hestParseStr[type](vP, optParms[op], " ", 1)) {
            fprintf(stderr, "%scouldn't parse %s\"%s\" as %s for %s\n", ME,
                    optDfltd[op] ? "(default) " : "", optParms[op], _hestTypeStr[type],
                    ident);
            return 1;
          }
          opt[op].alloc = 0;
          /* HEY sorry about confusion about hestOpt->parmStr versus the value set
          here, due to this "inversion" */
          if (1 == whichCase(opt, optDfltd, optParmNum, appr, op)) {
            /* we just parsed the default, but now we want to "invert" it */
            tmpD = airDLoad(vP, type);
            airIStore(vP, type, tmpD ? 0 : 1);
          }
          break;
        }
      }
      break;
    case 5:
      /* -------- multiple variable parameters -------- */
      if (optParms[op] && vP) {
        if (1 == whichCase(opt, optDfltd, optParmNum, appr, op)) {
          *((void **)vP) = NULL;
          /* alloc and sawP set above */
        } else {
          if (airTypeString == type) {
            /* this is sneakiness: we allocate one more element so that
               the resulting char** is, like argv, NULL-terminated */
            *((void **)vP) = calloc(optParmNum[op] + 1, size);
          } else {
            if (optParmNum[op]) {
              /* only allocate if there's something to allocate */
              *((void **)vP) = calloc(optParmNum[op], size);
            } else {
              *((void **)vP) = NULL;
            }
          }
          if (hparm->verbosity) {
            printf("%s: optParmNum[%d] = %u\n", me, op, optParmNum[op]);
            printf("%s: new array (size %u*%u) is at 0x%p\n", me, optParmNum[op],
                   (unsigned int)size, *((void **)vP));
          }
          if (*((void **)vP)) {
            airMopMem(pmop, vP, airMopOnError);
          }
          *(opt[op].sawP) = optParmNum[op];
          /* so far everything we've done is regardless of type */
          switch (type) {
          case airTypeEnum:
            opt[op].alloc = 1;
            if (optParmNum[op]
                != airParseStrE((int *)(*((void **)vP)), optParms[op], " ",
                                optParmNum[op], opt[op].enm)) {
              fprintf(stderr, "%scouldn't parse %s\"%s\" as %u %s%s for %s\n", ME,
                      optDfltd[op] ? "(default) " : "", optParms[op], optParmNum[op],
                      opt[op].enm->name, optParmNum[op] > 1 ? "s" : "", ident);
              return 1;
            }
            break;
          case airTypeOther:
            cP = (char *)(*((void **)vP));
            optParmsCopy = airStrdup(optParms[op]);
            opt[op].alloc = (opt[op].CB->destroy ? 3 : 1);
            for (p = 0; p < (int)optParmNum[op]; p++) { /* HEY scrutinize casts */
              tok = airStrtok(!p ? optParmsCopy : NULL, " ", &last);
              /* (Note from 2023-06-24: "hammerhead" was hammerhead.ucsd.edu, an Intel
              Itanium ("IA-64") machine that GLK had access to in 2003, presumably with
              an Intel compiler, providing a different debugging opportunity for this
              code. Revision r1985 from 2003-12-20 documented some issues discovered, in
              comments like the one below. Valgrind has hopefully resolved these issues
              now, but the comment below is preserved out of respect for the goals of
              Itanium, and nostalgia for that time at the end of grad school.)
                 hammerhead problems went away when this line
                 was replaced by the following one:
                 strcpy(cberr, "");
              */
              cberr[0] = 0;
              ret = opt[op].CB->parse(cP + p * size, tok, cberr);
              if (ret) {
                if (strlen(cberr))
                  fprintf(stderr,
                          "%serror parsing \"%s\" (in \"%s\") as %s "
                          "for %s:\n%s\n",
                          ME, tok, optParms[op], opt[op].CB->type, ident, cberr);

                else
                  fprintf(stderr,
                          "%serror parsing \"%s\" (in \"%s\") as %s "
                          "for %s: returned %d\n",
                          ME, tok, optParms[op], opt[op].CB->type, ident, ret);
                free(optParmsCopy);
                return 1;
              }
            }
            free(optParmsCopy);
            if (opt[op].CB->destroy) {
              for (p = 0; p < (int)optParmNum[op]; p++) { /* HEY scrutinize casts */
                /* avert your eyes.  vP is the address of an array of void*s.
                   We manage the void*s */
                airMopAdd(pmop, (*((void ***)vP)) + p, (airMopper)airSetNull,
                          airMopOnError);
                airMopAdd(pmop, *((*((void ***)vP)) + p), opt[op].CB->destroy,
                          airMopOnError);
              }
            }
            break;
          case airTypeString:
            opt[op].alloc = 3;
            if (optParmNum[op]
                != airParseStrS((char **)(*((void **)vP)), optParms[op], " ",
                                optParmNum[op] /*, hparm->greedySingleString */)) {
              fprintf(stderr, "%scouldn't parse %s\"%s\" as %d %s%s for %s\n", ME,
                      optDfltd[op] ? "(default) " : "", optParms[op], optParmNum[op],
                      _hestTypeStr[type], optParmNum[op] > 1 ? "s" : "", ident);
              return 1;
            }
            /* vP is the address of an array of char*s (a char ***), and
               what we manage with airMop is the individual (*vP)[p],
               as well as vP itself (above). */
            for (p = 0; p < (int)optParmNum[op]; p++) { /* HEY scrutinize casts */
              airMopAdd(pmop, (*((char ***)vP))[p], airFree, airMopOnError);
            }
            /* do the NULL-termination described above */
            (*((char ***)vP))[optParmNum[op]] = NULL;
            break;
          default:
            opt[op].alloc = 1;
            if (optParmNum[op]
                != _hestParseStr[type](*((void **)vP), optParms[op], " ",
                                       optParmNum[op])) {
              fprintf(stderr, "%scouldn't parse %s\"%s\" as %d %s%s for %s\n", ME,
                      optDfltd[op] ? "(default) " : "", optParms[op], optParmNum[op],
                      _hestTypeStr[type], optParmNum[op] > 1 ? "s" : "", ident);
              return 1;
            }
            break;
          }
        }
      }
      break;
    }
  }
  return 0;
}

/*
******** hestParse()
**
** documentation?
*/
int
hestParse(hestOpt *opt, int _argc, const char **_argv, char **errP,
          const hestParm *_hparm) {
  /* see note on ME (at top) for why me[] ends with ": " */
  static const char me[] = "hestParse: ";
  char **argv, **optParms;
  int a, argc, argc_used, respArgNum, *optAprd, *optDfltd, respFileNum, optNum, ret,
    sawHelp;
  unsigned int *optParmNum;
  airArray *mop;
  hestParm *hparm;

  optNum = hestOptNum(opt);

  /* -------- initialize the mop! */
  mop = airMopNew();

  /* -------- exactly one of (given) _hparm and (our) hparm is non-NULL */
  if (_hparm) {
    hparm = NULL;
  } else {
    hparm = hestParmNew();
    airMopAdd(mop, hparm, (airMopper)hestParmFree, airMopAlways);
  }
  /* how to const-correctly use hparm or _hparm in an expression */
#define HPARM (_hparm ? _hparm : hparm)

  /* -------- check on validity of the hestOpt array */
  if (_hestOPCheck(opt, HPARM)) {
    char *err = biffGetDone(HEST);
    if (errP) {
      *errP = err;
    } else {
      airMopAdd(mop, err, airFree, airMopAlways);
      fprintf(stderr, "%s: problem given hestOpt, hestParm:\n%s", __func__, err);
    }
    airMopError(mop);
    return 1;
  }

  /* -------- Create all the local arrays used to save state during
     the processing of all the different options */
  optParmNum = AIR_CALLOC(optNum, unsigned int);
  airMopMem(mop, &optParmNum, airMopAlways);
  optAprd = AIR_CALLOC(optNum, int);
  airMopMem(mop, &optAprd, airMopAlways);
  optDfltd = AIR_CALLOC(optNum, int);
  airMopMem(mop, &optDfltd, airMopAlways);
  optParms = AIR_CALLOC(optNum, char *);
  airMopMem(mop, &optParms, airMopAlways);
  for (a = 0; a < optNum; a++) {
    optParms[a] = NULL;
  }

  /* -------- find out how big the argv array needs to be, first
     by seeing how many args are in the response files, and then adding
     on the args from the actual argv (getting this right the first time
     greatly simplifies the problem of eliminating memory leaks) */
  if (argsInResponseFiles(&respArgNum, &respFileNum, _argv, HPARM)) {
    airMopError(mop);
    return 1;
  }
  /* the effective # args is superficial _argc, adjusted for how every response filename
     is effectively removed and then replace by its contents */
  argc = _argc + respArgNum - respFileNum;

  if (HPARM->verbosity) {
    printf("%s: respFileNum = %d; respArgNum = %d; _argc = %d --> argc = %d\n", me,
           respFileNum, respArgNum, _argc, argc);
  }
  argv = AIR_CALLOC(argc + 1, char *);
  airMopMem(mop, &argv, airMopAlways);

  /* -------- process response files (if any) and set the remaining
     elements of argv */
  opt->helpWanted = AIR_FALSE;
  if (HPARM->verbosity) printf("%s: #### calling copyArgv\n", me);
  argc_used = copyArgv(&sawHelp, argv, _argv, HPARM, mop);
  if (HPARM->verbosity) {
    printf("%s: #### copyArgv done (%d args copied; sawHelp=%d)\n", me, argc_used,
           sawHelp);
  }
  if (sawHelp) {
    /* saw "--help", which is not error, but is a show-stopper */
    opt->helpWanted = AIR_TRUE;
    /* the --help functionality has been grafted onto this code, >20 years after it was
       first written. Until it is more completely re-written, a goto does the job */
    goto parseEnd;
  }
  /* else !sawHelp; do sanity check on argc_used vs argc */
  if (argc_used < argc) {
    fprintf(stderr, "%sargc_used %d < argc %d; sorry, confused", ME, argc_used, argc);
    airMopError(mop);
    return 1;
  }

  /* -------- extract flags and their associated parameters from argv */
  if (HPARM->verbosity) printf("%s: #### calling extractFlagged\n", me);
  if (extractFlagged(optParms, optParmNum, optAprd, &argc_used, argv, opt, HPARM, mop)) {
    airMopError(mop);
    return 1;
  }
  if (HPARM->verbosity) printf("%s: #### extractFlagged done!\n", me);

  /* -------- extract args for unflagged options */
  if (HPARM->verbosity) printf("%s: #### calling extractUnflagged\n", me);
  if (extractUnflagged(optParms, optParmNum, &argc_used, argv, opt, HPARM, mop)) {
    airMopError(mop);
    return 1;
  }
  if (HPARM->verbosity) printf("%s: #### extractUnflagged done!\n", me);

  /* currently, any left-over arguments indicate error */
  if (argc_used) {
    /* char stops[3] = {'-', VAR_PARM_STOP_FLAG, '\0'}; triggers warning:
    initializer element is not computable at load time */
    char stops[3] = "-X";
    stops[1] = VAR_PARM_STOP_FLAG;
    if (strcmp(stops, argv[0])) {
      fprintf(stderr, "%sunexpected arg%s: \"%s\"\n", ME,
              ('-' == argv[0][0] ? " (or unrecognized flag)" : ""), argv[0]);
    } else {
      fprintf(stderr,
              "%sunexpected end-of-parameters flag \"%s\": "
              "not ending a flagged variable-parameter option\n",
              ME, stops);
    }
    airMopError(mop);
    return 1;
  }

  /* -------- learn defaults */
  if (HPARM->verbosity) printf("%s: #### calling hestDefaults\n", me);
  if (_hestDefaults(optParms, optDfltd, optParmNum, optAprd, opt, HPARM, mop)) {
    airMopError(mop);
    return 1;
  }
  if (HPARM->verbosity) printf("%s: #### hestDefaults done!\n", me);

  /* remove quotes from strings
         if greedy wasn't turned on for strings, then we have no hope
         of capturing filenames with spaces.
    (TeemV2 removed hparm->greedySingleString)
  char *param, *param_copy;
  if (HPARM->greedySingleString) {
    for (int i = 0; i < optNum; i++) {
      param = optParms[i];
      param_copy = NULL;
      if (param && strstr(param, " ")) {
        size_t start_index = 0;
        size_t end_index = strlen(param) - 1;
        if (param[start_index] == '\"') start_index++;
        if (param[end_index] == '\"') end_index--;
        param_copy = AIR_CALLOC(end_index - start_index + 2, char);
        strncpy(param_copy, &param[start_index], end_index - start_index + 1);
        param_copy[end_index - start_index + 1] = '\0';
        strcpy(param, param_copy);
        free(param_copy);
      }
    }
  }
  */

  /* -------- now, the actual parsing of values */
  if (HPARM->verbosity) printf("%s: #### calling setValues\n", me);
  /* this will also set hestOpt->parmStr */
  ret = setValues(optParms, optDfltd, optParmNum, optAprd, opt, HPARM, mop);
  if (ret) {
    airMopError(mop);
    return ret;
  }

  if (HPARM->verbosity) printf("%s: #### setValues done!\n", me);
#undef HPARM

parseEnd:
  airMopOkay(mop);
  if (*errP) {
    *errP = NULL;
  }
  return 0;
}

/*
******** hestParseFree()
**
** free()s whatever was allocated by hestParse()
**
** returns NULL only to facilitate use with the airMop functions.
** You should probably just ignore this quirk.
*/
void *
hestParseFree(hestOpt *opt) {
  int op, i, optNum;
  unsigned int ui;
  void **vP;
  void ***vAP;
  char **str;
  char ***strP;

  optNum = hestOptNum(opt);
  for (op = 0; op < optNum; op++) {
    opt[op].parmStr = airFree(opt[op].parmStr);
    /*
    printf("!hestParseFree: op = %d/%d -> kind = %d; type = %d; alloc = %d\n",
           op, optNum-1, opt[op].kind, opt[op].type, opt[op].alloc);
    */
    vP = (void **)opt[op].valueP;
    vAP = (void ***)opt[op].valueP;
    str = (char **)opt[op].valueP;
    strP = (char ***)opt[op].valueP;
    switch (opt[op].alloc) {
    case 0:
      /* nothing was allocated */
      break;
    case 1:
      if (airTypeOther != opt[op].type) {
        *vP = airFree(*vP);
      } else {
        /* alloc is one either because we parsed one thing, and we have a
           destroy callback, or, because we parsed a dynamically-created array
           of things, and we don't have a destroy callback */
        if (opt[op].CB->destroy) {
          *vP = opt[op].CB->destroy(*vP);
        } else {
          *vP = airFree(*vP);
        }
      }
      break;
    case 2:
      if (airTypeString == opt[op].type) {
        for (i = 0; i < (int)opt[op].min; i++) { /* HEY scrutinize casts */
          str[i] = (char *)airFree(str[i]);
        }
      } else {
        for (i = 0; i < (int)opt[op].min; i++) { /* HEY scrutinize casts */
          vP[i] = opt[op].CB->destroy(vP[i]);
        }
      }
      break;
    case 3:
      if (airTypeString == opt[op].type) {
        for (ui = 0; ui < *(opt[op].sawP); ui++) {
          (*strP)[ui] = (char *)airFree((*strP)[ui]);
        }
        *strP = (char **)airFree(*strP);
      } else {
        for (ui = 0; ui < *(opt[op].sawP); ui++) {
          (*vAP)[ui] = opt[op].CB->destroy((*vAP)[ui]);
        }
        *vAP = (void **)airFree(*vAP);
      }
      break;
    }
  }
  return NULL;
}

/*
hestParseOrDie()

Pre-June 2023 account:
** dumb little function which encapsulate a common usage of hest:
** first, make sure hestOpt is valid with hestOptCheck().  Then,
** if argc is 0 (and !hparm->noArgsIsNoProblem): maybe show
**    info, usage, and glossary, all according to given flags, then exit(1)
** if parsing failed: show error message, and maybe usage and glossary,
**    again according to boolean flags, then exit(1)
** if parsing succeeded: return

In June 2023 this function was completely re-written, but the description above should
still be true, if not the whole truth.  Long prior to re-write, "--version" and "--help"
had been usefully responded to as the sole argument, but only after a hestParse error
(which then sometimes leaked memory by not freeing errS). Now these are checked for prior
to calling hestParse, and (with hparm->respectDashDashHelp), "--help" is recognized
anywhere in the command-line.
*/
void
hestParseOrDie(hestOpt *opt, int argc, const char **argv, hestParm *hparm,
               const char *me, const char *info, int doInfo, int doUsage,
               int doGlossary) {
  int argcWanting, parseErr, wantHelp;
  char *errS = NULL;

  if (!(opt && argv)) {
    /* nothing to do given NULL pointers.  Since this function was first written, this
    condition (well actually just !opt) led to a plain return, which is what we do here,
    but it probably would be better to have an error message and exit, like below. */
    return;
  }

  if (hestOptCheck(opt, &errS)) {
    fprintf(stderr, "ERROR in hest usage:\n%s\n", errS);
    free(errS);
    /* exit, not return, since there's practically no recovery possible: hestOpts are
    effectively set up at compile time, even with the ubiquity of hestOptAdd. The caller
    is not going to be in a position to overcome the errors detected here, at runtime. */
    exit(1);
  }

  /* Pre-June 2023 these two check were done only after a hestParse error;
  why not check first? */
  if (argv[0] && !strcmp(argv[0], "--version")) {
    /* print version info and bail */
    char vbuff[AIR_STRLEN_LARGE + 1];
    airTeemVersionSprint(vbuff);
    printf("%s\n", vbuff);
    hestParmFree(hparm);
    hestOptFree(opt);
    exit(0);
  }
  if (argv[0] && !strcmp(argv[0], "--help")) {
    /* actually, not an error, --help was the first argument; does NOT depend on
    parm->respectDashDashHelp */
    argcWanting = AIR_FALSE;
    parseErr = 0;
    wantHelp = AIR_TRUE;
  } else {
    /* we call hestParse if there are args, or (else) having no args is ok */
    if (argc || (hparm && hparm->noArgsIsNoProblem)) {
      argcWanting = AIR_FALSE;
      parseErr = hestParse(opt, argc, argv, &errS, hparm);
      wantHelp = opt->helpWanted;
      if (wantHelp && parseErr) {
        /* should not happen at the same time */
        fprintf(stderr, "PANIC: hestParse both saw --help and had error:\n%s\n", errS);
        free(errS);
        exit(1);
      }
      /* at most one of wantHelp and parseErr is true */
    } else {
      /* the empty command-line is an implicit call for help */
      argcWanting = AIR_TRUE;
      parseErr = 0;
      /* subtle difference between argcWanting and wantHelp is for maintaining
      functionality of pre-June 2023 code */
      wantHelp = AIR_FALSE;
    }
  }
  if (!argcWanting && !wantHelp && !parseErr) {
    /* no help needed or wanted, and (if done) parsing was successful
    great; return to caller */
    return;
  }

  /* whether by argcWanting or wantHelp or parseErr, from here on out we are not
  returning to caller */
  if (parseErr) {
    fprintf(stderr, "ERROR: %s\n", errS);
    airFree(errS);
    /* but no return or exit; there's more to say */
  }
#define STDWUT (parseErr ? stderr : stdout)
  if (hparm && hparm->dieLessVerbose) {
    /* newer logic for when to print which things */
    if (wantHelp && info) hestInfo(STDWUT, me ? me : "", info, hparm);
    if (doUsage) hestUsage(STDWUT, opt, me ? me : "", hparm);
    if (wantHelp && doGlossary) {
      hestGlossary(STDWUT, opt, hparm);
    } else if ((!argc || parseErr) && me) {
      fprintf(STDWUT, "\"%s --help\" for more information\n", me);
    }
  } else {
    /* leave older (pre-dieLessVerbose) logic as is */
    if (!parseErr) {
      /* no error, just !argc */
      if (doInfo && info) hestInfo(stdout, me ? me : "", info, hparm);
    }
    if (doUsage) hestUsage(STDWUT, opt, me ? me : "", hparm);
    if (doGlossary) hestGlossary(STDWUT, opt, hparm);
  }
#undef STDWUT
  hestParmFree(hparm);
  hestOptFree(opt);
  exit(!!parseErr);
}
