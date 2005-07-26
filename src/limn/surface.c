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

limnTrisurf *
limnTrisurfNew(void) {
  limnTrisurf *tsf;

  tsf = (limnTrisurf *)calloc(1, sizeof(limnTrisurf));
  if (tsf) {
    tsf->vert = NULL;
    tsf->indx = NULL;
    tsf->type = NULL;
    tsf->vcnt = NULL;
    tsf->vertNum = 0;
    tsf->indxNum = 0;
    tsf->primNum = 0;
  }
  return tsf;
}

int
limnTrisurfAlloc(limnTrisurf *tsf,
                 unsigned int vertNum,
                 unsigned int indxNum,
                 unsigned int primNum) {
  char me[]="limnTrisurfAlloc", err[AIR_STRLEN_MED];
  
  if (!tsf) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(LIMN, err); return 1;
  }
  if (vertNum != tsf->vertNum) {
    tsf->vert = (limnVrt *)airFree(tsf->vert);
    if (vertNum) {
      tsf->vert = (limnVrt *)calloc(vertNum, sizeof(limnVrt));
      if (!tsf->vert) {
        sprintf(err, "%s: couldn't allocate %u vertices", me, vertNum);
        biffAdd(LIMN, err); return 1;
      }
    }
    tsf->vertNum = vertNum;
  }
  if (indxNum != tsf->indxNum) {
    tsf->indx = (unsigned int *)airFree(tsf->indx);
    if (indxNum) {
      tsf->indx = (unsigned int *)calloc(indxNum, sizeof(unsigned int));
      if (!tsf->indx) {
        sprintf(err, "%s: couldn't allocate %u indices", me, indxNum);
        biffAdd(LIMN, err); return 1;
      }
    }
    tsf->indxNum = indxNum;
  }
  if (primNum != tsf->primNum) {
    tsf->type = (signed char *)airFree(tsf->type);
    tsf->vcnt = (unsigned int *)airFree(tsf->vcnt);
    if (primNum) {
      tsf->type = (signed char *)calloc(primNum, sizeof(signed char));
      tsf->vcnt = (unsigned int *)calloc(primNum, sizeof(unsigned int));
      if (!(tsf->type && tsf->vcnt)) {
        sprintf(err, "%s: couldn't allocate %u primitives", me, primNum);
        biffAdd(LIMN, err); return 1;
      }
    }
    tsf->primNum = primNum;
  }
  return 0;
}

limnTrisurf *
limnTrisurfCopy(const limnTrisurf *otsf) {
  limnTrisurf *ntsf;

  ntsf = limnTrisurfNew(otsf->vertNum, otsf->indxNum, otsf->primNum);
  if (ntsf) {
    memcpy(ntsf->vert, otsf->vert, otsf->vertNum*sizeof(limnVertex));
    memcpy(ntsf->indx, otsf->indx, otsf->indxNum*sizeof(unsigned int));
    memcpy(ntsf->ptype, otsf->ptype, otsf->primNum*sizeof(unsigned char));
    memcpy(ntsf->vcnt, otsf->vcnt, otsf->primNum*sizeof(unsigned int));
  }
  return ntsf;
}

limnTrisurf *
limnTrisurfNix(limnTrisurf *tsf) {

  if (tsf) {
    airFree(tsf->vert);
    airFree(tsf->indx);
    airFree(tsf->ptype);
    airFree(tsf->vcnt);
  }
  airFree(tsf);
  return NULL;
}

