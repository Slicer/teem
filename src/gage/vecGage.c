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

#include "gage.h"
#include "privateGage.h"

/*
  gageVecVector,      *  0: component-wise-interpolatd (CWI) vector: GT[3]
  gageVecLength,      *  1: length of CWI vector: *GT
  gageVecNormalized,  *  2: normalized CWI vector: GT[3]
  gageVecJacobian,    *  3: component-wise Jacobian: GT[9]
  gageVecDivergence,  *  4: divergence (based on Jacobian): *GT
  gageVecCurl,        *  5: curl (based on Jacobian): GT[3]
  gageVecGradient0,   *  6: gradient of 1st component of vector: GT[3]
  gageVecGradient1,   *  7: gradient of 2nd component of vector: GT[3]
  gageVecGradient2,   *  8: gradient of 3rd component of vector: GT[3]
  gageVecMultiGrad,   *  9: sum of outer products of gradients: GT[9]
  gageVecL2MG,        * 10: L2 norm of multi-gradient: *GT
  gageVecMGEval,      * 11: eigenvalues of multi-gradient: GT[3]
  gageVecMGEvec,      * 12: eigenvectors of multi-gradient: GT[9]
  0   1   2   3   4   5   6   7   8   9  10  11  12
*/

/*
******** gageVecAnsLength[]
**
** the number of gage_t used for each answer
*/
int
gageVecAnsLength[GAGE_VEC_MAX+1] = {
  3,  1,  3,  9,  1,  3,  3,  3,  3,  9,  1,  3,  9
};

/*
******** gageVecAnsOffset[]
**
** the index into the answer array of the first element of the answer
*/
int
gageVecAnsOffset[GAGE_VEC_MAX+1] = {
  0,  3,  4,  7, 16, 17, 20, 23, 26, 29, 38, 39, 42  /* 51 */
};

/*
** _gageVecNeedDeriv[]
**
** each value is a BIT FLAG representing the different value/derivatives
** that are needed to calculate the quantity.  
*/
int
_gageVecNeedDeriv[GAGE_VEC_MAX+1] = {
  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2
};

/*
** _gageVecPrereq[]
** 
** this records the measurements which are needed as ingredients for any
** given measurement, but it is not necessarily the recursive expansion of
** that requirement.
*/
unsigned int
_gageVecPrereq[GAGE_VEC_MAX+1] = {
  /* gageVecVector */
  0,

  /* gageVecLength */
  (1<<gageVecVector),

  /* gageVecNormalized */
  (1<<gageVecVector) | (1<<gageVecLength),

  /* gageVecJacobian */
  0,

  /* gageVecDivergence */
  (1<<gageVecJacobian),

  /* gageVecCurl */
  (1<<gageVecJacobian),

  /* gageVecGradient0 */
  (1<<gageVecJacobian),

  /* gageVecGradient1 */
  (1<<gageVecJacobian),

  /* gageVecGradient2 */
  (1<<gageVecJacobian),

  /* gageVecMultiGrad */
  (1<<gageVecGradient0) | (1<<gageVecGradient1) | (1<<gageVecGradient2),

  /* gageVecL2MG */
  (1<<gageVecMultiGrad),

  /* gageVecMGEval */
  (1<<gageVecMultiGrad),

  /* gageVecMGEvec */
  (1<<gageVecMultiGrad) | (1<<gageVecMGEval)
};

void
_gageVecIv3Fill (gageContext *ctx, gagePerVolume *pvl) {
  int i, sx, sy, bidx, fd, fddd;
  void *here;

  sx = PADSIZE_X(ctx);
  sy = PADSIZE_Y(ctx);
  fd = GAGE_FD(ctx);
  fddd = fd*fd*fd;
  /* the reason to "- ctx->havePad" is that we need to locate the lowest
     corner of the cube of values needed for this probe location, which,
     due to filter size, is lower than the location of the probe itself */
  bidx = (ctx->point.xi - ctx->havePad 
	  + sx*(ctx->point.yi - ctx->havePad 
		+ sy*(ctx->point.zi - ctx->havePad)));
  here = ((char*)(pvl->npad->data) + (bidx * pvl->kind->valLen * 
				      nrrdTypeSize[pvl->npad->type]));
  for (i=0; i<fddd; i++) {
    /* the vector component axis is being shifted from the fastest to
       the slowest axis, to anticipate component-wise filtering
       operations */
    pvl->iv3[i + fddd*0] = pvl->lup(here, 0 + 3*ctx->off[i]);
    pvl->iv3[i + fddd*1] = pvl->lup(here, 1 + 3*ctx->off[i]);
    pvl->iv3[i + fddd*2] = pvl->lup(here, 2 + 3*ctx->off[i]);
  }

  return;
}

