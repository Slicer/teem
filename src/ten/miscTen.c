/*
  teem: Gordon Kindlmann's research software
  Copyright (C) 2002, 2001, 2000, 1999, 1998 University of Utah

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


#include "ten.h"
#include "tenPrivate.h"

#define SQR(i) ((i)*(i))

short
tenEvqOne(float vec[3], float scl) {
  float tmp, L1;
  int mi, bins, base, vi, ui;
  short ret;

  ELL_3V_NORM(vec, vec, tmp);
  L1 = AIR_ABS(vec[0]) + AIR_ABS(vec[1]) + AIR_ABS(vec[2]);
  ELL_3V_SCALE(vec, 1/L1, vec);
  scl = AIR_CLAMP(0.0, scl, 1.0);
  scl = pow(scl, 0.75);
  AIR_INDEX(0.0, scl, 1.0, 6, mi);
  if (mi) {
    switch (mi) {
    case 1: bins = 16; base = 1;                                 break;
    case 2: bins = 32; base = 1+SQR(16);                         break;
    case 3: bins = 48; base = 1+SQR(16)+SQR(32);                 break;
    case 4: bins = 64; base = 1+SQR(16)+SQR(32)+SQR(48);         break;
    case 5: bins = 80; base = 1+SQR(16)+SQR(32)+SQR(48)+SQR(64); break;
    }
    AIR_INDEX(-1, vec[0]+vec[1], 1, bins, vi);
    AIR_INDEX(-1, vec[0]-vec[1], 1, bins, ui);
    ret = vi*bins + ui + base;
  }
  else {
    ret = 0;
  }
  return ret;
}

int
tenEvqVolume(Nrrd *nout, Nrrd *nin, int which, int aniso) {
  char me[]="tenEvqVolume", err[AIR_STRLEN_MED];
  int sx, sy, sz, map[3];
  short *qdata;
  float *tdata, eval[3], evec[9], c[TEN_ANISO_MAX+1];
  size_t N, I;

  if (!(nout && nin)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(TEN, err); return 1;
  }
  if (!(AIR_IN_CL(0, which, 2))) {
    sprintf(err, "%s: eigenvector index %d not in range [0..2]", me, which);
    biffAdd(TEN, err); return 1;
  }
  if (!airEnumValValid(tenAniso, aniso)) {
    sprintf(err, "%s: anisotropy metric %d not valid", me, aniso);
    biffAdd(TEN, err); return 1;
  }
  if (tenTensorCheck(nin, nrrdTypeFloat, AIR_TRUE)) {
    sprintf(err, "%s: didn't get a valid DT volume", me);
    biffAdd(TEN, err); return 1;
  }
  sx = nin->axis[1].size;
  sy = nin->axis[2].size;
  sz = nin->axis[3].size;
  if (nrrdMaybeAlloc(nout, nrrdTypeShort, 3, sx, sy, sz)) {
    sprintf(err, "%s: can't allocate output", me);
    biffMove(TEN, err, NRRD); return 1;
  }
  N = sx*sy*sz;
  tdata = nin->data;
  qdata = nout->data;
  for (I=0; I<N; I++) {
    tenEigensolve(eval, evec, tdata);
    tenAnisoCalc(c, eval);
    qdata[I] = tenEvqOne(evec+ 3*which, c[aniso]);
    tdata += 7;
  }
  map[0] = 1;
  map[1] = 2;
  map[2] = 3;
  if (nrrdAxesCopy(nout, nin, map, NRRD_AXESINFO_NONE)) {
    sprintf(err, "%s: trouble", me);
    biffMove(TEN, err, NRRD); return 1;
  }
  
  return 0;
}
