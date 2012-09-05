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
** ... lots of gage stuff ...
**
** The main point of all this is to make sure that values and their
** their derivatives (where the gageKind supports it) are correctly
** handled in the multi-value gageKinds (gageKindVec, tenGageKind,
** tenDwiGageKind), relative to the gageKindScl ground-truth
*/

static int
genTensorVol(Nrrd *ncten, double noiseStdv,
             unsigned int sx, unsigned int sy, unsigned int sz) {
  static const char me[]="genTensorVol";
  hestParm *hparm;
  airArray *smop;
  char tmpStr[4][AIR_STRLEN_SMALL];
  airRandMTState *rng;
  unsigned int seed;
  Nrrd *nclean;
  NrrdIter *narg0, *narg1;
  const char *helixArgv[] = 
  /*   0     1     2    3     4     5     6     7     8     9 */
    {"-s", NULL, NULL, NULL, "-v", "0", "-r", "35", "-o", NULL, 
     "-ev", "0.00086", "0.00043", "0.00021", "-bg", "0.003", "-b", "5",
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
     is called from within C code; it was educational to get working.
     Learned: 
     * hest does NOT tolerate having empty or NULL elements of 
     its argv[]!  More error checking for this in hest is needed.
     * the "const char **argv" type is not very convenient to
     set up in a dynamic way; the per-element setting done below
     is certainly awkward 
  */
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
  nclean = nrrdNew();
  airMopAdd(smop, nclean, (airMopper)nrrdNuke, airMopAlways);
  if (nrrdLoad(nclean, tmpStr[3], NULL)) {
    biffAddf(PROBE, "%s: trouble loading from new vol %s", me, tmpStr[3]);
    airMopError(smop); return 1;
  }
  /* add some noise to tensor value; no, this isn't really physical;
     since we're adding noise to the tensor and then simulating DWIs,
     rather than adding noise to DWIs and then estimating tensor,
     but for the purposes of gage testing its fine */
  narg0 = nrrdIterNew();
  narg1 = nrrdIterNew();
  airMopAdd(smop, narg0, (airMopper)nrrdIterNix, airMopAlways);
  airMopAdd(smop, narg1, (airMopper)nrrdIterNix, airMopAlways);
  nrrdIterSetNrrd(narg0, nclean);
  nrrdIterSetValue(narg1, noiseStdv);
  if (nrrdArithIterBinaryOp(ncten, nrrdBinaryOpNormalRandScaleAdd,
                            narg0, narg1)) {
    biffMovef(PROBE, NRRD, "%s: trouble noisying output", me);
    airMopError(smop); return 1;
  }
  airMopOkay(smop);
  return 0;
}

static int
genScalarVol(Nrrd *nscl, const Nrrd *ncten) {
  static const char me[]="genScalarVol";

  if (tenAnisoVolume(nscl, ncten, tenAniso_Tr, 0)) {
    biffMovef(PROBE, TEN, "%s: trouble creating scalar volume", me);
    return 1;
  }
  return 0;
}

/* Makes a vector volume by measuring the gradient
** Being the gradient of the given scalar volume is just to make
** something vaguely interesting, but it is not the primary goal
** of the gage testing here. What is important is maintaining all
** the image orientation, since we'll be measuring the derivatives
** of the scalars, vectors, etc, and need them to match up
*/
static int
genVectorVol(Nrrd *nvec, const Nrrd *nscl) {
  static const char me[]="genVectorVol";
  ptrdiff_t padMin[4] = {0, 0, 0, 0}, padMax[4];
  Nrrd *ntmp;
  airArray *smop;
  float *vec, *scl;
  size_t sx, sy, sz, xi, yi, zi, Px, Mx, Py, My, Pz, Mz;
  double spcX, spcY, spcZ;
  
  if (nrrdTypeFloat != nscl->type) {
    biffAddf(PROBE, "%s: expected %s not %s type", me,
             airEnumStr(nrrdType, nrrdTypeFloat),
             airEnumStr(nrrdType, nscl->type));
    airMopError(smop); return 1;
  }
  smop = airMopNew();
  ntmp = nrrdNew();
  airMopAdd(smop, ntmp, (airMopper)nrrdNuke, airMopAlways);
  sx = nscl->axis[0].size;
  sy = nscl->axis[1].size;
  sz = nscl->axis[2].size;
  ELL_4V_SET(padMax, 2,
             AIR_CAST(ptrdiff_t, sx-1),
             AIR_CAST(ptrdiff_t, sy-1),
             AIR_CAST(ptrdiff_t, sz-1));
  /* we do axinsert and pad in order to keep all the per-axis info */
  if (nrrdAxesInsert(ntmp, nscl, 0)
      || nrrdPad_nva(nvec, ntmp, padMin, padMax, 
                     nrrdBoundaryPad, 0.0)) {
    biffMovef(PROBE, NRRD, "%s: trouble", me);
    airMopError(smop); return 1;
  }
  spcX = nrrdSpaceVecNorm(nscl->spaceDim, nscl->axis[0].spaceDirection);
  spcY = nrrdSpaceVecNorm(nscl->spaceDim, nscl->axis[1].spaceDirection);
  spcZ = nrrdSpaceVecNorm(nscl->spaceDim, nscl->axis[2].spaceDirection);
  vec = AIR_CAST(float *, nvec->data);
  scl = AIR_CAST(float *, nscl->data);
#define INDEX(xj, yj, zj) (xj + sx*(yj + sy*zj))
  for (zi=0; zi<sz; zi++) {
    Mz = (zi == 0 ? 0 : zi-1);
    Pz = AIR_MIN(zi+1, sz-1);
    for (yi=0; yi<sy; yi++) {
      My = (yi == 0 ? 0 : yi-1);
      Py = AIR_MIN(yi+1, sy-1);
      for (xi=0; xi<sx; xi++) {
        Mx = (xi == 0 ? 0 : xi-1);
        Px = AIR_MIN(xi+1, sx-1);
        vec[0 + 3*INDEX(xi, yi, zi)] = 
          (scl[INDEX(Px, yi, zi)] - scl[INDEX(Mx, yi, zi)])/(2.0*spcX);
        vec[1 + 3*INDEX(xi, yi, zi)] = 
          (scl[INDEX(xi, Py, zi)] - scl[INDEX(xi, My, zi)])/(2.0*spcY);
        vec[2 + 3*INDEX(xi, yi, zi)] = 
          (scl[INDEX(xi, yi, Pz)] - scl[INDEX(xi, yi, Mz)])/(2.0*spcZ);
      }
    }
  }
#undef INDEX

  airMopError(smop);
  return 0;
}

/*
** make a DWI volume by simulating DWIs from given tensor
** this includes generating a new gradient set 
*/
static int
genDwiVol(Nrrd *ndwi, Nrrd *ngrad, 
          unsigned int gradNum, double bval, const Nrrd *ncten) {
  static const char me[]="genDwiVol";
  tenGradientParm *gparm;
  tenExperSpec *espec;
  Nrrd *nten, *nb0;
  NrrdIter *narg0, *narg1;
  size_t cropMin[4] = {1, 0, 0, 0}, cropMax[4];
  airArray *smop;

  smop = airMopNew();
  gparm = tenGradientParmNew();
  airMopAdd(smop, gparm, (airMopper)tenGradientParmNix, airMopAlways);
  espec = tenExperSpecNew();
  airMopAdd(smop, espec, (airMopper)tenExperSpecNix, airMopAlways);
  gparm->verbose = 0;
  gparm->minMean = 0.002;
  gparm->seed = 4242;
  gparm->insertZeroVec = AIR_TRUE;
  if (tenGradientGenerate(ngrad, gradNum, gparm)
      || tenExperSpecGradSingleBValSet(espec, AIR_FALSE /* insertB0 */,
                                       bval, AIR_CAST(double *, ngrad->data),
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

int
engageMopDiceVector(gageContext *gctx, Nrrd *nvecComp[3],
                    airArray *mop, const Nrrd* nvec) {
  static const char me[]="engageMopDiceVector";
  gagePerVolume *pvl;
  unsigned int ci;
  char stmp[AIR_STRLEN_SMALL];

  if (!( 4 == nvec->dim && 3 == nvec->axis[0].size )) {
    biffAddf(PROBE, "%s: expected 4-D 3-by-X nrrd (not %u-D %s-by-X)",
             me, nvec->dim, airSprintSize_t(stmp, nvec->axis[0].size));
    return 1;
  }
  for (ci=0; ci<3; ci++) {
    nvecComp[ci] = nrrdNew();
    airMopAdd(mop, nvecComp[ci], (airMopper)nrrdNuke, airMopAlways);
    if (nrrdSlice(nvecComp[ci], nvec, 0, ci)) {
      biffMovef(PROBE, NRRD, "%s: trouble getting component %u", me, ci);
      return 1;
    }
    if ( !(pvl = gagePerVolumeNew(gctx, nvecComp[ci], gageKindScl))
         || gagePerVolumeAttach(gctx, pvl) ) {
      biffMovef(PROBE, GAGE, "%s: trouble engaging component %u", me, ci);
      return 1;
    }
  }
  
  return 0;
}

int
engageMopDiceTensor(gageContext *gctx, Nrrd *nctenComp[7],
                    airArray *mop, const Nrrd* ncten) {
  static const char me[]="engageMopDiceTensor";
  gagePerVolume *pvl;
  unsigned int ci;

  if (tenTensorCheck(ncten, nrrdTypeFloat, AIR_TRUE /* want4F */,
                     AIR_TRUE /* useBiff */)) {
    biffMovef(PROBE, TEN, "%s: didn't get tensor volume", me);
    return 1;
  }
  for (ci=0; ci<7; ci++) {
    nctenComp[ci] = nrrdNew();
    airMopAdd(mop, nctenComp[ci], (airMopper)nrrdNuke, airMopAlways);
    if (nrrdSlice(nctenComp[ci], ncten, 0, ci)) {
      biffMovef(PROBE, NRRD, "%s: trouble getting component %u", me, ci);
      return 1;
    }
    if ( !(pvl = gagePerVolumeNew(gctx, nctenComp[ci], gageKindScl))
         || gagePerVolumeAttach(gctx, pvl) ) {
      biffMovef(PROBE, GAGE, "%s: trouble engaging component %u", me, ci);
      return 1;
    }
  }
  
  return 0;
}

int
engageMopDiceDwi(gageContext *gctx, Nrrd ***ndwiCompP,
                 airArray *mop, const Nrrd* ndwi) {
  static const char me[]="mopDiceDwi";
  Nrrd **ndwiComp;
  size_t dwiNum;
  char stmp[AIR_STRLEN_SMALL];
  gagePerVolume *pvl;
  unsigned int ci;

  if (!( 4 == ndwi->dim )) {
    biffAddf(PROBE, "%s: wanted 4D volume (not %u)", me, ndwi->dim);
    return 1;
  }
  if (!( nrrdKindList == ndwi->axis[0].kind &&
         nrrdKindSpace == ndwi->axis[1].kind &&
         nrrdKindSpace == ndwi->axis[2].kind &&
         nrrdKindSpace == ndwi->axis[3].kind )) {
    biffAddf(PROBE, "%s: wanted kinds %s,3x%s, not %s,%s,%s,%s", me,
             airEnumStr(nrrdKind, nrrdKindList),
             airEnumStr(nrrdKind, nrrdKindSpace),
             airEnumStr(nrrdKind, ndwi->axis[0].kind),
             airEnumStr(nrrdKind, ndwi->axis[1].kind),
             airEnumStr(nrrdKind, ndwi->axis[2].kind),
             airEnumStr(nrrdKind, ndwi->axis[3].kind));
    return 1;
  }
  dwiNum = ndwi->axis[0].size;
  if (!(ndwiComp = AIR_CALLOC(dwiNum, Nrrd *))) {
    biffAddf(PROBE, "%s: couldn't alloc %s Nrrd*", me, 
             airSprintSize_t(stmp, dwiNum));
    return 1;
  }
  airMopAdd(mop, ndwiComp, airFree, airMopAlways);
  *ndwiCompP = ndwiComp;
  for (ci=0; ci<dwiNum; ci++) {
    ndwiComp[ci] = nrrdNew();
    airMopAdd(mop, ndwiComp[ci], (airMopper)nrrdNuke, airMopAlways);
    if (nrrdSlice(ndwiComp[ci], ndwi, 0, ci)) {
      biffMovef(PROBE, NRRD, "%s: trouble getting component %u", me, ci);
      return 1;
    }
    if ( !(pvl = gagePerVolumeNew(gctx, ndwiComp[ci], gageKindScl))
         || gagePerVolumeAttach(gctx, pvl) ) {
      biffMovef(PROBE, GAGE, "%s: trouble engaging component %u", me, ci);
      return 1;
    }
  }
  return 0;
}

#define KIND_NUM 4
#define KI_SCL 0
#define KI_VEC 1
#define KI_TEN 2
#define KI_DWI 3

int
main(int argc, const char **argv) {
  const char *me;
  char *err = NULL;

  airArray *mop;
  const gageKind *kind[KIND_NUM] = {
    /*    0            1           2         3          */
    gageKindScl, gageKindVec, tenGageKind, NULL /* dwi */};
  gageKind *dwikind = NULL;
  gageContext *gctxComp[KIND_NUM], *gctx[KIND_NUM];
  Nrrd *nin[KIND_NUM], *nvecComp[3], *nctenComp[7], **ndwiComp,
    *ngrad;  /* need access to list of gradients used to make DWIs;
                (this is not the gradient of a scalar field) */
  unsigned int kindIdx, volSize[3] = {45, 46, 47}, gradNum = 35;
  double bval = 1000, noiseStdv=0.0001;

  AIR_UNUSED(argc);
  me = argv[0];
  mop = airMopNew();
  
#define GAGE_CTX_NEW(gg, mm)                                      \
  (gg) = gageContextNew();                                        \
  airMopAdd((mm), (gg), (airMopper)gageContextNix, airMopAlways); \
  gageParmSet((gg), gageParmRenormalize, AIR_FALSE);              \
  gageParmSet((gg), gageParmCheckIntegrals, AIR_TRUE)

  for (kindIdx=0; kindIdx<KIND_NUM; kindIdx++) {
    GAGE_CTX_NEW(gctx[kindIdx], mop);
    if (KI_SCL == kindIdx) {
      /* don't need a per-component context for scalar */
      gctxComp[kindIdx] = NULL;
    } else {
      GAGE_CTX_NEW(gctxComp[kindIdx], mop);
    }
  }
#undef GAGE_CTX_NEW

  for (kindIdx=0; kindIdx<KIND_NUM; kindIdx++) {
    nin[kindIdx] = nrrdNew();
    airMopAdd(mop, nin[kindIdx], (airMopper)nrrdNuke, airMopAlways);
  }
  ngrad = nrrdNew();
  airMopAdd(mop, ngrad, (airMopper)nrrdNuke, airMopAlways);

  if (genTensorVol(nin[KI_TEN], noiseStdv,
                   volSize[0], volSize[1], volSize[2])
      || genScalarVol(nin[KI_SCL], nin[KI_TEN])
      || genVectorVol(nin[KI_VEC], nin[KI_SCL])
      || genDwiVol(nin[KI_DWI], ngrad, gradNum, bval, nin[KI_TEN])
      || engageMopDiceVector(gctxComp[KI_VEC], nvecComp, mop, nin[KI_VEC])
      || engageMopDiceTensor(gctxComp[KI_TEN], nctenComp, mop, nin[KI_TEN])
      || engageMopDiceDwi(gctxComp[KI_DWI], &ndwiComp, mop, nin[KI_DWI])) {
    airMopAdd(mop, err = biffGetDone(PROBE), airFree, airMopAlways);
    fprintf(stderr, "trouble creating volumes:\n%s", err);
    airMopError(mop); return 1;
  }
  /*
  nrrdSave("tmp-cten.nrrd", nin[KI_TEN], NULL);
  nrrdSave("tmp-scl.nrrd", nin[KI_SCL], NULL);
  nrrdSave("tmp-vec.nrrd", nin[KI_VEC], NULL);
  nrrdSave("tmp-dwi.nrrd", nin[KI_DWI], NULL);
  */
  dwikind = tenDwiGageKindNew();
  airMopAdd(mop, dwikind, (airMopper)tenDwiGageKindNix, airMopAlways);
  if (tenDwiGageKindSet(dwikind, -1 /* thresh */, 0 /* soft */,
                        bval, 1 /* valueMin */,
                        ngrad, NULL,
                        tenEstimate1MethodLLS,
                        tenEstimate2MethodPeled,
                        424242)) {
    airMopAdd(mop, err = biffGetDone(TEN), airFree, airMopAlways);
    fprintf(stderr, "trouble creating DWI context:\n%s", err);
    airMopError(mop); return 1;
  }
  /* access through kind[] is const, but not through dwikind */
  kind[3] = dwikind;
  
  /* make sure kinds can parse back to themselves
     (const vs non-const issues are confusing....)
  for (kindIdx=0; kindIdx<KIND_NUM; kindIdx++) {
    if (kind[kindIdx] != meetGageKindParse(kind[kindIdx]->name)) {
      fprintf(stderr, "%s: kind[%u]->name %s wasn't parsed\n", me,
              kindIdx, kind[kindIdx]->name);
      airMopError(mop); return 1;
    }
  }
  */
  
  /* this is a work in progress ... */
  
  /* gageContextCopy on stack! */
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
