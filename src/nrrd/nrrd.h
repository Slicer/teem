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


#ifndef NRRD_HAS_BEEN_INCLUDED
#define NRRD_HAS_BEEN_INCLUDED

#define NRRD "nrrd"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <errno.h>

#include <air.h>
#include <biff.h>

#include "nrrdDefines.h"
#include "nrrdMacros.h"
#include "nrrdEnums.h"

/*
******** nrrdBigInt typedef 
**
** biggest unsigned integral type allowed on system; used to hold
** number of elements in a nrrd.  Value 0 is used to represent "don't
** know" or "unset", since nrrds can't hold a zero number of items.  
*/
typedef unsigned long long int nrrdBigInt;

/*
******** NrrdIO struct
**
** Everything transient relating to how the nrrd is read and written.
** Once the nrrd has been read or written, this information is moot.
*/
typedef struct {
  char dir[NRRD_STRLEN_LINE],    /* allows us to remember the directory
                                    from whence this nrrd was "load"ed,
                                    or to whence this nrrd is "save"ed,
                                    so as to facilitate games with data
                                    files relative to header files */
    base[NRRD_STRLEN_LINE],      /* when "save"ing a nrrd into seperate 
                                    header and data, the name of the header
				    file (e.g. "output.nhdr"); this is
				    massaged to produce a header-relative
				    data filename.  This filename includes
				    the extension; "base" just signifies
				    "not full path" */
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
    seperateHeader,              /* nrrd is split into distinct header and
				    data (in either reading or writing) */
    bareTable,                   /* when writing a table, is there any
				    effort made to record the nrrd struct
				    info in the text file */
    charsPerLine,                /* when writing ASCII data in which we intend
				    only to write a huge long list of numbers
				    whose text formatting implies nothing, then
				    how many characters do we limit ourselves
				    to per line */
    valsPerLine,                 /* when writing ASCII data in which we DO
				    intend to sigify (or at least hint at)
				    something with the formatting, then what
				    is the max number of values to write on
				    a line */
    seen[NRRD_FIELD_MAX+1];      /* for error checking in header parsing */
} NrrdIO;

/*
******** NrrdAxis struct
**
** all the information which can sensibly be associated with
** one axis of a nrrd.  The only member which MUST be explicitly
** set to something meaningful is "size".
**
** The min and max values give the range of positions "represented"
** by the samples along this axis.  In node-centering, "min" IS the
** position at the lowest index.  In cell-centering, the position at
** the lowest index is between min and max (a touch bigger than min,
** assuming min < max).
*/
typedef struct {
  int size;                      /* number of elements along each axis */
  double spacing;                /* if non-NaN, distance between samples */
  double min, max;               /* if non-NaN, range of positions spanned
				    by the samples on this axis.  Obviously,
				    one can set "spacing" to something
				    incompatible with min and max: the idea
				    is that only one (min and max, or
				    spacing) should be taken to be significant
				    at any time. */
  int center;                    /* cell vs. node centering */
  char *label;                   /* short info string for each axis */
} NrrdAxis;

/*
******** Nrrd struct
**
** The struct used to wrap around the raw data array
*/
typedef struct {
  /* 
  ** NECESSARY information describing the main array.  This is
  ** generally set at the same time that either the nrrd is created,
  ** or at the time that the nrrd is wrapped around an existing array 
  */
  void *data;                    /* the data in memory */
  int type;                      /* a value from the nrrdType enum */
  int dim;                       /* what is dimension of data */

  /* 
  ** All per-axis specific information
  */
  NrrdAxis axis[NRRD_DIM_MAX];

  /* 
  ** Information of dubious standing- descriptive of whole array, but
  ** not necessary (meaningful only for some uses of a nrrd), but basic
  ** enough to be part of the basic nrrd type
  */
  char *content;                 /* briefly, just what the hell is this data */
  int blockSize;                 /* for nrrdTypeBlock array, block byte size */
  double min, max,               /* if non-NaN, nominally: extremal values for
				    array, but practically: the min and max 
				    values to use for nrrd calls for which
				    a min and max values are used */
    oldMin, oldMax;              /* if non-NaN, and if nrrd is of integral
				    type, extremal values for the array
				    BEFORE it was quantized */
  void *ptr;                     /* generic pointer which is NEVER read or
				    set by nrrd library. Use as you see fit. */
  int hasNonExist;               /* set to one of the nrrdNonExist enum values
				    by all of the nrrd functions which call
				    AIR_EXISTS on all the values */

  /* 
  ** Comments.  Read from, and written to, header.
  ** The comment array "cmt" is NOT NULL-terminated.
  ** The number of comments is cmtArr->len.
  */
  char **cmt;
  airArray *cmtArr;
} Nrrd;

