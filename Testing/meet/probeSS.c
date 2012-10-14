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
** The main point of all this is to make sure of two (or maybe 2.5) separate
** things about gage, the testing of which requires so much in common that one
** program might as well do them all. The tasks are:
**
** 1) ensure that values and their derivatives (where the gageKind supports
** it) are correctly handled in the multi-value gageKinds (gageKindVec,
** tenGageKind, tenDwiGageKind), relative to the gageKindScl ground-truth: the
** per-component values and derivatives have to match those reconstructed from
** sets of scalar volumes of the components.  
** ==> Also, that gageContextCopy works even on dynamic kinds (like DWIs)
**
** 1b) that there is expected consistency between the scalar, vector, tensor,
** and DWI properties of a related set of volumes.  So the test starts with a
** diffusion tensor field and generates DWIs, scalars (norm squared of the
** tensor), and vectors (gradient of norm squared) from this.
**
** 2) that scale-space reconstruction works: that sets of pre-blurred volumes
** can be generated and saved via the utilities in meet, that the smart
** hermite-spline based scale-space interpolation is working (for all kinds),
** and that gageContextCopy works on scale-space contexts
*/

#define BKEY "probeSS"
#define KIND_NUM 4
#define KI_SCL 0
#define KI_VEC 1
#define KI_TEN 2
#define KI_DWI 3

#define HALTON_BASE 100

static unsigned int volSize[3] = {45, 46, 47};

#define NRRD_NEW(nn, mm)                                        \
  (nn) = nrrdNew();                                             \
       airMopAdd((mm), (nn), (airMopper)nrrdNuke, airMopAlways)

/* the weird function names of the various local functions here (created in a
   rather adhoc and organic way) should be read in usual Teem order: from
   right to left */

