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

void
main() {
  int i, N;
  double bound, x, f;
  float arg[3];

  arg[0] = 1.0;
  arg[1] = 0;
  arg[2] = 0.5;
  N = 80;
  bound = 2;
  printf("v = [\n");
  for (i=0; i<=N-1; i++) {
    x = AIR_AFFINE(0, i, N-1, -bound, bound);
    f = nrrdKernel[nrrdKernelBCCubic].eval(x, arg);
    printf("%g %g;\n", x, f);
  }
  printf("]\n");
}
