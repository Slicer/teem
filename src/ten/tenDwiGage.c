/*
  Teem: Tools to process and visualize scientific data and images
  Copyright (C) 2006, 2005  Gordon Kindlmann
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

/* --------------------------------------------------------------------- */

char
_tenDwiGageStr[][AIR_STRLEN_SMALL] = {
  "(unknown tenDwiGage)",
  "all",
  "b0",
  "mdwi",
  "tlls",
  "tllserr",
  "tllserrlog",
  "tllslike",
  "twls",
  "twlserr",
  "twlserrlog",
  "twlslike",
  "tnls",
  "tnlserr",
  "tnlserrlog",
  "tnlslike",
  "tmle",
  "tmleerr",
  "tmleerrlog",
  "tmlelike",
  "t",
  "terr",
  "terrlog",
  "tlike",
  "c",
  "2qs",
  "2qserr",
};

int
_tenDwiGageVal[] = {
  tenDwiGageUnknown,
  tenDwiGageAll,
  tenDwiGageB0,
  tenDwiGageMeanDwiValue,
  tenDwiGageTensorLLS,
  tenDwiGageTensorLLSError,
  tenDwiGageTensorLLSErrorLog,
  tenDwiGageTensorLLSLikelihood,
  tenDwiGageTensorWLS,
  tenDwiGageTensorWLSError,
  tenDwiGageTensorWLSErrorLog,
  tenDwiGageTensorWLSLikelihood,
  tenDwiGageTensorNLS,
  tenDwiGageTensorNLSError,
  tenDwiGageTensorNLSErrorLog,
  tenDwiGageTensorNLSLikelihood,
  tenDwiGageTensorMLE,
  tenDwiGageTensorMLEError,
  tenDwiGageTensorMLEErrorLog,
  tenDwiGageTensorMLELikelihood,
  tenDwiGageTensor,
  tenDwiGageTensorError,
  tenDwiGageTensorErrorLog,
  tenDwiGageTensorLikelihood,
  tenDwiGageConfidence,
  tenDwiGage2TensorQSeg,
  tenDwiGage2TensorQSegError,
};

airEnum
_tenDwiGage = {
  "tenDwiGage",
  TEN_DWI_GAGE_ITEM_MAX+1,
  _tenDwiGageStr, _tenDwiGageVal,
  NULL,
  NULL, NULL,
  AIR_FALSE
};
airEnum *
tenDwiGage = &_tenDwiGage;

/* --------------------------------------------------------------------- */

