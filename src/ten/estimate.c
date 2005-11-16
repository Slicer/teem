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

/*

http://www.mathworks.com/access/helpdesk/help/toolbox/curvefit/ch_fitt5.html#40515

*/

/* ---------------------------------------------- */

char
_tenEstimateMethodStr[][AIR_STRLEN_SMALL] = {
  "(unknown tenEstimateMethod)",
  "LLS",
  "WLS",
  "NLS",
  "MLE"
};

char
_tenEstimateMethodDesc[][AIR_STRLEN_MED] = {
  "unknown tenEstimateMethod",
  "linear least-squares fit of log(DWI)",
  "weighted least-squares fit of log(DWI)",
  "non-linear least-squares fit of DWI",
  "maximum likelihood estimate from DWI",
};

airEnum
_tenEstimateMethod = {
  "tenEstimateMethod",
  TEN_ESTIMATE_METHOD_MAX,
  _tenEstimateMethodStr, NULL,
  _tenEstimateMethodDesc,
  NULL, NULL,
  AIR_FALSE
};
airEnum *
tenEstimateMethod= &_tenEstimateMethod;

/* ---------------------------------------------- */

/*
** _tenBesselI0 and _tenBesselI1 need to be replaced with 
** Teem-license-compatible code!
*/
double
_tenBesselI0(double x) {
  double ax, ans, y;

  ax = AIR_ABS(x);
  if (ax < 3.750) {
    y = x/3.750;
    y *= y;
    ans = 1.0 + y*(3.51562290 + y*(3.08994240 + y*(1.20674920 + y*(0.26597320 + y*(0.3607680e-1 + y*0.458130e-2)))));
  } else {
    y = 3.750/ax;
    ans = (exp(ax)/sqrt(ax))*(0.398942280 + y*(0.13285920e-1 + y*(0.2253190e-2 + y*(-0.1575650e-2 + y*(0.9162810e-2 + y*(-0.20577060e-1 + y*(0.26355370e-1 + y*(-0.16476330e-1 + y*0.3923770e-2))))))));
  }
  return ans;
}

double
_tenBesselI1(double x) {
  double ax,ans,y;

  ax=AIR_ABS(x);
  if (ax < 3.75) {
    y=x/3.75;
    y *= y;
    ans = ax*(0.50 + y*(0.878905940 + y*(0.514988690 + y*(0.150849340 + y*(0.26587330e-1 + y*(0.3015320e-2 + y*0.324110e-3))))));
  } else {
    y = 3.75/ax;
    ans = 0.22829670e-1 + y*(-0.28953120e-1 + y*(0.17876540e-1 - y*0.4200590e-2));
    ans = 0.398942280 + y*(-0.39880240e-1 + y*(-0.3620180e-2 + y*(0.1638010e-2 + y*(-0.10315550e-1 + y*ans))));
    ans *= exp(ax)/sqrt(ax);
  }
  return x < 0.0 ? -ans : ans;
}

double
_tenRician(double m /* measured */, 
           double t /* truth */, 
           double s /* sigma */) {
  double ret, scl;
  
  if (t > 0 && s > 0 && 
      ((t/s > 6) || (AIR_ABS(m-t)/s > 6))) {
    ret = exp(-(m - t)*(m - t)/(2*s*s))/(s*sqrt(2*AIR_PI));
  } else {
    scl = (m/s)*exp(-(m*m/s + t*t/s)/(2*s));
    ret = scl*_tenBesselI0((m/s)*(t/s))/s;
    /*
    fprintf(stderr, "%g*_tenBesselI0(%g*%g)/%g = %g*_tenBesselI0(%g)/%g = %g*%g/%g = %g\n", 
            (m/s)*exp(-(m*m/s + t*t/s)/(2*s)), (m/s), (t/s), s,
            (m/s)*exp(-(m*m/s + t*t/s)/(2*s)), (m/s)*(t/s), s, 
            (m/s)*exp(-(m*m/s + t*t/s)/(2*s)), _tenBesselI0((m/s)*(t/s)), s,
            ret);
    */
  }
  return ret;
}

enum {
  flagUnknown,
  flagEstimateMethod,
  flagBInfo,
  flagAllNum,
  flagDwiNum,
  flagAllAlloc,
  flagDwiAlloc,
  flagAllSet,
  flagDwiSet,
  flagWght,
  flagEmat,
  flagLast
};

void
_tenEstimateOutputInit(tenEstimateContext *tec) {

  tec->B0 = AIR_NAN;
  TEN_T_SET(tec->ten, AIR_NAN,
            AIR_NAN, AIR_NAN, AIR_NAN,
            AIR_NAN, AIR_NAN,
            AIR_NAN);
  tec->conf = AIR_NAN;
  tec->mdwi = AIR_NAN;
  tec->time = AIR_NAN;
  tec->errorDwi = AIR_NAN;
  tec->errorLogDwi = AIR_NAN;
  tec->likelihood = AIR_NAN;
  strcpy(tec->errStr, "");
  tec->errNum = 0;
}

tenEstimateContext *
tenEstimateContextNew() {
  tenEstimateContext *tec;
  unsigned int fi;

  tec = AIR_CAST(tenEstimateContext *, malloc(sizeof(tenEstimateContext)));
  if (tec) {
    tec->bValue = AIR_NAN;
    tec->valueMin = AIR_NAN;
    tec->sigma = AIR_NAN;
    tec->dwiConfThresh = AIR_NAN;
    tec->dwiConfSoft = AIR_NAN;
    tec->_ngrad = NULL;
    tec->_nbmat = NULL;
    tec->all_f = NULL;
    tec->all_d = NULL;
    tec->simulate = AIR_FALSE;
    tec->estimateMethod = tenEstimateMethodUnknown;
    tec->estimateB0 = AIR_TRUE;
    tec->recordTime = AIR_FALSE;
    tec->recordErrorDwi = AIR_FALSE;
    tec->recordErrorLogDwi = AIR_FALSE;
    tec->recordLikelihood = AIR_FALSE;
    tec->verbose = 0;
    for (fi=flagUnknown+1; fi<flagLast; fi++) {
      tec->flag[fi] = AIR_FALSE;
    }
    tec->allNum = 0;
    tec->dwiNum = 0;
    tec->nbmat = nrrdNew();
    tec->nwght = nrrdNew();
    tec->nemat = nrrdNew();
    tec->all = NULL;
    tec->bnorm = NULL;
    tec->allTmp = NULL;
    tec->dwiTmp = NULL;
    tec->dwi = NULL;
    _tenEstimateOutputInit(tec);
  }
  return tec;
}

tenEstimateContext *
tenEstimateContextNix(tenEstimateContext *tec) {

  if (tec) {
    nrrdNuke(tec->nbmat);
    nrrdNuke(tec->nwght);
    nrrdNuke(tec->nemat);
    airFree(tec->all);
    airFree(tec->bnorm);
    airFree(tec->allTmp);
    airFree(tec->dwiTmp);
    airFree(tec->dwi);
    airFree(tec);
  }
  return NULL;
}

void
tenEstimateVerboseSet(tenEstimateContext *tec,
                      int verbose) {
  if (tec) {
    tec->verbose = verbose;
  }
  return;
}

