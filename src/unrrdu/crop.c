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
	  "       min and max should be either a signed integer, or \n");
  fprintf(stderr, 
	  "       \"M\", \"M+n\", or \"M-n\", where M signifies top\n");
  fprintf(stderr, 
	  "       position along its respective axis (#samples - 1),\n");
  fprintf(stderr, "and +n/-n is an offset from that position\n");
  exit(1);
}

int
getint(char *str, int *n, int *offset) {

  *offset = 0;
  if ('M' == str[0]) {
    *n = INT_MAX;
    if (1 < strlen(str)) {
      if (('+' == str[1] || '-' == str[1])) {
	if (1 != sscanf(str+1, "%d", offset)) {
	  return 1;
	}
	/* else we succesfully parsed the offset */
      }
      else {
	/* something other that '+' or '-' after 'M' */
	return 1;
      }
    }
  }
  else {
    if (1 != sscanf(str, "%d", n)) {
      return 1;
    }
    /* else we successfully parsed n */
  }
  return 0;
}

int
main(int argc, char *argv[]) {
  char *inStr, *outStr, *err;
  int i, udim, min[NRRD_DIM_MAX], max[NRRD_DIM_MAX], 
    minoffset[NRRD_DIM_MAX], maxoffset[NRRD_DIM_MAX];
  Nrrd *nin, *nout;
  double t1, t2;

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
  if (nrrdLoad(nin=nrrdNew(), inStr)) {
    err = biffGet(NRRD);
    fprintf(stderr, "%s: trouble reading input:%s\n", me, err);
    free(err);
    exit(1);
  }
  if (udim != nin->dim) {
    fprintf(stderr, "%s: input nrrd has dimension %d, but given %d axes\n",
	    me, nin->dim, udim);
    exit(1);
  }
  for (i=0; i<=udim-1; i++) {
    if (INT_MAX == min[i])
      min[i] = nin->axis[i].size - 1 + minoffset[i];
    if (INT_MAX == max[i])
      max[i] = nin->axis[i].size - 1 + maxoffset[i];
    fprintf(stderr, "%s: axis % 2d: %d -> %d\n", me, i, min[i], max[i]);
  }

  nout = nrrdNew();
  t1 = airTime();
  if (nrrdSubvolume(nout, nin, min, max, 1)) {
    err = biffGet(NRRD);
    fprintf(stderr, "%s: error cropping nrrd:\n%s", me, err);
    free(err);
    exit(1);
  }
  t2 = airTime();
  printf("%s: nrrdSubvolume took %g seconds\n", me, t2-t1);
  if (nrrdSave(outStr, nout, NULL)) {
    err = biffGet(NRRD);
    fprintf(stderr, "%s: error writing nrrd:\n%s", me, err);
    free(err);
    exit(1);
  }

  nrrdNuke(nin);
  nrrdNuke(nout);
  exit(0);
}