void
_gageVecFilter (gageContext *ctx, gagePerVolume *pvl) {
  char me[]="_gageVecFilter";
  gage_t *fw00, *fw11, *fw22, *vec, *jac, tmp;
  int fd;

  fd = GAGE_FD(ctx);
  vec = ANSWER(pvl, gageVecVector);
  jac = ANSWER(pvl, gageVecJacobian);
  fw00 = ctx->fw + fd*3*gageKernel00;
  fw11 = ctx->fw + fd*3*gageKernel11;
  fw22 = ctx->fw + fd*3*gageKernel22;
  /* perform the filtering */
  if (ctx->parm.k3pack) {
    switch (fd) {
    case 2:
#define DOIT_2(J) \
      _gageScl3PFilter2(pvl->iv3 + J*8, pvl->iv2 + J*4, pvl->iv1 + J*2, \
			fw00, fw11, fw22, \
                        vec + J, jac + J*3, NULL, \
			pvl->needD[0], pvl->needD[1], AIR_FALSE)
      DOIT_2(0); DOIT_2(1); DOIT_2(2); 
      break;
    case 4:
#define DOIT_4(J) \
      _gageScl3PFilter4(pvl->iv3 + J*64, pvl->iv2 + J*16, pvl->iv1 + J*4, \
			fw00, fw11, fw22, \
                        vec + J, jac + J*3, NULL, \
			pvl->needD[0], pvl->needD[1], AIR_FALSE)
      DOIT_4(0); DOIT_4(1); DOIT_4(2); 
      break;
    default:
#define DOIT_N(J)\
      _gageScl3PFilterN(fd, \
                        pvl->iv3 + J*fd*fd*fd, \
                        pvl->iv2 + J*fd*fd, pvl->iv1 + J*fd, \
			fw00, fw11, fw22, \
                        vec + J, jac + J*3, NULL, \
			pvl->needD[0], pvl->needD[1], AIR_FALSE)
      DOIT_N(0); DOIT_N(1); DOIT_N(2); 
      break;
    }
  } else {
    fprintf(stderr, "!%s: sorry, 6pack filtering not implemented\n", me);
  }

  if (pvl->needD[1]) {
    /* because we operated component-at-a-time, and because matrices are
       in column order, the 1st column currently contains the three
       derivatives of the X component; this should be the 1st row, and
       likewise for the 2nd and 3rd column/rows.  */
    ELL_3M_TRANSPOSE_IP(jac, tmp);
  }

  return;
}

