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


#include "limn.h"

limnPolyData *
limnPolyDataNew(void) {
  limnPolyData *pld;

  pld = (limnPolyData *)calloc(1, sizeof(limnPolyData));
  if (pld) {
    pld->vertNum = 0;
    pld->xyzw = NULL;
    pld->rgba = NULL;
    pld->norm = NULL;
    pld->tex2D = NULL;
    pld->indxNum = 0;
    pld->indx = NULL;
    pld->primNum = 0;
    pld->type = NULL;
    pld->icnt = NULL;
  }
  return pld;
}

limnPolyData *
limnPolyDataNix(limnPolyData *pld) {

  if (pld) {
    airFree(pld->xyzw);
    airFree(pld->rgba);
    airFree(pld->norm);
    airFree(pld->tex2D);
    airFree(pld->indx);
    airFree(pld->type);
    airFree(pld->icnt);
  }
  airFree(pld);
  return NULL;
}

/*
** does NOT set pld->vertNum
*/
int
_limnPolyDataInfoAlloc(limnPolyData *pld, unsigned int infoBitFlag,
                       unsigned int vertNum) {
  char me[]="_limnPolyDataInfoAlloc", err[BIFF_STRLEN];
  
  if (vertNum != pld->vertNum) {
    if ((1 << limnPolyDataInfoRGBA) & infoBitFlag) {
      pld->rgba = (unsigned char *)airFree(pld->rgba);
      if (vertNum) {
        pld->rgba = (unsigned char *)calloc(vertNum, 4*sizeof(unsigned char));
        if (!pld->rgba) {
          sprintf(err, "%s: couldn't allocate %u rgba", me, vertNum);
          biffAdd(LIMN, err); return 1;
        }
      }
    }
    if ((1 << limnPolyDataInfoNorm) & infoBitFlag) {
      pld->norm = (float *)airFree(pld->norm);
      if (vertNum) {
        pld->norm = (float *)calloc(vertNum, 4*sizeof(float));
        if (!pld->norm) {
          sprintf(err, "%s: couldn't allocate %u norm", me, vertNum);
          biffAdd(LIMN, err); return 1;
        }
      }
    }
    if ((1 << limnPolyDataInfoTex2D) & infoBitFlag) {
      pld->tex2D = (float *)airFree(pld->tex2D);
      if (vertNum) {
        pld->tex2D = (float *)calloc(vertNum, 4*sizeof(float));
        if (!pld->tex2D) {
          sprintf(err, "%s: couldn't allocate %u tex2D", me, vertNum);
          biffAdd(LIMN, err); return 1;
        }
      }
    }
  }
  return 0;
}

int
limnPolyDataAlloc(limnPolyData *pld,
                  unsigned int infoBitFlag,
                  unsigned int vertNum,
                  unsigned int indxNum,
                  unsigned int primNum) {
  char me[]="limnPolyDataAlloc", err[BIFF_STRLEN];
  
  if (!pld) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(LIMN, err); return 1;
  }
  if (vertNum != pld->vertNum) {
    pld->xyzw = (float *)airFree(pld->xyzw);
    if (vertNum) {
      pld->xyzw = (float *)calloc(vertNum, 4*sizeof(float));
      if (!pld->xyzw) {
        sprintf(err, "%s: couldn't allocate %u xyzw", me, vertNum);
        biffAdd(LIMN, err); return 1;
      }
    }
    if (_limnPolyDataInfoAlloc(pld, infoBitFlag, vertNum)) {
      sprintf(err, "%s: couldn't allocate info", me);
      biffAdd(LIMN, err); return 1;
    }
    pld->vertNum = vertNum;
  }
  if (indxNum != pld->indxNum) {
    pld->indx = (unsigned int *)airFree(pld->indx);
    if (indxNum) {
      pld->indx = (unsigned int *)calloc(indxNum, sizeof(unsigned int));
      if (!pld->indx) {
        sprintf(err, "%s: couldn't allocate %u indices", me, indxNum);
        biffAdd(LIMN, err); return 1;
      }
    }
    pld->indxNum = indxNum;
  }
  if (primNum != pld->primNum) {
    pld->type = (unsigned char *)airFree(pld->type);
    pld->icnt = (unsigned int *)airFree(pld->icnt);
    if (primNum) {
      pld->type = (unsigned char *)calloc(primNum, sizeof(unsigned char));
      pld->icnt = (unsigned int *)calloc(primNum, sizeof(unsigned int));
      if (!(pld->type && pld->icnt)) {
        sprintf(err, "%s: couldn't allocate %u primitives", me, primNum);
        biffAdd(LIMN, err); return 1;
      }
    }
    pld->primNum = primNum;
  }
  return 0;
}

