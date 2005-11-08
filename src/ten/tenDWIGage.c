/*
  Teem: Tools to process and visualize scientific data and images
  Copyright (C) 2005  Gordon Kindlmann
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

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "ten.h"
#include "privateTen.h"

char
_tenDWIGageFitTypeStr[][AIR_STRLEN_SMALL] = {
  "(unknown tenDWIGageFitType)",
  "linear",
  "nonlinear"
};

char
_tenDWIGageFitTypeDesc[][AIR_STRLEN_MED] = {
  "unknown tenDWIGageFitType",
  "linear least-squares fit of log(DWI)",
  "non-linear least-squares fit of DWI"
};

char
_tenDWIGageFitTypeStrEqv[][AIR_STRLEN_SMALL] = {
  "linear", "lin",
  "non-linear", "nonlinear",
    "nonlin", "nlin",
  ""
};

int
_tenDWIGageFitTypeValEqv[] = {
  tenDWIGageFitTypeLinear, tenDWIGageFitTypeLinear,
  tenDWIGageFitTypeNonLinear, tenDWIGageFitTypeNonLinear,
    tenDWIGageFitTypeNonLinear, tenDWIGageFitTypeNonLinear
};

airEnum
_tenDWIGageFitType = {
  "tenDWIGageFitType",
  TEN_DWI_GAGE_FIT_TYPE_MAX,
  _tenDWIGageFitTypeStr, NULL,
  _tenDWIGageFitTypeDesc,
  _tenDWIGageFitTypeStrEqv, _tenDWIGageFitTypeValEqv,
  AIR_FALSE
};
airEnum *
tenDWIGageFitType= &_tenDWIGageFitType;

/* --------------------------------------------------------------------- */

char
_tenDWIGageStr[][AIR_STRLEN_SMALL] = {
  "(unknown tenDWIGage)",
  "all",
  "b0",
  "mdwi",
  "tlin",
  "tlinerr",
  "tnlin",
  "tnlinerr",
  "t",
  "terr",
  "c"
};

int
_tenDWIGageVal[] = {
  tenDWIGageUnknown,
  tenDWIGageAll,
  tenDWIGageB0,
  tenDWIGageMeanDWIValue,
  tenDWIGageTensorLinearFit,
  tenDWIGageTensorLinearFitError,
  tenDWIGageTensorNonLinearFit,
  tenDWIGageTensorNonLinearFitError,
  tenDWIGageTensor,
  tenDWIGageTensorError,
  tenDWIGageConfidence
};

airEnum
_tenDWIGage = {
  "tenDWIGage",
  TEN_DWI_GAGE_ITEM_MAX+1,
  _tenDWIGageStr, _tenDWIGageVal,
  NULL,
  NULL, NULL,
  AIR_FALSE
};
airEnum *
tenDWIGage = &_tenDWIGage;

/* --------------------------------------------------------------------- */

int tenDefDWIGageFitType = tenDWIGageFitTypeLinear;

typedef struct {
  /* -------- input */
  double bval;          /* the scalar b value */
  /* NOTE: ngrad or bmat is used according to which one is non-NULL.
     For both, axis[1].size is ONE LESS THAN the number of image values, 
     because the first value is the B0 image */
  const Nrrd *ngrad,    /* list of gradients */
    *nbmat;             /* list of B matrices */
  double dwiConfThresh, /* threshold value of mean DWI for confidence mask */
    dwiConfSoft;
  int fitType;          /* from tenDWIGageFitType enum */
  /* -------- internal */
  unsigned int num;     /* number of total values (both baseline and DWI) */
  Nrrd *nemat;          /* estimation matrix for linear least squares */
} tenDWIGageKindData;

typedef struct {
  double *vbuf;        /* kindData->num doubles; used in linear fitting */
} tenDWIGagePvlData;

