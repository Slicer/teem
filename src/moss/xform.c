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

#include "moss.h"
#include "privateMoss.h"

/*

 0  3  6
 1  4  7
 2  5  8

 a  c  tx
 b  d  ty
 0  0  1

 0  2  4
 1  3  5

*/

void
mossMatPrint (FILE *f, double mat[6]) {

  fprintf(f, "% 15.7f % 15.7f % 15.7f\n", 
	  (float)mat[0], (float)mat[2], (float)mat[4]);
  fprintf(f, "% 15.7f % 15.7f % 15.7f\n", 
	  (float)mat[1], (float)mat[3], (float)mat[5]);
}

double *
mossMatPreMultiply (double _mat[6], double _x[6]) {
  double mat[9], x[9];
  
  MOSS_MAT_6TO9(x, _x);
  MOSS_MAT_6TO9(mat, _mat);
  ell3mPreMul_d(mat, x);
  MOSS_MAT_9TO6(_mat, mat);
  return mat;
}

double *
mossMatPostMultiply (double _mat[6], double _x[6]) {
  double mat[9], x[9];
  
  MOSS_MAT_6TO9(x, _x);
  MOSS_MAT_6TO9(mat, _mat);
  ell3mPostMul_d(mat, x);
  MOSS_MAT_9TO6(_mat, mat);
  return mat;
}

double *
mossMatInvert (double inv[6], double mat[6]) {
  double inv9[9], mat9[9];

  MOSS_MAT_6TO9(mat9, mat);
  ell3mInvert_d(inv9, mat9);
  MOSS_MAT_9TO6(inv, inv9);
  return inv;
}

double *
mossMatIdentitySet (double mat[6]) {

  MOSS_MAT_SET(mat, 1, 0, 0, 1, 0, 0);
  return mat;
}

double *
mossMatTranslateSet (double mat[6], double tx, double ty) {

  MOSS_MAT_SET(mat, 1, 0, 0, 1, tx, ty);
  return mat;
}

double *
mossMatRotateSet (double mat[6], double angle) {

  angle *= M_PI/180.0;
  MOSS_MAT_SET(mat, cos(angle), sin(angle), -sin(angle), cos(angle), 0, 0);
  return mat;
}

double *
mossMatFlipSet (double mat[6], double angle) {
  double rot[6], flip[6];

  MOSS_MAT_SET(flip, -1, 0, 0, 1, 0, 0);
  mossMatIdentitySet(mat);
  mossMatPostMultiply(mat, mossMatRotateSet(rot, -angle));
  mossMatPostMultiply(mat, flip);
  mossMatPostMultiply(mat, mossMatRotateSet(rot, angle));
  return mat;
}

double *
mossMatScaleSet (double mat[6], double sx, double sy) {

  MOSS_MAT_SET(mat, sx, 0, 0, sy, 0, 0);
  return mat;
}

int
mossLinearTransform (Nrrd *nout, Nrrd *nin, double mat[6], mossSampler *msp,
		     double xMin, double xMax,
		     double yMin, double yMax,
		     int xSize, int ySize) {
  char me[]="mossLinearTransform", err[AIR_STRLEN_MED];
  int ncol, xi, yi, xo, yo, ci, sx, sy, ax0;
  float val, (*lup)(void *v, size_t I),
    (*ins)(void *v, size_t I, float f), (*clamp)(float val);
  double inv[6];

  if (!(nout && nin && mat && msp && mossImageValid(nin))) {
    sprintf(err, "%s: got NULL pointer or bad image", me);
    biffAdd(MOSS, err); return 1;
  }
  if (mossSamplerImageSet(msp, nin) || mossSamplerUpdate(msp)) {
    sprintf(err, "%s: trouble with sampler", me);
    biffAdd(MOSS, err); return 1;
  }
  if (!( xMin != xMax && yMin != yMax && xSize > 1 && ySize > 1 )) {
    sprintf(err, "%s: bad args: {x,y}Min == {x,y}Max or {x,y}Size <= 1", me);
    biffAdd(MOSS, err); return 1;
  }
  ax0 = MOSS_AXIS0(nin);
  if (!( AIR_EXISTS(nin->axis[ax0+0].min)
	 && AIR_EXISTS(nin->axis[ax0+0].max)
	 && AIR_EXISTS(nin->axis[ax0+1].min)
	 && AIR_EXISTS(nin->axis[ax0+1].max) )) {
    sprintf(err, "%s: input axis min,max not set on axes %d and %d", me,
	    ax0+0, ax0+1); biffAdd(MOSS, err); return 1;
  }
  ncol = MOSS_NCOL(nin);
  if (mossImageAlloc(nout, nin->type, xSize, ySize, ncol)) {
    sprintf(err, "%s: ", me); biffAdd(MOSS, err); return 1;
  }
  nout->axis[ax0+0].center = _mossCenter(nin->axis[ax0+0].center);
  nout->axis[ax0+1].center = _mossCenter(nin->axis[ax0+1].center);
  nout->axis[ax0+0].min = xMin;
  nout->axis[ax0+0].max = xMax;
  nout->axis[ax0+1].min = yMin;
  nout->axis[ax0+1].max = yMax;
  lup = nrrdFLookup[nin->type];
  ins = nrrdFInsert[nin->type];
  clamp = nrrdFClamp[nin->type];
  
  sx = MOSS_SX(nin);
  sy = MOSS_SY(nin);
  mossMatInvert(inv, mat);
  for (yi=0; yi<sy; yi++) {
    for (xi=0; xi<sx; xi++) {
      xo = mat[0]*xi + mat[2]*yi + mat[4];
      yo = mat[1]*xi + mat[3]*yi + mat[5];
      if (AIR_INSIDE(0, xo, sx-1) && AIR_INSIDE(0, yo, sy-1)) {
	for (ci=0; ci<ncol; ci++) {
	  val = lup(nin->data, ci + ncol*(xi + sx*yi));
	  val = clamp(val);
	  ins(nout->data, ci + ncol*(xo + sx*yo), val);
	}
      }
    }
  }

  return 0;
}