size_t
limnPolyDataSize(limnPolyData *pld) {
  size_t ret = 0;

  if (pld) {
    ret += pld->vertNum*sizeof(float)*4;  /* xyzw */
    if (pld->rgba) {
      ret += pld->vertNum*sizeof(unsigned char)*4;
    }
    if (pld->norm) {
      ret += pld->vertNum*sizeof(float)*3;
    }
    if (pld->tex2D) {
      ret += pld->vertNum*sizeof(float)*2;
    }
    ret += pld->indxNum*sizeof(unsigned int);
    ret += pld->primNum*sizeof(signed char);
    ret += pld->primNum*sizeof(unsigned int);
  }
  return ret;
}

int
limnPolyDataCopy(limnPolyData *pldB, const limnPolyData *pldA) {
  char me[]="limnPolyDataCopy", err[BIFF_STRLEN];

  if (!( pldB && pldA )) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(LIMN, err); return 1;
  }
  if (limnPolyDataAlloc(pldB, 
                        ((pldA->rgba ? 1 << limnPolyDataInfoRGBA : 0)
                         | (pldA->norm ? 1 << limnPolyDataInfoNorm : 0)
                         | (pldA->tex2D ? 1 << limnPolyDataInfoTex2D : 0)),
                        pldA->vertNum, pldA->indxNum, pldA->primNum)) {
    sprintf(err, "%s: couldn't allocate output", me);
    biffAdd(LIMN, err); return 1;
  }
  memcpy(pldB->xyzw, pldA->xyzw, pldA->vertNum*sizeof(float)*4);
  if (pldA->rgba) {
    memcpy(pldB->rgba, pldA->rgba, pldA->vertNum*sizeof(unsigned char)*4);
  }
  if (pldA->norm) {
    memcpy(pldB->norm, pldA->norm, pldA->vertNum*sizeof(float)*3);
  }
  if (pldA->tex2D) {
    memcpy(pldB->tex2D, pldA->tex2D, pldA->vertNum*sizeof(float)*2);
  }
  memcpy(pldB->indx, pldA->indx, pldA->indxNum*sizeof(unsigned int));
  memcpy(pldB->type, pldA->type, pldA->primNum*sizeof(signed char));
  memcpy(pldB->icnt, pldA->icnt, pldA->primNum*sizeof(unsigned int));
  return 0;
}

#if 0

/* GLK got lazy Sun Feb  5 18:31:57 EST 2006 and didn't bring this uptodate
   with the new limnPolyData */

int
limnPolyDataCopyN(limnPolyData *pldB, const limnPolyData *pldA,
                  unsigned int num) {
  char me[]="limnPolyDataCopyN", err[BIFF_STRLEN];
  unsigned int ii, jj;

  if (!( pldB && pldA )) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(LIMN, err); return 1;
  }
  if (limnPolyDataAlloc(pldB, num*pldA->vertNum,
                       num*pldA->indxNum, num*pldA->primNum)) {
    sprintf(err, "%s: couldn't allocate output", me);
    biffAdd(LIMN, err); return 1;
  }
  for (ii=0; ii<num; ii++) {
    memcpy(pldB->vert + ii*pldA->vertNum, pldA->vert,
           pldA->vertNum*sizeof(limnVrt));
    for (jj=0; jj<pldA->indxNum; jj++) {
      (pldB->indx + ii*pldA->indxNum)[jj] = pldA->indx[jj] + ii*pldA->vertNum;
    }
    memcpy(pldB->type + ii*pldA->primNum, pldA->type,
           pldA->primNum*sizeof(signed char));
    memcpy(pldB->icnt + ii*pldA->primNum, pldA->icnt,
           pldA->primNum*sizeof(unsigned int));
  }
  return 0;
}

#endif

