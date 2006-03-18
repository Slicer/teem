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

static unsigned int _limnCC_EqvIncr = 1024;

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

/*
** ONLY GOOD FOR limnPrimitiveTriangles!!
*/
static unsigned int
maxTrianglePerPrimitive(limnPolyData *pld) {
  unsigned int ret, primIdx;

  ret = 0;
  for (primIdx=0; primIdx<pld->primNum; primIdx++) {
    ret = AIR_MAX(ret, pld->icnt[primIdx]/3);
  }
  return ret;
}

static int
triangleWithVertex(Nrrd *nTriWithVert, limnPolyData *pld, int useUniform) { 
  char me[]="triangleWithVertex", err[BIFF_STRLEN];
  unsigned int *triWithVertNum, *triWithVert, baseVertIdx, primIdx, vertIdx, 
    maxTriPerVert, maxTriPerPrim, theTriIdx;
  airArray *mop;

  if (!(nTriWithVert && pld)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(LIMN, err); return 1;
  }
  if ((1 << limnPrimitiveTriangles) != limnPolyDataPrimitiveTypes(pld)) {
    sprintf(err, "%s: sorry, can only handle %s primitives", me,
            airEnumStr(limnPrimitive, limnPrimitiveTriangles));
    biffAdd(LIMN, err); return 1;
  }

  triWithVertNum = AIR_CAST(unsigned int*,
                            calloc(pld->xyzwNum, sizeof(unsigned int)));
  if (!triWithVertNum) {
    sprintf(err, "%s: couldn't allocate temp array", me);
    biffAdd(LIMN, err); return 1;
  }
  mop = airMopNew();
  airMopAdd(mop, triWithVertNum, airFree, airMopAlways);

  /* fill in triWithVertNum */
  baseVertIdx = 0;
  for (primIdx=0; primIdx<pld->primNum; primIdx++) {
    unsigned int triNum, triIdx, *indxLine, ii;
    triNum = pld->icnt[primIdx]/3;
    for (triIdx=0; triIdx<triNum; triIdx++) {
      indxLine = pld->indx + baseVertIdx + 3*triIdx;
      for (ii=0; ii<3; ii++) {
        triWithVertNum[indxLine[ii]]++;
      }
    }
    baseVertIdx += pld->icnt[primIdx];
  }

  maxTriPerVert = 0;
  for (vertIdx=0; vertIdx<pld->xyzwNum; vertIdx++) {
    maxTriPerVert = AIR_MAX(maxTriPerVert, triWithVertNum[vertIdx]);
  }
  if (nrrdMaybeAlloc_va(nTriWithVert, nrrdTypeUInt, 2, 
                        AIR_CAST(size_t, 1 + maxTriPerVert),
                        AIR_CAST(size_t, pld->xyzwNum))) {
    sprintf(err, "%s: couldn't allocate output", me);
    biffMove(LIMN, err, NRRD); airMopError(mop); return 1;
  }
  triWithVert = AIR_CAST(unsigned int*, nTriWithVert->data);

  maxTriPerPrim = maxTrianglePerPrimitive(pld);
  baseVertIdx = 0;
  if (!useUniform) {
    theTriIdx = 0;
  }
  for (primIdx=0; primIdx<pld->primNum; primIdx++) {
    unsigned int triNum, *indxLine, *twvLine, ii, triIdx;
    triNum = pld->icnt[primIdx]/3;
    for (triIdx=0; triIdx<triNum; triIdx++) {
      if (useUniform) {
        theTriIdx = triIdx + maxTriPerPrim*primIdx;
      }
      indxLine = pld->indx + baseVertIdx + 3*triIdx;
      for (ii=0; ii<3; ii++) {
        twvLine = triWithVert + (1+maxTriPerVert)*indxLine[ii];
        twvLine[1+twvLine[0]] = theTriIdx;
        twvLine[0]++;
      }
      if (!useUniform) {
        ++theTriIdx;
      }
    }
    baseVertIdx += pld->icnt[primIdx];
  }

  airMopOkay(mop);
  return 0;
}

