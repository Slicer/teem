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


#include <ten.h>

char *me;

void
usage() {
  /*                      0     1         2    (3) */
  fprintf(stderr, "usage: %s <nrrdIn> <nrrdOut>\n", me);
  exit(1);
}

int
main(int argc, char **argv) {
  char *ninStr, *noutStr;
  int I, x, y, z, sx, sy, sz;
  Nrrd *nin, *nout;
  float *tdata, *adata, *tensor, eval[3], evec[9], c[TEN_MAX_ANISO+1];

  me = argv[0];
  if (3 != argc)
    usage();
  
  ninStr = argv[1];
  noutStr = argv[2];
  
  if (nrrdLoad(nin=nrrdNew(), ninStr)) {
    fprintf(stderr, "%s: trouble reading %s:\n%s\n", 
	    me, ninStr, biffGet(NRRD));
    exit(1);
  }
  if (!tenValidTensor(nin, nrrdTypeFloat, AIR_TRUE)) {
    fprintf(stderr, "%s: %s isn't a tensor nrrd:\n%s\n",
	    me, ninStr, biffGet(TEN));
    exit(1);
  }

  sx = nin->axis[1].size;
  sy = nin->axis[2].size; 
  sz = nin->axis[3].size;
  
  if (nrrdAlloc_va(nout=nrrdNew(), nrrdTypeFloat, 4,
		   4, sx, sy, sz)) {
    fprintf(stderr, "%s: couldn't allocate anisotropy nrrd:\n%s\n",
	    me, biffGet(NRRD));
    exit(1);
  }
  nout->axis[0].spacing = nin->axis[0].spacing;
  nout->axis[1].spacing = nin->axis[1].spacing;
  nout->axis[2].spacing = nin->axis[2].spacing;
  nout->axis[3].spacing = nin->axis[3].spacing;
  nout->axis[0].label = airStrdup("cl;cp;ca;ct");
  nout->axis[1].label = airStrdup("x");
  nout->axis[2].label = airStrdup("y");
  nout->axis[3].label = airStrdup("z");

  tdata = nin->data;
  adata = nout->data;
  for (z=0; z<=sz-1; z++) {
    printf("%s: on z=%d of %d\n", me, z, sz-1);
    for (y=0; y<=sy-1; y++) {
      for (x=0; x<=sx-1; x++) {
  	I = x + sx*(y + sy*z);
	tensor = &(tdata[I*7]);
	tenEigensolve(eval, evec, tensor);
	if (6 == x && 6 == y && 12 == z) {
	  printf("tensor (list): %g %g %g %g %g %g\n",
		 tensor[1], tensor[2], tensor[3], 
		 tensor[4], tensor[5], tensor[6]);
	  printf(" --> evals: %g %g %g\n", eval[0], eval[1], eval[2]);
	  printf(" --> evecs: (%g,%g,%g); (%g,%g,%g); (%g,%g,%g)\n",
		 evec[0], evec[1], evec[2], 
		 evec[3], evec[4], evec[5], 
		 evec[6], evec[7], evec[8]);
	}
	tenAnisotropy(c, eval);
	adata[0 + 4*I] = tensor[0]*c[tenAniso_Cl];
	adata[1 + 4*I] = tensor[0]*c[tenAniso_Cp];
	adata[2 + 4*I] = tensor[0]*c[tenAniso_Ca];
	adata[3 + 4*I] = tensor[0]*c[tenAniso_Ct];
      }
    }
  }

  if (nrrdSave(noutStr, nout, NULL)) {
    fprintf(stderr, "%s: trouble writing %s:\n%s\n", 
	    me, noutStr, biffGet(NRRD));
    exit(1);
  }

  exit(0);
}
