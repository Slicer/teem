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
  /* char me[]="flipNeighborsGet"; */
  unsigned int intxNum, vertA, vertB, neighIdx, maxTriPerVert,
    *vertWithTri, *triWithVert;
  int ii;

  AIR_UNUSED(pld);
  vertWithTri = AIR_CAST(unsigned int*, nVertWithTri->data);
  triWithVert = AIR_CAST(unsigned int*, nTriWithVert->data);
  maxTriPerVert = nTriWithVert->axis[0].size - 1;
  for (ii=0; ii<3; ii++) {
    vertA = (vertWithTri + 3*totalTriIdx)[ii];
    vertB = (vertWithTri + 3*totalTriIdx)[AIR_MOD(ii+1, 3)];
    /*
    fprintf(stderr, "!%s: %u edge %u: vert{A,B} = %u %u\n", me,
            totalTriIdx, ii, vertA, vertB);
    */
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
      neighInfo[ii][1] = vertB;
      neighInfo[ii][2] = vertA;
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
  return (1 != AIR_MOD(bi - ai, 3));
}

static unsigned int
flipNeighborsPush(limnPolyData *pld, Nrrd *nTriWithVert, Nrrd *nVertWithTri,
                  unsigned char *triDone, airArray *todoArr,
                  unsigned int *buff, unsigned int totalTriIdx) {
  /* char me[]="flipNeighborsPush"; */
  unsigned int neighGot[3], neighInfo[3][3], ii, *todo, todoIdx,
    *vertWithTri, doneIncr;

  vertWithTri = AIR_CAST(unsigned int*, nVertWithTri->data);
  flipNeighborsGet(pld, nTriWithVert, nVertWithTri,
                   neighGot, neighInfo,
                   buff, totalTriIdx);
  /*
  for (ii=0; ii<3; ii++) {
    fprintf(stderr, "!%s: %u neigh[%u]: ", me, totalTriIdx, ii);
    if (neighGot[ii]) {
      fprintf(stderr, "%u (%u %u) (done %u)\n",
              neighInfo[ii][0], neighInfo[ii][1], neighInfo[ii][2],
              triDone[neighInfo[ii][0]]);
    } else {
      fprintf(stderr, "nope\n");
    }
  }
  */
  doneIncr = 0;
  for (ii=0; ii<3; ii++) {
    if (neighGot[ii] && !triDone[neighInfo[ii][0]]) {
      unsigned int tmp, *idxLine, need;
      need = flipNeed(pld, nTriWithVert, nVertWithTri,
                      neighInfo[ii][0], neighInfo[ii][1], neighInfo[ii][2]);
      /*
      fprintf(stderr, "!%s: need(%u,%u,%u) = %u\n",
              "flipNeighborsPush",
              neighInfo[ii][0], neighInfo[ii][1], neighInfo[ii][2], need);
      */
      if (need) {
        /*
        fprintf(stderr, "!%s: flipping %u\n", me, neighInfo[ii][0]);
        */
        idxLine = vertWithTri + 3*neighInfo[ii][0];
        tmp = idxLine[0];
        idxLine[0] = idxLine[1];
        idxLine[1] = tmp;
      }
      triDone[neighInfo[ii][0]] = AIR_TRUE;
      todoIdx = airArrayLenIncr(todoArr, 1);
      todo = AIR_CAST(unsigned int*, todoArr->data);
      todo[todoIdx] = neighInfo[ii][0];
      /*
      fprintf(stderr, "!%s: pushed %u\n", me, neighInfo[ii][0]);
      */
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
  /*
  fprintf(stderr, "!%s: hi\n", me);
  */
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
      || nrrdMaybeAlloc_va(nVertWithTri, nrrdTypeUInt, 2, 
                           AIR_CAST(size_t, 3),
                           AIR_CAST(size_t, maxTriPerPrim*pld->primNum))) {
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

  /*
  nrrdSave("triWithVert.nrrd", nTriWithVert, NULL);
  nrrdSave("vertWithTri.nrrd", nVertWithTri, NULL);
  */

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
    /*
    fprintf(stderr, "!%s: considering tri %u done (%u)\n",
            me, totalTriIdx, doneTriNum);
    */
    doneTriNum += flipNeighborsPush(pld, nTriWithVert, nVertWithTri,
                                    triDone, todoArr, 
                                    buff, totalTriIdx);
    while (todoArr->len) {
      unsigned int popped;
      popped = todo[todoArr->len-1];
      airArrayLenIncr(todoArr, -1);
      /*
      fprintf(stderr, "!%s: popped %u\n", me, popped);
      */
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
  /*  
  fprintf(stderr, "!%s: bye\n", me);
  */
  airMopOkay(mop);
  return 0;
}

