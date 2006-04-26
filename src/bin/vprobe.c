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


#include <stdio.h>

#include <teem/biff.h>
#include <teem/hest.h>
#include <teem/nrrd.h>
#include <teem/gage.h>
#include <teem/ten.h>

int
probeParseKind(void *ptr, char *str, char err[AIR_STRLEN_HUGE]) {
  char me[] = "probeParseKind";
  gageKind **kindP;
  
  if (!(ptr && str)) {
    sprintf(err, "%s: got NULL pointer", me);
    return 1;
  }
  kindP = (gageKind **)ptr;
  airToLower(str);
  if (!strcmp("scalar", str)) {
    *kindP = gageKindScl;
  } else if (!strcmp("vector", str)) {
    *kindP = gageKindVec;
  } else if (!strcmp("tensor", str)) {
    *kindP = tenGageKind;
  } else if (!strcmp("dwi", str)) {
    *kindP = tenDwiGageKindNew();
  } else {
    sprintf(err, "%s: not \"scalar\", \"vector\", \"tensor\", or \"dwi\"", me);
    return 1;
  }

  return 0;
}

void *
probeParseKindDestroy(void *ptr) {
  gageKind *kind;
  
  if (ptr) {
    kind = AIR_CAST(gageKind *, ptr);
    if (!strcmp(TEN_DWI_GAGE_KIND_NAME, kind->name)) {
      tenDwiGageKindNix(kind);
    }
  }
  return NULL;
}

hestCB probeKindHestCB = {
  sizeof(gageKind *),
  "kind",
  probeParseKind,
  probeParseKindDestroy
}; 

char *probeInfo = ("Shows off the functionality of the gage library. "
                   "Uses gageProbe() to query various kinds of volumes "
                   "to learn various measured or derived quantities. ");