int
limnPolyDataCube(limnPolyData *pld,
                 unsigned int infoBitFlag,
                 int sharpEdge) {
  char me[]="limnPolyDataCube", err[BIFF_STRLEN];
  unsigned int vertNum, vertIdx, primNum, indxNum, cnum, ci;
  float cn;

  vertNum = sharpEdge ? 6*4 : 8;
  primNum = 1;
  indxNum = 6*4;
  if (limnPolyDataAlloc(pld, infoBitFlag, vertNum, indxNum, primNum)) {
    sprintf(err, "%s: couldn't allocate output", me);
    biffAdd(LIMN, err); return 1;
  }
  
  vertIdx = 0;
  cnum = sharpEdge ? 3 : 1;
  cn = AIR_CAST(float, sqrt(3.0));
  for (ci=0; ci<cnum; ci++) {
    ELL_4V_SET(pld->xyzw + 4*vertIdx,  -1,  -1,  -1,  1); vertIdx++;
  }
  for (ci=0; ci<cnum; ci++) {
    ELL_4V_SET(pld->xyzw + 4*vertIdx,  -1,   1,  -1,  1); vertIdx++;
  }
  for (ci=0; ci<cnum; ci++) {
    ELL_4V_SET(pld->xyzw + 4*vertIdx,   1,   1,  -1,  1); vertIdx++;
  }
  for (ci=0; ci<cnum; ci++) {
    ELL_4V_SET(pld->xyzw + 4*vertIdx,   1,  -1,  -1,  1); vertIdx++;
  }
  for (ci=0; ci<cnum; ci++) {
    ELL_4V_SET(pld->xyzw + 4*vertIdx,  -1,  -1,   1,  1); vertIdx++;
  }
  for (ci=0; ci<cnum; ci++) {
    ELL_4V_SET(pld->xyzw + 4*vertIdx,  -1,   1,   1,  1); vertIdx++;
  }
  for (ci=0; ci<cnum; ci++) {
    ELL_4V_SET(pld->xyzw + 4*vertIdx,   1,   1,   1,  1); vertIdx++;
  }
  for (ci=0; ci<cnum; ci++) {
    ELL_4V_SET(pld->xyzw + 4*vertIdx,   1,  -1,   1,  1); vertIdx++;
  }

  vertIdx = 0;
  if (sharpEdge) {
    ELL_4V_SET(pld->indx + vertIdx,  0,  3,  6,  9); vertIdx += 4;
    ELL_4V_SET(pld->indx + vertIdx,  2, 14, 16,  5); vertIdx += 4;
    ELL_4V_SET(pld->indx + vertIdx,  4, 17, 18,  8); vertIdx += 4;
    ELL_4V_SET(pld->indx + vertIdx,  7, 19, 21, 10); vertIdx += 4;
    ELL_4V_SET(pld->indx + vertIdx,  1, 11, 23, 13); vertIdx += 4;
    ELL_4V_SET(pld->indx + vertIdx, 12, 22, 20, 15); vertIdx += 4;
  } else {
    ELL_4V_SET(pld->indx + vertIdx,  0,  1,  2,  3); vertIdx += 4;
    ELL_4V_SET(pld->indx + vertIdx,  0,  4,  5,  1); vertIdx += 4;
    ELL_4V_SET(pld->indx + vertIdx,  1,  5,  6,  2); vertIdx += 4;
    ELL_4V_SET(pld->indx + vertIdx,  2,  6,  7,  3); vertIdx += 4;
    ELL_4V_SET(pld->indx + vertIdx,  0,  3,  7,  4); vertIdx += 4;
    ELL_4V_SET(pld->indx + vertIdx,  4,  7,  6,  5); vertIdx += 4;
  }

  if ((1 << limnPolyDataInfoNorm) & infoBitFlag) {
    if (sharpEdge) {
      ELL_3V_SET(pld->norm +  3*0,  0,  0, -1);
      ELL_3V_SET(pld->norm +  3*3,  0,  0, -1);
      ELL_3V_SET(pld->norm +  3*6,  0,  0, -1);
      ELL_3V_SET(pld->norm +  3*9,  0,  0, -1);
      ELL_3V_SET(pld->norm +  3*2, -1,  0,  0);
      ELL_3V_SET(pld->norm +  3*5, -1,  0,  0);
      ELL_3V_SET(pld->norm + 3*14, -1,  0,  0);
      ELL_3V_SET(pld->norm + 3*16, -1,  0,  0);
      ELL_3V_SET(pld->norm +  3*4,  0,  1,  0);
      ELL_3V_SET(pld->norm +  3*8,  0,  1,  0);
      ELL_3V_SET(pld->norm + 3*17,  0,  1,  0);
      ELL_3V_SET(pld->norm + 3*18,  0,  1,  0);
      ELL_3V_SET(pld->norm +  3*7,  1,  0,  0);
      ELL_3V_SET(pld->norm + 3*10,  1,  0,  0);
      ELL_3V_SET(pld->norm + 3*19,  1,  0,  0);
      ELL_3V_SET(pld->norm + 3*21,  1,  0,  0);
      ELL_3V_SET(pld->norm +  3*1,  0, -1,  0);
      ELL_3V_SET(pld->norm + 3*11,  0, -1,  0);
      ELL_3V_SET(pld->norm + 3*13,  0, -1,  0);
      ELL_3V_SET(pld->norm + 3*23,  0, -1,  0);
      ELL_3V_SET(pld->norm + 3*12,  0,  0,  1);
      ELL_3V_SET(pld->norm + 3*15,  0,  0,  1);
      ELL_3V_SET(pld->norm + 3*20,  0,  0,  1);
      ELL_3V_SET(pld->norm + 3*22,  0,  0,  1);
    } else {
      ELL_3V_SET(pld->norm + 3*0, -cn, -cn, -cn);
      ELL_3V_SET(pld->norm + 3*1, -cn,  cn, -cn);
      ELL_3V_SET(pld->norm + 3*2,  cn,  cn, -cn);
      ELL_3V_SET(pld->norm + 3*3,  cn, -cn, -cn);
      ELL_3V_SET(pld->norm + 3*4, -cn, -cn,  cn);
      ELL_3V_SET(pld->norm + 3*5, -cn,  cn,  cn);
      ELL_3V_SET(pld->norm + 3*6,  cn,  cn,  cn);
      ELL_3V_SET(pld->norm + 3*7,  cn, -cn,  cn);
    }
  }

  pld->type[0] = limnPrimitiveQuads;
  pld->icnt[0] = indxNum;
  
  if ((1 << limnPolyDataInfoRGBA) & infoBitFlag) {
    for (vertIdx=0; vertIdx<pld->vertNum; vertIdx++) {
      ELL_4V_SET(pld->rgba + 4*vertIdx, 255, 255, 255, 255);
    }
  }

  return 0;
}

