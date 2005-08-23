/*
  Teem: Gordon Kindlmann's research software
  Copyright (C) 2005  Gordon Kindlmann
  Copyright (C) 2004, 2003, 2002, 2001, 2000, 1999, 1998  University of Utah

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


#include "limn.h"

limnSurface *
limnSurfaceNew(void) {
  limnSurface *srf;

  srf = (limnSurface *)calloc(1, sizeof(limnSurface));
  if (srf) {
    srf->vert = NULL;
    srf->indx = NULL;
    srf->type = NULL;
    srf->vcnt = NULL;
    srf->vertNum = 0;
    srf->indxNum = 0;
    srf->primNum = 0;
  }
  return srf;
}

limnSurface *
limnSurfaceNix(limnSurface *srf) {

  if (srf) {
    airFree(srf->vert);
    airFree(srf->indx);
    airFree(srf->type);
    airFree(srf->vcnt);
  }
  airFree(srf);
  return NULL;
}

int
limnSurfaceAlloc(limnSurface *srf,
                 unsigned int vertNum,
                 unsigned int indxNum,
                 unsigned int primNum) {
  char me[]="limnSurfaceAlloc", err[AIR_STRLEN_MED];
  
  if (!srf) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(LIMN, err); return 1;
  }
  if (vertNum != srf->vertNum) {
    srf->vert = (limnVrt *)airFree(srf->vert);
    if (vertNum) {
      srf->vert = (limnVrt *)calloc(vertNum, sizeof(limnVrt));
      if (!srf->vert) {
        sprintf(err, "%s: couldn't allocate %u vertices", me, vertNum);
        biffAdd(LIMN, err); return 1;
      }
    }
    srf->vertNum = vertNum;
  }
  if (indxNum != srf->indxNum) {
    srf->indx = (unsigned int *)airFree(srf->indx);
    if (indxNum) {
      srf->indx = (unsigned int *)calloc(indxNum, sizeof(unsigned int));
      if (!srf->indx) {
        sprintf(err, "%s: couldn't allocate %u indices", me, indxNum);
        biffAdd(LIMN, err); return 1;
      }
    }
    srf->indxNum = indxNum;
  }
  if (primNum != srf->primNum) {
    srf->type = (signed char *)airFree(srf->type);
    srf->vcnt = (unsigned int *)airFree(srf->vcnt);
    if (primNum) {
      srf->type = (signed char *)calloc(primNum, sizeof(signed char));
      srf->vcnt = (unsigned int *)calloc(primNum, sizeof(unsigned int));
      if (!(srf->type && srf->vcnt)) {
        sprintf(err, "%s: couldn't allocate %u primitives", me, primNum);
        biffAdd(LIMN, err); return 1;
      }
    }
    srf->primNum = primNum;
  }
  return 0;
}

size_t
limnSurfaceSize(limnSurface *srf) {
  size_t ret = 0;

  if (srf) {
    ret += srf->vertNum*sizeof(limnVrt);
    ret += srf->indxNum*sizeof(unsigned int);
    ret += srf->primNum*sizeof(signed char);
    ret += srf->primNum*sizeof(unsigned int);
  }
  return ret;
}

int
limnSurfaceCopy(limnSurface *srfB, const limnSurface *srfA) {
  char me[]="limnSurfaceCopy", err[AIR_STRLEN_MED];

  if (!( srfB && srfA )) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(LIMN, err); return 1;
  }
  if (limnSurfaceAlloc(srfB, srfA->vertNum, srfA->indxNum, srfA->primNum)) {
    sprintf(err, "%s: couldn't allocate output", me);
    biffAdd(LIMN, err); return 1;
  }
  memcpy(srfB->vert, srfA->vert, srfA->vertNum*sizeof(limnVrt));
  memcpy(srfB->indx, srfA->indx, srfA->indxNum*sizeof(unsigned int));
  memcpy(srfB->type, srfA->type, srfA->primNum*sizeof(signed char));
  memcpy(srfB->vcnt, srfA->vcnt, srfA->primNum*sizeof(unsigned int));
  return 0;
}

int
limnSurfaceCopyN(limnSurface *srfB, const limnSurface *srfA,
                 unsigned int num) {
  char me[]="limnSurfaceCopyN", err[AIR_STRLEN_MED];
  unsigned int ii, jj;

  if (!( srfB && srfA )) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(LIMN, err); return 1;
  }
  if (limnSurfaceAlloc(srfB, num*srfA->vertNum,
                       num*srfA->indxNum, num*srfA->primNum)) {
    sprintf(err, "%s: couldn't allocate output", me);
    biffAdd(LIMN, err); return 1;
  }
  for (ii=0; ii<num; ii++) {
    memcpy(srfB->vert + ii*srfA->vertNum, srfA->vert,
           srfA->vertNum*sizeof(limnVrt));
    for (jj=0; jj<srfA->indxNum; jj++) {
      (srfB->indx + ii*srfA->indxNum)[jj] = srfA->indx[jj] + ii*srfA->vertNum;
    }
    memcpy(srfB->type + ii*srfA->primNum, srfA->type,
           srfA->primNum*sizeof(signed char));
    memcpy(srfB->vcnt + ii*srfA->primNum, srfA->vcnt,
           srfA->primNum*sizeof(unsigned int));
  }
  return 0;
}

int
limnSurfaceCube(limnSurface *srf, int sharpEdge) {
  char me[]="limnSurfaceCube", err[AIR_STRLEN_MED];
  unsigned int vertNum, vertIdx, primNum, indxNum, cnum, ci;
  float cn;

  vertNum = sharpEdge ? 6*4 : 8;
  primNum = 1;
  indxNum = 6*4;
  if (limnSurfaceAlloc(srf, vertNum, indxNum, primNum)) {
    sprintf(err, "%s: couldn't allocate output", me);
    biffAdd(LIMN, err); return 1;
  }
  
  vertIdx = 0;
  cnum = sharpEdge ? 3 : 1;
  cn = sqrt(3.0);
  for (ci=0; ci<cnum; ci++) {
    ELL_4V_SET(srf->vert[vertIdx].xyzw,  -1,  -1,  -1,  1); vertIdx++;
  }
  for (ci=0; ci<cnum; ci++) {
    ELL_4V_SET(srf->vert[vertIdx].xyzw,  -1,   1,  -1,  1); vertIdx++;
  }
  for (ci=0; ci<cnum; ci++) {
    ELL_4V_SET(srf->vert[vertIdx].xyzw,   1,   1,  -1,  1); vertIdx++;
  }
  for (ci=0; ci<cnum; ci++) {
    ELL_4V_SET(srf->vert[vertIdx].xyzw,   1,  -1,  -1,  1); vertIdx++;
  }
  for (ci=0; ci<cnum; ci++) {
    ELL_4V_SET(srf->vert[vertIdx].xyzw,  -1,  -1,   1,  1); vertIdx++;
  }
  for (ci=0; ci<cnum; ci++) {
    ELL_4V_SET(srf->vert[vertIdx].xyzw,  -1,   1,   1,  1); vertIdx++;
  }
  for (ci=0; ci<cnum; ci++) {
    ELL_4V_SET(srf->vert[vertIdx].xyzw,   1,   1,   1,  1); vertIdx++;
  }
  for (ci=0; ci<cnum; ci++) {
    ELL_4V_SET(srf->vert[vertIdx].xyzw,   1,  -1,   1,  1); vertIdx++;
  }

  vertIdx = 0;
  if (sharpEdge) {
    ELL_4V_SET(srf->indx + vertIdx,  0,  3,  6,  9); vertIdx += 4;
    ELL_4V_SET(srf->indx + vertIdx,  2, 14, 16,  5); vertIdx += 4;
    ELL_4V_SET(srf->indx + vertIdx,  4, 17, 18,  8); vertIdx += 4;
    ELL_4V_SET(srf->indx + vertIdx,  7, 19, 21, 10); vertIdx += 4;
    ELL_4V_SET(srf->indx + vertIdx,  1, 11, 23, 13); vertIdx += 4;
    ELL_4V_SET(srf->indx + vertIdx, 12, 22, 20, 15); vertIdx += 4;
  } else {
    ELL_4V_SET(srf->indx + vertIdx,  0,  1,  2,  3); vertIdx += 4;
    ELL_4V_SET(srf->indx + vertIdx,  0,  4,  5,  1); vertIdx += 4;
    ELL_4V_SET(srf->indx + vertIdx,  1,  5,  6,  2); vertIdx += 4;
    ELL_4V_SET(srf->indx + vertIdx,  2,  6,  7,  3); vertIdx += 4;
    ELL_4V_SET(srf->indx + vertIdx,  0,  3,  7,  4); vertIdx += 4;
    ELL_4V_SET(srf->indx + vertIdx,  4,  7,  6,  5); vertIdx += 4;
  }

  if (sharpEdge) {
    ELL_3V_SET(srf->vert[ 0].norm,  0,  0, -1);
    ELL_3V_SET(srf->vert[ 3].norm,  0,  0, -1);
    ELL_3V_SET(srf->vert[ 6].norm,  0,  0, -1);
    ELL_3V_SET(srf->vert[ 9].norm,  0,  0, -1);
    ELL_3V_SET(srf->vert[ 2].norm, -1,  0,  0);
    ELL_3V_SET(srf->vert[ 5].norm, -1,  0,  0);
    ELL_3V_SET(srf->vert[14].norm, -1,  0,  0);
    ELL_3V_SET(srf->vert[16].norm, -1,  0,  0);
    ELL_3V_SET(srf->vert[ 4].norm,  0,  1,  0);
    ELL_3V_SET(srf->vert[ 8].norm,  0,  1,  0);
    ELL_3V_SET(srf->vert[17].norm,  0,  1,  0);
    ELL_3V_SET(srf->vert[18].norm,  0,  1,  0);
    ELL_3V_SET(srf->vert[ 7].norm,  1,  0,  0);
    ELL_3V_SET(srf->vert[10].norm,  1,  0,  0);
    ELL_3V_SET(srf->vert[19].norm,  1,  0,  0);
    ELL_3V_SET(srf->vert[21].norm,  1,  0,  0);
    ELL_3V_SET(srf->vert[ 1].norm,  0, -1,  0);
    ELL_3V_SET(srf->vert[11].norm,  0, -1,  0);
    ELL_3V_SET(srf->vert[13].norm,  0, -1,  0);
    ELL_3V_SET(srf->vert[23].norm,  0, -1,  0);
    ELL_3V_SET(srf->vert[12].norm,  0,  0,  1);
    ELL_3V_SET(srf->vert[15].norm,  0,  0,  1);
    ELL_3V_SET(srf->vert[20].norm,  0,  0,  1);
    ELL_3V_SET(srf->vert[22].norm,  0,  0,  1);
  } else {
    ELL_3V_SET(srf->vert[0].norm, -cn, -cn, -cn);
    ELL_3V_SET(srf->vert[1].norm, -cn,  cn, -cn);
    ELL_3V_SET(srf->vert[2].norm,  cn,  cn, -cn);
    ELL_3V_SET(srf->vert[3].norm,  cn, -cn, -cn);
    ELL_3V_SET(srf->vert[4].norm, -cn, -cn,  cn);
    ELL_3V_SET(srf->vert[5].norm, -cn,  cn,  cn);
    ELL_3V_SET(srf->vert[6].norm,  cn,  cn,  cn);
    ELL_3V_SET(srf->vert[7].norm,  cn, -cn,  cn);
  }

  srf->type[0] = limnPrimitiveQuads;
  srf->vcnt[0] = indxNum;
  
  /* set colors */
  for (vertIdx=0; vertIdx<srf->vertNum; vertIdx++) {
    ELL_4V_SET(srf->vert[vertIdx].rgba, 255, 255, 255, 255);
  }

  return 0;
}

