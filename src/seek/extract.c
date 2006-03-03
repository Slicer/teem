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

#include "seek.h"
#include "privateSeek.h"

typedef struct {
  int evti[12];  /* edge vertex index */
  double (*scllup)(const void *, size_t);
  const void *scldata;
  airArray *xyzwArr, *normArr, *indxArr;
} baggage;

static baggage *
baggageNew(seekContext *sctx) {
  baggage *bag;

  bag = (baggage *)calloc(1, sizeof(baggage));

  /*                     X      Y */
  bag->evti[ 0] = 0 + 5*(0 + sctx->sx*0);
  bag->evti[ 1] = 1 + 5*(0 + sctx->sx*0);
  bag->evti[ 2] = 1 + 5*(1 + sctx->sx*0);
  bag->evti[ 3] = 0 + 5*(0 + sctx->sx*1);
  bag->evti[ 4] = 2 + 5*(0 + sctx->sx*0);
  bag->evti[ 5] = 2 + 5*(1 + sctx->sx*0);
  bag->evti[ 6] = 2 + 5*(0 + sctx->sx*1);
  bag->evti[ 7] = 2 + 5*(1 + sctx->sx*1);
  bag->evti[ 8] = 3 + 5*(0 + sctx->sx*0);
  bag->evti[ 9] = 4 + 5*(0 + sctx->sx*0);
  bag->evti[10] = 4 + 5*(1 + sctx->sx*0);
  bag->evti[11] = 3 + 5*(0 + sctx->sx*1);

  if (seekTypeIsocontour == sctx->type) {
    if (sctx->ninscl) {
      bag->scllup = nrrdDLookup[sctx->ninscl->type];
      bag->scldata = sctx->ninscl->data;
    } else {
      bag->scllup = nrrdDLookup[sctx->nsclDerived->type];
      bag->scldata = sctx->nsclDerived->data;
    }
  } else {
    bag->scllup = NULL;
    bag->scldata = NULL;
  }

  bag->xyzwArr = NULL;
  bag->normArr = NULL;
  bag->indxArr = NULL;
  return bag;
}

static baggage *
baggageNix(baggage *bag) {

  if (bag) {
    airArrayNix(bag->normArr);
    airArrayNix(bag->xyzwArr);
    airArrayNix(bag->indxArr);

    airFree(bag);
  }
  return NULL;
}

static int
outputInit(seekContext *sctx, baggage *bag, limnPolyData *lpld) {
  char me[]="outputInit", err[BIFF_STRLEN];
  unsigned int estVertNum, estFaceNum, minI, maxI, valI, *spanHist;
  int E;

  if (seekTypeIsocontour == sctx->type) {
    unsigned int estVoxNum=0;
    /* nixed the short-cut based on seeing if the isovalue was outside
       the value range, since it complicated the logic... */
    /* estimate number of voxels, faces, and vertices involved */
    spanHist = AIR_CAST(unsigned int*, sctx->nspanHist->data);
    valI = airIndex(sctx->range->min, sctx->isovalue, sctx->range->max,
                    sctx->spanSize);
    for (minI=0; minI<=valI; minI++) {
      for (maxI=valI; maxI<sctx->spanSize; maxI++) {
        estVoxNum += spanHist[minI + sctx->spanSize*maxI];
      }
    }
    estVertNum = AIR_CAST(unsigned int, estVoxNum*(sctx->vertsPerVoxel));
    estFaceNum = AIR_CAST(unsigned int, estVoxNum*(sctx->facesPerVoxel));
    if (sctx->verbose) {
      fprintf(stderr, "%s: estimated vox --> vert, face: %u --> %u, %u\n", me,
              estVoxNum, estVertNum, estFaceNum);
    }
  } else {
    estVertNum = 0;
    estFaceNum = 0;
  }
  /* need something non-zero so that pre-allocations below aren't no-ops */
  estVertNum = AIR_MAX(1, estVertNum);
  estFaceNum = AIR_MAX(1, estFaceNum);
  
  /* initialize limnPolyData with estimated # faces and vertices */
  /* we will manage the innards of the limnPolyData entirely ourselves */
  if (limnPolyDataAlloc(lpld, 0, 0, 0, 0)) {
    sprintf(err, "%s: trouble emptying given polydata", me);
    biffAdd(SEEK, err); return 1;
  }
  bag->xyzwArr = airArrayNew((void**)&(lpld->xyzw), &(lpld->vertNum),
                             4*sizeof(float), sctx->pldArrIncr);
  if (sctx->normalsFind) {
    bag->normArr = airArrayNew((void**)&(lpld->norm), &(lpld->normNum),
                               3*sizeof(float), sctx->pldArrIncr);
  } else {
    bag->normArr = NULL;
  }
  bag->indxArr = airArrayNew((void**)&(lpld->indx), &(lpld->indxNum),
                             sizeof(unsigned int), sctx->pldArrIncr);
  lpld->primNum = 1;  /* for now, its just triangle soup */
  lpld->type = (unsigned char *)calloc(lpld->primNum, sizeof(unsigned char));
  lpld->icnt = (unsigned int *)calloc(lpld->primNum, sizeof(unsigned int));
  lpld->type[0] = limnPrimitiveTriangles;
  lpld->icnt[0] = 0;  /* incremented below */
  
  E = 0;
  airArrayLenPreSet(bag->xyzwArr, estVertNum);
  E |= !(bag->xyzwArr->data);
  if (sctx->normalsFind) {
    airArrayLenPreSet(bag->normArr, estVertNum);
    E |= !(bag->normArr->data);
  }
  airArrayLenPreSet(bag->indxArr, 3*estFaceNum);
  E |= !(bag->indxArr->data);
  if (E) {
    sprintf(err, "%s: couldn't pre-allocate contour geometry (%p %p %p)", me,
            bag->xyzwArr->data,
            (sctx->normalsFind ? bag->normArr->data : NULL),
            bag->indxArr->data);
    biffAdd(SEEK, err); return 1;
  }

  /* initialize output summary info */
  sctx->voxNum = 0;
  sctx->vertNum = 0;
  sctx->faceNum = 0;

  return 0;
}