void
_gageVecAnswer (gageContext *ctx, gagePerVolume *pvl) {
  char me[]="_gageVecAnswer";
  unsigned int query;
  double tmpMat[9], mgevec[9], mgeval[3];
  gage_t *ans, *vecAns, *normAns, *jacAns;

  /*
  gageVecVector,      *  0: component-wise-interpolatd (CWI) vector: GT[3] *
  gageVecLength,      *  1: length of CWI vector: *GT *
  gageVecNormalized,  *  2: normalized CWI vector: GT[3] *
  gageVecJacobian,    *  3: component-wise Jacobian: GT[9] *
  gageVecDivergence,  *  4: divergence (based on Jacobian): *GT *
  gageVecCurl,        *  5: curl (based on Jacobian): GT[3] *
  gageVecGradient0,   *  6: gradient of 1st component of vector: GT[3] *
  gageVecGradient1,   *  7: gradient of 2nd component of vector: GT[3] *
  gageVecGradient2,   *  8: gradient of 3rd component of vector: GT[3] *
  gageVecMultiGrad,   *  9: sum of outer products of gradients: GT[9] *
  gageVecL2MG,        * 10: L2 norm of multi-gradient: *GT *
  gageVecMGEval,      * 11: eigenvalues of multi-gradient: GT[3] *
  gageVecMGEvec,      * 12: eigenvectors of multi-gradient: GT[9] *
  gageVecLast
  */

  query = pvl->query;
  ans = pvl->ans;
  vecAns = ans + gageVecVector;
  jacAns = ans + gageVecJacobian;
  normAns = ans + gageVecNormalized;
  if (1 & (query >> gageVecVector)) {
    /* done if doV */
    if (ctx->verbose) {
      fprintf(stderr, "vec = ");
      ell3vPRINT(stderr, vecAns);
    }
  }
  if (1 & (query >> gageVecLength)) {
    ans[gageVecLength] = ELL_3V_LEN(vecAns);
  }
  if (1 & (query >> gageVecNormalized)) {
    if (ans[gageVecLength]) {
      ELL_3V_SCALE(normAns, 1.0/ans[gageVecLength], vecAns);
    } else {
      ELL_3V_COPY(normAns, gageZeroNormal);
    }
  }
  if (1 & (query >> gageVecJacobian)) {
    /* done if doV1 */
    /*
      0:dv_x/dx  3:dv_x/dy  6:dv_x/dz
      1:dv_y/dx  4:dv_y/dy  7:dv_y/dz
      2:dv_z/dx  5:dv_z/dy  8:dv_z/dz
    */
    if (ctx->verbose) {
      fprintf(stderr, "%s: jac = \n", me);
      ell3mPRINT(stderr, jacAns);
    }
  }
  if (1 & (query >> gageVecDivergence)) {
    ans[gageVecDivergence] = jacAns[0] + jacAns[4] + jacAns[8];
    if (ctx->verbose) {
      fprintf(stderr, "%s: div = %g + %g + %g  = %g\n", me,
	      jacAns[0], jacAns[4], jacAns[8], ans[gageVecDivergence]);
    }
  }
  if (1 & (query >> gageVecCurl)) {
    (ans + gageVecCurl)[0] = jacAns[5] - jacAns[7];
    (ans + gageVecCurl)[1] = jacAns[6] - jacAns[2];
    (ans + gageVecCurl)[2] = jacAns[1] - jacAns[3];
  }
  if (1 & (query >> gageVecGradient0)) {
    (ans + gageVecGradient0)[0] = jacAns[0];
    (ans + gageVecGradient0)[1] = jacAns[3];
    (ans + gageVecGradient0)[2] = jacAns[6];
  }
  if (1 & (query >> gageVecGradient1)) {
    (ans + gageVecGradient1)[0] = jacAns[1];
    (ans + gageVecGradient1)[1] = jacAns[4];
    (ans + gageVecGradient1)[2] = jacAns[7];
  }
  if (1 & (query >> gageVecGradient2)) {
    (ans + gageVecGradient2)[0] = jacAns[2];
    (ans + gageVecGradient2)[1] = jacAns[5];
    (ans + gageVecGradient2)[2] = jacAns[8];
  }
  if (1 & (query >> gageVecMultiGrad)) {
    ELL_3M_IDENTITY_SET(ans + gageVecMultiGrad);
    ELL_3MV_OUTERADD(ans + gageVecMultiGrad,
		     ans + gageVecGradient0, ans + gageVecGradient0);
    ELL_3MV_OUTERADD(ans + gageVecMultiGrad,
		     ans + gageVecGradient1, ans + gageVecGradient1);
    ELL_3MV_OUTERADD(ans + gageVecMultiGrad,
		     ans + gageVecGradient2, ans + gageVecGradient2);
  }
  if (1 & (query >> gageVecL2MG)) {
    ans[gageVecL2MG] = ELL_3M_L2NORM(ans + gageVecMultiGrad);
  }
  if (1 & (query >> gageVecMGEval)) {
    ELL_3M_COPY(tmpMat, ans + gageVecMultiGrad);
    /* HEY: look at the return value for root multiplicity? */
    ell3mEigensolve(mgeval, mgevec, tmpMat, AIR_TRUE);
    ELL_3V_COPY(ans + gageVecMGEval, mgeval);
  }
  if (1 & (query >> gageVecMGEvec)) {
    ELL_3M_COPY(ans + gageVecMGEvec, mgevec);
  }

  return;
}