int
limnSurfaceCylinder(limnSurface *srf, unsigned int thetaRes, int sharpEdge) {
  char me[]="limnSurfaceCylinderNew", err[AIR_STRLEN_MED];
  unsigned int vertNum, primNum, primIdx, indxNum, thetaIdx, vertIdx, blah;
  float theta, cth, sth, sq2;

  /* sanity bounds */
  thetaRes = AIR_MAX(3, thetaRes);

  vertNum = sharpEdge ? 4*thetaRes : 2*thetaRes;
  primNum = 3;
  indxNum = 2*thetaRes + 2*(thetaRes+1);  /* 2 fans + 1 strip */
  if (limnSurfaceAlloc(srf, vertNum, indxNum, primNum)) {
    sprintf(err, "%s: couldn't allocate output", me); 
    biffAdd(LIMN, err); return 1;
  }
  
  vertIdx = 0;
  for (blah=0; blah < (sharpEdge ? 2 : 1); blah++) {
    for (thetaIdx=0; thetaIdx<thetaRes; thetaIdx++) {
      theta = AIR_AFFINE(0, thetaIdx, thetaRes, 0, 2*AIR_PI);
      ELL_4V_SET(srf->vert[vertIdx].xyzw, cos(theta), sin(theta), 1, 1);
      /*
      fprintf(stderr, "!%s: vert[%u] = %g %g %g\n", me, vertIdx,
              srf->vert[vertIdx].xyzw[0],
              srf->vert[vertIdx].xyzw[1],
              srf->vert[vertIdx].xyzw[2]);
      */
      ++vertIdx;
    }
  }
  for (blah=0; blah < (sharpEdge ? 2 : 1); blah++) {
    for (thetaIdx=0; thetaIdx<thetaRes; thetaIdx++) {
      theta = AIR_AFFINE(0, thetaIdx, thetaRes, 0, 2*AIR_PI);
      ELL_4V_SET(srf->vert[vertIdx].xyzw, cos(theta), sin(theta), -1, 1);
      /*
      fprintf(stderr, "!%s: vert[%u] = %g %g %g\n", me, vertIdx,
              srf->vert[vertIdx].xyzw[0],
              srf->vert[vertIdx].xyzw[1],
              srf->vert[vertIdx].xyzw[2]);
      */
      ++vertIdx;
    }
  }

  primIdx = 0;
  vertIdx = 0;

  /* fan on top */
  for (thetaIdx=0; thetaIdx<thetaRes; thetaIdx++) {
    srf->indx[vertIdx++] = thetaIdx;
  }
  srf->type[primIdx] = limnPrimitiveTriangleFan;
  srf->vcnt[primIdx] = thetaRes;
  primIdx++;

  /* single strip around */
  for (thetaIdx=0; thetaIdx<thetaRes; thetaIdx++) {
    srf->indx[vertIdx++] = (sharpEdge ? 1 : 0)*thetaRes + thetaIdx;
    srf->indx[vertIdx++] = (sharpEdge ? 2 : 1)*thetaRes + thetaIdx;
  }
  srf->indx[vertIdx++] = (sharpEdge ? 1 : 0)*thetaRes;
  srf->indx[vertIdx++] = (sharpEdge ? 2 : 1)*thetaRes;
  srf->type[primIdx] = limnPrimitiveTriangleStrip;
  srf->vcnt[primIdx] = 2*(thetaRes+1);
  primIdx++;

  /* fan on bottom */
  for (thetaIdx=0; thetaIdx<thetaRes; thetaIdx++) {
    srf->indx[vertIdx++] = (sharpEdge ? 3 : 1)*thetaRes + thetaIdx;
  }
  srf->type[primIdx] = limnPrimitiveTriangleFan;
  srf->vcnt[primIdx] = thetaRes;
  primIdx++;

  /* set normals */
  sq2 = sqrt(2.0);
  if (sharpEdge) {
    for (thetaIdx=0; thetaIdx<thetaRes; thetaIdx++) {
      theta = AIR_AFFINE(0, thetaIdx, thetaRes, 0, 2*AIR_PI);
      cth = cos(theta);
      sth = sin(theta);
      ELL_3V_SET(srf->vert[thetaIdx + 0*thetaRes].norm, 0, 0, 1);
      ELL_3V_SET(srf->vert[thetaIdx + 1*thetaRes].norm, cth, sth, 0);
      ELL_3V_SET(srf->vert[thetaIdx + 2*thetaRes].norm, cth, sth, 0);
      ELL_3V_SET(srf->vert[thetaIdx + 3*thetaRes].norm, 0, 0, -1);
    }
  } else {
    for (thetaIdx=0; thetaIdx<thetaRes; thetaIdx++) {
      theta = AIR_AFFINE(0, thetaIdx, thetaRes, 0, 2*AIR_PI);
      cth = sq2*cos(theta);
      sth = sq2*sin(theta);
      ELL_3V_SET(srf->vert[thetaIdx + 0*thetaRes].norm, cth, sth, sq2);
      ELL_3V_SET(srf->vert[thetaIdx + 1*thetaRes].norm, cth, sth, -sq2);
    }
  }

  /* set colors */
  for (vertIdx=0; vertIdx<srf->vertNum; vertIdx++) {
    ELL_4V_SET(srf->vert[vertIdx].rgba, 255, 255, 255, 255);
  }

  return 0;
}