/*
******** NrrdKernel struct
**
** these are essentially the methods of the various kernels implemented.
**
** Nrrd's use of this sort of kernel always assumes support symmetric
** around zero, but does not assume anything about even- or oddness
*/
typedef struct {
  int numParam;                          /* number of parameters needed
					    (# elements in param[] used) */
  double (*support)(double *param);      /* smallest x (x > 0) such that
					    k(y) = 0 for all y > x, y < -x */
  double (*integral)(double *param);     /* integral of kernel from -support
					    to +support */
  float (*eval1_f)(float x,              /* evaluate once, single precision */
		   double *param);
  void (*evalN_f)(float *f, float *x,    /* evaluate N times, single prec. */
		  int N, double *param);   
  double (*eval1_d)(double x,            /* evaluate once, double precision */
		    double *param);
  void (*evalN_d)(double *f, double *x,  /* evaluate N times, double prec. */
		  int N, double *param);
} NrrdKernel;

/*
******** NrrdResampleInfo struct
**
** a struct to contain the many parameters needed for nrrdSpatialResample()
*/
typedef struct {
  /* kernel, samples, and param are all per-axis */
  NrrdKernel *kernel[NRRD_DIM_MAX]; /* kernels from nrrd (or something
				       supplied by the user) */
  int samples[NRRD_DIM_MAX];        /* number of samples */
  double param[NRRD_DIM_MAX][NRRD_KERNEL_PARAMS_MAX], /* kernel arguments */
    min[NRRD_DIM_MAX],
    max[NRRD_DIM_MAX];              /* min[i] and max[i] are the range, in
				       index space, along which to resample
				       axis i */
  int boundary,                     /* value from the nrrdBoundary enum */
    type,                           /* desired type of output, use
				       nrrdTypeUnknown for "same as input" */
    renormalize,                    /* when downsampling with a kernel with
		   		       non-zero integral, should we renormalize
				       the weights to match the kernel integral
				       so as to remove annoying ripple */
    clamp;                          /* when copying from the last intermediate
				       (floating point) result to the output
				       nrrd, should we clamp the values to the
				       range of values for the output type, a
				       concern only for fixed-point outputs */
  double padValue;                  /* if padding, what value to pad with */
} NrrdResampleInfo;

/*
******** NrrdIter struct
**
** to hold values, either a single value, or a whole nrrd of values,
** and to facilitate iterating through those values
*/
typedef struct {
  Nrrd *nrrd;                       /* nrrd to get values from */
  double val;                       /* single fixed value */
  int size;                         /* type size */
  char *data;                       /* where to get the next value */
  nrrdBigInt left;                  /* number of values beyond what "data"
				       currently points to */
  double (*load)(void*);            /* how to get a value out of "data" */
} NrrdIter;

/******** defaults (nrrdDef..) and state (nrrdState..) */
/* defaults.c */
extern int nrrdDefWrtEncoding;
extern int nrrdDefWrtSeperateHeader;
extern int nrrdDefWrtBareTable;
extern int nrrdDefWrtCharsPerLine;
extern int nrrdDefWrtValsPerLine;
extern int nrrdDefRsmpBoundary;
extern int nrrdDefRsmpType;
extern double nrrdDefRsmpScale;
extern int nrrdDefRsmpRenormalize;
extern int nrrdDefRsmpClamp;
extern double nrrdDefRsmpPadValue;
extern int nrrdDefCenter;
extern double nrrdDefSpacing;
extern double nrrdDefKernelParam0;
extern int nrrdStateVerboseIO;
extern int nrrdStateClever8BitMinMax;
extern int nrrdStateMeasureType;
extern int nrrdStateMeasureModeBins;
extern int nrrdStateMeasureHistoType;
extern int nrrdStateAlwaysSetContent;
extern char nrrdStateUnknownContent[];
extern int nrrdStateDisallowFixedPointNonExist;