limnTrisurf *
limnTrisurfCubeNew(int sharpEdge) {
  unsigned int vertNum, vertIdx, primNum, indxNum, cnum, ci;
  limnTrisurf *tsf=NULL;
  float cn;

  vertNum = sharpEdge ? 6*4 : 8;
  primNum = 1;
  indxNum = 6*4;
  tsf = limnTrisurfNew(vertNum, indxNum, primNum);
  if (!tsf) {
    return NULL;
  }

  vertIdx = 0;
  cnum = sharpEdge ? 3 : 1;
  cn = sqrt(3.0);
  for (ci=0; ci<cnum; ci++) {
    ELL_4V_SET(tsf->vert[vertIdx].xyzw,  -1,  -1,  -1,  1); vertIdx++;
  }
  for (ci=0; ci<cnum; ci++) {
    ELL_4V_SET(tsf->vert[vertIdx].xyzw,  -1,   1,  -1,  1); vertIdx++;
  }
  for (ci=0; ci<cnum; ci++) {
    ELL_4V_SET(tsf->vert[vertIdx].xyzw,   1,   1,  -1,  1); vertIdx++;
  }
  for (ci=0; ci<cnum; ci++) {
    ELL_4V_SET(tsf->vert[vertIdx].xyzw,   1,  -1,  -1,  1); vertIdx++;
  }
  for (ci=0; ci<cnum; ci++) {
    ELL_4V_SET(tsf->vert[vertIdx].xyzw,  -1,  -1,   1,  1); vertIdx++;
  }
  for (ci=0; ci<cnum; ci++) {
    ELL_4V_SET(tsf->vert[vertIdx].xyzw,  -1,   1,   1,  1); vertIdx++;
  }
  for (ci=0; ci<cnum; ci++) {
    ELL_4V_SET(tsf->vert[vertIdx].xyzw,   1,   1,   1,  1); vertIdx++;
  }
  for (ci=0; ci<cnum; ci++) {
    ELL_4V_SET(tsf->vert[vertIdx].xyzw,   1,  -1,   1,  1); vertIdx++;
  }

  vertIdx = 0;
  if (sharpEdge) {
    ELL_4V_SET(tsf->indx + vertIdx,  0,  3,  6,  9); vertIdx += 4;
    ELL_4V_SET(tsf->indx + vertIdx,  2, 14, 16,  5); vertIdx += 4;
    ELL_4V_SET(tsf->indx + vertIdx,  4, 17, 18,  8); vertIdx += 4;
    ELL_4V_SET(tsf->indx + vertIdx,  7, 19, 21, 10); vertIdx += 4;
    ELL_4V_SET(tsf->indx + vertIdx,  1, 11, 23, 13); vertIdx += 4;
    ELL_4V_SET(tsf->indx + vertIdx, 12, 22, 20, 15); vertIdx += 4;
  } else {
    ELL_4V_SET(tsf->indx + vertIdx,  0,  1,  2,  3); vertIdx += 4;
    ELL_4V_SET(tsf->indx + vertIdx,  0,  4,  5,  1); vertIdx += 4;
    ELL_4V_SET(tsf->indx + vertIdx,  1,  5,  6,  2); vertIdx += 4;
    ELL_4V_SET(tsf->indx + vertIdx,  2,  6,  7,  3); vertIdx += 4;
    ELL_4V_SET(tsf->indx + vertIdx,  0,  3,  7,  4); vertIdx += 4;
    ELL_4V_SET(tsf->indx + vertIdx,  4,  7,  6,  5); vertIdx += 4;
  }

  if (sharpEdge) {
    ELL_3V_SET(tsf->vert[ 0].norm,  0,  0, -1);
    ELL_3V_SET(tsf->vert[ 3].norm,  0,  0, -1);
    ELL_3V_SET(tsf->vert[ 6].norm,  0,  0, -1);
    ELL_3V_SET(tsf->vert[ 9].norm,  0,  0, -1);
    ELL_3V_SET(tsf->vert[ 2].norm, -1,  0,  0);
    ELL_3V_SET(tsf->vert[ 5].norm, -1,  0,  0);
    ELL_3V_SET(tsf->vert[14].norm, -1,  0,  0);
    ELL_3V_SET(tsf->vert[16].norm, -1,  0,  0);
    ELL_3V_SET(tsf->vert[ 4].norm,  0,  1,  0);
    ELL_3V_SET(tsf->vert[ 8].norm,  0,  1,  0);
    ELL_3V_SET(tsf->vert[17].norm,  0,  1,  0);
    ELL_3V_SET(tsf->vert[18].norm,  0,  1,  0);
    ELL_3V_SET(tsf->vert[ 7].norm,  1,  0,  0);
    ELL_3V_SET(tsf->vert[10].norm,  1,  0,  0);
    ELL_3V_SET(tsf->vert[19].norm,  1,  0,  0);
    ELL_3V_SET(tsf->vert[21].norm,  1,  0,  0);
    ELL_3V_SET(tsf->vert[ 1].norm,  0, -1,  0);
    ELL_3V_SET(tsf->vert[11].norm,  0, -1,  0);
    ELL_3V_SET(tsf->vert[13].norm,  0, -1,  0);
    ELL_3V_SET(tsf->vert[23].norm,  0, -1,  0);
    ELL_3V_SET(tsf->vert[12].norm,  0,  0,  1);
    ELL_3V_SET(tsf->vert[15].norm,  0,  0,  1);
    ELL_3V_SET(tsf->vert[20].norm,  0,  0,  1);
    ELL_3V_SET(tsf->vert[22].norm,  0,  0,  1);
  } else {
    ELL_3V_SET(tsf->vert[0].norm, -cn, -cn, -cn);
    ELL_3V_SET(tsf->vert[1].norm, -cn,  cn, -cn);
    ELL_3V_SET(tsf->vert[2].norm,  cn,  cn, -cn);
    ELL_3V_SET(tsf->vert[3].norm,  cn, -cn, -cn);
    ELL_3V_SET(tsf->vert[4].norm, -cn, -cn,  cn);
    ELL_3V_SET(tsf->vert[5].norm, -cn,  cn,  cn);
    ELL_3V_SET(tsf->vert[6].norm,  cn,  cn,  cn);
    ELL_3V_SET(tsf->vert[7].norm,  cn, -cn,  cn);
  }

  tsf->ptype[0] = limnPrimitiveQuads;
  tsf->vcnt[0] = indxNum;
  
  /* set colors */
  for (vertIdx=0; vertIdx<tsf->vertNum; vertIdx++) {
    ELL_4V_SET(tsf->vert[vertIdx].rgba, 255, 255, 255, 255);
  }

  return tsf;
}

