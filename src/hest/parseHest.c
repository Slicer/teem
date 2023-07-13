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

#include "hest.h"
#include "privateHest.h"

#include <string.h>

#define ME ((parm && parm->verbosity) ? me : "")

/*
_hestArgsInResponseFiles()

returns the number of "args" (i.e. the number of space-separated strings) that will be
parsed from the response files. The role of this function is solely to simplify the task
of avoiding memory leaks.  By knowing exactly how many args we'll get in the response
file, then hestParse() can allocate its local argv[] for exactly as long as it needs to
be, and we can avoid using an airArray.  The drawback is that we open and read through
the response files twice.  Alas.
*/
static int
_hestArgsInResponseFiles(int *argcP, int *nrfP, const char **argv, char *err,
                         const hestParm *parm) {
  FILE *file;
  static const char me[] = "_hestArgsInResponseFiles: ";
  char line[AIR_STRLEN_HUGE + 1], *pound;
  int ai, len;

  *argcP = 0;
  *nrfP = 0;
  if (!parm->respFileEnable) {
    /* don't do response files; we're done */
    return 0;
  }

  ai = 0;
  while (argv[ai]) {
    if (parm->respFileFlag == argv[ai][0]) {
      /* NOTE: despite the repeated temptation: "-" aka stdin cannot be a response file,
         because it is going to be read in twice: once by _hestArgsInResponseFiles, and
         then again by copyArgv */
      if (!(file = fopen(argv[ai] + 1, "rb"))) {
        /* can't open the indicated response file for reading */
        sprintf(err, "%scouldn't open \"%s\" for reading as response file", ME,
                argv[ai] + 1);
        *argcP = 0;
        *nrfP = 0;
        return 1;
      }
      len = airOneLine(file, line, AIR_STRLEN_HUGE + 1);
      while (len > 0) {
        if ((pound = strchr(line, parm->respFileComment))) {
          *pound = '\0';
        }
        airOneLinify(line);
        *argcP += airStrntok(line, AIR_WHITESPACE);
        len = airOneLine(file, line, AIR_STRLEN_HUGE + 1);
      }
      fclose(file); /* ok because file != stdin, see above */
      (*nrfP)++;
    }
    ai++;
  }
  return 0;
}

/* printArgv prints (for debugging) the given non-const argv array */
static void
printArgv(int argc, char **argv, const char *pfx) {
  int a;

  printf("%sargc=%d : ", pfx ? pfx : "", argc);
  for (a = 0; a < argc; a++) {
    printf("%s%s ", pfx ? pfx : "", argv[a]);
  }
  printf("%s\n", pfx ? pfx : "");
}

/*
copyArgv()

Copies given oldArgv to newArgv, including (if they are enabled) injecting the contents
of response files.  BUT: this stops upon seeing "--help" if parm->respectDashDashHelp.
Allocations of the strings in newArgv are remembered (to be airFree'd later) in the given
pmop. Returns the number of args set in newArgv, and sets sawHelp if saw "--help".

For a brief moment prior to the 2.0.0 release, this also stopped if it saw "--" (or
whatever parm->varParamStopFlag implies), but that meant "--" is a brick wall that
hestParse could never see past. But that misunderstands the relationship between how
hestParse works and how the world uses "--".  According to POSIX guidelines:
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
hest to implement the expected behavior for
"--", hest has to care about "--" only in the context of collecting parameters to
*flagged* options. But copyArgv() is upstream of that awareness (of flagged vs
unflagged), so we do not act on "--" here.

Note that there are lots of ways that hest does NOT conform to these POSIX guidelines
(such as: currently single-character flags cannot be grouped together, and options can
have their arguments be optional), but those guidelines are used here to help documenting
what "--" should mean.
*/
static int
copyArgv(int *sawHelp, char **newArgv, const char **oldArgv, const hestParm *parm,
         airArray *pmop) {
  static const char me[] = "copyArgv";
  char line[AIR_STRLEN_HUGE + 1], *pound;
  int len, newArgc, oldArgc, incr, ai;
  FILE *file;

  newArgc = oldArgc = 0;
  *sawHelp = AIR_FALSE;
  while (oldArgv[oldArgc]) {
    if (parm->respectDashDashHelp && !strcmp("--help", oldArgv[oldArgc])) {
      *sawHelp = AIR_TRUE;
      break;
    }
    /* else not a show-stopper argument */
    if (parm->verbosity) {
      printf("%s:________ newArgc = %d, oldArgc = %d -> \"%s\"\n", me, newArgc, oldArgc,
             oldArgv[oldArgc]);
      printArgv(newArgc, newArgv, "     ");
    }
    if (!parm->respFileEnable || parm->respFileFlag != oldArgv[oldArgc][0]) {
      /* nothing to do with a response file */
      newArgv[newArgc] = airStrdup(oldArgv[oldArgc]);
      airMopAdd(pmop, newArgv[newArgc], airFree, airMopAlways);
      newArgc += 1;
    } else {
      /* It is a response file.  Error checking on open-ability
         should have been done by _hestArgsInResponseFiles() */
      file = fopen(oldArgv[oldArgc] + 1, "rb");
      len = airOneLine(file, line, AIR_STRLEN_HUGE + 1);
      while (len > 0) {
        if (parm->verbosity) printf("%s: line: |%s|\n", me, line);
        /* HEY HEY too bad for you if you put # inside a string */
        if ((pound = strchr(line, parm->respFileComment))) *pound = '\0';
        if (parm->verbosity) printf("%s: -0-> line: |%s|\n", me, line);
        airOneLinify(line);
        incr = airStrntok(line, AIR_WHITESPACE);
        if (parm->verbosity) printf("%s: -1-> line: |%s|, incr=%d\n", me, line, incr);
        airParseStrS(newArgv + newArgc, line, AIR_WHITESPACE, incr, AIR_FALSE);
        for (ai = 0; ai < incr; ai++) {
          /* This time, we did allocate memory.  We can use airFree and
             not airFreeP because these will not be reset before mopping */
          airMopAdd(pmop, newArgv[newArgc + ai], airFree, airMopAlways);
        }
        len = airOneLine(file, line, AIR_STRLEN_HUGE + 1);
        newArgc += incr;
      }
      fclose(file);
    }
    oldArgc++;
    if (parm->verbosity) {
      printArgv(newArgc, newArgv, "     ");
      printf("%s: ^^^^^^^ newArgc = %d, oldArgc = %d\n", me, newArgc, oldArgc);
    }
  }
  newArgv[newArgc] = NULL;

  return newArgc;
}