/******** all the airEnums used through-out nrrd */
/* (the actual C enums are in nrrdEnums.h) */
/* enums.c */
extern airEnum nrrdFormat;
extern airEnum nrrdBoundary;
extern airEnum nrrdMagic;
extern airEnum nrrdType;
extern airEnum nrrdEncoding;
extern airEnum nrrdMeasure;
extern airEnum nrrdCenter;
extern airEnum nrrdAxesInfo;
extern airEnum nrrdField;
extern airEnum nrrdUnaryOp;
extern airEnum nrrdBinaryOp;
extern airEnum nrrdTernaryOp;

/******** arrays of things (poor-man's functions/predicates) */
/* arrays.c */
extern char nrrdTypeConv[][AIR_STRLEN_SMALL];
extern int nrrdEncodingEndianMatters[];
extern int nrrdTypeSize[];
extern int nrrdTypeFixed[];

/******** pseudo-constructors, pseudo-destructors, and such */
/* (methods.c) */
extern NrrdIO *nrrdIONew(void);
extern void nrrdIOReset(NrrdIO *io);
extern NrrdIO *nrrdIONix(NrrdIO *io);
extern NrrdResampleInfo *nrrdResampleInfoNew(void);
extern NrrdResampleInfo *nrrdResampleInfoNix(NrrdResampleInfo *info);
extern void nrrdInit(Nrrd *nrrd);
extern Nrrd *nrrdNew(void);
extern Nrrd *nrrdNix(Nrrd *nrrd);
extern Nrrd *nrrdEmpty(Nrrd *nrrd);
extern Nrrd *nrrdNuke(Nrrd *nrrd);
extern int nrrdWrap_nva(Nrrd *nrrd, void *data, int type, int dim, int *size);
extern int nrrdWrap(Nrrd *nrrd, void *data, int type, int dim, ...);
extern Nrrd *nrrdUnwrap(Nrrd *nrrd);
extern int nrrdCopy(Nrrd *nout, Nrrd *nin);
extern int nrrdAlloc_nva(Nrrd *nrrd, int type, int dim, int *size);
extern int nrrdAlloc(Nrrd *nrrd, int type, int dim, ...);
extern int nrrdMaybeAlloc_nva(Nrrd *nrrd, int type, int dim, int *size);
extern int nrrdMaybeAlloc(Nrrd *nrrd, int type, int dim, ...);
extern int nrrdPPM(Nrrd *, int sx, int sy);
extern int nrrdPGM(Nrrd *, int sx, int sy);
extern int nrrdTable(Nrrd *table, int sx, int sy);

/******** nrrd value iterator gadget */
/* (iter.c) */
extern NrrdIter *nrrdIterNew(void);
extern void nrrdIterSetValue(NrrdIter *iter, double val);
extern void nrrdIterSetNrrd(NrrdIter *iter, Nrrd *nrrd);
extern double nrrdIterValue(NrrdIter *iter);
extern NrrdIter *nrrdIterNix(NrrdIter *iter);
extern NrrdIter *nrrdIterNuke(NrrdIter *iter);

/******** axes related */
/* axes.c */
extern int nrrdAxesCopy(Nrrd *nout, Nrrd *nin, int *map, int bitflag);
extern void nrrdAxesSet_nva(Nrrd *nin, int axInfo, void *info);
extern void nrrdAxesSet(Nrrd *nin, int axInfo, ...);
extern void nrrdAxesGet_nva(Nrrd *nrrd, int axInfo, void *info);
extern void nrrdAxesGet(Nrrd *nrrd, int axInfo, ...);
extern double nrrdAxisPos(Nrrd *nrrd, int ax, double idx);
extern double nrrdAxisIdx(Nrrd *nrrd, int ax, double pos);
extern void nrrdAxisPosRange(double *loP, double *hiP, Nrrd *nrrd, int ax,
			     double loIdx, double hiIdx);
extern void nrrdAxisIdxRange(double *loP, double *hiP, Nrrd *nrrd, int ax,
			     double loPos, double hiPos);
extern void nrrdAxisSpacingSet(Nrrd *nrrd, int ax);
extern void nrrdAxisMinMaxSet(Nrrd *nrrd, int ax);

