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
  
  if (!(nin = nrrdNewLoad(ninStr))) {
    fprintf(stderr, "%s: trouble reading %s:\n%s\n", 
	    me, ninStr, biffGet(NRRD));
    exit(1);
  }
  if (!tenValidTensor(nin, nrrdTypeFloat, AIR_TRUE)) {
    fprintf(stderr, "%s: %s isn't a tensor nrrd:\n%s\n",
	    me, ninStr, biffGet(TEN));
    exit(1);
  }

  sx = nin->size[1];
  sy = nin->size[2]; 
  sz = nin->size[3];
  
  if (!(nout = nrrdNewAlloc(sx*sy*sz*4, nrrdTypeFloat, 4))) {
    fprintf(stderr, "%s: couldn't allocate anisotropy nrrd:\n%s\n",
	    me, biffGet(NRRD));
    exit(1);
  }
  nout->size[0] = 4;
  nout->size[1] = sx;
  nout->size[2] = sy;
  nout->size[3] = sz;
  nout->spacing[0] = nin->spacing[0];
  nout->spacing[1] = nin->spacing[1];
  nout->spacing[2] = nin->spacing[2];
  nout->spacing[3] = nin->spacing[3];
  strcpy(nout->label[0], "cl;cp;ca;ct");
  strcpy(nout->label[1], "x");
  strcpy(nout->label[2], "y");
  strcpy(nout->label[3], "z");

  tdata = nin->data;
  adata = nout->data;
  for (z=0; z<=sz-1; z++) {
    printf("%s: on z=%d of %d\n", me, z, sz-1);
    for (y=0; y<=sy-1; y++) {
      for (x=0; x<=sx-1; x++) {
  	I = x + sx*(y + sy*z);
	tensor = &(tdata[I*7]);
	tenEigensolve(eval, evec, tensor);
	tenAnisotropy(c, eval);
	adata[0 + 4*I] = tensor[0]*AIR_CLAMP(0, c[tenAnisoC_l], 1);
	adata[1 + 4*I] = tensor[0]*AIR_CLAMP(0, c[tenAnisoC_p], 1);
	adata[2 + 4*I] = tensor[0]*AIR_CLAMP(0, c[tenAnisoC_a], 1);
	adata[3 + 4*I] = tensor[0]*AIR_CLAMP(0, c[tenAnisoC_t], 1);
      }
    }
  }

  if (nrrdSave(noutStr, nout)) {
    fprintf(stderr, "%s: trouble writing %s:\n%s\n", 
	    me, noutStr, biffGet(NRRD));
    exit(1);
  }

  exit(0);
}
