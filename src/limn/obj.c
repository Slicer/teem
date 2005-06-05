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

  lookIdx = airArrayLenIncr(obj->lookArr, 1);
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

  obj->vertSpace = limnSpaceUnknown;
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
    airFree(face->vertIdx);
    airFree(face->edgeIdx);
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

  partIdx = airArrayLenIncr(obj->partArr, 1);
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
limnObjectVertexNumPreSet(limnObject *obj, int partIdx, int vertNum) {
  limnPart *part;  

  part = obj->part[partIdx];
  airArrayLenPreSet(obj->vertArr, vertNum);
  airArrayLenPreSet(part->vertIdxArr, vertNum);
  return 0;
}

int
limnObjectVertexAdd(limnObject *obj, int partIdx,
                    float x, float y, float z) {
  limnPart *part;
  limnVertex *vert;
  int vertIdx, vertIdxIdx;

  part = obj->part[partIdx];
  vertIdx = airArrayLenIncr(obj->vertArr, 1);
  vert = obj->vert + vertIdx;
  vertIdxIdx = airArrayLenIncr(part->vertIdxArr, 1);
  part->vertIdx[vertIdxIdx] = vertIdx;
  ELL_4V_SET(vert->world, x, y, z, 1);
  ELL_4V_SET(vert->coord, AIR_NAN, AIR_NAN, AIR_NAN, AIR_NAN);
  ELL_4V_SET(vert->rgba, 1, 1, 1, 1);
  /* ELL_3V_SET(vert->view, AIR_NAN, AIR_NAN, AIR_NAN); */
  /* ELL_3V_SET(vert->screen, AIR_NAN, AIR_NAN, AIR_NAN); */
  ELL_4V_SET(vert->worldNormal, AIR_NAN, AIR_NAN, AIR_NAN, AIR_NAN);

  return vertIdx;
}

int
limnObjectEdgeAdd(limnObject *obj, int partIdx, int lookIdx,
                  int faceIdx, int vertIdx0, int vertIdx1) {
  int tmp, edgeIdx=-42, edgeIdxIdx;
  limnEdge *edge=NULL;
  limnPart *part;
  
  part = obj->part[partIdx];
  if (vertIdx0 > vertIdx1) {
    ELL_SWAP2(vertIdx0, vertIdx1, tmp);
  }

  /* do a linear search through this part's existing edges */
  for (edgeIdxIdx=0; edgeIdxIdx<part->edgeIdxNum; edgeIdxIdx++) {
    edgeIdx = part->edgeIdx[edgeIdxIdx];
    edge = obj->edge + edgeIdx;
    if (edge->vertIdx[0] == vertIdx0
        && edge->vertIdx[1] == vertIdx1) {
      break;
    }
  }
  if (edgeIdxIdx == part->edgeIdxNum) {
    /* edge not found, add it */
    edgeIdx = airArrayLenIncr(obj->edgeArr, 1);
    edge = obj->edge + edgeIdx;
    edgeIdxIdx = airArrayLenIncr(part->edgeIdxArr, 1);
    part->edgeIdx[edgeIdxIdx] = edgeIdx;
    edge->vertIdx[0] = vertIdx0;
    edge->vertIdx[1] = vertIdx1;
    edge->faceIdx[0] = faceIdx;
    edge->faceIdx[1] = -1;
    edge->lookIdx = lookIdx;
    edge->partIdx = partIdx;
    edge->type = limnEdgeTypeUnknown;
    edge->once = AIR_FALSE;
  } else {
    /* edge already exists; "edge", "edgeIdx", and "edgeIdxIdx" are all set */
    edge->faceIdx[1] = faceIdx;
  }

  return edgeIdx;
}

int
limnObjectFaceNumPreSet(limnObject *obj, int partIdx, int faceNum) {
  limnPart *part;  

  part = obj->part[partIdx];
  airArrayLenPreSet(obj->faceArr, faceNum);
  airArrayLenPreSet(part->faceIdxArr, faceNum);
  return 0;
}

int
limnObjectFaceAdd(limnObject *obj, int partIdx,
                  int lookIdx, int sideNum, int *vertIdx) {
  limnFace *face;
  limnPart *part;
  int faceIdx, faceIdxIdx, sideIdx;

  part = obj->part[partIdx];
  faceIdx = airArrayLenIncr(obj->faceArr, 1);
  face = obj->face + faceIdx;
  faceIdxIdx = airArrayLenIncr(part->faceIdxArr, 1);
  part->faceIdx[faceIdxIdx] = faceIdx;
  
  face->vertIdx = (int*)calloc(sideNum, sizeof(int));
  face->sideNum = sideNum;
  if (obj->doEdges) {
    face->edgeIdx = (int*)calloc(sideNum, sizeof(int));
  }
  for (sideIdx=0; sideIdx<sideNum; sideIdx++) {
    face->vertIdx[sideIdx] = vertIdx[sideIdx];
    if (obj->doEdges) {
      face->edgeIdx[sideIdx] = 
        limnObjectEdgeAdd(obj, partIdx, 0, faceIdx,
                          vertIdx[sideIdx],
                          vertIdx[AIR_MOD(sideIdx+1, sideNum)]);
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

