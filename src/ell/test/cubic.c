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
#include "../ell.h"

char *me;

void
usage() {
  /*                      0   1   2   3   (4) */
  fprintf(stderr, "usage: %s <A> <B> <C>\n", me);
  fprintf(stderr, "for cubic x^3 + Ax^2 + Bx + C == 0\n");
  exit(1);
}

int
main(int argc, char **argv) {
  char buf[512];
  double ans0, ans1, ans2, A, B, C;
  int ret;
  double r[3];

  me = argv[0];
  if (argc != 4) {
    usage();
  }

  sprintf(buf, "%s %s %s", argv[1], argv[2], argv[3]);
  if (3 != sscanf(buf, "%lf %lf %lf", &A, &B, &C)) {
    fprintf(stderr, "%s: couldn't parse 3 floats from command line\n", me);
    exit(1);
  }

  ellDebug = AIR_TRUE;
  ret = ellCubic(r, A, B, C, AIR_TRUE);
  ans0 = C + r[0]*(B + r[0]*(A + r[0]));
  switch(ret) {
  case ellCubicRootSingle:
    printf("1 single root: %g -> %f\n", r[0], ans0);
    break;
  case ellCubicRootTriple:
    printf("1 triple root: %g -> %f\n", r[0], ans0);
    break;
  case ellCubicRootSingleDouble:
    ans1 = C + r[1]*(B + r[1]*(A + r[1]));
    printf("1 single root %g -> %f, 1 double root %g -> %f\n", 
	   r[0], ans0, r[1], ans1);
    break;
  case ellCubicRootThree:
    ans1 = C + r[1]*(B + r[1]*(A + r[1]));
    ans2 = C + r[2]*(B + r[2]*(A + r[2]));
    printf("3 distinct roots:\n %g -> %f\n %g -> %f\n %g -> %f\n",
	   r[0], ans0, r[1], ans1, r[2], ans2);
    break;
  default:
    printf("%s: something fatally wacky happened\n", me);
    exit(1);
  }
  exit(0);
}