/*
** this ALWAYS uses the uniform triangle indexing scheme
*/
static int
vertexWithTriangle(Nrrd *nVertWithTri, limnPolyData *pld) { 
  char me[]="vertexWithTriangle", err[BIFF_STRLEN];
  unsigned int maxTriPerPrim, baseVertIdx, primIdx, *vertWithTri, uniTriNum;

  if (!(nVertWithTri && pld)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(LIMN, err); return 1;
  }
  if ((1 << limnPrimitiveTriangles) != limnPolyDataPrimitiveTypes(pld)) {
    sprintf(err, "%s: sorry, can only handle %s primitives", me,
            airEnumStr(limnPrimitive, limnPrimitiveTriangles));
    biffAdd(LIMN, err); return 1;
  }

  maxTriPerPrim = maxTrianglePerPrimitive(pld);
  uniTriNum = maxTriPerPrim*pld->primNum;
  fprintf(stderr, "%s: %u * %u = %u\n", me, maxTriPerPrim, pld->primNum,
          uniTriNum);
  if (nrrdMaybeAlloc_va(nVertWithTri, nrrdTypeUInt, 2, 
                        AIR_CAST(size_t, 3),
                        AIR_CAST(size_t, uniTriNum))) {
    sprintf(err, "%s: couldn't allocate output", me);
    biffMove(LIMN, err, NRRD); return 1;
  }
  vertWithTri = AIR_CAST(unsigned int*, nVertWithTri->data);
  
  baseVertIdx = 0;
  for (primIdx=0; primIdx<pld->primNum; primIdx++) {
    unsigned int triNum, triIdx, *indxLine, uniTriIdx, ii;
    triNum = pld->icnt[primIdx]/3;
    for (triIdx=0; triIdx<triNum; triIdx++) {
      uniTriIdx = triIdx + maxTriPerPrim*primIdx;
      indxLine = pld->indx + baseVertIdx + 3*triIdx;
      for (ii=0; ii<3; ii++) {
        (vertWithTri + 3*uniTriIdx)[ii] = indxLine[ii];
      }
    }
    baseVertIdx += pld->icnt[primIdx];
  }
  
  return 0;
}

int
limnPolyDataVertexWindingFix(limnPolyData *pld) { 
  char me[]="limnPolyDataVertexWindingFix", err[BIFF_STRLEN];
  unsigned int
    primIdx,         /* for indexing through primitives */
    triIdx,          /* for indexing through triangles in each primitive */
    maxTriPerPrim,   /* max # triangles per primitive, which is essential for
                        the indexing of each triangle (in each primitive)
                        into a single triangle index */
    uniTriIdx,       /* uniform triangle index */
    uniTriNum,       /* total # uniform triangle indices */
    trueTriNum,      /* correct total # triangles in all primitives */
    baseVertIdx,     /* first vertex for current primitive */
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
    *triDone;        /* 1D array (len uniTriNum) record of done-ness */
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

  if ((1 << limnPrimitiveTriangles) != limnPolyDataPrimitiveTypes(pld)) {
    sprintf(err, "%s: sorry, can only handle %s primitives", me,
            airEnumStr(limnPrimitive, limnPrimitiveTriangles));
    biffAdd(LIMN, err); return 1;
  }

  maxTriPerPrim = maxTrianglePerPrimitive(pld);
  uniTriNum = maxTriPerPrim*pld->primNum;

  mop = airMopNew();
  triDone = AIR_CAST(unsigned char *, calloc(uniTriNum,
                                             sizeof(unsigned char)));
  airMopAdd(mop, triDone, airFree, airMopAlways);
  if (!triDone) {
    sprintf(err, "%s: couldn't allocate temp array", me);
    biffAdd(LIMN, err); airMopError(mop); return 1;
  }

  /* allocate TriWithVert, VertWithTri, buff */
  nTriWithVert = nrrdNew();
  airMopAdd(mop, nTriWithVert, (airMopper)nrrdNuke, airMopAlways);
  nVertWithTri = nrrdNew();
  airMopAdd(mop, nVertWithTri, (airMopper)nrrdNuke, airMopAlways);
  if (triangleWithVertex(nTriWithVert, pld, AIR_TRUE)
      || vertexWithTriangle(nVertWithTri, pld)) {
    sprintf(err, "%s: couldn't set nTriWithVert or nVertWithTri", me);
    biffAdd(LIMN, err); airMopError(mop); return 1;
  }
  vertWithTri = AIR_CAST(unsigned int*, nVertWithTri->data);
  triWithVert = AIR_CAST(unsigned int*, nTriWithVert->data);

  maxTriPerVert = nTriWithVert->axis[0].size - 1;
  buff = AIR_CAST(unsigned int*, calloc(maxTriPerVert, sizeof(unsigned int)));
  if (!buff) {
    sprintf(err, "%s: failed to alloc an itty bitty buffer", me);
    biffAdd(LIMN, err); airMopError(mop); return 1;
  }
  airMopAdd(mop, buff, airFree, airMopAlways);

  /*
  nrrdSave("triWithVert.nrrd", nTriWithVert, NULL);
  nrrdSave("vertWithTri.nrrd", nVertWithTri, NULL);
  */

  /* initialize the triDone array so that we can quickly scan it 
     for triangles left undone.  This is needed because of the way
     that triangles are given a uniform linear index- there may be
     indices that do not correspond to a triangle */
  for (uniTriIdx=0; uniTriIdx<uniTriNum; uniTriIdx++) {
    triDone[uniTriIdx] = AIR_TRUE;
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
  trueTriNum = 0;
  for (primIdx=0; primIdx<pld->primNum; primIdx++) {
    trueTriNum += pld->icnt[primIdx]/3;
  }
  while (doneTriNum < trueTriNum) {
    /* find first undone triangle, which should be on a different
       connected component than any processed so far */
    for (uniTriIdx=0; triDone[uniTriIdx]; uniTriIdx++)
      ;
    triDone[uniTriIdx] = AIR_TRUE;
    ++doneTriNum;
    /*
    fprintf(stderr, "!%s: considering tri %u done (%u)\n",
            me, uniTriIdx, doneTriNum);
    */
    doneTriNum += flipNeighborsPush(pld, nTriWithVert, nVertWithTri,
                                    triDone, todoArr, 
                                    buff, uniTriIdx);
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
      uniTriIdx = triIdx + maxTriPerPrim*primIdx;
      indxLine = pld->indx + baseVertIdx + 3*triIdx;
      for (ii=0; ii<3; ii++) {
        indxLine[ii] = (vertWithTri + 3*uniTriIdx)[ii];
      }
    }
    baseVertIdx += pld->icnt[primIdx];
  }
  /*  
  fprintf(stderr, "!%s: bye\n", me);
  */
  airMopOkay(mop);
  return 0;
}

