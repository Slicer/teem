/*
  Teem: Tools to process and visualize scientific data and images              
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

#include "ten.h"
#include "privateTen.h"

const char *
tenModelPrefixStr = "DWMRI_model";

static const tenModel *
str2model(const char *str) {
  const tenModel *ret = NULL;

  if (!strcmp(str, TEN_MODEL_STR_BALL))       ret = tenModelBall;
  if (!strcmp(str, TEN_MODEL_STR_1STICK))     ret = tenModel1Stick;
  if (!strcmp(str, TEN_MODEL_STR_BALL1STICK)) ret = tenModelBall1Stick;
  if (!strcmp(str, TEN_MODEL_STR_CYLINDER))   ret = tenModelCylinder;
  if (!strcmp(str, TEN_MODEL_STR_TENSOR2))    ret = tenModelTensor2;
  return ret;
}

int
tenModelParse(const tenModel **model, int *plusB0, 
              int requirePrefix, const char *_str) {
  static const char me[]="tenModelParse";
  char *str, *modstr, prefix[AIR_STRLEN_MED], *pre;
  airArray *mop;

  if (!( model && plusB0 && _str)) {
    biffAddf(TEN, "%s: got NULL pointer", me);
    return 1;
  }
  str = airToLower(airStrdup(_str));
  if (!str) {
    biffAddf(TEN, "%s: couldn't strdup", me);
    return 1;
  }
  mop = airMopNew();
  airMopAdd(mop, str, airFree, airMopAlways);
  sprintf(prefix, "%s:", tenModelPrefixStr);
  pre = strstr(str, prefix);
  if (pre) {
    str += strlen(prefix);
  } else {
    if (requirePrefix) {
      biffAddf(TEN, "%s: didn't see prefix \"%s\" in \"%s\"", me,
               prefix, _str);
      airMopError(mop); return 1;
    }
  }

  if ((modstr = strchr(str, '+'))) {
    *modstr = '\0';
    ++modstr;
    if (!strcmp(str, "b0")) {
      *plusB0 = AIR_TRUE;
    } else {
      biffAddf(TEN, "%s: string (\"%s\") prior to \"+\" not \"b0\"", me, str);
      airMopError(mop); return 1;
    }
  } else {
    *plusB0 = AIR_FALSE;
    modstr = str;
  }
  if (!(*model = str2model(modstr))) {
    biffAddf(TEN, "%s: didn't recognize \"%s\" as model", me, str);
    airMopError(mop); return 1;
  }
  airMopOkay(mop); 
  return 0;
}

int
tenModelFromAxisLearn(const tenModel **modelP,
                      int *plusB0,
                      const NrrdAxisInfo *axinfo) {
  static const char me[]="tenModelFromAxisLearn";
  
  if (!(modelP && plusB0 && axinfo)) {
    biffAddf(TEN, "%s: got NULL pointer", me);
    return 1;
  }
  *plusB0 = AIR_FALSE;
  /* first try to learn model from "kind" */
  if (nrrdKind3DSymMatrix == axinfo->kind
      || nrrdKind3DMaskedSymMatrix == axinfo->kind) {
    *modelP = tenModelTensor2;
  } else if (airStrlen(axinfo->label)) {
    /* try to parse from label */
    if (tenModelParse(modelP, plusB0, AIR_TRUE, axinfo->label)) {
      biffAddf(TEN, "%s: couldn't parse label \"%s\"", me, axinfo->label);
      *modelP = NULL;
      return 1;
    }    
  } else {
    biffAddf(TEN, "%s: don't have kind or label info to learn model", me);
    *modelP = NULL;
    return 1;
  }

  return 0;
}

