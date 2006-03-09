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
    pld->xyzw = NULL;
    pld->xyzwNum = 0;
    pld->rgba = NULL;
    pld->rgbaNum = 0;
    pld->norm = NULL;
    pld->normNum = 0;
    pld->tex2D = NULL;
    pld->tex2DNum = 0;
    pld->indx = NULL;
    pld->indxNum = 0;
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
** doesn't set pld->xyzwNum, only the per-attribute xxxNum variables
*/
int
_limnPolyDataInfoAlloc(limnPolyData *pld, unsigned int infoBitFlag,
                       unsigned int vertNum) {
  char me[]="_limnPolyDataInfoAlloc", err[BIFF_STRLEN];
  
  if (vertNum != pld->rgbaNum
      && ((1 << limnPolyDataInfoRGBA) & infoBitFlag)) {
    pld->rgba = (unsigned char *)airFree(pld->rgba);
    if (vertNum) {
      pld->rgba = (unsigned char *)calloc(vertNum, 4*sizeof(unsigned char));
      if (!pld->rgba) {
        sprintf(err, "%s: couldn't allocate %u rgba", me, vertNum);
        biffAdd(LIMN, err); return 1;
      }
    }
    pld->rgbaNum = vertNum;
  }

  if (vertNum != pld->normNum
      && ((1 << limnPolyDataInfoNorm) & infoBitFlag)) {
    pld->norm = (float *)airFree(pld->norm);
    if (vertNum) {
      pld->norm = (float *)calloc(vertNum, 4*sizeof(float));
      if (!pld->norm) {
        sprintf(err, "%s: couldn't allocate %u norm", me, vertNum);
        biffAdd(LIMN, err); return 1;
      }
    }
    pld->normNum = vertNum;
  }

  if (vertNum != pld->tex2DNum
      && ((1 << limnPolyDataInfoTex2D) & infoBitFlag)) {
    pld->tex2D = (float *)airFree(pld->tex2D);
    if (vertNum) {
      pld->tex2D = (float *)calloc(vertNum, 4*sizeof(float));
      if (!pld->tex2D) {
        sprintf(err, "%s: couldn't allocate %u tex2D", me, vertNum);
        biffAdd(LIMN, err); return 1;
      }
    }
    pld->tex2DNum = vertNum;
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
  if (vertNum != pld->xyzwNum) {
    pld->xyzw = (float *)airFree(pld->xyzw);
    if (vertNum) {
      pld->xyzw = (float *)calloc(vertNum, 4*sizeof(float));
      if (!pld->xyzw) {
        sprintf(err, "%s: couldn't allocate %u xyzw", me, vertNum);
        biffAdd(LIMN, err); return 1;
      }
    }
    pld->xyzwNum = vertNum;
  }
  if (_limnPolyDataInfoAlloc(pld, infoBitFlag, vertNum)) {
    sprintf(err, "%s: couldn't allocate info", me);
    biffAdd(LIMN, err); return 1;
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
    ret += pld->xyzwNum*sizeof(float)*4;  /* xyzw */
    if (pld->rgba) {
      ret += pld->rgbaNum*sizeof(unsigned char)*4;
    }
    if (pld->norm) {
      ret += pld->normNum*sizeof(float)*3;
    }
    if (pld->tex2D) {
      ret += pld->tex2DNum*sizeof(float)*2;
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
                        pldA->xyzwNum, pldA->indxNum, pldA->primNum)) {
    sprintf(err, "%s: couldn't allocate output", me);
    biffAdd(LIMN, err); return 1;
  }
  memcpy(pldB->xyzw, pldA->xyzw, pldA->xyzwNum*sizeof(float)*4);
  if (pldA->rgba) {
    memcpy(pldB->rgba, pldA->rgba, pldA->rgbaNum*sizeof(unsigned char)*4);
  }
  if (pldA->norm) {
    memcpy(pldB->norm, pldA->norm, pldA->normNum*sizeof(float)*3);
  }
  if (pldA->tex2D) {
    memcpy(pldB->tex2D, pldA->tex2D, pldA->tex2DNum*sizeof(float)*2);
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
  if (limnPolyDataAlloc(pldB, num*pldA->xyzwNum,
                       num*pldA->indxNum, num*pldA->primNum)) {
    sprintf(err, "%s: couldn't allocate output", me);
    biffAdd(LIMN, err); return 1;
  }
  for (ii=0; ii<num; ii++) {
    memcpy(pldB->vert + ii*pldA->xyzwNum, pldA->vert,
           pldA->xyzwNum*sizeof(limnVrt));
    for (jj=0; jj<pldA->indxNum; jj++) {
      (pldB->indx + ii*pldA->indxNum)[jj] = pldA->indx[jj] + ii*pldA->xyzwNum;
    }
    memcpy(pldB->type + ii*pldA->primNum, pldA->type,
           pldA->primNum*sizeof(signed char));
    memcpy(pldB->icnt + ii*pldA->primNum, pldA->icnt,
           pldA->primNum*sizeof(unsigned int));
  }
  return 0;
}

#endif

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
    for (vertIdx=0; vertIdx<pld->xyzwNum; vertIdx++) {
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
    for (vertIdx=0; vertIdx<pld->xyzwNum; vertIdx++) {
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
  if (pld) {
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
  }
  return ret;
}

/*
** determines intersection of elements of srcA and srcB.
** assumes: 
** - there are no repeats in either list
** - dstC is allocated for at least as long as the longer of srcA and srcB
*/
static unsigned int
flipListIntx(unsigned int *dstC,
             const unsigned int *_srcA, const unsigned int *_srcB) {
  const unsigned int *srcA, *srcB;
  unsigned int numA, numB, numC, idxA, idxB;

  numA = _srcA[0];
  srcA = _srcA + 1;
  numB = _srcB[0];
  srcB = _srcB + 1;
  numC = 0;
  for (idxA=0; idxA<numA; idxA++) {
    for (idxB=0; idxB<numB; idxB++) {
      if (srcA[idxA] == srcB[idxB]) {
        dstC[numC++] = srcA[idxA];
      }
    }
  }
  return numC;
}

static void
flipNeighborsGet(limnPolyData *pld, Nrrd *nTriWithVert, Nrrd *nVertWithTri,
                 unsigned int neighGot[3], unsigned int neighInfo[3][3],
                 unsigned int *intxBuff, unsigned int totalTriIdx) {
  unsigned int intxNum, vertA, vertB, neighIdx, maxTriPerVert,
    *vertWithTri, *triWithVert;
  int ii, iip;

  AIR_UNUSED(pld);
  vertWithTri = AIR_CAST(unsigned int*, nVertWithTri->data);
  triWithVert = AIR_CAST(unsigned int*, nTriWithVert->data);
  maxTriPerVert = nTriWithVert->axis[0].size - 1;
  for (ii=0; ii<3; ii++) {
    iip = AIR_MOD(ii+1, 3);
    vertA = (vertWithTri + 3*totalTriIdx)[ii];
    vertB = (vertWithTri + 3*totalTriIdx)[iip];
    intxNum = flipListIntx(intxBuff,
                           triWithVert + (1+maxTriPerVert)*vertA,
                           triWithVert + (1+maxTriPerVert)*vertB);
    if (2 == intxNum) {
      neighIdx = intxBuff[0];
      if (neighIdx == totalTriIdx) {
        neighIdx = intxBuff[1];
      }
      neighGot[ii] = AIR_TRUE;
      neighInfo[ii][0] = neighIdx;
      neighInfo[ii][1] = vertA;
      neighInfo[ii][2] = vertB;
    } else {
      neighGot[ii] = AIR_FALSE;
    }
  }
  return;
}

static int
flipNeed(limnPolyData *pld, Nrrd *nTriWithVert, Nrrd *nVertWithTri,
         unsigned int triIdx, unsigned int vertA, unsigned int vertB) {
  unsigned int *vertWithTri, vert[3];
  int ai, bi;

  AIR_UNUSED(pld);
  AIR_UNUSED(nTriWithVert);
  vertWithTri = AIR_CAST(unsigned int*, nVertWithTri->data);
  ELL_3V_COPY(vert, vertWithTri + 3*triIdx);
  for (ai=0; vert[ai] != vertA; ai++)
    ;
  for (bi=0; vert[bi] != vertB; bi++)
    ;
  return (1 != AIR_MOD(ai - bi, 3));
}

static unsigned int
flipNeighborsPush(limnPolyData *pld, Nrrd *nTriWithVert, Nrrd *nVertWithTri,
                  unsigned char *triDone, airArray *todoArr,
                  unsigned int *buff, unsigned int totalTriIdx) {
  unsigned int neighGot[3], neighInfo[3][3], ii, *todo, todoIdx,
    *vertWithTri, doneIncr;

  vertWithTri = AIR_CAST(unsigned int*, nVertWithTri->data);
  flipNeighborsGet(pld, nTriWithVert, nVertWithTri,
                   neighGot, neighInfo,
                   buff, totalTriIdx);
  doneIncr = 0;
  for (ii=0; ii<2; ii++) {
    if (neighGot[ii] && !triDone[neighInfo[ii][0]]) {
      unsigned int tmp, *idxLine;
      if (flipNeed(pld, nTriWithVert, nVertWithTri,
                   neighInfo[ii][0], neighInfo[ii][1], neighInfo[ii][2])) {
        idxLine = vertWithTri + 3*neighInfo[ii][0];
        tmp = idxLine[0];
        idxLine[0] = idxLine[1];
        idxLine[1] = tmp;
      }
      triDone[neighInfo[ii][0]] = AIR_TRUE;
      todoIdx = airArrayLenIncr(todoArr, 1);
      todo = AIR_CAST(unsigned int*, todoArr->data);
      todo[todoIdx] = neighInfo[ii][0];
      ++doneIncr;
    }
  }
  return doneIncr;
}

int
limnPolyDataVertexWindingFix(limnPolyData *pld) { 
  char me[]="limnPolyDataVertexWindingFix", err[BIFF_STRLEN];
  unsigned int
    primIdx,         /* for indexing through primitives */
    triIdx,          /* for indexing through triangles in each primitive */
    vertIdx,         /* for indexing through vertex indices */
    maxTriPerPrim,   /* max # triangles per primitive, which is essential for
                        the indexing of each triangle (in each primitive)
                        into a single triangle index */
    totalTriNum,     /* total # triangles in all primitives (actually not,
                        just the total number of plausible triangle indices) */
    totalTriIdx,     /* master triangle index */
    trueTotalTriNum, /* correct total # triangles in all primitives */
    baseVertIdx,     /* first vertex for current primitive */
    *triWithVertNum, /* 1D array (len totalTriNum) of # tri with vert[i] */
    maxTriPerVert,   /* max # of tris on single vertex */
    *triWithVert,    /* 2D array ((1+maxTriPerVert) x pld->xyzwNum) 
                        of per-vertex triangles */
    *vertWithTri,    /* 3D array (3 x maxTriPerPrim x pld->primNum)
                        of per-tri vertices (vertex indices), which is just
                        a repackaging of the information in the lpld */
    doneTriNum,      /* # triangles finished so far */
    *buff,           /* stupid buffer */
    *todo;           /* the to-do stack */
  unsigned char
    *triDone;        /* 1D array (len totalTriNum) record of done-ness */
  Nrrd *nTriWithVert, *nVertWithTri;
  airArray *mop, *todoArr;
  
  if (!pld) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(LIMN, err); return 1;
  }

  if (!(pld->xyzwNum && pld->primNum)) {
    /* this is empty? */
    return 0;
  }

  /* calculate maxTriPerPrim and totalTriNum */
  maxTriPerPrim = 0;
  trueTotalTriNum = 0;
  for (primIdx=0; primIdx<pld->primNum; primIdx++) {
    unsigned int triNum;
    if (limnPrimitiveTriangles != pld->type[primIdx]) {
      sprintf(err, "%s: sorry, can only have %s prims (prim[%u] is %s)",
              me, airEnumStr(limnPrimitive, limnPrimitiveTriangles),
              primIdx, airEnumStr(limnPrimitive, pld->type[primIdx]));
      biffAdd(LIMN, err); return 1;
    }
    triNum = pld->icnt[primIdx]/3;
    maxTriPerPrim = AIR_MAX(maxTriPerPrim, triNum);
    trueTotalTriNum += triNum;
  }
  totalTriNum = maxTriPerPrim*pld->primNum;

  /* allocate 1-D triWithVertNum and triDone */
  mop = airMopNew();
  triWithVertNum = AIR_CAST(unsigned int*,
                            calloc(pld->xyzwNum, sizeof(unsigned int)));
  airMopAdd(mop, triWithVertNum, airFree, airMopAlways);
  triDone = AIR_CAST(unsigned char *,
                     calloc(totalTriNum, sizeof(unsigned char)));
  airMopAdd(mop, triDone, airFree, airMopAlways);
  if (!(triWithVertNum && triDone)) {
    sprintf(err, "%s: couldn't allocate temp arrays", me);
    biffAdd(LIMN, err); airMopError(mop); return 1;
  }

  /* fill in triWithVertNum */
  baseVertIdx = 0;
  for (primIdx=0; primIdx<pld->primNum; primIdx++) {
    unsigned int triNum, *indxLine, ii;
    triNum = pld->icnt[primIdx]/3;
    for (triIdx=0; triIdx<triNum; triIdx++) {
      indxLine = pld->indx + baseVertIdx + 3*triIdx;
      for (ii=0; ii<3; ii++) {
        triWithVertNum[indxLine[ii]]++;
      }
    }
    baseVertIdx += 3*triNum;
  }

  /* find (1 less than) axis-0 size of TriWithVert from triWithVertNum */
  maxTriPerVert = 0;
  for (vertIdx=0; vertIdx<pld->xyzwNum; vertIdx++) {
    maxTriPerVert = AIR_MAX(maxTriPerVert, triWithVertNum[vertIdx]);
  }

  /* allocate TriWithVert, VertWithTri, buff */
  nTriWithVert = nrrdNew();
  airMopAdd(mop, nTriWithVert, (airMopper)nrrdNuke, airMopAlways);
  nVertWithTri = nrrdNew();
  airMopAdd(mop, nVertWithTri, (airMopper)nrrdNuke, airMopAlways);
  if (nrrdMaybeAlloc_va(nTriWithVert, nrrdTypeUInt, 2, 
                        AIR_CAST(size_t, 1+maxTriPerVert),
                        AIR_CAST(size_t, pld->xyzwNum))
      || nrrdMaybeAlloc_va(nVertWithTri, nrrdTypeUInt, 3, 
                           AIR_CAST(size_t, 3),
                           AIR_CAST(size_t, maxTriPerPrim),
                           AIR_CAST(size_t, pld->primNum))) {
    sprintf(err, "%s: couldn't allocate TriWithVert or VertWithTri", me);
    biffMove(LIMN, err, NRRD); airMopError(mop); return 1;
  }
  triWithVert = AIR_CAST(unsigned int*, nTriWithVert->data);
  vertWithTri = AIR_CAST(unsigned int*, nVertWithTri->data);
  buff = AIR_CAST(unsigned int*, calloc(maxTriPerVert, sizeof(unsigned int)));
  if (!buff) {
    sprintf(err, "%s: failed to alloc an itty bitty buffer", me);
    biffAdd(LIMN, err); airMopError(mop); return 1;
  }
  airMopAdd(mop, buff, airFree, airMopAlways);

  /* fill in TriWithVert and VertWithTri */
  baseVertIdx = 0;
  for (primIdx=0; primIdx<pld->primNum; primIdx++) {
    unsigned int triNum, *indxLine, *twvLine, ii;
    triNum = pld->icnt[primIdx]/3;
    for (triIdx=0; triIdx<triNum; triIdx++) {
      totalTriIdx = triIdx + maxTriPerPrim*primIdx;
      indxLine = pld->indx + baseVertIdx + 3*triIdx;
      for (ii=0; ii<3; ii++) {
        (vertWithTri + 3*totalTriIdx)[ii] = indxLine[ii];
        twvLine = triWithVert + (1+maxTriPerVert)*indxLine[ii];
        twvLine[1+twvLine[0]] = totalTriIdx;
        twvLine[0]++;
      }
    }
    baseVertIdx += 3*triNum;
  }

  /* initialize the triDone array so that we can quickly scan it 
     for triangles left undone.  This is needed because of the way
     that triangles are given a linear index- there may be linear
     indices that do not correspond to a triangle */
  for (totalTriIdx=0; totalTriIdx<totalTriNum; totalTriIdx++) {
    triDone[totalTriIdx] = AIR_TRUE;
  }
  for (primIdx=0; primIdx<pld->primNum; primIdx++) {
    unsigned int triNum;
    triNum = pld->icnt[primIdx]/3;
    for (triIdx=0; triIdx<triNum; triIdx++) {
      triDone[triIdx + maxTriPerPrim*primIdx] = AIR_FALSE;
    }
  }

  /* create the stack of recently fixed triangles */
  todoArr = airArrayNew((void**)(&todo), NULL, sizeof(unsigned int),
                        maxTriPerPrim);
  airMopAdd(mop, todoArr, (airMopper)airArrayNuke, airMopAlways);

  /* the skinny */
  doneTriNum = 0;
  while (doneTriNum < trueTotalTriNum) {
    /* find first undone triangle, which should be on a different
       connected component than any processed so far */
    for (totalTriIdx=0; triDone[totalTriIdx]; totalTriIdx++)
      ;
    triDone[totalTriIdx] = AIR_TRUE;
    ++doneTriNum;
    doneTriNum += flipNeighborsPush(pld, nTriWithVert, nVertWithTri,
                                    triDone, todoArr, 
                                    buff, totalTriIdx);
    while (todoArr->len) {
      unsigned int popped;
      popped = todo[todoArr->len-1];
      airArrayLenIncr(todoArr, -1);
      doneTriNum += flipNeighborsPush(pld, nTriWithVert, nVertWithTri,
                                      triDone, todoArr,
                                      buff, popped);
    }
  }

  /* Copy from nVertWithTri back into polydata */
  baseVertIdx = 0;
  for (primIdx=0; primIdx<pld->primNum; primIdx++) {
    unsigned int triNum, *indxLine, ii;
    triNum = pld->icnt[primIdx]/3;
    for (triIdx=0; triIdx<triNum; triIdx++) {
      totalTriIdx = triIdx + maxTriPerPrim*primIdx;
      indxLine = pld->indx + baseVertIdx + 3*triIdx;
      for (ii=0; ii<3; ii++) {
        indxLine[ii] = (vertWithTri + 3*totalTriIdx)[ii];
      }
    }
    baseVertIdx += 3*triNum;
  }

  airMopOkay(mop);
  return 0;
}

/*
int
limnPolyDataVertexNormals(limnPolyData *pld) { 
  char me[]="limnPolyDataVertexNormals", err[BIFF_STRLEN];

  if (limnPolyDataAlloc(pld,
                        
                                  unsigned int infoBitFlag,
                                  unsigned int vertNum,
                                  unsigned int indxNum,
                                  unsigned int primNum);

  return 0;
}
*/

