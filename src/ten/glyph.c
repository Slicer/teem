/*
  The contents of this file are subject to the University of Utah Public
  License (the "License"); you may not use this file except in
  compliance with the License.
  
  Software distributed under the License is distributed on an "AS IS"
  basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See
  the License for the specific language governing rights and limitations
  under the License.

  The Original Source Code is "teem", released March 23, 2001.
  
  The Original Source Code was developed by the University of Utah.
  Portions created by UNIVERSITY are Copyright (C) 2001, 1998 University
  of Utah. All Rights Reserved.
*/


#include "ten.h"

int
tenGlyphGen(limnObj *obj, Nrrd *nin) {
  char me[]="tenGlyphGen", err[128];
  int idx, sx, sy, sz, xi, yi, zi, ri;
  float *data, x, y, z, tt[16], eval[3], evec[9],
    sc[3];
  
  if (!(obj && nin)) {
    sprintf(err, BIFF_NULL, me); biffAdd(TEN, err); return 1;
  }
  if (!tenValidTensor(nin, nrrdTypeFloat, AIR_TRUE)) {
    sprintf(err, "%s: didn't get a tensor volume", me);
    biffAdd(TEN, err); return 1;
  }
  
  data = nin->data;
  sx = nin->size[1];
  sy = nin->size[2];
  sz = nin->size[3];
  idx = 0;
  for (zi=0; zi<sz; zi++) {
    z = AIR_AFFINE(0, zi, sz-1, -(sz-1)/2.0, (sz-1)/2.0);
    for (yi=0; yi<sy; yi++) {
      y = AIR_AFFINE(0, yi, sy-1, -(sy-1)/2.0, (sy-1)/2.0);
      for (xi=0; xi<sx; xi++) {
	x = AIR_AFFINE(0, xi, sx-1, -(sx-1)/2.0, (sx-1)/2.0);

	tenEigensolve(eval, evec, data+7*idx);
	ELL_3V_SCALE(sc, eval, 1.0);
	ri = limnObjCubeAdd(obj, 2);
	limnObjPartTransform(obj, ri, ELL_4M_SET_SCALE(tt,sc[0],sc[1],sc[2]));
	limnObjPartTransform(obj, ri, ELL_4M_SET_TRANSLATE(tt, x, y, z));

	idx++;
      }
    }
  }

  return 0;
}
