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

#define INFO "Converts DOS text files to normal, and more"
static const char *_unrrdu_undosInfoL
  = (INFO ". The characters involved are:\n *\t\t\t\t\t\t\t"
          "       carraige return = CR = '\\r' = 0x0d = decimal 13 = octal 015\n "
          "* \"new line\" = line feed = LF = '\\n' = 0x0a = decimal 10 = octal 012\n "
          "though see https://en.wikipedia.org/wiki/Newline for messy details. "
          "This program converts CR-LF pairs (DOS/Windows line breaks) "
          "to just LF (Unix line break). With the \"-r\" option, however, "
          "this converts the other way, for whatever sick reason you'd want that. "
          "With the \"-m\" option, this can convert legacy MAC text files "
          "(which use only CR for line break, which may appear as \"^M\" in text "
          "displays). Unlike simple sed/perl scripts for this purpose, this program "
          "is careful to be idempotent in all modes of operation. Also, this makes an "
          "effort to not meddle with binary files (on which this may be mistakenly "
          "invoked), by not converting files with a high percentage of non-printing "
          "characters, as controlled by the \"-pnp\" option. A message "
          "is printed to stderr for all the files actually modified.\n "
          "* (not actually based on Nrrd)");

#define CR 0x0d
#define LF 0x0a

static void
undosConvert(const char *me, const char *name, int reverse, int mac, int quiet,
             int noAction, float badPerc) {
  airArray *mop;
  FILE *fin, *fout;
  char *data = NULL;
  airArray *dataArr;
  unsigned int ci, len;
  int car, numBad, willConvert;
  airPtrPtrUnion appu;

  mop = airMopNew();
  if (!airStrlen(name)) {
    fprintf(stderr, "%s: empty filename\n", me);
    airMopError(mop);
    return;
  }

  /* -------------------------------------------------------- */
  /* open input file  */
  fin = airFopen(name, stdin, "rb");
  if (!fin) {
    if (!quiet) {
      fprintf(stderr, "%s: couldn't open \"%s\" for reading: \"%s\"\n", me, name,
              strerror(errno));
    }
    airMopError(mop);
    return;
  }
  airMopAdd(mop, fin, (airMopper)airFclose, airMopOnError);

  /* -------------------------------------------------------- */
  /* create buffer */
  appu.c = &data;
  dataArr = airArrayNew(appu.v, NULL, sizeof(char), AIR_STRLEN_HUGE);
  if (!dataArr) {
    if (!quiet) {
      fprintf(stderr, "%s: internal allocation error #1\n", me);
    }
    airMopError(mop);
    return;
  }
  airMopAdd(mop, dataArr, (airMopper)airArrayNuke, airMopAlways);

  /* -------------------------------------------------------- */
  /* read input file, testing for binary-ness along the way */
  numBad = 0;
  car = getc(fin);
  if (EOF == car) {
    if (!quiet) {
      fprintf(stderr, "%s: \"%s\" is empty, skipping ...\n", me, name);
    }
    airMopError(mop);
    return;
  }
  do {
    ci = airArrayLenIncr(dataArr, 1);
    if (!dataArr->data) {
      if (!quiet) {
        fprintf(stderr, "%s: internal allocation error #2\n", me);
      }
      airMopError(mop);
      return;
    }
    data[ci] = AIR_CAST(char, car);
    numBad += !isprint(car) && !isspace(car);
    car = getc(fin);
  } while (EOF != car && badPerc > 100.0 * numBad / dataArr->len);
  if (EOF != car) {
    if (!quiet) {
      fprintf(stderr,
              "%s: more than %g%% of \"%s\" is non-printing, "
              "skipping ...\n",
              me, badPerc, name);
    }
    airMopError(mop);
    return;
  }
  fin = airFclose(fin);
  len = dataArr->len; /* learn array length */

  /* -------------------------------------------------------- */
  /* see if we really need to do anything */
  willConvert = AIR_FALSE;
  if (!strcmp("-", name)) {
    willConvert = AIR_TRUE;
  } else if (reverse) { /* REVERSE operation, away from unix LF */
    for (ci = 0; ci < len; ci++) {
      if (LF == data[ci] && (ci && CR != data[ci - 1])) {
        /* If converting to DOS, we're looking for LF not preceded by CR.
           If converting to MAC, we could just look for LF, but the
           principle here is that we are only converting from unix,
           so a DOS CR-LF should also pass through unchanged */
        willConvert = AIR_TRUE;
        break;
      }
    }
  } else { /* !reverse, normal operation */
    for (ci = 0; ci < len; ci++) {
      if (mac) {
        if (CR == data[ci] && (ci + 1 < len && LF != data[ci + 1])) {
          /* If converting from MAC, our job is NOT to convert DOS CR-LFs */
          willConvert = AIR_TRUE;
          break;
        }
      } else {
        if (CR == data[ci] && (ci + 1 < len && LF == data[ci + 1])) {
          willConvert = AIR_TRUE;
          break;
        }
      }
    }
  }
  if (!willConvert) {
    /* no, we don't need to do anything; quietly quit */
    airMopOkay(mop);
    return;
  } else {
    if (!quiet) {
      fprintf(stderr, "%s: %s \"%s\" %s %s ... \n", me,
              noAction ? "would convert" : "converting", name, reverse ? "to" : "from",
              mac ? "MAC" : "DOS");
    }
  }
  if (noAction) {
    /* just joking, we won't actually write anything.
       (yes, even if input was stdin) */
    airMopOkay(mop);
    return;
  }

  /* -------------------------------------------------------- */
  /* open output file */
  fout = airFopen(name, stdout, "wb");
  if (!fout) {
    if (!quiet) {
      fprintf(stderr, "%s: couldn't open \"%s\" for writing: \"%s\"\n", me, name,
              strerror(errno));
    }
    airMopError(mop);
    return;
  }
  airMopAdd(mop, fout, (airMopper)airFclose, airMopOnError);

  /* -------------------------------------------------------- */
  /* write output file */
  car = 'a';     /* something not EOF */
  if (reverse) { /* away from LF to either CR-LF (or mac CR) */
    for (ci = 0; EOF != car && ci < len; ci++) {
      if (LF == data[ci]) {
        if (ci && CR == data[ci - 1]) {
          /* if this LF was preceded by a CR, it is, if !mac, already
             the intended DOS CR-LF, and we've already putc the CR, so
             now we putc LF. Or, if mac, we're narrowly focusing on
             converting only unix line breaks, so same thing: putc LF */
          car = putc(LF, fout);
        } else {
          /* this LF was not preceded by a CR, so do either MAC or DOS
             line break CR and LF */
          car = putc(CR, fout);
          if (!mac && EOF != car) car = putc(LF, fout);
        }
      } else {
        car = putc(data[ci], fout);
      }
    }
  } else { /* normal operation: from CR-LF (or mac CR but not CR-LF) to LF */
    for (ci = 0; EOF != car && ci < len; ci++) {
      if (CR == data[ci]) {
        if (mac) {
          if (ci + 1 < len && LF == data[ci + 1]) {
            /* not our job to convert CR-LF, so this CR passes through */
            car = putc(CR, fout);
          } else {
            /* just CR --> LF */
            car = putc(LF, fout);
          }
        } else {
          if (ci + 1 < len && LF == data[ci + 1]) {
            /* converting CR-LF to LF is our job */
            car = putc(LF, fout);
            ci++;
          } else {
            /* just a CR, not a CR-LF, but !mac, so this CR passes through */
            car = putc(CR, fout);
          }
        }
      } else {
        /* saw something other than CR */
        car = putc(data[ci], fout);
      }
    }
  }
  if (EOF == car) {
    if (!quiet) {
      fprintf(stderr,
              "%s: ERROR writing \"%s\" possible data loss !!! "
              "(sorry)\n",
              me, name);
    }
  }
  fout = airFclose(fout);

  airMopOkay(mop);
  return;
}

