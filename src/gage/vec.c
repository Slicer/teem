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
#include "private.h"

void
_gageVecAnswer(gageContext *ctx, gagePerVolume *pvl) {
  char me[]="_gageVecAnswer";
  gageVecAnswer *van;
  unsigned int query;
  gage_t len;

  /*
  gageVecVector,      *  0: component-wise-interpolatd (CWI) vector: GT[3] *
  gageVecLength,      *  1: length of CWI vector: *GT *
  gageVecNormalized,  *  2: normalized CWI vector: GT[3] *
  gageVecJacobian,    *  3: component-wise Jacobian: GT[9] *
  gageVecDivergence,  *  4: divergence (based on Jacobian): *GT *
  gageVecCurl,        *  5: curl (based on Jacobian): *GT *
  */

  query = pvl->query;
  van = (gageVecAnswer *)pvl->ans;
  if (1 & (query >> gageVecVector)) {
    /* done if doV */
    if (ctx->verbose) {
      fprintf(stderr, "vec = ");
      ell3vPRINT(stderr, van->vec);
    }
  }
  if (1 & (query >> gageVecLength)) {
    len = van->len[0] = ELL_3V_LEN(van->vec);
  }
  if (1 & (query >> gageVecNormalized)) {
    if (len) {
      ELL_3V_SCALE(van->norm, 1.0/len, van->vec);
    } else {
      ELL_3V_COPY(van->norm, gageZeroNormal);
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
      ell3mPRINT(stderr, van->jac);
    }
  }
  if (1 & (query >> gageVecDivergence)) {
    van->div[0] = van->jac[0] + van->jac[4] + van->jac[8];
  }
  if (1 & (query >> gageVecCurl)) {
    van->curl[0] = van->jac[5] - van->jac[7];
    van->curl[1] = van->jac[6] - van->jac[2];
    van->curl[2] = van->jac[1] - van->jac[3];
  }

  return;
}

void
_gageVecFilter(gageContext *ctx, gagePerVolume *pvl) {
  char me[]="_gageVecFilter";
  gageVecAnswer *van;
  gage_t *fw00, *fw11, *fw22, tmp;
  int fd;

  fd = ctx->fd;
  van = (gageVecAnswer *)pvl->ans;
  fw00 = ctx->fw + fd*3*gageKernel00;
  fw11 = ctx->fw + fd*3*gageKernel11;
  fw22 = ctx->fw + fd*3*gageKernel22;
  /* perform the filtering */
  if (ctx->k3pack) {
    switch (fd) {
    case 2:
#define DOIT_2(J) \
      _gageScl3PFilter2(pvl->iv3 + J*8, pvl->iv2 + J*4, pvl->iv1 + J*2, \
			fw00, fw11, fw22, \
                        van->vec + J, van->jac + J*3, NULL, \
			pvl->doV, pvl->doD1, AIR_FALSE)
      DOIT_2(0); DOIT_2(1); DOIT_2(2); 
      break;
    case 4:
#define DOIT_4(J) \
      _gageScl3PFilter4(pvl->iv3 + J*64, pvl->iv2 + J*16, pvl->iv1 + J*4, \
			fw00, fw11, fw22, \
                        van->vec + J, van->jac + J*3, NULL, \
			pvl->doV, pvl->doD1, AIR_FALSE)
      DOIT_4(0); DOIT_4(1); DOIT_4(2); 
      break;
    default:
#define DOIT_N(J)\
      _gageScl3PFilterN(fd, \
                        pvl->iv3 + J*fd*fd*fd, \
                        pvl->iv2 + J*fd*fd, pvl->iv1 + J*fd, \
			fw00, fw11, fw22, \
                        van->vec + J, van->jac + J*3, NULL, \
			pvl->doV, pvl->doD1, AIR_FALSE)
      DOIT_N(0); DOIT_N(1); DOIT_N(2); 
      break;
    }
  } else {
    fprintf(stderr, "!%s: sorry, 6pack filtering not implemented\n", me);
  }

  /* because we operated component-at-a-time, and because matrices are
     in column order, the 1st column currently contains the three
     derivatives of the X component; this should be the 1st row */
  ELL_3M_TRANSPOSE_IP(van->jac, tmp);

  return;
}

void
_gageVecIv3Fill(gageContext *ctx, gagePerVolume *pvl, void *here) {
  int i, fd, fddd;
  
  fd = ctx->fd;
  fddd = fd*fd*fd;
  for (i=0; i<fddd; i++) {
    /* note that the vector component axis is being shifted 
       from the fastest to the slowest axis, to anticipate
       component-wise filtering operations */
    pvl->iv3[i + fddd*0] = pvl->lup(here, ctx->off[0 + 3*i]);
    pvl->iv3[i + fddd*1] = pvl->lup(here, ctx->off[1 + 3*i]);
    pvl->iv3[i + fddd*2] = pvl->lup(here, ctx->off[2 + 3*i]);
  }

  return;
}
