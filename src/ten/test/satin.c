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


#include "../ten.h"

int debug = 0, verbose;
int i, xi, yi, zi;

double v1[3], v2[3], v3[3], m1[9], m2[9], m3[9], m4[9], t[9];

/************************************************************************/
/************************************************************************/
/************************************************************************/

char desc[1024];

void
satinEval(double ev[3], float parm, double x, double y, double z) {
  double bound, bound1, bound2, r;

  r = sqrt(x*x + y*y + z*z);
  bound1 = 0.5 - 0.5*airErf(20*(r-0.9));  /* 1 on inside, 0 on outside */
  bound2 = 0.5 - 0.5*airErf(20*(0.7-r));
  bound = AIR_MIN(bound1, bound2);        /* and 0 on the very inside too */

#define BLAH(bound, tmp) AIR_AFFINE(0.0, bound, 1.0, 1.0/3.0, tmp)
  
  ev[0] = 5*BLAH(bound, AIR_AFFINE(0.0, parm, 2.0, 1.0, 0.0001));
  ev[1] = 5*BLAH(bound, AIR_AFFINE(0.0, parm, 2.0, 0.0001, 1.0));
  ev[2] = 5*BLAH(bound, 0.0001);

  return;
}

void
satinEvec(double v1[3], double v2[3], double x, double y, double z) {
  double norm, tmp[3];
  
  /* v1: looking towards positive Y, points counter clockwise */
  v1[0] = -z;
  v1[1] = 0;
  v1[2] = x;
  ELL_3V_NORM(v1, v1, norm);

  /* v2: points towards pole at positive Y */
  tmp[0] = -x;
  tmp[1] = -y;
  tmp[2] = -z;
  ELL_3V_NORM(tmp, tmp, norm);
  ELL_3V_CROSS(v2, tmp, v1);
}

/************************************************************************/
/************************************************************************/
/************************************************************************/

void
matrixC(double *v1, double *v2, double *v3, double *m) {

  m[0] = v1[0]; m[1] = v2[0]; m[2] = v3[0];
  m[3] = v1[1]; m[4] = v2[1]; m[5] = v3[1];
  m[6] = v1[2]; m[7] = v2[2]; m[8] = v3[2];
}

void
matrixR(double *v1, double *v2, double *v3, double *m) {

  m[0] = v1[0]; m[1] = v1[1]; m[2] = v1[2];
  m[3] = v2[0]; m[4] = v2[1]; m[5] = v2[2];
  m[6] = v3[0]; m[7] = v3[1]; m[8] = v3[2];
}

void
diagonal(double a, double b, double c, double *m) {

  m[0] = a; m[1] = 0; m[2] = 0;
  m[3] = 0; m[4] = b; m[5] = 0;
  m[6] = 0; m[7] = 0; m[8] = c;
}

void
satinTensor(double mat[9], float parm, double x, double y, double z) {
  double norm, v1[3], v2[3], v3[3], m1[9], ev[3];
  
  satinEvec(v1, v2, x, y, z);
  ELL_3V_CROSS(v3, v1, v2);
  ELL_3V_NORM(v3, v3, norm);
  if (debug) {
    printf("xi = %d; yi = %d; zi = %d\n", xi, yi, zi);
    printf("x = %g, y = %g, z = %g\n", x, y, z);
    printf("evects: v1: % 21.10f % 21.10f % 21.10f\n", v1[0], v1[1], v1[2]);
    printf("        v2: % 21.10f % 21.10f % 21.10f\n", v2[0], v2[1], v2[2]);
    printf("        v3: % 21.10f % 21.10f % 21.10f\n", v3[0], v3[1], v3[2]);
  }
  satinEval(ev, parm, x, y, z);
  matrixR(v1, v2, v3, m1);
  ELL_3M_ZERO_SET(m2);
  ELL_3M_DIAG_SET(m2, ev[0], ev[1], ev[2]);
  if (debug) {
    printf("  eval1: % 21.10f\n", ev[0]);
    printf("  eval2: % 21.10f\n", ev[1]);
    printf("  eval3: % 21.10f\n", ev[2]);
  }
  matrixC(v1, v2, v3, m3);
  ELL_3M_MUL(m4, m2, m3);
  ELL_3M_MUL(mat, m1, m4);
  if (debug) {
    printf("tensor:  % 21.10f % 21.10f % 21.10f\n", mat[0], mat[1], mat[2]);
    printf("         % 21.10f % 21.10f % 21.10f\n", mat[3], mat[4], mat[5]);
    printf("         % 21.10f % 21.10f % 21.10f\n", mat[6], mat[7], mat[8]);
  }
  return;
}