static int
engageGenTensor(gageContext *gctx, Nrrd *ncten,
                /* out/in */
                double noiseStdv, unsigned int seed,
                unsigned int sx, unsigned int sy, unsigned int sz) {
  static const char me[]="engageGenTensor";
  hestParm *hparm;
  airArray *smop;
  char tmpStr[4][AIR_STRLEN_SMALL];
  Nrrd *nclean;
  NrrdIter *narg0, *narg1;
  const char *helixArgv[] = 
  /*   0     1     2    3     4     5     6     7     8     9 */
    {"-s", NULL, NULL, NULL, "-v", "0", "-r", "45", "-o", NULL, 
     "-ev", "0.00086", "0.00043", "0.00021", "-bg", "0.003", "-b", "5",
     NULL};
  int helixArgc;
  gagePerVolume *pvl;
  
  smop = airMopNew();
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
  sprintf(tmpStr[3], "tmp-ten.nrrd");
  helixArgv[9] = tmpStr[3];
  helixArgc = AIR_CAST(int, sizeof(helixArgv)/sizeof(char *)) - 1;
  if (tend_helixCmd.main(helixArgc, helixArgv, me, hparm)) {
    /* error already went to stderr, not to any biff container */
    biffAddf(BKEY, "%s: problem running tend %s", me, tend_helixCmd.name);
    airMopError(smop); return 1;
  }
  NRRD_NEW(nclean, smop);
  if (nrrdLoad(nclean, tmpStr[3], NULL)) {
    biffAddf(BKEY, "%s: trouble loading from new vol %s", me, tmpStr[3]);
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
  airSrandMT(seed);
  if (nrrdArithIterBinaryOp(ncten, nrrdBinaryOpNormalRandScaleAdd,
                            narg0, narg1)) {
    biffMovef(BKEY, NRRD, "%s: trouble noisying output", me);
    airMopError(smop); return 1;
  }

  /* wrap in gage context */
  if ( !(pvl = gagePerVolumeNew(gctx, ncten, tenGageKind))
       || gagePerVolumeAttach(gctx, pvl) ) {
    biffMovef(BKEY, GAGE, "%s: trouble engaging tensor", me);
    return 1;
  }

  airMopOkay(smop);
  return 0;
}

/* 
** takes in ncten, measures "S" to nscl, copies that to nsclCopy,
** wraps those in gctx and gctxComp
*/
static int
engageGenScalar(gageContext *gctx, Nrrd *nscl, 
                gageContext *gctxComp, Nrrd *nsclCopy,
                /* out/in */
                const Nrrd *ncten) {
  static const char me[]="engageGenScalar";
  gagePerVolume *pvl;

  if (tenAnisoVolume(nscl, ncten, tenAniso_S, 0)) {
    biffMovef(BKEY, TEN, "%s: trouble creating scalar volume", me);
    return 1;
  }
  if (nrrdCopy(nsclCopy, nscl)) {
    biffMovef(BKEY, NRRD, "%s: trouble copying scalar volume", me);
    return 1;
  }

  /* wrap both in gage context */
  if ( !(pvl = gagePerVolumeNew(gctx, nscl, gageKindScl))
       || gagePerVolumeAttach(gctx, pvl)
       || !(pvl = gagePerVolumeNew(gctxComp, nsclCopy, gageKindScl))
       || gagePerVolumeAttach(gctxComp, pvl)) {
    biffMovef(BKEY, GAGE, "%s: trouble engaging scalars", me);
    return 1;
  }

  return 0;
}

/* 
** Makes a vector volume by measuring the gradient
** Being the gradient of the given scalar volume is just to make
** something vaguely interesting, and to test consistency with
** the the same gradient as measured in the tensor field.
*/
static int
engageGenVector(gageContext *gctx, Nrrd *nvec,
                /* out/in */
                const Nrrd *nscl) {
  static const char me[]="engageGenVector";
  ptrdiff_t padMin[4] = {0, 0, 0, 0}, padMax[4];
  Nrrd *ntmp;
  airArray *smop;
  float *vec, *scl;
  size_t sx, sy, sz, xi, yi, zi, Px, Mx, Py, My, Pz, Mz;
  double dnmX, dnmY, dnmZ;
  gagePerVolume *pvl;
  
  smop = airMopNew();
  if (nrrdTypeFloat != nscl->type) {
    biffAddf(BKEY, "%s: expected %s not %s type", me,
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
    biffMovef(BKEY, NRRD, "%s: trouble", me);
    airMopError(smop); return 1;
  }
  dnmX = 0.5/nrrdSpaceVecNorm(nscl->spaceDim, nscl->axis[0].spaceDirection);
  dnmY = 0.5/nrrdSpaceVecNorm(nscl->spaceDim, nscl->axis[1].spaceDirection);
  dnmZ = 0.5/nrrdSpaceVecNorm(nscl->spaceDim, nscl->axis[2].spaceDirection);
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
          AIR_CAST(float, (scl[INDEX(Px, yi, zi)] - scl[INDEX(Mx, yi, zi)])*dnmX);
        vec[1 + 3*INDEX(xi, yi, zi)] = 
          AIR_CAST(float, (scl[INDEX(xi, Py, zi)] - scl[INDEX(xi, My, zi)])*dnmY);
        vec[2 + 3*INDEX(xi, yi, zi)] = 
          AIR_CAST(float, (scl[INDEX(xi, yi, Pz)] - scl[INDEX(xi, yi, Mz)])*dnmZ);
      }
    }
  }
#undef INDEX

  /* wrap in gage context */
  if ( !(pvl = gagePerVolumeNew(gctx, nvec, gageKindVec))
       || gagePerVolumeAttach(gctx, pvl) ) {
    biffMovef(BKEY, GAGE, "%s: trouble engaging vectors", me);
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
       /* out/in */
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
      || tenExperSpecGradSingleBValSet(espec, AIR_FALSE /* insertB0 */,
                                       bval,
                                       AIR_CAST(double *, ngrad->data),
                                       AIR_CAST(unsigned int,
                                                ngrad->axis[1].size))) {
    biffMovef(BKEY, TEN, "%s: trouble generating grads or espec", me);
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
    biffMovef(BKEY, NRRD, "%s: trouble slicing or cropping ten vol", me);
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
    biffMovef(BKEY, NRRD, "%s: trouble generating b0 vol", me);
    airMopError(smop); return 1;
  }
  if (tenModelSimulate(ndwi, nrrdTypeUShort, espec,
                       tenModel1Tensor2, 
                       nb0, nten, AIR_TRUE /* keyValueSet */)) {
    biffMovef(BKEY, TEN, "%s: trouble simulating DWI vol", me);
    airMopError(smop); return 1;
  }

  airMopOkay(smop);
  return 0;
}