/*
******** limnSurfaceSuperquadric
**
** makes a superquadric parameterized around the Z axis
*/
int
limnSurfaceSuperquadric(limnSurface *srf,
                        float alpha, float beta,
                        unsigned int thetaRes, unsigned int phiRes) {
  char me[]="limnSurfaceSuperquadric", err[AIR_STRLEN_MED];
  unsigned int vertIdx, vertNum, fanNum, stripNum, primNum, indxNum,
    thetaIdx, phiIdx, primIdx;
  float theta, phi;

  /* sanity bounds */
  thetaRes = AIR_MAX(3, thetaRes);
  phiRes = AIR_MAX(2, phiRes);
  alpha = AIR_MAX(0.00001, alpha);
  beta = AIR_MAX(0.00001, beta);

  vertNum = 2 + thetaRes*(phiRes-1);
  fanNum = 2;
  stripNum = phiRes-2;
  primNum = fanNum + stripNum;
  indxNum = (thetaRes+2)*fanNum + 2*(thetaRes+1)*stripNum;
  if (limnSurfaceAlloc(srf, vertNum, indxNum, primNum)) {
    sprintf(err, "%s: couldn't allocate output", me);
    biffAdd(LIMN, err); return 1;
  }

  vertIdx = 0;
  ELL_4V_SET(srf->vert[vertIdx].xyzw, 0, 0, 1, 1);
  ELL_3V_SET(srf->vert[vertIdx].norm, 0, 0, 1);
  ++vertIdx;
  for (phiIdx=1; phiIdx<phiRes; phiIdx++) {
    float cost, sint, cosp, sinp;
    phi = AIR_AFFINE(0, phiIdx, phiRes, 0, AIR_PI);
    cosp = cos(phi);
    sinp = sin(phi);
    for (thetaIdx=0; thetaIdx<thetaRes; thetaIdx++) {
      theta = AIR_AFFINE(0, thetaIdx, thetaRes, 0, 2*AIR_PI);
      cost = cos(theta);
      sint = sin(theta);
      ELL_4V_SET(srf->vert[vertIdx].xyzw,
                 airSgnPow(cost,alpha) * airSgnPow(sinp,beta),
                 airSgnPow(sint,alpha) * airSgnPow(sinp,beta),
                 airSgnPow(cosp,beta),
                 1.0);
      if (1 == alpha && 1 == beta) {
        ELL_3V_COPY(srf->vert[vertIdx].norm, srf->vert[vertIdx].xyzw);
      } else {
        ELL_3V_SET(srf->vert[vertIdx].norm,
                   2*airSgnPow(cost,2-alpha)*airSgnPow(sinp,2-beta)/beta,
                   2*airSgnPow(sint,2-alpha)*airSgnPow(sinp,2-beta)/beta,
                   2*airSgnPow(cosp,2-beta)/beta);
      }
      ++vertIdx;
    }
  }
  ELL_4V_SET(srf->vert[vertIdx].xyzw, 0, 0, -1, 1);
  ELL_3V_SET(srf->vert[vertIdx].norm, 0, 0, -1);
  ++vertIdx;

  /* triangle fan at top */
  vertIdx = 0;
  primIdx = 0;
  srf->indx[vertIdx++] = 0;
  for (thetaIdx=0; thetaIdx<thetaRes; thetaIdx++) {
    srf->indx[vertIdx++] = thetaIdx + 1;
  }
  srf->indx[vertIdx++] = 1;
  srf->type[primIdx] = limnPrimitiveTriangleFan;
  srf->vcnt[primIdx++] = thetaRes + 2;

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
      srf->indx[vertIdx++] = (phiIdx-1)*thetaRes + thetaIdx + 1;
      srf->indx[vertIdx++] = phiIdx*thetaRes + thetaIdx + 1;
    }
    /*
    fprintf(stderr, " [%u %u] %u %u (%u verts)\n", 
            vertIdx, vertIdx + 1,
            (phiIdx-1)*thetaRes + 1,
            phiIdx*thetaRes + 1, 2*(thetaRes+1));
    */
    srf->indx[vertIdx++] = (phiIdx-1)*thetaRes + 1;
    srf->indx[vertIdx++] = phiIdx*thetaRes + 1;
    srf->type[primIdx] = limnPrimitiveTriangleStrip;
    srf->vcnt[primIdx++] = 2*(thetaRes+1);
  }

  /* triangle fan at bottom */
  srf->indx[vertIdx++] = vertNum-1;
  for (thetaIdx=0; thetaIdx<thetaRes; thetaIdx++) {
    srf->indx[vertIdx++] = thetaRes*(phiRes-2) + thetaRes - thetaIdx;
  }
  srf->indx[vertIdx++] = thetaRes*(phiRes-2) + thetaRes;
  srf->type[primIdx] = limnPrimitiveTriangleFan;
  srf->vcnt[primIdx++] = thetaRes + 2;

  /* set colors to all white */
  for (vertIdx=0; vertIdx<srf->vertNum; vertIdx++) {
    ELL_4V_SET(srf->vert[vertIdx].rgba, 255, 255, 255, 255);
  }

  return 0;
}

