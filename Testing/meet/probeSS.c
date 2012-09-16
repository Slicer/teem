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

/*
** Tests:
** ... lots of gage stuff ...
**
** The main point of all this is to make sure of two separate things about
** gage, the testing of which requires so much in common that one program
** might as well do both.
**
** 1) that values and their their derivatives (where the gageKind supports
** it) are correctly handled in the multi-value gageKinds (gageKindVec,
** tenGageKind, tenDwiGageKind), relative to the gageKindScl ground-truth:
** the per-component values and derivatives have to match those reconstructed
** from sets of scalar volumes of the components.
*  Also, that gageContextCopy works even on dynamic kinds (like DWIs)
**
** 2) that scale-space reconstruction works: that sets of pre-blurred volumes
** can be generated and saved via the utilities in meet, that the smart
** hermite-spline based scale-space interpolation is working (for all kinds),
** and that gageContextCopy works on scale-space contexts
*/

/* the weird function names of the various local functions here (created in a
   rather adhoc and organic way) should be read in usual Teem order: from
   right to left */

#define PROBE "probeSS"
#define KIND_NUM 4
#define KI_SCL 0
#define KI_VEC 1
#define KI_TEN 2
#define KI_DWI 3

#define NRRD_NEW(nn, mm)                                        \
  (nn) = nrrdNew();                                             \
       airMopAdd((mm), (nn), (airMopper)nrrdNuke, airMopAlways)

static int
engageGenTensor(gageContext *gctx, Nrrd *ncten, double noiseStdv,
                unsigned int sx, unsigned int sy, unsigned int sz) {
  static const char me[]="engageGenTensor";
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
  gagePerVolume *pvl;

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
  NRRD_NEW(nclean, smop);
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

  /* wrap in gage context */
  if ( !(pvl = gagePerVolumeNew(gctx, ncten, tenGageKind))
       || gagePerVolumeAttach(gctx, pvl) ) {
    biffMovef(PROBE, GAGE, "%s: trouble engaging tensor", me);
    return 1;
  }

  airMopOkay(smop);
  return 0;
}

static int
engageGenScalar(gageContext *gctx, Nrrd *nscl, 
                gageContext *gctxComp, Nrrd *nsclCopy, const Nrrd *ncten) {
  static const char me[]="engageGenScalar";
  gagePerVolume *pvl;

  if (tenAnisoVolume(nscl, ncten, tenAniso_Tr, 0)) {
    biffMovef(PROBE, TEN, "%s: trouble creating scalar volume", me);
    return 1;
  }
  if (nrrdCopy(nsclCopy, nscl)) {
    biffMovef(PROBE, NRRD, "%s: trouble copying scalar volume", me);
    return 1;
  }

  /* wrap both in gage context */
  if ( !(pvl = gagePerVolumeNew(gctx, nscl, gageKindScl))
       || gagePerVolumeAttach(gctx, pvl)
       || !(pvl = gagePerVolumeNew(gctxComp, nsclCopy, gageKindScl))
       || gagePerVolumeAttach(gctxComp, pvl)) {
    biffMovef(PROBE, GAGE, "%s: trouble engaging scalars", me);
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
engageGenVector(gageContext *gctx, Nrrd *nvec, const Nrrd *nscl) {
  static const char me[]="engageGenVector";
  ptrdiff_t padMin[4] = {0, 0, 0, 0}, padMax[4];
  Nrrd *ntmp;
  airArray *smop;
  float *vec, *scl;
  size_t sx, sy, sz, xi, yi, zi, Px, Mx, Py, My, Pz, Mz;
  double spcX, spcY, spcZ;
  gagePerVolume *pvl;
  
  smop = airMopNew();
  if (nrrdTypeFloat != nscl->type) {
    biffAddf(PROBE, "%s: expected %s not %s type", me,
             airEnumStr(nrrdType, nrrdTypeFloat),
             airEnumStr(nrrdType, nscl->type));
    airMopError(smop); return 1;
  }
  NRRD_NEW(ntmp, smop);
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
          AIR_CAST(float, (scl[INDEX(Px, yi, zi)] - scl[INDEX(Mx, yi, zi)])/(2.0*spcX));
        vec[1 + 3*INDEX(xi, yi, zi)] = 
          AIR_CAST(float, (scl[INDEX(xi, Py, zi)] - scl[INDEX(xi, My, zi)])/(2.0*spcY));
        vec[2 + 3*INDEX(xi, yi, zi)] = 
          AIR_CAST(float, (scl[INDEX(xi, yi, Pz)] - scl[INDEX(xi, yi, Mz)])/(2.0*spcZ));
      }
    }
  }
