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

int
usage(char *me) {
                      /*  0     1       2          3    */
  fprintf(stderr, "usage: %s <nrrdIn> <axis> <baseNameOut>\n", me);
  return 1;
}

int
main(int argc, char *argv[]) {
  char *me, *in, *base, out[128], *err, format[128];
  int top, axis, pos, fit;
  Nrrd *nin, *nout = NULL;

  me = argv[0];
  if (4 != argc)
    return usage(me);
  if (1 != sscanf(argv[2], "%d", &axis)) {
    fprintf(stderr, "%s: couldn't parse %s as axis\n", me, argv[2]);
    return 1;
  }
  in = argv[1];
  base = argv[3];
  if (nrrdLoad(nin=nrrdNew(), in)) {
    err = biffGet(NRRD);
    fprintf(stderr, "%s: error reading nrrd from \"%s\":%s\n", me, in, err);
    free(err);
    return 1;
  }
  if (!(AIR_INSIDE(0, axis, nin->dim-1))) {
    fprintf(stderr, "%s: given axis (%d) outside range [0,%d]\n",
	    me, axis, nin->dim-1);
    return 1;
  }
  top = nin->axis[axis].size-1;
  if (top > 99999)
    sprintf(format, "%%s%%06d.nrrd");
  else if (top > 9999)
    sprintf(format, "%%s%%05d.nrrd");
  else if (top > 999)
    sprintf(format, "%%s%%04d.nrrd");
  else if (top > 99)
    sprintf(format, "%%s%%03d.nrrd");
  else if (top > 9)
    sprintf(format, "%%s%%02d.nrrd");
  else
    sprintf(format, "%%s%%01d.nrrd");
  nout = nrrdNew();
  for (pos=0; pos<=top; pos++) {
    if (nrrdSlice(nout, nin, axis, pos)) {
      err = biffGet(NRRD);
      fprintf(stderr, "%s: error slicing nrrd:%s\n", me, err);
      free(err);
      return 1;
    }
    if (0 == pos) {
      /* Wee if these slices would be better saved as PNM images.
	 Altering the file name will tell nrrdSave() to use a different
	 file format. */
      fit = nrrdFitsInFormat(nout, nrrdFormatPNM, AIR_FALSE);
      if (2 == fit) {
	strcpy(format + strlen(format) - 4, "pgm");
      }
      else if (3 == fit) {
	strcpy(format + strlen(format) - 4, "ppm");
      }
    }
    sprintf(out, format, base, pos);
    fprintf(stderr, "%s\n", out);
    if (nrrdSave(out, nout, NULL)) {
      err = biffGet(NRRD);
      fprintf(stderr, "%s: error writing nrrd to \"%s\":%s\n", me, out, err);
      free(err);
      return 1;
    }
  }
  nrrdNuke(nout);
  nrrdNuke(nin);
  return 0;
}
