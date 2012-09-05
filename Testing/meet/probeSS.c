/*
  Teem: Tools to process and visualize scientific data and images              
  Copyright (C) 2012, 2011, 2010, 2009  University of Chicago
  Copyright (C) 2008, 2007, 2006, 2005  Gordon Kindlmann
  Copyright (C) 2004, 2003, 2002, 2001, 2000, 1999, 1998  University of Utah

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public License
  (LGPL) as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  The terms of redistributing and/or modifying this software also
  include exceptions to the LGPL that facilitate static linking.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with this library; if not, write to Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "teem/meet.h"

#define PROBE "probeSS"

/*
** Tests:
** 
*/

static int
genTensorVol(Nrrd *ncten, 
             unsigned int sx, unsigned int sy, unsigned int sz) {
  static const char me[]="genTensorVol";
  hestParm *hparm;
  airArray *smop;
  char tmpStr[4][AIR_STRLEN_SMALL];
  airRandMTState *rng;
  unsigned int seed;
  const char *helixArgv[] = 
  /*   0     1     2    3     4     5     6     7     8     9 */
    {"-s", NULL, NULL, NULL, "-v", "0", "-r", "55", "-o", NULL, 
     NULL};
  int helixArgc;

  smop = airMopNew();
  /* not using random number until sure that CTest stuff 
     contributing to dashboard clears out its build/test directory 
     each time  
     seed = AIR_CAST(unsigned int, 1e6*fmod(airTime(), 1.0)); */
  seed = 42;
  rng = airRandMTStateNew(seed);
  airMopAdd(smop, rng, (airMopper)airRandMTStateNix, airMopAlways);
  /* NOTE: this is currently the only place where a unrrduCmd
     is called from C; learned: hest does NOT tolerate having 
     empty or NULL elements of argv[]! */
  hparm = hestParmNew();
  airMopAdd(smop, hparm, (airMopper)hestParmFree, airMopAlways);
  sprintf(tmpStr[0], "%u", sx); helixArgv[1] = tmpStr[0];
  sprintf(tmpStr[1], "%u", sy); helixArgv[2] = tmpStr[1];
  sprintf(tmpStr[2], "%u", sz); helixArgv[3] = tmpStr[2];
  sprintf(tmpStr[3], "tmp-%010u.nrrd", airUIrandMT_r(rng));
  helixArgv[9] = tmpStr[3];
  helixArgc = AIR_CAST(int, sizeof(helixArgv)/sizeof(char *)) - 1;
  if (tend_helixCmd.main(helixArgc, helixArgv, me, hparm)) {
    /* error already went to stderr, not to any biff container */
    biffAddf(PROBE, "%s: problem running tend %s", me, tend_helixCmd.name);
    airMopError(smop); return 1;
  }
  if (nrrdLoad(ncten, tmpStr[3], NULL)) {
    biffAddf(PROBE, "%s: trouble loading from new vol %s", me, tmpStr[3]);
    airMopError(smop); return 1;
  }
  airMopOkay(smop);
  return 0;
}

