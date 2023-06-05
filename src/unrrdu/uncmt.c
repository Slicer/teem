/*
  Teem: Tools to process and visualize scientific data and images
  Copyright (C) 2009--2019  University of Chicago
  Copyright (C) 2008, 2007, 2006, 2005  Gordon Kindlmann
  Copyright (C) 2004, 2003, 2002, 2001, 2000, 1999, 1998  University of Utah

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public License
  (LGPL) as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  The terms of redistributing and/or modifying this software also
  include exceptions to the LGPL that facilitate static linking.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with this library; if not, write to Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "unrrdu.h"
#include "privateUnrrdu.h"

#define INFO "Removes comments from a C99 input file"
static const char *_unrrdu_uncmtInfoL
  = (INFO
     ".\n "
     "This is useful for a class GLK teaches, wherein students are told not to use\n "
     "types \"float\" or \"double\" directly (instead they use a class-specific\n "
     "\"real\" typedef). Grepping for \"float\" and \"double\" isn't informative\n "
     "since they can show up in comments; hence the need for this. Catching\n "
     "implicit conversions between floating point precisions is handled separately,\n "
     "in case you were thinking about that.\n"
     "* (not actually based on Nrrd)");

enum {
  stateSlash,  /* 0: just got a '/' */
  stateSAcmt,  /* 1: in Slash Asterisk (traditional C) comment */
  stateSAcmtA, /* 2: in Slash Asterisk comment, and saw '*' */
  stateSScmt,  /* 3: in Slash Slash (C++ or C99) comment */
  stateStr,    /* 4: in "" String */
  stateStrEsc, /* 5: in "" String and saw \ */
  stateElse,   /* 6: everything else */
};

/*
Does the uncommenting.
Issues to fix:
-- does not handle \ newline continuation in the middle of starting / * or
   ending * / delimiter of comment.
-- DOS/Windows \r\n line termination inside a comment will be turned into \n
-- Totally ignorant about Unicode!
*/
static int
uncomment(const char *me, const char *nameIn, const char *nameOut) {
  airArray *mop;
  FILE *fin, *fout;
  int ci /* char in */, co /* char out */, state /* of scanner */;

  mop = airMopNew();
  if (!(airStrlen(nameIn) && airStrlen(nameOut))) {
    fprintf(stderr, "%s: empty filename for in (\"%s\") or out (\"%s\")\n", me, nameIn,
            nameOut);
    airMopError(mop);
    return 1;
  }

  /* -------------------------------------------------------- */
  /* open input and output files  */
  fin = airFopen(nameIn, stdin, "rb");
  if (!fin) {
    fprintf(stderr, "%s: couldn't open \"%s\" for reading: \"%s\"\n", me, nameIn,
            strerror(errno));
    airMopError(mop);
    return 1;
  }
  airMopAdd(mop, fin, (airMopper)airFclose, airMopOnError);
  fout = airFopen(nameOut, stdout, "wb");
  if (!fout) {
    fprintf(stderr, "%s: couldn't open \"%s\" for writing: \"%s\"\n", me, nameOut,
            strerror(errno));
    airMopError(mop);
    return 1;
  }
  airMopAdd(mop, fout, (airMopper)airFclose, airMopOnError);

  /* -------------------------------------------------------- */
  /* do the conversion.  Some sources of inspiration:
  https://en.cppreference.com/w/c/comment
  https://en.cppreference.com/w/c/language/string_literal
  https://en.cppreference.com/w/c/language/character_constant
  https://en.cppreference.com/w/c/language/escape
  https://stackoverflow.com/questions/2394017/remove-comments-from-c-c-code
  https://stackoverflow.com/questions/47565090/need-help-to-extract-comments-from-c-file
  https://stackoverflow.com/questions/27847725/reading-a-c-source-file-and-skipping-comments?rq=3
  */
  state = stateElse; /* start in straight copying mode */
  while ((ci = fgetc(fin)) != EOF) {
    /* job of uncommenting is to:
    - read character ci from input
    - set co = ' ' when ci is a character inside a comment, else set co = ci,
    - then print co to output.
    The "state" variable takes on values from the enum above
    to keep track of what state the scanning is in.
    */
    switch (state) {
    case stateElse:
      co = ci;
      if ('/' == ci) {
        state = stateSlash;
      } else if ('"' == ci) {
        state = stateStr;
      }
      /* else state stays same */
      break;
    case stateSlash:
      co = ci;
      if ('/' == ci) {
        state = stateSScmt;
      } else if ('*' == ci) {
        state = stateSAcmt;
      } else { /* was just a stand-alone slash */
        state = stateElse;
      }
      break;
    case stateSScmt:
      if ('\n' == ci) { /* the // comment has ended; record that in output */
        co = ci;
        state = stateElse;
      } else { /* still inside // comment; still converting to spaces */
        co = ' ';
      }
      break;
    case stateSAcmt:
      if ('*' == ci) { /* maybe comment is ending, no output until sure */
        co = 0;
        state = stateSAcmtA;
      } else {            /* still inside / * * / comment */
        if ('\n' == ci) { /* preserve line counts by copying out newlines */
          co = '\n';
        } else { /* else just turn to spaces */
          co = ' ';
        }
      }
      break;
    case stateSAcmtA:
      if ('/' == ci) { /* The comment has ended; output that ending */
        fputc('*', fout);
        co = ci;
        state = stateElse;
      } else {
        /* false alarm, the * in comment was just a *; convert it to space,
        and keep converting everything to space */
        fputc(' ', fout);
        co = ' ';
        state = stateSAcmt;
      }
      break;
    case stateStr:
      co = ci;
      if ('"' == ci) { /* unescaped ": string has ended */
        state = stateElse;
      } else if ('\\' == ci) { /* single backslash = start of an escape sequence */
        state = stateStrEsc;
      }
      /* else state stays same: still in string */
      break;
    case stateStrEsc:
      /* we don't really have to keep track of the different escape sequences;
      we just have to know its an escape sequence. This will handle \" being in the
      string, which does not end the string (hence the need for this side state),
      and but nor do we need code specific to that escape sequence. */
      co = ci;
      state = stateStr;
      break;
    default:
      fprintf(stderr, "%s: unimplemented state %d ?!?\n", me, state);
      return 1;
    }
    if (co) {
      fputc(co, fout);
    }
  } /* while fgetc loop */
  airMopOkay(mop);
  return 0;
}

static int
unrrdu_uncmtMain(int argc, const char **argv, const char *me, hestParm *hparm) {
  /* these are stock for unrrdu */
  hestOpt *opt = NULL;
  airArray *mop;
  int pret;
  char *err;
  /* these are specific to this command */
  char *nameIn, *nameOut;

  hestOptAdd(&opt, NULL, "fileIn", airTypeString, 1, 1, &nameIn, NULL,
             "Single input file to read; use \"-\" for stdin");
  hestOptAdd(&opt, NULL, "fileOut", airTypeString, 1, 1, &nameOut, NULL,
             "Single output filename; use \"-\" for stdout");

  mop = airMopNew();
  airMopAdd(mop, opt, hestOptFree_vp, airMopAlways);
  USAGE(_unrrdu_uncmtInfoL);
  PARSE();
  airMopAdd(mop, opt, (airMopper)hestParseFree, airMopAlways);

  if (uncomment(me, nameIn, nameOut)) {
    fprintf(stderr, "%s: something went wrong\n", me);
    airMopError(mop);
    return 1;
  }

  airMopOkay(mop);
  return 0;
}

UNRRDU_CMD_HIDE(uncmt, INFO);
