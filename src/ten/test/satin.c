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
int i, sx, sy, sz, xi, yi, zi;
float parm;

double v1[3], v2[3], v3[3], m1[9], m2[9], m3[9], m4[9], t[9];

/************************************************************************/
/************************************************************************/
/************************************************************************/

char desc[1024];

double
eval(int i, double x, double y, double z) {
  double bound, bound1, bound2, ret, r, f;

  r = sqrt(x*x + y*y + z*z);
  
  bound1 = 0.5 - 0.5*erf(20*(r-0.8));  /* 1 on inside, 0 on outside */
  bound2 = 0.5 - 0.5*erf(20*(0.6-r));
  bound = AIR_MIN(bound1, bound2);     /* and 0 on the very inside too */
  
  /*   hack = AIR_AFFINE(-1, z, 1, 0, 1); */
  switch (i) {
  case 1:
    f = AIR_AFFINE(0.0, parm, 2.0, 1.0, 0.0001);
    /* f = AIR_LERP(hack, f, 0.5); */
    break;
  case 2:
    f = AIR_AFFINE(0.0, parm, 2.0, 0.0001, 1.0);
    /* f = AIR_LERP(hack, f, 0.5); */
    break;
  case 3:
    f = 0.0001;
    break;
  }
  ret = AIR_AFFINE(1.0, bound, 0.0, f, 1.0/3.0);
  return 5*ret;
}

void
evec1(double x, double y, double z) {
  double norm;

  /* looking towards positive Y, v1 points counter clockwise */
  v1[0] = -z;
  v1[1] = 0;
  v1[2] = x;
  ELL_3V_NORM(v1, v1, norm);
}