/*
******** limnSurfacePolarSphere
**
** makes a unit sphere, centered at the origin, parameterized around Z axis
*/
int
limnSurfacePolarSphere(limnSurface *srf,
                       unsigned int thetaRes, unsigned int phiRes) {
  char me[]="limnSurfacePolarSphere", err[AIR_STRLEN_MED];

  if (limnSurfaceSuperquadric(srf, 1.0, 1.0, thetaRes, phiRes)) {
    sprintf(err, "%s: trouble", me);
    biffAdd(LIMN, err); return 1;
  }                              
  return 0;
}

/*
******** limnSurfaceTransform_f, limnSurfaceTransform_d
**
** transforms a surface (vertex positions in limnVrt.xyzw and normals
** in limnVrt.norm) by given homogenous transform
*/
void
limnSurfaceTransform_f(limnSurface *srf, const float homat[16]) {
  double hovec[4], mat[9], inv[9], norm[3], nmat[9];
  unsigned int vertIdx;

  if (srf && homat) {
    ELL_34M_EXTRACT(mat, homat);
    ell_3m_inv_d(inv, mat);
    ELL_3M_TRANSPOSE(nmat, inv);
    for (vertIdx=0; vertIdx<srf->vertNum; vertIdx++) {
      ELL_4MV_MUL(hovec, homat, srf->vert[vertIdx].xyzw);
      ELL_4V_COPY(srf->vert[vertIdx].xyzw, hovec);
      ELL_3MV_MUL(norm, nmat, srf->vert[vertIdx].norm);
      ELL_3V_COPY(srf->vert[vertIdx].norm, norm);
    }
  }
  return;
}