gageItemEntry
_tenDWIGageTable[TEN_DWI_GAGE_ITEM_MAX+1] = {
  /* enum value                      len,deriv,  prereqs,                                                                    parent item, parent index, needData*/
  /* the number of values is learned at run time */
  {tenDWIGageAll,         6660 /* NOT! */,  0,  {-1, -1, -1, -1, -1},                                                                  -1,        -1,    AIR_TRUE},
  {tenDWIGageB0,                       1,  0,  {tenDWIGageAll, -1, -1, -1, -1},                                            tenDWIGageAll,         0,    AIR_TRUE},
  {tenDWIGageMeanDWIValue,             1,  0,  {tenDWIGageAll, -1, -1, -1, -1},                                                       -1,        -1,    AIR_TRUE},
  {tenDWIGageTensorLinearFit,          7,  0,  {tenDWIGageAll, tenDWIGageMeanDWIValue, -1, -1, -1},                                   -1,        -1,    AIR_TRUE},
  {tenDWIGageTensorLinearFitError,     1,  0,  {tenDWIGageTensorLinearFit, -1, -1, -1, -1},                                           -1,        -1,    AIR_TRUE},
  {tenDWIGageTensorNonLinearFit,       7,  0,  {tenDWIGageAll, tenDWIGageMeanDWIValue, -1, -1, -1},                                   -1,        -1,    AIR_TRUE},
  {tenDWIGageTensorNonLinearFitError,  1,  0,  {tenDWIGageTensorNonLinearFit, -1, -1, -1, -1},                                        -1,        -1,    AIR_TRUE},
  /* these two are not sub-items: they are copies, as controlled by the kind->data */
  {tenDWIGageTensor,                   7,  0,  {-1 /* NOT! */, -1, -1, -1, -1},                                                       -1,        -1,    AIR_TRUE},
  {tenDWIGageTensorError,              1,  0,  {-1 /* NOT! */, -1, -1, -1, -1},                                                       -1,        -1,    AIR_TRUE},
  {tenDWIGageConfidence,               1,  0,  {tenDWIGageTensor, -1, -1, -1, -1},                                      tenDWIGageTensor,         0,    AIR_TRUE}
};

void
_tenDWIGageIv3Print(FILE *file, gageContext *ctx, gagePerVolume *pvl) {
  char me[]="_tenDWIGageIv3Print";

  AIR_UNUSED(ctx);
  AIR_UNUSED(pvl);
  fprintf(file, "%s: sorry, unimplemented\n", me);
  return;
}

void
_tenDWIGageFilter(gageContext *ctx, gagePerVolume *pvl) {
  char me[]="_tenDWIGageFilter";
  gage_t *fw00, *fw11, *fw22, *dwi;
  int fd;
  tenDWIGageKindData *kindData;
  unsigned int J, dwiNum;

  fd = 2*ctx->radius;
  dwi = pvl->directAnswer[tenDWIGageAll];
  kindData = AIR_CAST(tenDWIGageKindData *, pvl->kind->data);
  dwiNum = kindData->num;
  if (!ctx->parm.k3pack) {
    fprintf(stderr, "!%s: sorry, 6pack filtering not implemented\n", me);
    return;
  }
  fw00 = ctx->fw + fd*3*gageKernel00;
  fw11 = ctx->fw + fd*3*gageKernel11;
  fw22 = ctx->fw + fd*3*gageKernel22;
  /* HEY: these will have to be updated if there is ever any use for
     gradients in DWIs: the second-to-last argument would change */
  switch (fd) {
  case 2:
    for (J=0; J<dwiNum; J++) {
      gageScl3PFilter2(pvl->iv3 + J*8, pvl->iv2 + J*4, pvl->iv1 + J*2,
                       fw00, fw11, fw22,
                       dwi + J, NULL, NULL,
                       pvl->needD[0], AIR_FALSE, AIR_FALSE);
    }
    break;
  case 4:
    for (J=0; J<dwiNum; J++) {
      gageScl3PFilter4(pvl->iv3 + J*64, pvl->iv2 + J*16, pvl->iv1 + J*4,
                       fw00, fw11, fw22,
                       dwi + J, NULL, NULL,
                       pvl->needD[0], AIR_FALSE, AIR_FALSE);
    }
    break;
  default:
    for (J=0; J<dwiNum; J++) {
      gageScl3PFilterN(fd, pvl->iv3 + J*fd*fd*fd,
                       pvl->iv2 + J*fd*fd, pvl->iv1 + J*fd,
                       fw00, fw11, fw22,
                       dwi + J, NULL, NULL,
                       pvl->needD[0], AIR_FALSE, AIR_FALSE);
    }
    break;
  }

  return;
}

