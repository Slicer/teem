/*
  The contents of this file are subject to the University of Utah Public
  License (the "License"); you may not use this file except in
  compliance with the License.
  
  Software distributed under the License is distributed on an "AS IS"
  basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See
  the License for the specific language governing rights and limitations
  under the License.

  The Original Source Code is "teem", released March 23, 2001.
  
  The Original Source Code was developed by the University of Utah.
  Portions created by UNIVERSITY are Copyright (C) 2001, 1998 University
  of Utah. All Rights Reserved.
*/


#include <stdio.h>
#include <air.h>
#include "../hest.h"

int
parse(void *_ptr, char *str, char *err) {
  char **ptrP;

  ptrP = _ptr;
  *ptrP = airStrdup(str);
  if (!(*ptrP)) {
    sprintf(err, "couldn't strdup() str");
    return 1;
  }
  airToUpper(*ptrP);
  return 0;
}

hestCB cbinfo = {
  sizeof(char*),
  "token",
  parse,
  airFree
};

int
main(int argc, char **argv) {
  char *single, *triple[3], *maybe, **many;
  int howMany, i, N;
  hestOpt *opt = NULL;
  char *err = NULL;

  hestOptAdd(&opt, "A",      "token",           airTypeOther, 1,  1, &single, 
	     "alpha",        "testing A",       NULL,  NULL,  &cbinfo);
  hestOptAdd(&opt, "B",      "tok1 tok2 tok3",  airTypeOther, 3,  3, triple,
	     "beta psi rho", "testing B",       NULL,  NULL,  &cbinfo);
  hestOptAdd(&opt, "C",      "mtok",            airTypeOther, 0,  1, &maybe,
	     "gamma",        "testing C",       NULL,  NULL,  &cbinfo);
  hestOptAdd(&opt, "D",      "tok",             airTypeOther, 1, -1, &many,
	     "kappa omega",  "testing D",       &howMany, NULL, &cbinfo);
  hestOptAdd(&opt, "int",    "N",               airTypeInt,   1,  1, &N,
	     NULL,           "an integer");
  
  if (hestParse(opt, argc-1, argv+1, &err, NULL)) {
    fprintf(stderr, "ERROR: %s\n", err); free(err);
    hestUsage(stderr, opt, argv[0], NULL);
    hestGlossary(stderr, opt, NULL);
    exit(1);
  }

  printf("single: %s\n", single);
  printf("triple: %s %s %s\n", triple[0], triple[1], triple[2]);
  printf("maybe:  %s\n", maybe);
  printf("many(%d):", howMany);
  for (i=0; i<=howMany-1; i++) {
    printf(" %s", many[i]);
  }
  printf("\n");

  hestParseFree(opt);
  exit(0);
}
