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

#define INCR 32

/* -------------------------- hestArg = harg = hestArg = harg ---------------------- */

/* dereferences as char *, sets to '\0' */
static void
setNul(void *_c) {
  char *c = (char *)(_c);
  c[0] = '\0';
  return;
}

hestArg *
hestArgNew(void) {
  hestArg *harg = AIR_CALLOC(1, hestArg);
  assert(harg);
  harg->str = NULL;
  harg->len = 0;
  airPtrPtrUnion appu;
  appu.c = &(harg->str);
  harg->strArr = airArrayNew(appu.v, &(harg->len), sizeof(char), INCR);
  // underlying array harg->str will not be reallocated if shrunk
  harg->strArr->noReallocWhenSmaller = AIR_TRUE;
  airArrayStructCB(harg->strArr, setNul, NULL);
  harg->source = hestSourceUnknown;
  /* initialize with \0 so that harg->str is "" */
  airArrayLenIncr(harg->strArr, 1);
  /* now harg->str = {0:'\0'} and harg->len = 1; */
  return harg;
}

static void *
_hestArgNew_vp(void) {
  return AIR_VOIDP(hestArgNew());
}

hestArg *
hestArgNix(hestArg *harg) {
  if (harg) {
    airArrayNuke(harg->strArr);
    free(harg);
  }
  return NULL;
}

static void *
_hestArgNix_vp(void *_harg) {
  hestArg *harg = (hestArg *)_harg;
  return AIR_VOIDP(hestArgNix(harg));
}

void
hestArgReset(hestArg *harg) {
  assert(harg);
  airArrayLenSet(harg->strArr, 0);
  /* initialize with \0 so that harg->str is "" */
  airArrayLenIncr(harg->strArr, 1);
  return;
}

void
hestArgAddChar(hestArg *harg, char cc) {
  assert(harg);
  airArrayLenIncr(harg->strArr, 1);
  /* if this was first call after hestArgNew, we have
     harg->str = {0:'\0', 1:'\0'} and harg->len = 2 */
  harg->str[harg->len - 2] = cc;
  return;
}

static void
arg_AddOrSet_String(hestArg *harg, int resetFirst, const char *str) {
  assert(harg && str);
  if (resetFirst) {
    hestArgReset(harg);
  }
  uint len = AIR_UINT(strlen(str));
  for (uint si = 0; si < len; si++) {
    hestArgAddChar(harg, str[si]);
  }
  return;
}

void
hestArgSetString(hestArg *harg, const char *str) {
  arg_AddOrSet_String(harg, AIR_TRUE, str);
  return;
}

void
hestArgAddString(hestArg *harg, const char *str) {
  arg_AddOrSet_String(harg, AIR_FALSE, str);
  return;
}

/* ---------------------- hestArgVec = havec = hestArgVec = havec ------------------ */

/* to avoid strict aliasing warnings */
typedef union {
  hestArg **harg;
  hestArg ***hargP;
  hestArgVec **havec;
  hestInput **hin;
  void **v;
} hestPtrPtrUnion;

hestArgVec *
hestArgVecNew() {
  hestArgVec *havec = AIR_CALLOC(1, hestArgVec);
  assert(havec);
  havec->harg = NULL;
  havec->len = 0;
  hestPtrPtrUnion hppu;
  hppu.hargP = &(havec->harg);
  havec->hargArr = airArrayNew(hppu.v, &(havec->len), sizeof(hestArg *), INCR);
  // underlying array havec->harg will not be reallocated if shrunk
  havec->hargArr->noReallocWhenSmaller = AIR_TRUE;
  // airArrayStructCB(havec->hargArr, hargInit, hargDone);
  airArrayPointerCB(havec->hargArr, _hestArgNew_vp, _hestArgNix_vp);
  return havec;
}

void
hestArgVecReset(hestArgVec *havec) {
  if (havec) {
    airArrayLenSet(havec->hargArr, 0);
  }
  return;
}

