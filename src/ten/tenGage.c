/*
  teem: Gordon Kindlmann's research software
  Copyright (C) 2002, 2001, 2000, 1999, 1998 University of Utah

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
#include "tenPrivate.h"

  /*
  tenGageTensor,        *  0: "t", the reconstructed tensor: GT[7] *
  tenGageTrace,         *  1: "tr", trace of tensor: *GT *
  tenGageFrobTensor,    *  2: "fro", frobenius norm of tensor: *GT *
  tenGageEval,          *  3: "eval", eigenvalues of tensor
			       (sorted descending) : GT[3] *
  tenGageEvec,          *  4: "evec", eigenvectors of tensor: GT[9] *
  tenGageTensorGrad,    *  5: "tg", all tensor component gradients, starting with
			        the confidence value gradient: GT[21] *
  tenGageRR,            *  6: "rr", rr anisotropy: *GT *
  tenGageRRGradVec,     *  7: "rrv", gradient of rr anisotropy: GT[3] *
  tenGageRRGradMag,     *  8: "rrg", grad mag of rr anisotropy: *GT *
  tenGageRRNormal,      *  9: "rrn", normalized gradient of rr anisotropy: GT[3] *
  tenGageMultiGrad,     * 10: "mg", sum of outer products of the tensor 
			       matrix elements, correctly counting the off-diagonal
			       entries twice, but not counting the confidence
			       value: GT[9] *
  tenGageFrobMG,        * 11: "frmg", frobenius norm of multi gradient: *GT *
  tenGageMGEval,        * 12: "mgeval", eigenvalues of multi gradient: GT[3] *
  tenGageMGEvec,        * 13: "mgevec", eigenvectors of multi gradient: GT[9] *
  tenGageAniso,         * 14: "an", all anisotropies: GT[TEN_ANISO_MAX+1] *

  0   1   2   3   4   5   6   7   8   9  10  11  12  13  14
*/

int
tenGageAnsLength[TEN_GAGE_MAX+1] = {
  7,  1,  1,  3,  9, 21, 1,  3,  1,  3,  9,  1,  3,  9, TEN_ANISO_MAX+1
};

int
tenGageAnsOffset[TEN_GAGE_MAX+1] = {
  0,  7,  8,  9, 12, 21, 42, 43, 46, 47, 50, 59, 60, 63, 72
  /* --> 72+TEN_ANISO_MAX+1 == TEN_GAGE_TOTAL_ANS_LENGTH */
};

/*
** _tenGageNeedDeriv[]
**
** each value is a BIT FLAG representing the different value/derivatives
** that are needed to calculate the quantity.  
**
** 1: need value interpolation reconstruction (as with k00)
** 2: need first derivatives (as with k11)
** 4: need second derivatives (as with k22)
*/
int
_tenGageNeedDeriv[TEN_GAGE_MAX+1] = {
  1,  1,  1,  1,  1,  2,  1,  2,  2,  2,  2,  2,  2,  2,  1
};

/*
** _tenGagePrereq[]
** 
** this records the measurements which are needed as ingredients for any
** given measurement, but it is not necessarily the recursive expansion of
** that requirement (that role is performed by gageQuerySet())
*/
unsigned int
_tenGagePrereq[TEN_GAGE_MAX+1] = {
  /* 0: tenGageTensor */
  0,

  /* 1: tenGageTrace */
  (1<<tenGageTensor),

  /* 2: tenGageFrobTensor */
  (1<<tenGageTensor),

  /* 3: tenGageEval */
  (1<<tenGageTensor),

  /* 4: tenGageEvec */
  (1<<tenGageTensor),

  /* 5: tenGageTensorGrad */
  0,
  
  /* 6: tenGageRR */
  (1<<tenGageTensor),

  /* 7: tenGageRRGradVec */
  (1<<tenGageTensor) | (1<<tenGageTensorGrad),

  /* 8: tenGageRRGradMag */
  (1<<tenGageRRGradVec),

  /* 9: tenGageRRNormal */
  (1<<tenGageRRGradMag) | (1<<tenGageRRGradVec),

  /* 10: tenGageMultiGrad */
  (1<<tenGageTensorGrad),

  /* 11: tenGageFrobMG */
  (1<<tenGageMultiGrad),
  
  /* 12: tenGageMGEval */
  (1<<tenGageMultiGrad),

  /* 13: tenGageMGEvec */
  (1<<tenGageMultiGrad),

  /* 14: tenGageAniso */
  (1<<tenGageEval)
};

void
_tenGageIv3Print (FILE *file, gageContext *ctx, gagePerVolume *pvl) {
  
  fprintf(file, "_tenGageIv3Print() not implemented\n");
}

void
_tenGageFilter (gageContext *ctx, gagePerVolume *pvl) {
  char me[]="_gageVecFilter";
  gage_t *fw00, *fw11, *fw22, *tensor, *tgrad;
  int fd;

  fd = GAGE_FD(ctx);
  tensor = GAGE_ANSWER_POINTER(pvl, tenGageTensor);
  tgrad = GAGE_ANSWER_POINTER(pvl, tenGageTensorGrad);
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
    DOIT_2(0); DOIT_2(1); DOIT_2(2); DOIT_2(3); DOIT_2(4); DOIT_2(5); DOIT_2(6); 
    break;
  case 4:
#define DOIT_4(J) \
      gageScl3PFilter4(pvl->iv3 + J*64, pvl->iv2 + J*16, pvl->iv1 + J*4, \
		       fw00, fw11, fw22, \
                       tensor + J, tgrad + J*3, NULL, \
		       pvl->needD[0], pvl->needD[1], AIR_FALSE)
    DOIT_4(0); DOIT_4(1); DOIT_4(2); DOIT_4(3); DOIT_4(4); DOIT_4(5); DOIT_4(6); 
    break;
  default:
#define DOIT_N(J)\
      gageScl3PFilterN(fd, \
                       pvl->iv3 + J*fd*fd*fd, \
                       pvl->iv2 + J*fd*fd, pvl->iv1 + J*fd, \
		       fw00, fw11, fw22, \
                       tensor + J, tgrad + J*3, NULL, \
		       pvl->needD[0], pvl->needD[1], AIR_FALSE)
    DOIT_N(0); DOIT_N(1); DOIT_N(2); DOIT_N(3); DOIT_N(4); DOIT_N(5); DOIT_N(6); 
    break;
  }

  return;
}

