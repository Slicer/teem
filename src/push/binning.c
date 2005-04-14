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

/* returns NULL if position is outside simulation domain */
pushBin *
_pushBinLocate(pushContext *pctx, pushThing *thing) {
  push_t min, max, *pos;
  int be, xi, yi, zi;
  pushBin *ret;

  if (pctx->singleBin) {
    ret = pctx->bin[0];
  } else {
    pos = _pushThingPos(thing);
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
      ret = pctx->bin[xi + be*(yi + be*zi)];
    } else {
      ret = NULL;
    }
  }

  return ret;
}

/*
** HEY: doesn't check to make sure thing isn't already in bin
*/
void
_pushBinThingAdd(pushContext *pctx, pushBin *bin, pushThing *thing) {
  int thgI;

  thgI = airArrayIncrLen(bin->thingArr, 1);
  bin->thing[thgI] = thing;

  return;
}

/*
** remove the pointer to the thing from the bin, don't kill the thing
** because we don't know here if this is part of a killing, or a rebinning
*/
void
_pushBinThingRemove(pushContext *pctx, pushBin *bin, int loseIdx) {
  int thgI;

  for (thgI=loseIdx; thgI<bin->numThing-1; thgI++) {
    bin->thing[thgI] = bin->thing[thgI+1];
  }
  airArrayIncrLen(bin->thingArr, -1);
  
  return;
}

void
_pushBinNeighborSet(pushBin *bin, pushBin **nei, int num) {
  int neiI;

  bin->neighbor = airFree(bin->neighbor);
  bin->neighbor = (pushBin **)calloc(1+num, sizeof(pushBin *));
  for (neiI=0; neiI<num; neiI++) {
    bin->neighbor[neiI] = nei[neiI];
  }
  bin->neighbor[neiI] = NULL;
  return;
}

void
_pushBinAllNeighborSet(pushContext *pctx) {
  pushBin *nei[27];
  int numNei, be, xx, yy, zz, xi, yi, zi,
    xmin, xmax, ymin, ymax, zmin, zmax;

  if (pctx->singleBin) {
    numNei = 0;
    nei[numNei++] = pctx->bin[0];
    _pushBinNeighborSet(pctx->bin[0], nei, numNei);
  } else {
    be = pctx->binsEdge;
    for (zi=0; zi<(2 == pctx->dimIn ? 1 : be); zi++) {
      for (yi=0; yi<be; yi++) {
        for (xi=0; xi<be; xi++) {
          xmin = AIR_MAX(0, xi-1);
          xmax = AIR_MIN(xi+1, be-1);
          ymin = AIR_MAX(0, yi-1);
          ymax = AIR_MIN(yi+1, be-1);
          if (2 == pctx->dimIn) {
            zmin = zmax = 0;
          } else {
            zmin = AIR_MAX(0, zi-1);
            zmax = AIR_MIN(zi+1, be-1);
          }
          numNei = 0;
          for (zz=zmin; zz<=zmax; zz++) {
            for (yy=ymin; yy<=ymax; yy++) {
              for (xx=xmin; xx<=xmax; xx++) {
                nei[numNei++] = pctx->bin[xx + be*(yy + be*zz)];
              }
            }
          }
          _pushBinNeighborSet(pctx->bin[xi + be*(yi + be*zi)], nei, numNei);
        }
      }
    }
  }
  return;
}

int
pushBinAdd(pushContext *pctx, pushThing *thing) {
  char me[]="pushBinAdd";
  pushBin *bin;
  
  bin = _pushBinLocate(pctx, thing);
  if (!bin) {
    fprintf(stderr, "!%s: thing outside simulation domain\n", me);
  } else {
    _pushBinThingAdd(pctx, bin, thing);
  }
  return 0;
}

int
pushRebin(pushContext *pctx) {
  char me[]="pushRebin";
  int oldBinI, thgI;
  push_t *pos;
  pushBin *oldBin, *newBin;
  pushThing *thing;

  if (!pctx->singleBin) {
    for (oldBinI=0; oldBinI<pctx->numBin; oldBinI++) {
      oldBin = pctx->bin[oldBinI];
      for (thgI=0; thgI<oldBin->numThing; /* nope! */) {
        thing = oldBin->thing[thgI];
        newBin = _pushBinLocate(pctx, thing);
        if (NULL == newBin) {
          /* bad thing! I kill you now */
          pos = _pushThingPos(thing);
          fprintf(stderr, "%s: killing thing at (%g,%g,%g)\n", me,
                  pos[0], pos[1], pos[2]);
          _pushBinThingRemove(pctx, oldBin, thgI);
          pushThingNix(thing);
        } else {
          if (oldBin != newBin) {
            _pushBinThingRemove(pctx, oldBin, thgI);
            _pushBinThingAdd(pctx, newBin, thing);
            /* don't increment thgI; the *next* thing index is now thgI */
          } else {
            /* this thing is already in the right bin, move on */
            thgI++;
          }
        }
      } /* for thgI */
    }
  }

  return 0;
}

