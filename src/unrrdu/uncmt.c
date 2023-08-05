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

#define INFO "Change comment contents in a C99 input file"
static const char *_unrrdu_uncmtInfoL
  = (INFO
     ". By default comments are wholly excised, but it is also possible to preserve "
     "comment delimiters while over-writing comment contents. "
     "This command can also change contents of string literals in a "
     "very particular way. This is all motivated by a class GLK teaches, wherein "
     "students are not to use types \"float\" or \"double\" directly (but rather a "
     "class-specific \"real\" typedef). Grepping for \"float\" and \"double\" gives "
     "false positives since they can show up benignly in comments and string "
     "literals; these are avoided by passing the source file through \"unu uncmt "
     "-nfds\". Catching implicit conversions between floating point precisions is "
     "handled separately, in case you were thinking about that.\n "
     "* (not actually based on Nrrd)");

/* set of states for little DFA to know whether we're in a comment or not */
enum {
  stateSlash,  /* 0: just got a '/' */
  stateSAcmt,  /* 1: in Slash Asterisk (traditional C) comment */
  stateSAcmtA, /* 2: in Slash Asterisk comment, and saw '*' */
  stateSScmt,  /* 3: in Slash Slash (C++ or C99) comment */
  stateStr,    /* 4: in "" String */
  stateStrEsc, /* 5: in "" String and saw \ */
  stateCC, /* 6: in '' character constant.  Without this state, a '"' character constant
              sends the DFA into stateStr, and then we fail to uncomment. Funnily enough,
              this deficiency was discovered by running this command on this source file.
            */
  stateCCEsc, /* 7: in '' character constant and saw \ */
  stateElse   /* 8: everything else */
};

/* nfdsChar: next char for "No Float or Double in String" mode
Monitors input characters ci, to prevent output of "float" or "double"
state is maintained by the floatCount and doubleCount variables
passed by reference */
int
nfdsChar(unsigned int *floatCountP, unsigned int *doubleCountP, int ci) {
  int co = ci; /* by default, output == input */
  unsigned int fc = *floatCountP;
  unsigned int dc = *doubleCountP;

  switch (ci) {
  case 'f':
    fc = 3; /* no matter what came prior */
    break;
  case 'l':
    fc = (3 == fc ? 2 : 4);
    dc = (1 == dc ? 0 : 5);
    break;
  case 'o':
    fc = (2 == fc ? 1 : 4);
    dc = (4 == dc ? 3 : 5);
    break;
  case 'a':
    fc = (1 == fc ? 0 : 4);
    break;
  case 't':
    if (!fc) {
      /* boom - we were about say "float", but we won't */
      co = 'T';
    }
    fc = 4;
    break;
  case 'd':
    dc = 4; /* no matter what came prior */
    break;
  /* case 'o' handled above: dc 4 --> 3 */
  case 'u':
    dc = (3 == dc ? 2 : 5);
    break;
  case 'b':
    dc = (2 == dc ? 1 : 5);
    break;
    /* case 'l' handled above: dc 1 --> 0 */
  case 'e':
    if (!dc) {
      /* boom - we were about say "double", but we won't */
      co = 'E';
    }
    dc = 5;
    break;
  default:
    fc = 4;
    dc = 5;
  }

  *floatCountP = fc;
  *doubleCountP = dc;
  return co;
}