/*
** _hestPanic()
**
** all error checking on the given hest array itself (not the
** command line to be parsed).
**
** Prior to 2023 code revisit; this used to set the "kind" in all the opts
** but now that is more appropriately done at the time the option is added
** (by hestOptAdd, hestOptAdd_nva, hestOptSingleSet, or hestOptAdd_*_*)
*/
int
_hestPanic(hestOpt *opt, char *err, const hestParm *parm) {
  static const char me[] = "_hestPanic: ";
  char tbuff[AIR_STRLEN_HUGE + 1], *sep;
  int numvar, op, numOpts;

  numOpts = hestOptNum(opt);
  numvar = 0;
  for (op = 0; op < numOpts; op++) {
    if (!(AIR_IN_OP(airTypeUnknown, opt[op].type, airTypeLast))) {
      if (err)
        sprintf(err, "%s!!!!!! opt[%d].type (%d) not in valid range [%d,%d]", ME, op,
                opt[op].type, airTypeUnknown + 1, airTypeLast - 1);
      else
        fprintf(stderr, "%s: panic 0\n", me);
      return 1;
    }
    if (!(opt[op].valueP)) {
      if (err)
        sprintf(err, "%s!!!!!! opt[%d]'s valueP is NULL!", ME, op);
      else
        fprintf(stderr, "%s: panic 0.5\n", me);
      return 1;
    }
    if (-1 == opt[op].kind) {
      if (err)
        sprintf(err, "%s!!!!!! opt[%d]'s min (%d) and max (%d) incompatible", ME, op,
                opt[op].min, opt[op].max);
      else
        fprintf(stderr, "%s: panic 1\n", me);
      return 1;
    }
    if (5 == opt[op].kind && !(opt[op].sawP)) {
      if (err)
        sprintf(err,
                "%s!!!!!! have multiple variable parameters, "
                "but sawP is NULL",
                ME);
      else
        fprintf(stderr, "%s: panic 2\n", me);
      return 1;
    }
    if (airTypeEnum == opt[op].type) {
      if (!(opt[op].enm)) {
        if (err) {
          sprintf(err,
                  "%s!!!!!! opt[%d] (%s) is type \"enum\", but no "
                  "airEnum pointer given",
                  ME, op, opt[op].flag ? opt[op].flag : "?");
        } else {
          fprintf(stderr, "%s: panic 3\n", me);
        }
        return 1;
      }
    }
    if (airTypeOther == opt[op].type) {
      if (!(opt[op].CB)) {
        if (err) {
          sprintf(err,
                  "%s!!!!!! opt[%d] (%s) is type \"other\", but no "
                  "callbacks given",
                  ME, op, opt[op].flag ? opt[op].flag : "?");
        } else {
          fprintf(stderr, "%s: panic 4\n", me);
        }
        return 1;
      }
      if (!(opt[op].CB->size > 0)) {
        if (err)
          sprintf(err, "%s!!!!!! opt[%d]'s \"size\" (%d) invalid", ME, op,
                  (int)(opt[op].CB->size));
        else
          fprintf(stderr, "%s: panic 5\n", me);
        return 1;
      }
      if (!(opt[op].CB->type)) {
        if (err)
          sprintf(err, "%s!!!!!! opt[%d]'s \"type\" is NULL", ME, op);
        else
          fprintf(stderr, "%s: panic 6\n", me);
        return 1;
      }
      if (!(opt[op].CB->parse)) {
        if (err)
          sprintf(err, "%s!!!!!! opt[%d]'s \"parse\" callback NULL", ME, op);
        else
          fprintf(stderr, "%s: panic 7\n", me);
        return 1;
      }
      if (opt[op].CB->destroy && (sizeof(void *) != opt[op].CB->size)) {
        if (err)
          sprintf(err,
                  "%s!!!!!! opt[%d] has a \"destroy\", but size %lu isn't "
                  "sizeof(void*)",
                  ME, op, (unsigned long)(opt[op].CB->size));
        else
          fprintf(stderr, "%s: panic 8\n", me);
        return 1;
      }
    }
    if (opt[op].flag) {
      strcpy(tbuff, opt[op].flag);
      if ((sep = strchr(tbuff, parm->multiFlagSep))) {
        *sep = '\0';
        if (!(strlen(tbuff) && strlen(sep + 1))) {
          if (err)
            sprintf(err,
                    "%s!!!!!! either short (\"%s\") or long (\"%s\") flag"
                    " of opt[%d] is zero length",
                    ME, tbuff, sep + 1, op);
          else
            fprintf(stderr, "%s: panic 9\n", me);
          return 1;
        }
        if (parm->respectDashDashHelp && !strcmp("help", sep + 1)) {
          if (err)
            sprintf(err,
                    "%s!!!!!! long \"--%s\" flag of opt[%d] is same as \"--help\" "
                    "that requested hparm->respectDashDashHelp handles separately",
                    ME, sep + 1, op);
          else
            fprintf(stderr, "%s: panic 9.5\n", me);
          return 1;
        }
      } else {
        if (!strlen(opt[op].flag)) {
          if (err)
            sprintf(err, "%s!!!!!! opt[%d].flag is zero length", ME, op);
          else
            fprintf(stderr, "%s: panic 10\n", me);
          return 1;
        }
      }
      if (4 == opt[op].kind) {
        if (!opt[op].dflt) {
          if (err)
            sprintf(err,
                    "%s!!!!!! flagged single variable parameter must "
                    "specify a default",
                    ME);
          else
            fprintf(stderr, "%s: panic 11\n", me);
          return 1;
        }
        if (!strlen(opt[op].dflt)) {
          if (err)
            sprintf(err,
                    "%s!!!!!! flagged single variable parameter default "
                    "must be non-zero length",
                    ME);
          else
            fprintf(stderr, "%s: panic 12\n", me);
          return 1;
        }
      }
      /*
      sprintf(tbuff, "-%s", opt[op].flag);
      if (1 == sscanf(tbuff, "%f", &tmpF)) {
        if (err)
          sprintf(err, "%s!!!!!! opt[%d].flag (\"%s\") is numeric, bad news",
                  ME, op, opt[op].flag);
        return 1;
      }
      */
    }
    if (1 == opt[op].kind) {
      if (!opt[op].flag) {
        if (err)
          sprintf(err, "%s!!!!!! flags must have flags", ME);
        else
          fprintf(stderr, "%s: panic 13\n", me);
        return 1;
      }
    } else {
      if (!opt[op].name) {
        if (err)
          sprintf(err, "%s!!!!!! opt[%d] isn't a flag: must have \"name\"", ME, op);
        else
          fprintf(stderr, "%s: panic 14\n", me);
        return 1;
      }
    }
    if (4 == opt[op].kind && !opt[op].dflt) {
      if (err)
        sprintf(err,
                "%s!!!!!! opt[%d] is single variable parameter, but "
                "no default set",
                ME, op);
      else
        fprintf(stderr, "%s: panic 15\n", me);
      return 1;
    }
    numvar += ((int)opt[op].min < _hestMax(opt[op].max)
               && (NULL == opt[op].flag)); /* HEY scrutinize casts */
  }
  if (numvar > 1) {
    if (err)
      sprintf(err, "%s!!!!!! can't have %d unflagged min<max opts, only one", ME,
              numvar);
    else
      fprintf(stderr, "%s: panic 16\n", me);
    return 1;
  }
  return 0;
}

