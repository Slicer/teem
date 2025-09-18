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

#include <assert.h>
#include <sys/errno.h>

#define INCR 32

/* to avoid strict aliasing warnings */
typedef union {
  hestArg **harg;
  hestArgVec **havec;
  hestInput **hin;
  void **v;
} hestPtrPtrUnion;

/* -------------------------- hestArg = harg = hestArg = harg ---------------------- */

/* dereferences as char *, sets to '\0' */
static void
setNul(void *_c) {
  char *c = (char *)(_c);
  c[0] = '\0';
  return;
}

static void
hargInit(void *_harg) {
  hestArg *harg = (hestArg *)_harg;
  harg->str = NULL;
  harg->len = 0;
  airPtrPtrUnion appu;
  appu.c = &(harg->str);
  harg->strArr = airArrayNew(appu.v, &(harg->len), sizeof(char), INCR);
  airArrayStructCB(harg->strArr, setNul, NULL);
  /* initialize with \0 so that harg->str is "" */
  airArrayLenIncr(harg->strArr, 1);
  /* now harg->str = {0:'\0'} and harg->len = 1; */
  harg->finished = AIR_FALSE;
  return;
}

hestArg *
hestArgNew(void) {
  hestArg *harg = AIR_CALLOC(1, hestArg);
  assert(harg);
  hargInit(harg);
  return harg;
}

static void
hargDone(void *_harg) {
  hestArg *harg = (hestArg *)_harg;
  if (harg->str) {
    /* If caller wants to keep harg->str around,
       they need to have copied it (the pointer) and set harg->str to NULL */
    free(harg->str);
  }
  airArrayNix(harg->strArr); /* leave the underlying str alone */
  return;
}