int
limnPolyDataCylinder(limnPolyData *pld,
                     unsigned int infoBitFlag,
                     unsigned int thetaRes, int sharpEdge) {
  char me[]="limnPolyDataCylinderNew", err[BIFF_STRLEN];
  unsigned int vertNum, primNum, primIdx, indxNum, thetaIdx, vertIdx, blah;
  double theta, cth, sth, sq2;

  /* sanity bounds */
  thetaRes = AIR_MAX(3, thetaRes);

  vertNum = sharpEdge ? 4*thetaRes : 2*thetaRes;
  primNum = 3;
  indxNum = 2*thetaRes + 2*(thetaRes+1);  /* 2 fans + 1 strip */
  if (limnPolyDataAlloc(pld, infoBitFlag, vertNum, indxNum, primNum)) {
    sprintf(err, "%s: couldn't allocate output", me); 
    biffAdd(LIMN, err); return 1;
  }
  
  vertIdx = 0;
  for (blah=0; blah < (sharpEdge ? 2u : 1u); blah++) {
    for (thetaIdx=0; thetaIdx<thetaRes; thetaIdx++) {
      theta = AIR_AFFINE(0, thetaIdx, thetaRes, 0, 2*AIR_PI);
      ELL_4V_SET_TT(pld->xyzw + 4*vertIdx, float,
                    cos(theta), sin(theta), 1, 1);
      /*
      fprintf(stderr, "!%s: vert[%u] = %g %g %g\n", me, vertIdx,
              (pld->xyzw + 4*vertIdx)[0],
              (pld->xyzw + 4*vertIdx)[1],
              (pld->xyzw + 4*vertIdx)[2]);
      */
      ++vertIdx;
    }
  }
  for (blah=0; blah < (sharpEdge ? 2u : 1u); blah++) {
    for (thetaIdx=0; thetaIdx<thetaRes; thetaIdx++) {
      theta = AIR_AFFINE(0, thetaIdx, thetaRes, 0, 2*AIR_PI);
      ELL_4V_SET_TT(pld->xyzw + 4*vertIdx, float,
                    cos(theta), sin(theta), -1, 1);
      /*
      fprintf(stderr, "!%s: vert[%u] = %g %g %g\n", me, vertIdx,
              (pld->xyzw + 4*vertIdx)[0],
              (pld->xyzw + 4*vertIdx)[1],
              (pld->xyzw + 4*vertIdx)[2]);
      */
      ++vertIdx;
    }
  }

  primIdx = 0;
  vertIdx = 0;

  /* fan on top */
  for (thetaIdx=0; thetaIdx<thetaRes; thetaIdx++) {
    pld->indx[vertIdx++] = thetaIdx;
  }
  pld->type[primIdx] = limnPrimitiveTriangleFan;
  pld->icnt[primIdx] = thetaRes;
  primIdx++;

  /* single strip around */
  for (thetaIdx=0; thetaIdx<thetaRes; thetaIdx++) {
    pld->indx[vertIdx++] = (sharpEdge ? 1 : 0)*thetaRes + thetaIdx;
    pld->indx[vertIdx++] = (sharpEdge ? 2 : 1)*thetaRes + thetaIdx;
  }
  pld->indx[vertIdx++] = (sharpEdge ? 1 : 0)*thetaRes;
  pld->indx[vertIdx++] = (sharpEdge ? 2 : 1)*thetaRes;
  pld->type[primIdx] = limnPrimitiveTriangleStrip;
  pld->icnt[primIdx] = 2*(thetaRes+1);
  primIdx++;

  /* fan on bottom */
  for (thetaIdx=0; thetaIdx<thetaRes; thetaIdx++) {
    pld->indx[vertIdx++] = (sharpEdge ? 3 : 1)*thetaRes + thetaIdx;
  }
  pld->type[primIdx] = limnPrimitiveTriangleFan;
  pld->icnt[primIdx] = thetaRes;
  primIdx++;

  if ((1 << limnPolyDataInfoNorm) & infoBitFlag) {
    sq2 = sqrt(2.0);
    if (sharpEdge) {
      for (thetaIdx=0; thetaIdx<thetaRes; thetaIdx++) {
        theta = AIR_AFFINE(0, thetaIdx, thetaRes, 0, 2*AIR_PI);
        cth = cos(theta);
        sth = sin(theta);
        ELL_3V_SET_TT(pld->norm + 3*(thetaIdx + 0*thetaRes),
                      float, 0, 0, 1);
        ELL_3V_SET_TT(pld->norm + 3*(thetaIdx + 1*thetaRes),
                      float, cth, sth, 0);
        ELL_3V_SET_TT(pld->norm + 3*(thetaIdx + 2*thetaRes),
                      float, cth, sth, 0);
        ELL_3V_SET_TT(pld->norm + 3*(thetaIdx + 3*thetaRes),
                      float, 0, 0, -1);
      }
    } else {
      for (thetaIdx=0; thetaIdx<thetaRes; thetaIdx++) {
        theta = AIR_AFFINE(0, thetaIdx, thetaRes, 0, 2*AIR_PI);
        cth = sq2*cos(theta);
        sth = sq2*sin(theta);
        ELL_3V_SET_TT(pld->norm + 3*(thetaIdx + 0*thetaRes), float,
                      cth, sth, sq2);
        ELL_3V_SET_TT(pld->norm + 3*(thetaIdx + 1*thetaRes), float,
                      cth, sth, -sq2);
      }
    }
  }

  if ((1 << limnPolyDataInfoRGBA) & infoBitFlag) {
    for (vertIdx=0; vertIdx<pld->vertNum; vertIdx++) {
      ELL_4V_SET(pld->rgba + 4*vertIdx, 255, 255, 255, 255);
    }
  }

  return 0;
}