static int
unrrdu_undosMain(int argc, const char **argv, const char *me, hestParm *hparm) {
  /* these are stock for unrrdu */
  hestOpt *opt = NULL;
  airArray *mop;
  int pret;
  char *err;
  /* these are specific to this command */
  char **name;
  float badPerc;
  int reverse, quiet, noAction, mac;
  unsigned int ni, lenName;

  hestOptAdd_Flag(&opt, "r", &reverse,
                  "convert back to DOS, instead of converting from DOS to normal");
  hestOptAdd_Flag(&opt, "q", &quiet, "never print anything to stderr, even for errors.");
  hestOptAdd_Flag(&opt, "m", &mac,
                  "deal with legacy MAC text files, and files generated by "
                  "weird software running on OSX that hasn't gotten the memo "
                  "about OSX uses unix-style line breaks.");
  hestOptAdd_Flag(&opt, "n", &noAction,
                  "don't actually write converted files, just pretend to. "
                  "This is useful to see which files WOULD be converted. ");
  hestOptAdd_1_Float(&opt, "pnp", "perc", &badPerc, "2",
                     "if the percentage of non-printing characters (characters "
                     "c for which isprint(c) and isspace(c) are both false) "
                     "exceeds this threshold, then don't do anything to the file; "
                     "it is probably not a text file at all. ");
  hestOptAdd_Nv_String(&opt, NULL, "file", 1, -1, &name, NULL,
                       "all the files to convert.  Each file will be over-written "
                       "with its converted contents.  Use \"-\" to read from stdin "
                       "and write to stdout",
                       &lenName);

  mop = airMopNew();
  airMopAdd(mop, opt, hestOptFree_vp, airMopAlways);
  USAGE_OR_PARSE(_unrrdu_undosInfoL);
  airMopAdd(mop, opt, (airMopper)hestParseFree, airMopAlways);

  for (ni = 0; ni < lenName; ni++) {
    undosConvert(me, name[ni], reverse, mac, quiet, noAction, badPerc);
  }

  airMopOkay(mop);
  return 0;
}

UNRRDU_CMD_HIDE(undos, INFO);

#if 0 /* this is the program used for testing */
#  include <stdio.h>
#  include <math.h>

int
main(int argc, char **argv) {

#  if 0
#    define MM "M\r"   /* 4d 0d */
#    define DD "D\r\n" /* 44 0d 0a */
#    define UU "U\n"   /* 55 0a */
#  else
#    define MM "M\r\r"
#    define DD "D\r\n\r\n"
#    define UU "U\n\n"
#  endif

  if (2 == argc && 'M' == argv[1][0]) {
    printf(DD UU MM);
  } else if (2 == argc && 'D' == argv[1][0]) {
    printf(UU MM DD);
  } else {
    printf(MM DD UU);
  }
  return 0;
}
#endif
