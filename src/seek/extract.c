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

static double
mode(double v[3]) {
  double num, den;
  num = (v[0] + v[1] - 2*v[2])*(2*v[0] - v[1] - v[2])*(v[0] - 2*v[1] + v[2]);
  den = v[0]*v[0] + v[1]*v[1] + v[2]*v[2] - v[1]*v[2] - v[0]*v[1] - v[0]*v[2];
  den = sqrt(den);
  return (den ? num/(2*den*den*den) : 0);
}

typedef struct {
  airArray *xyzwArr, *normArr, *indxArr;
} baggage;

static baggage *
baggageNew() {
  baggage *bag;

  bag = (baggage *)calloc(1, sizeof(baggage));
  bag->xyzwArr = NULL;
  bag->normArr = NULL;
  bag->indxArr = NULL;
  return bag;
}

static baggage *
baggageNix(baggage *bag) {

  if (bag) {
    airArrayNix(bag->xyzwArr);
    airArrayNix(bag->normArr);
    airArrayNix(bag->indxArr);
    airFree(bag);
  }
  return NULL;
}

#if 0

#define VAL(xx, yy, zz)  (val[4*( (xx) + (yy)*(sx+2) + spi) + (zz+1)])
void
_seek3DVoxelGrads(double vgrad[8][3], double *val, int sx, int spi) {
  ELL_3V_SET(vgrad[0],
             VAL( 1,  0,  0) - VAL(-1,  0,  0),
             VAL( 0,  1,  0) - VAL( 0, -1,  0),
             VAL( 0,  0,  1) - VAL( 0,  0, -1));
  ELL_3V_SET(vgrad[1],
             VAL( 2,  0,  0) - VAL( 0,  0,  0),
             VAL( 1,  1,  0) - VAL( 1, -1,  0),
             VAL( 1,  0,  1) - VAL( 1,  0, -1));
  ELL_3V_SET(vgrad[2],
             VAL( 1,  1,  0) - VAL(-1,  1,  0),
             VAL( 0,  2,  0) - VAL( 0,  0,  0),
             VAL( 0,  1,  1) - VAL( 0,  1, -1));
  ELL_3V_SET(vgrad[3],
             VAL( 2,  1,  0) - VAL( 0,  1,  0),
             VAL( 1,  2,  0) - VAL( 1,  0,  0),
             VAL( 1,  1,  1) - VAL( 1,  1, -1));
  ELL_3V_SET(vgrad[4],
             VAL( 1,  0,  1) - VAL(-1,  0,  1),
             VAL( 0,  1,  1) - VAL( 0, -1,  1),
             VAL( 0,  0,  2) - VAL( 0,  0,  0));
  ELL_3V_SET(vgrad[5],
             VAL( 2,  0,  1) - VAL( 0,  0,  1),
             VAL( 1,  1,  1) - VAL( 1, -1,  1),
             VAL( 1,  0,  2) - VAL( 1,  0,  0));
  ELL_3V_SET(vgrad[6],
             VAL( 1,  1,  1) - VAL(-1,  1,  1),
             VAL( 0,  2,  1) - VAL( 0,  0,  1),
             VAL( 0,  1,  2) - VAL( 0,  1,  0));
  ELL_3V_SET(vgrad[7],
             VAL( 2,  1,  1) - VAL( 0,  1,  1),
             VAL( 1,  2,  1) - VAL( 1,  0,  1),
             VAL( 1,  1,  2) - VAL( 1,  1,  0));
}
#undef VAL

double
_seek3DIsocontourValue(seek3DContext *lctx,
                              unsigned int xi,
                              unsigned int yi,
                              unsigned int zi) {
  double val;

  if (lctx->gctx) {
    gageProbe(lctx->gctx, xi, yi, zi);
    val = lctx->valAns[0];
  } else {
    val = lctx->vollup(lctx->nvol->data, xi + lctx->sx*(yi + lctx->sy*zi));
  }
  return val;
}