/*
******** limnPolyDataSuperquadric
**
** makes a superquadric parameterized around the Z axis
*/
int
limnPolyDataSuperquadric(limnPolyData *pld,
                         unsigned int infoBitFlag,
                         float alpha, float beta,
                         unsigned int thetaRes, unsigned int phiRes) {
  char me[]="limnPolyDataSuperquadric", err[BIFF_STRLEN];
  unsigned int vertIdx, vertNum, fanNum, stripNum, primNum, indxNum,
    thetaIdx, phiIdx, primIdx;
  double theta, phi;

  /* sanity bounds */
  thetaRes = AIR_MAX(3u, thetaRes);
  phiRes = AIR_MAX(2u, phiRes);
  alpha = AIR_MAX(0.00001f, alpha);
  beta = AIR_MAX(0.00001f, beta);

  vertNum = 2 + thetaRes*(phiRes-1);
  fanNum = 2;
  stripNum = phiRes-2;
  primNum = fanNum + stripNum;
  indxNum = (thetaRes+2)*fanNum + 2*(thetaRes+1)*stripNum;
  if (limnPolyDataAlloc(pld, infoBitFlag, vertNum, indxNum, primNum)) {
    sprintf(err, "%s: couldn't allocate output", me);
    biffAdd(LIMN, err); return 1;
  }

  vertIdx = 0;
  ELL_4V_SET(pld->xyzw + 4*vertIdx, 0, 0, 1, 1);
  if ((1 << limnPolyDataInfoNorm) & infoBitFlag) {
    ELL_3V_SET(pld->norm + 3*vertIdx, 0, 0, 1);
  }
  ++vertIdx;
  for (phiIdx=1; phiIdx<phiRes; phiIdx++) {
    double cost, sint, cosp, sinp;
    phi = AIR_AFFINE(0, phiIdx, phiRes, 0, AIR_PI);
    cosp = cos(phi);
    sinp = sin(phi);
    for (thetaIdx=0; thetaIdx<thetaRes; thetaIdx++) {
      theta = AIR_AFFINE(0, thetaIdx, thetaRes, 0, 2*AIR_PI);
      cost = cos(theta);
      sint = sin(theta);
      ELL_4V_SET_TT(pld->xyzw + 4*vertIdx, float,
                    airSgnPow(cost,alpha) * airSgnPow(sinp,beta),
                    airSgnPow(sint,alpha) * airSgnPow(sinp,beta),
                    airSgnPow(cosp,beta),
                    1.0);
      if ((1 << limnPolyDataInfoNorm) & infoBitFlag) {
        if (1 == alpha && 1 == beta) {
          ELL_3V_COPY(pld->norm + 3*vertIdx, pld->xyzw + 4*vertIdx);
        } else {
          ELL_3V_SET_TT(pld->norm + 3*vertIdx, float,
                        2*airSgnPow(cost,2-alpha)*airSgnPow(sinp,2-beta)/beta,
                        2*airSgnPow(sint,2-alpha)*airSgnPow(sinp,2-beta)/beta,
                        2*airSgnPow(cosp,2-beta)/beta);
        }
      }
      ++vertIdx;
    }
  }
  ELL_4V_SET(pld->xyzw + 4*vertIdx, 0, 0, -1, 1);
  if ((1 << limnPolyDataInfoNorm) & infoBitFlag) {
    ELL_3V_SET(pld->norm + 3*vertIdx, 0, 0, -1);
  }
  ++vertIdx;

  /* triangle fan at top */
  vertIdx = 0;
  primIdx = 0;
  pld->indx[vertIdx++] = 0;
  for (thetaIdx=0; thetaIdx<thetaRes; thetaIdx++) {
    pld->indx[vertIdx++] = thetaIdx + 1;
  }
  pld->indx[vertIdx++] = 1;
  pld->type[primIdx] = limnPrimitiveTriangleFan;
  pld->icnt[primIdx++] = thetaRes + 2;

  /* tristrips around */
  for (phiIdx=1; phiIdx<phiRes-1; phiIdx++) {
    /*
    fprintf(stderr, "!%s: prim[%u] = vert[%u] =", me, primIdx, vertIdx);
    */
    for (thetaIdx=0; thetaIdx<thetaRes; thetaIdx++) {
      /*
      fprintf(stderr, " [%u %u] %u %u", 
              vertIdx, vertIdx + 1,
              (phiIdx-1)*thetaRes + thetaIdx + 1,
              phiIdx*thetaRes + thetaIdx + 1);
      */
      pld->indx[vertIdx++] = (phiIdx-1)*thetaRes + thetaIdx + 1;
      pld->indx[vertIdx++] = phiIdx*thetaRes + thetaIdx + 1;
    }
    /*
    fprintf(stderr, " [%u %u] %u %u (%u verts)\n", 
            vertIdx, vertIdx + 1,
            (phiIdx-1)*thetaRes + 1,
            phiIdx*thetaRes + 1, 2*(thetaRes+1));
    */
    pld->indx[vertIdx++] = (phiIdx-1)*thetaRes + 1;
    pld->indx[vertIdx++] = phiIdx*thetaRes + 1;
    pld->type[primIdx] = limnPrimitiveTriangleStrip;
    pld->icnt[primIdx++] = 2*(thetaRes+1);
  }

  /* triangle fan at bottom */
  pld->indx[vertIdx++] = vertNum-1;
  for (thetaIdx=0; thetaIdx<thetaRes; thetaIdx++) {
    pld->indx[vertIdx++] = thetaRes*(phiRes-2) + thetaRes - thetaIdx;
  }
  pld->indx[vertIdx++] = thetaRes*(phiRes-2) + thetaRes;
  pld->type[primIdx] = limnPrimitiveTriangleFan;
  pld->icnt[primIdx++] = thetaRes + 2;

  if ((1 << limnPolyDataInfoRGBA) & infoBitFlag) {
    for (vertIdx=0; vertIdx<pld->vertNum; vertIdx++) {
      ELL_4V_SET(pld->rgba + 4*vertIdx, 255, 255, 255, 255);
    }
  }

  return 0;
}