int
tenEstimateMethodSet(tenEstimateContext *tec, int estimateMethod) {
  char me[]="tenEstimateMethodSet", err[AIR_STRLEN_MED];

  if (!tec) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(TEN, err); return 1;
  }
  if (airEnumValCheck(tenEstimateMethod, estimateMethod)) {
    sprintf(err, "%s: estimateMethod %d not valid", me, estimateMethod);
    biffAdd(TEN, err); return 1;
  }

  if (tec->estimateMethod != estimateMethod) {
    tec->estimateMethod = estimateMethod;
    tec->flag[flagEstimateMethod] = AIR_TRUE;
  }
  
  return 0;
}

int
tenEstimateSigmaSet(tenEstimateContext *tec, double sigma) {
  char me[]="tenEstimateSigmaSet", err[AIR_STRLEN_MED];

  if (!tec) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(TEN, err); return 1;
  }
  if (!(AIR_EXISTS(sigma) && sigma >= 0.0)) {
    sprintf(err, "%s: given sigma (%g) not existent and >= 0.0", me, sigma);
    biffAdd(TEN, err); return 1;
  }

  tec->sigma = sigma;

  return 0;
}

int
tenEstimateValueMinSet(tenEstimateContext *tec, double valueMin) {
  char me[]="tenEstimateValueMinSet", err[AIR_STRLEN_MED];

  if (!tec) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(TEN, err); return 1;
  }
  if (!(AIR_EXISTS(valueMin) && valueMin > 0.0)) {
    sprintf(err, "%s: given valueMin (%g) not existant and > 0.0",
            me, valueMin);
    biffAdd(TEN, err); return 1;
  }

  tec->valueMin = valueMin;

  return 0;
}

int
tenEstimateGradientsSet(tenEstimateContext *tec,
                        const Nrrd *ngrad, double bValue, int estimateB0) {
  char me[]="tenEstimateGradientsSet", err[AIR_STRLEN_MED];

  if (!(tec && ngrad)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(TEN, err); return 1;
  }
  if (!AIR_EXISTS(bValue)) {
    sprintf(err, "%s: given b value doesn't exist", me);
    biffAdd(TEN, err); return 1;
  }
  if (tenGradientCheck(ngrad, nrrdTypeDefault, 7)) {
    sprintf(err, "%s: problem with gradient list", me);
    biffAdd(TEN, err); return 1;
  }

  tec->bValue = bValue;
  tec->_ngrad = ngrad;
  tec->_nbmat = NULL;
  tec->estimateB0 = estimateB0;

  tec->flag[flagBInfo] = AIR_TRUE;
  return 0;
}

int
tenEstimateBMatricesSet(tenEstimateContext *tec,
                        const Nrrd *nbmat, double bValue, int estimateB0) {
  char me[]="tenEstimateBMatricesSet", err[AIR_STRLEN_MED];

  if (!(tec && nbmat)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(TEN, err); return 1;
  }
  if (!AIR_EXISTS(bValue)) {
    sprintf(err, "%s: given b value doesn't exist", me);
    biffAdd(TEN, err); return 1;
  }
  if (tenBMatrixCheck(nbmat, nrrdTypeDefault, 7)) {
    sprintf(err, "%s: problem with b-matrix list", me);
    biffAdd(TEN, err); return 1;
  }

  tec->bValue = bValue;
  tec->_ngrad = NULL;
  tec->_nbmat = nbmat;
  tec->estimateB0 = estimateB0;

  tec->flag[flagBInfo] = AIR_TRUE;
  return 0;
}

int
tenEstimateThresholdSet(tenEstimateContext *tec,
                        double thresh, double soft) {
  char me[]="tenEstimateThresholdSet", err[AIR_STRLEN_MED];

  if (!tec) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(TEN, err); return 1;
  }
  if (!(AIR_EXISTS(thresh) && AIR_EXISTS(soft))) {
    sprintf(err, "%s: not both threshold (%g) and softness (%g) exist", me,
            thresh, soft);
    biffAdd(TEN, err); return 1;
  }

  tec->dwiConfThresh = thresh;
  tec->dwiConfSoft = soft;

  return 0;
}

int
_tenEstimateCheck(tenEstimateContext *tec) {
  char me[]="_tenEstimateCheck", err[AIR_STRLEN_MED];

  if (!tec) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(TEN, err); return 1;
  }
  if (!( AIR_EXISTS(tec->valueMin) && tec->valueMin > 0.0)) {
    sprintf(err, "%s: need a positive valueMin set (not %g)",
            me, tec->valueMin);
    biffAdd(TEN, err); return 1;
  }
  if (!tec->simulate) {
    if (!AIR_EXISTS(tec->bValue)) {
      sprintf(err, "%s: b-value not set", me);
      biffAdd(TEN, err); return 1;
    }
    if (airEnumValCheck(tenEstimateMethod, tec->estimateMethod)) {
      sprintf(err, "%s: estimation method not set", me);
      biffAdd(TEN, err); return 1;
    }
    if (tenEstimateMethodMLE == tec->estimateMethod
        && !(AIR_EXISTS(tec->sigma) && (tec->sigma >= 0.0))
        ) {
      sprintf(err, "%s: can't do %s estim w/out non-negative sigma set", me,
              airEnumStr(tenEstimateMethod, tenEstimateMethodMLE));
      biffAdd(TEN, err); return 1;
    }
    if (!(AIR_EXISTS(tec->dwiConfThresh) && AIR_EXISTS(tec->dwiConfSoft))) {
      sprintf(err, "%s: not both threshold (%g) and softness (%g) exist", me,
              tec->dwiConfThresh, tec->dwiConfSoft);
      biffAdd(TEN, err); return 1;
    }
  }
  if (!( tec->_ngrad || tec->_nbmat )) {
    sprintf(err, "%s: need to set either gradients or B-matrices", me);
    biffAdd(TEN, err); return 1;
  }

  return 0;
}

