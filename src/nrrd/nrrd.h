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


#ifndef NRRD_HAS_BEEN_INCLUDED
#define NRRD_HAS_BEEN_INCLUDED
#ifdef __cplusplus
extern "C" {
#endif

#define NRRD "nrrd"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>

#include <air.h>
#include <biff.h>

#include "nrrdDefines.h"
#include "nrrdMacros.h"
#include "nrrdEnums.h"

/*
******** nrrdBigInt typedef biggest unsigned integral type allowed on
** system; used to hold number of elements in a nrrd.  Value 0 is used
** to represent "don't know" or "unset", since nrrd's need to hold
** something
*/
typedef unsigned long long int nrrdBigInt;

/*
******** nrrdAxis struct
**
** all the information which can sensibly be associated with
** one axis of a nrrd
*/
typedef struct {
  unsigned int size;             /* number of elements along each axis */
  double spacing;                /* if non-NaN, distance between samples */
  double min, max;               /* if non-NaN, values associated with
                                    lowest and highest indices.
                                    Obviously, one can set "spacing"
                                    to something incompatible with
                                    axisMin, axisMax.  No clear policy
                                    on this one yet */
  int center;                    /* cell vs. node */
  char *label;                   /* short info string for each axis */
} nrrdAxis;

/*
******** nrrdIO struct
**
** everything transient relating to how the nrrd is read and written
*/
typedef struct {
  char dir[NRRD_STRLEN_LINE],    /* allows us to remember the directory
                                    from whence this nrrd was "load"ed,
                                    or to whence this nrrd is "save"ed,
                                    so as to facilitate games with data
                                    files relative to header files */
    base[NRRD_STRLEN_LINE],      /* when "save"ing a nrrd into seperate 
                                    header and data, the name of the 
				    header file (massaged to produce
				    header-relative data filename) */
    line[NRRD_STRLEN_LINE];      /* buffer for saving one line from file */
  int pos;                       /* line[pos] is beginning of stuff which
				    still has yet to be parsed */

  FILE *dataFile;                /* if non-NULL, where the data is to
                                    be read from or written to.  If
                                    NULL, data will be read from
                                    current file */

  int magic,                     /* on input, magic of file read */
    format,                      /* which format (from nrrdFormat) */
    encoding,                    /* which encoding (from nrrdEncoding) */
    endian,                      /* endian-ness of the data in file, for
				    those encoding/type combinations
				    for which it matters (from nrrdEndian) */
    lineSkip,                    /* if dataFile non-NULL, the number of lines
                                    in dataFile that should be skipped over
                                    (so as to bypass another form of ASCII
                                    header preceeding raw data) */
    byteSkip,                    /* exactly like lineSkip, but bytes 
                                    instead of lines.  First the lines are
                                    skipped, then the bytes */
    seperateHeader,              /* write seperate ".nhdr" and ".raw" files */
    bareTable,                   /* when writing a table, is there any
				    effort made to record the nrrd struct
				    info in the text file */
    seen[NRRD_FIELD_MAX+1];      /* for error checking in header parsing */
} nrrdIO;

/*
******** Nrrd struct
**
** The struct used to wrap around the raw data array
*/
typedef struct {
  /* 
  ** NECESSARY information describing the main array.  This is
  ** generally set at the same time that either the nrrd is created,
  ** or at the time that an existing nrrd is wrapped around an array 
  */
  void *data;                    /* the data in memory */
  nrrdBigInt num;                /* number of elements */
  int type;                      /* a value from the nrrdType enum */
  int dim;                       /* what is dimension of data */

  /* 
  ** All per-axis specific information
  */
  nrrdAxis axis[NRRD_DIM_MAX];

  /* 
  ** Information of dubious standing- descriptive of whole array, but
  ** not necessary (meaningful only for some uses of a nrrd), but basic
  ** enough to be part of the basic nrrd type
  */
  char *content;                 /* briefly, just what the hell is this data */
  int blockSize;                 /* for nrrdTypeBlock array, block byte size */
  double min, max,               /* if non-NaN, extremal values for array */
    oldMin, oldMax;              /* if non-NaN, and if nrrd is of integral
				    type, extremal values for the array
				    BEFORE it was quantized */
  void *ptr;                     /* generic pointer which is NEVER read or
				    set (except to NULL) by nrrd library. 
				    Use as you see fit. */

  /* 
  ** Comments.  Read from, and written to, header.
  ** The comment array "cmt" is NOT NULL terminated.
  */
  char **cmt;
  airArray *cmtArr;
} Nrrd;

/*
******** nrrdKernel struct
**
** these are essentially the methods of the various kernels implemented.
**
** Nrrd's use of this sort of kernel always assumes support symmetric
** around zero, but does not assume anything about even- or oddness
*/
typedef struct {
  int (*numParam)(void);                  /* number of parameters needed
					     (# elements in param[] used) */
  float (*support)(float *param);         /* smallest x (>0) such that
					     k(y) = 0 for all y>x, y<-x */
  float (*integral)(float *param);        /* integral of kernel from -support
					     to +support */
  float (*eval_f)(float x, float *param); /* evaluate the kernel once */
  void (*evalVec_f)(float *f, float *x,   /* evaluate many times */
		  int N, float *param);   
  double (*eval_d)(double x, float *param); /* evaluate once, double prec. */
  void (*evalVec_d)(double *f, double *x,   /* evaluate many times, double */
		   int N, float *param);
} nrrdKernel;

/*
******** nrrdResampleInfo struct
**
*/
typedef struct {
  nrrdKernel *kernel[NRRD_DIM_MAX];       /* kernels from nrrd, or something
					     supplied by the user */
  int samples[NRRD_DIM_MAX];              /* number of samples */
  float param[NRRD_DIM_MAX][NRRD_KERNEL_PARAMS_MAX], /* kernel arguments */
    min[NRRD_DIM_MAX],
    max[NRRD_DIM_MAX];           /* range, in index space, along which to
				    resample */
  int boundary,                  /* value from the nrrdBoundary enum */
    type,                        /* desired type of output, use
				    nrrdTypeUnknown for "same as input" */
    renormalize;                 /* when downsampling with a kernel with
				    non-zero integral, should we renormalize
				    the weights to match the kernel integral
				    so as to remove ripple */
  float padValue;                /* if padding, what value to pad with */
} nrrdResampleInfo;

/******** going between the enums' values and strings */
/* arrays.c */
extern char *nrrdEnumValToStr(int whichEnum, int val);
extern int nrrdEnumStrToVal(int whichEnum, char *_str);
extern char nrrdTypeConv[][NRRD_STRLEN_SMALL];
extern int nrrdEncodingEndianMatters[];
extern int nrrdTypeSize[];

/******** pseudo-constructors, pseudo-destructors, and such */
/* (methods.c) */
extern nrrdIO *nrrdIONew(void);
extern nrrdIO *nrrdIONix(nrrdIO *io);
extern nrrdResampleInfo *nrrdResampleInfoNew(void);
extern nrrdResampleInfo *nrrdResampleInfoNix(nrrdResampleInfo *info);
extern Nrrd *nrrdNew(void);
extern void nrrdInit(Nrrd *nrrd);
extern Nrrd *nrrdNix(Nrrd *nrrd);
extern Nrrd *nrrdEmpty(Nrrd *nrrd);
extern Nrrd *nrrdNuke(Nrrd *nrrd);
extern Nrrd *nrrdWrap(Nrrd *nrrd, void *data, 
		      nrrdBigInt num, int type, int dim);
extern Nrrd *nrrdWrap_va(Nrrd *nrrd, void *data, int type, int dim, ...);
extern Nrrd *nrrdUnwrap(Nrrd *nrrd);
extern int nrrdAlloc(Nrrd *nrrd, nrrdBigInt num, int type, int dim);
extern int nrrdAlloc_va(Nrrd *nrrd, int type, int dim, ...);
extern int nrrdMaybeAlloc(Nrrd *nrrd, nrrdBigInt num, int type, int dim);
extern int nrrdMaybeAlloc_va(Nrrd *nrrd, int type, int dim, ...);
extern int nrrdPPM(Nrrd *, int sx, int sy);
extern int nrrdPGM(Nrrd *, int sx, int sy);
extern int nrrdTable(Nrrd *table, int sx, int sy);
extern int nrrdCopy(Nrrd *nout, Nrrd *nin);

/******** simple things */
/* simple.c */
extern void nrrdDescribe(FILE *file, Nrrd *nrrd);
extern int nrrdCheck(Nrrd *nrrd);
extern int nrrdSameSize(Nrrd *n1, Nrrd *n2, int useBiff);
extern int nrrdElementSize(Nrrd *nrrd);
extern int nrrdFitsInFormat(Nrrd *nrrd, int format, int useBiff);

/******** axes related */
/* axes.c */
extern nrrdAxis *nrrdAxisNew(void);
extern nrrdAxis *nrrdAxisNix(nrrdAxis *axis);
extern int nrrdAxesCopy(Nrrd *nout, Nrrd *nin, int *map, int bitflag);
extern void nrrdAxesSet_va(Nrrd *nin, int axInfo, ...);
extern void nrrdAxesSet(Nrrd *nin, int axInfo, void *info);
extern void nrrdAxesGet(Nrrd *nrrd, int axInfo, void *info);

/******** comments related */
/* comment.c */
extern int nrrdCommentAdd(Nrrd *nrrd, char *_str, int useBiff);
extern void nrrdCommentClear(Nrrd *nrrd);
extern int nrrdCommentCopy(Nrrd *nout, Nrrd *nin, int useBiff);
extern int nrrdCommentScan(Nrrd *nrrd, char *key, char **valP);

/******** endian related */
/* (endian.c) */
extern void nrrdSwapEndian(Nrrd *nrrd);

/******** getting value into and out of an array of general type, and
   other simplistic functionality pseudo-parameterized by type */
/* accessors.c */
extern int    (*nrrdILoad[NRRD_TYPE_MAX+1])(void *v);
extern float  (*nrrdFLoad[NRRD_TYPE_MAX+1])(void *v);
extern double (*nrrdDLoad[NRRD_TYPE_MAX+1])(void *v);
extern int    (*nrrdIStore[NRRD_TYPE_MAX+1])(void *v, int j);
extern float  (*nrrdFStore[NRRD_TYPE_MAX+1])(void *v, float f);
extern double (*nrrdDStore[NRRD_TYPE_MAX+1])(void *v, double d);
extern int    (*nrrdILookup[NRRD_TYPE_MAX+1])(void *v, nrrdBigInt I);
extern float  (*nrrdFLookup[NRRD_TYPE_MAX+1])(void *v, nrrdBigInt I);
extern double (*nrrdDLookup[NRRD_TYPE_MAX+1])(void *v, nrrdBigInt I);
extern int    (*nrrdIInsert[NRRD_TYPE_MAX+1])(void *v, nrrdBigInt I, int j);
extern float  (*nrrdFInsert[NRRD_TYPE_MAX+1])(void *v, 
					      nrrdBigInt I, float f);
extern double (*nrrdDInsert[NRRD_TYPE_MAX+1])(void *v, 
					      nrrdBigInt I, double d);
extern int    (*nrrdSprint[NRRD_TYPE_MAX+1])(char *, void *);
extern int    (*nrrdFprint[NRRD_TYPE_MAX+1])(FILE *, void *);
extern float  (*nrrdFClamp[NRRD_TYPE_MAX+1])(float);
extern double (*nrrdDClamp[NRRD_TYPE_MAX+1])(double);

/******** getting information to and from files */
/* read.c */
extern int nrrdStrictPNMComments;
extern int nrrdRead(Nrrd *nrrd, FILE *file, nrrdIO *io);
extern int nrrdLoad(Nrrd *nrrd, char *filename);
/* write.c */
extern int nrrdWrite(FILE *file, Nrrd *nrrd, nrrdIO *io);
extern int nrrdSave(char *filename, Nrrd *nrrd, nrrdIO *io);

/******** sampling, slicing, cropping+padding */
/* subset.c */
extern int nrrdSample(void *val, Nrrd *nin, int *coord);
extern int nrrdSlice(Nrrd *nout, Nrrd *nin, int axis, int pos);
extern int nrrdSubvolume(Nrrd *nout, Nrrd *nin, 
			 int *minIdx, int *maxIdx, int clamp);

/******** permuting and shuffling */
/* reorder.c */
extern int nrrdInvertPerm(int *invp, int *perm, int n);
extern int nrrdPermuteAxes(Nrrd *nout, Nrrd *nin, int *axes);
extern int nrrdSwapAxes(Nrrd *nout, Nrrd *nin, int ax1, int ax2);
extern int nrrdShuffle(Nrrd *nout, Nrrd *nin, int axis, int *perm);
extern int nrrdJoin(Nrrd *nout, Nrrd **nin, int num, int axis, int incrDim);
extern int nrrdFlip(Nrrd *nout, Nrrd *nin, int axis);

/******** measuring and projecting */
/* measr.c */
extern int nrrdMeasureAxis(Nrrd *nout, Nrrd *nin, int axis, int measr);

/********* HISTOGRAMS!!! */
/* histogram.c */
extern int nrrdHistoAxis(Nrrd *nout, Nrrd *nin, int axis, unsigned int bins);
extern int nrrdHistoMulti(Nrrd *nout, Nrrd **nin, 
			  int num, int *bin, 
			  float *min, float *max, int *clamp);
extern int nrrdHisto(Nrrd *nout, Nrrd *nin, int bins);
extern int nrrdHistoDraw(Nrrd *nout, Nrrd *nin, int sy);

/******** point-wise value remapping, conversion, and such */
/* map.c */
extern int nrrdMinMaxDo(double *minP, double *maxP, 
			Nrrd *nrrd, int minmax, ...);
extern int nrrdMinMaxFind(double *minP, double *maxP, Nrrd *nrrd);
extern int nrrdConvert(Nrrd *nout, Nrrd *nin, int type);
extern int nrrdQuantize(Nrrd *nout, Nrrd *nin, int bits, int minmax);
extern int nrrdHistoEq(Nrrd *nrrd, Nrrd **nhistP, int bins, int smart);

/******** arithmetic and math on nrrds */
/* arith.c */
extern int nrrdArithGamma(Nrrd *nout, Nrrd *nin, double gamma, 
			  int minmax, ...);

/******** filtering and re-sampling */
/* filt.c */
extern int nrrdMedian(Nrrd *nout, Nrrd *nin, int radius, int bins);
extern int nrrdSpatialResample(Nrrd *nout, Nrrd *nin, nrrdResampleInfo *info);

/******** kernels (interpolation, 1st and 2nd derivatives) */
/* kernel.c */
extern nrrdKernel *nrrdKernelZero, /* zero everywhere */
  *nrrdKernelBox,                  /* box filter (nearest neighbor) */
  *nrrdKernelTent,                 /* tent filter (linear interpolation) */
  *nrrdKernelForwDiff,             /* forward-difference-ish 1st deriv. */
  *nrrdKernelCentDiff,             /* central-difference-ish 1st deriv. */
  *nrrdKernelBCCubic,              /* BC family of cubic polynomial splines */
  *nrrdKernelBCCubicD,             /* 1st deriv. of BC cubic family */
  *nrrdKernelBCCubicDD,            /* 2nd deriv. of BC cubic family */
  *nrrdKernelAQuartic,             /* A family of quartic C2 interp. splines */
  *nrrdKernelAQuarticD,            /* 1st deriv. of A quartic family */
  *nrrdKernelAQuarticDD;           /* 2nd deriv. of A quartic family */

/* extern C */
#ifdef __cplusplus
}
#endif
#endif /* NRRD_HAS_BEEN_INCLUDED */