static double
sclGet(seekContext *sctx, baggage *bag,
       unsigned int xi, unsigned int yi, unsigned int zi) {
  double val;

  zi = AIR_MIN(sctx->sz-1, zi);
  return bag->scllup(bag->scldata, xi + sctx->sx*(yi + sctx->sy*zi));
}

static void
shuffleProbe(seekContext *sctx, baggage *bag, unsigned int zi) {
  unsigned int xi, yi, sx, sy, si, spi;

  sx = AIR_CAST(unsigned int, sctx->sx);
  sy = AIR_CAST(unsigned int, sctx->sy);

  for (yi=0; yi<sy; yi++) {
    for (xi=0; xi<sx; xi++) {
      si = xi + sx*yi;
      spi = (xi+1) + (sx+2)*(yi+1);
      if (!zi) {
        sctx->vidx[0 + 5*si] = -1;
        sctx->vidx[1 + 5*si] = -1;
      } else {
        sctx->vidx[0 + 5*si] = sctx->vidx[3 + 5*si];
        sctx->vidx[1 + 5*si] = sctx->vidx[4 + 5*si];
      }
      sctx->vidx[2 + 5*si] = -1;
      sctx->vidx[3 + 5*si] = -1;
      sctx->vidx[4 + 5*si] = -1;
      if (seekTypeIsocontour == sctx->type) {
        if (!zi) {
          sctx->sclv[0 + 4*spi] = (sclGet(sctx, bag, xi, yi, 0)
                                   - sctx->isovalue);
          sctx->sclv[1 + 4*spi] = (sclGet(sctx, bag, xi, yi, 0)
                                   - sctx->isovalue);
          sctx->sclv[2 + 4*spi] = (sclGet(sctx, bag, xi, yi, 1)
                                   - sctx->isovalue);
        } else {
          sctx->sclv[0 + 4*spi] = sctx->sclv[1 + 4*spi];
          sctx->sclv[1 + 4*spi] = sctx->sclv[2 + 4*spi];
          sctx->sclv[2 + 4*spi] = sctx->sclv[3 + 4*spi];
        }
        sctx->sclv[3 + 4*spi] = (sclGet(sctx, bag, xi, yi, zi+2)
                                 - sctx->isovalue);
      }
    }
    /* copy ends of this scanline left/right to margin */
    if (seekTypeIsocontour == sctx->type) {
      ELL_4V_COPY(sctx->sclv + 4*(0    + (sx+2)*(yi+1)),
                  sctx->sclv + 4*(1    + (sx+2)*(yi+1)));
      ELL_4V_COPY(sctx->sclv + 4*(sx+1 + (sx+2)*(yi+1)),
                  sctx->sclv + 4*(sx   + (sx+2)*(yi+1)));
    }
  }
  /* copy top and bottom scanline up/down to margin */
  if (seekTypeIsocontour == sctx->type) {
    for (xi=0; xi<sx+2; xi++) {
      ELL_4V_COPY(sctx->sclv + 4*(xi + (sx+2)*0),
                  sctx->sclv + 4*(xi + (sx+2)*1));
      ELL_4V_COPY(sctx->sclv + 4*(xi + (sx+2)*(sy+1)),
                  sctx->sclv + 4*(xi + (sx+2)*sy));
    }
  }
  return;
}