static int
genDwiVol(Nrrd *ndwi, unsigned int gradNum, const Nrrd *ncten) {
  static const char me[]="genDwiVol";
  tenGradientParm *gparm;
  tenExperSpec *espec;
  Nrrd *ngrad, *nten, *nb0;
  NrrdIter *narg0, *narg1;
  size_t cropMin[4] = {1, 0, 0, 0}, cropMax[4];
  airArray *smop;

  smop = airMopNew();
  ngrad = nrrdNew();
  airMopAdd(smop, ngrad, (airMopper)nrrdNuke, airMopAlways);
  gparm = tenGradientParmNew();
  airMopAdd(smop, gparm, (airMopper)tenGradientParmNix, airMopAlways);
  espec = tenExperSpecNew();
  airMopAdd(smop, espec, (airMopper)tenExperSpecNix, airMopAlways);
  gparm->verbose = 0;
  gparm->minMean = 0.002;
  gparm->seed = 4242;
  if (tenGradientGenerate(ngrad, gradNum, gparm)
      || tenExperSpecGradSingleBValSet(espec, AIR_TRUE /* insertB0 */,
                                       1000.0 /* bval */,
                                       AIR_CAST(double *, ngrad->data),
                                       gradNum)) {
    biffMovef(PROBE, TEN, "%s: trouble generating grads or espec", me);
    airMopError(smop); return 1;
  }
  nten = nrrdNew();
  airMopAdd(smop, nten, (airMopper)nrrdNuke, airMopAlways);
  nb0 = nrrdNew();
  airMopAdd(smop, nb0, (airMopper)nrrdNuke, airMopAlways);
  ELL_4V_SET(cropMax,
             ncten->axis[0].size-1,
             ncten->axis[1].size-1,
             ncten->axis[2].size-1,
             ncten->axis[3].size-1);
  if (nrrdSlice(nb0, ncten, 0, 0)
      || nrrdCrop(nten, ncten, cropMin, cropMax)) {
    biffMovef(PROBE, NRRD, "%s: trouble slice or cropping ten vol", me);
    airMopError(smop); return 1;
  }
  narg0 = nrrdIterNew();
  narg1 = nrrdIterNew();
  airMopAdd(smop, narg0, (airMopper)nrrdIterNix, airMopAlways);
  airMopAdd(smop, narg1, (airMopper)nrrdIterNix, airMopAlways);
  nrrdIterSetValue(narg1, 50000.0);
  nrrdIterSetNrrd(narg0, nb0);
  if (nrrdArithIterBinaryOp(nb0, nrrdBinaryOpMultiply,
                            narg0, narg1)) {
    biffMovef(PROBE, NRRD, "%s: trouble generating b0 vol", me);
    airMopError(smop); return 1;
  }
  if (tenModelSimulate(ndwi, nrrdTypeUShort, espec,
                       tenModel1Tensor2, 
                       nb0, nten, AIR_TRUE /* keyValueSet */)) {
    biffMovef(PROBE, TEN, "%s: trouble simulating DWI vol", me);
    airMopError(smop); return 1;
  }

  airMopOkay(smop);
  return 0;
}

#define KIND_NUM 4

int
main(int argc, const char **argv) {
  const char *me;
  char *err = NULL;

  airArray *mop;
  const gageKind *kind[KIND_NUM] = {
    /*    0            1           2         3          */
    gageKindScl, gageKindVec, tenGageKind, NULL /* dwi */};
  Nrrd *nin[KIND_NUM];
  unsigned int kindIdx;

  AIR_UNUSED(argc);
  me = argv[0];
  mop = airMopNew();
  
  for (kindIdx=0; kindIdx<KIND_NUM; kindIdx++) {
    if (kind[kindIdx]) {
      if (kind[kindIdx] != meetConstGageKindParse(kind[kindIdx]->name)) {
        fprintf(stderr, "%s: kind[%u]->name %s wasn't parsed\n", me,
                kindIdx, kind[kindIdx]->name);
        airMopError(mop); return 1;
      }
    }
  }
  for (kindIdx=0; kindIdx<KIND_NUM; kindIdx++) {
    nin[kindIdx] = nrrdNew();
    airMopAdd(mop, nin[kindIdx], (airMopper)nrrdNuke, airMopAlways);
  }
  /* start by making tensor volume */
  if (genTensorVol(nin[2], 45, 46, 47)) {
    airMopAdd(mop, err = biffGetDone(PROBE), airFree, airMopAlways);
    fprintf(stderr, "trouble creating tensor volume:\n%s", err);
    airMopError(mop); return 1;
  }
  /* nrrdSave("tmp-cten.nrrd", nin[2], NULL); */
  if (genDwiVol(nin[3], 35, nin[2])) {
    airMopAdd(mop, err = biffGetDone(PROBE), airFree, airMopAlways);
    fprintf(stderr, "trouble creating DWI volume:\n%s", err);
    airMopError(mop); return 1;
  }
  /* nrrdSave("tmp-dwi.nrrd", nin[3], NULL); */
  
  /* this is a work in progress ... */
  
  /* gageContextCopy on stack! */
  /* create scalar/vector/tensor volume */
  /* create scale-space stacks with tent, ctmr, and hermite */
  /* save them, free, and read them back in */
  /* pick a scale in-between tau samples */
  /* for all the tau's half-way between tau samples in scale:
       blur at that tau to get correct values 
       check that error with hermite is lower than ctmr is lower than tent */
  /* for all tau samples:
       blur at that tau to (re-)get correct values 
       check that everything agrees there */

  /* single probe with high verbosity */

  airMopOkay(mop);
  return 0;
}