gageItemEntry
_tenDwiGageTable[TEN_DWI_GAGE_ITEM_MAX+1] = {
  /* enum value                      len,deriv,  prereqs,                                                                    parent item, parent index, needData*/
  /* the number of values is learned at run time */
  {tenDwiGageAll,        6660 /* NOT! */,  0,  {-1, -1, -1, -1, -1, -1},                                                              -1,        -1,    AIR_TRUE},
  {tenDwiGageB0,                       1,  0,  {tenDwiGageAll, /* MAYBE NOT... */ -1, -1, -1, -1, -1},                     tenDwiGageAll,         0,    AIR_TRUE},
  {tenDwiGageMeanDwiValue,             1,  0,  {tenDwiGageAll, -1, -1, -1, -1, -1},                                                   -1,        -1,    AIR_TRUE},

  {tenDwiGageTensorLLS,                7,  0,  {tenDwiGageAll, tenDwiGageMeanDwiValue, -1, -1, -1, -1},                               -1,        -1,    AIR_TRUE},
  {tenDwiGageTensorLLSError,           1,  0,  {tenDwiGageTensorLLS, -1, -1, -1, -1, -1},                                             -1,        -1,    AIR_TRUE},
  {tenDwiGageTensorLLSErrorLog,        1,  0,  {tenDwiGageTensorLLS, -1, -1, -1, -1, -1},                                             -1,        -1,    AIR_TRUE},
  {tenDwiGageTensorLLSLikelihood,      1,  0,  {tenDwiGageTensorLLS, -1, -1, -1, -1, -1},                                             -1,        -1,    AIR_TRUE},

  {tenDwiGageTensorWLS,                7,  0,  {tenDwiGageAll, tenDwiGageMeanDwiValue, -1, -1, -1, -1},                               -1,        -1,    AIR_TRUE},
  {tenDwiGageTensorWLSError,           1,  0,  {tenDwiGageTensorWLS, -1, -1, -1, -1, -1},                                             -1,        -1,    AIR_TRUE},
  {tenDwiGageTensorWLSErrorLog,        1,  0,  {tenDwiGageTensorWLS, -1, -1, -1, -1, -1},                                             -1,        -1,    AIR_TRUE},
  {tenDwiGageTensorWLSLikelihood,      1,  0,  {tenDwiGageTensorWLS, -1, -1, -1, -1, -1},                                             -1,        -1,    AIR_TRUE},

  {tenDwiGageTensorNLS,                7,  0,  {tenDwiGageAll, tenDwiGageMeanDwiValue, -1, -1, -1, -1},                               -1,        -1,    AIR_TRUE},
  {tenDwiGageTensorNLSError,           1,  0,  {tenDwiGageTensorNLS, -1, -1, -1, -1, -1},                                             -1,        -1,    AIR_TRUE},
  {tenDwiGageTensorNLSErrorLog,        1,  0,  {tenDwiGageTensorNLS, -1, -1, -1, -1, -1},                                             -1,        -1,    AIR_TRUE},
  {tenDwiGageTensorNLSLikelihood,      1,  0,  {tenDwiGageTensorNLS, -1, -1, -1, -1, -1},                                             -1,        -1,    AIR_TRUE},

  {tenDwiGageTensorMLE,                7,  0,  {tenDwiGageAll, tenDwiGageMeanDwiValue, -1, -1, -1, -1},                               -1,        -1,    AIR_TRUE},
  {tenDwiGageTensorMLEError,           1,  0,  {tenDwiGageTensorMLE, -1, -1, -1, -1, -1},                                             -1,        -1,    AIR_TRUE},
  {tenDwiGageTensorMLEErrorLog,        1,  0,  {tenDwiGageTensorMLE, -1, -1, -1, -1, -1},                                             -1,        -1,    AIR_TRUE},
  {tenDwiGageTensorMLELikelihood,      1,  0,  {tenDwiGageTensorMLE, -1, -1, -1, -1, -1},                                             -1,        -1,    AIR_TRUE},

  /* these are NOT sub-items: they are copies, as controlled by the kind->data */
  {tenDwiGageTensor,                   7,  0,  {-1 /* NOT! */, -1, -1, -1, -1, -1},                                                   -1,        -1,    AIR_TRUE},
  {tenDwiGageTensorError,              1,  0,  {-1 /* NOT! */, -1, -1, -1, -1, -1},                                                   -1,        -1,    AIR_TRUE},
  {tenDwiGageTensorErrorLog,           1,  0,  {-1 /* NOT! */, -1, -1, -1, -1, -1},                                                   -1,        -1,    AIR_TRUE},
  {tenDwiGageTensorLikelihood,         1,  0,  {-1 /* NOT! */, -1, -1, -1, -1, -1},                                                   -1,        -1,    AIR_TRUE},

  {tenDwiGageConfidence,               1,  0,  {tenDwiGageTensor, -1, -1, -1, -1, -1},                                  tenDwiGageTensor,         0,    AIR_TRUE},

  {tenDwiGage2TensorQSeg,             14,  0,  {tenDwiGageTensor, -1, -1, -1, -1, -1},                                                -1,         0,    AIR_TRUE},
  {tenDwiGage2TensorQSegError,         1,  0,  {tenDwiGageAll, tenDwiGage2TensorQSeg, -1, -1, -1, -1},                                -1,         0,    AIR_TRUE}
};

void
_tenDwiGageIv3Print(FILE *file, gageContext *ctx, gagePerVolume *pvl) {
  char me[]="_tenDwiGageIv3Print";

  AIR_UNUSED(ctx);
  AIR_UNUSED(pvl);
  fprintf(file, "%s: sorry, unimplemented\n", me);
  return;
}

