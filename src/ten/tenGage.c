/*
  teem: Gordon Kindlmann's research software
  Copyright (C) 2004, 2003, 2002, 2001, 2000, 1999, 1998 University of Utah

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

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

gageItemEntry
_tenGageTable[TEN_GAGE_ITEM_MAX+1] = {
  /* enum value              len,deriv,  prereqs,                                       parent item, index*/
  {tenGageTensor,              7,  0,  {-1, -1, -1, -1, -1},                                    -1,  -1},
  {tenGageTrace,               1,  0,  {tenGageTensor, -1, -1, -1, -1},                         -1,  -1},
  {tenGageB,                   1,  0,  {tenGageTensor, -1, -1, -1, -1},                         -1,  -1},
  {tenGageDet,                 1,  0,  {tenGageTensor, -1, -1, -1, -1},                         -1,  -1},
  {tenGageS,                   1,  0,  {tenGageTensor, -1, -1, -1, -1},                         -1,  -1},
  {tenGageQ,                   1,  0,  {tenGageS, tenGageB, -1, -1, -1},                        -1,  -1},
  {tenGageFA,                  1,  0,  {tenGageQ, tenGageS, -1, -1, -1},                        -1,  -1},
  {tenGageR,                   1,  0,  {tenGageTrace, tenGageB, tenGageDet, tenGageS, -1},      -1,  -1},
  {tenGageEval,                3,  0,  {tenGageTensor, -1, -1, -1, -1},                         -1,  -1},
  {tenGageEval0,               1,  0,  {tenGageEval, -1, -1, -1, -1},                  tenGageEval,   0},
  {tenGageEval1,               1,  0,  {tenGageEval, -1, -1, -1, -1},                  tenGageEval,   1},
  {tenGageEval2,               1,  0,  {tenGageEval, -1, -1, -1, -1},                  tenGageEval,   2},
  {tenGageEvec,                9,  0,  {tenGageTensor, -1, -1, -1, -1},                         -1,  -1},
  {tenGageEvec0,               3,  0,  {tenGageEvec, -1, -1, -1, -1},                  tenGageEvec,   0},
  {tenGageEvec1,               3,  0,  {tenGageEvec, -1, -1, -1, -1},                  tenGageEvec,   3},
  {tenGageEvec2,               3,  0,  {tenGageEvec, -1, -1, -1, -1},                  tenGageEvec,   6},
  {tenGageTensorGrad,         21,  1,  {-1, -1, -1, -1, -1},                                    -1,  -1},
  {tenGageTraceGradVec,        3,  1,  {tenGageTensor, tenGageTensorGrad, -1, -1, -1},          -1,  -1},
  {tenGageTraceGradMag,        1,  1,  {tenGageTraceGradVec, -1, -1, -1, -1},                   -1,  -1},
  {tenGageTraceNormal,         3,  1,  {tenGageTraceGradVec, tenGageTraceGradMag, -1, -1, -1},  -1,  -1},
  {tenGageBGradVec,            3,  1,  {tenGageTensor, tenGageTensorGrad, -1, -1, -1},          -1,  -1},
  {tenGageBGradMag,            1,  1,  {tenGageBGradVec, -1, -1, -1, -1},                       -1,  -1},
  {tenGageBNormal,             3,  1,  {tenGageBGradVec, tenGageBGradMag, -1, -1, -1},          -1,  -1},
  {tenGageDetGradVec,          3,  1,  {tenGageTensor, tenGageTensorGrad, -1, -1, -1},          -1,  -1},
  {tenGageDetGradMag,          1,  1,  {tenGageDetGradVec, -1, -1, -1, -1},                     -1,  -1},
  {tenGageDetNormal,           3,  1,  {tenGageDetGradVec, tenGageDetGradMag, -1, -1, -1},      -1,  -1},
  {tenGageSGradVec,            3,  1,  {tenGageTensor, tenGageTensorGrad, -1, -1, -1},          -1,  -1},
  {tenGageSGradMag,            1,  1,  {tenGageSGradVec, -1, -1, -1, -1},                       -1,  -1},
  {tenGageSNormal,             3,  1,  {tenGageSGradVec, tenGageSGradMag, -1, -1, -1},          -1,  -1},
  {tenGageQGradVec,            3,  1,  {tenGageTensor, tenGageTensorGrad, -1, -1, -1},          -1,  -1},
  {tenGageQGradMag,            1,  1,  {tenGageQGradVec, -1, -1, -1, -1},                       -1,  -1},
  {tenGageQNormal,             3,  1,  {tenGageQGradVec, tenGageQGradMag, -1, -1, -1},          -1,  -1},
  {tenGageFAGradVec,           3,  1,  {tenGageTensor, tenGageTensorGrad, -1, -1, -1},          -1,  -1},
  {tenGageFAGradMag,           1,  1,  {tenGageFAGradVec, -1, -1, -1, -1},                      -1,  -1},
  {tenGageFANormal,            3,  1,  {tenGageFAGradVec, tenGageFAGradMag, -1, -1, -1},        -1,  -1},
  {tenGageRGradVec,            3,  1,  {tenGageTensor, tenGageTensorGrad, -1, -1, -1},          -1,  -1},
  {tenGageRGradMag,            1,  1,  {tenGageRGradVec, -1, -1, -1, -1},                       -1,  -1},
  {tenGageRNormal,             3,  1,  {tenGageRGradVec, tenGageRGradMag, -1, -1, -1},          -1,  -1},
  {tenGageAniso, TEN_ANISO_MAX+1,  0,  {tenGageEval0, tenGageEval1, tenGageEval2, -1, -1},      -1,  -1}
};