/*
** if nB0 is given, and if parm vector is short by one (seems to be
** missing B0), then use that instead.  Otherwise parm vector has to
** be length that includes B0
**
** basic and axis info is derived from _nparm
*/
int
tenModelSimulate(Nrrd *ndwi, int typeOut,
                 tenExperSpec *espec,
                 const tenModel *model,
                 const Nrrd *_nB0,
                 const Nrrd *_nparm,
                 int keyValueSet) {
  static const char me[]="tenModelSimulate";
  double *ddwi, *parm, (*ins)(void *v, size_t I, double d);
  char *dwi;
  size_t szOut[NRRD_DIM_MAX], II, numSamp;
  const Nrrd *nB0,    /* B0 as doubles */
    *ndparm,          /* parm as doubles */
    *ndpparm,         /* parm as doubles, padded */
    *nparm;           /* final parm as doubles, padded, w/ correct B0 values */
  Nrrd *ntmp;         /* non-const pointer for working */
  airArray *mop;
  unsigned int gpsze, /* given parm size */
    ii;
  int useB0, needPad, axmap[NRRD_DIM_MAX];
  
  if (!(ndwi && espec && model /* _nB0 can be NULL */ && _nparm)) {
    biffAddf(TEN, "%s: got NULL pointer", me);
    return 1;
  }
  if (!espec->imgNum) {
    biffAddf(TEN, "%s: given espec wants 0 images, unset?", me);
    return 1;
  }

  gpsze = _nparm->axis[0].size;
  if (model->parmNum - 1 == gpsze) {
    /* got one less than needed parm #, see if we got B0 */
    if (!_nB0) {
      biffAddf(TEN, "%s: got %u parms, need %u (for %s), "
               "but didn't get B0 vol", 
               me, gpsze, model->parmNum, model->name);
      return 1;
    }
    useB0 = AIR_TRUE;
    needPad = AIR_TRUE;
  } else if (model->parmNum != gpsze) {
    biffAddf(TEN, "%s: mismatch between getting %u parms, "
             "needing %u (for %s)\n",
             me, gpsze, model->parmNum, model->name);
    return 1;
  } else {
    /* model->parmNum == gpsze */
    needPad = AIR_FALSE;
    useB0 = !!_nB0;
  }

  mop = airMopNew();
  /* get parms as doubles */
  if (nrrdTypeDouble == _nparm->type) {
    ndparm = _nparm;
  } else {
    ntmp = nrrdNew();
    airMopAdd(mop, ntmp, (airMopper)nrrdNuke, airMopAlways);
    if (nrrdConvert(ntmp, _nparm, nrrdTypeDouble)) {
      biffMovef(TEN, NRRD, "%s: couldn't convert parm to %s", me, 
                airEnumStr(nrrdType, nrrdTypeDouble));
      airMopError(mop); return 1;
    }
    ndparm = ntmp;
  }
  /* get parms the right length */
  if (!needPad) {
    ndpparm = ndparm;
  } else {
    ptrdiff_t min[NRRD_DIM_MAX], max[NRRD_DIM_MAX];
    unsigned int ax;

    ntmp = nrrdNew();
    airMopAdd(mop, ntmp, (airMopper)nrrdNuke, airMopAlways);
    for (ax=0; ax<ndparm->dim; ax++) {
      min[ax] = (!ax ? -1 : 0);
      max[ax] = ndparm->axis[ax].size-1;
    }
    if (nrrdPad_nva(ntmp, ndparm, min, max, nrrdBoundaryBleed, 0.0)) {
      biffMovef(TEN, NRRD, "%s: couldn't pad", me);
      airMopError(mop); return 1;
    }
    ndpparm = ntmp;
  }
  /* put in B0 values if needed */
  if (!useB0) {
    nparm = ndpparm;
  } else {
    if (nrrdTypeDouble == _nB0->type) {
      nB0 = _nB0;
    } else {
      ntmp = nrrdNew();
      airMopAdd(mop, ntmp, (airMopper)nrrdNuke, airMopAlways);
      if (nrrdConvert(ntmp, _nB0, nrrdTypeDouble)) {
        biffMovef(TEN, NRRD, "%s: couldn't convert B0 to %s", me, 
                  airEnumStr(nrrdType, nrrdTypeDouble));
        airMopError(mop); return 1;
      }
      nB0 = ntmp;
    }
    /* HEY: this is mostly likely a waste of memory,
       but its all complicated by const-correctness */
    ntmp = nrrdNew();
    airMopAdd(mop, ntmp, (airMopper)nrrdNuke, airMopAlways);
    if (nrrdSplice(ntmp, ndpparm, nB0, 0, 0)) {
      biffMovef(TEN, NRRD, "%s: couldn't splice in B0", me);
      airMopError(mop); return 1;
    }
    nparm = ntmp;
  }
  
  /* allocate output (and set axmap) */
  for (ii=0; ii<nparm->dim; ii++) {
    szOut[ii] = (!ii 
                 ? espec->imgNum
                 : nparm->axis[ii].size);
    axmap[ii] = (!ii
                 ? -1
                 : AIR_CAST(int, ii));
  }
  if (nrrdMaybeAlloc_nva(ndwi, typeOut, nparm->dim, szOut)) {
    biffMovef(TEN, NRRD, "%s: couldn't allocate output", me);
    airMopError(mop); return 1;
  }
  if (!(ddwi = AIR_CAST(double *, calloc(espec->imgNum, sizeof(double))))) {
    biffAddf(TEN, "%s: couldn't allocate dwi buffer", me);
    airMopError(mop); return 1;
  }
  airMopAdd(mop, ddwi, airFree, airMopAlways);
  numSamp = nrrdElementNumber(nparm)/nparm->axis[0].size;

  /* set output */
  ins = nrrdDInsert[typeOut];
  parm = AIR_CAST(double *, nparm->data);
  dwi = AIR_CAST(char *, ndwi->data);
  for (II=0; II<numSamp; II++) {
    model->simulate(ddwi, parm, espec);
    for (ii=0; ii<espec->imgNum; ii++) {
      ins(dwi, ii, ddwi[ii]);
    }
    parm += model->parmNum;
    dwi += espec->imgNum*nrrdTypeSize[typeOut];
  }

  if (keyValueSet) {
    if (tenDWMRIKeyValueFromExperSpecSet(ndwi, espec)) {
      biffAddf(TEN, "%s: trouble", me);
      airMopError(mop); return 1;
    }
  }

  if (nrrdAxisInfoCopy(ndwi, _nparm, axmap, NRRD_AXIS_INFO_SIZE_BIT)
      || nrrdBasicInfoCopy(ndwi, _nparm, 
                           NRRD_BASIC_INFO_DATA_BIT
                           | NRRD_BASIC_INFO_TYPE_BIT
                           | NRRD_BASIC_INFO_BLOCKSIZE_BIT
                           | NRRD_BASIC_INFO_DIMENSION_BIT
                           | NRRD_BASIC_INFO_CONTENT_BIT
                           | NRRD_BASIC_INFO_COMMENTS_BIT
                           | (nrrdStateKeyValuePairsPropagate
                              ? 0
                              : NRRD_BASIC_INFO_KEYVALUEPAIRS_BIT))) {
    biffMovef(TEN, NRRD, "%s: couldn't copy axis or basic info", me);
    airMopError(mop); return 1;
  }

  airMopOkay(mop);
  return 0;
}