/* !!! COPY AND PASTE !!! */
void
limnSurfaceTransform_d(limnSurface *srf, const double homat[16]) {
  double hovec[4], mat[9], inv[9], norm[3], nmat[9];
  unsigned int vertIdx;

  if (srf && homat) {
    ELL_34M_EXTRACT(mat, homat);
    ell_3m_inv_d(inv, mat);
    ELL_3M_TRANSPOSE(nmat, inv);
    for (vertIdx=0; vertIdx<srf->vertNum; vertIdx++) {
      ELL_4MV_MUL(hovec, homat, srf->vert[vertIdx].xyzw);
      ELL_4V_COPY(srf->vert[vertIdx].xyzw, hovec);
      ELL_3MV_MUL(norm, nmat, srf->vert[vertIdx].norm);
      ELL_3V_COPY(srf->vert[vertIdx].norm, norm);
    }
  }
  return;
}

unsigned int
limnSurfacePolygonNumber(limnSurface *srf) {
  unsigned int ret, primIdx;

  ret = 0;
  for (primIdx=0; primIdx<srf->primNum; primIdx++) {
    switch(srf->type[primIdx]) {
    case limnPrimitiveTriangles:
      ret += srf->vcnt[primIdx]/3;
      break;
    case limnPrimitiveTriangleStrip:
    case limnPrimitiveTriangleFan:
      ret += srf->vcnt[primIdx] - 2;
      break;
    case limnPrimitiveQuads:
      ret += srf->vcnt[primIdx]/4;
      break;
    }
  }
  return ret;
}
