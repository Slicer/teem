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

int
limnObjectLookAdd(limnObject *obj) {
  int lookIdx;
  limnLook *look;

  lookIdx = airArrayIncrLen(obj->lookArr, 1);
  look = &(obj->look[lookIdx]);
  ELL_4V_SET(look->rgba, 1, 1, 1, 1);
  ELL_3V_SET(look->kads, 0.0, 1.0, 0.0);
  look->spow = 50;
  return lookIdx;
}


limnObject *
limnObjectNew(int incr, int doEdges) {
  limnObject *obj;

  obj = (limnObject *)calloc(1, sizeof(limnObject));
  obj->vert = NULL;
  obj->edge = NULL;
  obj->face = NULL;
  obj->faceSort = NULL;
  obj->part = NULL;
  obj->look = NULL;

  /* create all various airArrays */
  obj->vertArr = airArrayNew((void**)&(obj->vert), &(obj->vertNum), 
                             sizeof(limnVertex), incr);
  obj->edgeArr = airArrayNew((void**)&(obj->edge), &(obj->edgeNum),
                             sizeof(limnEdge), incr);
  obj->faceArr = airArrayNew((void**)&(obj->face), &(obj->faceNum),
                             sizeof(limnFace), incr);
  obj->partArr = airArrayNew((void**)&(obj->part), &(obj->partNum),
                             sizeof(limnPart*), incr);
  obj->lookArr = airArrayNew((void**)&(obj->look), &(obj->lookNum),
                             sizeof(limnLook), incr);

  /* create (default) look 0 */
  limnObjectLookAdd(obj);

  obj->doEdges = doEdges;
  obj->incr = incr;
    
  return obj;
}

limnPart *
_limnObjectPartNix(limnPart *part) {

  if (part) {
    airArrayNuke(part->vertIdxArr);
    airArrayNuke(part->edgeIdxArr);
    airArrayNuke(part->faceIdxArr);
    airFree(part);
  }
  return NULL;
}

void
_limnObjectFaceEmpty(limnFace *face) {

  if (face) {
    airFree(face->vertIdxIdx);
    airFree(face->edgeIdxIdx);
  }
  return;
}

limnObject *
limnObjectNix(limnObject *obj) {
  int partIdx, faceIdx;

  for (partIdx=0; partIdx<obj->partNum; partIdx++) {
    _limnObjectPartNix(obj->part[partIdx]);
  }
  airArrayNuke(obj->partArr);
  for (faceIdx=0; faceIdx<obj->faceNum; faceIdx++) {
    _limnObjectFaceEmpty(obj->face + faceIdx);
  }
  airArrayNuke(obj->faceArr);

  airArrayNuke(obj->vertArr);
  airArrayNuke(obj->edgeArr);
  airFree(obj->faceSort);
  airArrayNuke(obj->lookArr);
  free(obj);
  return NULL;
}

int
limnObjectPartAdd(limnObject *obj) {
  int partIdx;
  limnPart *part;

  partIdx = airArrayIncrLen(obj->partArr, 1);
  part = obj->part[partIdx] = (limnPart*)calloc(1, sizeof(limnPart));
  
  part->vertIdx = NULL;
  part->edgeIdx = NULL;
  part->faceIdx = NULL;
  part->vertIdxArr = airArrayNew((void**)&(part->vertIdx), &(part->vertIdxNum),
                                 sizeof(int), obj->incr);
  part->edgeIdxArr = airArrayNew((void**)&(part->edgeIdx), &(part->edgeIdxNum),
                                 sizeof(int), obj->incr);
  part->faceIdxArr = airArrayNew((void**)&(part->faceIdx), &(part->faceIdxNum),
                                 sizeof(int), obj->incr);
  part->lookIdx = 0;  
  part->depth = AIR_NAN;
  
  return partIdx;
}