int
tenModelSqeFit(Nrrd *nparm, Nrrd **nsqeP, 
               const tenModel *model,
               const tenExperSpec *espec, const Nrrd *ndwi,
               int knownB0, int saveB0, int typeOut,
               unsigned int maxIter, double convEps) {
  static const char me[]="tenModelSqeFit";
  double *ddwi, *dwibuff, dparm[TEN_MODEL_PARM_MAXNUM],
    (*ins)(void *v, size_t I, double d),
    (*lup)(const void *v, size_t I);
  airArray *mop;
  unsigned int saveParmNum, dwiNum, ii;
  size_t szOut[NRRD_DIM_MAX], II, numSamp;
  int axmap[NRRD_DIM_MAX];
  const char *dwi;
  char *parm;

  if (!( nparm && model && espec && ndwi )) {
    biffAddf(TEN, "%s: got NULL pointer", me);
    return 1;
  }
  if (!( nrrdTypeFloat == typeOut || nrrdTypeDouble == typeOut )) {
    biffAddf(TEN, "%s: typeOut must be %s or %s, not %s", me,
             airEnumStr(nrrdType, nrrdTypeFloat),
             airEnumStr(nrrdType, nrrdTypeDouble),
             airEnumStr(nrrdType, typeOut));
    return 1;
  }
  dwiNum = ndwi->axis[0].size;
  if (!( espec->imgNum != dwiNum )) {
    biffAddf(TEN, "%s: espec expects %u images but dwi has %u on axis 0", 
             me, espec->imgNum, AIR_CAST(unsigned int, dwiNum));
    return 1;
  }
  
  /* allocate output (and set axmap) */
  saveParmNum = saveB0 ? model->parmNum : model->parmNum-1;
  for (ii=0; ii<nparm->dim; ii++) {
    szOut[ii] = (!ii 
                 ? saveParmNum
                 : ndwi->axis[ii].size);
    axmap[ii] = (!ii
                 ? -1
                 : AIR_CAST(int, ii));
  }
  if (nrrdMaybeAlloc_nva(nparm, typeOut, ndwi->dim, szOut)) {
    biffMovef(TEN, NRRD, "%s: couldn't allocate output", me);
    airMopError(mop); return 1;
  }
  ddwi = AIR_CAST(double *, calloc(espec->imgNum, sizeof(double)));
  dwibuff = AIR_CAST(double *, calloc(espec->imgNum, sizeof(double)));
  if (!(ddwi && dwibuff)) {
    biffAddf(TEN, "%s: couldn't allocate dwi buffers", me);
    airMopError(mop); return 1;
  }
  airMopAdd(mop, ddwi, airFree, airMopAlways);
  airMopAdd(mop, dwibuff, airFree, airMopAlways);

  /* set output */
  numSamp = nrrdElementNumber(ndwi)/ndwi->axis[0].size;
  ins = nrrdDInsert[typeOut];
  lup = nrrdDLookup[ndwi->type];
  parm = AIR_CAST(char *, nparm->data);
  dwi = AIR_CAST(char *, ndwi->data);
  for (II=0; II<numSamp; II++) {
    for (ii=0; ii<dwiNum; ii++) {
      ddwi[ii] = lup(dwi, ii);
    }
    /* fit to dparm */
    for (ii=0; ii<saveParmNum; ii++) {
      ins(parm, ii, dparm[ii]);
    }
    parm += saveParmNum*nrrdTypeSize[typeOut];
    dwi += espec->imgNum*nrrdTypeSize[ndwi->type];
  }

  if (nrrdAxisInfoCopy(nparm, ndwi, axmap, NRRD_AXIS_INFO_SIZE_BIT)
      || nrrdBasicInfoCopy(nparm, ndwi,
                           NRRD_BASIC_INFO_DATA_BIT
                           | NRRD_BASIC_INFO_TYPE_BIT
                           | NRRD_BASIC_INFO_BLOCKSIZE_BIT
                           | NRRD_BASIC_INFO_DIMENSION_BIT
                           | NRRD_BASIC_INFO_CONTENT_BIT
                           | NRRD_BASIC_INFO_COMMENTS_BIT
                           | (nrrdStateKeyValuePairsPropagate
                              ? 0
                              : NRRD_BASIC_INFO_KEYVALUEPAIRS_BIT))) {
    biffMovef(TEN, NRRD, "%s: couldn't copy axis or basic info", me);
    airMopError(mop); return 1;
  }

  airMopOkay(mop);
  return 0;
}

int
tenModelNllFit(Nrrd *nparm, Nrrd **nnllP, 
               const tenModel *model,
               const tenExperSpec *espec, const Nrrd *ndwi,
               int rician, double sigma, int knownB0) {

  AIR_UNUSED(nparm);
  AIR_UNUSED(nnllP);
  AIR_UNUSED(model);
  AIR_UNUSED(espec);
  AIR_UNUSED(ndwi);
  AIR_UNUSED(rician);
  AIR_UNUSED(sigma);
  AIR_UNUSED(knownB0);

  return 0;
}
