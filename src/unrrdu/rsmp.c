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

void
usage(char *me) {
  /*              0    1      2      3       4       (argc-1) */
  fprintf(stderr, 
	  "usage: %s <nin> <kern> <size0> <size1> ... <nout>\n\n", me);
  fprintf(stderr, "<kern> is the resampling kernel to use; options are:\n");
  fprintf(stderr, "  \"box\": box filter (nearest neighbor upsampling)\n");
  fprintf(stderr, "  \"tent\": tent filter (trilinear upsampling)\n");
  fprintf(stderr, "  \"cubic:B,C\": Mitchell/Netravali BC-family of piecewise cubics\n");
  fprintf(stderr, "     \"cubic:1,0\": B-spline w/ maximal blurring\n");
  fprintf(stderr, "     \"cubic:0,0.5\": Catmull-Rom kernel\n");
  fprintf(stderr, "    (\"cubic:0,C\": all interpolating cubics)\n");
  fprintf(stderr, "  \"quartic:A\": 1-parameter family of interpolating quartics\n");
  fprintf(stderr, "     \"quartic:0.0834\": minimizes aliasing (in one sense)\n");
  fprintf(stderr, "<sizeN> is per-axis resizing info; options are:\n");
  fprintf(stderr, "  \"=\": no resampling at all on this axis\n");
  fprintf(stderr, "  <int>: exact number of samples desired\n");
  fprintf(stderr, "  x<float>: resize ratio; (\"x0.5\": shrink by half; \"x3\": scale by three)\n\n");
  exit(1);
}

int
main(int argc, char *argv[]) {
  char *me, *in, *out, *err, *kernS, *sizeS;
  Nrrd *nin, *nout;
  nrrdResampleInfo *info;
  nrrdKernel *kern;
  double param[NRRD_KERNEL_PARAMS_MAX];
  float ratio;
  int d, size;

  me = argv[0];
  if (!(argc >= 5)) {
    usage(me);
  }
  in = argv[1];
  kernS = argv[2];
  out = argv[argc-1];
  if (nrrdLoad(nin=nrrdNew(), in)) {
    err = biffGet(NRRD);
    fprintf(stderr, "%s: trouble reading nrrd from \"%s\":%s\n", me, in, err);
    free(err);
    usage(me);
  }
  memset(param, 0, NRRD_KERNEL_PARAMS_MAX*sizeof(float));
  
  /* parse the kernel */
  if (nrrdKernelParse(&kern, param, kernS)) {
    fprintf(stderr, "%s: trouble parsing kernel:\n%s\n", me, biffGet(NRRD));
    exit(1);
  }

  info = nrrdResampleInfoNew();
  if (argc-4 != nin->dim) {
    fprintf(stderr, "%s: read in %d-D nrrd, but got %d resampling sizes\n",
	    me, nin->dim, argc-4);
    exit(1);
  }
  /* for the curious, the constraints/simplifications in this unrrdu,
     compared to the full options for resampler are:
     - always resampling full length of axis (min = 0, max = size-1)
       (no cropping, padding simultaneously with resampling) 
     - same kernel, with same parameters, for every axis
     - no rescaling of kernel (param[0] == 1.0)
     - boundary behavior is always bleed
     - output type is always input type
     - renormalization hack always on 
  */
  for (d=0; d<=nin->dim-1; d++) {
    sizeS = argv[3+d];
    if (!strcmp("=", sizeS)) {
      /* no resampling on this axis desired */
      info->kernel[d] = NULL;
      continue;
    }
    if ('x' == sizeS[0]) {
      if (1 != sscanf(sizeS+1, "%g", &ratio)) {
	fprintf(stderr, "%s: couldn't parse float in \"%s\"\n",	me, sizeS);
	usage(me);
      }
      info->samples[d] = nin->axis[d].size*ratio;
      if (!(info->samples[d] > 0)) {
	fprintf(stderr, "%s: invalid # samples (%d), axis %d (ratio=%g)\n", 
		me, info->samples[d], d, ratio);
	usage(me);
      }
    }
    else {
      if (1 != sscanf(sizeS, "%d", &size)) {
	fprintf(stderr, "%s: couldn't parse int in \"%s\"\n", me, sizeS);
	usage(me);
      }
      info->samples[d] = size;
    }
    info->kernel[d] = kern;
    memcpy(info->param[d], param, NRRD_KERNEL_PARAMS_MAX*sizeof(float));
    if (!( AIR_EXISTS(nin->axis[d].min) && AIR_EXISTS(nin->axis[d].max) ))
      nrrdAxisSetMinMax(nin, d);
    info->min[d] = nin->axis[d].min;
    info->max[d] = nin->axis[d].max;
  }
  info->boundary = nrrdBoundaryBleed;
  info->type = nin->type;
  info->renormalize = AIR_TRUE;

  nout = nrrdNew();
  if (nrrdSpatialResample(nout, nin, info)) {
    err = biffGet(NRRD);
    fprintf(stderr, "%s: trouble resampling:\n%s\n", me, err);
    free(err);
    exit(1);
  }
  if (nrrdSave(out, nout, NULL)) {
    fprintf(stderr, "%s: error writing nrrd:\n%s", me, biffGet(NRRD));
    exit(1);
  }
  nrrdNuke(nin);
  nrrdNuke(nout);
  nrrdResampleInfoNix(info);
  exit(0);
}