/******** simple things */
/* simple.c */
extern int nrrdContentSet(Nrrd *nout, const char *func,
			  Nrrd *nin, const char *format, ...);
extern void nrrdDescribe(FILE *file, Nrrd *nrrd);
extern int nrrdValid(Nrrd *nrrd);
extern int nrrdElementSize(Nrrd *nrrd);
extern nrrdBigInt nrrdElementNumber(Nrrd *nrrd);
extern int nrrdHasNonExistSet(Nrrd *nrrd);
extern int nrrdSanity(void);
extern int nrrdSameSize(Nrrd *n1, Nrrd *n2, int useBiff);
extern int nrrdFitsInFormat(Nrrd *nrrd, int format, int useBiff);

/******** comments related */
/* comment.c */
extern int nrrdCommentAdd(Nrrd *nrrd, char *str);
extern void nrrdCommentClear(Nrrd *nrrd);
extern int nrrdCommentCopy(Nrrd *nout, Nrrd *nin);
extern char *nrrdCommentScan(Nrrd *nrrd, char *key);

/******** endian related */
/* (endian.c) */
extern void nrrdSwapEndian(Nrrd *nrrd);

/******** getting value into and out of an array of general type, and
   all other simplistic functionality pseudo-parameterized by type */
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
extern float  (*nrrdFInsert[NRRD_TYPE_MAX+1])(void *v, nrrdBigInt I, float f);
extern double (*nrrdDInsert[NRRD_TYPE_MAX+1])(void *v, nrrdBigInt I, double d);
extern int    (*nrrdSprint[NRRD_TYPE_MAX+1])(char *, void *);
extern int    (*nrrdFprint[NRRD_TYPE_MAX+1])(FILE *, void *);
extern float  (*nrrdFClamp[NRRD_TYPE_MAX+1])(float);
extern double (*nrrdDClamp[NRRD_TYPE_MAX+1])(double);
extern void (*nrrdFindMinMax[NRRD_TYPE_MAX+1])(void *minP, void *maxP,
					       Nrrd *nrrd);
extern int (*nrrdValCompare[NRRD_TYPE_MAX+1])(const void *, const void *);

/******** getting information to and from files */
/* read.c */
extern int (*nrrdReadData[NRRD_ENCODING_MAX+1])(Nrrd *, NrrdIO *);
extern int nrrdLoad(Nrrd *nrrd, char *filename);
extern int nrrdRead(Nrrd *nrrd, FILE *file, NrrdIO *io);
/* write.c */
extern int (*nrrdWriteData[NRRD_ENCODING_MAX+1])(Nrrd *, NrrdIO *);
extern int nrrdSave(char *filename, Nrrd *nrrd, NrrdIO *io);
extern int nrrdWrite(FILE *file, Nrrd *nrrd, NrrdIO *io);

/******** some of the point-wise value remapping, conversion, and such */
/* map.c */
extern void nrrdMinMaxSet(Nrrd *nrrd);
extern int nrrdMinMaxClever(Nrrd *nrrd);
extern int nrrdConvert(Nrrd *nout, Nrrd *nin, int type);
extern int nrrdQuantize(Nrrd *nout, Nrrd *nin, int bits);
extern int nrrdHistoEq(Nrrd *nout, Nrrd *nin,
		       Nrrd **nhistP, int bins, int smart);

/******** rest of point-wise value remapping, and "color"mapping */
/* apply.c */
extern int nrrdApply1DLut(Nrrd *nout, Nrrd *nlut, Nrrd *nin);
extern int nrrdApply1DRegMap(Nrrd *nout, Nrrd *nmap, Nrrd *nin,
			     NrrdKernel *kernel, double *param);

/******** sampling, slicing, cropping */
/* subset.c */
extern int nrrdSample_nva(void *val, Nrrd *nin, int *coord);
extern int nrrdSample(void *val, Nrrd *nin, ...);
extern int nrrdSlice(Nrrd *nout, Nrrd *nin, int axis, int pos);
extern int nrrdCrop(Nrrd *nout, Nrrd *nin, int *min, int *max);
extern int nrrdSimpleCrop(Nrrd *nout, Nrrd *nin, int crop);

/******** padding */
/* superset.c */
extern int nrrdPad(Nrrd *nout, Nrrd *nin, int *min, int *max, 
		   int boundary, ...);