#undef INDEX

  /* wrap in gage context */
  if ( !(pvl = gagePerVolumeNew(gctx, nvec, gageKindVec))
       || gagePerVolumeAttach(gctx, pvl) ) {
    biffMovef(PROBE, GAGE, "%s: trouble engaging vectors", me);
    return 1;
  }
  airMopError(smop);
  return 0;
}

/*
** make a DWI volume by simulating DWIs from given tensor
** this includes generating a new gradient set 
*/
static int
genDwi(Nrrd *ndwi, Nrrd *ngrad, 
       unsigned int gradNum, double bval, const Nrrd *ncten) {
  static const char me[]="genDwi";
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
      || tenExperSpecGradSingleBValSet(espec, AIR_FALSE /* insertB0 */, bval,
                                       AIR_CAST(double *, ngrad->data),
                                       AIR_CAST(unsigned int,
                                                ngrad->axis[1].size))) {
    biffMovef(PROBE, TEN, "%s: trouble generating grads or espec", me);
    airMopError(smop); return 1;
  }
  NRRD_NEW(nten, smop);
  NRRD_NEW(nb0, smop);
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
    NRRD_NEW(nvecComp[ci], mop);
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
    NRRD_NEW(nctenComp[ci], mop);
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
    NRRD_NEW(ndwiComp[ci], mop);
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

#if 0
typedef struct {
  double **aptr;       /* array of answer pointers */
  unsigned int *alen;  /* array of answer lengths */
  unsigned int anum;   /* lenth of aptr and alen arrays */
  double *san;         /* single buffer for copy + concating answers */
  unsigned int slen;   /* length of single buffer (sum of all alen[i]) */
} multiAnswer;

static multiAnswer*
multiAnswerNew(unsigned int num) {
  multiAnswer *man;
  
  man = AIR_CALLOC(1, multiAnswer);
  man->aptr = AIR_CALLOC(num, double *);
  man->alen = AIR_CALLOC(num, unsigned int);
  /* don't know answer lengths yet */
  man->san = NULL;
  man->slen = 0;
  return man;
}

static multiAnswer*
multiAnswerNix(multiAnswer *man) {
  
  airFree(man->aptr);
  airFree(man->alen);
  airFree(man->san);
  airFree(man);
  return NULL;
}
#endif

