/*
  Teem: Tools to process and visualize scientific data and images
  Copyright (C) 2009--2023  University of Chicago
  Copyright (C) 2005--2008  Gordon Kindlmann
  Copyright (C) 1998--2004  University of Utah

  This library is free software; you can redistribute it and/or modify it under the terms
  of the GNU Lesser General Public License (LGPL) as published by the Free Software
  Foundation; either version 2.1 of the License, or (at your option) any later version.
  The terms of redistributing and/or modifying this software also include exceptions to
  the LGPL that facilitate static linking.

  This library is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
  PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License along with
  this library; if not, write to Free Software Foundation, Inc., 51 Franklin Street,
  Fifth Floor, Boston, MA 02110-1301 USA
*/

#include "moss.h"
#include "privateMoss.h"

/*

 0  1  2
 3  4  5
 6  7  8

 a  c  tx
 b  d  ty
 0  0  1

 0  1  2
 3  4  5

*/

void
mossMatPrint(FILE *f, const double *mat) {

  fprintf(f, "% 15.7f % 15.7f % 15.7f\n", (float)mat[0], (float)mat[1], (float)mat[2]);
  fprintf(f, "% 15.7f % 15.7f % 15.7f\n", (float)mat[3], (float)mat[4], (float)mat[5]);
}

double * /* Biff: nope */
mossMatRightMultiply(double *_mat, const double *_x) {
  double mat[9], x[9];

  MOSS_MAT_6TO9(x, _x);
  MOSS_MAT_6TO9(mat, _mat);
  ell_3m_pre_mul_d(mat, x);
  MOSS_MAT_9TO6(_mat, mat);
  return _mat;
}

double * /* Biff: nope */
mossMatLeftMultiply(double *_mat, const double *_x) {
  double mat[9], x[9];

  MOSS_MAT_6TO9(x, _x);
  MOSS_MAT_6TO9(mat, _mat);
  ell_3m_post_mul_d(mat, x);
  MOSS_MAT_9TO6(_mat, mat);
  return _mat;
}

double * /* Biff: nope */
mossMatInvert(double *inv, const double *mat) {
  double inv9[9], mat9[9];

  MOSS_MAT_6TO9(mat9, mat);
  ell_3m_inv_d(inv9, mat9);
  MOSS_MAT_9TO6(inv, inv9);
  return inv;
}

double * /* Biff: nope */
mossMatIdentitySet(double *mat) {

  MOSS_MAT_SET(mat, 1, 0, 0, 0, 1, 0);
  return mat;
}

double * /* Biff: nope */
mossMatTranslateSet(double *mat, double tx, double ty) {

  MOSS_MAT_SET(mat, 1, 0, tx, 0, 1, ty);
  return mat;
}

double * /* Biff: nope */
mossMatRotateSet(double *mat, double angle) {

  angle *= AIR_PI / 180.0;
  MOSS_MAT_SET(mat, cos(angle), -sin(angle), 0, sin(angle), cos(angle), 0);
  return mat;
}

double * /* Biff: nope */
mossMatFlipSet(double *mat, double angle) {
  double rot[6], flip[6];

  MOSS_MAT_SET(flip, -1, 0, 0, 0, 1, 0);
  mossMatIdentitySet(mat);
  mossMatLeftMultiply(mat, mossMatRotateSet(rot, -angle));
  mossMatLeftMultiply(mat, flip);
  mossMatLeftMultiply(mat, mossMatRotateSet(rot, angle));
  return mat;
}

double * /* Biff: nope */
mossMatShearSet(double *mat, double angleFixed, double amount) {
  double rot[6], shear[6];

  MOSS_MAT_SET(shear, 1, amount, 0, 0, 1, 0);
  mossMatIdentitySet(mat);
  mossMatLeftMultiply(mat, mossMatRotateSet(rot, -angleFixed));
  mossMatLeftMultiply(mat, shear);
  mossMatLeftMultiply(mat, mossMatRotateSet(rot, angleFixed));
  return mat;
}

double * /* Biff: nope */
mossMatScaleSet(double *mat, double sx, double sy) {

  MOSS_MAT_SET(mat, sx, 0, 0, 0, sy, 0);
  return mat;
}

void
mossMatApply(double *ox, double *oy, const double *mat, double ix, double iy) {

  *ox = mat[0] * ix + mat[1] * iy + mat[2];
  *oy = mat[3] * ix + mat[4] * iy + mat[5];
}