void
evec2(double x, double y, double z) {
  double norm, tmp[3];

  /* v2 points towards pole at positive Y */
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
tensor(double x, double y, double z, double *t) {
  double norm;
  
  evec1(x, y, z);    /* sets v1 */
  evec2(x, y, z);    /* sets v2 */
  ELL_3V_CROSS(v3, v1, v2);
  ELL_3V_NORM(v3, v3, norm);
  if (debug) {
    printf("xi = %d; yi = %d; zi = %d\n", xi, yi, zi);
    printf("x = %g, y = %g, z = %g\n", x, y, z);
    printf("evects: v1: % 21.10f % 21.10f % 21.10f\n", v1[0], v1[1], v1[2]);
    printf("        v2: % 21.10f % 21.10f % 21.10f\n", v2[0], v2[1], v2[2]);
    printf("        v3: % 21.10f % 21.10f % 21.10f\n", v3[0], v3[1], v3[2]);
  }
  matrixR(v1, v2, v3, m1);
  diagonal(eval(1, x, y, z),
	   eval(2, x, y, z),
	   eval(3, x, y, z), m2);
  if (debug) {
    printf("  eval1: % 21.10f\n", eval(1, x, y, z));
    printf("  eval2: % 21.10f\n", eval(2, x, y, z));
    printf("  eval3: % 21.10f\n", eval(3, x, y, z));
  }
  matrixC(v1, v2, v3, m3);
  ELL_3M_MUL(m4, m2, m3);
  ELL_3M_MUL(t, m1, m4);
  if (debug) {
    printf("tensor:  % 21.10f % 21.10f % 21.10f\n", t[0], t[1], t[2]);
    printf("         % 21.10f % 21.10f % 21.10f\n", t[3], t[4], t[5]);
    printf("         % 21.10f % 21.10f % 21.10f\n", t[6], t[7], t[8]);
  }
}

void
usage(char *me) {
  /*                      0    1    2    3    4       5 */
  fprintf(stderr, "usage: %s <sx> <sy> <sz> <parm> <outputname>\n", me);
  exit(1);
}
  /* parm: 
     0 for linear along v1, 
     1 for planar, 
     2 for linear along v2 */

int
main(int argc, char **argv) {
  char *me;
  hestParm *hparm;
  hestOpt *hopt = NULL;
  airArray *mop;

  char *outS;
  int size[3];
  float parm;

  mop = airMopNew();
  me = argv[0];
  hparm = hestParmNew();
  airMopAdd(mop, hparm, (airMopper)hestParmFree, airMopAlways);
  hestOptAdd(&hopt, "p", "aniso parm", airTypeFloat, 1, 1, &parm, NULL,
	     "anisotropy parameter.  0.0 for linear along lines of constant "
	     "longitude (from pole to pole), 1.0 for planar, 2.0 for linear "
	     "along lines of constant latitude");
  hestOptAdd(&hopt, "s", "sx sy sz", airTypeInt, 3, 3, size, "64 64 64",
	     "dimensions of output volume");
  hestOptAdd(&hopt, "o", "nout", airTypeString, 1, 1, &outS, NULL,
	     "output filename");

  char *me, *out;
  float x, y, z, *data;
  Nrrd *nrrd;

  me = argv[0];
  if (!(6 == argc)) {
    usage(me);
  }
  if (1 != sscanf(argv[4], "%f", &parm)) {
    fprintf(stderr, "%s: couldn't parse %s as float\n", me, argv[4]);
    exit(1);
  }
  out = argv[5];
  if (1 != sscanf(argv[1], "%d", &sx) ||
      1 != sscanf(argv[2], "%d", &sy) ||
      1 != sscanf(argv[3], "%d", &sz)) {
    fprintf(stderr, "%s: couldn't parse \"%s\", \"%s\", \"%s\" as int sizes\n",
	    me, argv[1], argv[2], argv[3]);
    usage(me);
  }

  if (nrrdAlloc(nrrd=nrrdNew(), sx*sy*sz*7, nrrdTypeFloat, 4)) {
    fprintf(stderr, "%s: can't allocate nrrd\n", me);
    exit(1);
  }
  nrrd->axis[0].size = 7;
  nrrd->axis[1].size = sx;
  nrrd->axis[2].size = sy;
  nrrd->axis[3].size = sz;
  nrrd->axis[0].spacing = 1.0;
  nrrd->axis[1].spacing = 1.0;
  nrrd->axis[2].spacing = 1.0;
  nrrd->axis[3].spacing = 1.0;
  sprintf(desc, "satin %g", parm);
  nrrd->content = airStrdup(desc);
  data = nrrd->data;

  for (zi=0; zi<=sz-1; zi++) {
    z = AIR_AFFINE(0, zi, sz-1, -1.0, 1.0);
    for (yi=0; yi<=sy-1; yi++) {
      y = AIR_AFFINE(0, yi, sy-1, -1.0, 1.0);
      for (xi=0; xi<=sx-1; xi++) {
	x = AIR_AFFINE(0, xi, sx-1, -1.0, 1.0);

	tensor(x, y, z, t);
	data[0 + 7*(xi + sx*(yi + sy*zi))] = 1.0;
	data[1 + 7*(xi + sx*(yi + sy*zi))] = t[0];
	data[2 + 7*(xi + sx*(yi + sy*zi))] = t[1];
	data[3 + 7*(xi + sx*(yi + sy*zi))] = t[2];
	data[4 + 7*(xi + sx*(yi + sy*zi))] = t[4];
	data[5 + 7*(xi + sx*(yi + sy*zi))] = t[5];
	data[6 + 7*(xi + sx*(yi + sy*zi))] = t[8];
      }
    }
  }
  
  nrrd->axis[0].label = airStrdup("tens");
  nrrd->axis[1].label = airStrdup("x");
  nrrd->axis[2].label = airStrdup("y");
  nrrd->axis[3].label = airStrdup("z");
  if (nrrdSave(out, nrrd, NULL)) {
    fprintf(stderr, "%s: error writing nrrd:\n%s", me, biffGet(NRRD));
    exit(1);
  }

  exit(0);
}