void
_tenDwiGageFilter(gageContext *ctx, gagePerVolume *pvl) {
  char me[]="_tenDwiGageFilter";
  double *fw00, *fw11, *fw22, *dwi;
  int fd;
  tenDwiGageKindData *kindData;
  unsigned int J, dwiNum;

  fd = 2*ctx->radius;
  dwi = pvl->directAnswer[tenDwiGageAll];
  kindData = AIR_CAST(tenDwiGageKindData *, pvl->kind->data);
  dwiNum = pvl->kind->valLen;
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
_tenDwiGageAnswer(gageContext *ctx, gagePerVolume *pvl) {
  char me[]="_tenDwiGageAnswer";
  unsigned int dwiIdx;
  tenDwiGageKindData *kindData;
  tenDwiGagePvlData *pvlData;
  double *dwiAll, dwiMean=0;

  kindData = AIR_CAST(tenDwiGageKindData *, pvl->kind->data);
  pvlData = AIR_CAST(tenDwiGagePvlData *, pvl->data);

  dwiAll = pvl->directAnswer[tenDwiGageAll];
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenDwiGageAll)) {
    /* done if doV */
    if (ctx->verbose) {
      for (dwiIdx=0; dwiIdx<pvl->kind->valLen; dwiIdx++) {
        fprintf(stderr, "%s: dwi[%u] = %g\n", me, dwiIdx, dwiAll[dwiIdx]);
      }
    }
  }
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenDwiGage2TensorQSeg)) {
    double *twoten;
    unsigned int valIdx, E;

    twoten = pvl->directAnswer[tenDwiGage2TensorQSeg];

    /* bogus: simulate a 2-tensor segmentation */
    for (valIdx=0; valIdx<pvl->kind->valLen; valIdx++) {
      pvlData->wght[valIdx] = valIdx % 2;
    }

    E = 0;
    for (valIdx=0; valIdx<pvl->kind->valLen; valIdx++) {
      if (!E) E |= tenEstimateSkipSet(kindData->tec, valIdx,
                                      pvlData->wght[valIdx]);
    }
    if (!E) E |= tenEstimateUpdate(kindData->tec);
    if (!E) E |= tenEstimate1TensorSingle_d(kindData->tec,
                                            twoten + 0, dwiAll);
    for (valIdx=0; valIdx<pvl->kind->valLen; valIdx++) {
      if (!E) E |= tenEstimateSkipSet(kindData->tec, valIdx,
                                      1 - pvlData->wght[valIdx]);
    }
    if (!E) E |= tenEstimateUpdate(kindData->tec);
    if (!E) E |= tenEstimate1TensorSingle_d(kindData->tec,
                                            twoten + 7, dwiAll);
    if (E) {
      fprintf(stderr, "!%s: %s\n", me, biffGetDone(TEN));
    }

    twoten[0] = 1.0;   /* confidence for two-tensor fit */
    twoten[7] = 0.5;   /* fraction that is the first tensor */
    /* twoten[1 .. 6] = first tensor */
    /* twoten[8 .. 13] = second tensor */
  }
  /*
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenDwiGageB0)) {
    done if doV
  }
  */
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenDwiGageMeanDwiValue)) {
    dwiMean = 0;
    for (dwiIdx=1; dwiIdx<pvl->kind->valLen; dwiIdx++) {
      dwiMean += dwiAll[dwiIdx];
    }
    dwiMean /= pvl->kind->valLen;
    pvl->directAnswer[tenDwiGageMeanDwiValue][0] = dwiMean;
  }
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenDwiGageTensorLLS)) {
    /*
      not compiling ...
    tenEstimateLinearSingle_d(pvl->directAnswer[tenDwiGageTensorLLS],
                              NULL, dwiAll,
                              AIR_CAST(double *, kindData->nemat->data),
                              pvlData->vbuf, pvl->kind->valLen,
                              AIR_TRUE, kindData->dwiConfThresh,
                              kindData->dwiConfSoft, kindData->bval);
    */
  }
  /*
  tenDwiGageTensorLinearFitError,
  tenDwiGageTensorNonLinearFit,
  tenDwiGageTensorNonLinearFitError,
  tenDwiGageTensor,
  tenDwiGageTensorError,
  tenDwiGageConfidence
  */
  return;
}

tenDwiGageKindData*
tenDwiGageKindDataNew() {
  tenDwiGageKindData *ret;

  ret = AIR_CAST(tenDwiGageKindData *, malloc(sizeof(tenDwiGageKindData)));
  if (ret) {
    ret->tec = tenEstimateContextNew();
    ret->ngrad = NULL;
  }
  return ret;
}

