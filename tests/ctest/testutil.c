/* testutil: minimal testing library functions for Teem
   Copyright (C) 2025  University of Chicago
   See ../../LICENSE.txt for licensing terms */

#include <stdlib.h> /* for getenv, malloc */
#include <string.h> /* for strlen */
#include <stdio.h>  /* for fprintf */
#include <assert.h> /* for assert */

#include "testutil.h"

// prependEnvVar(envVar, fName) returns a *newly-allocated* string concatenating
// the value of environment variable `envVar`, '/', and `fName`
static char *
prependEnvVar(const char *envVar, const char *fName) {
  if (!(envVar && strlen(envVar) && fName && strlen(fName))) {
    fprintf(stderr, "%s: ERROR: got NULL (or empty) envvar or file name\n", __func__);
    return NULL;
  }
  const char *evStr = getenv(envVar);
  if (!evStr) {
    fprintf(stderr,
            "%s: ERROR: environment variable \"%s\" not set\n",
            __func__,
            envVar);
    return NULL;
  }
  size_t evLen = strlen(evStr);
  if (!evLen) {
    fprintf(stderr,
            "%s: ERROR: environment variable \"%s\" set to empty string\n",
            __func__,
            envVar);
    return NULL;
  }
  size_t fNameLen = strlen(fName);
  char *ret = malloc(evLen + strlen("/") + fNameLen + 1);
  assert(ret);
  strcpy(ret, evStr);
  ret[evLen] = '/';
  strcpy(ret + evLen + 1, fName);
  return ret;
}

// for test data filename `fName`, this returns a new-allocated path to it
char *
teemTestDataPath(const char *fName) {
  return prependEnvVar("TEEM_TEST_DATA_DIR", fName);
}

// for temporary file name `fName`, this returns a new-allocated path to it
char *
teemTestTmpPath(const char *fName) {
  return prependEnvVar("TEEM_TEST_TMP_DIR", fName);
}

/* (for testing)
int
main(int argc, const char *argv[]) {
  if (argc == 2) {
    char *ret = teemTestDataPath(argv[1]);
    if (ret) {
      printf("teemTestDataPath: |%s|\n", ret);
      free(ret);
    }
    ret = teemTestTmpPath(argv[1]);
    if (ret) {
      printf("teemTestTmpPath:  |%s|\n", ret);
      free(ret);
    }
  }
  return 0;
}
*/