int
_hestErrStrlen(const hestOpt *opt, int argc, const char **argv) {
  int a, numOpts, ret, other;

  ret = 0;
  numOpts = hestOptNum(opt);
  other = AIR_FALSE;
  if (argv) {
    for (a = 0; a < argc; a++) {
      ret = AIR_MAX(ret, (int)airStrlen(argv[a]));
    }
  }
  for (a = 0; a < numOpts; a++) {
    ret = AIR_MAX(ret, (int)airStrlen(opt[a].flag));
    ret = AIR_MAX(ret, (int)airStrlen(opt[a].name));
    other |= opt[a].type == airTypeOther;
  }
  for (a = airTypeUnknown + 1; a < airTypeLast; a++) {
    ret = AIR_MAX(ret, (int)airStrlen(airTypeStr[a]));
  }
  if (other) {
    /* the callback's error() function may sprintf an error message
       into a buffer which is size AIR_STRLEN_HUGE+1 */
    ret += AIR_STRLEN_HUGE + 1;
  }
  ret += 4 * 12; /* as many as 4 ints per error message */
  ret += 257;    /* function name and text of hest's error message */

  return ret;
}

/*
identStr()
copies into ident a string for identifying an option in error and usage messages
*/
static char *
identStr(char *ident, hestOpt *opt, const hestParm *parm, int brief) {
  char copy[AIR_STRLEN_HUGE + 1], *sep;

  if (opt->flag && (sep = strchr(opt->flag, parm->multiFlagSep))) {
    strcpy(copy, opt->flag);
    sep = strchr(copy, parm->multiFlagSep);
    *sep = '\0';
    if (brief)
      sprintf(ident, "-%s%c--%s option", copy, parm->multiFlagSep, sep + 1);
    else
      sprintf(ident, "-%s option", copy);
  } else {
    sprintf(ident, "%s%s%s option", opt->flag ? "\"-" : "<",
            opt->flag ? opt->flag : opt->name, opt->flag ? "\"" : ">");
  }
  return ident;
}

/*
whichFlag()

given a string in "flag" (with the hypen prefix) finds which of the flags in the given
array of options matches that.  Returns the index of the matching option, or -1 if
there is no match, but returns -2 if the flag is the end-of-parameters
marker "--" (or whatever parm->varParamStopFlag implies)
*/
static int
whichFlag(hestOpt *opt, char *flag, const hestParm *parm) {
  static const char me[] = "whichFlag";
  char buff[2 * AIR_STRLEN_HUGE + 1], copy[AIR_STRLEN_HUGE + 1], *sep;
  int op, numOpts;

  numOpts = hestOptNum(opt);
  if (parm->verbosity)
    printf("%s: (a) looking for flag |%s| in numOpts=%d options\n", me, flag, numOpts);
  for (op = 0; op < numOpts; op++) {
    if (parm->verbosity) printf("%s:      op = %d\n", me, op);
    if (!opt[op].flag) continue;
    if (strchr(opt[op].flag, parm->multiFlagSep)) {
      strcpy(copy, opt[op].flag);
      sep = strchr(copy, parm->multiFlagSep);
      *sep = '\0';
      /* first try the short version */
      sprintf(buff, "-%s", copy);
      if (!strcmp(flag, buff)) return op;
      /* then try the long version */
      sprintf(buff, "--%s", sep + 1);
      if (!strcmp(flag, buff)) return op;
    } else {
      /* flag has only the short version */
      sprintf(buff, "-%s", opt[op].flag);
      if (!strcmp(flag, buff)) return op;
    }
  }
  if (parm->verbosity) printf("%s: (b) numOpts = %d\n", me, numOpts);
  if (parm->varParamStopFlag) {
    sprintf(buff, "-%c", parm->varParamStopFlag);
    if (parm->verbosity)
      printf("%s: does flag |%s| == -parm->varParamStopFlag |%s| ?\n", me, flag, buff);
    if (!strcmp(flag, buff)) {
      if (parm->verbosity) printf("%s: yes, it does! returning -2\n", me);
      return -2;
    }
  }
  if (parm->verbosity) printf("%s: (c) returning -1\n", me);
  return -1;
}

/*
extractToStr: takes "pnum" parameters, starting at "base", out of the given argv, and
puts them into a string WHICH THIS FUNCTION ALLOCATES, and also adjusts the argc value
given as "*argcP".
*/
static char *
extractToStr(int *argcP, char **argv, unsigned int base, unsigned int pnum,
             unsigned int *pnumGot, const hestParm *parm) {
  unsigned int len, pidx, true_pnum;
  char *ret;
  char stops[3] = "";

  if (!pnum) return NULL;

  if (parm) {
    stops[0] = '-';
    stops[1] = parm->varParamStopFlag;
    stops[2] = '\0';
  } /* else stops stays as empty string */

  /* find length of buffer we'll have to allocate */
  len = 0;
  for (pidx = 0; pidx < pnum; pidx++) {
    if (base + pidx == AIR_UINT(*argcP)) {
      /* ran up against end of argv array */
      return NULL;
    }
    if (!strcmp(argv[base + pidx], stops)) {
      /* saw something like "--", so that's the end */
      break;
    }
    /* increment by strlen of current arg */
    len += AIR_UINT(strlen(argv[base + pidx]));
    /* and by 2, for 2 '"'s around quoted parm */
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
    /* add space prior to anticipated next parm */
    if (pidx < true_pnum - 1) strcat(ret, " ");
  }
  for (pidx = base + true_pnum; pidx <= AIR_UINT(*argcP); pidx++) {
    argv[pidx - true_pnum] = argv[pidx];
  }
  *argcP -= true_pnum;
  return ret;
}