/*
******** limnPolyDataSpiralSuperquadric
**
** puts a superquadric into a single spiral triangle strip
*/
int
limnPolyDataSpiralSuperquadric(limnPolyData *pld,
                               unsigned int infoBitFlag,
                               float alpha, float beta,
                               unsigned int thetaRes, unsigned int phiRes) {
  char me[]="limnPolyDataSpiralSuperquadric", err[BIFF_STRLEN];
  unsigned int vertIdx, vertNum, indxNum, thetaIdx, phiIdx;

  /* sanity bounds */
  thetaRes = AIR_MAX(3u, thetaRes);
  phiRes = AIR_MAX(2u, phiRes);
  alpha = AIR_MAX(0.00001f, alpha);
  beta = AIR_MAX(0.00001f, beta);

  vertNum = thetaRes*phiRes + 1;
  indxNum = 2*thetaRes*(phiRes+1) - 2;
  if (limnPolyDataAlloc(pld, infoBitFlag, vertNum, indxNum, 1)) {
    sprintf(err, "%s: couldn't allocate output", me);
    biffAdd(LIMN, err); return 1;
  }

  vertIdx = 0;
  for (phiIdx=0; phiIdx<phiRes; phiIdx++) {
    for (thetaIdx=0; thetaIdx<thetaRes; thetaIdx++) {
      double cost, sint, cosp, sinp;
      double phi = (AIR_AFFINE(0, phiIdx, phiRes, 0, AIR_PI)
                    + AIR_AFFINE(0, thetaIdx, thetaRes, 0, AIR_PI)/phiRes);
      double theta = AIR_AFFINE(0, thetaIdx, thetaRes, 0.0, 2*AIR_PI);
      cosp = cos(phi);
      sinp = sin(phi);
      cost = cos(theta);
      sint = sin(theta);
      ELL_4V_SET_TT(pld->xyzw + 4*vertIdx, float,
                    airSgnPow(cost,alpha) * airSgnPow(sinp,beta),
                    airSgnPow(sint,alpha) * airSgnPow(sinp,beta),
                    airSgnPow(cosp,beta),
                    1.0);
      if ((1 << limnPolyDataInfoNorm) & infoBitFlag) {
        if (1 == alpha && 1 == beta) {
          ELL_3V_COPY(pld->norm + 3*vertIdx, pld->xyzw + 4*vertIdx);
        } else {
          ELL_3V_SET_TT(pld->norm + 3*vertIdx, float,
                        2*airSgnPow(cost,2-alpha)*airSgnPow(sinp,2-beta)/beta,
                        2*airSgnPow(sint,2-alpha)*airSgnPow(sinp,2-beta)/beta,
                        2*airSgnPow(cosp,2-beta)/beta);
        }
      }
      ++vertIdx;
    }
  }
  ELL_4V_SET(pld->xyzw + 4*vertIdx, 0, 0, -1, 1);
  if ((1 << limnPolyDataInfoNorm) & infoBitFlag) {
    ELL_3V_SET(pld->norm + 3*vertIdx, 0, 0, -1);
  }
  ++vertIdx;

  /* single triangle strip */
  pld->type[0] = limnPrimitiveTriangleStrip;
  pld->icnt[0] = indxNum;
  vertIdx = 0;
  for (thetaIdx=1; thetaIdx<thetaRes; thetaIdx++) {
    pld->indx[vertIdx++] = 0;
    pld->indx[vertIdx++] = thetaIdx;
  }
  for (phiIdx=0; phiIdx<phiRes-1; phiIdx++) {
    for (thetaIdx=0; thetaIdx<thetaRes; thetaIdx++) {
      pld->indx[vertIdx++] = ((phiIdx + 0) * thetaRes) + thetaIdx;
      pld->indx[vertIdx++] = ((phiIdx + 1) * thetaRes) + thetaIdx;
    }
  }
  for (thetaIdx=0; thetaIdx<thetaRes; thetaIdx++) {
    pld->indx[vertIdx++] = (phiRes - 1)*thetaRes + thetaIdx;
    pld->indx[vertIdx++] = (phiRes - 0)*thetaRes;
  }

  if ((1 << limnPolyDataInfoRGBA) & infoBitFlag) {
    for (vertIdx=0; vertIdx<pld->vertNum; vertIdx++) {
      ELL_4V_SET(pld->rgba + 4*vertIdx, 255, 255, 255, 255);
    }
  }

  return 0;
}