int
_tenEstimateNumUpdate(tenEstimateContext *tec) {
  char me[]="_tenEstimateNumUpdate", err[AIR_STRLEN_MED];
  unsigned int newAllNum, newDwiNum, allIdx, size0;
  double (*lup)(const void *, size_t), gg[3], bb[6];

  if (tec->flag[flagBInfo]) {
    if (tec->_ngrad) {
      size0 = AIR_CAST(unsigned int, tec->_ngrad->axis[0].size);
      newAllNum = AIR_CAST(unsigned int, tec->_ngrad->axis[1].size);
      lup = nrrdDLookup[tec->_ngrad->type];
    } else {
      size0 = AIR_CAST(unsigned int, tec->_nbmat->axis[0].size);
      newAllNum = AIR_CAST(unsigned int, tec->_nbmat->axis[1].size);
      lup = nrrdDLookup[tec->_nbmat->type];
    }
    if (tec->allNum != newAllNum) {
      tec->allNum = newAllNum;
      tec->flag[flagAllNum] = AIR_TRUE;
    }

    /* HEY: this should be its own update function */
    airFree(tec->bnorm);
    tec->bnorm = AIR_CAST(double *, calloc(tec->allNum, sizeof(double)));
    if (!tec->bnorm) {
      sprintf(err, "%s: couldn't allocate bnorm vec %u\n", me, tec->allNum);
      biffAdd(TEN, err); return 1;
    }
    for (allIdx=0; allIdx<tec->allNum; allIdx++) {
      if (tec->_ngrad) {
        gg[0] = lup(tec->_ngrad->data, 0 + 3*allIdx);
        gg[1] = lup(tec->_ngrad->data, 1 + 3*allIdx);
        gg[2] = lup(tec->_ngrad->data, 2 + 3*allIdx);
        bb[0] = gg[0]*gg[0];
        bb[1] = gg[1]*gg[0];
        bb[2] = gg[2]*gg[0];
        bb[3] = gg[1]*gg[1];
        bb[4] = gg[2]*gg[1];
        bb[5] = gg[2]*gg[2];
      } else {
        bb[0] = lup(tec->_nbmat->data, 0 + 6*allIdx);
        bb[1] = lup(tec->_nbmat->data, 1 + 6*allIdx);
        bb[2] = lup(tec->_nbmat->data, 2 + 6*allIdx);
        bb[3] = lup(tec->_nbmat->data, 3 + 6*allIdx);
        bb[4] = lup(tec->_nbmat->data, 4 + 6*allIdx);
        bb[5] = lup(tec->_nbmat->data, 5 + 6*allIdx);
      }
      tec->bnorm[allIdx] = sqrt(bb[0]*bb[0] + 2*bb[1]*bb[1] + 2*bb[2]*bb[2]
                                + bb[3]*bb[3] + 2*bb[4]*bb[4]
                                + bb[5]*bb[5]);
    }
    
    if (tec->estimateB0) {
      newDwiNum = tec->allNum;
    } else {
      newDwiNum = 0;
      for (allIdx=0; allIdx<tec->allNum; allIdx++) {
        newDwiNum += (0.0 != tec->bnorm[allIdx]);
      }
    }
    if (tec->dwiNum != newDwiNum) {
      tec->dwiNum = newDwiNum;
      tec->flag[flagDwiNum] = AIR_TRUE;
    }
    if (!tec->estimateB0 && (tec->allNum == tec->dwiNum)) {
      sprintf(err, "%s: don't want to estimate B0, but all values are DW", me);
      biffAdd(TEN, err); return 1;
    }
  }
  return 0;
}

int
_tenEstimateAllAllocUpdate(tenEstimateContext *tec) {
  char me[]="_tenEstimateAllAllocUpdate", err[AIR_STRLEN_MED];

  if (tec->flag[flagAllNum]) {
    airFree(tec->all);
    airFree(tec->allTmp);
    tec->all = AIR_CAST(double *, calloc(tec->allNum, sizeof(double)));
    tec->allTmp = AIR_CAST(double *, calloc(tec->allNum, sizeof(double)));
    if (!( tec->all && tec->allTmp )) {
      sprintf(err, "%s: couldn't allocate \"all\" arrays (length %u)", me,
              tec->allNum);
      biffAdd(TEN, err); return 1;
    }
    tec->flag[flagAllAlloc] = AIR_TRUE;
  }
  return 0;
}

int
_tenEstimateDwiAllocUpdate(tenEstimateContext *tec) {
  char me[]="_tenEstimateDwiAllocUpdate", err[AIR_STRLEN_MED];
  size_t size[2];
  int E;

  if (tec->flag[flagDwiNum]) {
    airFree(tec->dwi);
    airFree(tec->dwiTmp);
    tec->dwi = AIR_CAST(double *, calloc(tec->dwiNum, sizeof(double)));
    tec->dwiTmp = AIR_CAST(double *, calloc(tec->dwiNum, sizeof(double)));
    if (!(tec->dwi && tec->dwiTmp)) {
      sprintf(err, "%s: couldn't allocate DWI arrays (length %u)", me,
              tec->dwiNum);
      biffAdd(TEN, err); return 1;
    }
    E = 0;
    if (!E) size[0] = (tec->estimateB0 ? 7 : 6);
    if (!E) size[1] = tec->dwiNum;
    if (!E) E |= nrrdMaybeAlloc_nva(tec->nbmat, nrrdTypeDouble, 2, size);
    if (!E) size[0] = tec->dwiNum;
    if (!E) size[1] = tec->dwiNum;
    if (!E) E |= nrrdMaybeAlloc_nva(tec->nwght, nrrdTypeDouble, 2, size);
    if (E) {
      sprintf(err, "%s: couldn't allocate dwi nrrds", me);
      biffMove(TEN, err, NRRD); return 1;
    }
    /* nrrdSave("0-nbmat.txt", tec->nbmat, NULL); */
    tec->flag[flagDwiAlloc] = AIR_TRUE;
  }
  return 0;
}

int
_tenEstimateAllSetUpdate(tenEstimateContext *tec) {
  /* char me[]="_tenEstimateAllSetUpdate", err[AIR_STRLEN_MED]; */
  /* unsigned int allIdx, dwiIdx; */

  if (tec->flag[flagAllAlloc]
      || tec->flag[flagDwiNum]) {

  }
  return 0;
}

int
_tenEstimateDwiSetUpdate(tenEstimateContext *tec) {
  /* char me[]="_tenEstimateDwiSetUpdate", err[AIR_STRLEN_MED]; */
  double (*lup)(const void *, size_t I), gg[3], *bmat;
  unsigned int allIdx, dwiIdx, bmIdx;
  
  if (tec->flag[flagAllNum]
      || tec->flag[flagDwiAlloc]) {
    if (tec->_ngrad) {
      lup = nrrdDLookup[tec->_ngrad->type];
    } else {
      lup = nrrdDLookup[tec->_nbmat->type];
    }
    dwiIdx = 0;
    bmat = AIR_CAST(double*, tec->nbmat->data);
    for (allIdx=0; allIdx<tec->allNum; allIdx++) {
      if (tec->estimateB0 || tec->bnorm[allIdx]) {
        if (tec->_ngrad) {
          gg[0] = lup(tec->_ngrad->data, 0 + 3*allIdx);
          gg[1] = lup(tec->_ngrad->data, 1 + 3*allIdx);
          gg[2] = lup(tec->_ngrad->data, 2 + 3*allIdx);
          bmat[0] = gg[0]*gg[0];
          bmat[1] = gg[1]*gg[0];
          bmat[2] = gg[2]*gg[0];
          bmat[3] = gg[1]*gg[1];
          bmat[4] = gg[2]*gg[1];
          bmat[5] = gg[2]*gg[2];
        } else {
          for (bmIdx=0; bmIdx<6; bmIdx++) {
            bmat[bmIdx] = lup(tec->_nbmat->data, bmIdx + 6*allIdx);
          }
        }
        bmat[1] *= 2.0;
        bmat[2] *= 2.0;
        bmat[4] *= 2.0;
        if (tec->estimateB0) {
          bmat[6] = -1;
        }
        bmat += tec->nbmat->axis[0].size;
        dwiIdx++;
      }
    }
  }
  return 0;
}

int
_tenEstimateWghtUpdate(tenEstimateContext *tec) {
  /* char me[]="_tenEstimateWghtUpdate", err[AIR_STRLEN_MED]; */
  unsigned int dwiIdx;
  double *wght;
  
  wght = AIR_CAST(double *, tec->nwght->data);
  if (tec->flag[flagDwiAlloc]
      || tec->flag[flagEstimateMethod]) {
    
    /* HEY: this is only useful for linear LS, no? */
    for (dwiIdx=0; dwiIdx<tec->dwiNum; dwiIdx++) {
      wght[dwiIdx + tec->dwiNum*dwiIdx] = 1.0;
    }
    
    tec->flag[flagEstimateMethod] = AIR_FALSE;
    tec->flag[flagWght] = AIR_TRUE;
  }
  return 0;
}

