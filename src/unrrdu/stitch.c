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
                      /*  0      1        2          argc-3  argc-2  argc-1 */
  fprintf(stderr, "usage: %s <NameIn0> <NameIn1> ... <axis> <label> <NameOut>\n", me);
  fprintf(stderr, "       <axis> is which axis the given slices should\n");
  fprintf(stderr, "       lie along in the new (output) nrrd, that is,\n");
  fprintf(stderr, "       which axis along which you'd slice the new\n");
  fprintf(stderr, "       nrrd to get back the given slices\n");
  exit(1);
}

int
main(int argc, char *argv[]) {
  FILE *fin, *fout;
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

  label = argv[argc-2];
  printf("%s: planning to stitch %d slices\n", me, size);
  if (1 != sscanf(argv[argc-3], "%d", &axis)) {
    fprintf(stderr, "%s: couldn't parse %s as axis\n", me, argv[argc-3]);
    exit(1);
  }
  nin = (Nrrd **)calloc(size, sizeof(Nrrd *));
  if (!(fin = fopen(argv[1], "r"))) {
    fprintf(stderr, "%s: can't open %s for reading\n", me, argv[1]);
    exit(1);
  }
  if (!(nin[0] = nrrdNewRead(fin))) {
    err = biffGet(NRRD);
    fprintf(stderr, "%s: error reading nrrd:%s\n", me, err);
    exit(1);
  }
  for (pos=1; pos<=size-1; pos++) {
    if (!(fin = fopen(argv[1+pos], "r"))) {
      fprintf(stderr, "%s: can't open %s for reading\n", me, argv[1+pos]);
      exit(1);
    }
    if (!(nin[pos] = nrrdNewRead(fin))) {
      err = biffGet(NRRD);
      fprintf(stderr, "%s: error reading nrrd:%s\n", me, err);
      exit(1);
    }
    fclose(fin);
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
    fprintf(stderr, "%s: couldn't make tmp nrrd:%s\n", 
	    me, biffGet(NRRD));
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
    fprintf(stderr, "%s: error permuting nrrd:\n%s", me, biffGet(NRRD));
    exit(1);
  }
  if (!(fout = fopen(argv[argc-1], "w"))) {
    fprintf(stderr, "%s: couldn't open %s for writing\n", me, argv[argc-1]);
    exit(1);
  }
  nout->encoding = nrrdEncodingRaw;
  if (nrrdWrite(fout, nout)) {
    fprintf(stderr, "%s: error writing nrrd:\n%s", me, biffGet(NRRD));
    exit(1);
  }
  fclose(fout);
  free(nin);
  nrrdNuke(nout);
  exit(0);
}