int
limnObjectVertexAdd(limnObject *obj, int partIdx, int lookIdx,
                    float x, float y, float z) {
  limnPart *part;
  limnVertex *vert;
  int vertIdx, vertIdxIdx;

  part = obj->part[partIdx];
  vertIdx = airArrayIncrLen(obj->vertArr, 1);
  vert = obj->vert + vertIdx;
  vertIdxIdx = airArrayIncrLen(part->vertIdxArr, 1);
  part->vertIdx[vertIdxIdx] = vertIdx;
  ELL_4V_SET(vert->world, x, y, z, 1);
  ELL_3V_SET(vert->view, AIR_NAN, AIR_NAN, AIR_NAN);
  ELL_3V_SET(vert->screen, AIR_NAN, AIR_NAN, AIR_NAN);
  ELL_3V_SET(vert->worldNormal, AIR_NAN, AIR_NAN, AIR_NAN);
  vert->device[0] = vert->device[1] = AIR_NAN;
  vert->partIdx = partIdx;
  vert->lookIdx = lookIdx;

  return vertIdxIdx;
}

int
limnObjectEdgeAdd(limnObject *obj, int partIdx, int lookIdx,
                  int faceIdxIdx, int vertIdxIdx0, int vertIdxIdx1) {
  int tmp, edgeIdx, edgeIdxIdx;
  limnEdge *edge=NULL;
  limnPart *part;
  
  part = obj->part[partIdx];
  if (vertIdxIdx0 > vertIdxIdx1) {
    ELL_SWAP2(vertIdxIdx0, vertIdxIdx1, tmp);
  }

  /* do a linear search through this part's existing edges */
  for (edgeIdxIdx=0; edgeIdxIdx<part->edgeIdxNum; edgeIdxIdx++) {
    edgeIdx = part->edgeIdx[edgeIdxIdx];
    edge = obj->edge + edgeIdx;
    if (edge->vertIdxIdx[0] == vertIdxIdx0
        && edge->vertIdxIdx[1] == vertIdxIdx1) {
      break;
    }
  }
  if (edgeIdxIdx == part->edgeIdxNum) {
    /* edge not found, add it */
    edgeIdx = airArrayIncrLen(obj->edgeArr, 1);
    edge = obj->edge + edgeIdx;
    edgeIdxIdx = airArrayIncrLen(part->edgeIdxArr, 1);
    part->edgeIdx[edgeIdxIdx] = edgeIdx;
    edge->vertIdxIdx[0] = vertIdxIdx0;
    edge->vertIdxIdx[1] = vertIdxIdx1;
    edge->faceIdxIdx[0] = faceIdxIdx;
    edge->faceIdxIdx[1] = -1;
    edge->lookIdx = lookIdx;
    edge->partIdx = partIdx;
    edge->type = limnEdgeTypeUnknown;
    edge->once = AIR_FALSE;
  } else {
    /* edge already exists; "edge", "edgeIdx", and "edgeIdxIdx" are all set */
    edge->faceIdxIdx[1] = faceIdxIdx;
  }

  return edgeIdxIdx;
}

int
limnObjectFaceAdd(limnObject *obj, int partIdx,
                  int lookIdx, int sideNum, int *vertIdxIdx) {
  limnFace *face;
  limnPart *part;
  int faceIdx, faceIdxIdx, sideIdx;

  part = obj->part[partIdx];
  faceIdx = airArrayIncrLen(obj->faceArr, 1);
  face = obj->face + faceIdx;
  faceIdxIdx = airArrayIncrLen(part->faceIdxArr, 1);
  part->faceIdx[faceIdxIdx] = faceIdx;
  
  face->vertIdxIdx = (int*)calloc(sideNum, sizeof(int));
  face->sideNum = sideNum;
  if (obj->doEdges) {
    face->edgeIdxIdx = (int*)calloc(sideNum, sizeof(int));
  }
  for (sideIdx=0; sideIdx<sideNum; sideIdx++) {
    face->vertIdxIdx[sideIdx] = vertIdxIdx[sideIdx];
    if (obj->doEdges) {
      face->edgeIdxIdx[sideIdx] = 
        limnObjectEdgeAdd(obj, partIdx, 0, faceIdxIdx,
                          vertIdxIdx[sideIdx],
                          vertIdxIdx[AIR_MOD(sideIdx+1, sideNum)]);
    }
  }
  ELL_3V_SET(face->worldNormal, AIR_NAN, AIR_NAN, AIR_NAN);
  ELL_3V_SET(face->screenNormal, AIR_NAN, AIR_NAN, AIR_NAN);
  face->lookIdx = lookIdx;
  face->partIdx = partIdx;
  face->visible = AIR_FALSE;
  face->depth = AIR_NAN;
  
  return faceIdx;
}

