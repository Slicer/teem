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

void
hestArgSetString(hestArg *harg, const char *str) {
  assert(harg && str);
  hestArgReset(harg);
  uint len = AIR_UINT(strlen(str));
  for (uint si = 0; si < len; si++) {
    hestArgAddChar(harg, str[si]);
  }
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

hestArg *
hestArgVecRemove(hestArgVec *havec, uint popIdx) {
  hestArg *ret = NULL;
  if (havec && popIdx < havec->len) { // note: this implies that havec->len >= 1
    ret = havec->harg[popIdx];
    uint ai;
    for (ai = popIdx; ai < havec->len - 1; ai++) {
      // shuffle down info inside the hestArg elements of havec->harg
      havec->harg[ai] = havec->harg[ai + 1];
      // hestArgSetString(havec->harg + ai, (havec->harg + ai + 1)->str);
      // (havec->harg + ai)->source = (havec->harg + ai + 1)->source;
      /* why cannot just memcpy:
         because then the last hestArg element of havec->harg
           (the one that is being forgotten)
         and the second-to-last element (the last one being kept)
         will share ->str pointers.
         When hargDone is called on the last hestArg's address
         as the callack from airArrayLenIncr(), then it will also
         free the str inside the second-to-last element; oops */
    }
    // NULL out final out, using final value of ai
    havec->harg[ai] = NULL;
    // decrement the nominal length of havec->harg
    airArrayLenIncr(havec->hargArr, -1);
  }
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