char
_gageVecStr[][AIR_STRLEN_SMALL] = {
  "(unknown gageVec)",
  "vector",
  "length",
  "normalized",
  "Jacobian",
  "divergence",
  "curl",
  "gradient0",
  "gradient1",
  "gradient2",
  "multigrad",
  "L2(multigrad)",
  "multigrad eigenvalues",
  "multigrad eigenvectors",
};

char
_gageVecDesc[][AIR_STRLEN_MED] = {
  "unknown gageVec query",
  "component-wise-interpolated vector",
  "length of vector",
  "normalized vector",
  "3x3 Jacobian",
  "divergence",
  "curl",
  "gradient of 1st component of vector",
  "gradient of 2nd component of vector",
  "gradient of 3rd component of vector",
  "multi-gradient: sum of outer products of gradients",
  "L2 norm of multi-gradient",
  "eigenvalues of multi-gradient",
  "eigenvectors of multi-gradient"
};

int
_gageVecVal[] = {
  gageVecUnknown,
  gageVecVector,
  gageVecLength,
  gageVecNormalized,
  gageVecJacobian,
  gageVecDivergence,
  gageVecCurl,
  gageVecGradient0,
  gageVecGradient1,
  gageVecGradient2,
  gageVecMultiGrad,
  gageVecL2MG,
  gageVecMGEval,
  gageVecMGEvec,
};

#define GV_V gageVecVector
#define GV_L gageVecLength
#define GV_N gageVecNormalized
#define GV_J gageVecJacobian
#define GV_D gageVecDivergence
#define GV_C gageVecCurl
#define GV_G0 gageVecGradient0
#define GV_G1 gageVecGradient1
#define GV_G2 gageVecGradient2
#define GV_MG gageVecMultiGrad
#define GV_LM gageVecL2MG
#define GV_ML gageVecMGEval
#define GV_MC gageVecMGEvec

char
_gageVecStrEqv[][AIR_STRLEN_SMALL] = {
  "v", "vector", "vec",
  "l", "length", "len",
  "n", "normalized", "normalized vector",
  "jacobian", "jac", "j",
  "divergence", "div", "d",
  "curl", "c",
  "g0", "grad0", "gradient0",
  "g1", "grad1", "gradient1",
  "g2", "grad2", "gradient2",
  "mg", "multigrad",
  "l2mg", "l2multigrad",
  "mgeval", "mg eval", "multigrad eigenvalues",
  "mgevec", "mg evec", "multigrad eigenvectors",
  ""
};

int
_gageVecValEqv[] = {
  GV_V, GV_V, GV_V,
  GV_L, GV_L, GV_L,
  GV_N, GV_N, GV_N,
  GV_J, GV_J, GV_J,
  GV_D, GV_D, GV_D,
  GV_C, GV_C,
  GV_G0, GV_G0, GV_G0,
  GV_G1, GV_G1, GV_G1,
  GV_G2, GV_G2, GV_G2,
  GV_MG, GV_MG,
  GV_LM, GV_LM,
  GV_ML, GV_ML, GV_ML,
  GV_MC, GV_MC, GV_MC
};

airEnum
_gageVec = {
  "gageVec",
  GAGE_VEC_MAX+1,
  _gageVecStr, _gageVecVal,
  _gageVecDesc,
  _gageVecStrEqv, _gageVecValEqv,
  AIR_FALSE
};
airEnum *
gageVec = &_gageVec;

gageKind
_gageKindVec = {
  "vector",
  &_gageVec,
  1,
  3,
  GAGE_VEC_MAX,
  gageVecAnsLength,
  gageVecAnsOffset,
  GAGE_VEC_TOTAL_ANS_LENGTH,
  _gageVecNeedDeriv,
  _gageVecPrereq,
  _gageVecPrint_query,
  _gageVecIv3Fill,
  _gageVecIv3Print,
  _gageVecFilter,
  _gageVecAnswer
};
gageKind *
gageKindVec = &_gageKindVec;

