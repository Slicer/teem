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


#include <stdio.h>
#include "../gage.h"

#define TEN_LIST2MAT(m, l) ( \
   (m)[0] = (l)[1],          \
   (m)[1] = (l)[2],          \
   (m)[2] = (l)[3],          \
   (m)[3] = (l)[2],          \
   (m)[4] = (l)[4],          \
   (m)[5] = (l)[5],          \
   (m)[6] = (l)[3],          \
   (m)[7] = (l)[5],          \
   (m)[8] = (l)[6] )

#define TEN_MAT2LIST(l, m) ( \
   (l)[1] = (m)[0],          \
   (l)[2] = (m)[3],          \
   (l)[3] = (m)[6],          \
   (l)[4] = (m)[4],          \
   (l)[5] = (m)[7],          \
   (l)[6] = (m)[8] )


void
usage(char *me) {
  /*               0   1      2      3     4    5    6     7    (8) */
  fprintf(stderr, 
	  "usage: %s <nin> <what> <scale> <k0> <k1> <k2> <nout>\n", me);
  exit(1);
}

int
main(int argc, char *argv[]) {
  char *me, *ninS, *whatS, *scaleS, *k0S, *k1S, *k2S, *noutS;
  float x, y, z, scale, *out;
  Nrrd *nin, *npad, *nout;
  int a, idx, what, ansLen, offset, E, xi, yi, zi, pad,
    six, siy, siz, sox, soy, soz;
  double t0, t1, param[3][NRRD_KERNEL_PARAMS_MAX];
  gageSclContext *ctx;
  NrrdKernel *k0, *k1, *k2;

  me = argv[0];
  if (8 != argc) 
    usage(me);
  ninS = argv[1];
  whatS = argv[2];
  scaleS = argv[3];
  k0S = argv[4];
  k1S = argv[5];
  k2S = argv[6];
  noutS = argv[7];

  if (nrrdLoad(nin=nrrdNew(), ninS)) {
    fprintf(stderr, "%s: trouble:\n%s\n", me, biffGet(NRRD));
    exit(1);
  }
  if (3 != nin->dim) {
    fprintf(stderr, "%s: need a 3-dimensional nrrd (not %d)\n", me, nin->dim);
    exit(1);
  }
  if (gageSclUnknown == (what = airEnumVal(gageScl, whatS))) {
    fprintf(stderr, "%s: couldn't parse \"%s\" as int\n", me, whatS);
    exit(1);
  }
  if (!( AIR_BETWEEN(gageSclUnknown, what, gageSclLast) )) {
    fprintf(stderr, "%s: what %d out of range [%d,%d]\n", me,
	    what, gageSclUnknown+1, gageSclLast-1);
    exit(1);
  }
  ansLen = gageSclAnsLength[what];
  printf("ansLen = %d --> ", ansLen);
  if ((gageSclHess == what) || (gageSclGeomTens == what)) {
    ansLen = 7;
  }
  printf("%d\n", ansLen);
  if (1 != sscanf(scaleS, "%f", &scale)) {
    fprintf(stderr, "%s: couldn't parse \"%s\" as float\n", me, scaleS);
    exit(1);
  }
  E = 0;
  if (!E) E |= nrrdKernelParse(&k0, param[0], k0S);
  if (!E) E |= nrrdKernelParse(&k1, param[1], k1S);
  if (!E) E |= nrrdKernelParse(&k2, param[2], k2S);
  if (E) {
    fprintf(stderr, "%s: problem parsing kernels:\n%s\n", me, biffGet(NRRD));
    exit(1);
  }

  ctx = gageSclContextNew();
  ctx->verbose = 1;   /* but this is reset when we start traversing */
  ctx->renormalize = AIR_TRUE;
  E = 0;
  if (!E) E |= gageSclSetQuery(ctx, 1<<what);
  if (!E) E |= gageSclSetKernel(ctx, gageKernel00, k0, param[0]);
  if (!E) E |= gageSclSetKernel(ctx, gageKernel11, k1, param[1]);
  if (!E) E |= gageSclSetKernel(ctx, gageKernel22, k2, param[2]);
  if (E) {
    fprintf(stderr, "%s: trouble:\n%s\n", me, biffGet(GAGE));
    exit(1);
  }
  pad = gageSclGetNeedPad(ctx);
  if (nrrdSimplePad(npad=nrrdNew(), nin, 2*pad, nrrdBoundaryBleed)) {
    fprintf(stderr, "%s: trouble:\n%s\n", me, biffGet(NRRD));
    exit(1);
  }
  if (!E) E |= gageSclSetPaddedVolume(ctx, 2*pad, npad);
  if (!E) E |= gageSclUpdate(ctx);
  if (E) {
    fprintf(stderr, "%s: trouble:\n%s\n", me, biffGet(GAGE));
    exit(1);
  }

  six = nin->axis[0].size;
  siy = nin->axis[1].size;
  siz = nin->axis[2].size;
  sox = scale*six;
  soy = scale*siy;
  soz = scale*siz;
  if (ansLen > 1) {
    printf("%s: creating %d x %d x %d x %d output\n", 
	   me, ansLen, sox, soy, soz);
    if (!E) E != nrrdAlloc(nout=nrrdNew(), nrrdTypeFloat, 
			      4, ansLen, sox, soy, soz);
  }
  else {
    printf("%s: creating %d x %d x %d output\n", me, sox, soy, soz);
    if (!E) E != nrrdAlloc(nout=nrrdNew(), nrrdTypeFloat, 
			   3, sox, soy, soz);
  }
  if (E) {
    fprintf(stderr, "%s: trouble:\n%s\n", me, biffGet(NRRD));
    exit(1);
  }
  out = nout->data;
  offset = gageSclAnsOffset[what];
  t0 = airTime();
  fprintf(stderr, "%s: si{x,y,z} = %d, %d, %d\n", me, six, siy, siz);
  fprintf(stderr, "%s: so{x,y,z} = %d, %d, %d\n", me, sox, soy, soz);
  for (zi=0; zi<=soz-1; zi++) {
    /* printf("%s: z = % 3d/%d\n", me, zi, soz-1); */
    z = AIR_AFFINE(0, zi, soz-1, 0, siz-1);
    for (yi=0; yi<=soy-1; yi++) {
      y = AIR_AFFINE(0, yi, soy-1, 0, siy-1);
      for (xi=0; xi<=sox-1; xi++) {
	x = AIR_AFFINE(0, xi, sox-1, 0, six-1);
	idx = xi + sox*(yi + soy*zi);

	ctx->verbose = 2*( ((40 == xi) && (30 == yi) && (62 == zi)) ||
			   ((40 == xi) && (30 == yi) && (63 == zi)) );

	gageSclProbe(ctx, x, y, z);
	switch (what) {
	case gageSclHess:
	  TEN_MAT2LIST(out + 7*idx, ctx->hess);
	  out[0 + 7*idx] = 1.0;
	  break;
	case gageSclGeomTens:
	  TEN_MAT2LIST(out + 7*idx, ctx->gten);
	  out[0 + 7*idx] = 1.0;
	  break;
	default:
	  if (1 == ansLen) {
	    out[ansLen*idx] = ctx->ans[offset];
	  }
	  else {
	    for (a=0; a<=ansLen-1; a++) {
	      out[a + ansLen*idx] = ctx->ans[a + offset];
	    }
	  }
	  break;
	}
      }
    }
  }
  t1 = airTime();
  printf("probe rate = %g/sec\n", sox*soy*soz/(t1-t0));
  nrrdSave(noutS, nout, NULL);

  nrrdNuke(nin);
  nrrdNuke(npad);
  nrrdNuke(nout);
  gageSclContextNix(ctx);
  exit(0);
}