/*
******** limnPolyDataPolarSphere
**
** makes a unit sphere, centered at the origin, parameterized around Z axis
*/
int
limnPolyDataPolarSphere(limnPolyData *pld,
                        unsigned int infoBitFlag,
                        unsigned int thetaRes, unsigned int phiRes) {
  char me[]="limnPolyDataPolarSphere", err[BIFF_STRLEN];

  if (limnPolyDataSuperquadric(pld, infoBitFlag,
                               1.0, 1.0, thetaRes, phiRes)) {
    sprintf(err, "%s: trouble", me);
    biffAdd(LIMN, err); return 1;
  }                              
  return 0;
}

int
limnPolyDataSpiralSphere(limnPolyData *pld,
                         unsigned int infoBitFlag,
                         unsigned int thetaRes,
                         unsigned int phiRes) {
  char me[]="limnPolyDataSpiralSphere", err[BIFF_STRLEN];

  if (limnPolyDataSpiralSuperquadric(pld, infoBitFlag,
                                     1.0, 1.0, thetaRes, phiRes)) {
    sprintf(err, "%s: trouble", me);
    biffAdd(LIMN, err); return 1;
  }                              
  return 0;
}

int
limnPolyDataPlane(limnPolyData *pld,
                  unsigned int infoBitFlag,
                  unsigned int uRes, unsigned int vRes) {
  char me[]="limnPolyDataPlane", err[BIFF_STRLEN];
  unsigned int vertNum, indxNum, primNum, uIdx, vIdx, vertIdx, primIdx;
  float uu, vv;

  /* sanity */
  uRes = AIR_MAX(2, uRes);
  vRes = AIR_MAX(2, vRes);

  vertNum = uRes*vRes;
  primNum = vRes-1;
  indxNum = primNum*2*uRes;
  if (limnPolyDataAlloc(pld, infoBitFlag, vertNum, indxNum, primNum)) {
    sprintf(err, "%s: couldn't allocate output", me); 
    biffAdd(LIMN, err); return 1;
  }
  
  vertIdx = 0;
  for (vIdx=0; vIdx<vRes; vIdx++) {
    vv = AIR_CAST(float, AIR_AFFINE(0, vIdx, vRes-1, -1.0, 1.0));
    for (uIdx=0; uIdx<uRes; uIdx++) {
      uu = AIR_CAST(float, AIR_AFFINE(0, uIdx, uRes-1, -1.0, 1.0));
      ELL_4V_SET(pld->xyzw + 4*vertIdx, uu, vv, 0.0, 1.0);
      if ((1 << limnPolyDataInfoNorm) & infoBitFlag) {
        ELL_4V_SET(pld->norm + 3*vertIdx, 0.0, 0.0, 1.0, 0.0);
      }
      if ((1 << limnPolyDataInfoRGBA) & infoBitFlag) {
        ELL_4V_SET(pld->rgba + 4*vertIdx, 255, 255, 255, 255);
      }
      ++vertIdx;
    }
  }

  vertIdx = 0;
  for (primIdx=0; primIdx<primNum; primIdx++) {
    for (uIdx=0; uIdx<uRes; uIdx++) {
      pld->indx[vertIdx++] = uIdx + uRes*(primIdx+1);
      pld->indx[vertIdx++] = uIdx + uRes*(primIdx);
    }    
    pld->type[primIdx] = limnPrimitiveTriangleStrip;
    pld->icnt[primIdx] = 2*uRes;
  }

  return 0;
}