hestArgVec *
hestArgVecNix(hestArgVec *havec) {
  if (havec) {
    airArrayNuke(havec->hargArr);
    free(havec);
  }
  return NULL;
}

// return havec->harg[popIdx] and shift higher indices down
hestArg *
hestArgVecRemove(hestArgVec *havec, uint popIdx) {
  hestArg *ret = NULL;
  if (havec && popIdx < havec->len) { // note: this implies that havec->len >= 1
    ret = havec->harg[popIdx];
    uint ai;
    for (ai = popIdx; ai < havec->len - 1; ai++) {
      havec->harg[ai] = havec->harg[ai + 1];
    }
    // NULL out final out, using final value of ai
    havec->harg[ai] = NULL;
    // decrement the nominal length of havec->harg
    airArrayLenIncr(havec->hargArr, -1);
  }
  return ret;
}

/* hestArgVecSprint goes is opposite of the shell-style tokenization of
parsest.c/argstGo: generate a single human-friendly string that could be tokenized to
recover the hestArgVec we started with.
ChatGPT helped with prototyping.
Here are instructive examples of the same kind of argv pretty-printing:
https://github.com/git/git/blob/master/quote.c
and here https://www.opencoverage.net/coreutils/index_html/source_213.html
with its (more baroque) quotearg_buffer_restyled() function
*/

// plainWord(str) is true if nothing in str needs quoting or escaping
static int
plainWord(const char *s) {
  if (*s == '\0') {
    // wut - we got the empty string, yes needs quoting
    return 0;
  }
  int plain = AIR_TRUE;
  for (; *s; s++) {
    plain &= (isalnum(*s)               //
              || *s == '_' || *s == '-' //
              || *s == '.' || *s == '/');
    if (!plain) break;
  }
  return plain;
}

/* Assuming that `str` needs some quoting or ecaping to be retokenized as a single arg
then figure out if that should be via single or double quoting, by doing both and picking
the shorter one */
void
argAddQuotedString(hestArg *harg, const char *str) {
  hestArg *singQ = hestArgNew();
  hestArg *doubQ = hestArgNew();
  hestArgAddChar(singQ, '\'');
  hestArgAddChar(doubQ, '"');
  const char *src = str;
  for (; *src; src++) {
    // -- single quoting to singQ
    if ('\'' == *src) {
      // can't escape ' inside ''-quoting, so have to:
      // stop ''-quoting, write (escaped) \', then re-start ''-quoting
      hestArgAddString(singQ, "'\\''");
    } else {
      hestArgAddChar(singQ, *src);
    }
    // -- double quoting to doubQ
    if ('"' == *src || '\\' == *src || '`' == *src || '$' == *src) {
      // this character needs escaping
      hestArgAddChar(doubQ, '\\');
    }
    hestArgAddChar(doubQ, *src);
  }
  hestArgAddChar(singQ, '\'');
  hestArgAddChar(doubQ, '"');
  // use single-quoting when it is shorter, else double-quoting
  hestArgAddString(harg, singQ->len < doubQ->len ? singQ->str : doubQ->str);
  hestArgNix(singQ);
  hestArgNix(doubQ);
}

char *
hestArgVecSprint(const hestArgVec *havec, int showIdx) {
  if (!havec) {
    return NULL;
  }
  hestArg *retArg = hestArgNew();
  // if we got an empty havec, then we'll leave with an appropriately empty string
  for (uint ai = 0; ai < havec->len; ai++) {
    if (ai) {
      // add the space between previous and this arg
      hestArgAddChar(retArg, ' ');
    }
    if (showIdx) {
      char buff[AIR_STRLEN_SMALL + 1];
      sprintf(buff, "%u", ai);
      hestArgAddString(retArg, buff);
      hestArgAddChar(retArg, ':');
    }
    const char *astr = havec->harg[ai]->str;
    if (plainWord(astr)) {
      hestArgAddString(retArg, astr);
    } else {
      argAddQuotedString(retArg, astr);
    }
  }
  // the real hestArgNix: keep the string but lose everything around it
  char *ret = retArg->str;
  airArrayNix(retArg->strArr);
  free(retArg);
  return ret;
}