/*
Does the uncommenting.
Issues to fix:
-- does not handle \ newline continuation in the middle of starting / * or
   ending * / delimiter of comment.
-- DOS/Windows \r\n line termination ending a // comment will be turned into \n
-- Totally ignorant about Unicode!
*/
static int
uncomment(const char *me, const char *nameOut, int nixcmt, const char *cmtSub, int nfds,
          const char *nameIn) {
  airArray *mop;
  FILE *fin, *fout;
  int ci /* char in */, co /* char out */, state /* of scanner */;
  unsigned int csLen, csIdx; /* length and index into cmtSub */
  /* with nfds (No Float or Double in String) mode,
  we convert 't' to 'T' at end of "float" inside a string, and
  we convert 'e' to 'E' at end of "double" inside a string.
  These variables maintain a countdown to when that happens
  (4 == strlen("float") - 1; 5 == strlen("double") - 1) */
  unsigned int floatCount = 4, doubleCount = 5;

  /* cmtSub and strSub strings can be NULL */
  if (!(airStrlen(nameIn) && airStrlen(nameOut))) {
    fprintf(stderr, "%s: empty filename for in (\"%s\") or out (\"%s\")\n", me, nameIn,
            nameOut);
    return 1;
  }
  if (airStrlen(cmtSub) >= 2) {
    csLen = strlen(cmtSub);
    if (strstr(cmtSub, "*/") || ('/' == cmtSub[0] && '*' == cmtSub[csLen - 1])) {
      fprintf(stderr, "%s: comment substitution |%s| contains \"*/\"; no good\n", me,
              cmtSub);
      return 1;
    }
  }

  /* -------------------------------------------------------- */
  /* open input and output files  */
  mop = airMopNew();
  fin = airFopen(nameIn, stdin, "rb");
  if (!fin) {
    fprintf(stderr, "%s: couldn't open \"%s\" for reading: \"%s\"\n", me, nameIn,
            strerror(errno));
    airMopError(mop);
    return 1;
  }
  airMopAdd(mop, fin, (airMopper)airFclose, airMopAlways);
  fout = airFopen(nameOut, stdout, "wb");
  if (!fout) {
    fprintf(stderr, "%s: couldn't open \"%s\" for writing: \"%s\"\n", me, nameOut,
            strerror(errno));
    airMopError(mop);
    return 1;
  }
  airMopAdd(mop, fout, (airMopper)airFclose, airMopAlways);

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
  /* initialize substitution string variables */
  if (cmtSub && strlen(cmtSub)) {
    csLen = strlen(cmtSub); /* 1 or bigger */
    csIdx = csLen - 1;      /* last valid index, anticipating action of SUB below */
  } else {
    csLen = csIdx = 0;
  }
#define CMT_SUB(CI)                                                                     \
  (nixcmt     /* */                                                                     \
     ? ' '    /* */                                                                     \
     : (csLen /* */                                                                     \
          ? (csIdx = (csIdx + 1) % csLen, cmtSub[csIdx])                                \
          : (CI)))
#define STR_SUB(CI) (nfds ? nfdsChar(&floatCount, &doubleCount, (CI)) : (CI))
  state = stateElse; /* start in straight copying mode */
  while ((ci = fgetc(fin)) != EOF) {
    /* job of uncommenting is to:
    - read character ci from input
    - set co to something that depends on current state (often same as ci)
    - print co to output.
    The "state" variable takes on values from the enum above
    to keep track of what state the scanning is in.
    */
    switch (state) {
    case stateElse:
      co = ci;
      if ('/' == ci) {
        if (nixcmt) {
          /* actually can't output / because it might be start of comment */
          co = 0;
        }
        state = stateSlash;
      } else if ('\'' == ci) {
        state = stateCC;
      } else if ('"' == ci) {
        state = stateStr;
      }
      /* else state stays same */
      break;
    case stateSlash:
      co = ci;
      if ('/' == ci) {
        state = stateSScmt;
        if (nixcmt) {
          /* have to transform the / / start of comment */
          fputc(' ', fout);
          co = ' ';
        }
      } else if ('*' == ci) {
        state = stateSAcmt;
        if (nixcmt) {
          /* have to transform the / * start of comment */
          fputc(' ', fout);
          co = ' ';
        }
      } else { /* was just a stand-alone slash */
        if (nixcmt) {
          /* false alarm, do output / now */
          fputc('/', fout);
        }
        state = stateElse;
      }
      break;
    case stateSScmt:
      if ('\n' == ci) { /* the // comment has ended; record that in output */
        co = ci;
        state = stateElse;
      } else {
        /* in comment contents, copy out all whitespace, else substitute */
        co = isspace(ci) ? ci : CMT_SUB(ci);
      }
      break;
    case stateSAcmt:
      if ('*' == ci) { /* maybe comment is ending, no output until sure */
        co = 0;
        state = stateSAcmtA;
      } else { /* still inside / * * / comment */
        /* copy out all whitespace (thus preserving line counts), else substitute */
        co = isspace(ci) ? ci : CMT_SUB(ci);
      }
      break;
    case stateSAcmtA:
      if ('/' == ci) { /* The comment has ended */
        if (nixcmt) {
          fputc(' ', fout);
          co = ' ';
        } else { /* output the * / ending of comment */
          fputc('*', fout);
          co = ci;
        }
        state = stateElse;
      } else if ('*' == ci) {
        /* saw ** inside comment; first * was plain comment
        content; but the second * puts us back in this state */
        fputc(CMT_SUB('*'), fout);
        co = 0;
        state = stateSAcmtA;
      } else {
        /* false alarm: * in comment was not followed by / so convert it normally */
        fputc(CMT_SUB('*'), fout);
        /* carry on converting comment contents */
        co = isspace(ci) ? ci : CMT_SUB(ci);
        state = stateSAcmt;
      }
      break;
    case stateStr:
      if ('"' == ci) { /* unescaped ": string has ended */
        co = '"';
        state = stateElse;
      } else {
        if ('\\' == ci) { /* single backslash = start of an escape sequence */
          state = stateStrEsc;
        } /* else state stays in stateStr */
        /* whether starting ecape sequence or not; we're still in string */
        co = STR_SUB(ci);
      }
      break;
    case stateCC:
      /* this code is basically copy-paste from stateStr above */
      co = ci;
      if ('\'' == ci) { /* unescaped ': character constant has ended */
        state = stateElse;
      } else {
        if ('\\' == ci) { /* single backslash = start of an escape sequence */
          state = stateCCEsc;
        } /* else state stays in stateCC */
        /* whether starting ecape sequence or not; we're still in character constant */
      }
      break;
    case stateStrEsc:
      /* we don't have to keep track of the different escape sequences;
      we just have to know it's an escape sequence. This will handle \" being in the
      string, which does not end the string (hence the need for this side state),
      and but nor do we need code specific to that escape sequence. */
      co = STR_SUB(ci);
      state = stateStr;
      break;
    case stateCCEsc:
      /* (same logic as above for stateStrEsc) */
      co = ci;
      state = stateCC;
      break;
    default:
      fprintf(stderr, "%s: unimplemented state %d ?!?\n", me, state);
      return 1;
    }
    if (co) {
      fputc(co, fout);
    }
  } /* while fgetc loop */
#undef CMT_SUB
#undef STR_SUB
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
  char *cmtSubst, *nameIn, *nameOut;
  int nfds, nosub;

  hparm->elideSingleEmptyStringDefault = AIR_FALSE;
  hestOptAdd_1_String(
    &opt, "cs", "cmtsub", &cmtSubst, "",
    /* the \t character is turned by hest into non-breaking space */
    "Given a non-empty string, those characters are looped through to replace the "
    "non-white space characters is contents; EXCEPT if a length-2 string of a "
    "repeating character is given, (e.g. \"-cs\txx\") in which case the string contents "
    "are preserved (contrary to the intended purpose of this command). To preserve "
    "comment delimiters but turn comments entirely into whitespace, use "
    "-cs\t\"\t\". To remove the comment delimiters and turn comment contents into "
    "whitespace (the default), pass an empty string here.");
  hestOptAdd_Flag(
    &opt, "nfds", &nfds,
    "prevent \"float\" or \"double\" from appearing in string literals. String "
    "literal contents (unlike comment contents) are typically preserved by this "
    "command (since doing naive per-character substitutions in the same way as done for "
    "comments would totally break printf formatting strings). If this option is used, "
    "\"float\" and \"double\" are prevented from appearing in string literals by "
    "turning them into \"floaT\" and \"doublE\". This functionality is part of the "
    "motivation for this command to exist, but the behavior is weird enough to be off "
    "by default.");
  hestOptAdd_1_String(&opt, NULL, "fileIn", &nameIn, NULL,
                      "Single input file to read; use \"-\" for stdin");
  hestOptAdd_1_String(&opt, NULL, "fileOut", &nameOut, NULL,
                      "Single output filename; use \"-\" for stdout");

  mop = airMopNew();
  airMopAdd(mop, opt, hestOptFree_vp, airMopAlways);
  USAGE_OR_PARSE(_unrrdu_uncmtInfoL);
  airMopAdd(mop, opt, (airMopper)hestParseFree, airMopAlways);

  nosub = (2 == strlen(cmtSubst) && cmtSubst[0] == cmtSubst[1]);
  if (uncomment(me, nameOut,             /* */
                !strlen(cmtSubst),       /* nixcmt is true iff cmtSubst is empty */
                nosub ? NULL : cmtSubst, /* */
                nfds, nameIn)) {
    fprintf(stderr, "%s: something went wrong\n", me);
    airMopError(mop);
    return 1;
  }

  airMopOkay(mop);
  return 0;
}

UNRRDU_CMD_HIDE(uncmt, INFO);