/*
extractFlagged()

extracts the parameters associated with all flagged options from the given argc and argv,
storing them in prms[], recording the number of parameters in nprm[], and whether or not
the flagged option appeared in appr[].

The sawP information is not set here, since it is better set at value parsing time, which
happens after defaults are enstated.

This is where, thanks to the action of whichFlag(), "--" (or whatever
parm->varParamStopFlag implies) is used as a marker for the end of a *flagged* variable
parameter option.  AND, the "--" marker is removed from the argv.
*/
static int
extractFlagged(char **prms, unsigned int *nprm, int *appr, int *argcP, char **argv,
               hestOpt *opt, char *err, const hestParm *parm, airArray *pmop) {
  static const char me[] = "extractFlagged: ";
  char ident1[AIR_STRLEN_HUGE + 1], ident2[AIR_STRLEN_HUGE + 1];
  int a, np, flag, endflag, numOpts, op;

  a = 0;
  if (parm->verbosity) printf("%s: *argcP = %d\n", me, *argcP);
  while (a <= *argcP - 1) {
    if (parm->verbosity) {
      printf("%s: ----------------- a = %d -> argv[a] = %s\n", me, a, argv[a]);
    }
    flag = whichFlag(opt, argv[a], parm);
    if (parm->verbosity) printf("%s: A: a = %d -> flag = %d\n", me, a, flag);
    if (!(0 <= flag)) {
      /* not a flag, move on */
      a++;
      if (parm->verbosity) printf("%s: !(0 <= %d), so: continue\n", me, flag);
      continue;
    }
    /* see if we can associate some parameters with the flag */
    if (parm->verbosity) printf("%s: flag = %d; any parms?\n", me, flag);
    np = 0;
    endflag = 0;
    while (np < _hestMax(opt[flag].max) /* */
           && a + np + 1 <= *argcP - 1  /* */
           && -1 == (endflag = whichFlag(opt, argv[a + np + 1], parm))) {
      np++;
      if (parm->verbosity)
        printf("%s: np --> %d with flag = %d; endflag = %d\n", me, np, flag, endflag);
    }
    /* we stopped because we got the max number of parameters, or
       because we hit the end of the command line, or
       because whichFlag() returned something other than -1,
       which means it returned -2, or a valid option index.  If
       we stopped because of whichFlag()'s return value,
       endflag has been set to that return value */
    if (parm->verbosity)
      printf("%s: B: stopped with np = %d; flag = %d; endflag = %d\n", me, np, flag,
             endflag);
    if (np < (int)opt[flag].min) { /* HEY scrutinize casts */
      /* didn't get minimum number of parameters */
      if (!(a + np + 1 <= *argcP - 1)) {
        sprintf(err,
                "%shit end of line before getting %d parameter%s "
                "for %s (got %d)",
                ME, opt[flag].min, opt[flag].min > 1 ? "s" : "",
                identStr(ident1, opt + flag, parm, AIR_TRUE), np);
      } else if (-2 != endflag) {
        sprintf(err, "%shit %s before getting %d parameter%s for %s (got %d)", ME,
                identStr(ident1, opt + endflag, parm, AIR_FALSE), opt[flag].min,
                opt[flag].min > 1 ? "s" : "",
                identStr(ident2, opt + flag, parm, AIR_FALSE), np);
      } else {
        sprintf(err,
                "%shit \"-%c\" (option-parameter-stop flag) before getting %d "
                "parameter%s for %s (got %d)",
                ME, parm->varParamStopFlag, opt[flag].min, opt[flag].min > 1 ? "s" : "",
                identStr(ident2, opt + flag, parm, AIR_FALSE), np);
      }
      return 1;
    }
    nprm[flag] = np;
    if (parm->verbosity) {
      printf("%s:________ a=%d, *argcP = %d -> flag = %d\n", me, a, *argcP, flag);
      printArgv(*argcP, argv, "     ");
    }
    /* lose the flag argument */
    free(extractToStr(argcP, argv, a, 1, NULL, NULL));
    /* extract the args after the flag */
    if (appr[flag]) {
      airMopSub(pmop, prms[flag], airFree);
      prms[flag] = (char *)airFree(prms[flag]);
    }
    prms[flag] = extractToStr(argcP, argv, a, nprm[flag], NULL, NULL);
    airMopAdd(pmop, prms[flag], airFree, airMopAlways);
    appr[flag] = AIR_TRUE;
    if (-2 == endflag) {
      /* we drop the option-parameter-stop flag */
      free(extractToStr(argcP, argv, a, 1, NULL, NULL));
    }
    if (parm->verbosity) {
      printArgv(*argcP, argv, "     ");
      printf("%s:^^^^^^^^ *argcP = %d\n", me, *argcP);
      printf("%s: prms[%d] = %s\n", me, flag, prms[flag] ? prms[flag] : "(null)");
    }
  }

  /* make sure that flagged options without default were given */
  numOpts = hestOptNum(opt);
  for (op = 0; op < numOpts; op++) {
    if (1 != opt[op].kind && opt[op].flag && !opt[op].dflt && !appr[op]) {
      sprintf(err, "%sdidn't get required %s", ME,
              identStr(ident1, opt + op, parm, AIR_FALSE));
      return 1;
    }
  }

  return 0;
}

static int
_hestNextUnflagged(int op, hestOpt *opt, int numOpts) {

  for (; op <= numOpts - 1; op++) {
    if (!opt[op].flag) break;
  }
  return op;
}

static int
extractUnflagged(char **prms, unsigned int *nprm, int *argcP, char **argv, hestOpt *opt,
                 char *err, const hestParm *parm, airArray *pmop) {
  static const char me[] = "extractUnflagged: ";
  char ident[AIR_STRLEN_HUGE + 1];
  int nvp, np, op, unflag1st, unflagVar, numOpts;

  numOpts = hestOptNum(opt);
  unflag1st = _hestNextUnflagged(0, opt, numOpts);
  if (numOpts == unflag1st) {
    /* no unflagged options; we're done */
    return 0;
  }
  if (parm->verbosity) {
    printf("%s numOpts %d != unflag1st %d: have some (of %d) unflagged options\n", me,
           numOpts, unflag1st, numOpts);
  }

  for (unflagVar = unflag1st; unflagVar != numOpts;
       unflagVar = _hestNextUnflagged(unflagVar + 1, opt, numOpts)) {
    if (AIR_INT(opt[unflagVar].min) < _hestMax(opt[unflagVar].max)) {
      break;
    }
  }
  /* now, if there is a variable parameter unflagged opt, unflagVar is its
     index in opt[], or else unflagVar is numOpts */
  if (parm->verbosity) {
    printf("%s unflagVar %d\n", me, unflagVar);
  }

  /* grab parameters for all unflagged opts before opt[t] */
  for (op = _hestNextUnflagged(0, opt, numOpts); op < unflagVar;
       op = _hestNextUnflagged(op + 1, opt, numOpts)) {
    if (parm->verbosity) {
      printf("%s op = %d; unflagVar = %d\n", me, op, unflagVar);
    }
    np = opt[op].min; /* min == max */
    if (!(np <= *argcP)) {
      sprintf(err, "%sdon't have %d parameter%s %s%s%sfor %s", ME, np, np > 1 ? "s" : "",
              argv[0] ? "starting at \"" : "", argv[0] ? argv[0] : "",
              argv[0] ? "\" " : "", identStr(ident, opt + op, parm, AIR_TRUE));
      return 1;
    }
    prms[op] = extractToStr(argcP, argv, 0, np, NULL, NULL);
    airMopAdd(pmop, prms[op], airFree, airMopAlways);
    nprm[op] = np;
  }
  /* we skip over the variable parameter unflagged option, subtract from *argcP
     the number of parameters in all the opts which follow it, in order to get
     the number of parameters in the sole variable parameter option,
     store this in nvp */
  nvp = *argcP;
  for (op = _hestNextUnflagged(unflagVar + 1, opt, numOpts); op < numOpts;
       op = _hestNextUnflagged(op + 1, opt, numOpts)) {
    nvp -= opt[op].min; /* min == max */
  }
  if (nvp < 0) {
    op = _hestNextUnflagged(unflagVar + 1, opt, numOpts);
    np = opt[op].min;
    sprintf(err, "%sdon't have %d parameter%s for %s", ME, np, np > 1 ? "s" : "",
            identStr(ident, opt + op, parm, AIR_FALSE));
    return 1;
  }
  /* else we had enough args for all the unflagged options following
     the sole variable parameter unflagged option, so snarf them up */
  for (op = _hestNextUnflagged(unflagVar + 1, opt, numOpts); op < numOpts;
       op = _hestNextUnflagged(op + 1, opt, numOpts)) {
    np = opt[op].min;
    prms[op] = extractToStr(argcP, argv, nvp, np, NULL, NULL);
    airMopAdd(pmop, prms[op], airFree, airMopAlways);
    nprm[op] = np;
  }

  /* now we grab the parameters of the sole variable parameter unflagged opt,
     if it exists (unflagVar < numOpts) */
  if (parm->verbosity) {
    printf("%s (still here) unflagVar %d vs numOpts %d (nvp %d)\n", me, unflagVar,
           numOpts, nvp);
  }
  if (unflagVar < numOpts) {
    if (parm->verbosity) {
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
        sprintf(err, "%sdidn't get minimum of %d arg%s for %s (got %d)", ME,
                opt[unflagVar].min, opt[unflagVar].min > 1 ? "s" : "",
                identStr(ident, opt + unflagVar, parm, AIR_TRUE), nvp);
        return 1;
      }
      prms[unflagVar] = extractToStr(argcP, argv, 0, nvp, &gotp, parm);
      if (parm->verbosity) {
        printf("%s extracted %u to new string |%s| (*argcP now %d)\n", me, gotp,
               prms[unflagVar], *argcP);
      }
      airMopAdd(pmop, prms[unflagVar], airFree, airMopAlways);
      nprm[unflagVar] = gotp; /* which is < nvp in case of "--" */
    } else {
      prms[unflagVar] = NULL;
      nprm[unflagVar] = 0;
    }
  }
  return 0;
}

