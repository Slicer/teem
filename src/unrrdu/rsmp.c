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

#include "private.h"

char *rsmpName = "rsmp";
char *rsmpInfo = "Upsampling and downsampling with a seperable kernel";

typedef struct {
  nrrdKernel *kernel;
  double param[NRRD_KERNEL_PARAMS_MAX];
} unuNrrdKernel;

int
unuParseKernel(void *ptr, char *str, char err[AIR_STRLEN_HUGE]) {
  unuNrrdKernel *ker;
  char me[]="unuParseKernel", *nerr;
  int *typeP;

  if (!(ptr && str)) {
    sprintf(err, "%s: got NULL pointer", me);
    return 1;
  }
  ker = ptr;
  if (nrrdKernelParse(&(ker->kernel), ker->param, str)) {
    nerr = biffGetDone(NRRD);
    if (strlen(nerr) > AIR_STRLEN_HUGE - 1)
      nerr[AIR_STRLEN_HUGE - 1] = '\0';
    strcpy(err, nerr);
    free(nerr);
    return 1;
  }
  return 0;
}

hestCB unuKernelHestCB = {
  sizeof(unuNrrdKernel),
  "kernel specification",
  unuParseKernel,
  NULL
};

int
unuParseScale(void *ptr, char *str, char err[AIR_STRLEN_HUGE]) {
  float *scale;
  int num;
  
  if (!(ptr && str)) {
    sprintf(err, "%s: got NULL pointer", me);
    return 1;
  }
  scale = ptr;
  if (!strcmp("=", str)) {
    scale[0] = 0.0;
    scale[1] = 0.0;
    return 0;
  }

  /* else */
  if ('x' == str[0]) {
    if (1 != sscanf(str+1, "%f", scale+1)) {
      sprintf(err, "%s: can't parse \"%s\" as x<float>", me, str);
      return 1;
    }
    scale[0] = 1.0;
  }
  else {
    if (1 != sscanf(str, "%d", &num)) {
      sprintf(err, "%s: can't parse \"%s\" as int", me, str);
      return 1;
    }
    scale[0] = 2.0;
    scale[1] = num;
  }
  return 0;
}

hestCB unuScaleHestCB = {
  2*sizeof(float),
  "sampling specification",
  unuParseScale,
  NULL
};

int
rsmpMain(int argc, char **argv, char *me) {
  hestOpt *opt = NULL;
  char *out, *err;
  Nrrd *nin, *nout;
  int axis;
  airArray *mop;

  OPT_ADD_NIN(nin, "input");
  OPT_ADD_AXIS(axis, "axis to slice along");
  OPT_ADD_NOUT(out, "output nrrd");

  mop = airMopInit();
  airMopAdd(mop, opt, (airMopper)hestOptFree, airMopAlways);

  USAGE(rsmpInfo);
  PARSE();

  nout = nrrdNew();
  airMopAdd(mop, nout, (airMopper)nrrdNuke, airMopAlways);

  /*
  if (nrrdFlip(nout, nin, axis)) {
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: error resampling nrrd:\n%s", me, err);
    airMopError(mop);
    return 1;
  }
  */

  SAVE();

  airMopOkay(mop);
  return 0;
}
