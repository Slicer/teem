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

#ifndef MOSS_HAS_BEEN_INCLUDED
#define MOSS_HAS_BEEN_INCLUDED

#include <math.h>
#include <air.h>
#include <hest.h>
#include <biff.h>
#include <ell.h>
#include <nrrd.h>
#include <dye.h>

#if defined(WIN32) && !defined(TEEM_BUILD)
#define moss_export __declspec(dllimport)
#else
#define moss_export
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define MOSS "moss"

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
  NrrdKernel *kernel;                  /* which kernel to use on both axes */
  double kparm[NRRD_KERNEL_PARMS_NUM]; /* kernel arguments */
  float *ivc2, *ivc1;                  /* intermediate value caches, 2 and 1
					  dimensional */
  double *xFslw, *yFslw;               /* filter sample locations->weights */
  int fdiam, ncol;                     /* filter diameter; ivc2 is allocated
					  for fdiam x fdiam x ncol
					  doubles, ivc1 is fdiam x ncol
					  w/ those axis orderings */
  int *xIdx, *yIdx;                    /* arrays for x and y coordinates,
					  both allocated for fdiam */
  int boundary;                        /* from nrrdBoundary* enum */
  int flag[MOSS_FLAG_NUM];             /* I'm a flag-waving struct */
} mossSampler;

/* defaultsMoss.c */
extern int mossDefBoundary;
extern int mossDefCenter;

/* methodsMoss.c */
extern mossSampler *mossSamplerNew();
extern int mossSamplerFill(mossSampler *smplr, int fdiam, int ncol);
extern void mossSamplerEmpty(mossSampler *smplr);
extern mossSampler *mossSamplerNix(mossSampler *smplr);
extern int mossImageValid(Nrrd *image);
extern int mossImageAlloc(Nrrd *image, int type, int sx, int sy, int ncol);

/* sampler.c */
extern int mossSamplerImageSet(mossSampler *smplr, Nrrd *image);
extern int mossSamplerKernelSet(mossSampler *smplr,
				NrrdKernel *kernel, double *kparm);
extern int mossSamplerUpdate(mossSampler *smplr);
extern int mossSamplerSample(float *val, mossSampler *smplr,
			     double xPos, double yPos);

/* hestMoss.c */
extern hestCB *mossHestTransform;
extern hestCB *mossHestOrigin;

/* xform.c */
extern void mossMatPrint(FILE *f, double mat[6]);
extern double *mossMatPreMultiply(double mat[6], double x[6]);
extern double *mossMatPostMultiply (double mat[6], double x[6]);
extern double *mossMatInvert(double inv[6], double mat[6]);
extern double *mossMatIdentitySet(double mat[6]);
extern double *mossMatTranslateSet(double mat[6], double tx, double ty);
extern double *mossMatRotateSet(double mat[6], double angle);
extern double *mossMatFlipSet(double mat[6], double angle);
extern double *mossMatScaleSet(double mat[6], double sx, double sy);
extern int mossLinearTransform(Nrrd *nout, Nrrd *nin, double mat[6],
			       mossSampler *msp,
			       double xMin, double xMax,
			       double yMin, double yMax,
			       int sx, int sy);


#ifdef __cplusplus
}
#endif

#endif /* MOSS_HAS_BEEN_INCLUDED */