void
_tenDWIGageAnswer(gageContext *ctx, gagePerVolume *pvl) {
  char me[]="_tenDWIGageAnswer";
  unsigned int dwiIdx;
  tenDWIGageKindData *kindData;
  tenDWIGagePvlData *pvlData;
  gage_t *dwiAll, dwiMean=0;

  kindData = AIR_CAST(tenDWIGageKindData *, pvl->kind->data);
  pvlData = AIR_CAST(tenDWIGagePvlData *, pvl->data);

  dwiAll = pvl->directAnswer[tenDWIGageAll];
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenDWIGageAll)) {
    /* done if doV */
    if (ctx->verbose) {
      for (dwiIdx=0; dwiIdx<kindData->num; dwiIdx++) {
        fprintf(stderr, "%s: dwi[%u] = %g\n", me, dwiIdx, dwiAll[dwiIdx]);
      }
    }
  }
  /*
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenDWIGageB0)) {
    done if doV
  }
  */
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenDWIGageMeanDWIValue)) {
    dwiMean = 0;
    for (dwiIdx=1; dwiIdx<kindData->num; dwiIdx++) {
      dwiMean += dwiAll[dwiIdx];
    }
    dwiMean /= 1.0f/(kindData->num - 1);
    pvl->directAnswer[tenDWIGageMeanDWIValue][0] = dwiMean;
  }
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenDWIGageTensorLinearFit)) {
#if GAGE_TYPE_FLOAT
    tenEstimateLinearSingle_f
#else
    tenEstimateLinearSingle_d
#endif    
      (pvl->directAnswer[tenDWIGageTensorLinearFit], NULL,
       dwiAll, AIR_CAST(double *, kindData->nemat->data),
       pvlData->vbuf, kindData->num,
       AIR_TRUE, kindData->dwiConfThresh,
       kindData->dwiConfSoft, kindData->bval);
  }
  /*
  tenDWIGageTensorLinearFitError,
  tenDWIGageTensorNonLinearFit,
  tenDWIGageTensorNonLinearFitError,
  tenDWIGageTensor,
  tenDWIGageTensorError,
  tenDWIGageConfidence
  */
  return;
}

tenDWIGageKindData*
tenDWIGageKindDataNew() {
  tenDWIGageKindData *ret;

  ret = AIR_CAST(tenDWIGageKindData *, malloc(sizeof(tenDWIGageKindData)));
  if (ret) {
    ret->bval = AIR_NAN;
    ret->ngrad = NULL;
    ret->nbmat = NULL;
    ret->dwiConfThresh = 0;
    ret->dwiConfSoft = 0;
    ret->fitType = tenDefDWIGageFitType;
    ret->num = 6661 /* NOT: set by _tenDWIGageKindNumSet() */;
    ret->nemat = nrrdNew();
  }
  return ret;
}

tenDWIGageKindData*
tenDWIGageKindDataNix(tenDWIGageKindData *kindData) {

  if (kindData) {
    nrrdNuke(kindData->nemat);
    airFree(kindData);
  }
  return NULL;
}

int
tenDWIGageKindCheck(const gageKind *kind) {
  char me[]="tenDWIGageKindCheck", err[AIR_STRLEN_MED];

  if (!kind) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(TEN, err); return 1;
  }
  if (strcmp(kind->name, TEN_DWI_GAGE_KIND_NAME)) {
    sprintf(err, "%s: got \"%s\" kind, not \"%s\"", me,
            kind->name, TEN_DWI_GAGE_KIND_NAME);
    biffAdd(TEN, err); return 1;
  }
  if (!kind->data) {
    sprintf(err, "%s: kind->data is NULL", me);
    biffAdd(TEN, err); return 1;
  }
  return 0;
}

int
_tenDWIGageKindReadyCheck(const gageKind *kind) {
  char me[]="_tenDWIGageKindReadyCheck", err[AIR_STRLEN_MED];
  tenDWIGageKindData *kindData;

  if (tenDWIGageKindCheck(kind)) {
    sprintf(err, "%s: didn't get valid kind", me);
    biffAdd(TEN, err); return 1;
  }
  kindData = AIR_CAST(tenDWIGageKindData *, kind->data);
  if (!AIR_EXISTS(kindData->bval)) {
    sprintf(err, "%s: bval doesn't exist", me);
    biffAdd(TEN, err); return 1;
  }
  if (!( !!(kindData->ngrad) ^ !!(kindData->nbmat) )) {
    sprintf(err, "%s: not one of ngrad or nbmat set", me);
    biffAdd(TEN, err); return 1;
  }
  if (!( AIR_EXISTS(kindData->dwiConfThresh) 
         && AIR_EXISTS(kindData->dwiConfSoft) )) {
    sprintf(err, "%s: thresh and soft not set", me);
    biffAdd(TEN, err); return 1;
  }
  if (airEnumValCheck(tenDWIGageFitType, kindData->fitType)) {
    sprintf(err, "%s: fitType %d not valid", me, kindData->fitType);
    biffAdd(TEN, err); return 1;
  }
  return 0;
}

