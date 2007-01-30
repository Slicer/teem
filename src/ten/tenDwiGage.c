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
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307	USA
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
  "2qsnerr",
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
  tenDwiGage2TensorQSegAndError,
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
  /* enum value                    len,deriv, prereqs,                           parent item, parent index, needData */
  {tenDwiGageUnknown,                0,  0,  {0},                                                  0,  0, AIR_TRUE},
  /* len == 0 for tenDwiGageAll means "learn later at run-time" */
  {tenDwiGageAll,                    0,  0,  {0},                                                  0,  0, AIR_TRUE},
  {tenDwiGageB0,                     1,  0,  {tenDwiGageAll},                          tenDwiGageAll,  0, AIR_TRUE},
  {tenDwiGageMeanDwiValue,           1,  0,  {tenDwiGageAll},                                      0,  0, AIR_TRUE},

  {tenDwiGageTensorLLS,              7,  0,  {tenDwiGageAll, tenDwiGageMeanDwiValue},              0,  0, AIR_TRUE},
  {tenDwiGageTensorLLSError,         1,  0,  {tenDwiGageTensorLLS},                                0,  0, AIR_TRUE},
  {tenDwiGageTensorLLSErrorLog,      1,  0,  {tenDwiGageTensorLLS},                                0,  0, AIR_TRUE},
  {tenDwiGageTensorLLSLikelihood,    1,  0,  {tenDwiGageTensorLLS},                                0,  0, AIR_TRUE},

  {tenDwiGageTensorWLS,              7,  0,  {tenDwiGageAll, tenDwiGageMeanDwiValue},              0,  0, AIR_TRUE},
  {tenDwiGageTensorWLSError,         1,  0,  {tenDwiGageTensorWLS},                                0,  0, AIR_TRUE},
  {tenDwiGageTensorWLSErrorLog,      1,  0,  {tenDwiGageTensorWLS},                                0,  0, AIR_TRUE},
  {tenDwiGageTensorWLSLikelihood,    1,  0,  {tenDwiGageTensorWLS},                                0,  0, AIR_TRUE},

  {tenDwiGageTensorNLS,              7,  0,  {tenDwiGageAll, tenDwiGageMeanDwiValue},              0,  0, AIR_TRUE},
  {tenDwiGageTensorNLSError,         1,  0,  {tenDwiGageTensorNLS},                                0,  0, AIR_TRUE},
  {tenDwiGageTensorNLSErrorLog,      1,  0,  {tenDwiGageTensorNLS},                                0,  0, AIR_TRUE},
  {tenDwiGageTensorNLSLikelihood,    1,  0,  {tenDwiGageTensorNLS},                                0,  0, AIR_TRUE},

  {tenDwiGageTensorMLE,              7,  0,  {tenDwiGageAll, tenDwiGageMeanDwiValue},              0,  0, AIR_TRUE},
  {tenDwiGageTensorMLEError,         1,  0,  {tenDwiGageTensorMLE},                                0,  0, AIR_TRUE},
  {tenDwiGageTensorMLEErrorLog,      1,  0,  {tenDwiGageTensorMLE},                                0,  0, AIR_TRUE},
  {tenDwiGageTensorMLELikelihood,    1,  0,  {tenDwiGageTensorMLE},                                0,  0, AIR_TRUE},

  /* these are NOT sub-items: they are copies, as controlled by the
     kind->data, but not the query: the query can't capture the kind
     of dependency implemented by having a dynamic kind */
  {tenDwiGageTensor,                 7,  0,  {0}, /* -1 == "learn later at run time" */           0,  0, AIR_TRUE},
  {tenDwiGageTensorError,            1,  0,  {tenDwiGageTensor},                                   0,  0, AIR_TRUE},
  {tenDwiGageTensorErrorLog,         1,  0,  {tenDwiGageTensor},                                   0,  0, AIR_TRUE},
  {tenDwiGageTensorLikelihood,       1,  0,  {tenDwiGageTensor},                                   0,  0, AIR_TRUE},
  {tenDwiGageConfidence,             1,  0,  {tenDwiGageTensor},                    tenDwiGageTensor,  0, AIR_TRUE},

  /* it actually doesn't make sense for tenDwiGage2TensorQSegAndError to be the parent,
     because of the situations where you want the q-seg result, but don't care about error */
  {tenDwiGage2TensorQSeg,           14,  0,  {tenDwiGageAll},                                     0,   0, AIR_TRUE},
  {tenDwiGage2TensorQSegError,       1,  0,  {tenDwiGageAll, tenDwiGage2TensorQSeg},              0,   0, AIR_TRUE},
  {tenDwiGage2TensorQSegAndError,   15,  0,  {tenDwiGage2TensorQSeg, tenDwiGage2TensorQSegError}, 0,   0, AIR_TRUE}
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
     derivatives in DWIs: the second-to-last argument would change */
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
  double *dwiAll, dwiMean=0, tentmp[7];

  kindData = AIR_CAST(tenDwiGageKindData *, pvl->kind->data);
  pvlData = AIR_CAST(tenDwiGagePvlData *, pvl->data);

  dwiAll = pvl->directAnswer[tenDwiGageAll];
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenDwiGageAll)) {
    /* done if doV */
    if (ctx->verbose) {
      for (dwiIdx=0; dwiIdx<pvl->kind->valLen; dwiIdx++) {
        fprintf(stderr, "%s(%d+%g,%d+%g,%d+%g): dwi[%u] = %g\n", me,
                ctx->point.xi, ctx->point.xf,
                ctx->point.yi, ctx->point.yf,
                ctx->point.zi, ctx->point.zf,
                dwiIdx, dwiAll[dwiIdx]);
      }
      fprintf(stderr, "%s: type(ngrad) = %d = %s\n", me,
              kindData->ngrad->type,
              airEnumStr(nrrdType, kindData->ngrad->type));
    }
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

  /* note: the gage interface to tenEstimate functionality 
     allows you exactly one kind of tensor estimation (per kind),
     so the function call to do the estimation is actually
     repeated over and over again; the copy into the answer
     buffer is what changes... */
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenDwiGageTensorLLS)) {
    tenEstimate1TensorSingle_d(pvlData->tec1, tentmp, dwiAll);
    TEN_T_COPY(pvl->directAnswer[tenDwiGageTensorLLS], tentmp);
  }
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenDwiGageTensorWLS)) {
    tenEstimate1TensorSingle_d(pvlData->tec1, tentmp, dwiAll);
    TEN_T_COPY(pvl->directAnswer[tenDwiGageTensorWLS], tentmp);
  }
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenDwiGageTensorNLS)) {
    tenEstimate1TensorSingle_d(pvlData->tec1, tentmp, dwiAll);
    TEN_T_COPY(pvl->directAnswer[tenDwiGageTensorNLS], tentmp);
  }
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenDwiGageTensorMLE)) {
    tenEstimate1TensorSingle_d(pvlData->tec1, tentmp, dwiAll);
    TEN_T_COPY(pvl->directAnswer[tenDwiGageTensorMLE], tentmp);
  }
  /* HEY: have to implement all the different kinds of errors */

  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenDwiGageTensor)) {
    /* pretty sneaky */
    gageItemEntry *item;
    item = pvl->kind->table + tenDwiGageTensor;
    TEN_T_COPY(pvl->directAnswer[tenDwiGageTensor],
               pvl->directAnswer[item->prereq[0]]);
  }
  /* HEY: have to copy all the different kinds of errors */

  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenDwiGage2TensorQSeg)) {
    const double *grads;
    int gradcount;
    double *twoten;
    unsigned int valIdx, E;
    
    twoten = pvl->directAnswer[tenDwiGage2TensorQSeg];
    
    gradcount = pvl->kind->valLen -1; /* Dont count b0 */
    grads = ((const double*) kindData->ngrad->data) +3; /* Ignore b0 grad */
    if (dwiAll[0] != 0) { /*  S0 = 0 */
      qball( pvlData->tec2->bValue, gradcount, dwiAll, grads, pvlData->qvals );
      qvals2points( gradcount, pvlData->qvals, grads, pvlData->qpoints );
      segsamp2( gradcount, pvlData->qvals, grads, pvlData->qpoints,
                pvlData->wght + 1, pvlData->dists );
    } else {
      /* stupid; should really return right here since data is garbage */
      for (valIdx=1; valIdx < AIR_CAST(unsigned int, gradcount+1); valIdx++) {
        pvlData->wght[valIdx] = valIdx % 2;
      }
    }
    
    E = 0;
    for (valIdx=1; valIdx<pvl->kind->valLen; valIdx++) {
      if (!E) E |= tenEstimateSkipSet(pvlData->tec2, valIdx,
                                      pvlData->wght[valIdx]);
    }
    if (!E) E |= tenEstimateUpdate(pvlData->tec2);
    if (!E) E |= tenEstimate1TensorSingle_d(pvlData->tec2,
                                            twoten + 0, dwiAll);
    for (valIdx=1; valIdx<pvl->kind->valLen; valIdx++) {
      if (!E) E |= tenEstimateSkipSet(pvlData->tec2, valIdx,
                                      1 - pvlData->wght[valIdx]);
    }
    if (!E) E |= tenEstimateUpdate(pvlData->tec2);
    if (!E) E |= tenEstimate1TensorSingle_d(pvlData->tec2,
                                            twoten + 7, dwiAll);
    if (E) {
      fprintf(stderr, "!%s: (trouble) %s\n", me, biffGetDone(TEN));
    }
    
    /* hack: confidence for two-tensor fit */
    twoten[0] = (twoten[0] + twoten[7])/2;
    twoten[7] = 0.5; /* fraction that is the first tensor (initial value) */
    /* twoten[1 .. 6] = first tensor */
    /* twoten[8 .. 13] = second tensor */
    
    /* Compute fraction between tensors if not garbage in this voxel */
    if (twoten[0] > 0.5) {
      double exp0,exp1,d,e=0,g=0, a=0,b=0;
      int i;
      
      for( i=0; i < gradcount; i++ ) {
        exp0 = exp(-pvlData->tec2->bValue * TEN_T3V_CONTR(twoten + 0,
                                                          grads + 3*i));
        exp1 = exp(-pvlData->tec2->bValue * TEN_T3V_CONTR(twoten + 7,
                                                          grads + 3*i));
        
        d = dwiAll[i+1] / dwiAll[0];
        e = exp0 - exp1;
        g = d - exp1;
        
        a += .5*e*e;
        b += e*g;
      }
      
      twoten[7] = AIR_CLAMP(0, 0.5*(b/a), 1);
    }
  }
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenDwiGage2TensorQSegError)) {
    const double *grads;
    int gradcount;
    double *twoten, d;
    int i;
    
    /* HEY: should switch to tenEstimate-based DWI simulation */
    if (dwiAll[0] != 0) { /* S0 = 0 */
      twoten = pvl->directAnswer[tenDwiGage2TensorQSeg];
      gradcount = pvl->kind->valLen -1; /* Dont count b0 */
      grads = ((const double*) kindData->ngrad->data) +3; /* Ignore b0 grad */
      
      pvl->directAnswer[tenDwiGage2TensorQSegError][0] = 0;
      for( i=0; i < gradcount; i++ ) {
        d = twoten[7]*exp(-pvlData->tec2->bValue * TEN_T3V_CONTR(twoten + 0,
                                                                 grads + 3*i));
        d += (1 - twoten[7])*exp(-pvlData->tec2->bValue 
                                 *TEN_T3V_CONTR(twoten + 7, grads + 3*i));
        d = dwiAll[i+1]/dwiAll[0] - d;
        pvl->directAnswer[tenDwiGage2TensorQSegError][0] += d*d;
      }
      pvl->directAnswer[tenDwiGage2TensorQSegError][0] = 
        sqrt( pvl->directAnswer[tenDwiGage2TensorQSegError][0] );
    } else {
      /* HEY: COMPLETELY WRONG!! An error is not defined! */
      pvl->directAnswer[tenDwiGage2TensorQSegError][0] = 0;
    }
    /* printf("%f\n",pvl->directAnswer[tenDwiGage2TensorQSegError][0]); */
  }
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenDwiGage2TensorQSegAndError)) {
    double *twoten, *err, *twotenerr;
    
    twoten = pvl->directAnswer[tenDwiGage2TensorQSeg];
    err = pvl->directAnswer[tenDwiGage2TensorQSegError];
    twotenerr = pvl->directAnswer[tenDwiGage2TensorQSegAndError];
    TEN_T_COPY(twotenerr + 0, twoten + 0);
    TEN_T_COPY(twotenerr + 7, twoten + 7);
    twotenerr[14] = err[0];
  }
  return;
}