int /* Biff: 1 */
mossLinearTransform(Nrrd *nout, const Nrrd *nin, int boundary, const double *bg,
                    const double *mat, mossSampler *msp, double xMin, double xMax,
                    double yMin, double yMax, int xSize, int ySize) {
  static const char me[] = "mossLinearTransform";
  int xi, yi, ax0, xCent, yCent;
  unsigned int ci, nchan;
  double *val, (*ins)(void *v, size_t I, double f), (*clamp)(double val);
  double inv[6], xInPos, xOutPos, yInPos, yOutPos;

  if (!(nout && nin && mat && msp && !mossImageCheck(nin))) {
    biffAddf(MOSS, "%s: got NULL pointer or bad image", me);
    return 1;
  }
  msp->verbose = (msp->verbPixel[0] >= 0 && msp->verbPixel[1] >= 0);
  if (mossSamplerImageSet(msp, nin, boundary, bg) || mossSamplerUpdate(msp)) {
    biffAddf(MOSS, "%s: trouble with sampler", me);
    return 1;
  }
  msp->verbose = 0;
  if (!(xMin != xMax && yMin != yMax && xSize > 1 && ySize > 1)) {
    biffAddf(MOSS, "%s: bad args: {x,y}Min == {x,y}Max or {x,y}Size <= 1", me);
    return 1;
  }
  ax0 = MOSS_AXIS0(nin);
  if (!(AIR_EXISTS(nin->axis[ax0 + 0].min) && AIR_EXISTS(nin->axis[ax0 + 0].max)
        && AIR_EXISTS(nin->axis[ax0 + 1].min) && AIR_EXISTS(nin->axis[ax0 + 1].max))) {
    biffAddf(MOSS, "%s: input axis min,max not set on axes %d and %d", me, ax0 + 0,
             ax0 + 1);
    return 1;
  }

  nchan = MOSS_CHAN_NUM(nin);
  if (mossImageAlloc(nout, nin->type, xSize, ySize, nchan)) {
    biffAddf(MOSS, "%s: ", me);
    return 1;
  }
  val = AIR_CALLOC(nchan, double);
  if (nrrdCenterUnknown == nout->axis[ax0 + 0].center)
    nout->axis[ax0 + 0].center = _mossCenter(nin->axis[ax0 + 0].center);
  xCent = nout->axis[ax0 + 0].center;
  if (nrrdCenterUnknown == nout->axis[ax0 + 1].center)
    nout->axis[ax0 + 1].center = _mossCenter(nin->axis[ax0 + 1].center);
  yCent = nout->axis[ax0 + 1].center;
  nout->axis[ax0 + 0].min = xMin;
  nout->axis[ax0 + 0].max = xMax;
  nout->axis[ax0 + 1].min = yMin;
  nout->axis[ax0 + 1].max = yMax;
  ins = nrrdDInsert[nin->type];
  clamp = nrrdDClamp[nin->type];

  if (mossSamplerSample(val, msp, 0, 0)) {
    biffAddf(MOSS, "%s: trouble in sampler", me);
    free(val);
    return 1;
  }

  mossMatInvert(inv, mat);
  for (yi = 0; yi < ySize; yi++) {
    yOutPos = NRRD_POS(yCent, yMin, yMax, ySize, yi);
    for (xi = 0; xi < xSize; xi++) {
      xOutPos = NRRD_POS(xCent, xMin, xMax, xSize, xi);
      msp->verbose = (xi == msp->verbPixel[0] && yi == msp->verbPixel[1]);
      mossMatApply(&xInPos, &yInPos, inv, xOutPos, yOutPos);
      xInPos = NRRD_IDX(xCent, nin->axis[ax0 + 0].min, nin->axis[ax0 + 0].max,
                        nin->axis[ax0 + 0].size, xInPos);
      yInPos = NRRD_IDX(yCent, nin->axis[ax0 + 1].min, nin->axis[ax0 + 1].max,
                        nin->axis[ax0 + 1].size, yInPos);
      mossSamplerSample(val, msp, xInPos, yInPos);
      for (ci = 0; ci < nchan; ci++) {
        ins(nout->data, ci + nchan * (xi + xSize * yi), clamp(val[ci]));
      }
    }
  }

  free(val);
  return 0;
}