void *
_tenDWIGagePvlDataNew(const gageKind *kind) {
  char me[]="_tenDWIGagePvlDataNew", err[AIR_STRLEN_MED];
  tenDWIGagePvlData *pvlData;
  tenDWIGageKindData *kindData;

  if (_tenDWIGageKindReadyCheck(kind)) {
    sprintf(err, "%s: kindData not ready for use", me);
    biffMove(GAGE, err, TEN); return NULL;
  }

  pvlData = AIR_CAST(tenDWIGagePvlData *,
                     malloc(sizeof(tenDWIGagePvlData)));
  if (pvlData) {
    kindData = AIR_CAST(tenDWIGageKindData *, kind->data);
    pvlData->vbuf = AIR_CAST(double *,
                             calloc(kindData->num, sizeof(double)));
  }
  return AIR_CAST(void*, pvlData);
}

void *
_tenDWIGagePvlDataCopy(const gageKind *kind, const void *pvlData) {

  AIR_UNUSED(pvlData);
  return _tenDWIGagePvlDataNew(kind);
}

void *
_tenDWIGagePvlDataNix(const gageKind *kind, void *_pvlData) {
  tenDWIGagePvlData *pvlData;

  AIR_UNUSED(kind);
  if (_pvlData) {
    pvlData = AIR_CAST(tenDWIGagePvlData *, _pvlData);
    airFree(pvlData->vbuf);
    airFree(pvlData);
  }
  return NULL;
}

gageKind
_tenDWIGageKind = {
  TEN_DWI_GAGE_KIND_NAME,
  &_tenDWIGage,
  1,
  6662 /* NOT: set by _tenDWIGageKindNumSet() */,
  TEN_DWI_GAGE_ITEM_MAX,
  NULL /* NOT: modified copy of _tenDWIGageTable,
          allocated by tenDWIGageKindNew(), and
          set by _tenDWIGageKindNumSet() */,
  _tenDWIGageIv3Print,
  _tenDWIGageFilter,
  _tenDWIGageAnswer,
  _tenDWIGagePvlDataNew,
  _tenDWIGagePvlDataCopy,
  _tenDWIGagePvlDataNix,
  NULL /* NOT: set by tenDWIGageKindNew() */
};

gageKind *
tenDWIGageKindNew() {
  gageKind *kind;

  kind = AIR_CAST(gageKind *, malloc(sizeof(gageKind)));
  memcpy(kind, &_tenDWIGageKind, sizeof(gageKind));

  kind->table = AIR_CAST(gageItemEntry *,
                         malloc(sizeof(_tenDWIGageTable)));
  memcpy(kind->table, _tenDWIGageTable, sizeof(_tenDWIGageTable));

  kind->data = AIR_CAST(void *, tenDWIGageKindDataNew());

  return kind;
}

gageKind *
tenDWIGageKindNix(gageKind *kind) {

  if (kind) {
    airFree(kind->table);
    tenDWIGageKindDataNix(AIR_CAST(tenDWIGageKindData *, kind->data));
    airFree(kind);
  }
  return NULL;
}

void
_tenDWIGageKindNumSet(gageKind *kind, unsigned int num) {
  char me[]="_tenDWIGageKindNumSet";

  kind->valLen = num;
  fprintf(stderr, "%s: table[%u].answerLength = %u\n", me, 
          tenDWIGageAll, num);
  kind->table[tenDWIGageAll].answerLength = num;
  AIR_CAST(tenDWIGageKindData *, kind->data)->num = num;
  return;
}

