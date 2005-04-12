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

#include "push.h"
#include "privatePush.h"

/* returns -1 if position is outside simulation domain */
int
_pushBinFind(pushContext *pctx, push_t *pos) {
  push_t min, max;
  int be, xi, yi, zi, bi;

  if (pctx->singleBin) {
    bi = 0;
  } else {
    min = -1.0 - pctx->margin;
    max = 1.0 + pctx->margin;
    be = pctx->binsEdge;
    if (AIR_IN_CL(min, pos[0], max)
        && AIR_IN_CL(min, pos[1], max)
        && AIR_IN_CL(min, pos[2], max)) {
      AIR_INDEX(min, pos[0], max, be, xi);
      AIR_INDEX(min, pos[1], max, be, yi);
      if (2 == pctx->dimIn) {
        zi = 0;
      } else {
        AIR_INDEX(min, pos[2], max, be, zi);
      }
      bi = xi + be*(yi + be*zi);
    } else {
      bi = -1;
    }
  }

  return bi;
}

void
_pushBinPointAdd(pushContext *pctx, int bi, int pi) {
  int pii;

  pii = airArrayIncrLen(pctx->pidxArr[bi], 1);
  pctx->pidx[bi][pii] = pi;
  return;
}

void
_pushBinPointRemove(pushContext *pctx, int bi, int losePii) {
  airArray *pidxArr;
  int *pidx, npi, pii;

  pidx = pctx->pidx[bi];
  pidxArr = pctx->pidxArr[bi];

  /*
  fprintf(stderr, "______________ bi=%d, losePii=%d\n", bi, losePii);
  for (pii=0; pii<pidxArr->len; pii++) {
    fprintf(stderr, " %d", pidx[pii]);
  }
  fprintf(stderr, "\n");
  */
  
  npi = pidxArr->len;
  for (pii=losePii; pii<npi-1; pii++) {
    pidx[pii] = pidx[pii+1];
  }
  airArrayIncrLen(pidxArr, -1);

  /*
  for (pii=0; pii<pidxArr->len; pii++) {
    fprintf(stderr, " %d", pidx[pii]);
  }
  fprintf(stderr, "\n");
  fprintf(stderr, "^^^^^^^^^^^^^^\n");
  */

  return;
}

/* does no checking for being out of bounds */
void
_pushBinPointsAllAdd(pushContext *pctx) {
  int bi, pi, np;
  push_t *attr, *pos;

  np = pctx->nPointAttr->axis[1].size;
  attr = (push_t*)pctx->nPointAttr->data;
  for (pi=0; pi<np; pi++) {
    pos = attr + PUSH_POS + PUSH_ATTR_LEN*pi;
    bi = _pushBinFind(pctx, pos);
    _pushBinPointAdd(pctx, bi, pi);
  }
  return;
}

int
_pushBinPointsRebin(pushContext *pctx) {
  char me[]="_pushBinPointsRebin" /* , err[AIR_STRLEN_MED] */;
  airArray *pidxArr;
  int oldbi, newbi, pi, pii;
  push_t *attr, *pos;

  if (!pctx->singleBin) {
    attr = (push_t*)pctx->nPointAttr->data;
    for (oldbi=0; oldbi<pctx->numBin; oldbi++) {
      pidxArr = pctx->pidxArr[oldbi];
      for (pii=0; pii<pidxArr->len; /* nope! */) {
        pi = pctx->pidx[oldbi][pii];
        pos = attr + PUSH_POS + PUSH_ATTR_LEN*pi;
        newbi = _pushBinFind(pctx, pos);
        /*
        if (-1 == newbi) {
          sprintf(err, "%s: point %d pos (%g,%g,%g) outside domain", me,
                  pi, pos[0], pos[1], pos[2]);
          biffAdd(PUSH, err); return 1;
        }
        */
        if (-1 == newbi) {
          /* bad point! I kill you now */
          _pushBinPointRemove(pctx, oldbi, pii);
          fprintf(stderr, "%s: killing point %d at (%g,%g,%g)\n", me,
                  pi, pos[0], pos[1], pos[2]);
          ELL_3V_SET(pos, AIR_NAN, AIR_NAN, AIR_NAN);
        } else {
          if (oldbi != newbi) {
            _pushBinPointRemove(pctx, oldbi, pii);
            _pushBinPointAdd(pctx, newbi, pi);
            /* don't increment pii; the next point index is now at pii */
          } else {
            /* this point is already in the right bin, move to next */
            pii++;
          }
        }
      }
    }
  }

  return 0;
}

int
_pushBinNeighborhoodFind(pushContext *pctx, int *nei, int bin, int dimIn) {
  int numNei, be, tmp, xx, yy, zz, xi, yi, zi,
    xmin, xmax, ymin, ymax, zmin, zmax;

  numNei = 0;
  if (pctx->singleBin) {
    nei[numNei++] = 0;
  } else {
    be = pctx->binsEdge;
    tmp = bin;
    xi = tmp % be;
    tmp = (tmp-xi)/be;
    yi = tmp % be;
    xmin = AIR_MAX(0, xi-1);
    xmax = AIR_MIN(xi+1, be-1);
    ymin = AIR_MAX(0, yi-1);
    ymax = AIR_MIN(yi+1, be-1);
    if (2 == pctx->dimIn) {
      zmin = zmax = 0;
    } else {
      zi = (tmp-yi)/be;
      zmin = AIR_MAX(0, zi-1);
      zmax = AIR_MIN(zi+1, be-1);
    }
    for (zz=zmin; zz<=zmax; zz++) {
      for (yy=ymin; yy<=ymax; yy++) {
        for (xx=xmin; xx<=xmax; xx++) {
          nei[numNei++] = xx + be*(yy + be*zz);
        }
      }
    }
  }
  
  return numNei;
}