void
_tenGageIv3Print (FILE *file, gageContext *ctx, gagePerVolume *pvl) {
  
  fprintf(file, "_tenGageIv3Print() not implemented\n");
}

void
_tenGageFilter (gageContext *ctx, gagePerVolume *pvl) {
  char me[]="_tenGageFilter";
  gage_t *fw00, *fw11, *fw22, *tensor, *tgrad;
  int fd;

  fd = GAGE_FD(ctx);
  tensor = pvl->directAnswer[tenGageTensor];
  tgrad = pvl->directAnswer[tenGageTensorGrad];
  if (!ctx->parm.k3pack) {
    fprintf(stderr, "!%s: sorry, 6pack filtering not implemented\n", me);
    return;
  }
  fw00 = ctx->fw + fd*3*gageKernel00;
  fw11 = ctx->fw + fd*3*gageKernel11;
  fw22 = ctx->fw + fd*3*gageKernel22;
  /* perform the filtering */
  switch (fd) {
  case 2:
#define DOIT_2(J) \
      gageScl3PFilter2(pvl->iv3 + J*8, pvl->iv2 + J*4, pvl->iv1 + J*2, \
		       fw00, fw11, fw22, \
                       tensor + J, tgrad + J*3, NULL, \
		       pvl->needD[0], pvl->needD[1], AIR_FALSE)
    DOIT_2(0); DOIT_2(1); DOIT_2(2); DOIT_2(3);
    DOIT_2(4); DOIT_2(5); DOIT_2(6); 
    break;
  case 4:
#define DOIT_4(J) \
      gageScl3PFilter4(pvl->iv3 + J*64, pvl->iv2 + J*16, pvl->iv1 + J*4, \
		       fw00, fw11, fw22, \
                       tensor + J, tgrad + J*3, NULL, \
		       pvl->needD[0], pvl->needD[1], AIR_FALSE)
    DOIT_4(0); DOIT_4(1); DOIT_4(2); DOIT_4(3);
    DOIT_4(4); DOIT_4(5); DOIT_4(6); 
    break;
  default:
#define DOIT_N(J)\
      gageScl3PFilterN(fd, \
                       pvl->iv3 + J*fd*fd*fd, \
                       pvl->iv2 + J*fd*fd, pvl->iv1 + J*fd, \
		       fw00, fw11, fw22, \
                       tensor + J, tgrad + J*3, NULL, \
		       pvl->needD[0], pvl->needD[1], AIR_FALSE)
    DOIT_N(0); DOIT_N(1); DOIT_N(2); DOIT_N(3);
    DOIT_N(4); DOIT_N(5); DOIT_N(6); 
    break;
  }

  return;
}

