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


#include "limn.h"

int 
limnWriteAsOBJ(FILE *file, limnObj *obj) {
  char me[] = "limnWriteAsOBJ", err[128];
  int i, j, vidx;

  if (!(file && obj)) {
    sprintf(err, "%s: got NULL pointer\n", me);
    biffSet(LIMN, err); return 1;
  }

  if (limnNormHC(obj)) {
    sprintf(err, "%s: trouble normalizing homog. coords", me);
    biffSet(LIMN, err); return 1;
  }

  fprintf(file, "# %d vertices\n", obj->numP);
  for (i=0; i<=obj->numP-1; i++) {
    fprintf(file, "v %f %f %f\n", 
	    obj->p[i].w[0], obj->p[i].w[1], obj->p[i].w[2]);
  }
  fprintf(file, "\n");
  fprintf(file, "# %d faces\n", obj->numF);
  for (i=0; i<=obj->numF-1; i++) {
    fprintf(file, "f");
    vidx = obj->f[i].vidx;
    for (j=0; j<=obj->f[i].sides-1; j++) {
      fprintf(file, " %d", obj->v[vidx+j] + 1);
      if (1 == j % 15)
	fprintf(file, "\n");
    }
    fprintf(file, "\n");
  }
  return(0);
}
