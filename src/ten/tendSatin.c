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

#include "ten.h"
#include "tenPrivate.h"

#define INFO "Generate a pretty synthetic DT volume"
char *_tend_satinInfoL =
  (INFO
   ".  The surface of a sphere is covered with either linear or "
   "planar anisotropic tensors, or somewhere in between.");

void
tend_satinEigen(float *eval, float *evec, float x, float y, float z,
		float parm, float level) {
  double bound, bound1, bound2, r, norm, tmp[3], meval;
  double tmp1[3];

  r = sqrt(x*x + y*y + z*z);
  bound1 = 0.5 - 0.5*airErf(20*(r-0.9));  /* 1 on inside, 0 on outside */
  bound2 = 0.5 - 0.5*airErf(20*(0.7-r));
  bound = AIR_MIN(bound1, bound2);        /* and 0 on the very inside too */

#define BLAH(bound, tmp) AIR_AFFINE(0.0, bound, 1.0, 1.0/3.0, tmp)
  
  eval[0] = 5*BLAH(bound, AIR_AFFINE(0.0, parm, 2.0, 1.0, 0.0001));
  eval[1] = 5*BLAH(bound, AIR_AFFINE(0.0, parm, 2.0, 0.0001, 1.0));
  eval[2] = 5*BLAH(bound, 0.0001);
  meval = (eval[0] + eval[1] + eval[2])/3;
  eval[0] = AIR_AFFINE(0.0, level, 1.0, meval, eval[0]);
  eval[1] = AIR_AFFINE(0.0, level, 1.0, meval, eval[1]);
  eval[2] = AIR_AFFINE(0.0, level, 1.0, meval, eval[2]);

  /* v1: looking down positive Z, points counter clockwise */
  if (x || y) {
    ELL_3V_SET(evec + 3*0, y, -x, 0);
    ELL_3V_NORM(evec + 3*0, evec + 3*0, norm);

    /* v2: points towards pole at positive Z */
    ELL_3V_SET(tmp, -x, -y, -z);
    ELL_3V_NORM(tmp, tmp, norm);
    ELL_3V_CROSS(evec + 3*1, tmp, evec + 3*0);
    ELL_3V_CROSS(evec + 3*2, evec + 3*0, evec + 3*1);
  } else {
    /* not optimal, but at least it won't show up in glyph visualizations */
    ELL_3M_IDENTITY_SET(evec);
  }
  ELL_3V_CROSS(tmp1, evec + 3*0, evec + 3*1);
  ELL_3V_CROSS(tmp1, evec + 3*0, evec + 3*2);
  ELL_3V_CROSS(tmp1, evec + 3*1, evec + 3*2);
  return;
}

int
tend_satinGen(Nrrd *nout, float parm, float level, int size[3]) {
  char me[]="tend_satinGen", err[AIR_STRLEN_MED], buff[AIR_STRLEN_SMALL];
  Nrrd *nconf, *neval, *nevec;
  float *conf, *eval, *evec;
  int xi, yi, zi;
  float x, y, z;

  if (nrrdMaybeAlloc(nconf=nrrdNew(), nrrdTypeFloat, 3,
		     size[0], size[1], size[2]) ||
      nrrdMaybeAlloc(neval=nrrdNew(), nrrdTypeFloat, 4,
		     3, size[0], size[1], size[2]) ||
      nrrdMaybeAlloc(nevec=nrrdNew(), nrrdTypeFloat, 4,
		     9, size[0], size[1], size[2])) {
    sprintf(err, "%s: trouble allocating temp nrrds", me);
    biffMove(TEN, err, NRRD); return 1;
  }

  conf = nconf->data;
  eval = neval->data;
  evec = nevec->data;
  for (zi=0; zi<size[2]; zi++) {
    z = AIR_AFFINE(0, zi, size[2]-1, -1.0, 1.0);
    for (yi=0; yi<size[1]; yi++) {
      y = AIR_AFFINE(0, yi, size[1]-1, -1.0, 1.0);
      for (xi=0; xi<size[0]; xi++) {
	x = AIR_AFFINE(0, xi, size[0]-1, -1.0, 1.0);
	*conf = 1.0;
	tend_satinEigen(eval, evec, x, y, z, parm, level);
	conf += 1;
	eval += 3;
	evec += 9;
      }
    }
  }

  if (tenTensorMake(nout, nconf, neval, nevec)) {
    sprintf(err, "%s: trouble generating output", me);
    biffAdd(TEN, err); return 1;
  }

  nrrdNuke(nconf);
  nrrdNuke(neval);
  nrrdNuke(nevec);
  nrrdAxesSet(nout, nrrdAxesInfoSpacing, AIR_NAN, 1.0, 1.0, 1.0);
  nrrdAxesSet(nout, nrrdAxesInfoLabel, "tensor", "x", "y", "z");
  sprintf(buff, "satin(%g,%g)", parm, level);
  nout->content = airStrdup(buff);
  return 0;
}

int
tend_satinMain(int argc, char **argv, char *me, hestParm *hparm) {
  int pret;
  hestOpt *hopt = NULL;
  char *perr, *err;
  airArray *mop;

  int size[3];
  float parm, level;
  Nrrd *nout;
  char *outS;

  hestOptAdd(&hopt, "p", "aniso parm", airTypeFloat, 1, 1, &parm, NULL,
	     "anisotropy parameter.  0.0 for linear along lines of constant "
	     "longitude (from pole to pole), 1.0 for planar, 2.0 for linear "
	     "along lines of constant latitude");
  hestOptAdd(&hopt, "ca", "aniso level", airTypeFloat, 1, 1, &level, "1.0",
	     "the non-spherical-ness of the anisotropy used.  \"1.0\" means "
	     "completely linear or completely planar anisotropy");
  hestOptAdd(&hopt, "s", "sx sy sz", airTypeInt, 3, 3, size, "32 32 32",
	     "dimensions of output volume");
  hestOptAdd(&hopt, "o", "nout", airTypeString, 1, 1, &outS, "-",
	     "output filename");

  mop = airMopNew();
  airMopAdd(mop, hopt, (airMopper)hestOptFree, airMopAlways);
  USAGE(_tend_satinInfoL);
  PARSE();
  airMopAdd(mop, hopt, (airMopper)hestParseFree, airMopAlways);

  nout = nrrdNew();
  airMopAdd(mop, nout, (airMopper)nrrdNuke, airMopAlways);

  if (tend_satinGen(nout, parm, level, size)) {
    airMopAdd(mop, err=biffGetDone(TEN), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble making volume:\n%s\n", me, err);
    airMopError(mop); return 1;
  }

  if (nrrdSave(outS, nout, NULL)) {
    airMopAdd(mop, err=biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble writing:\n%s\n", me, err);
    airMopError(mop); return 1;
  }

  airMopOkay(mop);
  return 0;
}
/* TEND_CMD(satin, INFO); */
unrrduCmd tend_satinCmd = { "satin", INFO, tend_satinMain };