int
_tenEstimateEmatUpdate(tenEstimateContext *tec) {
  char me[]="tenEstimateEmatUpdate", err[AIR_STRLEN_MED];
  
  if (tec->flag[flagDwiSet]
      || tec->flag[flagWght]) {
    
    if (!tec->simulate) {
      /* HEY: ignores weights! */
      if (ell_Nm_pseudo_inv(tec->nemat, tec->nbmat)) {
        sprintf(err, "%s: trouble pseudo-inverting %ux%u B-matrix", me,
                AIR_CAST(unsigned int, tec->nbmat->axis[1].size),
                AIR_CAST(unsigned int, tec->nbmat->axis[0].size));
        biffMove(TEN, err, ELL); return 1;
      }
    }
    /*
    nrrdSave("nbmat.txt", tec->nbmat, NULL);
    nrrdSave("nemat.txt", tec->nemat, NULL);
    */
    
    tec->flag[flagDwiSet] = AIR_FALSE;
    tec->flag[flagWght] = AIR_FALSE;
  }
  return 0;
}

int
tenEstimateUpdate(tenEstimateContext *tec) {
  char me[]="tenEstimateUpdate", err[AIR_STRLEN_MED];

  if (_tenEstimateCheck(tec)
      || _tenEstimateNumUpdate(tec)
      || _tenEstimateAllAllocUpdate(tec)
      || _tenEstimateDwiAllocUpdate(tec)
      || _tenEstimateAllSetUpdate(tec)
      || _tenEstimateDwiSetUpdate(tec)
      || _tenEstimateWghtUpdate(tec)
      || _tenEstimateEmatUpdate(tec)) {
    sprintf(err, "%s: problem updating", me);
    biffAdd(TEN, err); return 1;
  }
  return 0;
}

/*
** from given tec->all_f or tec->all_d (whichever is non-NULL), sets:
** tec->all[],
** tec->dwi[]
** tec->B0, if !tec->estimateB0, 
** tec->mdwi,
** tec->conf (from tec->mdwi)
*/
void
_tenEstimateValuesSet(tenEstimateContext *tec) {
  unsigned int allIdx, dwiIdx, B0Num;
  double normSum;

  if (!tec->estimateB0) {
    tec->B0 = 0;
  }
  normSum = 0;
  tec->mdwi = 0;
  B0Num = 0;
  dwiIdx = 0;
  for (allIdx=0; allIdx<tec->allNum; allIdx++) {
    tec->all[allIdx] = (tec->all_f 
                        ? tec->all_f[allIdx]
                        : tec->all_d[allIdx]);
    tec->mdwi += tec->bnorm[allIdx]*tec->all[allIdx];
    normSum += tec->bnorm[allIdx];
    if (tec->estimateB0 || tec->bnorm[allIdx]) {
      tec->dwi[dwiIdx++] = tec->all[allIdx];
    } else {
      tec->B0 += tec->all[allIdx];
      B0Num += 1;
    }
  }
  if (!tec->estimateB0) {
    tec->B0 /= B0Num;
  }
  tec->mdwi /= normSum;
  if (tec->dwiConfSoft > 0) {
    tec->conf = AIR_AFFINE(-1, airErf((tec->mdwi - tec->dwiConfThresh)
                                      /tec->dwiConfSoft), 1,
                           0, 1);
  } else {
    tec->conf = tec->mdwi > tec->dwiConfThresh;
  }
  return;
}

/*
** ASSUMES THAT dwiTmp[] has been stuff with all values simulated from model
*/
double
_tenEstimateErrorDwi(tenEstimateContext *tec) {
  unsigned int dwiIdx;
  double err, diff;
  
  err = 0;
  for (dwiIdx=0; dwiIdx<tec->dwiNum; dwiIdx++) {
    diff = tec->dwi[dwiIdx] - tec->dwiTmp[dwiIdx];
    err += diff*diff;
  }
  return err;
}
double
_tenEstimateErrorLogDwi(tenEstimateContext *tec) {
  unsigned int dwiIdx;
  double err, diff;
  
  err = 0;
  for (dwiIdx=0; dwiIdx<tec->dwiNum; dwiIdx++) {
    diff = (log(AIR_MAX(tec->valueMin, tec->dwi[dwiIdx])) -
            log(AIR_MAX(tec->valueMin, tec->dwiTmp[dwiIdx])));
    err += diff*diff;
  }
  return err;
}

/*
** sets:
** tec->dwiTmp[]
*/
void
_tenEstimate1TensorSimulateSingle(tenEstimateContext *tec,
                                  double sigma, double bValue, double B0,
                                  const double ten[7]) {
  unsigned int dwiIdx, jj;
  double nr, ni, vv;
  const double *bmat;

  bmat = AIR_CAST(const double *, tec->nbmat->data);
  for (dwiIdx=0; dwiIdx<tec->dwiNum; dwiIdx++) {
    vv = 0;
    for (jj=0; jj<6; jj++) {
      vv += bmat[jj]*ten[1+jj];
    }
    vv = B0*exp(-bValue*vv);
    if (sigma > 0) {
      airNormalRand(&nr, &ni);
      nr *= sigma;
      ni *= sigma;
      vv = sqrt((vv+nr)*(vv+nr) + ni*ni);
    }
    tec->dwiTmp[dwiIdx] = vv;
    bmat += tec->nbmat->axis[0].size;
  }
  return;
}

void
tenEstimate1TensorSimulateSingle_f(tenEstimateContext *tec,
                                   float *simval,
                                   float sigma, float bValue, float B0,
                                   const float _ten[7]) {
  char me[]="tenEstimate1TensorSimulateSingle_f";
  unsigned int allIdx, dwiIdx;
  double ten[7];
  
  if (!tec) {
    return;
  }
  if (!(simval && ten)) {
    sprintf(tec->errStr, "%s: got NULL pointer", me);
    tec->errNum = 1;
    return;
  }

  TEN_T_COPY_T(ten, double, _ten);
  _tenEstimate1TensorSimulateSingle(tec, sigma, bValue, B0, ten);
  dwiIdx = 0;
  for (allIdx=0; allIdx<tec->allNum; allIdx++) {
    if (tec->estimateB0 || tec->bnorm[allIdx]) {
      simval[allIdx] = AIR_CAST(float, tec->dwiTmp[dwiIdx++]);
    } else {
      simval[allIdx] = B0;
    }
  }
  return;
}

void
tenEstimate1TensorSimulateSingle_d(tenEstimateContext *tec,
                                   double *simval,
                                   double sigma, double bValue, double B0,
                                   const double ten[7]) {
  char me[]="tenEstimate1TensorSimulateSingle_d";
  unsigned int allIdx, dwiIdx;
  
  if (!tec) {
    return;
  }
  if (!(simval && ten)) {
    sprintf(tec->errStr, "%s: got NULL pointer", me);
    tec->errNum = 1;
    return;
  }

  _tenEstimate1TensorSimulateSingle(tec, sigma, bValue, B0, ten);
  dwiIdx = 0;
  for (allIdx=0; allIdx<tec->allNum; allIdx++) {
    if (tec->estimateB0 || tec->bnorm[allIdx]) {
      simval[allIdx] = tec->dwiTmp[dwiIdx++];
    } else {
      simval[allIdx] = B0;
    }
  }

  return;
}