/* --------------------- pvlData */

/* note use of the GAGE biff key */
void *
_tenDwiGagePvlDataNew(const gageKind *kind) {
  char me[]="_tenDwiGagePvlDataNew", err[BIFF_STRLEN];
  tenDwiGagePvlData *pvlData;
  tenDwiGageKindData *kindData;
  const int segcount = 2;
  unsigned int num;
  int E;
  
  if (tenDwiGageKindCheck(kind)) {
    sprintf(err, "%s: kindData not ready for use", me);
    biffMove(GAGE, err, TEN); return NULL;
  }
  kindData = AIR_CAST(tenDwiGageKindData *, kind->data);
  
  pvlData = AIR_CAST(tenDwiGagePvlData *,
                     malloc(sizeof(tenDwiGagePvlData)));
  if (!pvlData) {
    sprintf(err, "%s: couldn't allocate pvl data!", me);
    biffAdd(GAGE, err); return NULL;
  }
  pvlData->tec1 = tenEstimateContextNew();
  pvlData->tec2 = tenEstimateContextNew();
  for (num=1; num<=2; num++) {
    tenEstimateContext *tec;
    tec = (1 == num ? pvlData->tec1 : pvlData->tec2);
    E = 0;
    if (!E) tenEstimateVerboseSet(tec, 0);
    if (!E) tenEstimateNegEvalShiftSet(tec, AIR_TRUE);
    if (!E) E |= tenEstimateMethodSet(tec, 1 == num
                                      ? kindData->est1Method
                                      : kindData->est2Method);
    if (!E) E |= tenEstimateValueMinSet(tec, kindData->valueMin);
    if (kindData->ngrad->data) {
      if (!E) E |= tenEstimateGradientsSet(tec, kindData->ngrad,
                                           kindData->bval, AIR_FALSE);
    } else {
      if (!E) E |= tenEstimateBMatricesSet(tec, kindData->nbmat,
                                           kindData->bval, AIR_FALSE);
    }
    if (!E) E |= tenEstimateThresholdSet(tec, 
                                         kindData->thresh, kindData->soft);
    if (!E) E |= tenEstimateUpdate(tec);
    if (E) {
      fprintf(stderr, "%s: trouble setting %u estimation", me, num);
      biffMove(GAGE, err, TEN); return NULL;
    }
  }
  pvlData->vbuf = AIR_CAST(double *,
                           calloc(kind->valLen, sizeof(double)));
  pvlData->wght = AIR_CAST(unsigned int *,
                           calloc(kind->valLen, sizeof(unsigned int)));
  /* HEY: this is where we act on the the assumption about
     having val[0] be T2 baseline and all subsequent val[i] be DWIs */
  pvlData->wght[0] = 1;
  pvlData->qvals = AIR_CAST(double *,
                            calloc(kind->valLen-1, sizeof(double)));
  pvlData->qpoints = AIR_CAST(double *,
                              calloc(kind->valLen-1,  3*sizeof(double)));
  pvlData->dists = AIR_CAST(double *,
                            calloc(segcount*(kind->valLen-1), 
                                   sizeof(double)));
  pvlData->weights = AIR_CAST(double *,
                              calloc(segcount*(kind->valLen-1),
                                     sizeof(double)));
  return AIR_CAST(void *, pvlData);
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
  pvlData = AIR_CAST(tenDwiGagePvlData *, _pvlData);
  if (pvlData) {
    tenEstimateContextNix(pvlData->tec1);
    tenEstimateContextNix(pvlData->tec2);
    airFree(pvlData->vbuf);
    airFree(pvlData->wght);
    airFree(pvlData->qvals);
    airFree(pvlData->qpoints);
    airFree(pvlData->dists);
    airFree(pvlData->weights);
    airFree(pvlData);
  }
  return NULL;
}