static int
_hestDefaults(char **prms, int *udflt, unsigned int *nprm, int *appr, hestOpt *opt,
              char *err, const hestParm *parm, airArray *mop) {
  static const char me[] = "_hestDefaults: ";
  char *tmpS, ident[AIR_STRLEN_HUGE + 1];
  int op, numOpts;

  numOpts = hestOptNum(opt);
  for (op = 0; op < numOpts; op++) {
    if (parm->verbosity)
      printf("%s op=%d/%d: \"%s\" --> kind=%d, nprm=%u, appr=%d\n", me, op, numOpts - 1,
             prms[op], opt[op].kind, nprm[op], appr[op]);
    switch (opt[op].kind) {
    case 1:
      /* -------- (no-parameter) boolean flags -------- */
      /* default is indeed always ignored for the sake of setting the
         option's value, but udflt is used downstream to set
         the option's source. The info came from the user if
         the flag appears, otherwise it is from the default. */
      udflt[op] = !appr[op];
      break;
    case 2:
    case 3:
      /* -------- one required parameter -------- */
      /* -------- multiple required parameters -------- */
      /* we'll used defaults if the flag didn't appear */
      udflt[op] = opt[op].flag && !appr[op];
      break;
    case 4:
      /* -------- optional single variables -------- */
      /* if the flag appeared (if there is a flag) but the parameter didn't,
         we'll "invert" the default; if the flag didn't appear (or if there
         isn't a flag) and the parameter also didn't appear, we'll use the
         default.  In either case, nprm[op] will be zero, and in both cases,
         we need to use the default information. */
      udflt[op] = (0 == nprm[op]);
      /* fprintf(stderr, "%s nprm[%d] = %u --> udflt[%d] = %d\n", me,
       *       op, nprm[op], op, udflt[op]); */
      break;
    case 5:
      /* -------- multiple optional parameters -------- */
      /* we'll use the default if there is a flag and it didn't appear,
         Otherwise (with a flagged option), if nprm[op] is zero, we'll use the default
         if user has given zero parameters, yet the the option requires at least one.
         If an unflagged option can have zero parms, and user has given zero parms,
         then we don't use the default */
      udflt[op] = (opt[op].flag
                     ? !appr[op] /* option is flagged and flag didn't appear */
                     /* else: option is unflagged, and there were no given parms,
                     and yet the option requires at least one parm */
                     : !nprm[op] && opt[op].min >= 1);
      /* fprintf(stderr,
       *       "!%s: opt[%d].flag = %d; appr[op] = %d; nprm[op] = %d; opt[op].min = %d "
       *       "--> udflt[op] = %d\n",
       *       me, op, !!opt[op].flag, appr[op], nprm[op], opt[op].min, udflt[op]); */
      break;
    }
    /* if not using the default, we're done with this option */
    if (!udflt[op]) continue;
    prms[op] = airStrdup(opt[op].dflt);
    if (parm->verbosity) {
      printf("%s: prms[%d] = |%s|\n", me, op, prms[op]);
    }
    if (prms[op]) {
      airMopAdd(mop, prms[op], airFree, airMopAlways);
      airOneLinify(prms[op]);
      tmpS = airStrdup(prms[op]);
      nprm[op] = airStrntok(tmpS, " ");
      airFree(tmpS);
    }
    /* fprintf(stderr,
     *       "!%s: after default; nprm[%d] = %u; varparm = %d (min %d vs max %d)\n", me,
     *       op, nprm[op], AIR_INT(opt[op].min) < _hestMax(opt[op].max),
     *       ((int)opt[op].min), _hestMax(opt[op].max)); */
    if (AIR_INT(opt[op].min) < _hestMax(opt[op].max)) {
      if (!AIR_IN_CL(AIR_INT(opt[op].min), AIR_INT(nprm[op]), _hestMax(opt[op].max))) {
        if (-1 == opt[op].max) {
          sprintf(err, "%s# parameters (in default) for %s is %d, but need %d or more",
                  ME, identStr(ident, opt + op, parm, AIR_TRUE), nprm[op], opt[op].min);
        } else {
          sprintf(err,
                  "%s# parameters (in default) for %s is %d, but need between %d and %d",
                  ME, identStr(ident, opt + op, parm, AIR_TRUE), nprm[op], opt[op].min,
                  _hestMax(opt[op].max));
        }
        return 1;
      }
    }
  }
  return 0;
}

