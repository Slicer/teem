/*
  teem: Gordon Kindlmann's research software
  Copyright (C) 2004, 2003, 2002, 2001, 2000, 1999, 1998 University of Utah

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

#ifndef MOSS_HAS_BEEN_INCLUDED
#define MOSS_HAS_BEEN_INCLUDED

#include <math.h>

#include <teem/air.h>
#include <teem/hest.h>
#include <teem/biff.h>
#include <teem/ell.h>
#include <teem/nrrd.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MOSS mossBiffKey

#define MOSS_NCOL(img) (3 == (img)->dim ? (img)->axis[0].size : 1)
#define MOSS_AXIS0(img) (3 == (img)->dim ? 1 : 0)
#define MOSS_SX(img) (3 == (img)->dim \
                      ? (img)->axis[1].size \
                      : (img)->axis[0].size )
#define MOSS_SY(img) (3 == (img)->dim \
                      ? (img)->axis[2].size \
                      : (img)->axis[1].size )

enum {
  mossFlagUnknown=-1,  /* -1: nobody knows */
  mossFlagImage,       /*  0: image being sampled */
  mossFlagKernel,      /*  1: kernel(s) used for sampling */
  mossFlagLast
};
#define MOSS_FLAG_NUM      2

typedef struct {
  Nrrd *image;                         /* the image to sample */
  const NrrdKernel *kernel;            /* which kernel to use on both axes */
  double kparm[NRRD_KERNEL_PARMS_NUM]; /* kernel arguments */
  float *ivc;                          /* intermediate value cache */
  double *xFslw, *yFslw;               /* filter sample locations->weights */
  int fdiam, ncol;                     /* filter diameter; ivc is allocated
                                          for (fdiam+1) x (fdiam+1) x ncol
                                          doubles, with that axis ordering */
  int *xIdx, *yIdx;                    /* arrays for x and y coordinates,
                                          both allocated for fdiam */
  float *bg;                           /* background color */
  int boundary;                        /* from nrrdBoundary* enum */
  int flag[MOSS_FLAG_NUM];             /* I'm a flag-waving struct */
} mossSampler;

/* defaultsMoss.c */
TEEM_API const char *mossBiffKey;
TEEM_API int mossDefBoundary;
TEEM_API int mossDefCenter;
TEEM_API int mossVerbose;

/* methodsMoss.c */
TEEM_API mossSampler *mossSamplerNew();
TEEM_API int mossSamplerFill(mossSampler *smplr, int fdiam, int ncol);
TEEM_API void mossSamplerEmpty(mossSampler *smplr);
TEEM_API mossSampler *mossSamplerNix(mossSampler *smplr);
TEEM_API int mossImageCheck(Nrrd *image);
TEEM_API int mossImageAlloc(Nrrd *image, int type, int sx, int sy, int ncol);

/* sampler.c */
TEEM_API int mossSamplerImageSet(mossSampler *smplr, Nrrd *image, float *bg);
TEEM_API int mossSamplerKernelSet(mossSampler *smplr,
                                  const NrrdKernel *kernel, double *kparm);
TEEM_API int mossSamplerUpdate(mossSampler *smplr);
TEEM_API int mossSamplerSample(float *val, mossSampler *smplr,
                               double xPos, double yPos);

/* hestMoss.c */
TEEM_API hestCB *mossHestTransform;
TEEM_API hestCB *mossHestOrigin;

/* xform.c */
TEEM_API void mossMatPrint(FILE *f, double *mat);
TEEM_API double *mossMatRightMultiply(double *mat, double *x);
TEEM_API double *mossMatLeftMultiply (double *mat, double *x);
TEEM_API double *mossMatInvert(double *inv, double *mat);
TEEM_API double *mossMatIdentitySet(double *mat);
TEEM_API double *mossMatTranslateSet(double *mat, double tx, double ty);
TEEM_API double *mossMatRotateSet(double *mat, double angle);
TEEM_API double *mossMatFlipSet(double *mat, double angle);
TEEM_API double *mossMatShearSet(double *mat, double angleFixed,
                                 double amount);
TEEM_API double *mossMatScaleSet(double *mat, double sx, double sy);
TEEM_API void mossMatApply(double *ox, double *oy, double *mat,
                           double ix, double iy);
TEEM_API int mossLinearTransform(Nrrd *nout, Nrrd *nin, float *bg,
                                 double *mat,
                                 mossSampler *msp,
                                 double xMin, double xMax,
                                 double yMin, double yMax,
                                 int sx, int sy);


#ifdef __cplusplus
}
#endif

#endif /* MOSS_HAS_BEEN_INCLUDED */
