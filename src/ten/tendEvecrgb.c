/*
  teem: Gordon Kindlmann's research software
  Copyright (C) 2003, 2002, 2001, 2000, 1999, 1998 University of Utah

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

#define INFO "Make an RGB volume from an eigenvector and an anisotropy"
char *_tend_evecrgbInfoL =
  (INFO
   ". ");

int
tend_evecrgbMain(int argc, char **argv, char *me, hestParm *hparm) {
  int pret;
  hestOpt *hopt = NULL;
  char *perr, *err;
  airArray *mop;

  int aniso, ret, cc, sx, sy, sz;
  Nrrd *nin, *nout;
  char *outS;
  float *cdata, *tdata, eval[3], evec[9], 
    bg, gray, gamma, R, G, B, an[TEN_ANISO_MAX], conf;
  size_t N, I;

  hestOptAdd(&hopt, "c", "evec index", airTypeInt, 1, 1, &cc, NULL,
	     "which eigenvector will be colored. \"0\" for the "
	     "principal, \"1\" for the middle, \"2\" for the minor");
  hestOptAdd(&hopt, "a", "aniso", airTypeEnum, 1, 1, &aniso, NULL,
	     "Which anisotropy to use for modulating the saturation "
	     "of the colors.  " TEN_ANISO_DESC,
	     NULL, tenAniso);
  hestOptAdd(&hopt, "bg", "background", airTypeFloat, 1, 1, &bg, "0",
	     "gray level to use for voxels who's confidence is zero ");
  hestOptAdd(&hopt, "gr", "gray", airTypeFloat, 1, 1, &gray, "0",
	     "the gray level to desaturate towards as anisotropy "
	     "decreases (while confidence remains 1.0)");
  hestOptAdd(&hopt, "gam", "gamma", airTypeFloat, 1, 1, &gamma, "1",
	     "gamma to use on color components");
  hestOptAdd(&hopt, "i", "nin", airTypeOther, 1, 1, &nin, "-",
	     "input diffusion tensor volume", NULL, NULL, nrrdHestNrrd);
  hestOptAdd(&hopt, "o", "nout", airTypeString, 1, 1, &outS, "-",
	     "output image (floating point)");

  mop = airMopNew();
  airMopAdd(mop, hopt, (airMopper)hestOptFree, airMopAlways);
  USAGE(_tend_evecrgbInfoL);
  PARSE();
  airMopAdd(mop, hopt, (airMopper)hestParseFree, airMopAlways);

  if (!AIR_IN_CL(0, cc, 2)) {
    fprintf(stderr, "%s: requested component %d not in [0..2]\n", me, cc);
    airMopError(mop); return 1;
  }
  if (tenTensorCheck(nin, nrrdTypeFloat, AIR_TRUE, AIR_TRUE)) {
    airMopAdd(mop, err=biffGetDone(TEN), airFree, airMopAlways);
    fprintf(stderr, "%s: didn't get a valid DT volume:\n%s\n", me, err);
    airMopError(mop); return 1;
  }
  
  sx = nin->axis[1].size;
  sy = nin->axis[2].size;
  sz = nin->axis[3].size;

  nout = nrrdNew();
  airMopAdd(mop, nout, (airMopper)nrrdNuke, airMopAlways);
  ret = nrrdMaybeAlloc(nout, nrrdTypeFloat, 4, 3, sx, sy, sz);
  if (ret) {
    airMopAdd(mop, err=biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble allocating output:\n%s\n", me, err);
    airMopError(mop); return 1;
  }

  N = sx*sy*sz;
  cdata = nout->data;
  tdata = nin->data;
  for (I=0; I<N; I++) {
    tenEigensolve(eval, evec, tdata);
    tenAnisoCalc(an, eval);
    R = AIR_ABS(evec[0 + 3*cc]);
    G = AIR_ABS(evec[1 + 3*cc]);
    B = AIR_ABS(evec[2 + 3*cc]);
    R = pow(R, 1.0/gamma);
    G = pow(G, 1.0/gamma);
    B = pow(B, 1.0/gamma);
    R = AIR_AFFINE(0.0, an[aniso], 1.0, gray, R);
    G = AIR_AFFINE(0.0, an[aniso], 1.0, gray, G);
    B = AIR_AFFINE(0.0, an[aniso], 1.0, gray, B);
    conf = AIR_CLAMP(0, tdata[0], 1);
    R = AIR_AFFINE(0.0, conf, 1.0, bg, R);
    G = AIR_AFFINE(0.0, conf, 1.0, bg, G);
    B = AIR_AFFINE(0.0, conf, 1.0, bg, B);
    ELL_3V_SET(cdata, R, G, B);
    cdata += 3;
    tdata += 7;
  }
  if (nrrdAxisInfoCopy(nout, nin, NULL, NRRD_AXIS_INFO_SIZE_BIT)) {
    airMopAdd(mop, err=biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble:\n%s\n", me, err);
    airMopError(mop); return 1;
  }
  AIR_FREE(nout->axis[0].label);
  nout->axis[0].label = airStrdup("rgb");

  if (nrrdSave(outS, nout, NULL)) {
    airMopAdd(mop, err=biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble writing:\n%s\n", me, err);
    airMopError(mop); return 1;
  }

  airMopOkay(mop);
  return 0;
}
TEND_CMD(evecrgb, INFO);