int
tenEstimate1TensorSimulateVolume(tenEstimateContext *tec,
                                 Nrrd *ndwi, 
                                 double sigma, double bValue,
                                 const Nrrd *nB0, const Nrrd *nten,
                                 int outType) {
  char me[]="tenEstimate1TensorSimulateVolume", err[AIR_STRLEN_MED];
  size_t sizeTen, sizeX, sizeY, sizeZ, NN, II;
  double (*tlup)(const void *, size_t), (*blup)(const void *, size_t), 
    ten_d[7], *dwi_d, B0;
  float *dwi_f, ten_f[7];
  unsigned int tt;
  int axmap[4];
  airArray *mop;

  if (!(tec && ndwi && nB0 && nten)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(TEN, err); return 1;
  }
  if (!(AIR_EXISTS(sigma) && sigma >= 0.0 
        && AIR_EXISTS(bValue) && bValue > 0.0)) {
    sprintf(err, "%s: got invalid sigma (%g) or bValue (%g)\n", me,
            sigma, bValue);
    biffAdd(TEN, err); return 1;
  }
  if (airEnumValCheck(nrrdType, outType)) {
    sprintf(err, "%s: requested output type %d not valid", me, outType);
    biffAdd(TEN, err); return 1;
  }
  if (!( nrrdTypeFloat == outType || nrrdTypeDouble == outType )) {
    sprintf(err, "%s: requested output type (%s) not %s or %s", me,
            airEnumStr(nrrdType, outType),
            airEnumStr(nrrdType, nrrdTypeFloat),
            airEnumStr(nrrdType, nrrdTypeDouble));
    biffAdd(TEN, err); return 1;
  }

  mop = airMopNew();

  sizeTen = nrrdKindSize(nrrdKind3DMaskedSymMatrix);
  sizeX = nten->axis[1].size;
  sizeY = nten->axis[2].size;
  sizeZ = nten->axis[3].size;
  if (!(3 == nB0->dim &&
        sizeX == nB0->axis[0].size &&
        sizeY == nB0->axis[1].size &&
        sizeZ == nB0->axis[2].size)) {
    sprintf(err, "%s: given B0 (%u-D) volume not 3-D " _AIR_SIZE_T_CNV
            "x" _AIR_SIZE_T_CNV "x" _AIR_SIZE_T_CNV, me, nB0->dim,
            sizeX, sizeY, sizeZ);
    biffAdd(TEN, err); return 1;
  }
  if (nrrdMaybeAlloc(ndwi, outType, 4,
                     AIR_CAST(size_t, tec->allNum), sizeX, sizeY, sizeZ)) {
    sprintf(err, "%s: couldn't allocate DWI output", me);
    biffMove(TEN, err, NRRD); airMopError(mop); return 1;
  }
  NN = sizeX * sizeY * sizeZ;
  tlup = nrrdDLookup[nten->type];
  blup = nrrdDLookup[nB0->type];
  dwi_d = AIR_CAST(double *, ndwi->data);
  dwi_f = AIR_CAST(float *, ndwi->data);
  for (II=0; II<NN; II++) {
    B0 = blup(nB0->data, II);
    if (nrrdTypeDouble == outType) {
      for (tt=0; tt<7; tt++) {
        ten_d[tt] = tlup(nten->data, tt + sizeTen*II);
      }
      tenEstimate1TensorSimulateSingle_d(tec, dwi_d, sigma, bValue, B0, ten_d);
      dwi_d += tec->allNum;
    } else {
      for (tt=0; tt<7; tt++) {
        ten_f[tt] = tlup(nten->data, tt + sizeTen*II);
      }
      tenEstimate1TensorSimulateSingle_f(tec, dwi_f, sigma, bValue, B0, ten_f);
      dwi_f += tec->allNum;
    }
    if (tec->errNum) {
      biffAdd(TEN, tec->errStr);
      sprintf(err, "%s: failed at sample " _AIR_SIZE_T_CNV, me, II);
      biffAdd(TEN, err); airMopError(mop); return 1;
    }
  }

  ELL_4V_SET(axmap, -1, 1, 2, 3);
  nrrdAxisInfoCopy(ndwi, nten, axmap, NRRD_AXIS_INFO_NONE);
  ndwi->axis[0].kind = nrrdKindList;
  if (nrrdBasicInfoCopy(ndwi, nten,
                        NRRD_BASIC_INFO_ALL ^ NRRD_BASIC_INFO_SPACE)) {
    sprintf(err, "%s:", me);
    biffAdd(NRRD, err); return 1;
  }
  airMopOkay(mop);
  return 0;
}

/*
** sets:
** tec->ten[1..6]
** tec->B0, if tec->estimateB0
*/
void
_tenEstimate1Tensor_LLS(tenEstimateContext *tec) {
  /* char me[]="_tenEstimate1Tensor_LLS"; */
  double *emat, tmp, logB0;
  unsigned int ii, jj;
  
  emat = AIR_CAST(double *, tec->nemat->data);
  if (tec->estimateB0) {
    for (ii=0; ii<tec->allNum; ii++) {
      tmp = AIR_MAX(tec->valueMin, tec->all[ii]);
      tec->allTmp[ii] = -log(tmp)/tec->bValue;
    }
    for (jj=0; jj<7; jj++) {
      tmp = 0;
      for (ii=0; ii<tec->allNum; ii++) {
        tmp += emat[ii + tec->allNum*jj]*tec->allTmp[ii];
      }
      if (jj < 6) {
        tec->ten[1+jj] = tmp;
      } else {
        /* we're on seventh row, for finding B0 */
        tec->B0 = exp(tec->bValue*tmp);
      }
    }
  } else {
    logB0 = log(AIR_MAX(tec->valueMin, tec->B0));
    for (ii=0; ii<tec->dwiNum; ii++) {
      tmp = AIR_MAX(tec->valueMin, tec->dwi[ii]);
      tec->dwiTmp[ii] = (logB0 - log(tmp))/tec->bValue;
    }
    for (jj=0; jj<6; jj++) {
      tmp = 0;
      for (ii=0; ii<tec->dwiNum; ii++) {
        tmp += emat[ii + tec->dwiNum*jj]*tec->dwiTmp[ii];
      }
      tec->ten[1+jj] = tmp;
    }
  }
  return;
}

void
_tenEstimate1Tensor_WLS(tenEstimateContext *tec) {
  char me[]="_tenEstimate1Tensor_WLS";
  unsigned int dwiIdx;
  double *wght, dwi;

  wght = AIR_CAST(double *, tec->nwght->data);
  for (dwiIdx=0; dwiIdx<tec->dwiNum; dwiIdx++) {
    dwi = AIR_MAX(FLT_MIN, tec->dwi[dwiIdx]);
    wght[dwiIdx + tec->dwiNum*dwiIdx] = dwi*dwi;
  }
  /* nrrdSave("the-nbmat.txt", tec->nbmat, NULL); */
  if (ell_Nm_wght_pseudo_inv(tec->nemat, tec->nbmat, tec->nwght)) {
    sprintf(tec->errStr, "%s: trouble wght-pseudo-inverting %ux%u B-matrix:\n"
            "%s", me, AIR_CAST(unsigned int, tec->nbmat->axis[1].size),
            AIR_CAST(unsigned int, tec->nbmat->axis[0].size),
            biffGet(ELL));
    tec->errNum = 1;
    return;
  }
  _tenEstimate1Tensor_LLS(tec);

  /* just one more iteration seems to make a big difference */
  _tenEstimate1TensorSimulateSingle(tec, 0.0, tec->bValue, tec->B0, tec->ten);
  for (dwiIdx=0; dwiIdx<tec->dwiNum; dwiIdx++) {
    wght[dwiIdx + tec->dwiNum*dwiIdx] 
      = tec->dwiTmp[dwiIdx]*tec->dwiTmp[dwiIdx];
  }
  ell_Nm_wght_pseudo_inv(tec->nemat, tec->nbmat, tec->nwght);
  _tenEstimate1Tensor_LLS(tec);
  
  return;
}

