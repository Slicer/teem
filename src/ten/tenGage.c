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
  {tenGageFrobTensor,          1,  0,  {tenGageTensor, -1, -1, -1, -1},                         -1,  -1},
  {tenGageEval,                3,  0,  {tenGageTensor, -1, -1, -1, -1},                         -1,  -1},
  {tenGageEval0,               1,  0,  {tenGageEval, -1, -1, -1, -1},                  tenGageEval,   0},
  {tenGageEval1,               1,  0,  {tenGageEval, -1, -1, -1, -1},                  tenGageEval,   1},
  {tenGageEval2,               1,  0,  {tenGageEval, -1, -1, -1, -1},                  tenGageEval,   2},
  {tenGageEvec,                9,  0,  {tenGageTensor, -1, -1, -1, -1},                         -1,  -1},
  {tenGageEvec0,               3,  0,  {tenGageEvec, -1, -1, -1, -1},                  tenGageEvec,   0},
  {tenGageEvec1,               3,  0,  {tenGageEvec, -1, -1, -1, -1},                  tenGageEvec,   3},
  {tenGageEvec2,               3,  0,  {tenGageEvec, -1, -1, -1, -1},                  tenGageEvec,   6},
  {tenGageTensorGrad,         21,  1,  {-1, -1, -1, -1, -1},                                    -1,  -1},
  {tenGageQ,                   1,  0,  {-1, -1, -1, -1, -1},                                    -1,  -1},
  {tenGageQGradVec,            3,  1,  {tenGageTensor, tenGageTensorGrad, -1, -1, -1},          -1,  -1},
  {tenGageQGradMag,            1,  1,  {tenGageQGradVec, -1, -1, -1, -1},                       -1,  -1},
  {tenGageQNormal,             3,  1,  {tenGageQGradVec, tenGageQGradMag, -1, -1, -1},          -1,  -1},
  {tenGageMultiGrad,           9,  1,  {tenGageTensorGrad, -1, -1, -1, -1},                     -1,  -1},
  {tenGageFrobMG,              1,  1,  {tenGageMultiGrad, -1, -1, -1, -1},                      -1,  -1},
  {tenGageMGEval,              3,  1,  {tenGageMultiGrad, -1, -1, -1, -1},                      -1,  -1},
  {tenGageMGEvec,              9,  1,  {tenGageMultiGrad, -1, -1, -1, -1},                      -1,  -1},
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
  gage_t *tenAns, *tgradAns, *QgradAns, *evalAns, *evecAns, tmptmp=0,
    dtA=0, dtB=0, dtC=0, dtD=0, dtE=0, dtF=0, cbA, cbB;

#if !GAGE_TYPE_FLOAT
  int ci;
  float tenAnsF[7], evalAnsF[3], evecAnsF[9], aniso[TEN_ANISO_MAX+1];
#endif

  tenAns = pvl->directAnswer[tenGageTensor];
  tgradAns = pvl->directAnswer[tenGageTensorGrad];
  QgradAns = pvl->directAnswer[tenGageQGradVec];
  evalAns = pvl->directAnswer[tenGageEval];
  evecAns = pvl->directAnswer[tenGageEvec];
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageTensor)) {
    /* done if doV */
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
    pvl->directAnswer[tenGageTrace][0] = dtA + dtD + dtF;
  }
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageFrobTensor)) {
    pvl->directAnswer[tenGageFrobTensor][0] = sqrt(dtA*dtA + 2*dtB*dtB 
						   + 2*dtC*dtC + dtD*dtD
						   + 2*dtE*dtE + dtF*dtF);
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
  }
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageQ)) {
    cbA = -(dtA + dtD + dtF);
    cbB = dtA*dtD - dtB*dtB + dtA*dtF - dtC*dtC + dtD*dtF - dtE*dtE;
    /*
    cbC = -(dtA*dtD*dtF + 2*dtB*dtE*dtC
	    - dtC*dtC*dtD - dtB*dtB*dtF - dtA*dtE*dtE);
    */
    pvl->directAnswer[tenGageQ][0] = cbA*cbA - 3*cbB;
  }
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageQGradVec)) {
    ELL_3V_SET(QgradAns, 0, 0, 0);
    ELL_3V_SCALE_INCR(QgradAns,   dtA, tgradAns + 1*3);
    ELL_3V_SCALE_INCR(QgradAns, 2*dtB, tgradAns + 2*3);
    ELL_3V_SCALE_INCR(QgradAns, 2*dtC, tgradAns + 3*3);
    ELL_3V_SCALE_INCR(QgradAns,   dtD, tgradAns + 4*3);
    ELL_3V_SCALE_INCR(QgradAns, 2*dtE, tgradAns + 5*3);
    ELL_3V_SCALE_INCR(QgradAns,   dtF, tgradAns + 6*3);
    tmptmp = -(dtA + dtD + dtF)/3;
    ELL_3V_SCALE_INCR(QgradAns, tmptmp, tgradAns + 1*3);
    ELL_3V_SCALE_INCR(QgradAns, tmptmp, tgradAns + 4*3);
    ELL_3V_SCALE_INCR(QgradAns, tmptmp, tgradAns + 6*3);
  }
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageQGradMag)) {
    tmptmp = pvl->directAnswer[tenGageQGradMag][0] = ELL_3V_LEN(QgradAns);
  }
  if (GAGE_QUERY_ITEM_TEST(pvl->query, tenGageQNormal)) {
    ELL_3V_SCALE(pvl->directAnswer[tenGageQNormal], 1.0/tmptmp, QgradAns);
  }
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