static int
evecTrans(seek3DContext *lctx, int verb,
          double evecB[3], unsigned char *evidxOut, /* output */
          const double evecA[3], const unsigned int evidxInit,
          const double posA[3], const double edge[3]) {
  char me[]="evecTrans", err[BIFF_STRLEN];
  double u, du, current[3], next[3][3], posNext[3], posB[3],
    bestDot, dot[3];
  double wantDot = 0.9; /* between cos(pi/6) and cos(pi/8) */
  double minDu = 0.0001;
  unsigned char bestDotIdx;

#define ORIENT(evi) \
  if (ELL_3V_DOT(current, next[evi]) < 0) { \
    ELL_3V_SCALE(next[evi], -1, next[evi]); \
  } \
  dot[evi] = ELL_3V_DOT(current, next[evi])

#define SETNEXT \
  ELL_3V_SCALE_ADD2(posNext, 1.0-(u+du), posA, u+du, posB); \
  gageProbe(lctx->gctx, posNext[0], posNext[1], posNext[2]); \
  ELL_3V_COPY(next[0], lctx->evecAns + 3*0); \
  ELL_3V_COPY(next[1], lctx->evecAns + 3*1); \
  ELL_3V_COPY(next[2], lctx->evecAns + 3*2); \
  ORIENT(0); \
  ORIENT(1); \
  ORIENT(2)

  u = 0;
  du = 0.49999;
  /* take evecA as reference */
  ELL_3V_COPY(current, evecA);
  ELL_3V_ADD2(posB, posA, edge);
  if (verb) {
    fprintf(stderr, "!%s: ******** current = (%g,%g,%g)\n", me,
            current[0], current[1], current[2]);
    fprintf(stderr, "    posA=(%g,%g,%g)   posB=(%g,%g,%g);  u=%g,du=%g\n", 
            posA[0], posA[1], posA[2], posB[0], posB[1], posB[2], u, du);
  }
  while (u+du < 1.0) {
    SETNEXT;
    while (bestDot < wantDot) {
      /* angle between current and next is too big; reduce step */
      du /= 2;
      if (du < minDu) {
        sprintf(err, "%s: got to du=%g < minDu=%g (dot=%g) at "
                "u=%g, posA=(%g,%g,%g), edge=(%g,%g,%g)", me, du, minDu,
                ELL_3V_DOT(current, next[bestDotIdx]),
                u, posA[0], posA[1], posA[2], edge[0], edge[1], edge[2]);
        biffAdd(SEEK, err); return 1;
      }
      SETNEXT;
    }
    /* current and next have a small angle between them */
    ELL_3V_COPY(current, next[bestDotIdx]);
    u += du;
  }
  ELL_3V_COPY(evecB, current);
  *evidxOut = bestDotIdx;
  if (verb) {
    fprintf(stderr, "%s: done\n", me);
  }

#undef ORIENT
#undef SETNEXT

  return 0;
}