void
hestArgVecAppendString(hestArgVec *havec, const char *str) {
  uint idx = airArrayLenIncr(havec->hargArr, 1);
  hestArgSetString(havec->harg[idx], str);
}

void
hestArgVecAppendArg(hestArgVec *havec, hestArg *harg) {
  uint idx = airArrayLenIncr(havec->hargArr, 1);
  // oops, delete the hestArg just created via callback
  hestArgNix(havec->harg[idx]);
  havec->harg[idx] = harg;
}

void
hestArgVecPrint(const char *caller, const char *info, const hestArgVec *havec) {
  // fprintf(stderr, "!%s: %s hestArgVec %p has %u args:\n", caller, info, havec,
  // havec->len);
  char srcch[] = {
    // quick way of identifying source
    '?', // 0: hestSourceUnknown
    'c', // 1: hestSourceCommandLine
    'r', // 2: hestSourceResponseFile
    'd', // 3: hestSourceDefault
  };
  printf("%s%s%s hestArgVec %p has %u args:\n   ", airStrlen(caller) ? caller : "", //
         airStrlen(caller) ? ": " : "",                                             //
         info, havec, havec->len);
  for (uint idx = 0; idx < havec->hargArr->len; idx++) {
    const hestArg *harg;
    harg = havec->harg[idx];
    // fprintf(stderr, "!%s harg@%p=%u:<%s>\n", "", AIR_VOIDP(harg), idx,
    //        harg->str ? harg->str : "NULL");
    printf(" %u%c:<%s>", idx, srcch[harg->source], harg->str ? harg->str : "NULL");
  }
  printf("\n");
  char *ppargs = hestArgVecSprint(havec, AIR_FALSE);
  printf("%s%s%s OR pretty-printed as:\n%s\n", airStrlen(caller) ? caller : "", //
         airStrlen(caller) ? ": " : "", info, ppargs);
  free(ppargs);
  return;
}

/* ------------------------- hestInput = hin = hestInput = hin --------------------- */

static void
hinInit(void *_hin) {
  hestInput *hin = (hestInput *)_hin;
  hin->source = hestSourceUnknown;
  hin->argc = 0;
  hin->argv = NULL;
  hin->argIdx = 0;
  hin->rfname = NULL;
  hin->rfile = NULL;
  hin->dfltStr = NULL;
  hin->carIdx = 0;
  hin->dashBraceComment = 0;
  return;
}

hestInput *
hestInputNew(void) {
  hestInput *hin = AIR_CALLOC(1, hestInput);
  assert(hin);
  hinInit(hin);
  return hin;
}

static void
hinDone(void *_hin) {
  hestInput *hin = (hestInput *)_hin;
  hin->rfile = airFclose(hin->rfile);
  hin->rfname = airFree(hin->rfname);
  return;
}

hestInput *
hestInputNix(hestInput *hin) {
  assert(hin);
  hinDone(hin);
  free(hin);
  return NULL;
}

/* ------------------- hestInputStack = hist = hestInputStack = hist --------------- */

hestInputStack *
hestInputStackNew(void) {
  hestInputStack *hist = AIR_CALLOC(1, hestInputStack);
  assert(hist);
  hist->hin = NULL;
  hist->len = 0;
  hestPtrPtrUnion hppu;
  hppu.hin = &(hist->hin);
  hist->hinArr = airArrayNew(hppu.v, &(hist->len), sizeof(hestInput), INCR);
  airArrayStructCB(hist->hinArr, hinInit, hinDone);
  hist->stdinRead = AIR_FALSE;
  return hist;
}

hestInputStack *
hestInputStackNix(hestInputStack *hist) {
  assert(hist);
  airArrayNuke(hist->hinArr);
  free(hist);
  return NULL;
}