void
_tenEstimate1Tensor_Descent(tenEstimateContext *tec,
                            double (*stepCB)(tenEstimateContext *tec,
                                             double stepTen[7],
                                             double B0,
                                             double ten[7]),
                            double (*badnessCB)(tenEstimateContext *tec,
                                                double B0,
                                                double ten[7])) {
  char me[]="_tenEstimate1Tensor_Descent";
  double currB0, lastB0, currTen[7], lastTen[7], stepB0, stepTen[7],
    stepSize, badInit, bad, badDelta, stepMin = 0.00000000001, badLast;
  unsigned int iter, iterMax = 10000;

  /* start with WLS fit since its probably closer than LLS */
  _tenEstimate1Tensor_WLS(tec);
  
  badInit = badnessCB(tec, tec->B0, tec->ten); 
  if (tec->verbose) {
    fprintf(stderr, "\n%s: ________________________________________\n", me);
    fprintf(stderr, "%s: start: badInit = %g ---------------\n", me, badInit);
  }

  stepB0 = stepCB(tec, stepTen, tec->B0, tec->ten);

  if (!AIR_EXISTS(stepB0)) {
    tec->ten[0] = 0;
    return;
  }

  stepSize = 0.1;
  do {
    stepSize /= 10;
    TEN_T_SCALE_ADD2(currTen, 1.0, tec->ten, stepSize, stepTen);
    currB0 = tec->B0 + stepSize*stepB0;
    bad = badnessCB(tec, currB0, currTen);
    if (tec->verbose) {
      fprintf(stderr, "%s: ************ stepSize = %g --> bad = %g\n",
              me, stepSize, bad);
    }
  } while (bad > badInit && stepSize > stepMin);

  if (stepSize <= stepMin) {
    /*
    sprintf(tec->errStr, "%s: never found a usable step size", me);
    tec->errNum = 1;
    */
    /* HEY: for MLE sometimes it seems we nail the minima, 
       in which case this isn't an error, maybe ? 
    tec->ten[0] = 0;
    */
    ELL_6V_COPY(tec->ten+1, currTen+1);
    tec->B0 = currB0;
    tec->ten[0] = 0.93;  /* a little reminder */
    return;
  }

  iter = 0;
  badLast = bad;
  do {
    iter++;
    stepB0 = stepCB(tec, stepTen, currB0, currTen);
    if (!AIR_EXISTS(stepB0)) {
      tec->ten[0] = 0;
      return;
    }
    TEN_T_COPY(lastTen, currTen);
    lastB0 = currB0;
    TEN_T_SCALE_INCR(currTen, stepSize, stepTen);
    currB0 += stepSize*stepB0;
    bad = badnessCB(tec, currB0, currTen);
    if (tec->verbose) {
      fprintf(stderr, "%s: %u bad = %g\n", me, iter, bad);
    }
    badDelta = bad - badLast;
    badLast = bad;
    if (badDelta > 0) {
      stepSize /= 10;
      if (tec->verbose) {
        fprintf(stderr, "%s: badDelta %g > 0 ---> stepSize = %g\n",
                me, badDelta, stepSize);
      }
      badDelta = -1;  /* bogus */
      TEN_T_COPY(currTen, lastTen);
      currB0 = lastB0;
      /*  goto tryagain; */
    }
  } while (iter < iterMax && (iter < 2 || badDelta < -0.00005));
  if (tec->verbose) {
    fprintf(stderr, "%s: finished\n", me);
  }

  ELL_6V_COPY(tec->ten+1, currTen+1);
  tec->B0 = currB0;

}
                            
double
_tenEstimate1Tensor_StepNLS(tenEstimateContext *tec, double stepTen[7],
                            double currB0, double currTen[7]) {
  /* char me[]="_tenEstimate1Tensor_StepNLS"; */
  double *bmat, dot, tmp, diff, scl, stepB0;
  unsigned int dwiIdx;

  /* fprintf(stderr, "%s step\n", me); */
  TEN_T_SET(stepTen, 0,   0, 0, 0,    0, 0,   0);
  stepB0 = 0;
  bmat = AIR_CAST(double *, tec->nbmat->data);
  for (dwiIdx=0; dwiIdx<tec->dwiNum; dwiIdx++) {
    /*
    fprintf(stderr, "%s[%u]: bmat = %g %g %g    %g %g     %g\n",
            me, dwiIdx,
            bmat[0], bmat[1], bmat[2], 
            bmat[3], bmat[4], 
            bmat[5]);
    */
    dot = ELL_6V_DOT(bmat, currTen+1);
    tmp = currB0*exp(-(tec->bValue)*dot);
    diff = tec->dwi[dwiIdx] - tmp;
    scl = -2*diff*tmp*(tec->bValue);
    ELL_6V_SCALE_INCR(stepTen+1, scl, bmat);
    bmat += tec->nbmat->axis[0].size;
    /* HEY: calculate stepB0 */
  }
  return stepB0;
}

double
_tenEstimate1Tensor_BadnessNLS(tenEstimateContext *tec,
                               double currB0, double currTen[7]) {

  _tenEstimate1TensorSimulateSingle(tec, 0.0, tec->bValue, currB0, currTen);
  return _tenEstimateErrorDwi(tec);
}

void
_tenEstimate1Tensor_NLS(tenEstimateContext *tec) {
  /* char me[]="_tenEstimate1Tensor_NLS"; */

  /* fprintf(stderr, "%s -----------------------------------\n", me); */
  _tenEstimate1Tensor_Descent(tec, _tenEstimate1Tensor_StepNLS,
                              _tenEstimate1Tensor_BadnessNLS);

  return;
}