static void
triangulate(seekContext *sctx, baggage *bag, limnPolyData *lpld,
            unsigned int zi) {
  int e2v[12][2] = {        /* maps edge index to corner vertex indices */
    {0, 1},  /*  0 */
    {0, 2},  /*  1 */
    {1, 3},  /*  2 */
    {2, 3},  /*  3 */
    {0, 4},  /*  4 */
    {1, 5},  /*  5 */
    {2, 6},  /*  6 */
    {3, 7},  /*  7 */
    {4, 5},  /*  8 */
    {4, 6},  /*  9 */
    {5, 7},  /* 10 */
    {6, 7}   /* 11 */
  };
  double vccoord[8][3] = {  /* vertex corner coordinates */
    {0, 0, 0},  /* 0 */
    {1, 0, 0},  /* 1 */
    {0, 1, 0},  /* 2 */
    {1, 1, 0},  /* 3 */
    {0, 0, 1},  /* 4 */
    {1, 0, 1},  /* 5 */
    {0, 1, 1},  /* 6 */
    {1, 1, 1}   /* 7 */
  };

  unsigned xi, yi, sx, sy, si, spi;

  sx = AIR_CAST(unsigned int, sctx->sx);
  sy = AIR_CAST(unsigned int, sctx->sy);

  for (yi=0; yi<sy-1; yi++) {
    double vval[8], vgrad[8][3], vert[3], tvertA[4], tvertB[4], ww;
    unsigned char vcase;
    int ti, vi, ei, vi0, vi1, ecase;
    const int *tcase;
    unsigned int vii[3];
    for (xi=0; xi<sx-1; xi++) {
      si = xi + sx*yi;
      spi = (xi+1) + (sx+2)*(yi+1);
      if (seekTypeIsocontour == sctx->type) {
        /* learn voxel values */
        /*                      X   Y                 Z */
        vval[0] = sctx->sclv[4*(0 + 0*(sx+2) + spi) + 1];
        vval[1] = sctx->sclv[4*(1 + 0*(sx+2) + spi) + 1];
        vval[2] = sctx->sclv[4*(0 + 1*(sx+2) + spi) + 1];
        vval[3] = sctx->sclv[4*(1 + 1*(sx+2) + spi) + 1];
        vval[4] = sctx->sclv[4*(0 + 0*(sx+2) + spi) + 2];
        vval[5] = sctx->sclv[4*(1 + 0*(sx+2) + spi) + 2];
        vval[6] = sctx->sclv[4*(0 + 1*(sx+2) + spi) + 2];
        vval[7] = sctx->sclv[4*(1 + 1*(sx+2) + spi) + 2];
      }
      /*
      if (seekTypeRidgeSurface == sctx->type
          || seekTypeValleySurface == sctx->type) {
        vvalSurfSet(sctx, vval, evidx, xi, yi);
      }
      */
      /* determine voxel and edge case */
      vcase = 0;
      for (vi=0; vi<8; vi++) {
        vcase |= (vval[vi] > 0) << vi;
      }
      if (0 == vcase || 255 == vcase) {
        /* no triangles added here */
        continue;
      }
      /* set voxel corner gradients */
      if (sctx->normalsFind && !sctx->normAns) {
        /* voxelGrads(vgrad, sctx->sclv, sx, spi); */
      }
      sctx->voxNum++;
      ecase = seekContour3DTopoHackEdge[vcase];
      /* create new vertices as needed */
      for (ei=0; ei<12; ei++) {
        if ((ecase & (1 << ei))
            && -1 == sctx->vidx[bag->evti[ei] + 5*si]) {
          int ovi;
          double tvec[3], grad[3], tlen;
          /* this edge is needed for triangulation,
             and, we haven't already created a vertex for it */
          vi0 = e2v[ei][0];
          vi1 = e2v[ei][1];
          ww = vval[vi0]/(vval[vi0] - vval[vi1]);
          ELL_3V_LERP(vert, ww, vccoord[vi0], vccoord[vi1]);
          ELL_4V_SET(tvertA, vert[0] + xi, vert[1] + yi, vert[2] + zi, 1);
          ELL_4MV_MUL(tvertB, sctx->shape->ItoW, tvertA);
          ELL_4V_HOMOG(tvertB, tvertB);
          ovi = sctx->vidx[bag->evti[ei] + 5*si] = 
            airArrayLenIncr(bag->xyzwArr, 1);
          ELL_4V_SET_TT(lpld->xyzw + 4*ovi, float,
                        tvertB[0], tvertB[1], tvertB[2], 1.0);
          if (sctx->normalsFind) {
            airArrayLenIncr(bag->normArr, 1);
            if (sctx->normAns) {
              gageProbe(sctx->gctx, tvertA[0], tvertA[1], tvertA[2]);
              ELL_3V_SCALE_TT(lpld->norm + 3*ovi, float, -1, sctx->normAns);
              if (sctx->reverse) {
                ELL_3V_SCALE(lpld->norm + 3*ovi, -1, lpld->norm + 3*ovi);
              }
            } else {
              ELL_3V_LERP(grad, ww, vgrad[vi0], vgrad[vi1]);
              ELL_3MV_MUL(tvec, sctx->txfNormal, grad);
              ELL_3V_NORM_TT(lpld->norm + 3*ovi, float, tvec, tlen);
            }
          }
          sctx->vertNum++;
          /*
            fprintf(stderr, "%s: vert %d (edge %d) of (%d,%d,%d) "
            "at %g %g %g\n",
            me, sctx->vidx[bag->evti[ei] + 5*si], ei, xi, yi, zi,
            vert[0] + xi, vert[1] + yi, vert[2] + zi);
          */
        }
      }
      /* add triangles */
      ti = 0;
      tcase = seekContour3DTopoHackTriangle[vcase];
      while (-1 != tcase[0 + 3*ti]) {
        unsigned iii;
        ELL_3V_SET(vii,
                   sctx->vidx[bag->evti[tcase[0 + 3*ti]] + 5*si],
                   sctx->vidx[bag->evti[tcase[1 + 3*ti]] + 5*si],
                   sctx->vidx[bag->evti[tcase[2 + 3*ti]] + 5*si]);
        if (sctx->reverse) {
          int tmpi;
          tmpi = vii[1]; vii[1] = vii[2]; vii[2] = tmpi;
        }
        iii = airArrayLenIncr(bag->indxArr, 3);
        ELL_3V_COPY(lpld->indx + iii, vii);
        lpld->icnt[0] += 3;
        sctx->faceNum++;
        ti++;
      }
    }
  }
  return;
}