int /* Biff: 1 */
mossFourPointTransform(Nrrd *nout, const Nrrd *nin, int boundary, const double *bg,
                       const double xyc[8], mossSampler *msp, int xSize, int ySize) {
  static const char me[] = "mossFourPointTransform";
  int xi, yi;
  unsigned int ci, nchan;
  double *val, (*ins)(void *v, size_t I, double f), (*clamp)(double val), PM[8];

  if (!(nout && nin && msp && !mossImageCheck(nin))) {
    biffAddf(MOSS, "%s: got NULL pointer or bad image", me);
    return 1;
  }
  /* HEY SET UP verbPixel for this HEY HEY HEY */
  msp->verbose = (msp->verbPixel[0] >= 0 && msp->verbPixel[1] >= 0);
  if (mossSamplerImageSet(msp, nin, boundary, bg) || mossSamplerUpdate(msp)) {
    biffAddf(MOSS, "%s: trouble with sampler", me);
    return 1;
  }
  msp->verbose = 0;

  nchan = MOSS_CHAN_NUM(nin);
  if (mossImageAlloc(nout, nin->type, xSize, ySize, nchan)) {
    biffAddf(MOSS, "%s: ", me);
    return 1;
  }
  val = AIR_CALLOC(nchan, double);
  if (mossSamplerSample(val, msp, 0, 0)) {
    biffAddf(MOSS, "%s: trouble using sampler", me);
    free(val);
    return 1;
  }

  {
    double x0 = xyc[0], y0 = xyc[1];
    double x1 = xyc[2], y1 = xyc[3];
    double x2 = xyc[4], y2 = xyc[5];
    double x3 = xyc[6], y3 = xyc[7];
    /* expressions for matrix entries from GLK using Mathematica */
    PM[0] = -((-(x1 * x2 * y0) + x1 * x3 * y0 + x0 * x2 * y1 - x0 * x3 * y1
               + x0 * x3 * y2 - x1 * x3 * y2 - x0 * x2 * y3 + x1 * x2 * y3)
              / (x2 * y1 - x3 * y1 - x1 * y2 + x3 * y2 + x1 * y3 - x2 * y3));
    PM[1] = -((x1 * x2 * y0 - x2 * x3 * y0 - x0 * x3 * y1 + x2 * x3 * y1 - x0 * x1 * y2
               + x0 * x3 * y2 + x0 * x1 * y3 - x1 * x2 * y3)
              / (x2 * y1 - x3 * y1 - x1 * y2 + x3 * y2 + x1 * y3 - x2 * y3));
    PM[2] = x0;
    PM[3] = -((-(x1 * y0 * y2) + x3 * y0 * y2 + x0 * y1 * y2 - x3 * y1 * y2
               + x1 * y0 * y3 - x2 * y0 * y3 - x0 * y1 * y3 + x2 * y1 * y3)
              / (x2 * y1 - x3 * y1 - x1 * y2 + x3 * y2 + x1 * y3 - x2 * y3));
    PM[4] = -((x2 * y0 * y1 - x3 * y0 * y1 - x0 * y1 * y2 + x3 * y1 * y2 + x1 * y0 * y3
               - x2 * y0 * y3 + x0 * y2 * y3 - x1 * y2 * y3)
              / (x2 * y1 - x3 * y1 - x1 * y2 + x3 * y2 + x1 * y3 - x2 * y3));
    PM[5] = y0;
    PM[6] = -((-(x2 * y0) + x3 * y0 + x2 * y1 - x3 * y1 + x0 * y2 - x1 * y2 - x0 * y3
               + x1 * y3)
              / (x2 * y1 - x3 * y1 - x1 * y2 + x3 * y2 + x1 * y3 - x2 * y3));
    PM[7]
      = -((x1 * y0 - x3 * y0 - x0 * y1 + x2 * y1 - x1 * y2 + x3 * y2 + x0 * y3 - x2 * y3)
          / (x2 * y1 - x3 * y1 - x1 * y2 + x3 * y2 + x1 * y3 - x2 * y3));
  }

  ins = nrrdDInsert[nin->type];
  clamp = nrrdDClamp[nin->type];
  for (yi = 0; yi < ySize; yi++) {
    /* or should it be cell-centered? */
    double yr = NRRD_NODE_POS(0.0, 1.0, ySize, yi);
    for (xi = 0; xi < xSize; xi++) {
      double xr = NRRD_NODE_POS(0.0, 1.0, xSize, xi);
      double xx = PM[0] * xr + PM[1] * yr + PM[2];
      double yy = PM[3] * xr + PM[4] * yr + PM[5];
      double ww = PM[6] * xr + PM[7] * yr + 1;
      msp->verbose = (xi == msp->verbPixel[0] && yi == msp->verbPixel[1]);
      if (msp->verbose) {
        printf("%s[%d,%d] --> rect x,y = %g,%g --> x,y,z = %g %g %g --> %g %g\n", me, xi,
               yi, xr, yr, xx, yy, ww, xx / ww, yy / ww);
      }
      mossSamplerSample(val, msp, xx / ww, yy / ww);
      for (ci = 0; ci < nchan; ci++) {
        ins(nout->data, ci + nchan * (xi + xSize * yi), clamp(val[ci]));
      }
    }
  }

  free(val);
  return 0;
}