/*
** setting up gageContexts for the first of the two tasks listed above:
** making sure per-component information is handled correctly.  NOTE: The
** combination of function calls made here is very atypical for a Teem-using
** program
*/
static int
updateQueryKernelSetTask1(gageContext *gctxComp[KIND_NUM],
                          gageContext *gctx[KIND_NUM],
                          double support) {
  static const char me[]="updateQueryKernelSetTask1";
  double parm1[NRRD_KERNEL_PARMS_NUM], parmV[NRRD_KERNEL_PARMS_NUM];
  unsigned int kindIdx;

  if (4 != KIND_NUM) {
    biffAddf(PROBE, "%s: sorry, confused: KIND_NUM %u != 4", 
             me, KIND_NUM);
    return 1;
  }
  parm1[0] = 1.0;
  parmV[0] = support;
  for (kindIdx=0; kindIdx<KIND_NUM; kindIdx++) {
    if (0 /* logical no-op just for formatting */
        || gageKernelSet(gctxComp[kindIdx], gageKernel00, nrrdKernelCos4SupportDebug, parm1)
        || gageKernelSet(gctxComp[kindIdx], gageKernel11, nrrdKernelCos4SupportDebugD, parm1)
        || gageKernelSet(gctxComp[kindIdx], gageKernel22, nrrdKernelCos4SupportDebugDD, parm1)
        || gageKernelSet(gctx[kindIdx], gageKernel00, nrrdKernelCos4SupportDebug, parmV)
        || gageKernelSet(gctx[kindIdx], gageKernel11, nrrdKernelCos4SupportDebugD, parmV)
        || gageKernelSet(gctx[kindIdx], gageKernel22, nrrdKernelCos4SupportDebugDD, parmV)) {
      biffMovef(PROBE, GAGE, "%s: trouble setting kernel (kind %u)",
                me, kindIdx);
      return 1;
    }
  }
  /* Note that the contexts for the diced-up volumes of components are always
     of kind gageKindScl, and all the items are from the gageScl* enum */
  for (kindIdx=0; kindIdx<KIND_NUM; kindIdx++) {
    if (0 /* logical no-op just for formatting */
        || gageQueryItemOn(gctxComp[kindIdx], gctxComp[kindIdx]->pvl[0], gageSclValue)
        || gageQueryItemOn(gctxComp[kindIdx], gctxComp[kindIdx]->pvl[0], gageSclGradVec)
        || gageQueryItemOn(gctxComp[kindIdx], gctxComp[kindIdx]->pvl[0], gageSclHessian)) {
      biffMovef(PROBE, GAGE, "%s: trouble setting query (kind %u)",
                me, kindIdx);
      return 1;
    }
  }
  /* For these contexts, we have to use the kind-specific items that
     correspond to the values and derivatives */
  if (0 /* logical no-op just for formatting */
      || gageQueryItemOn(gctx[KI_SCL], gctx[KI_SCL]->pvl[0], gageSclValue)
      || gageQueryItemOn(gctx[KI_SCL], gctx[KI_SCL]->pvl[0], gageSclGradVec)
      || gageQueryItemOn(gctx[KI_SCL], gctx[KI_SCL]->pvl[0], gageSclHessian)
      
      || gageQueryItemOn(gctx[KI_VEC], gctx[KI_VEC]->pvl[0], gageVecVector)
      || gageQueryItemOn(gctx[KI_VEC], gctx[KI_VEC]->pvl[0], gageVecJacobian)
      || gageQueryItemOn(gctx[KI_VEC], gctx[KI_VEC]->pvl[0], gageVecHessian)

      || gageQueryItemOn(gctx[KI_TEN], gctx[KI_TEN]->pvl[0], tenGageTensor)
      || gageQueryItemOn(gctx[KI_TEN], gctx[KI_TEN]->pvl[0], tenGageTensorGrad)
      || gageQueryItemOn(gctx[KI_TEN], gctx[KI_TEN]->pvl[0], tenGageHessian)

      || gageQueryItemOn(gctx[KI_DWI], gctx[KI_DWI]->pvl[0], tenDwiGageAll)) {
    biffMovef(PROBE, GAGE, "%s: trouble setting item", me);
    return 1;
  }
  for (kindIdx=0; kindIdx<KIND_NUM; kindIdx++) {
    if (gageUpdate(gctxComp[kindIdx])) {
      biffMovef(PROBE, GAGE, "%s: trouble updating comp gctx %u", 
                me, kindIdx);
      return 1;
    }
  }  
  for (kindIdx=0; kindIdx<KIND_NUM; kindIdx++) {
    if (gageUpdate(gctx[kindIdx])) {
      biffMovef(PROBE, GAGE, "%s: trouble updating single gctx %u", 
                me, kindIdx);
      return 1;
    }
  }

  /* TODO have to figure out how to get all the answer pointers
     in some logical way */

  return 0;
}

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
  Nrrd *nin[KIND_NUM],
    /* these are the volumes that are used in gctxComp[] */
    *nsclCopy, *nvecComp[3], *nctenComp[7], **ndwiComp,
    *ngrad;  /* need access to list of gradients used to make DWIs;
                (this is not the gradient of a scalar field) */
  unsigned int kindIdx, volSize[3] = {45, 46, 47},
    gradNum = 9; /* small number so that missing one will produce
                    a big reconstruction error */
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
    GAGE_CTX_NEW(gctxComp[kindIdx], mop);
  }