static int
eflipSet(seek3DContext *lctx, unsigned int evidx,
         const int zi, const int zdo) {
  char me[]="eflipSet", err[BIFF_STRLEN];
  unsigned int sx, sy, xi, yi, P[3];
  signed char E[3], *eflip;
  double edge[3], evecA[3], evecB[3], evecC[3], pos[3],
    mode, mmin, mmax;
  int ret;

  /*
  char fname[AIR_STRLEN_SMALL];
  Nrrd *ntmp;
  ntmp = nrrdNew();
  nrrdWrap_va(ntmp, lctx->evec, nrrdTypeDouble, 4,
              AIR_CAST(size_t, 9), AIR_CAST(size_t, 2),
              AIR_CAST(size_t, lctx->sx), AIR_CAST(size_t, lctx->sy));
  sprintf(fname, "evec-%02d.nrrd", zi);
  nrrdSave(fname, ntmp, NULL);
  nrrdWrap_va(ntmp, lctx->eval, nrrdTypeDouble, 4,
              AIR_CAST(size_t, 3), AIR_CAST(size_t, 2),
              AIR_CAST(size_t, lctx->sx), AIR_CAST(size_t, lctx->sy));
  sprintf(fname, "eval-%02d.nrrd", zi);
  nrrdSave(fname, ntmp, NULL);
  nrrdWrap_va(ntmp, lctx->grad, nrrdTypeDouble, 4,
              AIR_CAST(size_t, 3), AIR_CAST(size_t, 2),
              AIR_CAST(size_t, lctx->sx), AIR_CAST(size_t, lctx->sy));
  sprintf(fname, "grad-%02d.nrrd", zi);
  nrrdSave(fname, ntmp, NULL);
  ntmp = nrrdNix(ntmp);
  */

  fprintf(stderr, "!%s(%d): hello\n", me, zi);

#define MMINMAX \
  mmin = AIR_MIN(mmin, mode); \
  mmax = AIR_MAX(mmax, mode)

#define SETPOS \
  ELL_3V_SET(pos, xi+P[0], yi+P[1], zi+P[2]); \
  ELL_3V_COPY(evecA, lctx->evec + 3*evidx \
                     + 9*(P[2] + 2*(xi+P[0] + sx*(yi+P[1]))))

#define TRANSPORT \
  ELL_3V_COPY(evecC, lctx->evec + 3*evidx \
                     + 9*(P[2]+E[2] + 2*(xi+P[0]+E[0] + sx*(yi+P[1]+E[1])))); \
  ELL_3V_COPY(edge, E); \
  ret = evecTrans(lctx, AIR_FALSE, evecB, evecA, evidx, pos, edge); \
  if (1 == ret) { \
    sprintf(err, "%s: evec %u at (%g,%g,%g) along (%g,%g,%g), " \
            "mode range [%g,%g]", me, \
            evidx, pos[0], pos[1], pos[2], edge[0], edge[1], edge[2], \
            mmin, mmax); \
    biffAdd(SEEK, err); return 1; \
  } \
  if (2 == ret) { \
    /* we hit positive mode inside a voxel when it didn't have positive \
       mode on any corner.  HACK: set mode to zero on first corner \
       before moving to next voxel */ \
    lctx->mode[0 + 2*(xi+0 + sx*(yi+0))] = 0.0; \
    continue; \
  }

#define EVECDOT ELL_3V_DOT(evecB, evecC) > 0 ? 1 : -1

  sx = lctx->sx;
  sy = lctx->sy;
  for (yi=0; yi<sy-1; yi++) {
    for (xi=0; xi<sx-1; xi++) {
      mode = lctx->mode[0 + 2*(xi+0 + sx*(yi+0))]; mmin = mmax = mode;
      mode = lctx->mode[0 + 2*(xi+1 + sx*(yi+0))]; MMINMAX;
      mode = lctx->mode[0 + 2*(xi+0 + sx*(yi+1))]; MMINMAX;
      mode = lctx->mode[0 + 2*(xi+1 + sx*(yi+1))]; MMINMAX;
      mode = lctx->mode[1 + 2*(xi+0 + sx*(yi+0))]; MMINMAX;
      mode = lctx->mode[1 + 2*(xi+1 + sx*(yi+0))]; MMINMAX;
      mode = lctx->mode[1 + 2*(xi+0 + sx*(yi+1))]; MMINMAX;
      mode = lctx->mode[1 + 2*(xi+1 + sx*(yi+1))]; MMINMAX;
      if (0) {
        fprintf(stderr, "%s: (xi,yi,zi) = (%u,%u,%d); mode == ",
                me, xi, yi, zi);
        fprintf(stderr, "%g %g %g %g %g %g %g %g\n", 
                lctx->mode[0 + 2*(xi+0 + sx*(yi+0))],
                lctx->mode[0 + 2*(xi+1 + sx*(yi+0))],
                lctx->mode[0 + 2*(xi+0 + sx*(yi+1))],
                lctx->mode[0 + 2*(xi+1 + sx*(yi+1))],
                lctx->mode[1 + 2*(xi+0 + sx*(yi+0))],
                lctx->mode[1 + 2*(xi+1 + sx*(yi+0))],
                lctx->mode[1 + 2*(xi+0 + sx*(yi+1))],
                lctx->mode[1 + 2*(xi+1 + sx*(yi+1))]);
      }
      if ((seekTypeRidgeSurface == lctx->featureType
           && mmax >= 0)
          || (seekTypeValleySurface == lctx->featureType
              && mmin <= 0)) {
        /* the eigenvalue distribution suggests that we're not anywhere
           near the kind of feature we're looking for */
        continue;
      }
      /*
      eflip = lctx->eflip + 12*(xi + sx*yi);
      ELL_3V_SET(P, 0, 0, 1); SETPOS;
      ELL_3V_SET(E, 1, 0, 0); TRANSPORT; eflip[ 8] = EVECDOT;
      ELL_3V_SET(E, 0, 1, 0); TRANSPORT; eflip[ 9] = EVECDOT;
      if (zdo) {
        ELL_3V_SET(E, 0, 0, -1); TRANSPORT; eflip[ 4] = EVECDOT;
      }
      ELL_3V_SET(P, 1, 0, 1); SETPOS;
      ELL_3V_SET(E, 0, 1, 0); TRANSPORT; eflip[10] = EVECDOT;
      */
    }
  }

#undef MMINMAX
#undef SETPOS
#undef TRANSPORT
#undef EVECDOT

  return 0;
}