static int
surfaceExtract(seekContext *sctx, limnPolyData *lpld) {
  char me[]="surfaceExtract", err[BIFF_STRLEN];
  int E;
  unsigned int zi, si, spi, minI, maxI, valI, evidx;
  baggage *bag;

  bag = baggageNew(sctx);

  /* this creates the airArrays in bag */
  if (outputInit(sctx, bag, lpld)) {
    sprintf(err, "%s: trouble", me);
    biffAdd(SEEK, err); return 1;
  }

  shuffleProbe(sctx, bag, 0);
  for (zi=1; zi<sctx->sz; zi++) {
    shuffleProbe(sctx, bag, zi);
    triangulate(sctx, bag, lpld, zi);
  }

  /* this cleans up the airArrays in bag */
  baggageNix(bag);

  return 0;
}

int
seekExtract(seekContext *sctx, limnPolyData *lpld) {
  char me[]="seekExtract", err[BIFF_STRLEN];
  double time0;
  int E;

  if (!( sctx && lpld )) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(SEEK, err); return 1;
  }

  if (sctx->verbose) {
    fprintf(stderr, "%s: --------------------\n", me);
    fprintf(stderr, "%s: flagResult = %d\n", me,
            sctx->flag[flagResult]);
  }

  /* start time */
  time0 = airTime();

  switch(sctx->type) {
  case seekTypeIsocontour:
  case seekTypeRidgeSurface:
  case seekTypeValleySurface:
    E = surfaceExtract(sctx, lpld);
    break;
  default:
    sprintf(err, "%s: sorry, %s extraction not implemented", me,
            airEnumStr(seekType, sctx->type));
    biffAdd(SEEK, err); return 1;
    break;
  }
  if (E) {
    sprintf(err, "%s: trouble", me);
    biffAdd(SEEK, err); return 1;
  }

  /* end time */
  sctx->time = airTime() - time0;

  sctx->flag[flagResult] = AIR_FALSE;

  return 0;
}