tenDwiGageKindData*
tenDwiGageKindDataNix(tenDwiGageKindData *kindData) {

  if (kindData) {
    tenEstimateContextNix(kindData->tec);
    nrrdNuke(kindData->ngrad);
    airFree(kindData);
  }
  return NULL;
}

int
tenDwiGageKindCheck(const gageKind *kind) {
  char me[]="tenDwiGageKindCheck", err[BIFF_STRLEN];
  
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
_tenDwiGageKindReadyCheck(const gageKind *kind) {
  char me[]="_tenDwiGageKindReadyCheck", err[BIFF_STRLEN];
  /* tenDwiGageKindData *kindData; */

  if (tenDwiGageKindCheck(kind)) {
    sprintf(err, "%s: didn't get valid kind", me);
    biffAdd(TEN, err); return 1;
  }
  
  /* HEY: isn't there more to do here ? */

  return 0;
}

void *
_tenDwiGagePvlDataNew(const gageKind *kind) {
  char me[]="_tenDwiGagePvlDataNew", err[BIFF_STRLEN];
  tenDwiGagePvlData *pvlData;
  tenDwiGageKindData *kindData;

  if (_tenDwiGageKindReadyCheck(kind)) {
    sprintf(err, "%s: kindData not ready for use", me);
    biffMove(GAGE, err, TEN); return NULL;
  }

  pvlData = AIR_CAST(tenDwiGagePvlData *,
                     malloc(sizeof(tenDwiGagePvlData)));
  if (pvlData) {
    kindData = AIR_CAST(tenDwiGageKindData *, kind->data);
    pvlData->vbuf = AIR_CAST(double *,
                             calloc(kind->valLen, sizeof(double)));
    pvlData->wght = AIR_CAST(unsigned int *,
                             calloc(kind->valLen, sizeof(unsigned int)));
  }
  return AIR_CAST(void*, pvlData);
}

void *
_tenDwiGagePvlDataCopy(const gageKind *kind, const void *pvlData) {

  AIR_UNUSED(pvlData);
  return _tenDwiGagePvlDataNew(kind);
}

void *
_tenDwiGagePvlDataNix(const gageKind *kind, void *_pvlData) {
  tenDwiGagePvlData *pvlData;

  AIR_UNUSED(kind);
  if (_pvlData) {
    pvlData = AIR_CAST(tenDwiGagePvlData *, _pvlData);
    airFree(pvlData->vbuf);
    airFree(pvlData->wght);
    airFree(pvlData);
  }
  return NULL;
}

gageKind
_tenDwiGageKind = {
  TEN_DWI_GAGE_KIND_NAME,
  &_tenDwiGage,
  1,
  6662 /* NOT: set by _tenDwiGageKindNumSet() */,
  TEN_DWI_GAGE_ITEM_MAX,
  NULL /* NOT: modified copy of _tenDwiGageTable,
          allocated by tenDwiGageKindNew(), and
          set by _tenDwiGageKindNumSet() */,
  _tenDwiGageIv3Print,
  _tenDwiGageFilter,
  _tenDwiGageAnswer,
  _tenDwiGagePvlDataNew,
  _tenDwiGagePvlDataCopy,
  _tenDwiGagePvlDataNix,
  NULL /* NOT: set by tenDwiGageKindNew() */
};

gageKind *
tenDwiGageKindNew() {
  gageKind *kind;

  kind = AIR_CAST(gageKind *, malloc(sizeof(gageKind)));
  memcpy(kind, &_tenDwiGageKind, sizeof(gageKind));

  kind->table = AIR_CAST(gageItemEntry *,
                         malloc(sizeof(_tenDwiGageTable)));
  memcpy(kind->table, _tenDwiGageTable, sizeof(_tenDwiGageTable));

  kind->data = AIR_CAST(void *, tenDwiGageKindDataNew());

  return kind;
}

gageKind *
tenDwiGageKindNix(gageKind *kind) {

  if (kind) {
    airFree(kind->table);
    tenDwiGageKindDataNix(AIR_CAST(tenDwiGageKindData *, kind->data));
    airFree(kind);
  }
  return NULL;
}

void
tenDwiGageKindNumSet(gageKind *kind, unsigned int num) {

  if (kind) {
    kind->valLen = num;
    kind->table[tenDwiGageAll].answerLength = num;
  }
}

