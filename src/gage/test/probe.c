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


#include <stdio.h>
#include <hest.h>
#include <nrrd.h>
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
  float x, y, z, scale;
  gage_t *out;
  Nrrd *nin, *nout;
  int a, idx, what, ansLen, offset, E, xi, yi, zi,
    six, siy, siz, sox, soy, soz;
  double t0, t1, kparm[3][NRRD_KERNEL_PARMS_NUM];
  gageSclAnswer *san;
  gageSimple *gsl;
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
    fprintf(stderr, "%s: couldn't parse \"%s\" as gageScl\n", me, whatS);
    exit(1);
  }
  if (!( AIR_BETWEEN(gageSclUnknown, what, gageSclLast) )) {
    fprintf(stderr, "%s: what %d out of range [%d,%d]\n", me,
	    what, gageSclUnknown+1, gageSclLast-1);
    exit(1);
  }
  ansLen = gageSclAnsLength[what];
  printf("%s: ansLen = %d --> ", me, ansLen);
  if ((gageSclHessian == what) || (gageSclGeomTens == what)) {
    ansLen = 7;
  }
  printf("%d\n", ansLen);
  if (1 != sscanf(scaleS, "%f", &scale)) {
    fprintf(stderr, "%s: couldn't parse \"%s\" as float\n", me, scaleS);
    exit(1);
  }
  E = 0;
  if (!E) E |= nrrdKernelParse(&k0, kparm[0], k0S);
  if (!E) E |= nrrdKernelParse(&k1, kparm[1], k1S);
  if (!E) E |= nrrdKernelParse(&k2, kparm[2], k2S);
  if (E) {
    fprintf(stderr, "%s: problem parsing kernels:\n%s\n", me, biffGet(NRRD));
    exit(1);
  }

  /***
  **** Except for the gageSimpleProbe() call in the inner loop below,
  **** and the gageSimpleNix() call at the very end, all the gage
  **** calls which set up the simple context and state are here.
  ***/
  gsl = gageSimpleNew();
  gageValSet(gsl->ctx, gageValVerbose, 1);
  gageValSet(gsl->ctx, gageValRenormalize, AIR_TRUE);
  gageValSet(gsl->ctx, gageValCheckIntegrals, AIR_TRUE);
  E = 0;
  if (!E) E |= gageSimpleKernelSet(gsl, gageKernel00, k0, kparm[0]);
  if (!E) E |= gageSimpleKernelSet(gsl, gageKernel11, k1, kparm[1]);
  if (!E) E |= gageSimpleKernelSet(gsl, gageKernel22, k2, kparm[2]);
  if (E) {
    fprintf(stderr, "%s: trouble:\n%s\n", me, biffGet(GAGE));
    exit(1);
  }
  gsl->nin = nin;
  gsl->kind = gageKindScl;
  gsl->query = 1<<what;
  if (gageSimpleUpdate(gsl)) {
    fprintf(stderr, "%s: trouble:\n%s\n", me, biffGet(GAGE));
    exit(1);
  }
  gsl->pvl->verbose = gageValGet(gsl->ctx, gageValVerbose);
  san = (gageSclAnswer *)(gsl->pvl->ans);
  /***
  **** end gage setup.
  ***/

  six = nin->axis[0].size;
  siy = nin->axis[1].size;
  siz = nin->axis[2].size;
  sox = scale*six;
  soy = scale*siy;
  soz = scale*siz;
  if (ansLen > 1) {
    printf("%s: creating %d x %d x %d x %d output\n", 
	   me, ansLen, sox, soy, soz);
    if (!E) E != nrrdAlloc(nout=nrrdNew(), gage_nrrdType,
			      4, ansLen, sox, soy, soz);
  } else {
    printf("%s: creating %d x %d x %d output\n", me, sox, soy, soz);
    if (!E) E != nrrdAlloc(nout=nrrdNew(), gage_nrrdType,
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
    printf("%d/%d ", zi, soz-1); fflush(stdout);
    z = AIR_AFFINE(0, zi, soz-1, 0, siz-1);
    for (yi=0; yi<=soy-1; yi++) {
      y = AIR_AFFINE(0, yi, soy-1, 0, siy-1);
      for (xi=0; xi<=sox-1; xi++) {
	x = AIR_AFFINE(0, xi, sox-1, 0, six-1);
	idx = xi + sox*(yi + soy*zi);

	gsl->ctx->verbose = 3*( !xi && !yi && !zi ||
				/* ((100 == xi) && (8 == yi) && (8 == zi)) */
				((61 == xi) && (51 == yi) && (46 == zi))
				/* ((40==xi) && (30==yi) && (62==zi)) || */
				/* ((40==xi) && (30==yi) && (63==zi)) */ ); 

	if (gageSimpleProbe(gsl, x, y, z)) {
	  fprintf(stderr, 
		  "%s: trouble at i=(%d,%d,%d) -> f=(%g,%g,%g):\n%s\n(%d)\n",
		  me, xi, yi, zi, x, y, z, gageErrStr, gageErrNum);
	  exit(1);
	}
	switch (what) {
	case gageSclHessian:
	  TEN_MAT2LIST(out + 7*idx, san->hess);
	  out[0 + 7*idx] = 1.0;
	  break;
	case gageSclGeomTens:
	  TEN_MAT2LIST(out + 7*idx, san->gten);
	  out[0 + 7*idx] = 1.0;
	  break;
	default:
	  if (1 == ansLen) {
	    out[ansLen*idx] = san->ans[offset];
	  } else {
	    for (a=0; a<=ansLen-1; a++) {
	      out[a + ansLen*idx] = san->ans[a + offset];
	    }
	  }
	  break;
	}
      }
    }
  }
  printf("\n");
  t1 = airTime();
  printf("probe rate = %g/sec\n", sox*soy*soz/(t1-t0));
  nrrdSave(noutS, nout, NULL);

  nrrdNuke(nin);
  nrrdNuke(nout);
  gageSimpleNix(gsl);
  exit(0);
}