/* --------------------- kindData */

tenDwiGageKindData*
tenDwiGageKindDataNew() {
  tenDwiGageKindData *ret;
  
  ret = AIR_CAST(tenDwiGageKindData *, malloc(sizeof(tenDwiGageKindData)));
  if (ret) {
    /* it may be that only one of these is actually filled */
    ret->ngrad = nrrdNew();
    ret->nbmat = nrrdNew();
    ret->thresh = ret->soft = ret->bval = AIR_NAN;
    ret->est1Method = tenEstimate1MethodUnknown;
    ret->est2Method = tenEstimate2MethodUnknown;
  }
  return ret;
}

tenDwiGageKindData*
tenDwiGageKindDataNix(tenDwiGageKindData *kindData) {
  
  if (kindData) {
    nrrdNuke(kindData->ngrad);
    nrrdNuke(kindData->nbmat);
    airFree(kindData);
  }
  return NULL;
}

/* --------------------- dwiKind, and dwiKind->data setting*/

/*
** Because this kind has to be dynamically allocated,
** this is not the kind, but just the template for it
*/
gageKind
_tenDwiGageKindTmpl = {
  AIR_TRUE, /* dynamically allocated */
  TEN_DWI_GAGE_KIND_NAME,
  &_tenDwiGage,
  1,
  0, /* NOT: set later by tenDwiGageKindSet() */
  TEN_DWI_GAGE_ITEM_MAX,
  NULL, /* NOT: modified copy of _tenDwiGageTable,
           allocated by tenDwiGageKindNew(), and
           set by _tenDwiGageKindSet() */
  _tenDwiGageIv3Print,
  _tenDwiGageFilter,
  _tenDwiGageAnswer,
  _tenDwiGagePvlDataNew,
  _tenDwiGagePvlDataCopy,
  _tenDwiGagePvlDataNix,
  NULL /* NOT: allocated by tenDwiGageKindNew(),
          insides set by tenDwiGageKindSet() */
};

