/*
  Teem: Tools to process and visualize scientific data and images
  Copyright (C) 2009--2023  University of Chicago
  Copyright (C) 2005--2008  Gordon Kindlmann
  Copyright (C) 1998--2004  University of Utah

  This library is free software; you can redistribute it and/or modify it under the terms
  of the GNU Lesser General Public License (LGPL) as published by the Free Software
  Foundation; either version 2.1 of the License, or (at your option) any later version.
  The terms of redistributing and/or modifying this software also include exceptions to
  the LGPL that facilitate static linking.

  This library is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
  PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License along with
  this library; if not, write to Free Software Foundation, Inc., 51 Franklin Street,
  Fifth Floor, Boston, MA 02110-1301 USA
*/

#include "ten.h"
#include "privateTen.h"

#define INFO "Calculate one or more eigenvectors in a DT volume"
static const char *_tend_evecInfoL = (INFO ". ");

static int
tend_evecMain(int argc, const char **argv, const char *me, hestParm *hparm) {
  int pret;
  hestOpt *hopt = NULL;
  char *perr, *err;
  airArray *mop;

  int ret, *comp;
  unsigned int cc, compLen;
  Nrrd *nin, *nout;
  char *outS;
  float thresh, *edata, *tdata, eval[3], evec[9], scl;
  size_t N, I, sx, sy, sz;

  hestOptAdd_Nv_Int(&hopt, "c", "c0 ", 1, 3, &comp, NULL,
                    "which eigenvalues should be saved out. \"0\" for the "
                    "largest, \"1\" for the middle, \"2\" for the smallest, "
                    "\"0 1\", \"1 2\", \"0 1 2\" or similar for more than one",
                    &compLen);
  hestOptAdd_1_Float(&hopt, "t", "thresh", &thresh, "0.5", "confidence threshold");
  hestOptAdd_1_Other(&hopt, "i", "nin", &nin, "-", "input diffusion tensor volume",
                     nrrdHestNrrd);
  hestOptAdd_1_String(&hopt, "o", "nout", &outS, "-", "output image (floating point)");

  mop = airMopNew();
  airMopAdd(mop, hopt, (airMopper)hestOptFree, airMopAlways);
  USAGE_PARSE(_tend_evecInfoL);
  airMopAdd(mop, hopt, (airMopper)hestParseFree, airMopAlways);

  for (cc = 0; cc < compLen; cc++) {
    if (!AIR_IN_CL(0, comp[cc], 2)) {
      fprintf(stderr, "%s: requested component %d (%d of 3) not in [0..2]\n", me,
              comp[cc], cc + 1);
      airMopError(mop);
      return 1;
    }
  }
  if (tenTensorCheck(nin, nrrdTypeFloat, AIR_TRUE, AIR_TRUE)) {
    airMopAdd(mop, err = biffGetDone(TEN), airFree, airMopAlways);
    fprintf(stderr, "%s: didn't get a valid DT volume:\n%s\n", me, err);
    airMopError(mop);
    return 1;
  }

  sx = nin->axis[1].size;
  sy = nin->axis[2].size;
  sz = nin->axis[3].size;

  nout = nrrdNew();
  airMopAdd(mop, nout, (airMopper)nrrdNuke, airMopAlways);
  ret = nrrdMaybeAlloc_va(nout, nrrdTypeFloat, 4, AIR_SIZE_T(3 * compLen), sx, sy, sz);
  if (ret) {
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble allocating output:\n%s\n", me, err);
    airMopError(mop);
    return 1;
  }

  N = sx * sy * sz;
  edata = (float *)nout->data;
  tdata = (float *)nin->data;
  if (1 == compLen) {
    for (I = 0; I < N; I++) {
      tenEigensolve_f(eval, evec, tdata);
      scl = AIR_FLOAT(tdata[0] >= thresh);
      ELL_3V_SCALE(edata, scl, evec + 3 * comp[0]);
      edata += 3;
      tdata += 7;
    }
  } else {
    for (I = 0; I < N; I++) {
      tenEigensolve_f(eval, evec, tdata);
      scl = AIR_FLOAT(tdata[0] >= thresh);
      for (cc = 0; cc < compLen; cc++) {
        ELL_3V_SCALE(edata + 3 * cc, scl, evec + 3 * comp[cc]);
      }
      edata += 3 * compLen;
      tdata += 7;
    }
  }
  if (nrrdAxisInfoCopy(nout, nin, NULL, NRRD_AXIS_INFO_SIZE_BIT)) {
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble:\n%s\n", me, err);
    airMopError(mop);
    return 1;
  }
  if (nrrdBasicInfoCopy(nout, nin, NRRD_BASIC_INFO_ALL ^ NRRD_BASIC_INFO_SPACE)) {
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble:\n%s\n", me, err);
    airMopError(mop);
    return 1;
  }
  nout->axis[0].label = (char *)airFree(nout->axis[0].label);
  nout->axis[0].kind = nrrdKindUnknown;

  if (nrrdSave(outS, nout, NULL)) {
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble writing:\n%s\n", me, err);
    airMopError(mop);
    return 1;
  }

  airMopOkay(mop);
  return 0;
}
TEND_CMD(evec, INFO);
