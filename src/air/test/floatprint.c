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


#include "../air.h"

char *me;

int
main(int argc, char *argv[]) {
  char *fS, buff[128];
  float f, sf;
  double d;
  int ret;

  me = argv[0];
  if (2 != argc) {
    fprintf(stderr, "usage: %s <float>\n", me);
    exit(1);
  }
  fS = argv[1];

  ret = sscanf(fS, "%f", &sf);
  if (!ret) {
    printf("%s: sscanf(%s, \"%%f\") failed\n", me, fS);
    printf("\n");
  }
  if (1 != airSingleSscanf(fS, "%f", &f)) {
    fprintf(stderr, "%s: couldn't parse \"%s\" as float\n", me, fS);
    exit(1);
  }
  if (ret && (sf != f)) {
    printf("%s: sscanf result (%f) != airSingleSscanf (%f) !!!\n", me, sf, f);
    printf("\n");
  }
  d = f;
  airSinglePrintf(NULL, buff, "%f", f);
  printf("%s: printf/airSinglePrintf as float:\n%f\n%s\n", me, f, buff);
  airSinglePrintf(NULL, buff, "%f", d);
  printf("\n");
  printf("%s: printf/airSinglePrintf as double:\n%f\n%s\n", me, d, buff);
  printf("\n");
  printf("%s: airFPFprintf_f:\n", me);
  airFPFprintf_f(stderr, f);

  {
    double l2d = log2(AIR_ABS(d));
    double fl2d = floor(l2d)+1;
    double ss = l2d - fl2d;
    double ess = pow(2,ss);
    int ee;
    double frx = frexp(d, &ee);
      printf("d=%.17g -> l2d=%.17g -> fl2d=%.17g\n -> ss=%.17g -> ess=%.17g vs %0.17g|%d\n",
             d, l2d, fl2d, ss, ess, frx, ee);
  }

  ret = sscanf(fS, "%lf", &d);
  if (!ret) {
    printf("%s: sscanf(%s, \"%%lf\") failed\n", me, fS);
    printf("\n");
  }
  airFPFprintf_d(stderr, d);

  d = (double)1/10;
  d = d+d+d+d+d+d+d+d+d+d;
  printf("d = %.17g\n", d);
  airFPFprintf_d(stderr, d);

  exit(0);
}