#undef GAGE_CTX_NEW

  printf("#  0\n");
  for (kindIdx=0; kindIdx<KIND_NUM; kindIdx++) {
    NRRD_NEW(nin[kindIdx], mop);
  }
  NRRD_NEW(ngrad, mop);
  NRRD_NEW(nsclCopy, mop);
  printf("#  1\n");
  if (engageGenTensor(gctx[KI_TEN], nin[KI_TEN], noiseStdv,
                      volSize[0], volSize[1], volSize[2])
      || engageGenScalar(gctx[KI_SCL], nin[KI_SCL],
                         gctxComp[KI_SCL], nsclCopy, nin[KI_TEN])
      || engageGenVector(gctx[KI_VEC], nin[KI_VEC], nin[KI_SCL])
      /* engage'ing of nin[KI_DWI] done below */
      || genDwi(nin[KI_DWI], ngrad, gradNum /* for B0 */,
                bval, nin[KI_TEN])
      || engageMopDiceVector(gctxComp[KI_VEC], nvecComp, mop, nin[KI_VEC])
      || engageMopDiceTensor(gctxComp[KI_TEN], nctenComp, mop, nin[KI_TEN])
      || engageMopDiceDwi(gctxComp[KI_DWI], &ndwiComp, mop, nin[KI_DWI])) {
    airMopAdd(mop, err = biffGetDone(PROBE), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble creating volumes:\n%s", me, err);
    airMopError(mop); return 1;
  }
  printf("#  2\n");
  dwikind = tenDwiGageKindNew();
  airMopAdd(mop, dwikind, (airMopper)tenDwiGageKindNix, airMopAlways);
  if (tenDwiGageKindSet(dwikind, -1 /* thresh */, 0 /* soft */,
                        bval, 1 /* valueMin */,
                        ngrad, NULL,
                        tenEstimate1MethodLLS,
                        tenEstimate2MethodPeled,
                        424242)) {
    airMopAdd(mop, err = biffGetDone(TEN), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble creating DWI kind:\n%s", me, err);
    airMopError(mop); return 1;
  }
  printf("#  3\n");
  /* access through kind[] is const, but not through dwikind */
  kind[KI_DWI] = dwikind;
  /* engage dwi vol */
  {
    gagePerVolume *pvl;
    if ( !(pvl = gagePerVolumeNew(gctx[KI_DWI], nin[KI_DWI], kind[KI_DWI]))
         || gagePerVolumeAttach(gctx[KI_DWI], pvl) ) {
      airMopAdd(mop, err = biffGetDone(GAGE), airFree, airMopAlways);
      fprintf(stderr, "%s: trouble creating DWI context:\n%s", me, err);
      airMopError(mop); return 1;
    }
  }
  
  printf("#  4\n");
  /* make sure kinds can parse back to themselves */
  /* the messiness here is in part because of problems with the gage
     API, and the weirdness of how meetGageKindParse is either allocating
     something, or not, depending on its input.  There is no way to 
     refer to the "name" of a dwi kind without having allocated something. */
  for (kindIdx=0; kindIdx<KIND_NUM; kindIdx++) {
    printf("#  5.%u\n", kindIdx);
    if (!(kind[kindIdx]->dynamicAlloc)) {
      if (kind[kindIdx] != meetGageKindParse(kind[kindIdx]->name)) {
        fprintf(stderr, "%s: static kind[%u]->name %s wasn't parsed\n", me,
                kindIdx, kind[kindIdx]->name);
        airMopError(mop); return 1;
      }
    } else {
      gageKind *ktmp;
      ktmp = meetGageKindParse(kind[kindIdx]->name);
      if (!ktmp) {
        fprintf(stderr, "%s: dynamic kind[%u]->name %s wasn't parsed\n", me,
                kindIdx, kind[kindIdx]->name);
        airMopError(mop); return 1;
      }
      if (airStrcmp(ktmp->name, kind[kindIdx]->name)) {
        fprintf(stderr, "%s: parsed dynamic kind[%u]->name %s didn't match %s\n", me,
                kindIdx, ktmp->name, kind[kindIdx]->name);
        airMopError(mop); return 1;
      }
      if (!airStrcmp(TEN_DWI_GAGE_KIND_NAME, ktmp->name)) {
        /* HEY: there needs to be a generic way of freeing
           a dynamic kind, perhaps a new nixer field in gageKind */
        ktmp = tenDwiGageKindNix(ktmp);
      }
    }
  }
  
  /* ========================== TASK 1 */
  printf("#  6\n");
  if (updateQueryKernelSetTask1(gctxComp, gctx, 1.0 /* support */)) {
    airMopAdd(mop, err = biffGetDone(PROBE), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble updating contexts:\n%s", me, err);
    airMopError(mop); return 1;
  }
  
  /*
  nrrdSave("tmp-cten.nrrd", nin[KI_TEN], NULL);
  nrrdSave("tmp-scl.nrrd", nin[KI_SCL], NULL);
  nrrdSave("tmp-vec.nrrd", nin[KI_VEC], NULL);
  nrrdSave("tmp-dwi.nrrd", nin[KI_DWI], NULL);
  */
  /*
  for (kindIdx=0; kindIdx<KIND_NUM; kindIdx++) {
    printf("%s: %s (%s) -> (%u) %u\n", me, kind[kindIdx]->name,
           kind[kindIdx]->dynamicAlloc ? "dynamic" : "static",
           kind[kindIdx]->baseDim, kind[kindIdx]->valLen);
  }
  */

  /* varying kernel support for multi-variate volumes */
  /* make sure per-component reconstruction of values and
     derivatives (where possible) is working correctly */
  /* gageContextCopy on multi-variate values */

  /* ========================== TASK 2 */
  /* create scale-space stacks with tent, ctmr, and hermite */
  /* save them, free, and read them back in */
  /* gageContextCopy on stack */
  /* pick a scale in-between tau samples */
  /* for all the tau's half-way between tau samples in scale:
       blur at that tau to get correct values 
       check that error with hermite is lower than ctmr is lower than tent */
  /* for all tau samples:
       blur at that tau to (re-)get correct values 
       check that everything agrees there */


  /* single probe with high verbosity */

  printf("#  7\n");
  airMopOkay(mop);
  return 0;
}
#undef NRRD_NEW