/*
** this function moved from air/miscAir; the usage below
** is its only usage in Teem
*/
static int
airIStore(void *v, int t, int i) {

  switch (t) {
  case airTypeBool:
    return (*((int *)v) = !!i);
    break;
  case airTypeInt:
    return (*((int *)v) = i);
    break;
  case airTypeUInt:
    return (int)(*((unsigned int *)v) = i);
    break;
  case airTypeLongInt:
    return (int)(*((long int *)v) = i);
    break;
  case airTypeULongInt:
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
** this function moved from air/miscAir; the usage below
** is its only usage in Teem
*/
static double
airDLoad(void *v, int t) {

  switch (t) {
  case airTypeBool:
    return AIR_CAST(double, *((int *)v));
    break;
  case airTypeInt:
    return AIR_CAST(double, *((int *)v));
    break;
  case airTypeUInt:
    return AIR_CAST(double, *((unsigned int *)v));
    break;
  case airTypeLongInt:
    return AIR_CAST(double, *((long int *)v));
    break;
  case airTypeULongInt:
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
whichCase(hestOpt *opt, int *udflt, unsigned int *nprm, int *appr, int op) {

  if (opt[op].flag && !appr[op]) {
    return 0;
  } else if ((4 == opt[op].kind && udflt[op]) || (5 == opt[op].kind && !nprm[op])) {
    return 1;
  } else {
    return 2;
  }
}

static int
setValues(char **prms, int *udflt, unsigned int *nprm, int *appr, hestOpt *opt,
          char *err, const hestParm *parm, airArray *pmop) {
  static const char me[] = "setValues: ";
  char ident[AIR_STRLEN_HUGE + 1], cberr[AIR_STRLEN_HUGE + 1], *tok, *last, *prmsCopy;
  double tmpD;
  int op, type, numOpts, p, ret;
  void *vP;
  char *cP;
  size_t size;

  numOpts = hestOptNum(opt);
  for (op = 0; op < numOpts; op++) {
    identStr(ident, opt + op, parm, AIR_TRUE);
    opt[op].source = udflt[op] ? hestSourceDefault : hestSourceUser;
    /* 2023 GLK notes that r6388 2020-05-14 GLK was asking:
        How is it that, once the command-line has been parsed, there isn't an
        easy way to see (or print, for an error message) the parameter (or
        concatenation of parameters) that was passed for a given option?
    and it turns out that adding this was as simple as adding this one following
    line. The inscrutability of the hest code (or really the self-reinforcing
    learned fear of working with the hest code) seems to have been the barrier. */
    opt[op].parmStr = airStrdup(prms[op]);
    type = opt[op].type;
    size = (airTypeEnum == type /* */
              ? sizeof(int)
              : (airTypeOther == type /* */
                   ? opt[op].CB->size
                   : airTypeSize[type]));
    cP = (char *)(vP = opt[op].valueP);
    if (parm->verbosity) {
      printf("%s %d of %d: \"%s\": |%s| --> kind=%d, type=%d, size=%u\n", me, op,
             numOpts - 1, prms[op], ident, opt[op].kind, type, (unsigned int)size);
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
      /* 2023 GLK is really curious why "if (prms[op] && vP) {" is (​repeatedly)
      guarding all the work in these blocks, and why that wasn't factored out */
      if (prms[op] && vP) {
        switch (type) {
        case airTypeEnum:
          if (1 != airParseStrE((int *)vP, prms[op], " ", 1, opt[op].enm)) {
            sprintf(err, "%scouldn\'t parse %s\"%s\" as %s for %s", ME,
                    udflt[op] ? "(default) " : "", prms[op], opt[op].enm->name, ident);
            return 1;
          }
          break;
        case airTypeOther:
          strcpy(cberr, "");
          ret = opt[op].CB->parse(vP, prms[op], cberr);
          if (ret) {
            if (strlen(cberr)) {
              sprintf(err, "%serror parsing \"%s\" as %s for %s:\n%s", ME, prms[op],
                      opt[op].CB->type, ident, cberr);
            } else {
              sprintf(err, "%serror parsing \"%s\" as %s for %s: returned %d", ME,
                      prms[op], opt[op].CB->type, ident, ret);
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
              != airParseStrS((char **)vP, prms[op], " ", 1, parm->greedySingleString)) {
            sprintf(err, "%scouldn't parse %s\"%s\" as %s for %s", ME,
                    udflt[op] ? "(default) " : "", prms[op], airTypeStr[type], ident);
            return 1;
          }
          /* vP is the address of a char* (a char **), but what we
             manage with airMop is the char * */
          opt[op].alloc = 1;
          airMopMem(pmop, vP, airMopOnError);
          break;
        default:
          /* type isn't string or enum, so no last arg to airParseStr[type] */
          if (1 != airParseStr[type](vP, prms[op], " ", 1)) {
            sprintf(err, "%scouldn't parse %s\"%s\" as %s for %s", ME,
                    udflt[op] ? "(default) " : "", prms[op], airTypeStr[type], ident);
            return 1;
          }
          break;
        }
      }
      break;
    case 3:
      /* -------- multiple required parameters -------- */
      if (prms[op] && vP) {
        switch (type) {
        case airTypeEnum:
          if (opt[op].min != /* min == max */
              airParseStrE((int *)vP, prms[op], " ", opt[op].min, opt[op].enm)) {
            sprintf(err, "%scouldn't parse %s\"%s\" as %d %s%s for %s", ME,
                    udflt[op] ? "(default) " : "", prms[op], opt[op].min,
                    opt[op].enm->name, opt[op].min > 1 ? "s" : "", ident);
            return 1;
          }
          break;
        case airTypeOther:
          prmsCopy = airStrdup(prms[op]);
          for (p = 0; p < (int)opt[op].min; p++) { /* HEY scrutinize casts */
            tok = airStrtok(!p ? prmsCopy : NULL, " ", &last);
            strcpy(cberr, "");
            ret = opt[op].CB->parse(cP + p * size, tok, cberr);
            if (ret) {
              if (strlen(cberr))
                sprintf(err,
                        "%serror parsing \"%s\" (in \"%s\") as %s "
                        "for %s:\n%s",
                        ME, tok, prms[op], opt[op].CB->type, ident, cberr);
              else
                sprintf(err,
                        "%serror parsing \"%s\" (in \"%s\") as %s "
                        "for %s: returned %d",
                        ME, tok, prms[op], opt[op].CB->type, ident, ret);
              free(prmsCopy);
              return 1;
            }
          }
          free(prmsCopy);
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
              airParseStr[type](vP, prms[op], " ", opt[op].min, AIR_FALSE)) {
            sprintf(err, "%scouldn't parse %s\"%s\" as %d %s%s for %s", ME,
                    udflt[op] ? "(default) " : "", prms[op], opt[op].min,
                    airTypeStr[type], opt[op].min > 1 ? "s" : "", ident);
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
              airParseStr[type](vP, prms[op], " ", opt[op].min)) {
            sprintf(err, "%scouldn't parse %s\"%s\" as %d %s%s for %s", ME,
                    udflt[op] ? "(default) " : "", prms[op], opt[op].min,
                    airTypeStr[type], opt[op].min > 1 ? "s" : "", ident);
            return 1;
          }
          break;
        }
      }
      break;
    case 4:
      /* -------- optional single variables -------- */
      if (prms[op] && vP) {
        switch (type) {
        case airTypeChar:
          /* no "inversion" for chars: using the flag with no parameter is the same as
          not using the flag i.e. we just parse from the default string */
          if (1 != airParseStr[type](vP, prms[op], " ", 1)) {
            sprintf(err, "%scouldn't parse %s\"%s\" as %s for %s", ME,
                    udflt[op] ? "(default) " : "", prms[op], airTypeStr[type], ident);
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
          persists.*/
          if (1 != airParseStr[type](vP, prms[op], " ", 1, parm->greedySingleString)) {
            sprintf(err, "%scouldn't parse %s\"%s\" as %s for %s", ME,
                    udflt[op] ? "(default) " : "", prms[op], airTypeStr[type], ident);
            return 1;
          }
          opt[op].alloc = 1;
          if (opt[op].flag && 1 == whichCase(opt, udflt, nprm, appr, op)) {
            /* we just parsed the default, but now we want to "invert" it */
            *((char **)vP) = (char *)airFree(*((char **)vP));
            opt[op].alloc = 0;
          }
          /* vP is the address of a char* (a char**), and what we
             manage with airMop is the char * */
          airMopMem(pmop, vP, airMopOnError);
          break;
        case airTypeEnum:
          if (1 != airParseStrE((int *)vP, prms[op], " ", 1, opt[op].enm)) {
            sprintf(err, "%scouldn't parse %s\"%s\" as %s for %s", ME,
                    udflt[op] ? "(default) " : "", prms[op], opt[op].enm->name, ident);
            return 1;
          }
          break;
        case airTypeOther:
          /* we're parsing an single single "other".  We will not perform the special
             flagged single variable parameter games as done above, so
             whether this option is flagged or unflagged, we're going
             to treat it like an unflagged single variable parameter option:
             if the parameter didn't appear, we'll parse it from the default,
             if it did appear, we'll parse it from the command line.  Setting
             up prms[op] thusly has already been done by _hestDefaults() */
          strcpy(cberr, "");
          ret = opt[op].CB->parse(vP, prms[op], cberr);
          if (ret) {
            if (strlen(cberr))
              sprintf(err, "%serror parsing \"%s\" as %s for %s:\n%s", ME, prms[op],
                      opt[op].CB->type, ident, cberr);
            else
              sprintf(err, "%serror parsing \"%s\" as %s for %s: returned %d", ME,
                      prms[op], opt[op].CB->type, ident, ret);
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
          if (1 != airParseStr[type](vP, prms[op], " ", 1)) {
            sprintf(err, "%scouldn't parse %s\"%s\" as %s for %s", ME,
                    udflt[op] ? "(default) " : "", prms[op], airTypeStr[type], ident);
            return 1;
          }
          opt[op].alloc = 0;
          /* HEY sorry about confusion about hestOpt->parmStr versus the value set
          here, due to this "inversion" */
          if (1 == whichCase(opt, udflt, nprm, appr, op)) {
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
      if (prms[op] && vP) {
        if (1 == whichCase(opt, udflt, nprm, appr, op)) {
          *((void **)vP) = NULL;
          /* alloc and sawP set above */
        } else {
          if (airTypeString == type) {
            /* this is sneakiness: we allocate one more element so that
               the resulting char** is, like argv, NULL-terminated */
            *((void **)vP) = calloc(nprm[op] + 1, size);
          } else {
            if (nprm[op]) {
              /* only allocate if there's something to allocate */
              *((void **)vP) = calloc(nprm[op], size);
            } else {
              *((void **)vP) = NULL;
            }
          }
          if (parm->verbosity) {
            printf("%s: nprm[%d] = %u\n", me, op, nprm[op]);
            printf("%s: new array (size %u*%u) is at 0x%p\n", me, nprm[op],
                   (unsigned int)size, *((void **)vP));
          }
          if (*((void **)vP)) {
            airMopMem(pmop, vP, airMopOnError);
          }
          *(opt[op].sawP) = nprm[op];
          /* so far everything we've done is regardless of type */
          switch (type) {
          case airTypeEnum:
            opt[op].alloc = 1;
            if (nprm[op]
                != airParseStrE((int *)(*((void **)vP)), prms[op], " ", nprm[op],
                                opt[op].enm)) {
              sprintf(err, "%scouldn't parse %s\"%s\" as %u %s%s for %s", ME,
                      udflt[op] ? "(default) " : "", prms[op], nprm[op],
                      opt[op].enm->name, nprm[op] > 1 ? "s" : "", ident);
              return 1;
            }
            break;
          case airTypeOther:
            cP = (char *)(*((void **)vP));
            prmsCopy = airStrdup(prms[op]);
            opt[op].alloc = (opt[op].CB->destroy ? 3 : 1);
            for (p = 0; p < (int)nprm[op]; p++) { /* HEY scrutinize casts */
              tok = airStrtok(!p ? prmsCopy : NULL, " ", &last);
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
                  sprintf(err,
                          "%serror parsing \"%s\" (in \"%s\") as %s "
                          "for %s:\n%s",
                          ME, tok, prms[op], opt[op].CB->type, ident, cberr);

                else
                  sprintf(err,
                          "%serror parsing \"%s\" (in \"%s\") as %s "
                          "for %s: returned %d",
                          ME, tok, prms[op], opt[op].CB->type, ident, ret);
                free(prmsCopy);
                return 1;
              }
            }
            free(prmsCopy);
            if (opt[op].CB->destroy) {
              for (p = 0; p < (int)nprm[op]; p++) { /* HEY scrutinize casts */
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
            if (nprm[op]
                != airParseStrS((char **)(*((void **)vP)), prms[op], " ", nprm[op],
                                parm->greedySingleString)) {
              sprintf(err, "%scouldn't parse %s\"%s\" as %d %s%s for %s", ME,
                      udflt[op] ? "(default) " : "", prms[op], nprm[op],
                      airTypeStr[type], nprm[op] > 1 ? "s" : "", ident);
              return 1;
            }
            /* vP is the address of an array of char*s (a char ***), and
               what we manage with airMop is the individual (*vP)[p],
               as well as vP itself (above). */
            for (p = 0; p < (int)nprm[op]; p++) { /* HEY scrutinize casts */
              airMopAdd(pmop, (*((char ***)vP))[p], airFree, airMopOnError);
            }
            /* do the NULL-termination described above */
            (*((char ***)vP))[nprm[op]] = NULL;
            break;
          default:
            opt[op].alloc = 1;
            if (nprm[op] != airParseStr[type](*((void **)vP), prms[op], " ", nprm[op])) {
              sprintf(err, "%scouldn't parse %s\"%s\" as %d %s%s for %s", ME,
                      udflt[op] ? "(default) " : "", prms[op], nprm[op],
                      airTypeStr[type], nprm[op] > 1 ? "s" : "", ident);
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
hestParse(hestOpt *opt, int _argc, const char **_argv, char **_errP,
          const hestParm *_parm) {
  static const char me[] = "hestParse: ";
  char *param, *param_copy;
  char **argv, **prms, *err;
  int a, argc, argc_used, argr, *appr, *udflt, nrf, numOpts, big, ret, i, sawHelp;
  unsigned int *nprm;
  airArray *mop;
  hestParm *parm;
  size_t start_index, end_index;

  numOpts = hestOptNum(opt);

  /* -------- initialize the mop! */
  mop = airMopNew();

  /* -------- either copy given _parm, or allocate one */
  if (_parm) {
    parm = NULL;
  } else {
    parm = hestParmNew();
    airMopAdd(mop, parm, (airMopper)hestParmFree, airMopAlways);
  }
  /* how to const-correctly use parm or _parm in an expression */
#define PARM (_parm ? _parm : parm)

  /* -------- allocate the err string.  To determine its size with total
     ridiculous safety we have to find the biggest things which can appear
     in the string. */
  big = _hestErrStrlen(opt, _argc, _argv);
  if (!(err = AIR_CALLOC(big, char))) {
    fprintf(stderr,
            "%s PANIC: couldn't allocate error message "
            "buffer (size %d)\n",
            me, big);
    /* exit(1); */
  }
  if (_errP) {
    /* if they care about the error string, than it is mopped only
       when there _wasn't_ an error */
    *_errP = err;
    airMopAdd(mop, _errP, (airMopper)airSetNull, airMopOnOkay);
    airMopAdd(mop, err, airFree, airMopOnOkay);
  } else {
    /* otherwise, we're making the error string just for our own
       convenience, and we'll always clean it up on exit */
    airMopAdd(mop, err, airFree, airMopAlways);
  }

  /* -------- check on validity of the hestOpt array */
  if (_hestPanic(opt, err, PARM)) {
    airMopError(mop);
    return 1;
  }

  /* -------- Create all the local arrays used to save state during
     the processing of all the different options */
  nprm = AIR_CALLOC(numOpts, unsigned int);
  airMopMem(mop, &nprm, airMopAlways);
  appr = AIR_CALLOC(numOpts, int);
  airMopMem(mop, &appr, airMopAlways);
  udflt = AIR_CALLOC(numOpts, int);
  airMopMem(mop, &udflt, airMopAlways);
  prms = AIR_CALLOC(numOpts, char *);
  airMopMem(mop, &prms, airMopAlways);
  for (a = 0; a < numOpts; a++) {
    prms[a] = NULL;
  }

  /* -------- find out how big the argv array needs to be, first
     by seeing how many args are in the response files, and then adding
     on the args from the actual argv (getting this right the first time
     greatly simplifies the problem of eliminating memory leaks) */
  if (_hestArgsInResponseFiles(&argr, &nrf, _argv, err, PARM)) {
    airMopError(mop);
    return 1;
  }
  argc = argr + _argc - nrf;

  if (PARM->verbosity) {
    printf("%s: nrf = %d; argr = %d; _argc = %d --> argc = %d\n", me, nrf, argr, _argc,
           argc);
  }
  argv = AIR_CALLOC(argc + 1, char *);
  airMopMem(mop, &argv, airMopAlways);

  /* -------- process response files (if any) and set the remaining
     elements of argv */
  opt->helpWanted = AIR_FALSE;
  if (PARM->verbosity) printf("%s: #### calling copyArgv\n", me);
  argc_used = copyArgv(&sawHelp, argv, _argv, PARM, mop);
  if (PARM->verbosity) {
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
    sprintf(err, "%sargc_used %d < argc %d; sorry, confused", ME, argc_used, argc);
    airMopError(mop);
    return 1;
  }

  /* -------- extract flags and their associated parameters from argv */
  if (PARM->verbosity) printf("%s: #### calling extractFlagged\n", me);
  if (extractFlagged(prms, nprm, appr, &argc_used, argv, opt, err, PARM, mop)) {
    airMopError(mop);
    return 1;
  }
  if (PARM->verbosity) printf("%s: #### extractFlagged done!\n", me);

  /* -------- extract args for unflagged options */
  if (PARM->verbosity) printf("%s: #### calling extractUnflagged\n", me);
  if (extractUnflagged(prms, nprm, &argc_used, argv, opt, err, PARM, mop)) {
    airMopError(mop);
    return 1;
  }
  if (PARM->verbosity) printf("%s: #### extractUnflagged done!\n", me);

  /* currently, any left-over arguments indicate error */
  if (argc_used) {
    /* char stops[3] = {'-', PARM->varParamStopFlag, '\0'}; triggers warning:
    initializer element is not computable at load time */
    char stops[3] = "-X";
    stops[1] = PARM->varParamStopFlag;
    if (strcmp(stops, argv[0])) {
      sprintf(err, "%sunexpected arg%s: \"%s\"", ME,
              ('-' == argv[0][0] ? " (or unrecognized flag)" : ""), argv[0]);
    } else {
      sprintf(err,
              "%sunexpected end-of-parameters flag \"%s\": "
              "not ending a flagged variable-parameter option",
              ME, stops);
    }
    airMopError(mop);
    return 1;
  }

  /* -------- learn defaults */
  if (PARM->verbosity) printf("%s: #### calling hestDefaults\n", me);
  if (_hestDefaults(prms, udflt, nprm, appr, opt, err, PARM, mop)) {
    airMopError(mop);
    return 1;
  }
  if (PARM->verbosity) printf("%s: #### hestDefaults done!\n", me);

  /* remove quotes from strings
         if greedy wasn't turned on for strings, then we have no hope
         of capturing filenames with spaces. */
  if (PARM->greedySingleString) {
    for (i = 0; i < numOpts; i++) {
      param = prms[i];
      param_copy = NULL;
      if (param && strstr(param, " ")) {
        start_index = 0;
        end_index = strlen(param) - 1;
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

  /* -------- now, the actual parsing of values */
  if (PARM->verbosity) printf("%s: #### calling setValues\n", me);
  /* this will also set hestOpt->parmStr */
  ret = setValues(prms, udflt, nprm, appr, opt, err, PARM, mop);
  if (ret) {
    airMopError(mop);
    return ret;
  }

  if (PARM->verbosity) printf("%s: #### setValues done!\n", me);
#undef PARM

parseEnd:
  airMopOkay(mop);
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
  int op, i, numOpts;
  unsigned int ui;
  void **vP;
  void ***vAP;
  char **str;
  char ***strP;

  numOpts = hestOptNum(opt);
  for (op = 0; op < numOpts; op++) {
    opt[op].parmStr = airFree(opt[op].parmStr);
    /*
    printf("!hestParseFree: op = %d/%d -> kind = %d; type = %d; alloc = %d\n",
           op, numOpts-1, opt[op].kind, opt[op].type, opt[op].alloc);
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
** if argc is 0 (and !parm->noArgsIsNoProblem): maybe show
**    info, usage, and glossary, all according to given flags, then exit(1)
** if parsing failed: show error message, and maybe usage and glossary,
**    again according to boolean flags, then exit(1)
** if parsing succeeded: return

In June 2023 this function was completely re-written, but the description above should
still be true, if not the whole truth.  Long prior to re-write, "--version" and "--help"
had been usefully responded to as the sole argument, but only after a hestParse error
(which then sometimes leaked memory by not freeing errS). Now these are checked for prior
to calling hestParse, and (with parm->respectDashDashHelp), "--help" is recognized
anywhere in the command-line.
*/
void
hestParseOrDie(hestOpt *opt, int argc, const char **argv, hestParm *parm, const char *me,
               const char *info, int doInfo, int doUsage, int doGlossary) {
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
    hestParmFree(parm);
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
    if (argc || (parm && parm->noArgsIsNoProblem)) {
      argcWanting = AIR_FALSE;
      parseErr = hestParse(opt, argc, argv, &errS, parm);
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
  if (parm && parm->dieLessVerbose) {
    /* newer logic for when to print which things */
    if (wantHelp && info) hestInfo(STDWUT, me ? me : "", info, parm);
    if (doUsage) hestUsage(STDWUT, opt, me ? me : "", parm);
    if (wantHelp && doGlossary) {
      hestGlossary(STDWUT, opt, parm);
    } else if ((!argc || parseErr) && me) {
      fprintf(STDWUT, "\"%s --help\" for more information\n", me);
    }
  } else {
    /* leave older (pre-dieLessVerbose) logic as is */
    if (!parseErr) {
      /* no error, just !argc */
      if (doInfo && info) hestInfo(stdout, me ? me : "", info, parm);
    }
    if (doUsage) hestUsage(STDWUT, opt, me ? me : "", parm);
    if (doGlossary) hestGlossary(STDWUT, opt, parm);
  }
#undef STDWUT
  hestParmFree(parm);
  hestOptFree(opt);
  exit(!!parseErr);
}