double
_tenEstimate1Tensor_StepMLE(tenEstimateContext *tec, double stepTen[7],
                            double currB0, double currTen[7]) {
  char me[]="_tenEstimate1Tensor_StepMLE";
  double *bmat, dot, barg, tmp, scl, stepB0, dwi, sigma, bval;
  unsigned int dwiIdx;

  if (tec->verbose) {
    fprintf(stderr, "%s step (currTen = %g %g %g   %g %g    %g)\n", me,
            currTen[1], currTen[2], currTen[3],
            currTen[4], currTen[5],
            currTen[6]);
  }
  TEN_T_SET(stepTen, 0,   0, 0, 0,    0, 0,   0);
  stepB0 = 0;
  sigma = tec->sigma;
  bval = tec->bValue;
  bmat = AIR_CAST(double *, tec->nbmat->data);
  for (dwiIdx=0; dwiIdx<tec->dwiNum; dwiIdx++) {
    dwi = tec->dwi[dwiIdx];
    dot = ELL_6V_DOT(bmat, currTen+1);
     barg = exp(-bval*dot)*(dwi/sigma)*(currB0/sigma);
    tmp = (exp(bval*dot)/sigma)*dwi/_tenBesselI0(barg);
    if (tec->verbose) {
      fprintf(stderr, "%s[%u]: dot = %g, barg = %g, tmp = %g\n", me, dwiIdx,
              dot, barg, tmp);
    }
    if (tmp > DBL_MIN) {
      tmp = currB0/sigma - tmp*_tenBesselI1(barg);
    } else {
      tmp = currB0/sigma;
    }
    if (tec->verbose) {
      fprintf(stderr, " ---- tmp = %g\n", tmp);
    }
    scl = tmp*exp(-2*bval*dot)*bval*currB0/sigma;
    ELL_6V_SCALE_INCR(stepTen+1, scl, bmat);

    if (tec->verbose) {
      fprintf(stderr, "%s[%u]: bmat = %g %g %g    %g %g     %g\n",
              me, dwiIdx,
              bmat[0], bmat[1], bmat[2], 
              bmat[3], bmat[4], 
              bmat[5]);
      fprintf(stderr, "%s[%u]: scl = %g -> stepTen = %g %g %g    %g %g   %g\n",
              me, dwiIdx, scl,
              stepTen[1], stepTen[2], stepTen[3],
              stepTen[4], stepTen[5],
              stepTen[6]);
    }
    if (!AIR_EXISTS(scl)) {
      fprintf(stderr, "%s: scl = %g BYE\n", me, scl);
      return AIR_NAN;
    }
    bmat += tec->nbmat->axis[0].size;
    /* HEY: calculate stepB0 */
  }
  if (tec->verbose) {
    fprintf(stderr, "%s: final stepTen = %g %g %g    %g %g   %g\n", me,
            stepTen[1], stepTen[2], stepTen[3],
            stepTen[4], stepTen[5],
            stepTen[6]);
  }
  return stepB0;
}

double
_tenEstimate1Tensor_BadnessMLE(tenEstimateContext *tec,
                               double currB0, double currTen[7]) {
  char me[]="_tenEstimate1Tensor_BadnessMLE";
  unsigned int dwiIdx;
  double *bmat, loglike, rice, logrice, arg, dot;

  if (tec->verbose) {
    fprintf(stderr, "%s: currTen = %g %g %g    %g %g     %g\n",
            me,
            currTen[1], currTen[2], currTen[3], 
            currTen[4], currTen[5], 
            currTen[6]);
  }
  loglike = 0;
  bmat = AIR_CAST(double *, tec->nbmat->data);
  for (dwiIdx=0; dwiIdx<tec->dwiNum; dwiIdx++) {
    dot = ELL_6V_DOT(bmat, currTen+1);
    arg = currB0*exp(-(tec->bValue)*dot);
    if (tec->verbose) {
      fprintf(stderr, "%s[%u]: B0*exp(-(tec->bValue)*dot) = "
              "%g * exp(-%g*%g) = %g * exp(%g) = %g * %g = %g\n",
              me, dwiIdx,
              currB0, tec->bValue, dot,
              currB0, -(tec->bValue)*dot,
              currB0, exp(-(tec->bValue)*dot), arg);
    }
    rice = _tenRician(tec->dwi[dwiIdx], arg, tec->sigma);
    if (rice > FLT_MIN /* yea I know */) {
      logrice = log(rice);
    } else {
      logrice = -FLT_MAX/tec->dwiNum;
    }
    logrice = log(rice + FLT_MIN);
    if (tec->verbose) {
      fprintf(stderr, "%s[%u]: dot = %g, " 
              "rice(%g,%g,%g) = %g, log = %g\n",
              me, dwiIdx, dot, 
              tec->dwi[dwiIdx], arg, tec->sigma,
              rice, logrice);
    }
    loglike += logrice;
    if (!AIR_EXISTS(loglike)) {
      fprintf(stderr, "%s[%u]: dot = %g, " 
              "rice(%g,%g,%g) = %g, log = %g\n",
              me, dwiIdx, dot, 
              tec->dwi[dwiIdx], arg, tec->sigma,
              rice, logrice);
      fprintf(stderr, "%s[%u]: loglike = %g DOES NOT EXIST\n", 
              me, dwiIdx, loglike);
      exit(0);
    }
    bmat += tec->nbmat->axis[0].size;
  }
  loglike *= -1;
  if (tec->verbose) {
    fprintf(stderr, "%s: returning %g\n", me, loglike);
  }
  return loglike;
}

void
_tenEstimate1Tensor_MLE(tenEstimateContext *tec) {
  /* char me[]="_tenEstimate1Tensor_MLE"; */

  /* fprintf(stderr, "%s -----------------------------------\n", me); */
  _tenEstimate1Tensor_Descent(tec, _tenEstimate1Tensor_StepMLE,
                              _tenEstimate1Tensor_BadnessMLE);

  return;
}

/*
** sets:
** tec->ten[0] (from tec->conf)
** tec->time, if tec->recordTime
** tec->errorDwi, if tec->recordErrorDwi
** tec->errorLogDwi, if tec->recordErrorLogDwi
** tec->likelihood, if tec->recordLikelihood
*/
void
_tenEstimate1TensorSingle(tenEstimateContext *tec) {
  char me[]="_tenEstimate1TensorSingle";
  double time0;

  _tenEstimateOutputInit(tec);
  time0 = tec->recordTime ? airTime() : 0;
  _tenEstimateValuesSet(tec);
  tec->ten[0] = tec->conf;
  switch(tec->estimateMethod) {
  case tenEstimateMethodLLS:
    _tenEstimate1Tensor_LLS(tec);
    break;
  case tenEstimateMethodWLS:
    _tenEstimate1Tensor_WLS(tec);
    break;
  case tenEstimateMethodNLS:
    _tenEstimate1Tensor_NLS(tec);
    break;
  case tenEstimateMethodMLE:
    _tenEstimate1Tensor_MLE(tec);

    tec->ten[0] = _tenEstimate1Tensor_BadnessMLE(tec, tec->B0, tec->ten);

    break;
  default:
    sprintf(tec->errStr, "%s: estimation method %d unknown!",
            me, tec->estimateMethod);
    tec->errNum = 1;
    break;
  }
  tec->time = tec->recordTime ? airTime() - time0 : 0;
  /* record errors! */
  return;
}

void
tenEstimate1TensorSingle_f(tenEstimateContext *tec,
                           float ten[7], const float *all) {
  char me[]="tenEstimate1TensorSingle_f";

  if (!tec) {
    return; 
  }
  if (!(ten && all)) {
    sprintf(tec->errStr, "%s: got NULL pointer", me);
    tec->errNum = 1;
    return;
  }

  tec->all_f = all;
  tec->all_d = NULL;
  _tenEstimate1TensorSingle(tec);      /* may set errStr and errNum */
  TEN_T_COPY_T(ten, float, tec->ten);

  return;
}

void
tenEstimate1TensorSingle_d(tenEstimateContext *tec,
                           double ten[7], const double *all) {
  char me[]="tenEstimate1TensorSingle_d";

  if (!tec) {
    return;
  }
  if (!(ten && all)) {
    sprintf(tec->errStr, "%s: got NULL pointer", me);
    tec->errNum = 1;
    return;
  }

  tec->all_f = NULL;
  tec->all_d = all;
  _tenEstimate1TensorSingle(tec);      /* may set errStr and errNum */
  TEN_T_COPY(ten, tec->ten);

  return;
}