int
main(int argc, char *argv[]) {
  gageKind *kind;
  char *me, *outS, *whatS, *err, hackKeyStr[]="TEEM_VPROBE_HACK_ZI",
    *hackValStr;
  hestParm *hparm;
  hestOpt *hopt = NULL;
  NrrdKernelSpec *k00, *k11, *k22;
  int what, E=0, otype, renorm, hackSet;
  unsigned int iBaseDim, oBaseDim, axi;
  const double *answer;
  const char *key=NULL;
  Nrrd *nin, *nout;
  Nrrd *ngrad=NULL, *nbmat=NULL;
  size_t ai, ansLen, idx, xi, yi, zi, six, siy, siz, sox, soy, soz;
  double bval=0, gmc;
  gageContext *ctx;
  gagePerVolume *pvl;
  double t0, t1, x, y, z, scale[3], rscl[3], min[3], maxOut[3], maxIn[3];
  airArray *mop;
  unsigned int hackZi, *skip, skipNum, skipIdx;
  double (*ins)(void *v, size_t I, double d);

  mop = airMopNew();
  me = argv[0];
  hparm = hestParmNew();
  airMopAdd(mop, hparm, AIR_CAST(airMopper, hestParmFree), airMopAlways);
  hparm->elideSingleOtherType = AIR_TRUE;
  hestOptAdd(&hopt, "i", "nin", airTypeOther, 1, 1, &nin, NULL,
             "input volume", NULL, NULL, nrrdHestNrrd);
  hestOptAdd(&hopt, "k", "kind", airTypeOther, 1, 1, &kind, NULL,
             "\"kind\" of volume (\"scalar\", \"vector\", "
             "\"tensor\", or \"dwi\")",
             NULL, NULL, &probeKindHestCB);
  hestOptAdd(&hopt, "q", "query", airTypeString, 1, 1, &whatS, NULL,
             "the quantity (scalar, vector, or matrix) to learn by probing");
  hestOptAdd(&hopt, "s", "sclX sclY sxlZ", airTypeDouble, 3, 3, scale,
             "1.0 1.0 1.0",
             "scaling factor for resampling on each axis "
             "(>1.0 : supersampling)");
  hestOptAdd(&hopt, "k00", "kern00", airTypeOther, 1, 1, &k00,
             "tent", "kernel for gageKernel00",
             NULL, NULL, nrrdHestKernelSpec);
  hestOptAdd(&hopt, "k11", "kern11", airTypeOther, 1, 1, &k11,
             "cubicd:1,0", "kernel for gageKernel11",
             NULL, NULL, nrrdHestKernelSpec);
  hestOptAdd(&hopt, "k22", "kern22", airTypeOther, 1, 1, &k22,
             "cubicdd:1,0", "kernel for gageKernel22",
             NULL, NULL, nrrdHestKernelSpec);
  hestOptAdd(&hopt, "rn", NULL, airTypeInt, 0, 0, &renorm, NULL,
             "renormalize kernel weights at each new sample location. "
             "\"Accurate\" kernels don't need this; doing it always "
             "makes things go slower");
  hestOptAdd(&hopt, "gmc", "min gradmag", airTypeDouble, 1, 1, &gmc,
             "0.0", "For curvature-based queries, use zero when gradient "
             "magnitude is below this");
  hestOptAdd(&hopt, "t", "type", airTypeEnum, 1, 1, &otype, "float",
             "type of output volume", NULL, nrrdType);
  hestOptAdd(&hopt, "o", "nout", airTypeString, 1, 1, &outS, "-",
             "output volume");
  hestParseOrDie(hopt, argc-1, argv+1, hparm,
                 me, probeInfo, AIR_TRUE, AIR_TRUE, AIR_TRUE);
  airMopAdd(mop, hopt, AIR_CAST(airMopper, hestOptFree), airMopAlways);
  airMopAdd(mop, hopt, AIR_CAST(airMopper, hestParseFree), airMopAlways);

  what = airEnumVal(kind->enm, whatS);
  if (-1 == what) {
    /* -1 indeed always means "unknown" for any gageKind */
    fprintf(stderr, "%s: couldn't parse \"%s\" as measure of \"%s\" volume\n",
            me, whatS, kind->name);
    hestUsage(stderr, hopt, me, hparm);
    hestGlossary(stderr, hopt, hparm);
    airMopError(mop);
    return 1;
  }

  /* special set-up required for DWI kind */
  if (!strcmp(TEN_DWI_GAGE_KIND_NAME, kind->name)) {
    tenDwiGageKindData *kindData;

    kindData = AIR_CAST(tenDwiGageKindData *, kind->data);
    if (tenDWMRIKeyValueParse(&ngrad, &nbmat, &bval, &skip, &skipNum, nin)) {
      airMopAdd(mop, err = biffGetDone(TEN), airFree, airMopAlways);
      fprintf(stderr, "%s: trouble parsing DWI info:\n%s\n", me, err);
      airMopError(mop); return 1;
    }
    if (!E) tenEstimateVerboseSet(kindData->tec, AIR_TRUE);
    if (!E) tenEstimateNegEvalShiftSet(kindData->tec, AIR_TRUE);
    if (!E) E |= tenEstimateMethodSet(kindData->tec, tenEstimateMethodLLS);
    if (!E) E |= tenEstimateValueMinSet(kindData->tec, 1.0);
    if (ngrad) {
      if (!E) E |= tenEstimateGradientsSet(kindData->tec, ngrad, bval, 
					   AIR_FALSE);
      tenDwiGageKindNumSet(kind, ngrad->axis[1].size);
      kindData->ngrad = nrrdNew();
      nrrdCopy(kindData->ngrad, ngrad);
      if (!E) airMopAdd(mop, ngrad, (airMopper)nrrdNuke, airMopAlways);
    } else {
      if (!E) E |= tenEstimateBMatricesSet(kindData->tec, nbmat, bval,
					   AIR_FALSE);
      tenDwiGageKindNumSet(kind, nbmat->axis[1].size);
      kindData->ngrad = NULL;
      if (!E) airMopAdd(mop, nbmat, (airMopper)nrrdNuke, airMopAlways);
    }
    for (skipIdx=0; skipIdx<skipNum; skipIdx++) {
      if (!E) E |= tenEstimateSkipSet(kindData->tec, skip[skipIdx], AIR_TRUE);
    }
    if (E) {
      airMopAdd(mop, err = biffGetDone(key), airFree, airMopAlways);
      fprintf(stderr, "%s: trouble setting grad/bmat info:\n%s\n", me, err);
      airMopError(mop); return 1;
    }
  }

  /***
  **** Except for the gageProbe() call in the inner loop below,
  **** and the gageContextNix() call at the very end, all the gage
  **** calls which set up (and take down) the context and state are here.
  ***/
  ctx = gageContextNew();
  airMopAdd(mop, ctx, AIR_CAST(airMopper, gageContextNix), airMopAlways);
  gageParmSet(ctx, gageParmGradMagCurvMin, gmc);
  gageParmSet(ctx, gageParmVerbose, 1);
  gageParmSet(ctx, gageParmRenormalize, renorm ? AIR_TRUE : AIR_FALSE);
  gageParmSet(ctx, gageParmCheckIntegrals, AIR_TRUE);
  E = 0;
  if (!E) E |= !(pvl = gagePerVolumeNew(ctx, nin, kind));
  if (!E) E |= gagePerVolumeAttach(ctx, pvl);
  if (!E) E |= gageKernelSet(ctx, gageKernel00, k00->kernel, k00->parm);
  if (!E) E |= gageKernelSet(ctx, gageKernel11, k11->kernel, k11->parm); 
  if (!E) E |= gageKernelSet(ctx, gageKernel22, k22->kernel, k22->parm);
  if (!E) E |= gageQueryItemOn(ctx, pvl, what);
  if (!E) E |= gageUpdate(ctx);
  if (E) {
    airMopAdd(mop, err = biffGetDone(GAGE), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble:\n%s\n", me, err);
    airMopError(mop);
    return 1;
  }
  answer = gageAnswerPointer(ctx, pvl, what);
  gageParmSet(ctx, gageParmVerbose, 0);
  /***
  **** end gage setup.
  ***/

  ansLen = kind->table[what].answerLength;
  iBaseDim = kind->baseDim;
  oBaseDim = 1 == ansLen ? 0 : 1;
  six = nin->axis[0+iBaseDim].size;
  siy = nin->axis[1+iBaseDim].size;
  siz = nin->axis[2+iBaseDim].size;
  sox = AIR_CAST(size_t, scale[0]*six);
  soy = AIR_CAST(size_t, scale[1]*siy);
  soz = AIR_CAST(size_t, scale[2]*siz);
  rscl[0] = AIR_CAST(double, six)/sox;
  rscl[1] = AIR_CAST(double, siy)/soy;
  rscl[2] = AIR_CAST(double, siz)/soz;

  fprintf(stderr, "%s: kernel support = %d^3 samples\n", me,
          2*ctx->radius);
  fprintf(stderr, "%s: effective scaling is %g %g %g\n", me,
          rscl[0], rscl[1], rscl[2]);
  if (ansLen > 1) {
    fprintf(stderr, "%s: creating " _AIR_SIZE_T_CNV " x " _AIR_SIZE_T_CNV
            " x " _AIR_SIZE_T_CNV " x " _AIR_SIZE_T_CNV " output\n", 
            me, ansLen, sox, soy, soz);
    if (!E) E |= nrrdMaybeAlloc_va(nout=nrrdNew(), otype, 4,
                                   ansLen, sox, soy, soz);
  } else {
    fprintf(stderr, "%s: creating " _AIR_SIZE_T_CNV " x " _AIR_SIZE_T_CNV
            " x " _AIR_SIZE_T_CNV " output\n", me, sox, soy, soz);
    if (!E) E |= nrrdMaybeAlloc_va(nout=nrrdNew(), otype, 3,
                                   sox, soy, soz);
  }
  airMopAdd(mop, nout, AIR_CAST(airMopper, nrrdNuke), airMopAlways);
  if (E) {
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble:\n%s\n", me, err);
    airMopError(mop);
    return 1;
  }

  hackSet = nrrdGetenvUInt(&hackZi, &hackValStr, hackKeyStr);
  if (AIR_FALSE == hackSet) {
    fprintf(stderr, "%s: couldn't parse value of \"%s\" (\"%s\") as uint\n",
            me, hackKeyStr, hackValStr);
    airMopError(mop);
    return 1;
  }
  if (AIR_TRUE == hackSet) {
    fprintf(stderr, "%s: %s hack on: will only measure Zi=%u\n", 
            me, hackKeyStr, hackZi);
  }

  if (nrrdCenterCell == ctx->shape->center) {
    ELL_3V_SET(min, -0.5, -0.5, -0.5);
    ELL_3V_SET(maxOut, sox-0.5, soy-0.5, soz-0.5);
    ELL_3V_SET(maxIn,  six-0.5, siy-0.5, siz-0.5);
  } else {
    ELL_3V_SET(min, 0, 0, 0);
    ELL_3V_SET(maxOut, sox-1, soy-1, soz-1);
    ELL_3V_SET(maxIn,  six-1, siy-1, siz-1);
  }
  t0 = airTime();
  ins = nrrdDInsert[nout->type];
  for (zi=0; zi<soz; zi++) {
    fprintf(stderr, " " _AIR_SIZE_T_CNV "/" _AIR_SIZE_T_CNV,
            zi, soz-1); fflush(stderr);

    if (AIR_TRUE == hackSet) {
      if (hackZi != zi) {
        continue;
      }
    }

    z = AIR_AFFINE(min[2], zi, maxOut[2], min[2], maxIn[2]);
    for (yi=0; yi<soy; yi++) {
      y = AIR_AFFINE(min[1], yi, maxOut[1], min[1], maxIn[1]);
      for (xi=0; xi<sox; xi++) {
        x = AIR_AFFINE(min[0], xi, maxOut[0], min[0], maxIn[0]);
        idx = xi + sox*(yi + soy*zi);
        ctx->verbose = 0*( (!xi && !yi && !zi) ||
                           /* ((100 == xi) && (8 == yi) && (8 == zi)) */
                           ((61 == xi) && (51 == yi) && (46 == zi))
                           /* ((40==xi) && (30==yi) && (62==zi)) || */
                           /* ((40==xi) && (30==yi) && (63==zi)) */ );
        if (gageProbe(ctx, x, y, z)) {
          fprintf(stderr, 
                  "%s: trouble at i=(" _AIR_SIZE_T_CNV "," _AIR_SIZE_T_CNV
                  "," _AIR_SIZE_T_CNV ") -> f=(%g,%g,%g):\n%s\n(%d)\n",
                  me, xi, yi, zi, x, y, z,
                  ctx->errStr, ctx->errNum);
          airMopError(mop);
          return 1;
        }
        if (1 == ansLen) {
          ins(nout->data, idx, *answer);
        } else {
          for (ai=0; ai<=ansLen-1; ai++) {
            ins(nout->data, ai + ansLen*idx, answer[ai]);
          }
        }
      }
    }
  }

  /* HEY: this isn't actually correct in general, but is true
     for gageKindScl and gageKindVec */
  nrrdContentSet_va(nout, "probe", nin, "%s", airEnumStr(kind->enm, what));

  for (axi=0; axi<3; axi++) {
    nout->axis[axi+oBaseDim].label = airStrdup(nin->axis[axi+iBaseDim].label);
    nout->axis[axi+oBaseDim].center = ctx->shape->center;
  }

  nrrdBasicInfoCopy(nout, nin, (NRRD_BASIC_INFO_DATA_BIT
                                | NRRD_BASIC_INFO_TYPE_BIT
                                | NRRD_BASIC_INFO_BLOCKSIZE_BIT
                                | NRRD_BASIC_INFO_DIMENSION_BIT
                                | NRRD_BASIC_INFO_CONTENT_BIT
                                | NRRD_BASIC_INFO_SPACEORIGIN_BIT
                                | NRRD_BASIC_INFO_OLDMIN_BIT
                                | NRRD_BASIC_INFO_OLDMAX_BIT
                                | NRRD_BASIC_INFO_COMMENTS_BIT
                                | NRRD_BASIC_INFO_KEYVALUEPAIRS_BIT));
  if (ctx->shape->fromOrientation) {
    nrrdSpaceSet(nout, nin->space);
    nrrdSpaceVecCopy(nout->spaceOrigin, nin->spaceOrigin);
    for (axi=0; axi<3; axi++) {
      nrrdSpaceVecScale(nout->axis[axi+oBaseDim].spaceDirection,
                        rscl[axi],
                        nin->axis[axi+iBaseDim].spaceDirection);
      z = AIR_AFFINE(min[axi], 0, maxOut[axi], min[axi], maxIn[axi]);
      nrrdSpaceVecScaleAdd2(nout->spaceOrigin,
                            1.0, nout->spaceOrigin,
                            z, nin->axis[axi+iBaseDim].spaceDirection);
    }
  } else {
    for (axi=0; axi<3; axi++) {
      nout->axis[axi+oBaseDim].spacing = 
        rscl[axi]*nin->axis[axi+iBaseDim].spacing;
    }
  }

  fprintf(stderr, "\n");
  t1 = airTime();
  fprintf(stderr, "probe rate = %g KHz\n", sox*soy*soz/(1000.0*(t1-t0)));
  if (nrrdSave(outS, nout, NULL)) {
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble saving output:\n%s\n", me, err);
    airMopError(mop);
    return 1;
  }

  airMopOkay(mop);
  return 0;
}