int
engageMopDiceVector(gageContext *gctx, Nrrd *nvecComp[3],
                    /* out/in */
                    airArray *mop, const Nrrd* nvec) {
  static const char me[]="engageMopDiceVector";
  gagePerVolume *pvl;
  unsigned int ci;
  char stmp[AIR_STRLEN_SMALL];

  if (!( 4 == nvec->dim && 3 == nvec->axis[0].size )) {
    biffAddf(BKEY, "%s: expected 4-D 3-by-X nrrd (not %u-D %s-by-X)",
             me, nvec->dim, airSprintSize_t(stmp, nvec->axis[0].size));
    return 1;
  }
  for (ci=0; ci<3; ci++) {
    NRRD_NEW(nvecComp[ci], mop);
    if (nrrdSlice(nvecComp[ci], nvec, 0, ci)) {
      biffMovef(BKEY, NRRD, "%s: trouble getting component %u", me, ci);
      return 1;
    }
    if ( !(pvl = gagePerVolumeNew(gctx, nvecComp[ci], gageKindScl))
         || gagePerVolumeAttach(gctx, pvl) ) {
      biffMovef(BKEY, GAGE, "%s: trouble engaging component %u", me, ci);
      return 1;
    }
  }
  
  return 0;
}

int
engageMopDiceTensor(gageContext *gctx, Nrrd *nctenComp[7],
                    /* out/in */
                    airArray *mop, const Nrrd* ncten) {
  static const char me[]="engageMopDiceTensor";
  gagePerVolume *pvl;
  unsigned int ci;

  if (tenTensorCheck(ncten, nrrdTypeFloat, AIR_TRUE /* want4F */,
                     AIR_TRUE /* useBiff */)) {
    biffMovef(BKEY, TEN, "%s: didn't get tensor volume", me);
    return 1;
  }
  for (ci=0; ci<7; ci++) {
    NRRD_NEW(nctenComp[ci], mop);
    if (nrrdSlice(nctenComp[ci], ncten, 0, ci)) {
      biffMovef(BKEY, NRRD, "%s: trouble getting component %u", me, ci);
      return 1;
    }
    if ( !(pvl = gagePerVolumeNew(gctx, nctenComp[ci], gageKindScl))
         || gagePerVolumeAttach(gctx, pvl) ) {
      biffMovef(BKEY, GAGE, "%s: trouble engaging component %u", me, ci);
      return 1;
    }
  }
  
  return 0;
}