static void
vvalSurfSet(seek3DContext *lctx, double vval[8],
            unsigned int evidx, unsigned int xi, unsigned int yi) {
  unsigned int sx, sy, si, iii;
  signed char eflip[12];
  double evec[8][3], grad[8][3], *evp, mode, mmin, mmax;

  sx = lctx->sx;
  sy = lctx->sy;
  si = xi + sx*sy;
  evp = lctx->evec + 3*evidx;

#define MMINMAX \
  mmin = AIR_MIN(mmin, mode); \
  mmax = AIR_MAX(mmax, mode)

  /* !!! HOLY STUPID CUT + PASTE !!! */
  mode = lctx->mode[0 + 2*(xi+0 + sx*(yi+0))]; mmin = mmax = mode;
  mode = lctx->mode[0 + 2*(xi+1 + sx*(yi+0))]; MMINMAX;
  mode = lctx->mode[0 + 2*(xi+0 + sx*(yi+1))]; MMINMAX;
  mode = lctx->mode[0 + 2*(xi+1 + sx*(yi+1))]; MMINMAX;
  mode = lctx->mode[1 + 2*(xi+0 + sx*(yi+0))]; MMINMAX;
  mode = lctx->mode[1 + 2*(xi+1 + sx*(yi+0))]; MMINMAX;
  mode = lctx->mode[1 + 2*(xi+0 + sx*(yi+1))]; MMINMAX;
  mode = lctx->mode[1 + 2*(xi+1 + sx*(yi+1))]; MMINMAX;
  if ((seekTypeRidgeSurface == lctx->featureType
       && mmax >= 0)
      || (seekTypeValleySurface == lctx->featureType
          && mmin <= 0)) {
    /* the eigenvalue distribution suggests that we're not near
       the kind of feature we're looking for */
    for (iii=0; iii<8; iii++) {
      vval[iii] = 1.0;
    }
  } else {
    ELL_3V_COPY(grad[0], lctx->grad + 3*(0 + 2*(xi+0 + sx*(yi+0))));
    ELL_3V_COPY(evec[0], evp        + 9*(0 + 2*(xi+0 + sx*(yi+0))));
    ELL_3V_COPY(grad[1], lctx->grad + 3*(0 + 2*(xi+1 + sx*(yi+0))));
    ELL_3V_COPY(evec[1], evp        + 9*(0 + 2*(xi+1 + sx*(yi+0))));
    ELL_3V_COPY(grad[2], lctx->grad + 3*(0 + 2*(xi+0 + sx*(yi+1))));
    ELL_3V_COPY(evec[2], evp        + 9*(0 + 2*(xi+0 + sx*(yi+1))));
    ELL_3V_COPY(grad[3], lctx->grad + 3*(0 + 2*(xi+1 + sx*(yi+1))));
    ELL_3V_COPY(evec[3], evp        + 9*(0 + 2*(xi+1 + sx*(yi+1))));
    ELL_3V_COPY(grad[4], lctx->grad + 3*(1 + 2*(xi+0 + sx*(yi+0))));
    ELL_3V_COPY(evec[4], evp        + 9*(1 + 2*(xi+0 + sx*(yi+0))));
    ELL_3V_COPY(grad[5], lctx->grad + 3*(1 + 2*(xi+1 + sx*(yi+0))));
    ELL_3V_COPY(evec[5], evp        + 9*(1 + 2*(xi+1 + sx*(yi+0))));
    ELL_3V_COPY(grad[6], lctx->grad + 3*(1 + 2*(xi+0 + sx*(yi+1))));
    ELL_3V_COPY(evec[6], evp        + 9*(1 + 2*(xi+0 + sx*(yi+1))));
    ELL_3V_COPY(grad[7], lctx->grad + 3*(1 + 2*(xi+1 + sx*(yi+1))));
    ELL_3V_COPY(evec[7], evp        + 9*(1 + 2*(xi+1 + sx*(yi+1))));
    for (iii=0; iii<12; iii++) {
      eflip[iii] = (lctx->eflip + 12*si)[iii];
    }
    /*
    ELL_3V_SCALE(evec[1], eflip[0], evec[1]);
    ELL_3V_SCALE(evec[2], eflip[1], evec[2]);
    ELL_3V_SCALE(evec[3], eflip[0]*eflip[2], evec[3]);
    ELL_3V_SCALE(evec[4], eflip[4], evec[4]);
    ELL_3V_SCALE(evec[5], eflip[4]*eflip[8], evec[5]);
    ELL_3V_SCALE(evec[6], eflip[4]*eflip[9], evec[6]);
    ELL_3V_SCALE(evec[7], eflip[4]*eflip[8]*eflip[10], evec[7]);
    */
    for (iii=0; iii<8; iii++) {
      vval[iii] = ELL_3V_DOT(evec[iii], grad[iii]);
    }
  }

#undef MINMAX

  return;
}

