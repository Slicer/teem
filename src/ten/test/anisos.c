/*
  teem: Gordon Kindlmann's research software
  Copyright (C) 2002, 2001, 2000, 1999, 1998 University of Utah

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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
  float *tdata, *adata, *tensor, eval[3], evec[9], c[TEN_ANISO_MAX+1];

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
  
  if (nrrdAlloc(nout=nrrdNew(), nrrdTypeFloat, 4, 4, sx, sy, sz)) {
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
    fprintf(stderr, "%s: on z=%d of %d\n", me, z, sz-1);
    for (y=0; y<=sy-1; y++) {
      for (x=0; x<=sx-1; x++) {
  	I = x + sx*(y + sy*z);
	tensor = &(tdata[I*7]);
	tenEigensolve(eval, evec, tensor);
	if (6 == x && 6 == y && 12 == z) {
	  fprintf(stderr, "tensor (list): %g %g %g %g %g %g\n",
		  tensor[1], tensor[2], tensor[3], 
		  tensor[4], tensor[5], tensor[6]);
	  fprintf(stderr, " --> evals: %g %g %g\n", eval[0], eval[1], eval[2]);
	  fprintf(stderr, " --> evecs: (%g,%g,%g); (%g,%g,%g); (%g,%g,%g)\n",
		  evec[0], evec[1], evec[2], 
		  evec[3], evec[4], evec[5], 
		  evec[6], evec[7], evec[8]);
	}
	tenAnisoCalc(c, eval);
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
