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

/* the output of this can be read into matlab with:
 [d,f] = textread('outfile', '%f %f')
 where d is the vector of domain positions, and f is the value of the function
*/

char *me;

#include "../nrrd.h"

void
usage() {
  /*                       0    1    2      3     (4) */
  fprintf(stderr, "usage: %s <kern> <N> <outfile>\n", me);
  exit(1);
}

void
main(int argc, char *argv[]) {
  char *kernS, *NS, *out, tkS[512];
  int i, N;
  double bound, x, f;
  float param[NRRD_MAX_KERNEL_PARAMS];
  nrrdKernel *kern;
  FILE *file;

  me = argv[0];
  if (argc != 4) 
    usage();
  kernS = argv[1];
  NS = argv[2];
  out = argv[3];

  if (1 != sscanf(NS, "%d", &N)) {
    fprintf(stderr, "%s: couldn't parse \"%s\" as int\n", me, NS);
    usage();
  }
  
  /* parse the kernel */
  param[0] = 1.0;
  strcpy(tkS, "box");
  if (!strcmp(tkS, kernS)) {
    kern = nrrdKernelBox;
    goto kparsed;
  }

  strcpy(tkS, "tent");
  if (!strcmp(tkS, kernS)) {
    kern = nrrdKernelTent;
    goto kparsed;
  }

  strcpy(tkS, "cubic");
  if (!strncmp(tkS, kernS, strlen(tkS))) {
    kern = nrrdKernelBCCubic;
    if (2 != sscanf(kernS+strlen(tkS), ":%f,%f", param+1, param+2)) {
      fprintf(stderr, "%s: couldn't parse parameters \"%s\" for %s kernel\n", 
	      me, kernS+strlen(tkS), tkS);
      usage();
    }
    goto kparsed;
  }

  strcpy(tkS, "quartic");
  if (!strncmp(tkS, kernS, strlen(tkS))) {
    kern = nrrdKernelAQuartic;
    if (1 != sscanf(kernS+strlen(tkS), ":%f", param+1)) {
      fprintf(stderr, "%s: couldn't parse parameters \"%s\" for %s kernel\n", 
	      me, kernS+strlen(tkS), tkS);
      usage();
    }
    goto kparsed;
  }

  fprintf(stderr, "%s: couldn't parse kernel \"%s\"\n", me, kernS);
  usage();
  
 kparsed:

  if (!(file = fopen(out, "w"))) {
    fprintf(stderr, "%s: couldn't open \"%s\" for writing\n", me, out);
    exit(1);
  }

  bound = kern->support(param);
  for (i=0; i<=N-1; i++) {
    x = AIR_AFFINE(0, i, N-1, -bound, bound);
    f = kern->eval(x, param);
    fprintf(file, "%g %g\n", x, f);
  }

  fclose(file);
  exit(0);
}