#endif

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
  return 0;
}

#if 0
static int
slabInit(seekContext *sctx, baggage *bag) {
  /* char me[]="slabInit", err[BIFF_STRLEN]; */

  /* initialize per-slice stuff */
  sx = sctx->sx;
  sy = sctx->sy;
  sz = sctx->sz;
  for (yi=0; yi<sy; yi++) {
    for (xi=0; xi<sx; xi++) {
      si = xi + sx*yi;
      spi = (xi+1) + (sx+2)*(yi+1);
      sctx->vidx[0 + 5*si] = -1;
      sctx->vidx[1 + 5*si] = -1;
      sctx->vidx[2 + 5*si] = -1;
      sctx->vidx[3 + 5*si] = -1;
      sctx->vidx[4 + 5*si] = -1;
      if (seekTypeIsocontour == sctx->featureType) {
        sctx->val[0 + 4*spi] = AIR_NAN;
        sctx->val[1 + 4*spi] =
          _seek3DIsocontourValue(sctx, xi, yi, 0) - sctx->isovalue;
        sctx->val[2 + 4*spi] = sctx->val[1 + 4*spi];
        sctx->val[3 + 4*spi] = 
          _seek3DIsocontourValue(sctx, xi, yi, 1) - sctx->isovalue;
      }
      if (seekTypeRidgeSurface == sctx->featureType
          || seekTypeValleySurface == sctx->featureType) {
        gageProbe(sctx->gctx, xi, yi, 0);
        ELL_3V_COPY(sctx->grad + 3*(1 + 2*si), sctx->gradAns);
        ELL_3V_COPY(sctx->eval + 3*(1 + 2*si), sctx->evalAns);
        sctx->mode[0 + 2*si] = 0; /* need to set something for voxel 
                                     skipping based on extremal mode */
        sctx->mode[1 + 2*si] = mode(sctx->eval + 3*(1 + 2*si));
        ELL_3M_COPY(sctx->evec + 9*(1 + 2*si), sctx->evecAns);
        if ((sctx->evec + 6 + 9*(1 + 2*si))[2] < 0) {
          ELL_3V_SCALE((sctx->evec + 6 + 9*(1 + 2*si)), -1,
                       (sctx->evec + 6 + 9*(1 + 2*si)));
        }
      }
    }
    if (seekTypeIsocontour == sctx->featureType) {
      ELL_4V_COPY(sctx->val + 4*(0    + (sx+2)*(yi+1)),
                  sctx->val + 4*(1    + (sx+2)*(yi+1)));
      ELL_4V_COPY(sctx->val + 4*(sx+1 + (sx+2)*(yi+1)),
                  sctx->val + 4*(sx   + (sx+2)*(yi+1)));
    }
  }
  if (seekTypeIsocontour == sctx->featureType) {
    for (xi=0; xi<sx+2; xi++) {
      ELL_4V_COPY(sctx->val + 4*(xi + (sx+2)*0),
                  sctx->val + 4*(xi + (sx+2)*1));
      ELL_4V_COPY(sctx->val + 4*(xi + (sx+2)*(sy+1)),
                  sctx->val + 4*(xi + (sx+2)*sy));
    }
  }

  evidx = (seekTypeRidgeSurface == sctx->featureType
           ? 2
           : 0);

  if (seekTypeRidgeSurface == sctx->featureType
      || seekTypeValleySurface == sctx->featureType) {
    if (eflipSet(sctx, evidx, -1, AIR_FALSE)) {
      sprintf(err, "%s: on initial slab", me);
      biffAdd(SEEK, err); return 1;
    }
  }

  return 0;
}

