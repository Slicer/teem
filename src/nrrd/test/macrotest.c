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


#include <nrrd.h>

#define NRRDROUND(x) (int)((x) + 0.5)

void
main() {
  double a, b, c, d, e, nand;
  int i, x1, x2;

  a = 1.0;
  b = 2.0;
  c = -1.0;
  printf("NRRDMAX(%lf,%lf) = %lf\n", a, b, NRRDMAX(a,b));
  printf("NRRDMIN(%lf,%lf) = %lf\n", a, b, NRRDMIN(a,b));
  printf("NRRDABS(%lf) = %lf\n", c, NRRDABS(c));
  c = 3.0;
  printf("NRRDABS(%lf) = %lf\n", c, NRRDABS(c));

  for (i=0; i<=10; i++) {
    a = i/10.0;
    printf("NRRDROUND(%lf) = %d\n", a, NRRDROUND(a));
    printf("NRRDLERP(%lf,%lf,%lf) = %lf\n", a, b, c, NRRDLERP(a, b, c));
  }

  nand = nrrdNand();
  printf("NRRDEXISTS(%lf) = %d\n", nand, NRRDEXISTS(nand));
  printf("NRRDEXISTS(%lf) = %d\n", c, NRRDEXISTS(c));

  a = 0.0;
  c = 1.0;
  d = 2.5; 
  e = 3.5;
  for (i=-10; i<=20; i++) {
    b = i/10.0;
    printf("NRRDINSIDE(%lf,%lf,%lf) = %d\n", a, b, c, NRRDINSIDE(a, b, c));
    printf("NRRDCLAMP(%lf,%lf,%lf) = %lf\n", a, b, c, NRRDCLAMP(a, b, c));
    printf("NRRDAFFINE(%lf,%lf,%lf,%lf,%lf) = %lf\n",
	   d, b+d, e, 0.0, 10.0, NRRDAFFINE(d, b+d, e, 0.0, 10.0));
  }

  for (i=-20; i<=120; i++) {
    b = i/100.0;
    NRRDINDEX(0.0,b,1.0,7.0,x1);
    NRRDCLAMPINDEX(0.0,b,1.0,7.0,x2);
    printf("NRRDINDEX(0.0,%lf,1.0,7.0)-> %d; clamp -> %d\n", b, x1, x2);
  }
}