#define INFO "Generates a synthetic (and implausible) diffusion tensor " \
	      "volume"

int
main(int argc, char **argv) {
  char *me, *err;
  hestParm *hparm;
  hestOpt *hopt = NULL;
  airArray *mop;

  char *outS, buff[AIR_STRLEN_SMALL];
  int size[3], xi, yi, zi;
  float parm, level, x, y, z, *data;
  double mat[9];
  Nrrd *nout;

  mop = airMopNew();
  me = argv[0];
  hparm = hestParmNew();
  airMopAdd(mop, hparm, (airMopper)hestParmFree, airMopAlways);
  hestOptAdd(&hopt, "p", "aniso parm", airTypeFloat, 1, 1, &parm, NULL,
	     "anisotropy parameter.  0.0 for linear along lines of constant "
	     "longitude (from pole to pole), 1.0 for planar, 2.0 for linear "
	     "along lines of constant latitude");
  hestOptAdd(&hopt, "ca", "aniso level", airTypeFloat, 1, 1, &level, "1.0",
	     "the non-spherical-ness of the anisotropy used.  \"1.0\" means "
	     "completely linear or completely planar anisotropy");
  hestOptAdd(&hopt, "s", "sx sy sz", airTypeInt, 3, 3, size, "32 32 32",
	     "dimensions of output volume");
  hestOptAdd(&hopt, "o", "nout", airTypeString, 1, 1, &outS, NULL,
	     "output filename");
  hestParseOrDie(hopt, argc-1, argv+1, hparm,
		 me, INFO, AIR_TRUE, AIR_TRUE, AIR_TRUE);
  airMopAdd(mop, hopt, (airMopper)hestOptFree, airMopAlways);
  airMopAdd(mop, hopt, (airMopper)hestParseFree, airMopAlways);

  if (nrrdAlloc(nout=nrrdNew(), nrrdTypeFloat, 4,
		7, size[0], size[1], size[2])) {
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble allocating output:\n%s\n", me, err);
    airMopError(mop);
    return 1;
  }
  nrrdAxesSet(nout, nrrdAxesInfoSpacing, AIR_NAN, 1.0, 1.0, 1.0);
  nrrdAxesSet(nout, nrrdAxesInfoLabel, "tensor", "x", "y", "z");
  sprintf(buff, "satin(%g,%g)", parm, level);
  nout->content = airStrdup(buff);

  data = nout->data;
  for (zi=0; zi<=size[2]-1; zi++) {
    z = AIR_AFFINE(0, zi, size[2]-1, -1.0, 1.0);
    for (yi=0; yi<=size[1]-1; yi++) {
      y = AIR_AFFINE(0, yi, size[1]-1, -1.0, 1.0);
      for (xi=0; xi<=size[0]-1; xi++) {
	x = AIR_AFFINE(0, xi, size[0]-1, -1.0, 1.0);

	satinTensor(mat, parm, x, y, z);
	data[0] = 1.0;
	data[1] = mat[0];
	data[2] = mat[1];
	data[3] = mat[2];
	data[4] = mat[4];
	data[5] = mat[5];
	data[6] = mat[8];
	data += 7;
      }
    }
  }
  
  if (nrrdSave(outS, nout, NULL)) {
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble saving output:\n%s\n", me, err);
    airMopError(mop);
    return 1;
  }

  airMopOkay(mop);
  exit(0);
}