#endif

static void
shuffleProbe(seekContext *sctx, baggage *bag, unsigned int zi) {

#if 0
    /* ----------- begin shuffle up and probe */
    for (yi=0; yi<sy; yi++) {
      for (xi=0; xi<sx; xi++) {
        si = xi + sx*yi;
        spi = (xi+1) + (sx+2)*(yi+1);
        sctx->vidx[0 + 5*si] = sctx->vidx[3 + 5*si];
        sctx->vidx[1 + 5*si] = sctx->vidx[4 + 5*si];
        sctx->vidx[2 + 5*si] = -1;
        sctx->vidx[3 + 5*si] = -1;
        sctx->vidx[4 + 5*si] = -1;
        if (seekTypeIsocontour == sctx->featureType) {
          sctx->val[0 + 4*spi] = sctx->val[1 + 4*spi];
          sctx->val[1 + 4*spi] = sctx->val[2 + 4*spi];
          sctx->val[2 + 4*spi] = sctx->val[3 + 4*spi];
          sctx->val[3 + 4*spi] = 
            _seek3DIsocontourValue(sctx, xi, yi, zpi) - sctx->isovalue;
        }
        if (seekTypeRidgeSurface == sctx->featureType
            || seekTypeValleySurface == sctx->featureType) {
          ELL_3V_COPY(sctx->grad + 3*(0 + 2*si), sctx->grad + 3*(1 + 2*si));
          ELL_3V_COPY(sctx->eval + 3*(0 + 2*si), sctx->eval + 3*(1 + 2*si));
          sctx->mode[0 + 2*si] = sctx->mode[1 + 2*si];
          ELL_3M_COPY(sctx->evec + 9*(0 + 2*si), sctx->evec + 9*(1 + 2*si));
          (sctx->eflip + 12*si)[ 0] = (sctx->eflip + 12*si)[ 8];
          (sctx->eflip + 12*si)[ 1] = (sctx->eflip + 12*si)[ 9];
          (sctx->eflip + 12*si)[ 2] = (sctx->eflip + 12*si)[10];
          (sctx->eflip + 12*si)[ 3] = (sctx->eflip + 12*si)[11];
          gageProbe(sctx->gctx, xi, yi, zi+1);
          ELL_3V_COPY(sctx->grad + 3*(1 + 2*si), sctx->gradAns);
          ELL_3V_COPY(sctx->eval + 3*(1 + 2*si), sctx->evalAns);
          sctx->mode[1 + 2*si] = mode(sctx->eval + 3*(1 + 2*si));
          ELL_3M_COPY(sctx->evec + 9*(1 + 2*si), sctx->evecAns);
          if ((sctx->evec + 6 + 9*(1 + 2*si))[2] < 0) {
            ELL_3V_SCALE((sctx->evec + 6 + 9*(1 + 2*si)), -1,
                         (sctx->evec + 6 + 9*(1 + 2*si)));
          }
        }
      }
      if (seekTypeIsocontour == sctx->featureType) {
        ELL_4V_COPY(sctx->val + 4*(0    + (sx+2)*(yi+1)),
                    sctx->val + 4*(1    + (sx+2)*(yi+1)));
        ELL_4V_COPY(sctx->val + 4*(sx+1 + (sx+2)*(yi+1)),
                    sctx->val + 4*(sx   + (sx+2)*(yi+1)));
      }
    }
    if (seekTypeIsocontour == sctx->featureType) {
      for (xi=0; xi<sx+2; xi++) {
        ELL_4V_COPY(sctx->val + 4*(xi + (sx+2)*0),
                    sctx->val + 4*(xi + (sx+2)*1));
        ELL_4V_COPY(sctx->val + 4*(xi + (sx+2)*(sy+1)),
                    sctx->val + 4*(xi + (sx+2)*sy));
      }
    }
    if (seekTypeRidgeSurface == sctx->featureType
        || seekTypeValleySurface == sctx->featureType) {
      if (eflipSet(sctx, evidx, zi, AIR_TRUE)) {
        sprintf(err, "%s: on zi=%u", me, zi);
        biffAdd(SEEK, err); return 1;
      }
    }
    /* ----------- end shuffle up and probe */
#endif
  return;
}