void
_tenGageAnswer (gageContext *ctx, gagePerVolume *pvl) {
  /* char me[]="_tenGageAnswer"; */
  gage_t *tenAns, *evalAns, *evecAns, *vecTmp=NULL, magTmp=0,
    tmp0, tmp1,
    dtConf=0, dtA=0, dtB=0, dtC=0, dtD=0, dtE=0, dtF=0,
    *gradDtA=NULL, *gradDtB=NULL, *gradDtC=NULL,
    *gradDtD=NULL, *gradDtE=NULL, *gradDtF=NULL,
    gradCbA[3], *gradCbS=NULL, *gradCbB=NULL, *gradCbQ=NULL,
    cbQ=0, cbA=0, cbB=0, cbC=0, cbS=0;

#if !GAGE_TYPE_FLOAT
  int ci;
  float tenAnsF[7], evalAnsF[3], evecAnsF[9], aniso[TEN_ANISO_MAX+1];
#endif

  tenAns = pvl->directAnswer[tenGageTensor];
  evalAns = pvl->directAnswer[tenGageEval];
  evecAns = pvl->directAnswer[tenGageEvec];
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageTensor)) {
    /* done if doV */
    dtConf = tenAns[0];
    dtA = tenAns[1];
    dtB = tenAns[2];
    dtC = tenAns[3];
    dtD = tenAns[4];
    dtE = tenAns[5];
    dtF = tenAns[6];
    if (ctx->verbose) {
      fprintf(stderr, "tensor = (%g) %g %g %g   %g %g   %g\n", tenAns[0],
	      dtA, dtB, dtC, dtD, dtE, dtF);
    }
  }
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageTrace)) {
    cbA = -(pvl->directAnswer[tenGageTrace][0] = dtA + dtD + dtF);
  }
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageB)) {
    cbB = pvl->directAnswer[tenGageB][0] = 
      dtA*dtD + dtA*dtF + dtD*dtF - dtB*dtB - dtC*dtC - dtE*dtE;
  }
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageDet)) {
    cbC = -(pvl->directAnswer[tenGageDet][0] = 
	    2*dtB*dtC*dtE + dtA*dtD*dtF 
	    - dtC*dtC*dtD - dtA*dtE*dtE - dtB*dtB*dtF);
  }
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageS)) {
    cbS = (pvl->directAnswer[tenGageS][0] = 
	   dtA*dtA + dtD*dtD + dtF*dtF + 2*dtB*dtB + 2*dtC*dtC + 2*dtE*dtE);
  }
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageQ)) {
    cbQ = pvl->directAnswer[tenGageQ][0] = (cbS - cbB)/9;
  }
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageFA)) {
    pvl->directAnswer[tenGageFA][0] = 3*sqrt(cbQ/cbS);
  }
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageR)) {
    pvl->directAnswer[tenGageR][0] = (5*cbA*cbB - 27*cbC - 2*cbA*cbS)/54;
  }
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageEvec)) {
    /* we do the longer process to get eigenvectors, and in the process
       we always find the eigenvalues, whether or not they were asked for */
#if GAGE_TYPE_FLOAT
    tenEigensolve(evalAns, evecAns, tenAns);
#else
    TEN_LIST_COPY(tenAnsF, tenAns);
    tenEigensolve(evalAnsF, evecAnsF, tenAnsF);
    ELL_3V_COPY(evalAns, evalAnsF);
    ELL_3M_COPY(evecAns, evecAnsF);
#endif
  } else if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageEval)) {
    /* else eigenvectors are NOT needed, but eigenvalues ARE needed */
#if GAGE_TYPE_FLOAT
    tenEigensolve(evalAns, NULL, tenAns);
#else
    TEN_LIST_COPY(tenAnsF, tenAns);
    tenEigensolve(evalAnsF, NULL, tenAnsF);
    ELL_3V_COPY(evalAns, evalAnsF);
#endif
  }
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageTensorGrad)) {
    /* done if doD1 */
    vecTmp = pvl->directAnswer[tenGageTensorGrad];
    gradDtA = vecTmp + 1*3;
    gradDtB = vecTmp + 2*3;
    gradDtC = vecTmp + 3*3;
    gradDtD = vecTmp + 4*3;
    gradDtE = vecTmp + 5*3;
    gradDtF = vecTmp + 6*3;
  }

  /* --- Trace --- */
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageTraceGradVec)) {
    vecTmp = pvl->directAnswer[tenGageTraceGradVec];
    ELL_3V_ADD3(vecTmp, gradDtA, gradDtD, gradDtF);
    ELL_3V_SCALE(gradCbA, -1, vecTmp);
  }
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageTraceGradMag)) {
    magTmp = pvl->directAnswer[tenGageTraceGradMag][0] = ELL_3V_LEN(vecTmp);
  }
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageTraceNormal)) {
    ELL_3V_SCALE(pvl->directAnswer[tenGageTraceNormal], 1.0/magTmp, vecTmp);
  }
  /* --- B --- */
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageBGradVec)) {
    gradCbB = vecTmp = pvl->directAnswer[tenGageBGradVec];
    ELL_3V_SCALE_ADD6(vecTmp, 
		      dtD + dtF, gradDtA,
		      -2*dtB, gradDtB,
		      -2*dtC, gradDtC,
		      dtA + dtF, gradDtD,
		      -2*dtE, gradDtE,
		      dtA + dtD, gradDtF);
  }
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageBGradMag)) {
    magTmp = pvl->directAnswer[tenGageBGradMag][0] = ELL_3V_LEN(vecTmp);
  }
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageBNormal)) {
    ELL_3V_SCALE(pvl->directAnswer[tenGageBNormal], 1.0/magTmp, vecTmp);
  }
  /* --- Det --- */
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageDetGradVec)) {
    vecTmp = pvl->directAnswer[tenGageDetGradVec];
    ELL_3V_SCALE_ADD6(vecTmp,
		      dtD*dtF - dtE*dtE, gradDtA,
		      2*(dtC*dtE - dtB*dtF), gradDtB,
		      2*(dtB*dtE - dtC*dtD), gradDtC,
		      dtA*dtF - dtC*dtC, gradDtD,
		      2*(dtB*dtC - dtA*dtE), gradDtE,
		      dtA*dtD - dtB*dtB, gradDtF);
  }
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageDetGradMag)) {
    magTmp = pvl->directAnswer[tenGageDetGradMag][0] = ELL_3V_LEN(vecTmp);
  }
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageDetNormal)) {
    ELL_3V_SCALE(pvl->directAnswer[tenGageDetNormal], 1.0/magTmp, vecTmp);
  }
  /* --- S --- */
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageSGradVec)) {
    gradCbS = vecTmp = pvl->directAnswer[tenGageSGradVec];
    ELL_3V_SCALE_ADD6(vecTmp,
		      2*dtA, gradDtA,
		      4*dtB, gradDtB,
		      4*dtC, gradDtC,
		      2*dtD, gradDtD,
		      4*dtE, gradDtE,
		      2*dtF, gradDtF);
  }
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageSGradMag)) {
    magTmp = pvl->directAnswer[tenGageSGradMag][0] = ELL_3V_LEN(vecTmp);
  }
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageSNormal)) {
    ELL_3V_SCALE(pvl->directAnswer[tenGageSNormal], 1.0/magTmp, vecTmp);
  }
  /* --- Q --- */
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageQGradVec)) {
    gradCbQ = vecTmp = pvl->directAnswer[tenGageQGradVec];
    ELL_3V_SCALE_ADD2(vecTmp,
		      1.0/9.0, gradCbS, 
		      -1.0/9.0, gradCbB);
  }
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageQGradMag)) {
    magTmp = pvl->directAnswer[tenGageQGradMag][0] = ELL_3V_LEN(vecTmp);
  }
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageQNormal)) {
    ELL_3V_SCALE(pvl->directAnswer[tenGageQNormal], 1.0/magTmp, vecTmp);
  }
  /* --- FA --- */
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageFAGradVec)) {
    vecTmp = pvl->directAnswer[tenGageFAGradVec];
    tmp0 = 9.0/(2*pvl->directAnswer[tenGageFA][0]*cbS);
    tmp1 = -tmp0*cbQ/cbS;
    ELL_3V_SCALE_ADD2(vecTmp,
		      tmp0, gradCbQ, 
		      tmp1, gradCbS);
  }
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageFAGradMag)) {
    magTmp = pvl->directAnswer[tenGageFAGradMag][0] = ELL_3V_LEN(vecTmp);
  }
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageFANormal)) {
    ELL_3V_SCALE(pvl->directAnswer[tenGageFANormal], 1.0/magTmp, vecTmp);
  }
  /* --- R --- */
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageRGradVec)) {
    vecTmp = pvl->directAnswer[tenGageRGradVec];
    ELL_3V_SCALE_ADD4(vecTmp,
		      (5*cbB - 2*cbS)/54.0, gradCbA,
		      1.0/2.0, pvl->directAnswer[tenGageDetGradVec],
		      5.0*cbA/54.0, gradCbB,
		      -cbA/27.0, gradCbS);
  }
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageRGradMag)) {
    magTmp = pvl->directAnswer[tenGageRGradMag][0] = ELL_3V_LEN(vecTmp);
  }
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageRNormal)) {
    ELL_3V_SCALE(pvl->directAnswer[tenGageRNormal], 1.0/magTmp, vecTmp);
  }
  /* --- Aniso --- */
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageAniso)) {
#if GAGE_TYPE_FLOAT
    tenAnisoCalc(pvl->directAnswer[tenGageAniso], evalAns);
#else
    tenAnisoCalc(aniso, evalAnsF);
    for (ci=0; ci<=TEN_ANISO_MAX; ci++) {
      pvl->directAnswer[tenGageAniso][ci] = aniso[ci];
    }
#endif
  }
  return;
}

gageKind
_tenGageKind = {
  "tensor",
  &_tenGage,
  1,
  7,
  TEN_GAGE_ITEM_MAX,
  _tenGageTable,
  _tenGageIv3Print,
  _tenGageFilter,
  _tenGageAnswer
};
gageKind *
tenGageKind = &_tenGageKind;
