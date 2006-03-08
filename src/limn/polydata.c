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
static void
edgeNeighbors(limnPolyData *pld, int neigh[3], unsigned int face) {

}
*/

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
    baseVertIdx,     /* first vertex for current primitive */
    *triWithVertNum, /* 1D array (len totalTriNum) of # tri with vert[i] */
    twvnm,           /* max # of tris on single vertex */
    *triWithVert,    /* 2D array (twvnm-by-totalTriNum) of per-vert tris */
    doneTriNum,      /* # triangles finished so far */
    *todo;           /* the to-do stack */
  unsigned char
    *triDone;        /* 1D array (len totalTriNum) record of done tris */
  Nrrd *nTriWithVert;
  airArray *mop, *todoArr;
  
  if (!pld) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(LIMN, err); return 1;
  }

  /* calculate maxTriPerPrim and totalTriNum */
  maxTriPerPrim = 0;
  for (primIdx=0; primIdx<pld->primNum; primIdx++) {
    if (limnPrimitiveTriangles != pld->type[primIdx]) {
      sprintf(err, "%s: sorry, can only have %s prims now (prim %u is %s)",
              me, airEnumStr(limnPrimitive, limnPrimitiveTriangles),
              primIdx, airEnumStr(limnPrimitive, pld->type[primIdx]));
      biffAdd(LIMN, err); return 1;
    }
    maxTriPerPrim = AIR_MAX(maxTriPerPrim, pld->icnt[primIdx]/3);
  }
  totalTriNum = maxTriPerPrim*pld->primNum;

  /* allocate 1-D triWithVertNum */
  mop = airMopNew();
  triWithVertNum = AIR_CAST(unsigned int*,
                            calloc(pld->xyzwNum, sizeof(unsigned int)));
  if (!triWithVertNum) {
    sprintf(err, "%s: couldn't allocate temp arrays", me);
    biffAdd(LIMN, err); airMopError(mop); return 1;
  }
  airMopAdd(mop, triWithVertNum, airFree, airMopAlways);

  /* fill in 1-D triWithVertNum */
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

  /* find (1 less than) axis-0 size of TriWithVert */
  twvnm = 0;
  for (vertIdx=0; vertIdx<pld->xyzwNum; vertIdx++) {
    fprintf(stderr, "!%s: triWithVertNum[%u] = %u\n", me, vertIdx,
            triWithVertNum[vertIdx]);
    twvnm = AIR_MAX(twvnm, triWithVertNum[vertIdx]);
  }

  /* allocate TriWithVert */
  nTriWithVert = nrrdNew();
  airMopAdd(mop, nTriWithVert, (airMopper)nrrdNuke, airMopAlways);
  if (nrrdMaybeAlloc_va(nTriWithVert, nrrdTypeUInt, 2, 
                        AIR_CAST(size_t, 1+twvnm),
                        AIR_CAST(size_t, pld->xyzwNum))) {
    sprintf(err, "%s: couldn't allocate TriWithVert", me);
    biffMove(LIMN, err, NRRD); airMopError(mop); return 1;
  }
  triWithVert = AIR_CAST(unsigned int*, nTriWithVert->data);

  /* fill in TriWithVert.  Note clever indexing scheme for triangles */
  for (primIdx=0; primIdx<pld->primNum; primIdx++) {
    unsigned int triNum, *indxLine, *twvLine, ii;
    triNum = pld->icnt[primIdx]/3;
    for (triIdx=0; triIdx<triNum; triIdx++) {
      indxLine = pld->indx + baseVertIdx + 3*triIdx;
      for (ii=0; ii<3; ii++) {
        twvLine = triWithVert + (1+twvnm)*indxLine[ii];
        fprintf(stderr, "%d/%u/%d/%d  ", ii, indxLine[ii],
                twvLine - triWithVert,
                (twvLine - triWithVert)/(1+twvnm));
        /* twvLine[1+twvLine[0]] = triIdx + maxTriPerPrim*primIdx; */
        /* twvLine[0]++; */
      }
      fprintf(stderr, "\n");
    }
    baseVertIdx += 3*triNum;
  }
  
  /* nrrdSave("ntwv.nrrd", nTriWithVert, NULL); */

  /*
  create todo stack;

  while (#done < total # tri) {

    pick first undone triangle, flag as done
    neigh = edgeNeighbors(first);
    push all (neigh,v0,v1)
  
    while (todo) {
      (this,v0,v1) = todo.pop;
      fix(this,v0,v1);
      flag this as done;
      neigh = edgeNeighbors(this);
      push (neigh,v0,v1) not done;
    }
  }
  */  

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

