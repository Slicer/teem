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
#include <limits.h>

char *me; 

void
usage() {
                      
  fprintf(stderr, /* 0  1        2        3                 argc-2  argc-1*/ 
	  "usage: %s <nIn> <ax0min> <ax0max> <ax1min> ... <axN-1max> <nOut>\n",
	  me);
  fprintf(stderr, 
	  "       axis min and max can be positive, negative, or \"-\"\n");
  exit(1);
}

int
getint(char *str, int *n, int *offset) {
  
  *offset = 0;
  if (!strcmp(str, "-")) {
    *n = INT_MAX;
    return(0);
  }
  else if (!strcmp(str, "--")) {
    *n = INT_MIN;
    return(0);
  }
  else if ('-' == str[0]) {
    if (1 != sscanf(str+1, "%d", n))
      return(1);
    *n = -*n;
    *offset = 1;
    return(0);
  }
  else if ('+' == str[0]) {
    if (1 != sscanf(str+1, "%d", n))
      return(1);
    *offset = 1;
    return(0);
  }
  else if (1 != sscanf(str, "%d", n)) {
    return(1);
  }
  else {
    return(0);
  }
}

int
main(int argc, char *argv[]) {
  char *inStr, *outStr, *errStr;
  int i, udim, min[NRRD_MAX_DIM], max[NRRD_MAX_DIM], 
    minoffset[NRRD_MAX_DIM], maxoffset[NRRD_MAX_DIM];
  Nrrd *nin, *nout;

  me = argv[0];
  if (0 != (argc-3) % 2) {
    usage();
  }
  udim = (argc - 3)/2;
  if (!(udim > 1)) {
    usage();
  }
  inStr = argv[1];
  outStr = argv[argc-1];
  for (i=0; i<=udim-1; i++) {
    if (getint(argv[2 + 2*i + 0], min + i, minoffset + i)) {
      printf("%s: Couldn't parse min for axis %d, \"%s\"\n",
	     me, i, argv[2 + 2*i + 0]);
      usage();
    }
    if (getint(argv[2 + 2*i + 1], max + i, maxoffset + i)) {
      printf("%s: Couldn't parse max for axis %d, \"%s\"\n",
	     me, i, argv[2 + 2*i + 1]);
      usage();
    }
  }
  if (!(nin = nrrdNewOpen(inStr))) {
    errStr = biffGet(NRRD);
    fprintf(stderr, "%s: trouble reading input:%s\n", me, errStr);
    exit(1);
  }
  if (udim != nin->dim) {
    fprintf(stderr, "%s: input nrrd has dimension %d, but given %d axes\n",
	    me, nin->dim, udim);
    exit(1);
  }
  for (i=0; i<=udim-1; i++) {
    if (minoffset[i]) {
      min[i] = -min[i];
    }
    else {
      min[i] = min[i] == INT_MAX ? 0 : min[i];
      min[i] = min[i] == INT_MIN ? nin->size[i] - 1 : min[i];
    }
    if (maxoffset[i]) {
      max[i] += nin->size[i]-1;
    }
    else {
      max[i] = max[i] == INT_MAX ? nin->size[i] - 1 : max[i];
      max[i] = max[i] == INT_MIN ? 0 : max[i];
    }
    printf("%s: axis %d: %d --> %d\n", me, i, min[i], max[i]);
  }
  if (!(nout = nrrdNewSubvolume(nin, min, max, 1))) {
    fprintf(stderr, "%s: error cropping nrrd:\n%s", me, biffGet(NRRD));
    exit(1);
  }
  nout->encoding = nin->encoding;
  if (nrrdSave(outStr, nout)) {
    fprintf(stderr, "%s: error writing nrrd:\n%s", me, biffGet(NRRD));
    exit(1);
  }
  nrrdNuke(nin);
  nrrdNuke(nout);
  exit(0);
}