int
engageMopDiceDwi(gageContext *gctx, Nrrd ***ndwiCompP,
                 /* out/in */
                 airArray *mop, const Nrrd* ndwi) {
  static const char me[]="mopDiceDwi";
  Nrrd **ndwiComp;
  size_t dwiNum;
  char stmp[AIR_STRLEN_SMALL];
  gagePerVolume *pvl;
  unsigned int ci;

  if (!( 4 == ndwi->dim )) {
    biffAddf(BKEY, "%s: wanted 4D volume (not %u)", me, ndwi->dim);
    return 1;
  }
  if (!( nrrdKindList == ndwi->axis[0].kind &&
         nrrdKindSpace == ndwi->axis[1].kind &&
         nrrdKindSpace == ndwi->axis[2].kind &&
         nrrdKindSpace == ndwi->axis[3].kind )) {
    biffAddf(BKEY, "%s: wanted kinds %s,3x%s, not %s,%s,%s,%s", me,
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
    biffAddf(BKEY, "%s: couldn't alloc %s Nrrd*", me, 
             airSprintSize_t(stmp, dwiNum));
    return 1;
  }
  airMopAdd(mop, ndwiComp, airFree, airMopAlways);
  *ndwiCompP = ndwiComp;
  for (ci=0; ci<dwiNum; ci++) {
    NRRD_NEW(ndwiComp[ci], mop);
    if (nrrdSlice(ndwiComp[ci], ndwi, 0, ci)) {
      biffMovef(BKEY, NRRD, "%s: trouble getting component %u", me, ci);
      return 1;
    }
    if ( !(pvl = gagePerVolumeNew(gctx, ndwiComp[ci], gageKindScl))
         || gagePerVolumeAttach(gctx, pvl) ) {
      biffMovef(BKEY, GAGE, "%s: trouble engaging component %u", me, ci);
      return 1;
    }
  }
  return 0;
}

typedef struct {
  const double **aptr; /* array of pointers to (const) answers */
  gageItemSpec *ispec; /* array of gageItemSpecs (not pointers to them) */
  unsigned int *alen;  /* array of answer lengths */
  unsigned int *sidx;  /* array of index (within san) of copied answer */
  unsigned int anum;   /* lenth of aptr, ispec, alen, sidx arrays */
  double *san;         /* single buffer for copy + concating answers */
  unsigned int slen;   /* length of single buffer (sum of all alen[i]) */
} multiAnswer;

static multiAnswer*
multiAnswerNew(void) {
  multiAnswer *man;
  
  man = AIR_CALLOC(1, multiAnswer);
  man->aptr = NULL;
  man->ispec = NULL;
  man->alen = NULL;
  man->sidx = NULL;
  man->anum = 0;
  man->san = NULL;
  man->slen = 0;
  return man;
}

void
multiAnswerLenSet(multiAnswer *man, unsigned int anum) {
  
  man->aptr = AIR_CALLOC(anum, const double *);
  man->ispec = AIR_CALLOC(anum, gageItemSpec);
  man->alen = AIR_CALLOC(anum, unsigned int);
  man->sidx = AIR_CALLOC(anum, unsigned int);
  man->anum = anum;
  /* don't know answer lengths yet */
  man->san = airFree(man->san);
  man->slen = 0;
  return;
}

static multiAnswer*
multiAnswerNix(multiAnswer *man) {
  
  airFree(man->aptr);
  airFree(man->ispec);
  airFree(man->alen);
  airFree(man->sidx);
  airFree(man->san);
  airFree(man);
  return NULL;
}

static void
multiAnswerAdd(multiAnswer *man, unsigned int ansIdx, 
               const gageContext *gctx, const gagePerVolume *pvl,
               unsigned int item) {

  man->aptr[ansIdx] = gageAnswerPointer(gctx, pvl, item);
  man->ispec[ansIdx].kind = pvl->kind;
  man->ispec[ansIdx].item = item;
  man->alen[ansIdx] = gageAnswerLength(gctx, pvl, item);
  /* HEY hack: doing this tally and allocation only on the last time
     we're called, presuming calls with increasing ansIdx */
  man->slen = 0;
  if (ansIdx == man->anum-1) {
    unsigned int ai;
    for (ai=0; ai<man->anum; ai++) {
      man->sidx[ai] = man->slen;
      man->slen += man->alen[ai];
    }
    man->san = AIR_CALLOC(man->slen, double);
    /*
    fprintf(stderr, "!%s: (%s) slen = %u\n", "multiAnswerAdd",
            pvl->kind->name, man->slen);
    */
  }
  return;
}

static void
multiAnswerCollect(multiAnswer *man) {
  unsigned int ai;

  for (ai=0; ai<man->anum; ai++) {
    memcpy(man->san + man->sidx[ai],
           man->aptr[ai],
           man->alen[ai]*sizeof(double));
  }
  return;
}

static int 
multiAnswerCompare(multiAnswer *man1, multiAnswer *man2) {
  static const char me[]="multiAnswerCompare";
  unsigned int si;

  if (man1->slen != man2->slen) {
    biffAddf("%s: man1->slen %u != man2->slen %u", me, 
             man1->slen, man2->slen);
    return 1;
  }
  for (si=0; si<man1->slen; si++) {
    if (man1->san[si] != man2->san[si]) {
      /* HEY should track down which part of which answer,
         in man1 and man2, is different, which was the 
         purpose of recording ispec and sidx */
      biffAddf(BKEY, "%s: man1->san[si] %.17g != man2->san[si] %.17g", me,
               man1->san[si], man2->san[si]);
      return 1;
    }
  }
  return 0;
}

/*
** setting up gageContexts for the first of the two tasks listed above: making
** sure per-component information is handled correctly.  NOTE: The combination
** of function calls made here is very atypical for a Teem-using program
**
** The component "Comp" (scalar volume) contexts are the ground truth, so we
** probe them with radius=1 cos4 kernel, and probe the non-scalar volumes with
** the variable radius cos4 kernel
*/
static int
updateQueryKernelSetTask1(gageContext *gctxComp[KIND_NUM],
                          gageContext *gctx[KIND_NUM],
                          multiAnswer *manComp[KIND_NUM],
                          multiAnswer *man[KIND_NUM],
                          double support) {
  static const char me[]="updateQueryKernelSetTask1";
  double parm1[NRRD_KERNEL_PARMS_NUM], parmV[NRRD_KERNEL_PARMS_NUM];
  unsigned int kindIdx;

  if (4 != KIND_NUM) {
    biffAddf(BKEY, "%s: sorry, confused: KIND_NUM %u != 4", 
             me, KIND_NUM);
    return 1;
  }
  parm1[0] = 1.0;
  parmV[0] = support;
  for (kindIdx=0; kindIdx<KIND_NUM; kindIdx++) {
    if (0 /* (no-op for formatting) */
        || gageKernelSet(gctxComp[kindIdx], gageKernel00, nrrdKernelCos4SupportDebug, parm1)
        || gageKernelSet(gctxComp[kindIdx], gageKernel11, nrrdKernelCos4SupportDebugD, parm1)
        || gageKernelSet(gctxComp[kindIdx], gageKernel22, nrrdKernelCos4SupportDebugDD, parm1)
        || gageKernelSet(gctx[kindIdx], gageKernel00, nrrdKernelCos4SupportDebug, parmV)
        || gageKernelSet(gctx[kindIdx], gageKernel11, nrrdKernelCos4SupportDebugD, parmV)
        || gageKernelSet(gctx[kindIdx], gageKernel22, nrrdKernelCos4SupportDebugDD, parmV)) {
      biffMovef(BKEY, GAGE, "%s: trouble setting kernel (kind %u)",
                me, kindIdx);
      return 1;
    }
  }
  /* Note that the contexts for the diced-up volumes of components are always
     of kind gageKindScl, and all the items are from the gageScl* enum */
  for (kindIdx=0; kindIdx<KIND_NUM; kindIdx++) {
    unsigned int pvi;
    /*
    fprintf(stderr, "!%s: gctx[%u] has %u pvls\n", me, kindIdx, gctx[kindIdx]->pvlNum);
    fprintf(stderr, "!%s: gctxComp[%u] has %u pvls\n", me, kindIdx, gctxComp[kindIdx]->pvlNum);
    */
    for (pvi=0; pvi<gctxComp[kindIdx]->pvlNum; pvi++) {
      if (0 /* (no-op for formatting) */
          || gageQueryItemOn(gctxComp[kindIdx], gctxComp[kindIdx]->pvl[pvi], gageSclValue)
          || gageQueryItemOn(gctxComp[kindIdx], gctxComp[kindIdx]->pvl[pvi], gageSclGradVec)
          || gageQueryItemOn(gctxComp[kindIdx], gctxComp[kindIdx]->pvl[pvi], gageSclHessian)) {
        biffMovef(BKEY, GAGE, "%s: trouble setting query (kind %u) on pvi %u",
                  me, kindIdx, pvi);
        return 1;
      }
    }
  }
  /* For the original contexts, we have to use the kind-specific items that
     correspond to the values and derivatives */
  if (0 /* (no-op for formatting) */
      || gageQueryItemOn(gctx[KI_SCL], gctx[KI_SCL]->pvl[0], gageSclValue)
      || gageQueryItemOn(gctx[KI_SCL], gctx[KI_SCL]->pvl[0], gageSclGradVec)
      || gageQueryItemOn(gctx[KI_SCL], gctx[KI_SCL]->pvl[0], gageSclHessian)
      
      || gageQueryItemOn(gctx[KI_VEC], gctx[KI_VEC]->pvl[0], gageVecVector)
      || gageQueryItemOn(gctx[KI_VEC], gctx[KI_VEC]->pvl[0], gageVecJacobian)
      || gageQueryItemOn(gctx[KI_VEC], gctx[KI_VEC]->pvl[0], gageVecHessian)

      || gageQueryItemOn(gctx[KI_TEN], gctx[KI_TEN]->pvl[0], tenGageTensor)
      || gageQueryItemOn(gctx[KI_TEN], gctx[KI_TEN]->pvl[0], tenGageTensorGrad)
      || gageQueryItemOn(gctx[KI_TEN], gctx[KI_TEN]->pvl[0], tenGageSGradVec)
      || gageQueryItemOn(gctx[KI_TEN], gctx[KI_TEN]->pvl[0], tenGageHessian)
      || gageQueryItemOn(gctx[KI_TEN], gctx[KI_TEN]->pvl[0], tenGageSHessian)

      || gageQueryItemOn(gctx[KI_DWI], gctx[KI_DWI]->pvl[0], tenDwiGageAll)) {
    biffMovef(BKEY, GAGE, "%s: trouble setting item", me);
    return 1;
  }
  for (kindIdx=0; kindIdx<KIND_NUM; kindIdx++) {
    if (gageUpdate(gctxComp[kindIdx])) {
      biffMovef(BKEY, GAGE, "%s: trouble updating comp gctx %u", 
                me, kindIdx);
      return 1;
    }
  }  
  for (kindIdx=0; kindIdx<KIND_NUM; kindIdx++) {
    if (gageUpdate(gctx[kindIdx])) {
      biffMovef(BKEY, GAGE, "%s: trouble updating single gctx %u", 
                me, kindIdx);
      return 1;
    }
  }
  for (kindIdx=0; kindIdx<KIND_NUM; kindIdx++) {
    unsigned int pvi;
    /* to get started, just comparing (scalar) components */
    multiAnswerLenSet(manComp[kindIdx], gctxComp[kindIdx]->pvlNum);
    for (pvi=0; pvi<gctxComp[kindIdx]->pvlNum; pvi++) {
      multiAnswerAdd(manComp[kindIdx], pvi, 
                     gctxComp[kindIdx], gctxComp[kindIdx]->pvl[pvi],
                     gageSclValue);
    }
    multiAnswerLenSet(man[kindIdx], 1);
    switch(kindIdx) {
    case KI_SCL:
      multiAnswerAdd(man[kindIdx], 0, gctx[kindIdx], gctx[kindIdx]->pvl[0], gageSclValue);
      break;
    case KI_VEC:
      multiAnswerAdd(man[kindIdx], 0, gctx[kindIdx], gctx[kindIdx]->pvl[0], gageVecVector);
      break;
    case KI_TEN:
      multiAnswerAdd(man[kindIdx], 0, gctx[kindIdx], gctx[kindIdx]->pvl[0], tenGageTensor);
      break;
    case KI_DWI:
      multiAnswerAdd(man[kindIdx], 0, gctx[kindIdx], gctx[kindIdx]->pvl[0], tenDwiGageAll);
      break;
    }
  }

  return 0;
}

static int
probeTask1(gageContext *gctxComp[KIND_NUM],
           gageContext *gctx[KIND_NUM],
           multiAnswer *manComp[KIND_NUM],
           multiAnswer *man[KIND_NUM],
           unsigned int probeNum, double ksupport) {
  static const char me[]="probeTask1";
  unsigned int kindIdx, probeIdx;
  double pos[3], upos[3], minp[3], maxp[3];

  ELL_3V_SET(minp, ksupport, ksupport, ksupport);
  ELL_3V_SET(maxp, volSize[0]-1-ksupport, volSize[1]-1-ksupport, volSize[2]-1-ksupport);

  for (probeIdx=0; probeIdx<probeNum; probeIdx++) {
    airHalton(upos, probeIdx+HALTON_BASE, airPrimeList + 0, 3);
    pos[0] = AIR_AFFINE(0.0, upos[0], 1.0, minp[0], maxp[0]);
    pos[1] = AIR_AFFINE(0.0, upos[1], 1.0, minp[1], maxp[1]);
    pos[2] = AIR_AFFINE(0.0, upos[2], 1.0, minp[2], maxp[2]);
    for (kindIdx=0; kindIdx<KIND_NUM; kindIdx++) {
#define PROBE(gctx, what, pos)                                          \
      if (gageProbeSpace((gctx), (pos)[0], (pos)[1], (pos)[2],          \
                         AIR_TRUE /* indexSpace */,                     \
                         AIR_FALSE /* clamp */)) {                      \
        biffAddf(BKEY, "%s: probe (%s) error(%d): %s", me,              \
                 what, (gctx)->errNum, (gctx)->errStr);                 \
        return 1;                                                       \
      }

      PROBE(gctxComp[kindIdx], "comp", pos);
      PROBE(gctx[kindIdx], "single", pos);

#undef PROBE

      multiAnswerCollect(man[kindIdx]);
      multiAnswerCollect(manComp[kindIdx]);
      if (multiAnswerCompare(manComp[kindIdx], man[kindIdx])) {
        biffAddf(BKEY, "%s: on point %u of kindIdx %u", me,
                 probeIdx, kindIdx);
        return 1;
      }
    }
  }

  return 0;
}

static const char *testInfo = "for testing gage";

int
main(int argc, const char **argv) {
  const char *me;
  char *err = NULL;
  hestOpt *hopt=NULL;
  hestParm *hparm;
  airArray *mop;

  const gageKind *kind[KIND_NUM] = {
    /*    0            1           2         3          */
    gageKindScl, gageKindVec, tenGageKind, NULL /* dwi */};
  gageKind *dwikind = NULL;
  gageContext *gctxComp[KIND_NUM], *gctx[KIND_NUM];
  multiAnswer *manComp[KIND_NUM], *man[KIND_NUM];
  Nrrd *nin[KIND_NUM],
    /* these are the volumes that are used in gctxComp[] */
    *nsclCopy, *nvecComp[3], *nctenComp[7], **ndwiComp,
    *ngrad;  /* need access to list of gradients used to make DWIs;
                (this is not the gradient of a scalar field) */
  double bval = 1000, noiseStdv=0.0001, ksupport;
  unsigned int kindIdx, probeNum, 
    gradNum = 10; /* small number so that missing one will produce
                     a big reconstruction error */

  me = argv[0];
  mop = airMopNew();
  hparm = hestParmNew();
  airMopAdd(mop, hparm, (airMopper)hestParmFree, airMopAlways);
  /* learn things from hest */
  hestOptAdd(&hopt, "supp", "r", airTypeDouble, 1, 1, &ksupport, "1.0",
             "kernel support");
  hestOptAdd(&hopt, "pnum", "N", airTypeUInt, 1, 1, &probeNum, "100",
             "# of probes");
  hestParseOrDie(hopt, argc-1, argv+1, hparm, me, testInfo,
                 AIR_TRUE, AIR_TRUE, AIR_TRUE);
  airMopAdd(mop, hopt, (airMopper)hestOptFree, airMopAlways);
  airMopAdd(mop, hopt, (airMopper)hestParseFree, airMopAlways);

  /* This was a tricky bug: Adding gageContextNix(gctx) to the mop
     as soon as a gctx is created makes sense.  But, in the first
     version of this code, gageContextNix was added to the mop 
     BEFORE tenDwiGageKindNix was added, which meant that gageContextNix
     was being called AFTER tenDwiGageKindNix.  However, that meant
     that existing pvls had their pvl->kind being freed from under them,
     but gagePerVolumeNix certainly needs to look at and possibly call
     pvl->kind->pvlDataNix in order clean up pvl->data.  *Therefore*,
     we have to add tenDwiGageKindNix to the mop prior to gageContextNix,
     even though nothing about the (dyanamic) kind has been set yet.
     If we had something like smart pointers, then tenDwiGageKindNix
     would not have freed something that an extant pvl was using */
  dwikind = tenDwiGageKindNew();
  airMopAdd(mop, dwikind, (airMopper)tenDwiGageKindNix, airMopAlways);

#define GAGE_CTX_NEW(gg, mm)                                      \
  (gg) = gageContextNew();                                        \
  airMopAdd((mm), (gg), (airMopper)gageContextNix, airMopAlways); \
  gageParmSet((gg), gageParmRenormalize, AIR_FALSE);              \
  gageParmSet((gg), gageParmCheckIntegrals, AIR_TRUE)

  for (kindIdx=0; kindIdx<KIND_NUM; kindIdx++) {
    GAGE_CTX_NEW(gctx[kindIdx], mop);
    GAGE_CTX_NEW(gctxComp[kindIdx], mop);
    man[kindIdx] = multiAnswerNew();
    manComp[kindIdx] = multiAnswerNew();
    airMopAdd(mop, man[kindIdx], (airMopper)multiAnswerNix, airMopAlways);
    airMopAdd(mop, manComp[kindIdx], (airMopper)multiAnswerNix, airMopAlways);
  }
#undef GAGE_CTX_NEW

  for (kindIdx=0; kindIdx<KIND_NUM; kindIdx++) {
    NRRD_NEW(nin[kindIdx], mop);
  }
  NRRD_NEW(ngrad, mop);
  NRRD_NEW(nsclCopy, mop);
  if (engageGenTensor(gctx[KI_TEN], nin[KI_TEN],
                      /* out/in */
                      noiseStdv, 42 /* RNG seed, could be a command-line option */,
                      volSize[0], volSize[1], volSize[2])
      || engageGenScalar(gctx[KI_SCL], nin[KI_SCL],
                         gctxComp[KI_SCL], nsclCopy,
                         /* out/in */
                         nin[KI_TEN])
      || engageGenVector(gctx[KI_VEC], nin[KI_VEC], 
                         /* out/in */
                         nin[KI_SCL])
      /* engage'ing of nin[KI_DWI] done below */
      || genDwi(nin[KI_DWI], ngrad, 
                /* out/in */
                gradNum /* for B0 */, bval, nin[KI_TEN])
      || engageMopDiceVector(gctxComp[KI_VEC], nvecComp, 
                             /* out/in */
                             mop, nin[KI_VEC])
      || engageMopDiceTensor(gctxComp[KI_TEN], nctenComp, 
                             /* out/in */
                             mop, nin[KI_TEN])
      || engageMopDiceDwi(gctxComp[KI_DWI], &ndwiComp, 
                          /* out/in */
                          mop, nin[KI_DWI])) {
    airMopAdd(mop, err = biffGetDone(BKEY), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble creating volumes:\n%s", me, err);
    airMopError(mop); return 1;
  }
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
  
  /* make sure kinds can parse back to themselves */
  /* the messiness here is in part because of problems with the gage
     API, and the weirdness of how meetGageKindParse is either allocating
     something, or not, depending on its input.  There is no way to 
     refer to the "name" of a dwi kind without having allocated something. */
  for (kindIdx=0; kindIdx<KIND_NUM; kindIdx++) {
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
  /*
  for (kindIdx=0; kindIdx<KIND_NUM; kindIdx++) {
    printf("%s: %s (%s) -> (%u) %u\n", me, kind[kindIdx]->name,
           kind[kindIdx]->dynamicAlloc ? "dynamic" : "static",
           kind[kindIdx]->baseDim, kind[kindIdx]->valLen);
  }
  */
  
  /*
  nrrdSave("tmp-cten.nrrd", nin[KI_TEN], NULL);
  nrrdSave("tmp-scl.nrrd", nin[KI_SCL], NULL);
  nrrdSave("tmp-vec.nrrd", nin[KI_VEC], NULL);
  nrrdSave("tmp-dwi.nrrd", nin[KI_DWI], NULL);
  */

  /* ========================== TASK 1 */
  if (updateQueryKernelSetTask1(gctxComp, gctx, manComp, man, ksupport)) {
    airMopAdd(mop, err = biffGetDone(BKEY), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble updating contexts:\n%s", me, err);
    airMopError(mop); return 1;
  }
  if (probeTask1(gctxComp, gctx, manComp, man, probeNum, ksupport)) {
    airMopAdd(mop, err = biffGetDone(BKEY), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble with probing:\n%s", me, err);
    airMopError(mop); return 1;
  }
  
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

  airMopOkay(mop);
  return 0;
}
#undef NRRD_NEW