static void
triangulate(seekContext *sctx, baggage *bag, limnPolyData *lpld,
            unsigned int zi) {

#if 0

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

    /* ----------- begin triangulate slice */
    for (yi=0; yi<sy-1; yi++) {
      double vval[8], vgrad[8][3], vert[3], tvertA[4], tvertB[4], ww;
      unsigned char vcase;
      int ti, vi, ei, vi0, vi1, ecase, *tcase;
      unsigned int vii[3];
      for (xi=0; xi<sx-1; xi++) {
        si = xi + sx*yi;
        spi = (xi+1) + (sx+2)*(yi+1);
        if (seekTypeIsocontour == sctx->featureType) {
          /* learn voxel values */
          /*                     X   Y                 Z */
          vval[0] = sctx->val[4*(0 + 0*(sx+2) + spi) + 1];
          vval[1] = sctx->val[4*(1 + 0*(sx+2) + spi) + 1];
          vval[2] = sctx->val[4*(0 + 1*(sx+2) + spi) + 1];
          vval[3] = sctx->val[4*(1 + 1*(sx+2) + spi) + 1];
          vval[4] = sctx->val[4*(0 + 0*(sx+2) + spi) + 2];
          vval[5] = sctx->val[4*(1 + 0*(sx+2) + spi) + 2];
          vval[6] = sctx->val[4*(0 + 1*(sx+2) + spi) + 2];
          vval[7] = sctx->val[4*(1 + 1*(sx+2) + spi) + 2];
        }
        if (seekTypeRidgeSurface == sctx->featureType
            || seekTypeValleySurface == sctx->featureType) {
          vvalSurfSet(sctx, vval, evidx, xi, yi);
        }
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
          _seek3DVoxelGrads(vgrad, sctx->val, sx, spi);
        }
        sctx->voxNum++;
        ecase = _seek3DEdge[vcase];
        /* create new vertices as needed */
        for (ei=0; ei<12; ei++) {
          if ((ecase & (1 << ei))
              && -1 == sctx->vidx[vidx[ei] + 5*si]) {
            int ovi;
            double tvec[3], grad[3], tlen;
            /* this edge is needed for triangulation,
               and, we haven't already created a vertex for it */
            vi0 = e2v[ei][0];
            vi1 = e2v[ei][1];
            ww = vval[vi0]/(vval[vi0] - vval[vi1]);
            ELL_3V_LERP(vert, ww, vccoord[vi0], vccoord[vi1]);
            ELL_4V_SET(tvertA, vert[0] + xi, vert[1] + yi, vert[2] + zi, 1);
            ELL_4MV_MUL(tvertB, sctx->transform, tvertA);
            ELL_4V_HOMOG(tvertB, tvertB);
            ovi = sctx->vidx[vidx[ei] + 5*si] = airArrayLenIncr(xyzwArr, 1);
            ELL_4V_SET_TT(lpld->xyzw + 4*ovi, float,
                          tvertB[0], tvertB[1], tvertB[2], 1.0);
            if (sctx->normalsFind) {
              airArrayLenIncr(normArr, 1);
              if (sctx->normAns) {
                gageProbe(sctx->gctx, tvertA[0], tvertA[1], tvertA[2]);
                ELL_3V_SCALE_TT(lpld->norm + 3*ovi, float, -1, sctx->normAns);
                if (sctx->lowerInside) {
                  ELL_3V_SCALE(lpld->norm + 3*ovi, -1, lpld->norm + 3*ovi);
                }
              } else {
                ELL_3V_LERP(grad, ww, vgrad[vi0], vgrad[vi1]);
                ELL_3MV_MUL(tvec, sctx->normalTransform, grad);
                ELL_3V_NORM_TT(lpld->norm + 3*ovi, float, tvec, tlen);
              }
            }
            sctx->vertNum++;
            /*
            fprintf(stderr, "%s: vert %d (edge %d) of (%d,%d,%d) "
                    "at %g %g %g\n",
                    me, sctx->vidx[vidx[ei] + 5*si], ei, xi, yi, zi,
                    vert[0] + xi, vert[1] + yi, vert[2] + zi);
            */
          }
        }
        /* add triangles */
        ti = 0;
        tcase = _seek3DTriangle[vcase];
        while (-1 != tcase[0 + 3*ti]) {
          unsigned iii;
          ELL_3V_SET(vii,
                     sctx->vidx[vidx[tcase[0 + 3*ti]] + 5*si],
                     sctx->vidx[vidx[tcase[1 + 3*ti]] + 5*si],
                     sctx->vidx[vidx[tcase[2 + 3*ti]] + 5*si]);
          if (sctx->reverse) {
            int tmpi;
            tmpi = vii[1]; vii[1] = vii[2]; vii[2] = tmpi;
          }
          iii = airArrayLenIncr(indxArr, 3);
          ELL_3V_COPY(lpld->indx + iii, vii);
          lpld->icnt[0] += 3;
          sctx->faceNum++;
          ti++;
        }
      }
    } /* ----------- end triangulate slice */