int
tenDWIGageKindGradients(gageKind *kind, double bval, const Nrrd *ngrad) {
  char me[]="tenDWIGageKindGradients", err[AIR_STRLEN_MED];
  tenDWIGageKindData *kindData;
  Nrrd *nbmat;
  airArray *mop;

  if (tenDWIGageKindCheck(kind)) {
    sprintf(err, "%s: trouble with given kind", me);
    biffAdd(TEN, err); return 1;
  }
  if (!AIR_EXISTS(bval)) {
    sprintf(err, "%s: got non-existent bval", me);
    biffAdd(TEN, err); return 1;
  }
  if (tenGradientCheck(ngrad, nrrdTypeDefault, 6)) {
    sprintf(err, "%s: problem with gradient list", me);
    biffAdd(TEN, err); return 1;
  }

  kindData = AIR_CAST(tenDWIGageKindData *, kind->data);
  kindData->bval = bval;
  kindData->ngrad = ngrad;
  kindData->nbmat = NULL;
  _tenDWIGageKindNumSet(kind, 1 + AIR_CAST(unsigned int, ngrad->axis[1].size));
  mop = airMopNew();
  nbmat = nrrdNew();
  airMopAdd(mop, nbmat, (airMopper)nrrdNuke, airMopAlways);
  if (tenBMatrixCalc(nbmat, kindData->ngrad)
      || tenEMatrixCalc(kindData->nemat, nbmat, AIR_TRUE)) {
    sprintf(err, "%s: trouble creating or inverting B-matrix", me);
    biffAdd(TEN, err); airMopError(mop); return 1;
  }
  airMopOkay(mop);
  
  return 0;
}

int
tenDWIGageKindBMatrices(gageKind *kind, double bval, const Nrrd *nbmat) {
  char me[]="tenDWIGageKindBMatrices", err[AIR_STRLEN_MED];
  tenDWIGageKindData *kindData;

  if (tenDWIGageKindCheck(kind)) {
    sprintf(err, "%s: trouble with given kind", me);
    biffAdd(TEN, err); return 1;
  }
  if (!AIR_EXISTS(bval)) {
    sprintf(err, "%s: got non-existent bval", me);
    biffAdd(TEN, err); return 1;
  }
  if (tenBMatrixCheck(nbmat, 6)) {
    sprintf(err, "%s: problem with b-matrix list", me);
    biffAdd(TEN, err); return 1;
  }

  kindData = AIR_CAST(tenDWIGageKindData *, kind->data);
  kindData->bval = bval;
  kindData->ngrad = NULL;
  kindData->nbmat = nbmat;
  _tenDWIGageKindNumSet(kind, 1 + AIR_CAST(unsigned int, nbmat->axis[1].size));
  if (tenEMatrixCalc(kindData->nemat, kindData->nbmat, AIR_TRUE)) {
    sprintf(err, "%s: trouble inverting B-matrix", me);
    biffAdd(TEN, err); return 1;
  }

  return 0;
}

int
tenDWIGageKindConfThreshold(gageKind *kind, double thresh, double soft) {
  char me[]="tenDWIGageKindConfThreshold", err[AIR_STRLEN_MED];
  tenDWIGageKindData *kindData;

  if (tenDWIGageKindCheck(kind)) {
    sprintf(err, "%s: trouble with given kind", me);
    biffAdd(TEN, err); return 1;
  }
  if (!( AIR_EXISTS(thresh) && AIR_EXISTS(soft) )) {
    sprintf(err, "%s: got non-existent thresh or soft", me);
    biffAdd(TEN, err); return 1;
  }

  kindData = AIR_CAST(tenDWIGageKindData *, kind->data);
  kindData->dwiConfThresh = thresh;
  kindData->dwiConfSoft = soft;

  return 0;
}

int
tenDWIGageKindFitType(gageKind *kind, int fitType) {
  char me[]="tenDWIGageKindFitType", err[AIR_STRLEN_MED];

  if (tenDWIGageKindCheck(kind)) {
    sprintf(err, "%s: trouble with given kind", me);
    biffAdd(TEN, err); return 1;
  }
  if (airEnumValCheck(tenDWIGageFitType, fitType)) {
    sprintf(err, "%s: %d not a valid %s", me, fitType, 
            tenDWIGageFitType->name);
    biffAdd(TEN, err); return 1;
  }

  switch(fitType) {
  case tenDWIGageFitTypeLinear:
    kind->table[tenDWIGageTensor].prereq[0] = 
      tenDWIGageTensorLinearFit;
    kind->table[tenDWIGageTensorError].prereq[0] = 
      tenDWIGageTensorLinearFitError;
    break;
  case tenDWIGageFitTypeNonLinear:
    kind->table[tenDWIGageTensor].prereq[0] = 
      tenDWIGageTensorNonLinearFit;
    kind->table[tenDWIGageTensorError].prereq[0] = 
      tenDWIGageTensorNonLinearFitError;
    break;
  default:
    sprintf(err, "%s: fitType %d not implemented!", me, fitType);
    biffAdd(TEN, err); return 1;
    break;
  }

  return 0;
}