gageKind *
tenDwiGageKindNew() {
  gageKind *kind;
  
  kind = AIR_CAST(gageKind *, malloc(sizeof(gageKind)));
  if (kind) {
    memcpy(kind, &_tenDwiGageKindTmpl, sizeof(gageKind));
    kind->valLen = 0; /* still has to be set later */
    kind->table = AIR_CAST(gageItemEntry *,
                           malloc(sizeof(_tenDwiGageTable)));
    memcpy(kind->table, _tenDwiGageTable, sizeof(_tenDwiGageTable));
    kind->data = AIR_CAST(void *, tenDwiGageKindDataNew());
  }
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

/*
** NOTE: this sets information in both the kind and kindData
*/
int
tenDwiGageKindSet(gageKind *dwiKind,
                  double thresh, double soft, double bval, double valueMin,
                  const Nrrd *ngrad,
                  const Nrrd *nbmat,
                  int e1method, int e2method) {
  char me[]="tenDwiGageKindSet", err[BIFF_STRLEN];
  tenDwiGageKindData *kindData;
  double grad[3], (*lup)(const void *, size_t);
  gageItemEntry *item;
  unsigned int gi;

  if (!dwiKind) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(TEN, err); return 0;
  }
  if (!( !!(ngrad) ^ !!(nbmat) )) {
    sprintf(err, "%s: need exactly one non-NULL in {ngrad,nbmat}", me);
    biffAdd(TEN, err); return 1;
  }
  if (nbmat) {
    sprintf(err, "%s: sorry, B-matrices temporarily disabled", me);
    biffAdd(TEN, err); return 1;
  }
  if (tenGradientCheck(ngrad, nrrdTypeDefault, 7)) {
    sprintf(err, "%s: problem with given gradients", me);
    biffAdd(TEN, err); return 1;
  }
  /* make sure that gradient lengths are as expected */
  lup = nrrdDLookup[ngrad->type];
  grad[0] = lup(ngrad->data, 0);
  grad[1] = lup(ngrad->data, 1);
  grad[2] = lup(ngrad->data, 2);
  if (0.0 != ELL_3V_LEN(grad)) {
    sprintf(err, "%s: sorry, currently need grad[0] to be len 0 (not %g)",
            me, ELL_3V_LEN(grad));
    biffAdd(TEN, err); return 1;
  }
  for (gi=1; gi<ngrad->axis[1].size; gi++) {
    grad[0] = lup(ngrad->data, 0 + 3*gi);
    grad[1] = lup(ngrad->data, 1 + 3*gi);
    grad[2] = lup(ngrad->data, 2 + 3*gi);
    if (0.0 == ELL_3V_LEN(grad)) {
      sprintf(err, "%s: sorry, all but first gradient must be non-zero "
              "(%u is zero)", me, gi);
      biffAdd(TEN, err); return 1;
    }
  }
  if (airEnumValCheck(tenEstimate1Method, e1method)) {
    sprintf(err, "%s: e1method %d is not a valid %s", me, 
            e1method, tenEstimate1Method->name);
    biffAdd(TEN, err); return 1;
  }
  if (airEnumValCheck(tenEstimate2Method, e2method)) {
    sprintf(err, "%s: emethod %d is not a valid %s", me, 
            e2method, tenEstimate2Method->name);
    biffAdd(TEN, err); return 1;
  }

  kindData = AIR_CAST(tenDwiGageKindData *, dwiKind->data);
  if (nrrdConvert(kindData->ngrad, ngrad, nrrdTypeDouble)) {
    sprintf(err, "%s: trouble converting", me);
    biffMove(TEN, err, NRRD); return 1;
  }
  dwiKind->valLen = kindData->ngrad->axis[1].size;
  dwiKind->table[tenDwiGageAll].answerLength = dwiKind->valLen;
  /* there is additional info to be set in item table */
  item = dwiKind->table + tenDwiGageTensor;
  switch (e1method) {
  case tenEstimate1MethodLLS:
    item->prereq[0] = tenDwiGageTensorLLS;
    break;
  case tenEstimate1MethodWLS:
    item->prereq[0] = tenDwiGageTensorWLS;
    break;
  case tenEstimate1MethodNLS:
    item->prereq[0] = tenDwiGageTensorNLS;
    break;
  case tenEstimate1MethodMLE:
    item->prereq[0] = tenDwiGageTensorMLE;
    break;
  default:
    sprintf(err, "%s: unimplemented %s: %s (%d)", me,
            tenEstimate1Method->name,
            airEnumStr(tenEstimate1Method, e1method), e1method);
    biffAdd(TEN, err); return 1;
    break;
  }
  kindData->thresh = thresh;
  kindData->soft = soft;
  kindData->bval = bval;
  kindData->valueMin = valueMin;
  kindData->est1Method = e1method;
  kindData->est2Method = e2method;
  return 0;
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
  if (0 == kind->valLen) {
    sprintf(err, "%s: don't yet know valLen", me);
    biffAdd(TEN, err); return 1;
  }
  if (!kind->data) {
    sprintf(err, "%s: kind->data is NULL", me);
    biffAdd(TEN, err); return 1;
  }
  return 0;
}
