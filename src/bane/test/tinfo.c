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


#include "../bane.h"

char *me;

void
usage() {
  /*                      0     1       2     3    (4)  */
  fprintf(stderr, "usage: %s <hvolin> <dim> <nout>\n", me);
  exit(1);
}

int
main(int argc, char *argv[]) {
  Nrrd *hvol, *info;
  char *iStr, *dStr, *oStr;
  int dim;

  me = argv[0];
  if (argc != 4)
    usage();
  iStr = argv[1];
  dStr = argv[2];
  oStr = argv[3];

  if (nrrdLoad(hvol=nrrdNew(), iStr)) {
    fprintf(stderr, "%s: trouble reading hvol:\n%s\n", me, biffGet(NRRD));
    usage();
  }
  if (1 != sscanf(dStr, "%d", &dim)) {
    fprintf(stderr, "%s: trouble parsing %s as an int\n", me, dStr);
    usage();
  }
  if (baneOpacInfo(info = nrrdNew(), hvol, dim, nrrdMeasureHistoMean)) {
    fprintf(stderr, "%s: trouble calculting %d-D opacity info:\n%s\n",
	    me, dim, biffGet(BANE));
    exit(1);
  }
  if (nrrdSave(oStr, info, NULL)) {
    fprintf(stderr, "%s: trouble saving nrrd to %s:\n%s\n", me, oStr,
	    biffGet(NRRD));
    exit(1);
  }
  nrrdNuke(hvol);
  nrrdNuke(info);
  exit(0);
}