int
limnPolyDataCCFind(limnPolyData *pld) { 
  char me[]="limnPolyDataCCFind", err[BIFF_STRLEN];
  unsigned int realTriNum, *triMap, *triWithVert, vertIdx, *ccSize,
    *indxOld, *indxNew, primNumOld, *icntOld, *icntNew, *baseIndx,
    primIdxNew, primNumNew, passIdx, eqvNum;
  unsigned char *typeOld, *typeNew;
  Nrrd *nTriWithVert, *nccSize, *nTriMap;
  airArray *mop, *eqvArr;
  
  if (!pld) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(LIMN, err); return 1;
  }
  if (!(pld->xyzwNum && pld->primNum)) {
    /* this is empty? */
    return 0;
  }

  if ((1 << limnPrimitiveTriangles) != limnPolyDataPrimitiveTypes(pld)) {
    sprintf(err, "%s: sorry, can only handle %s primitives", me,
            airEnumStr(limnPrimitive, limnPrimitiveTriangles));
    biffAdd(LIMN, err); return 1;
  }

  mop = airMopNew();
  
  realTriNum = limnPolyDataPolygonNumber(pld);

  eqvArr = airArrayNew(NULL, NULL, 2*sizeof(unsigned int), _limnCC_EqvIncr);
  airMopAdd(mop, eqvArr, (airMopper)airArrayNuke, airMopAlways);

  nTriWithVert = nrrdNew();
  airMopAdd(mop, nTriWithVert, (airMopper)nrrdNuke, airMopAlways);
  if (triangleWithVertex(nTriWithVert, pld, AIR_FALSE)) {
    sprintf(err, "%s: couldn't set nTriWithVert", me);
    biffAdd(LIMN, err); airMopError(mop); return 1;
  }

  /* simple profiling showed that stupid amount of time was spent
     adding the equivalences.  So we go in two passes- first two see
     how many equivalences are needed, and then actually adding them */
  triWithVert = AIR_CAST(unsigned int*, nTriWithVert->data);
  for (passIdx=0; passIdx<2; passIdx++) {
    if (0 == passIdx) {
      eqvNum = 0;
    } else {
      airArrayLenPreSet(eqvArr, eqvNum);
    }
    for (vertIdx=0; vertIdx<nTriWithVert->axis[1].size; vertIdx++) {
      unsigned int *triLine, triIdx;
      triLine = triWithVert + vertIdx*(nTriWithVert->axis[0].size);
      for (triIdx=1; triIdx<triLine[0]; triIdx++) {
        if (0 == passIdx) {
          ++eqvNum;
        } else {
          airEqvAdd(eqvArr, triLine[1], triLine[1+triIdx]);
        }
      }
    }
  }

  nTriMap = nrrdNew();
  airMopAdd(mop, nTriMap, (airMopper)nrrdNuke, airMopAlways);
  nccSize = nrrdNew();
  airMopAdd(mop, nccSize, (airMopper)nrrdNuke, airMopAlways);
  if (nrrdMaybeAlloc_va(nTriMap, nrrdTypeUInt, 1,
                        AIR_CAST(size_t, realTriNum))) {
    sprintf(err, "%s: couldn't allocate equivalence map", me);
    biffMove(LIMN, err, NRRD); airMopError(mop); return 1;
  }
  triMap = AIR_CAST(unsigned int*, nTriMap->data);
  primNumNew = 1 + airEqvMap(eqvArr, triMap, realTriNum);
  if (nrrdHisto(nccSize, nTriMap, NULL, NULL, primNumNew, nrrdTypeUInt)) {
    sprintf(err, "%s: couldn't histogram CC map", me);
    biffMove(LIMN, err, NRRD); airMopError(mop); return 1;
  }
  ccSize = AIR_CAST(unsigned int*, nccSize->data);

  /* indxNumOld == indxNumNew */
  indxOld = pld->indx;
  primNumOld = pld->primNum;
  if (1 != primNumOld) {
    sprintf(err, "%s: sorry! stupid implementation can't "
            "do primNum %u (only 1)",
            me, primNumOld);
    biffAdd(LIMN, err); airMopError(mop); return 1;
  }
  typeOld = pld->type;
  icntOld = pld->icnt;
  indxNew = AIR_CAST(unsigned int*,
                     calloc(pld->indxNum, sizeof(unsigned int)));
  typeNew = AIR_CAST(unsigned char*,
                     calloc(primNumNew, sizeof(unsigned char)));
  icntNew = AIR_CAST(unsigned int*,
                     calloc(primNumNew, sizeof(unsigned int)));
  if (!(indxNew && typeNew && icntNew)) {
    sprintf(err, "%s: couldn't allocate new polydata arrays", me);
    biffAdd(LIMN, err); airMopError(mop); return 1;
  }
  pld->indx = indxNew;
  pld->primNum = primNumNew;
  pld->type = typeNew;
  pld->icnt = icntNew;
  airMopAdd(mop, indxOld, airFree, airMopAlways);
  airMopAdd(mop, typeOld, airFree, airMopAlways);
  airMopAdd(mop, icntOld, airFree, airMopAlways);

  /* this multi-pass thing is really stupid 
     (and assumes stupid primNumOld = 1) */
  baseIndx = pld->indx;
  for (primIdxNew=0; primIdxNew<pld->primNum; primIdxNew++) {
    unsigned int realTriIdx;
    pld->type[primIdxNew] = limnPrimitiveTriangles;
    pld->icnt[primIdxNew] = 0;
    for (realTriIdx=0; realTriIdx<realTriNum; realTriIdx++) {
      if (triMap[realTriIdx] == primIdxNew) {
        ELL_3V_COPY(baseIndx, indxOld + 3*realTriIdx);
        baseIndx += 3;
        pld->icnt[primIdxNew] += 3;
      }
    }
  }
  
  airMopOkay(mop);
  return 0;
}

