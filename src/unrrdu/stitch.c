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
              /*  0    1      2       3      4          argc-2  argc-1 (argc)*/
  fprintf(stderr, 
	  "usage: %s <axis> <label> <nin0> <nin1> ... <nin(n-1)> <NameOut>\n",
	  me);
  fprintf(stderr, "       <axis> is which axis the given slices should\n");
  fprintf(stderr, "       lie along in the new (output) nrrd, that is,\n");
  fprintf(stderr, "       which axis along which you'd slice the new\n");
  fprintf(stderr, "       nrrd to get back the given slices\n");
  exit(1);
}

int
main(int argc, char *argv[]) {
  char *err, *label;
  int dim,         /* the dimension of the slices */
    sliceMemSize,  /* size in bytes of data of one slice */
    size,          /* number of slices we're putting together */
    axes[NRRD_MAX_DIM],
    pos, axis, j;
  char *data;  /* data pointer for ntmp */
  Nrrd **nin, *ntmp, *nout = NULL;

  me = argv[0];
  size = argc-4;
  if (!(size >= 1))
    usage();

  label = argv[2];
  if (1 != sscanf(argv[1], "%d", &axis)) {
    fprintf(stderr, "%s: couldn't parse %s as axis\n", me, argv[1]);
    exit(1);
  }
  printf("%s: planning to stitch %d slices\n", me, size);
  if (!(nin = (Nrrd **)calloc(size, sizeof(Nrrd *)))) {
    fprintf(stderr, "%s: couldn't alloc array of input nrrds\n", me);
    exit(1);
  }
  /* the first nrrd we read in sets the standard for dimension,
     number, type, and axis sizes that all subsequent nrrds have to
     match */
  if (!(nin[0] = nrrdNewLoad(argv[3]))) {
    err = biffGet(NRRD);
    fprintf(stderr, "%s: error reading nrrd from \"%s\":\n%s\n", 
	    me, argv[3], err);
    free(err);
    exit(1);
  }
  for (pos=1; pos<=size-1; pos++) {
    if (!(nin[pos] = nrrdNewLoad(argv[3+pos]))) {
      err = biffGet(NRRD);
      fprintf(stderr, "%s: error reading nrrd from \"%s\":\n%s\n", 
	      me, argv[3+pos], err);
      free(err);
      exit(1);
    }
    if (!(nin[pos]->dim == nin[0]->dim &&
	  nin[pos]->num == nin[0]->num &&
	  nin[pos]->type == nin[0]->type)) {
      fprintf(stderr, "%s: slice %d != slice 0 in dim, num, or type\n",
	      me, pos);
      exit(1);
    }
    for (j=0; j<=nin[0]->dim-1; j++) {
      if (!(nin[pos]->size[j] == nin[0]->size[j])) {
	fprintf(stderr, "%s: slice %d axis %d wrong size (%d, not %d)\n",
		me, pos, j, nin[pos]->size[j], nin[0]->size[j]);
	exit(1);
      }
    }
  }

  /* we've read in all the nrrds, now concatenate the data */

  /* make a new tmp nrrd, which we'll permute later */
  if (!(ntmp = nrrdNewAlloc(size*nin[0]->num, 
			    nin[0]->type, 
			    nin[0]->dim+1))) {
    err = biffGet(NRRD);
    fprintf(stderr, "%s: couldn't make tmp nrrd:%s\n", me, err);
    free(err);
    exit(1);
  }
  sprintf(ntmp->content, "(slices)");

  /* copy data from old slices into new space */
  data = ntmp->data;
  dim = nin[0]->dim;
  sliceMemSize = nin[0]->num*nrrdTypeSize[nin[0]->type];
  for (pos=0; pos<=size-1; pos++) {
    memcpy(data + pos*sliceMemSize, nin[pos]->data, sliceMemSize);
  }

  /* size and label arrays for new nrrd is same as slice, 
     but with new last entry */
  for (j=0; j<=dim-1; j++) {
    ntmp->size[j] = nin[0]->size[j];
    strcpy(ntmp->label[j], nin[0]->label[j]);
  }
  ntmp->size[dim] = size;
  strcpy(ntmp->label[dim], label);

  /* at this point we can ditch all the little nrrd slices */
  for (pos=0; pos<=size-1; pos++) {
    nrrdNuke(nin[pos]);
  }

  /* construct the axes ordering, and call permute */
  for (j=0; j<=dim; j++) {
    axes[j] = (j < axis 
	       ? j 
	       : (j == axis
		  ? dim
		  : j - 1));
    /* printf("   axes[%d] = %d\n", j, axes[j]); */
  }
  if (!(nout = nrrdNewPermuteAxes(ntmp, axes))) {
    err = biffGet(NRRD);
    fprintf(stderr, "%s: error permuting nrrd:\n%s", me, err);
    free(err);
    exit(1);
  }
  nout->encoding = nrrdEncodingRaw;
  if (nrrdSave(argv[argc-1], nout)) {
    err = biffGet(NRRD);
    fprintf(stderr, "%s: error writing nrrd to \"%s\":\n%s", 
	    me, argv[argc-1], err);
    free(err);
    exit(1);
  }

  free(nin);
  nrrdNuke(nout);
  exit(0);
}
