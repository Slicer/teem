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

#define INCR 32

/* dereferences as char *, sets to '\0' */
static void
setNul(void *_c) {
  char *c = (char *)(_c);
  c[0] = '\0';
  return;
}

static void
hargInit(void *_harg) {
  airPtrPtrUnion appu;
  hestArg *harg;
  harg = (hestArg *)_harg;
  harg->str = NULL;
  harg->len = 0;
  appu.c = &(harg->str);
  harg->strArr = airArrayNew(appu.v, &(harg->len), 1 /* unit */, INCR);
  airArrayStructCB(harg->strArr, setNul, NULL);
  /* initialize with \0 so that harg->str is "" */
  airArrayLenIncr(harg->strArr, 1);
  /* now harg->str = {0:'\0'} and harg->len = 1; */
  return;
}

hestArg *
hestArgNew(void) {
  hestArg *harg;

  harg = AIR_CALLOC(1, hestArg);
  assert(harg);
  hargInit(harg);
  return harg;
}

hestArg *
hestArgNix(hestArg *harg) {
  if (harg) {
    if (harg->str) {
      /* If caller wants to keep harg->str around,
         they need to have copied it (the pointer) and set harg->str to NULL */
      free(harg->str);
    }
    airArrayNix(harg->strArr); /* leave the underlying str alone */
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
  uint len, si;
  len = AIR_UINT(strlen(str));
  for (si = 0; si < len; si++) {
    hestArgAddChar(harg, str[si]);
  }
  return;
}

typedef union {
  hestArg **harg;
  hestArgVec **havec;
  void **v;
} hestPtrPtrUnion;

hestArgVec *
hestArgVecNew() {
  hestPtrPtrUnion hppu;
  hestArgVec *havec;
  havec = AIR_CALLOC(1, hestArgVec);
  assert(havec);
  havec->harg = NULL;
  havec->len = 0;
  hppu.harg = &(havec->harg);
  havec->hargArr = airArrayNew(hppu.v, &(havec->len), sizeof(hestArgVec), INCR);
  airArrayStructCB(havec->hargArr, hargInit, NULL);
  return havec;
}

void
hestArgVecAppendString(hestArgVec *havec, const char *str) {
  uint idx;
  idx = airArrayLenIncr(havec->hargArr, 1);
  hestArgAddString(havec->harg + idx, str);
}

void
hestArgVecPrint(const hestArgVec *havec) {
  uint idx;
  printf("hestArgVec %p has %u args:\n", havec, havec->len);
  for (idx = 0; idx < havec->hargArr->len; idx++) {
    const hestArg *harg;
    harg = havec->harg + idx;
    printf(" %u:<%s>", idx, harg->str);
  }
  printf("\n");
}

hestInput *
hestInputNew(void) {
  hestInput *hin;
  hin = AIR_CALLOC(1, hestInput);
  assert(hin);
  hin->source = hestSourceUnknown;
  hin->dflt = NULL;
  hin->argc = 0;
  hin->argv = NULL;
  hin->argIdx = 0;
  hin->fname = NULL;
  hin->file = NULL;
  return hin;
}

#if 0

/* what is the thing we're currently processing to build up the arg vec */
typedef struct {
  int source; /* from the hestSource* enum */
  /* ------ if source == hestSourceDefault ------ */
  const char *dflt;
  /* ------ if source == hestSourceCommandLine ------ */
  int argc;
  const char **argv;
  unsigned int argIdx;
  /* ------ if source == hestSourceResponseFile ------ */
  char *fname;
  FILE *file;
} hestInput;

#endif