int
limnPolyDataPrimitiveSort(limnPolyData *pld, const Nrrd *_nval) { 
  char me[]="limnPolyDataPrimitiveSort", err[BIFF_STRLEN];
  Nrrd *nval, *nrec;
  const Nrrd *ntwo[2];
  airArray *mop;
  double *rec;
  unsigned int primIdx, **startIndx, *indxNew, *baseIndx, *icntNew;
  unsigned char *typeNew;
  int E;
  
  if (!(pld && _nval)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(LIMN, err); return 1;
  }
  if (!(1 == _nval->dim
        && nrrdTypeBlock != _nval->type
        && _nval->axis[0].size == pld->primNum)) {
    sprintf(err, "%s: need 1-D %u-len scalar nrrd "
            "(not %u-D type %s, axis[0].size %u)", me,
            pld->primNum,
            _nval->dim, airEnumStr(nrrdType, _nval->type),
            AIR_CAST(unsigned int, _nval->axis[0].size));
    biffAdd(LIMN, err); return 1;
  }

  mop = airMopNew();
  nval = nrrdNew();
  airMopAdd(mop, nval, (airMopper)nrrdNuke, airMopAlways);
  nrec = nrrdNew();
  airMopAdd(mop, nrec, (airMopper)nrrdNuke, airMopAlways);
  E = 0;
  if (!E) E |= nrrdConvert(nval, _nval, nrrdTypeDouble);
  ntwo[0] = nval;
  ntwo[1] = nval;
  if (!E) E |= nrrdJoin(nrec, ntwo, 2, 0, AIR_TRUE);
  if (E) {
    sprintf(err, "%s: problem creating records", me);
    biffMove(LIMN, err, NRRD); airMopError(mop); return 1;
  }
  rec = AIR_CAST(double *, nrec->data);
  for (primIdx=0; primIdx<pld->primNum; primIdx++) {
    rec[1 + 2*primIdx] = primIdx;
  }
  qsort(rec, pld->primNum, 2*sizeof(double),
        nrrdValCompareInv[nrrdTypeDouble]);

  startIndx = AIR_CAST(unsigned int**, calloc(pld->primNum,
                                              sizeof(unsigned int*)));
  indxNew = AIR_CAST(unsigned int*, calloc(pld->indxNum,
                                           sizeof(unsigned int)));
  icntNew = AIR_CAST(unsigned int*, calloc(pld->primNum,
                                           sizeof(unsigned int)));
  typeNew = AIR_CAST(unsigned char*, calloc(pld->primNum,
                                            sizeof(unsigned char)));
  if (!(startIndx && indxNew && icntNew && typeNew)) {
    sprintf(err, "%s: couldn't allocated temp buffers", me);
    biffAdd(LIMN, err); airMopError(mop); return 1;
  }
  airMopAdd(mop, startIndx, airFree, airMopAlways);

  baseIndx = pld->indx;
  for (primIdx=0; primIdx<pld->primNum; primIdx++) {
    startIndx[primIdx] = baseIndx;
    baseIndx += pld->icnt[primIdx];
  }
  baseIndx = indxNew;
  for (primIdx=0; primIdx<pld->primNum; primIdx++) {
    unsigned int sortIdx;
    sortIdx = AIR_CAST(unsigned int, rec[1 + 2*primIdx]);
    memcpy(baseIndx, startIndx[sortIdx],
           pld->icnt[sortIdx]*sizeof(unsigned int));
    icntNew[primIdx] = pld->icnt[sortIdx];
    typeNew[primIdx] = pld->type[sortIdx];
    baseIndx += pld->icnt[sortIdx];
  }

  airFree(pld->indx);
  pld->indx = indxNew;
  airFree(pld->type);
  pld->type = typeNew;
  airFree(pld->icnt);
  pld->icnt = icntNew;

  airMopOkay(mop);
  return 0;
}

int
limnPolyDataVertexWindingFlip(limnPolyData *pld) { 
  char me[]="limnPolyDataVertexWindingFlip", err[BIFF_STRLEN];
  unsigned int baseVertIdx, primIdx;

  if (!pld) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(LIMN, err); return 1;
  }
  if ((1 << limnPrimitiveTriangles) != limnPolyDataPrimitiveTypes(pld)) {
    sprintf(err, "%s: sorry, can only handle %s primitives", me,
            airEnumStr(limnPrimitive, limnPrimitiveTriangles));
    biffAdd(LIMN, err); return 1;
  }

  baseVertIdx = 0;
  for (primIdx=0; primIdx<pld->primNum; primIdx++) {
    unsigned int triNum, triIdx, *indxLine, tmpIdx;
    triNum = pld->icnt[primIdx]/3;
    for (triIdx=0; triIdx<triNum; triIdx++) {
      indxLine = pld->indx + baseVertIdx + 3*triIdx;
      tmpIdx = indxLine[0];
      indxLine[0] = indxLine[2];
      indxLine[2] = tmpIdx;
    }
    baseVertIdx += pld->icnt[primIdx];
  }

  return 0;
}