int
tenEstimate1TensorVolume4D(tenEstimateContext *tec,
                           Nrrd *nten, Nrrd **nB0P, Nrrd **nterrP,
                           const Nrrd *ndwi, int outType) {
  char me[]="tenEstimate1TensorVolume4D", err[AIR_STRLEN_MED];
  size_t sizeTen, sizeX, sizeY, sizeZ, NN, II;
  double *all, ten[7], (*lup)(const void *, size_t),
    (*ins)(void *v, size_t I, double d);
  unsigned int dd;
  airArray *mop;
  int axmap[4];

#if 0
  /* little thing to test stability of BesselI[1,x] */
#define NUM 800
  double val[NUM], minVal=0, maxVal=10, arg;
  unsigned int valIdx;
  Nrrd *nval;
  for (valIdx=0; valIdx<NUM; valIdx++) {
    arg = AIR_AFFINE(0, valIdx, NUM-1, minVal, maxVal);
    val[valIdx] = _tenRician(arg, 1, 1);
  }
  nval = nrrdNew();
  nrrdWrap(nval, val, nrrdTypeDouble, 1, AIR_CAST(size_t, NUM));
  nrrdSave("nval.nrrd", nval, NULL);
  nrrdNix(nval);
#endif

  if (!(tec && nten && ndwi)) {
    /* nerrP and _NB0P can be NULL */
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(TEN, err); return 1;
  }
  if (nrrdCheck(ndwi)) {
    sprintf(err, "%s: DWI volume not valid", me);
    biffMove(TEN, err, NRRD); return 1;
  }
  if (!( 4 == ndwi->dim && 7 <= ndwi->axis[0].size )) {
    sprintf(err, "%s: DWI volume should be 4-D with axis 0 size >= 7", me);
    biffAdd(TEN, err); return 1;
  }
  if (tec->allNum != ndwi->axis[0].size) {
    sprintf(err, "%s: from %s info, expected %u values per sample, "
            "but have " _AIR_SIZE_T_CNV " in volume", me,
            tec->_ngrad ? "gradient" : "B-matrix", tec->allNum,
            ndwi->axis[0].size);
    biffAdd(TEN, err); return 1;
  }
  if (nrrdTypeBlock == ndwi->type) {
    sprintf(err, "%s: DWI volume has non-scalar type %s", me,
            airEnumStr(nrrdType, ndwi->type));
    biffAdd(TEN, err); return 1;
  }
  if (airEnumValCheck(nrrdType, outType)) {
    sprintf(err, "%s: requested output type %d not valid", me, outType);
    biffAdd(TEN, err); return 1;
  }
  if (!( nrrdTypeFloat == outType || nrrdTypeDouble == outType )) {
    sprintf(err, "%s: requested output type (%s) not %s or %s", me,
            airEnumStr(nrrdType, outType),
            airEnumStr(nrrdType, nrrdTypeFloat),
            airEnumStr(nrrdType, nrrdTypeDouble));
    biffAdd(TEN, err); return 1;
  }

  mop = airMopNew();

  sizeTen = nrrdKindSize(nrrdKind3DMaskedSymMatrix);
  sizeX = ndwi->axis[1].size;
  sizeY = ndwi->axis[2].size;
  sizeZ = ndwi->axis[3].size;
  all = AIR_CAST(double *, calloc(tec->allNum, sizeof(double)));
  if (!all) {
    sprintf(err, "%s: couldn't allocate length %u array", me, tec->allNum);
    biffAdd(TEN, err); airMopError(mop); return 1;
  }
  airMopAdd(mop, all, airFree, airMopAlways);

  if (nrrdMaybeAlloc(nten, outType, 4, sizeTen, sizeX, sizeY, sizeZ)) {
    sprintf(err, "%s: couldn't allocate tensor output", me);
    biffMove(TEN, err, NRRD); airMopError(mop); return 1;
  }
  if (nB0P) {
    *nB0P = nrrdNew();
    if (nrrdMaybeAlloc(*nB0P, outType, 3, sizeX, sizeY, sizeZ)) {
      sprintf(err, "%s: couldn't allocate B0 output", me);
      biffMove(TEN, err, NRRD); airMopError(mop); return 1;
    }
    airMopAdd(mop, *nB0P, (airMopper)nrrdNuke, airMopOnError);
    airMopAdd(mop, nB0P, (airMopper)airSetNull, airMopOnError);
  }
  if (nterrP) {
    *nterrP = nrrdNew();
    if (nrrdMaybeAlloc(*nterrP, outType, 3, sizeX, sizeY, sizeZ)) {
      sprintf(err, "%s: couldn't allocate fitting error output", me);
      biffMove(TEN, err, NRRD); airMopError(mop); return 1;
    }
    airMopAdd(mop, *nterrP, (airMopper)nrrdNuke, airMopOnError);
    airMopAdd(mop, nterrP, (airMopper)airSetNull, airMopOnError);
  }
  NN = sizeX * sizeY * sizeZ;
  lup = nrrdDLookup[ndwi->type];
  ins = nrrdDInsert[outType];
  /*
  fprintf(stderr, "%s: ", me); 
  fflush(stderr);
  */
  for (II=0; II<NN; II++) {
    /*
    fprintf(stderr, " %u/%u", AIR_CAST(unsigned int, II),
            AIR_CAST(unsigned int, NN));
    fflush(stderr);
    */
    tec->verbose = (0 && 35 == II);

    for (dd=0; dd<tec->allNum; dd++) {
      all[dd] = lup(ndwi->data, dd + tec->allNum*II);
    }
    /* 
    fprintf(stderr, "%s: __________ begin %u\n",
            me, AIR_CAST(unsigned int, II));
    */
    tenEstimate1TensorSingle_d(tec, ten, all);
    /*
    fprintf(stderr, "%s: ^^^^^^^^^^ end %u\n",
            me, AIR_CAST(unsigned int, II));
    */
    if (tec->errNum) {
      biffAdd(TEN, tec->errStr);
      sprintf(err, "%s: failed at sample " _AIR_SIZE_T_CNV, me, II);
      biffAdd(TEN, err); airMopError(mop); return 1;
    }
    ins(nten->data, 0 + sizeTen*II, ten[0]);
    ins(nten->data, 1 + sizeTen*II, ten[1]);
    ins(nten->data, 2 + sizeTen*II, ten[2]);
    ins(nten->data, 3 + sizeTen*II, ten[3]);
    ins(nten->data, 4 + sizeTen*II, ten[4]);
    ins(nten->data, 5 + sizeTen*II, ten[5]);
    ins(nten->data, 6 + sizeTen*II, ten[6]);
    if (nB0P) {
      ins((*nB0P)->data, II, tec->B0);
    }
    if (nterrP) {
      ins((*nterrP)->data, II, tec->errorDwi);
    }
  }

  ELL_4V_SET(axmap, -1, 1, 2, 3);
  nrrdAxisInfoCopy(nten, ndwi, axmap, NRRD_AXIS_INFO_NONE);
  nten->axis[0].kind = nrrdKind3DMaskedSymMatrix;
  if (nrrdBasicInfoCopy(nten, ndwi,
                        NRRD_BASIC_INFO_ALL ^ NRRD_BASIC_INFO_SPACE)) {
    sprintf(err, "%s:", me);
    biffAdd(NRRD, err); return 1;
  }

  airMopOkay(mop);
  return 0;
}
