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

char *me; 

void
usage() {
              /*  0    1    2  ... argc-6   argc-5  argc-4  argc-3   argc-2    argc-1 */
  fprintf(stderr, 
	  "usage: %s <n0> <n1> ... <n(n-1)> <axis> <label> <spacing> <incrDim> <nout>\n",
	  me);
  fprintf(stderr, "       <n0> <n1> ... <n(n-1)> are Nrrds to be joined\n");
  fprintf(stderr, "       <axis> is which axis the given parts should lie along in the new (output)\n");
  fprintf(stderr, "       nrrd, that is, which axis along which you'd slice/crop the new nrrd\n");
  fprintf(stderr, "       to get back the given parts\n");
  fprintf(stderr, "       <label> and <spacing> are for the new axis\n");
  fprintf(stderr, "       <incrDim> non-zero means that you really want to add a dimension to\n");
  fprintf(stderr, "       the nrrd, as opposed to arranging them side by side\n");
  exit(1);
}

int
main(int argc, char *argv[]) {
  char *err, *label, *axisS, *spacingS, *out, *incrS;
  int i, axis, num, incr;
  float spacing;
  Nrrd **nin, *nout;

  me = argv[0];
  num = argc-6;
  if (!(num >= 1))
    usage();
  axisS = argv[argc-5];
  label = argv[argc-4];
  spacingS = argv[argc-3];
  incrS = argv[argc-2];
  out = argv[argc-1];
  
  if (1 != sscanf(axisS, "%d", &axis)) {
    fprintf(stderr, "%s: couldn't parse axis \"%s\" as int\n", 
	    me, axisS);
    exit(1);
  }
  if (1 != sscanf(spacingS, "%f", &spacing)) {
    fprintf(stderr, "%s: couldn't parse spacing \"%s\" as float\n", 
	    me, spacingS);
    exit(1);
  }
  if (1 != sscanf(incrS, "%d", &incr)) {
    fprintf(stderr, "%s: couldn't parse incrDim \"%s\" as int\n", 
	    me, incrS);
    exit(1);
  }
  fprintf(stderr, "%s: planning to join %d parts\n", me, num);
  if (!(nin = (Nrrd **)calloc(num, sizeof(Nrrd *)))) {
    fprintf(stderr, "%s: couldn't alloc array of input Nrrd pointers\n", me);
    exit(1);
  }
  for (i=0; i<=num-1; i++) {
    if (nrrdLoad(nin[i]=nrrdNew(), argv[1+i])) {
      err = biffGet(NRRD);
      fprintf(stderr, "%s: error loading nrrd #%d from \"%s\":\n%s\n",
	      me, i, argv[1+i], err);
      free(err); 
      exit(1);
    }
  }

  /* do the deed */
  nout = nrrdNew();
  if (nrrdJoin(nout, nin, num, axis, incr)) {
    err = biffGet(NRRD);
    fprintf(stderr, "%s: trouble joining all inputs:\n%s\n", me, err);
    free(err); 
    exit(1);
  }
  nout->axis[axis].spacing = spacing;
  nout->axis[axis].label = airStrdup(label);

  /* nuke all the inputs */
  for (i=0; i<=num-1; i++) {
    nin[i] = nrrdNuke(nin[i]);
  }
  free(nin);

  /* save the output */
  if (nrrdSave(out, nout, NULL)) {
    err = biffGet(NRRD);
    fprintf(stderr, "%s: error saving nrrd to \"%s\":\n%s", me, out, err);
    free(err);
    exit(1);
  }
  nout = nrrdNuke(nout);
  
  exit(0);
}