hestArg *
hestArgNix(hestArg *harg) {
  if (harg) {
    hargDone(harg);
    free(harg);
  }
  return NULL;
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

void
hestArgAddString(hestArg *harg, const char *str) {
  assert(harg && str);
  uint len = AIR_UINT(strlen(str));
  for (uint si = 0; si < len; si++) {
    hestArgAddChar(harg, str[si]);
  }
  /* The assumption is that if you have a string to put here; then you know that the
  string is finished.  User can modify this if that's not the case. */
  harg->finished = AIR_TRUE;
  return;
}

/* ---------------------- hestArgVec = havec = hestArgVec = havec ------------------ */

hestArgVec *
hestArgVecNew() {
  hestArgVec *havec = AIR_CALLOC(1, hestArgVec);
  assert(havec);
  havec->harg = NULL;
  havec->len = 0;
  hestPtrPtrUnion hppu;
  hppu.harg = &(havec->harg);
  havec->hargArr = airArrayNew(hppu.v, &(havec->len), sizeof(hestArg), INCR);
  airArrayStructCB(havec->hargArr, hargInit, hargDone);
  return havec;
}

hestArgVec *
hestArgVecNix(hestArgVec *havec) {
  assert(havec);
  airArrayNuke(havec->hargArr);
  free(havec);
  return NULL;
}

void
hestArgVecAppendString(hestArgVec *havec, const char *str) {
  uint idx = airArrayLenIncr(havec->hargArr, 1);
  hestArgAddString(havec->harg + idx, str);
}

void
hestArgVecPrint(const char *caller, const hestArgVec *havec) {
  printf("%s: hestArgVec %p has %u args:\n", caller, havec, havec->len);
  for (uint idx = 0; idx < havec->hargArr->len; idx++) {
    const hestArg *harg;
    harg = havec->harg + idx;
    printf(" %u:<%s>%c", idx, harg->str, harg->finished ? '.' : '~');
  }
  printf("\n");
}

/* ------------------------- hestInput = hin = hestInput = hin --------------------- */

static void
hinInit(void *_hin) {
  hestInput *hin = (hestInput *)_hin;
  hin->source = hestSourceUnknown;
  hin->dflt = NULL;
  hin->argc = 0;
  hin->argv = NULL;
  hin->argIdx = 0;
  hin->rfname = NULL;
  hin->rfile = NULL;
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
  /* nothing, for now*/
  AIR_UNUSED(hin);
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

#define ME ((hparm && hparm->verbosity) ? me : "")

int
hestInputStackPushCommandLine(hestInputStack *hist, int argc, const char **argv,
                              char *err, const hestParm *hparm) {
  static const char me[] = "hestInputStackPushCommandLine: ";
  if (!(hist && argv && hparm)) { // (as if all this can go wrong but err is non-NULL)
    sprintf(err, "%s: got NULL pointer (hist %p, argv %p, hparm %p)", __func__,
            AIR_VOIDP(hist), AIR_VOIDP(argv), AIR_VOIDP(hparm));
    return 1;
  }
  if (hparm->verbosity) {
    printf("%s: changing stack height: %u --> %u with argc=%d,argv=%p; "
           "setting argIdx to 0\n",
           __func__, hist->hinArr->len, hist->hinArr->len + 1, argc, AIR_VOIDP(argv));
  }
  uint idx = airArrayLenIncr(hist->hinArr, 1);
  if (hparm->verbosity > 1) {
    printf("%snew hinTop = %p\n", ME, AIR_VOIDP(hist->hin + idx));
  }
  hist->hin[idx].source = hestSourceCommandLine;
  hist->hin[idx].argc = argc;
  hist->hin[idx].argv = argv;
  hist->hin[idx].argIdx = 0;
  return 0;
}

int
hestInputStackPushResponseFile(hestInputStack *hist, const char *rfname, char *err,
                               const hestParm *hparm) {
  static const char me[] = "hestInputStackPushResponseFile: ";
  if (!(hist && rfname && hparm)) {
    sprintf(err, "%s: got NULL pointer (hist %p, rfname %p, hparm %p)", __func__,
            AIR_VOIDP(hist), AIR_VOIDP(rfname), AIR_VOIDP(hparm));
    return 1;
  }
  if (!strlen(rfname)) {
    sprintf(err,
            "%ssaw arg start with response file flag \"%c\" "
            "but no filename followed",
            ME, RESPONSE_FILE_FLAG);
    return 1;
  }
  // "- 1" safe because hestParse always starts with argc/argv, not a response file
  uint topHinIdx = hist->len - 1;
  // have we seen rfname before?
  for (uint hidx = 0; hidx < topHinIdx; hidx++) {
    hestInput *oldHin = hist->hin + hidx;
    if (hestSourceResponseFile == oldHin->source //
        && !strcmp(oldHin->rfname, rfname)) {
      // HEY test this error
      sprintf(err, "%salready currently reading \"%s\" as response file", ME, rfname);
      return 1;
    }
  }
  // are we trying to read stdin twice?
  if (!strcmp("-", rfname) && hist->stdinRead) {
    // HEY test this error
    sprintf(err, "%sresponse filename \"%s\" but previously read stdin", ME, rfname);
    return 1;
  }
  // try to open response file
  FILE *rfile = airFopen(rfname, stdin, "r");
  if (!(rfile)) {
    // HEY test this error
    sprintf(err, "%scouldn't fopen(\"%s\",\"r\"): %s", ME, rfname, strerror(errno));
    return 1;
  }
  // okay, we actually opened the response file; put it on the stack
  uint idx = airArrayLenIncr(hist->hinArr, 1);
  if (hparm->verbosity > 1) {
    printf("%snew hinTop = %p\n", ME, AIR_VOIDP(hist->hin + idx));
  }
  hist->hin[idx].source = hestSourceResponseFile;
  hist->hin[idx].rfname = rfname;
  hist->hin[idx].rfile = rfile;
  return 0;
}

int
hestInputStackPop(hestInputStack *hist, char *err, const hestParm *hparm) {
  assert(hist);
  uint len = hist->hinArr->len;
  if (!len) {
    sprintf(err, "%s: cannot pop from stack height 0", __func__);
    return 1;
  }
  if (hparm->verbosity) {
    printf("%s: changing stack height: %u --> %u; popping %s source\n", __func__, len,
           len - 1, airEnumStr(hestSource, hist->hin[len - 1].source));
  }
  airArrayLenIncr(hist->hinArr, -1);
  return 0;
}