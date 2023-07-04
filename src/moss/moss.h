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

#ifndef MOSS_HAS_BEEN_INCLUDED
#define MOSS_HAS_BEEN_INCLUDED

/* NOTE: this library has not undergone the changes as other Teem
   libraries in order to make sure that array lengths and indices
   are stored in unsigned types */

#include <math.h>

#include <teem/air.h>
#include <teem/hest.h>
#include <teem/biff.h>
#include <teem/nrrd.h>
#include <teem/ell.h>

#if defined(_WIN32) && !defined(__CYGWIN__) && !defined(TEEM_STATIC)
#  if defined(TEEM_BUILD) || defined(moss_EXPORTS) || defined(teem_EXPORTS)
#    define MOSS_EXPORT extern __declspec(dllexport)
#  else
#    define MOSS_EXPORT extern __declspec(dllimport)
#  endif
#else /* TEEM_STATIC || UNIX */
#  define MOSS_EXPORT extern
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define MOSS mossBiffKey

/* used by ilk, hence not in privateMoss.h */
#define MOSS_AXIS0(img)    (3 == (img)->dim ? 1 : 0)
/* how to learn chanNum for a given img Nrrd */
#define MOSS_CHAN_NUM(img) AIR_UINT(3 == (img)->dim ? (img)->axis[0].size : 1)

enum {
  mossFlagUnknown,    /* 0: nobody knows */
  mossFlagImage,      /* 1: image being sampled */
  mossFlagKernel,     /* 2: kernel(s) used for sampling */
  mossFlagChanNum,    /* 3: number of per-pixel channels */
  mossFlagFilterDiam, /* 4: kernel filter diameter */
  mossFlagLast
};
#define MOSS_FLAG_MAX 4

/* container for moss sampling. With July 2022 re-write, changed all floating-point
   types to double (from a confusing mix of float and double) */
typedef struct {
  int verbose,                 /* verbosity (set directly, rather than by a
                                  mossSampler..Set() function) */
    verbPixel[2];              /* debug pixel indices */
  const Nrrd *image;           /* the image to sample */
  int boundary;                /* from nrrdBoundary* enum */
  NrrdKernelSpec *kspec;       /* kernel to use on both (spatial) axes */
  unsigned int filterDiam,     /* filter diameter */
    chanNum;                   /* number of per-pixel channels */
  int *xIdx, *yIdx;            /* arrays for x and y indices into image
                                  both allocated for filterDiam */
  double *ivc,                 /* intermediate value cache, allocated for
                                  filterDiam x filterDiam x chanNum
                                  doubles, with that axis ordering */
    *xFslw, *yFslw,            /* filter sample locations->weights
                                  both allocated for filterDiam */
    *bg;                       /* background color. If non-NULL,
                                  allocated for chanNum, handled (unusually) by
                                  mossSamplerImageSet */
  int flag[MOSS_FLAG_MAX + 1]; /* I'm a flag-waving struct */
} mossSampler;

/* defaultsMoss.c */
MOSS_EXPORT const char *const mossBiffKey;
MOSS_EXPORT int mossDefCenter;

/* methodsMoss.c */
MOSS_EXPORT const int mossPresent;
MOSS_EXPORT mossSampler *mossSamplerNew(void);
MOSS_EXPORT mossSampler *mossSamplerNix(mossSampler *smplr);
MOSS_EXPORT int mossImageCheck(const Nrrd *image);
MOSS_EXPORT int mossImageAlloc(Nrrd *image, int type, unsigned int sx, unsigned int sy,
                               unsigned int chanNum);

/* sampler.c */
MOSS_EXPORT int mossSamplerImageSet(mossSampler *smplr, const Nrrd *image, int boundary,
                                    const double *bg);
MOSS_EXPORT int mossSamplerKernelSet(mossSampler *smplr, const NrrdKernelSpec *kspec);
MOSS_EXPORT int mossSamplerUpdate(mossSampler *smplr);
MOSS_EXPORT int mossSamplerSample(double *val, mossSampler *smplr, double xPos,
                                  double yPos);

/* hestMoss.c */
MOSS_EXPORT const hestCB *const mossHestTransform;
MOSS_EXPORT const hestCB *const mossHestOrigin;

/* xform.c */
MOSS_EXPORT void mossMatPrint(FILE *f, const double *mat);
MOSS_EXPORT double *mossMatRightMultiply(double *mat, const double *x);
MOSS_EXPORT double *mossMatLeftMultiply(double *mat, const double *x);
MOSS_EXPORT double *mossMatInvert(double *inv, const double *mat);
MOSS_EXPORT double *mossMatIdentitySet(double *mat);
MOSS_EXPORT double *mossMatTranslateSet(double *mat, double tx, double ty);
MOSS_EXPORT double *mossMatRotateSet(double *mat, double angle);
MOSS_EXPORT double *mossMatFlipSet(double *mat, double angle);
MOSS_EXPORT double *mossMatShearSet(double *mat, double angleFixed, double amount);
MOSS_EXPORT double *mossMatScaleSet(double *mat, double sx, double sy);
MOSS_EXPORT void mossMatApply(double *ox, double *oy, const double *mat, double ix,
                              double iy);
MOSS_EXPORT int mossLinearTransform(Nrrd *nout, const Nrrd *nin, int boundary,
                                    const double *bg, const double *mat,
                                    mossSampler *msp, double xMin, double xMax,
                                    double yMin, double yMax, int sx, int sy);
MOSS_EXPORT int mossFourPointTransform(Nrrd *nout, const Nrrd *nin, int boundary,
                                       const double *bg, const double xyc[8],
                                       mossSampler *msp, int xSize, int ySize);

#ifdef __cplusplus
}
#endif

#endif /* MOSS_HAS_BEEN_INCLUDED */