limnTrisurf *
limnTrisurfCylinderNew(unsigned int thetaRes, int sharpEdge) {
  /* char me[]="limnTrisurfCylinderNew"; */
  unsigned int vertNum, primNum, primIdx, indxNum, thetaIdx, vertIdx, blah;
  float theta, cth, sth, sq2;
  limnTrisurf *tsf;
  
  /* sanity bounds */
  thetaRes = AIR_MAX(3, thetaRes);

  vertNum = sharpEdge ? 4*thetaRes : 2*thetaRes;
  primNum = 3;
  indxNum = 2*thetaRes + 2*(thetaRes+1);  /* 2 fans + 1 strip */
  tsf = limnTrisurfNew(vertNum, indxNum, primNum);
  if (!tsf) {
    return NULL;
  }
  
  vertIdx = 0;
  for (blah=0; blah < (sharpEdge ? 2 : 1); blah++) {
    for (thetaIdx=0; thetaIdx<thetaRes; thetaIdx++) {
      theta = AIR_AFFINE(0, thetaIdx, thetaRes, 0, 2*AIR_PI);
      ELL_4V_SET(tsf->vert[vertIdx].xyzw, cos(theta), sin(theta), 1, 1);
      /*
      fprintf(stderr, "!%s: vert[%u] = %g %g %g\n", me, vertIdx,
              tsf->vert[vertIdx].xyzw[0],
              tsf->vert[vertIdx].xyzw[1],
              tsf->vert[vertIdx].xyzw[2]);
      */
      ++vertIdx;
    }
  }
  for (blah=0; blah < (sharpEdge ? 2 : 1); blah++) {
    for (thetaIdx=0; thetaIdx<thetaRes; thetaIdx++) {
      theta = AIR_AFFINE(0, thetaIdx, thetaRes, 0, 2*AIR_PI);
      ELL_4V_SET(tsf->vert[vertIdx].xyzw, cos(theta), sin(theta), -1, 1);
      /*
      fprintf(stderr, "!%s: vert[%u] = %g %g %g\n", me, vertIdx,
              tsf->vert[vertIdx].xyzw[0],
              tsf->vert[vertIdx].xyzw[1],
              tsf->vert[vertIdx].xyzw[2]);
      */
      ++vertIdx;
    }
  }

  primIdx = 0;
  vertIdx = 0;

  /* fan on top */
  for (thetaIdx=0; thetaIdx<thetaRes; thetaIdx++) {
    tsf->indx[vertIdx++] = thetaIdx;
  }
  tsf->ptype[primIdx] = limnPrimitiveTriangleFan;
  tsf->vcnt[primIdx] = thetaRes;
  primIdx++;

  /* single strip around */
  for (thetaIdx=0; thetaIdx<thetaRes; thetaIdx++) {
    tsf->indx[vertIdx++] = (sharpEdge ? 1 : 0)*thetaRes + thetaIdx;
    tsf->indx[vertIdx++] = (sharpEdge ? 2 : 1)*thetaRes + thetaIdx;
  }
  tsf->indx[vertIdx++] = (sharpEdge ? 1 : 0)*thetaRes;
  tsf->indx[vertIdx++] = (sharpEdge ? 2 : 1)*thetaRes;
  tsf->ptype[primIdx] = limnPrimitiveTriangleStrip;
  tsf->vcnt[primIdx] = 2*(thetaRes+1);
  primIdx++;

  /* fan on bottom */
  for (thetaIdx=0; thetaIdx<thetaRes; thetaIdx++) {
    tsf->indx[vertIdx++] = (sharpEdge ? 3 : 1)*thetaRes + thetaIdx;
  }
  tsf->ptype[primIdx] = limnPrimitiveTriangleFan;
  tsf->vcnt[primIdx] = thetaRes;
  primIdx++;

  /* set normals */
  sq2 = sqrt(2.0);
  if (sharpEdge) {
    for (thetaIdx=0; thetaIdx<thetaRes; thetaIdx++) {
      theta = AIR_AFFINE(0, thetaIdx, thetaRes, 0, 2*AIR_PI);
      cth = cos(theta);
      sth = sin(theta);
      ELL_3V_SET(tsf->vert[thetaIdx + 0*thetaRes].norm, 0, 0, 1);
      ELL_3V_SET(tsf->vert[thetaIdx + 1*thetaRes].norm, cth, sth, 0);
      ELL_3V_SET(tsf->vert[thetaIdx + 2*thetaRes].norm, cth, sth, 0);
      ELL_3V_SET(tsf->vert[thetaIdx + 3*thetaRes].norm, 0, 0, -1);
    }
  } else {
    for (thetaIdx=0; thetaIdx<thetaRes; thetaIdx++) {
      theta = AIR_AFFINE(0, thetaIdx, thetaRes, 0, 2*AIR_PI);
      cth = sq2*cos(theta);
      sth = sq2*sin(theta);
      ELL_3V_SET(tsf->vert[thetaIdx + 0*thetaRes].norm, cth, sth, sq2);
      ELL_3V_SET(tsf->vert[thetaIdx + 1*thetaRes].norm, cth, sth, -sq2);
    }
  }

  /* set colors */
  for (vertIdx=0; vertIdx<tsf->vertNum; vertIdx++) {
    ELL_4V_SET(tsf->vert[vertIdx].rgba, 255, 255, 255, 255);
  }

  return tsf;
}


