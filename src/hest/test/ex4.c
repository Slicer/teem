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
#include "air.h"
#include "hest.h"

int
parse(void *_ptr, char *str, char *err) {
  double *ptr;
  int ret;

  ptr = _ptr;
  ret = sscanf(str, "%lf,%lf", ptr + 0, ptr + 1);
  if (2 != ret) {
    sprintf(err, "parsed %d, not 2 doubles", ret);
    return 1;
  }
  return 0;
}

hestCB cbinfo = {
  2*sizeof(double),
  "location",
  parse,
  NULL
};

int
main(int argc, char **argv) {
  static double single[2], triple[6], maybe[2], *many;
  static int howMany;
  hestOpt opt[] = {
    {"A",   "x,y", airTypeOther,   1,  1,  single,  "30,50", 
     "testing A",  NULL,  &cbinfo},
    {"B",   "x1,y1 x2,y2 x3,y3", airTypeOther, 3, 3, triple, "1,2 3,4 5,6",
     "testing B",  NULL,  &cbinfo},
    {"C",   "mx,my", airTypeOther,   0,  1,  maybe,  "-0.1,-0.2", 
     "testing C",  NULL,  &cbinfo},
    {"D",   "nx,ny", airTypeOther,   1,  -1,  &many,  "8,8 7,7", 
     "testing D",  &howMany,  &cbinfo},
    {NULL, NULL, 0}
  };
  char *err = NULL;
  int i;
  
  hestVerbosity = 10;
  if (hestParse(opt, argc-1, argv+1, &err, NULL)) {
    fprintf(stderr, "ERROR: %s\n", err); free(err);
    hestUsage(stderr, opt, argv[0], NULL);
    hestGlossary(stderr, opt, NULL);
    exit(1);
  }

  printf("single: (%g,%g)\n", single[0], single[1]);
  printf("triple: (%g,%g) (%g,%g) (%g,%g)\n", triple[0], triple[1],
	 triple[2], triple[3], triple[4], triple[5]);
  printf("maybe: (%g,%g)\n", maybe[0], maybe[1]);
  printf("many(%d):", howMany);
  for (i=0; i<=howMany-1; i++) {
    printf(" (%g,%g)", many[0 + 2*i], many[1 + 2*i]);
  }
  printf("\n");

  exit(0);
}
