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
limnTrisurfNew(unsigned int vertNum,
               unsigned int indxNum,
               unsigned int primNum) {
  limnTrisurf *tsf;

  tsf = (limnTrisurf *)calloc(1, sizeof(limnTrisurf));
  if (tsf) {
    if (!( vertNum && indxNum && primNum )) {
      tsf->vert = NULL;
      tsf->indx = NULL;
      tsf->vcnt = NULL;
    } else {
      tsf->vert = (limnVertex *)calloc(vertNum, sizeof(limnVertex));
      tsf->indx = (unsigned int *)calloc(indxNum, sizeof(unsigned int));
      tsf->ptype = (unsigned char *)calloc(primNum, sizeof(unsigned char));
      tsf->vcnt = (unsigned int *)calloc(primNum, sizeof(unsigned int));
      if (!( tsf->vert && tsf->indx && tsf->ptype && tsf->vcnt )) {
        /* allocation error. we leak. */
        return NULL;
      }
    }
    tsf->vertNum = vertNum;
    tsf->indxNum = indxNum;
    tsf->primNum = primNum;
  }
  return tsf;
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

/*
******** limnTrisurfPolarSphereNew
**
** makes a unit sphere, centered at the origin, parameterized around Z axis
*/
limnTrisurf *
limnTrisurfPolarSphereNew(unsigned int thetaRes, unsigned int phiRes) {
  /* char me[]="limnTrisurfPolarSphereNew"; */
  limnTrisurf *tsf;
  unsigned int vertNum, indxNum, primNum, triNum, stripNum,
    vertIdx, thetaIdx, phiIdx, primIdx;
  float theta, phi;

  /* sanity bounds */
  thetaRes = AIR_MAX(3, thetaRes);
  phiRes = AIR_MAX(2, phiRes);

  vertNum = 2 + thetaRes*(phiRes-1);
  triNum = 2*thetaRes;
  stripNum = phiRes-2;
  primNum = triNum + stripNum;
  indxNum = 3*triNum + 2*(thetaRes+1)*stripNum;
  tsf = limnTrisurfNew(vertNum, indxNum, primNum);
  if (!tsf) {
    return NULL;
  }
  /* else allocation is successful and done */

  vertIdx = 0;
  ELL_4V_SET(tsf->vert[vertIdx].world, 0, 0, 1, 1);
  ++vertIdx;
  for (phiIdx=1; phiIdx<phiRes; phiIdx++) {
    phi = AIR_AFFINE(0, phiIdx, phiRes, 0, AIR_PI);
    for (thetaIdx=0; thetaIdx<thetaRes; thetaIdx++) {
      theta = AIR_AFFINE(0, thetaIdx, thetaRes, 0, 2*AIR_PI);
      ELL_4V_SET(tsf->vert[vertIdx].world,
                 sin(phi)*cos(theta),
                 sin(phi)*sin(theta),
                 cos(phi),
                 1.0);
      ++vertIdx;
    }
  }
  ELL_4V_SET(tsf->vert[vertIdx].world, 0, 0, -1, 1);
  ++vertIdx;

  /* triangle fan at top */
  vertIdx = 0;
  primIdx = 0;
  tsf->indx[vertIdx++] = 0;
  for (thetaIdx=0; thetaIdx<thetaRes; thetaIdx++) {
    tsf->indx[vertIdx++] = thetaIdx + 1;
  }
  tsf->indx[vertIdx++] = 1;
  tsf->ptype[primIdx] = limnPrimitiveFan;
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
    tsf->ptype[primIdx] = limnPrimitiveStrip;
    tsf->vcnt[primIdx++] = 2*(thetaRes+1);
  }
  /* triangle fan at bottom */
  tsf->indx[vertIdx++] = vertNum-1;
  for (thetaIdx=0; thetaIdx<thetaRes; thetaIdx++) {
    tsf->indx[vertIdx++] = thetaRes*(phiRes-2) + thetaRes - thetaIdx;
  }
  tsf->indx[vertIdx++] = thetaRes*(phiRes-2) + thetaRes;
  tsf->ptype[primIdx] = limnPrimitiveFan;
  tsf->vcnt[primIdx++] = thetaRes + 2;

  /* set normals and colors */
  for (vertIdx=0; vertIdx<tsf->vertNum; vertIdx++) {
    ELL_3V_COPY(tsf->vert[vertIdx].worldNormal, tsf->vert[vertIdx].world);
    ELL_4V_SET(tsf->vert[vertIdx].rgba, 1, 1, 1, 1);
  }

  return tsf;
}

