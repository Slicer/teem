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


#include "../nrrd.h"

int
main(int argc, char **argv) {
  char *err;
  Nrrd *nin, *nout;
  int i;
  nrrdResampleInfo *info;

  if (2 != argc) {
    printf("gimme something\n");
    exit(1);
  }
  if (!(nin = nrrdNewOpen(argv[1]))) {
    err = biffGet(NRRD);
    printf("nrrdNewOpen failed:\n%s\n", err);
    exit(1);
  }
  info = nrrdResampleInfoNew();
  info->boundary = nrrdBoundaryBleed;
  info->padValue = 128;
  for (i=0; i<=nin->dim-1; i++) {
    /*
    info->kernel[i] = nrrdKernelAQuartic;
    info->param[i][1] = 0.25;
    */
    info->kernel[i] = nrrdKernelBCCubic;
    info->param[i][1] = 0.0;
    info->param[i][2] = 0.5;
    /* info->samples[i] = 222; */
    info->samples[i] = 90;
    info->min[i] = 0;
    info->max[i] = nin->size[i]-1;
    printf("info->samples[%d] = %d\n", i, info->samples[i]);
  }
  info->kernel[1] = NULL;
  /*
  info->kernel[2] = NULL;
  info->kernel[0] = NULL;
  */
  /*
  info->samples[2] = 10;
  */
  nout = nrrdNew();
  if (nrrdSpatialResample(nout, nin, info)) {
    printf("%s\n", biffGet(NRRD));
  }
  nout->encoding = nrrdEncodingRaw;
  if (nrrdSave("out.nhdr", nout)) {
    printf("%s\n", biffGet(NRRD));
  }
  nrrdResampleInfoNix(info);
  nrrdNuke(nin);
  nrrdNuke(nout);
  exit(0);
}