/*
******** limnTrisurfPolarSphereNew
**
** makes a unit sphere, centered at the origin, parameterized around Z axis
*/
limnTrisurf *
limnTrisurfPolarSphereNew(unsigned int thetaRes, unsigned int phiRes) {
  /* char me[]="limnTrisurfPolarSphereNew"; */
  unsigned int vertIdx, vertNum, fanNum, stripNum, primNum, indxNum,
    thetaIdx, phiIdx, primIdx;
  float theta, phi;

  /* sanity bounds */
  thetaRes = AIR_MAX(3, thetaRes);
  phiRes = AIR_MAX(2, phiRes);

  vertNum = 2 + thetaRes*(phiRes-1);
  fanNum = 2;
  stripNum = phiRes-2;
  primNum = fanNum + stripNum;
  indxNum = (thetaRes+2)*fanNum + 2*(thetaRes+1)*stripNum;
  limnTrisurf *tsf = limnTrisurfNew(vertNum, indxNum, primNum);
  if (!tsf) {
    return NULL;
  }
  /* else allocation is successful and done */

  vertIdx = 0;
  ELL_4V_SET(tsf->vert[vertIdx].xyzw, 0, 0, 1, 1);
  ++vertIdx;
  for (phiIdx=1; phiIdx<phiRes; phiIdx++) {
    phi = AIR_AFFINE(0, phiIdx, phiRes, 0, AIR_PI);
    for (thetaIdx=0; thetaIdx<thetaRes; thetaIdx++) {
      theta = AIR_AFFINE(0, thetaIdx, thetaRes, 0, 2*AIR_PI);
      ELL_4V_SET(tsf->vert[vertIdx].xyzw,
                 sin(phi)*cos(theta),
                 sin(phi)*sin(theta),
                 cos(phi),
                 1.0);
      ++vertIdx;
    }
  }
  ELL_4V_SET(tsf->vert[vertIdx].xyzw, 0, 0, -1, 1);
  ++vertIdx;

  /* triangle fan at top */
  vertIdx = 0;
  primIdx = 0;
  tsf->indx[vertIdx++] = 0;
  for (thetaIdx=0; thetaIdx<thetaRes; thetaIdx++) {
    tsf->indx[vertIdx++] = thetaIdx + 1;
  }
  tsf->indx[vertIdx++] = 1;
  tsf->ptype[primIdx] = limnPrimitiveTriangleFan;
  tsf->vcnt[primIdx++] = thetaRes + 2;

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
      tsf->indx[vertIdx++] = (phiIdx-1)*thetaRes + thetaIdx + 1;
      tsf->indx[vertIdx++] = phiIdx*thetaRes + thetaIdx + 1;
    }
    /*
    fprintf(stderr, " [%u %u] %u %u (%u verts)\n", 
            vertIdx, vertIdx + 1,
            (phiIdx-1)*thetaRes + 1,
            phiIdx*thetaRes + 1, 2*(thetaRes+1));
    */
    tsf->indx[vertIdx++] = (phiIdx-1)*thetaRes + 1;
    tsf->indx[vertIdx++] = phiIdx*thetaRes + 1;
    tsf->ptype[primIdx] = limnPrimitiveTriangleStrip;
    tsf->vcnt[primIdx++] = 2*(thetaRes+1);
  }

  /* triangle fan at bottom */
  tsf->indx[vertIdx++] = vertNum-1;
  for (thetaIdx=0; thetaIdx<thetaRes; thetaIdx++) {
    tsf->indx[vertIdx++] = thetaRes*(phiRes-2) + thetaRes - thetaIdx;
  }
  tsf->indx[vertIdx++] = thetaRes*(phiRes-2) + thetaRes;
  tsf->ptype[primIdx] = limnPrimitiveTriangleFan;
  tsf->vcnt[primIdx++] = thetaRes + 2;

  /* set normals and colors */
  for (vertIdx=0; vertIdx<tsf->vertNum; vertIdx++) {
    ELL_3V_COPY(tsf->vert[vertIdx].norm, tsf->vert[vertIdx].xyzw);
    ELL_4V_SET(tsf->vert[vertIdx].rgba, 255, 255, 255, 255);
  }

  return tsf;
}