#endif

  return;
}

static int
surfaceExtract(seekContext *sctx, limnPolyData *lpld) {
  char me[]="surfaceExtract", err[BIFF_STRLEN];
  int E, vidx[12];
  unsigned int zi, si, spi, minI, maxI, valI, evidx;
  baggage *bag;

  /* initialize output summary info */
  sctx->voxNum = 0;
  sctx->vertNum = 0;
  sctx->faceNum = 0;

  bag = baggageNew();
  if (outputInit(sctx, bag, lpld)) {
    sprintf(err, "%s: trouble", me);
    biffAdd(SEEK, err); return 1;
  }

  /* set up vidx */
  /*                X      Y */
  vidx[ 0] = 0 + 5*(0 + sctx->sx*0);
  vidx[ 1] = 1 + 5*(0 + sctx->sx*0);
  vidx[ 2] = 1 + 5*(1 + sctx->sx*0);
  vidx[ 3] = 0 + 5*(0 + sctx->sx*1);
  vidx[ 4] = 2 + 5*(0 + sctx->sx*0);
  vidx[ 5] = 2 + 5*(1 + sctx->sx*0);
  vidx[ 6] = 2 + 5*(0 + sctx->sx*1);
  vidx[ 7] = 2 + 5*(1 + sctx->sx*1);
  vidx[ 8] = 3 + 5*(0 + sctx->sx*0);
  vidx[ 9] = 4 + 5*(0 + sctx->sx*0);
  vidx[10] = 4 + 5*(1 + sctx->sx*0);
  vidx[11] = 3 + 5*(0 + sctx->sx*1);

  shuffleProbe(sctx, bag, 0);
  for (zi=1; zi<sctx->sz; zi++) {
    shuffleProbe(sctx, bag, zi);
    triangulate(sctx, bag, lpld, zi);
  }

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

  return 0;
}