void
_tenGageAnswer (gageContext *ctx, gagePerVolume *pvl) {
  /* char me[]="_tenGageAnswer"; */
  unsigned int query;
  gage_t *ans, *tenAns, *tgradAns, *rrgradAns, *evalAns, *evecAns, tmptmp=0,
    a, b, c, d, e, f, A, B, C, Q, R;
  int *offset;

#if !GAGE_TYPE_FLOAT
  int ci;
  float tenAnsF[7], evalAnsF[3], evecAnsF[9], c[TEN_ANISO_MAX+1];
#endif

  query = pvl->query;
  ans = pvl->ans;
  offset = tenGageKind->ansOffset;
  tenAns = ans + offset[tenGageTensor];
  tgradAns = ans + offset[tenGageTensorGrad];
  rrgradAns = ans + offset[tenGageRRGradVec];
  evalAns = ans + offset[tenGageEval];
  evecAns = ans + offset[tenGageEvec];
  if (1 & (query >> tenGageTensor)) {
    /* done if doV */
    a = tenAns[1];
    b = tenAns[2];
    c = tenAns[3];
    d = tenAns[4];
    e = tenAns[5];
    f = tenAns[6];
    if (ctx->verbose) {
      fprintf(stderr, "tensor = (%g) %g %g %g   %g %g   %g\n", tenAns[0],
	      a, b, c, d, e, f);
    }
  }
  if (1 & (query >> tenGageTrace)) {
    ans[offset[tenGageTrace]] = a + d + f;
  }
  if (1 & (query >> tenGageFrobTensor)) {
    ans[offset[tenGageTrace]] = sqrt(a*a + 2*b*b + 2*c*c + d*d + 2*e*e + f*f);
  }
  /* HEY: this is pretty sub-optimal if the only thing we want is the 
     eigenvalues for doing anisotropy determination ... */
  if ( (1 & (query >> tenGageEval)) || (1 & (query >> tenGageEvec)) ) {
#if GAGE_TYPE_FLOAT
      tenEigensolve(evalAns, evecAns, tenAns);
#else
      TEN_LIST_COPY(tenAnsF, tenAns);
      tenEigensolve(evalAnsF, evecAnsF, tenAnsF);
      ELL_3V_COPY(evalAns, evalAnsF);
      ELL_3M_COPY(evecAns, evecAnsF);
#endif
  }
  if (1 & (query >> tenGageTensorGrad)) {
    /* done if doD1 */
  }
  if (1 & (query >> tenGageRR)) {
    A = -(a + d + f);
    B = a*d - b*b + a*f - c*c + d*f - e*e;
    C = -(a*d*f + 2*b*e*c - c*c*d - b*b*f - a*e*e);
    ans[offset[tenGageRR]] = A*A - 3*B;
  }
  if (1 & (query >> tenGageRRGradVec)) {
    ELL_3V_SET(rrgradAns, 0, 0, 0);
    ELL_3V_SCALEINCR(rrgradAns,   a, tgradAns + 1*3);
    ELL_3V_SCALEINCR(rrgradAns, 2*b, tgradAns + 2*3);
    ELL_3V_SCALEINCR(rrgradAns, 2*c, tgradAns + 3*3);
    ELL_3V_SCALEINCR(rrgradAns,   d, tgradAns + 4*3);
    ELL_3V_SCALEINCR(rrgradAns, 2*e, tgradAns + 5*3);
    ELL_3V_SCALEINCR(rrgradAns,   f, tgradAns + 6*3);
    tmptmp = -(a + d + f)/3;
    ELL_3V_SCALEINCR(rrgradAns, tmptmp, tgradAns + 1*3);
    ELL_3V_SCALEINCR(rrgradAns, tmptmp, tgradAns + 4*3);
    ELL_3V_SCALEINCR(rrgradAns, tmptmp, tgradAns + 6*3);
  }
  if (1 & (query >> tenGageRRGradMag)) {
    tmptmp = ans[offset[tenGageRRGradMag]] = ELL_3V_LEN(rrgradAns);
  }
  if (1 & (query >> tenGageRRNormal)) {
    ELL_3V_SCALE(ans + offset[tenGageRRNormal], 1.0/tmptmp, rrgradAns);
  }
  if (1 & (query >> tenGageAniso)) {
#if GAGE_TYPE_FLOAT
    tenAnisoCalc(ans + offset[tenGageAniso], evalAns);
#else
    tenAnisoCalc(c, evalAnsF);
    for (ci=0; ci<=TEN_ANISO_MAX; ci++) {
      (ans + offset[tenGageAniso])[ci] = c[ci];
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
  TEN_GAGE_MAX,
  tenGageAnsLength,
  tenGageAnsOffset,
  TEN_GAGE_TOTAL_ANS_LENGTH,
  _tenGageNeedDeriv,
  _tenGagePrereq,
  _tenGageIv3Print,
  _tenGageFilter,
  _tenGageAnswer
};
gageKind *
tenGageKind = &_tenGageKind;