/*
******** limnPolyDataTransform_f, limnPolyDataTransform_d
**
** transforms a surface (both positions, and normals (if set))
** by given homogenous transform
*/
void
limnPolyDataTransform_f(limnPolyData *pld,
                        const float homat[16]) {
  double hovec[4], mat[9], inv[9], norm[3], nmat[9];
  unsigned int vertIdx;

  if (pld && homat) {
    if (pld->norm) {
      ELL_34M_EXTRACT(mat, homat);
      ell_3m_inv_d(inv, mat);
      ELL_3M_TRANSPOSE(nmat, inv);
    } else {
      ELL_3M_IDENTITY_SET(nmat);  /* hush unused value compiler warnings */
    }
    for (vertIdx=0; vertIdx<pld->vertNum; vertIdx++) {
      ELL_4MV_MUL(hovec, homat, pld->xyzw + 4*vertIdx);
      ELL_4V_COPY_TT(pld->xyzw + 4*vertIdx, float, hovec);
      if (pld->norm) {
        ELL_3MV_MUL(norm, nmat, pld->norm + 3*vertIdx);
        ELL_3V_COPY_TT(pld->norm + 3*vertIdx, float, norm);
      }
    }
  }
  return;
}

/* !!! COPY AND PASTE !!!  (except for double homat[16]) */
void
limnPolyDataTransform_d(limnPolyData *pld, const double homat[16]) {
  double hovec[4], mat[9], inv[9], norm[3], nmat[9];
  unsigned int vertIdx;

  if (pld && homat) {
    if (pld->norm) {
      ELL_34M_EXTRACT(mat, homat);
      ell_3m_inv_d(inv, mat);
      ELL_3M_TRANSPOSE(nmat, inv);
    } else {
      ELL_3M_IDENTITY_SET(nmat);  /* hush unused value compiler warnings */
    }
    for (vertIdx=0; vertIdx<pld->vertNum; vertIdx++) {
      ELL_4MV_MUL(hovec, homat, pld->xyzw + 4*vertIdx);
      ELL_4V_COPY_TT(pld->xyzw + 4*vertIdx, float, hovec);
      if (pld->norm) {
        ELL_3MV_MUL(norm, nmat, pld->norm + 3*vertIdx);
        ELL_3V_COPY_TT(pld->norm + 3*vertIdx, float, norm);
      }
    }
  }
  return;
}

unsigned int
limnPolyDataPolygonNumber(limnPolyData *pld) {
  unsigned int ret, primIdx;

  ret = 0;
  for (primIdx=0; primIdx<pld->primNum; primIdx++) {
    switch(pld->type[primIdx]) {
    case limnPrimitiveTriangles:
      ret += pld->icnt[primIdx]/3;
      break;
    case limnPrimitiveTriangleStrip:
    case limnPrimitiveTriangleFan:
      ret += pld->icnt[primIdx] - 2;
      break;
    case limnPrimitiveQuads:
      ret += pld->icnt[primIdx]/4;
      break;
    }
  }
  return ret;
}