extern int nrrdSimplePad(Nrrd *nout, Nrrd *nin, int pad, int boundary, ...);

/******** permuting and shuffling */
/* reorder.c */
extern int nrrdInvertPerm(int *invp, int *perm, int n);
extern int nrrdPermuteAxes(Nrrd *nout, Nrrd *nin, int *axes);
extern int nrrdSwapAxes(Nrrd *nout, Nrrd *nin, int ax1, int ax2);
extern int nrrdShuffle(Nrrd *nout, Nrrd *nin, int axis, int *perm);
extern int nrrdFlip(Nrrd *nout, Nrrd *nin, int axis);
extern int nrrdJoin(Nrrd *nout, Nrrd **nin, int num, int axis, int incrDim);
extern int nrrdReshape(Nrrd *nout, Nrrd *nin, int dim, ...);
extern int nrrdReshape_nva(Nrrd *nout, Nrrd *nin, int dim, int *size);
extern int nrrdBlock(Nrrd *nout, Nrrd *nin);
extern int nrrdUnblock(Nrrd *nout, Nrrd *nin, int type);

/******** measuring and projecting */
/* measr.c */
extern int nrrdProject(Nrrd *nout, Nrrd *nin, int axis, int measr);

/********* various kinds of histograms */
/* histogram.c */
extern int nrrdHisto(Nrrd *nout, Nrrd *nin, int bins, int type);
extern int nrrdHistoDraw(Nrrd *nout, Nrrd *nin, int sy);
extern int nrrdHistoAxis(Nrrd *nout, Nrrd *nin, int axis, int bins, int type);
extern int nrrdHistoJoint(Nrrd *nout, Nrrd **nin, 
			  int numNrrds, int *bins, int type, int *clamp);

/******** arithmetic and math on nrrds */
/* arith.c */
extern int nrrdArithGamma(Nrrd *nout, Nrrd *nin, double gamma,
			  double min, double max);
extern int nrrdArithUnaryOp(Nrrd *nout, int op, Nrrd *nin);
extern int nrrdArithBinaryOp(Nrrd *nout, int op,
			     NrrdIter *inA, NrrdIter *inB);
extern int nrrdArithTernaryOp(Nrrd *nout, int op,
			      NrrdIter *inA, NrrdIter *inB, NrrdIter *inC);

/******** filtering and re-sampling */
/* filt.c */
extern int nrrdCheapMedian(Nrrd *nout, Nrrd *nin,
			   int radius, float wght, int bins);
/* rsmp.c */
extern int nrrdSpatialResample(Nrrd *nout, Nrrd *nin, NrrdResampleInfo *info);
extern int nrrdSimpleResample(Nrrd *nout, Nrrd *nin,
			      NrrdKernel *kernel, double *param,
			      int *samples, double *scalings);

/******** kernels (interpolation, 1st and 2nd derivatives) */
/* kernel.c */
extern NrrdKernel *nrrdKernelZero, /* zero everywhere */
  *nrrdKernelBox,                  /* box filter (nearest neighbor) */
  *nrrdKernelTent,                 /* tent filter (linear interpolation) */
  *nrrdKernelForwDiff,             /* forward-difference-ish 1st deriv. */
  *nrrdKernelCentDiff,             /* central-difference-ish 1st deriv. */
  *nrrdKernelBCCubic,              /* BC family of cubic polynomial splines */
  *nrrdKernelBCCubicD,             /* 1st deriv. of BC cubic family */
  *nrrdKernelBCCubicDD,            /* 2nd deriv. of BC cubic family */
  *nrrdKernelAQuartic,             /* A family of quartic C2 interp. splines */
  *nrrdKernelAQuarticD,            /* 1st deriv. of A quartic family */
  *nrrdKernelAQuarticDD,           /* 2nd deriv. of A quartic family */
  *nrrdKernelGaussian,             /* Gaussian */
  *nrrdKernelGaussianD,            /* 1st derivative of Gaussian */
  *nrrdKernelGaussianDD;           /* 2nd derivative of Gaussian */
extern int nrrdKernelParse(NrrdKernel **kernelP, double *param, char *str);

#endif /* NRRD_HAS_BEEN_INCLUDED */
