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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <errno.h>
#include <float.h>

#include <air.h>
#include <hest.h>
#include <biff.h>

#include "nrrdDefines.h"
#include "nrrdMacros.h"
#include "nrrdEnums.h"

#if defined(_WIN32) && !defined(TEEM_BUILD)
#define nrrd_export __declspec(dllimport)
#else
#define nrrd_export
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define NRRD "nrrd"

/*
******** NrrdIO struct
**
** Everything transient relating to how the nrrd is read and written.
** Once the nrrd has been read or written, this information is moot.
*/
typedef struct {
  char *dir,                /* allows us to remember the directory
			       from whence this nrrd was "load"ed, or
			       to whence this nrrd is "save"ed, so as
			       to facilitate games with data files
			       relative to header files */
    *base,                  /* when "save"ing a nrrd into seperate
			       header and data, the name of the header
			       file (e.g. "output.nhdr"); this is
			       massaged to produce a header-relative
			       data filename.  This filename includes
			       the extension; "base" just signifies
			       "not full path" */
    *dataFN,                /* for "unu make -h" only: the exact name
			       of the data file.  In fact, we use the
			       non-NULL-ity of this as the flag indicating
			       that we're doing the unu make -h game */
    *line;                  /* buffer for saving one line from file */
  
  int lineLen,              /* allocated size of line, including the
			       last character for \0 */
    pos;                    /* line[pos] is beginning of stuff which
			       still has yet to be parsed */

  FILE *dataFile;           /* if non-NULL, where the data is to be
			       read from or written to.  If NULL, data
			       will be read from current file */

  int magic,                /* on input, magic of file read */
    format,                 /* which format (from nrrdFormat) */
    encoding,               /* which encoding (from nrrdEncoding) */
    endian,                 /* endian-ness of the data in file, for
			       those encoding/type combinations for
			       which it matters (from nrrdEndian) */
    lineSkip,               /* if dataFile non-NULL, the number of
			       lines in dataFile that should be
			       skipped over (so as to bypass another
			       form of ASCII header preceeding raw
			       data) */
    byteSkip,               /* exactly like lineSkip, but bytes
			       instead of lines.  First the lines are
			       skipped, then the bytes */
    seperateHeader,         /* nrrd is split into distinct header and
			       data (in either reading or writing) */
    bareTable,              /* when writing a table, is there any
			       effort made to record the nrrd struct
			       info in the text file */
    charsPerLine,           /* when writing ASCII data in which we
			       intend only to write a huge long list
			       of numbers whose text formatting
			       implies nothing, then how many
			       characters do we limit ourselves to per
			       line */
    valsPerLine,            /* when writing ASCII data in which we DO
			       intend to sigify (or at least hint at)
			       something with the formatting, then
			       what is the max number of values to
			       write on a line */
    skipData,               /* if non-zero, on input, we don't read
			       data, instead setting nrrd->data to
			       NULL.  This results in a broken Nrrd,
			       so be careful. */
    keepSeperateDataFileOpen, /* this hack for the sake of unu data */
    zlibLevel,              /* zlib compression level (0-9, -1 for
			       default[6], 0 for no compression). */
    zlibStrategy,           /* zlib compression strategy, can be one
			       of the nrrdZlibStrategy enums, default is
			       nrrdZlibStrategyDefault. */
    bzip2BlockSize,         /* block size used for compression, 
			       roughly equivalent to better but slower
			       (1-9, -1 for default[9]). */
    seen[NRRD_FIELD_MAX+1]; /* for error checking in header parsing */
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
  char *unit;                    /* short string for identifying units */
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
  NrrdAxis axis[NRRD_DIM_MAX];   /* axis[0] is the fastest axis in the scan-
				    line ordering, the one who's coordinates
				    change the fastest as the elements are
				    accessed in the order in which they appear
				    in memory */

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
**
** It is a strong but very simplifying assumption that the paramater
** array ("parm") is always type double.  There is essentially no
** value in allowing flexibility between float and double, and much
** teem code assumes that it will always be type double.
*/
typedef struct {
  char name[AIR_STRLEN_SMALL];           /* terse string representation of
					    kernel function, irrespective of
					    the parameter vector */
  int numParm;                           /* number of parameters needed
					    (# elements in parm[] used) */
  double (*support)(double *parm);       /* smallest x (x > 0) such that
					    k(y) = 0 for all y > x, y < -x */
  double (*integral)(double *parm);      /* integral of kernel from -support
					    to +support */
  float (*eval1_f)(float x,              /* evaluate once, single precision */
		   double *parm);
  void (*evalN_f)(float *f, float *x,    /* evaluate N times, single prec. */
		  size_t N, double *parm);   
  double (*eval1_d)(double x,            /* evaluate once, double precision */
		    double *parm);
  void (*evalN_d)(double *f, double *x,  /* evaluate N times, double prec. */
		  size_t N, double *parm);
} NrrdKernel;

/*
******** NrrdKernelSpec struct
** 
** for those times when it makes most sense to directly associate a
** NrrdKernel with its parameter vector (that is, a full kernel
** "spec"ification), basically: using hest.
*/
typedef struct {
  NrrdKernel *kernel;
  double parm[NRRD_KERNEL_PARMS_NUM];
} NrrdKernelSpec;

/*
******** NrrdResampleInfo struct
**
** a struct to contain the many parameters needed for nrrdSpatialResample()
*/
typedef struct {
  NrrdKernel *kernel[NRRD_DIM_MAX]; /* which kernel to use on each axis;
				       use NULL to signify no resampling
				       whatsoever on this axis */
  int samples[NRRD_DIM_MAX];        /* number of samples per axis */
  double parm[NRRD_DIM_MAX][NRRD_KERNEL_PARMS_NUM], /* kernel arguments */
    min[NRRD_DIM_MAX],
    max[NRRD_DIM_MAX];              /* min[i] and max[i] are the range, in
				       WORLD space, along which to resample
				       axis i. axis mins and maxs are required
				       on resampled axes. */
  int boundary,                     /* value from the nrrdBoundary enum */
    type,                           /* desired type of output, use
				       nrrdTypeUnknown for "same as input" */
    renormalize,                    /* when downsampling with a kernel with
		   		       non-zero integral, should we renormalize
				       the weights to match the kernel integral
				       so as to remove annoying ripple */
    round,                          /* when copying from the last intermediate
				       (floating point) result to the output
				       nrrd, for integer outputs, do we round
				       to the nearest integer first, before
				       clamping and assigning.  Enabling this
				       fixed the mystery of downsampling large
				       constant regions of 255 (uchar), and
				       ending up with 254 */
    clamp;                          /* when copying from the last intermediate
				       (floating point) result to the output
				       nrrd, should we clamp the values to the
				       range of values for the output type, a
				       concern only for integer outputs */
  double padValue;                  /* if padding, what value to pad with */
} NrrdResampleInfo;

/*
******** NrrdIter struct
**
** To hold values: either a single value, or a whole nrrd of values.
** Also, this facilitates iterating through those values
*/
typedef struct {
  Nrrd *nrrd;                       /* nrrd to get values from */
  double val;                       /* single fixed value */
  int size;                         /* type size */
  char *data;                       /* where to get the next value */
  size_t left;                      /* number of values beyond what "data"
				       currently points to */
  double (*load)(void*);            /* how to get a value out of "data" */
} NrrdIter;

/******** defaults (nrrdDef..) and state (nrrdState..) */
/* defaultsNrrd.c */
extern nrrd_export int nrrdDefWrtEncoding;
extern nrrd_export int nrrdDefWrtSeperateHeader;
extern nrrd_export int nrrdDefWrtBareTable;
extern nrrd_export int nrrdDefWrtCharsPerLine;
extern nrrd_export int nrrdDefWrtValsPerLine;
extern nrrd_export int nrrdDefRsmpBoundary;
extern nrrd_export int nrrdDefRsmpType;
extern nrrd_export double nrrdDefRsmpScale;
extern nrrd_export int nrrdDefRsmpRenormalize;
extern nrrd_export int nrrdDefRsmpRound;
extern nrrd_export int nrrdDefRsmpClamp;
extern nrrd_export double nrrdDefRsmpPadValue;
extern nrrd_export int nrrdDefCenter;
extern nrrd_export double nrrdDefSpacing;
extern nrrd_export double nrrdDefKernelParm0;
extern nrrd_export int nrrdStateVerboseIO;
extern nrrd_export int nrrdStateClever8BitMinMax;
extern nrrd_export int nrrdStateMeasureType;
extern nrrd_export int nrrdStateMeasureModeBins;
extern nrrd_export int nrrdStateMeasureHistoType;
extern nrrd_export int nrrdStateAlwaysSetContent;
extern nrrd_export char nrrdStateUnknownContent[];
extern nrrd_export int nrrdStateDisallowIntegerNonExist;
extern nrrd_export int nrrdStateGrayscaleImage3D;

/******** all the airEnums used through-out nrrd */
/* (the actual C enums are in nrrdEnums.h) */
/* enumsNrrd.c */
extern nrrd_export airEnum *nrrdFormat;
extern nrrd_export airEnum *nrrdBoundary;
extern nrrd_export airEnum *nrrdMagic;
extern nrrd_export airEnum *nrrdType;
extern nrrd_export airEnum *nrrdEncoding;
extern nrrd_export airEnum *nrrdMeasure;
extern nrrd_export airEnum *nrrdCenter;
extern nrrd_export airEnum *nrrdAxesInfo;
extern nrrd_export airEnum *nrrdField;
extern nrrd_export airEnum *nrrdUnaryOp;
extern nrrd_export airEnum *nrrdBinaryOp;
extern nrrd_export airEnum *nrrdTernaryOp;

/******** arrays of things (poor-man's functions/predicates) */
/* arraysNrrd.c */
extern nrrd_export char nrrdTypeConv[][AIR_STRLEN_SMALL];
extern nrrd_export int nrrdFormatIsAvailable[];
extern nrrd_export int nrrdFormatIsImage[];
extern nrrd_export int nrrdEncodingEndianMatters[];
extern nrrd_export int nrrdEncodingIsCompression[];
extern nrrd_export int nrrdEncodingIsAvailable[];
extern nrrd_export int nrrdTypeSize[];
extern nrrd_export int nrrdTypeInteger[];
extern nrrd_export int nrrdTypeUnsigned[];
extern nrrd_export double nrrdTypeMin[];
extern nrrd_export double nrrdTypeMax[];
extern nrrd_export double nrrdTypeNumberValues[];

/******** things useful with hest */
/* hestNrrd.c */
extern nrrd_export hestCB *nrrdHestNrrd;
extern nrrd_export hestCB *nrrdHestKernelSpec;
extern nrrd_export hestCB *nrrdHestIter;

/******** pseudo-constructors, pseudo-destructors, and such */
/* methodsNrrd.c */
extern NrrdIO *nrrdIONew(void);
extern void nrrdIOReset(NrrdIO *io);
extern NrrdIO *nrrdIONix(NrrdIO *io);
extern NrrdResampleInfo *nrrdResampleInfoNew(void);
extern NrrdResampleInfo *nrrdResampleInfoNix(NrrdResampleInfo *info);
extern NrrdKernelSpec *nrrdKernelSpecNew();
extern void nrrdKernelSpecSet(NrrdKernelSpec *ksp, NrrdKernel *k,
			      double kparm[NRRD_KERNEL_PARMS_NUM]);
extern void nrrdKernelParmSet(NrrdKernel **kP,
			      double kparm[NRRD_KERNEL_PARMS_NUM],
			      NrrdKernelSpec *ksp);
extern NrrdKernelSpec *nrrdKernelSpecNix(NrrdKernelSpec *ksp);
extern void nrrdInit(Nrrd *nrrd);
extern Nrrd *nrrdNew(void);
extern Nrrd *nrrdNix(Nrrd *nrrd);
extern Nrrd *nrrdEmpty(Nrrd *nrrd);
extern Nrrd *nrrdNuke(Nrrd *nrrd);
extern int nrrdWrap_nva(Nrrd *nrrd, void *data, int type, int dim, int *size);
extern int nrrdWrap(Nrrd *nrrd, void *data, int type, int dim,
		    ... /* sx, sy, .., axis(dim-1) size */);
extern Nrrd *nrrdUnwrap(Nrrd *nrrd);
extern int nrrdCopy(Nrrd *nout, Nrrd *nin);
extern int nrrdAlloc_nva(Nrrd *nrrd, int type, int dim, int *size);
extern int nrrdAlloc(Nrrd *nrrd, int type, int dim,
		     ... /* sx, sy, .., axis(dim-1) size */);
extern int nrrdMaybeAlloc_nva(Nrrd *nrrd, int type, int dim, int *size);
extern int nrrdMaybeAlloc(Nrrd *nrrd, int type, int dim,
			  ... /* sx, sy, .., axis(dim-1) size */);
extern int nrrdPPM(Nrrd *, int sx, int sy);
extern int nrrdPGM(Nrrd *, int sx, int sy);
extern int nrrdTable(Nrrd *table, int sx, int sy);

/******** nrrd value iterator gadget */
/* iter.c */
extern NrrdIter *nrrdIterNew(void);
extern void nrrdIterSetValue(NrrdIter *iter, double val);
extern void nrrdIterSetNrrd(NrrdIter *iter, Nrrd *nrrd);
extern double nrrdIterValue(NrrdIter *iter);
extern char *nrrdIterContent(NrrdIter *iter);
extern NrrdIter *nrrdIterNix(NrrdIter *iter);
extern NrrdIter *nrrdIterNuke(NrrdIter *iter);

/******** axes related */
/* axes.c */
extern int nrrdAxesCopy(Nrrd *nout, Nrrd *nin, int *map, int bitflag);
extern void nrrdAxesSet_nva(Nrrd *nin, int axInfo, void *info);
extern void nrrdAxesSet(Nrrd *nin, int axInfo,
			... /* void* */);
extern void nrrdAxesGet_nva(Nrrd *nrrd, int axInfo, void *info);
extern void nrrdAxesGet(Nrrd *nrrd, int axInfo,
			... /* void* */);
extern double nrrdAxisPos(Nrrd *nrrd, int ax, double idx);
extern double nrrdAxisIdx(Nrrd *nrrd, int ax, double pos);
extern void nrrdAxisPosRange(double *loP, double *hiP, Nrrd *nrrd, int ax,
			     double loIdx, double hiIdx);
extern void nrrdAxisIdxRange(double *loP, double *hiP, Nrrd *nrrd, int ax,
			     double loPos, double hiPos);
extern void nrrdAxisSpacingSet(Nrrd *nrrd, int ax);
extern void nrrdAxisMinMaxSet(Nrrd *nrrd, int ax, int defCenter);

/******** simple things */
/* simple.c */
extern int nrrdPeripheralInit(Nrrd *nrrd);
extern int nrrdPeripheralCopy(Nrrd *nout, Nrrd *nin);
extern int nrrdContentSet(Nrrd *nout, const char *func,
			  Nrrd *nin, const char *format,
			  ... /* printf-style arg list */ );
extern void nrrdDescribe(FILE *file, Nrrd *nrrd);
extern int nrrdCheck(Nrrd *nrrd);
extern int nrrdElementSize(Nrrd *nrrd);
extern size_t nrrdElementNumber(Nrrd *nrrd);
extern int nrrdHasNonExistSet(Nrrd *nrrd);
extern int nrrdSanity(void);
extern int nrrdSameSize(Nrrd *n1, Nrrd *n2, int useBiff);
extern int nrrdFitsInFormat(Nrrd *nrrd, int encoding, int format, int useBiff);

/******** comments related */
/* comment.c */
extern int nrrdCommentAdd(Nrrd *nrrd, const char *str);
extern void nrrdCommentClear(Nrrd *nrrd);
extern int nrrdCommentCopy(Nrrd *nout, Nrrd *nin);
extern char *nrrdCommentScan(Nrrd *nrrd, const char *key);

/******** endian related */
/* endianNrrd.c */
extern void nrrdSwapEndian(Nrrd *nrrd);

/******** getting value into and out of an array of general type, and
   all other simplistic functionality pseudo-parameterized by type */
/* accessors.c */
extern nrrd_export int    (*nrrdILoad[NRRD_TYPE_MAX+1])(void *v);
extern nrrd_export float  (*nrrdFLoad[NRRD_TYPE_MAX+1])(void *v);
extern nrrd_export double (*nrrdDLoad[NRRD_TYPE_MAX+1])(void *v);
extern nrrd_export int    (*nrrdIStore[NRRD_TYPE_MAX+1])(void *v, int j);
extern nrrd_export float  (*nrrdFStore[NRRD_TYPE_MAX+1])(void *v, float f);
extern nrrd_export double (*nrrdDStore[NRRD_TYPE_MAX+1])(void *v, double d);
extern nrrd_export int    (*nrrdILookup[NRRD_TYPE_MAX+1])(void *v, size_t I);
extern nrrd_export float  (*nrrdFLookup[NRRD_TYPE_MAX+1])(void *v, size_t I);
extern nrrd_export double (*nrrdDLookup[NRRD_TYPE_MAX+1])(void *v, size_t I);
extern nrrd_export int    (*nrrdIInsert[NRRD_TYPE_MAX+1])(void *v,
							  size_t I, int j);
extern nrrd_export float  (*nrrdFInsert[NRRD_TYPE_MAX+1])(void *v,
							  size_t I, float f);
extern nrrd_export double (*nrrdDInsert[NRRD_TYPE_MAX+1])(void *v,
							  size_t I, double d);
extern nrrd_export int    (*nrrdSprint[NRRD_TYPE_MAX+1])(char *, void *);
extern nrrd_export int    (*nrrdFprint[NRRD_TYPE_MAX+1])(FILE *, void *);
extern nrrd_export float  (*nrrdFClamp[NRRD_TYPE_MAX+1])(float);
extern nrrd_export double (*nrrdDClamp[NRRD_TYPE_MAX+1])(double);
extern nrrd_export void (*nrrdFindMinMax[NRRD_TYPE_MAX+1])(void *minP,
							   void *maxP,
							   Nrrd *nrrd);
extern nrrd_export int (*nrrdValCompare[NRRD_TYPE_MAX+1])(const void *,
							  const void *);

/******** getting information to and from files */
/* read.c */
extern nrrd_export int (*nrrdReadData[NRRD_ENCODING_MAX+1])(Nrrd *, NrrdIO *);
extern int nrrdLineSkip(NrrdIO *io);
extern int nrrdByteSkip(Nrrd *nrrd, NrrdIO *io);
extern int nrrdLoad(Nrrd *nrrd, const char *filename);
extern int nrrdRead(Nrrd *nrrd, FILE *file, NrrdIO *io);
extern void nrrdDirBaseSet(NrrdIO *io, const char *name);
/* write.c */
extern nrrd_export int (*nrrdWriteData[NRRD_ENCODING_MAX+1])(Nrrd *, NrrdIO *);
extern int nrrdSave(const char *filename, Nrrd *nrrd, NrrdIO *io);
extern int nrrdWrite(FILE *file, Nrrd *nrrd, NrrdIO *io);

/******** some of the point-wise value remapping, conversion, and such */
/* map.c */
extern void nrrdMinMaxSet(Nrrd *nrrd);
extern int nrrdMinMaxCleverSet(Nrrd *nrrd);
extern int nrrdConvert(Nrrd *nout, Nrrd *nin, int type);
extern int nrrdQuantize(Nrrd *nout, Nrrd *nin, int bits);
extern int nrrdUnquantize(Nrrd *nout, Nrrd *nin, int type);
extern int nrrdHistoEq(Nrrd *nout, Nrrd *nin, Nrrd **nhistP,
		       int bins, int smart, float amount);

/******** rest of point-wise value remapping, and "color"mapping */
/* apply1D.c */
extern int nrrdApply1DLut(Nrrd *nout, Nrrd *nin, Nrrd *nlut, 
			  int typeOut, int rescale);
extern int nrrdApply1DRegMap(Nrrd *nout, Nrrd *nin, Nrrd *nmap,
			     int typeOut, int rescale);
extern int nrrd1DIrregMapCheck(Nrrd *nmap);
extern int nrrd1DIrregAclGenerate(Nrrd *nacl, Nrrd *nmap, int aclLen);
extern int nrrd1DIrregAclCheck(Nrrd *nacl);
extern int nrrdApply1DIrregMap(Nrrd *nout, Nrrd *nin, Nrrd *nmap, Nrrd *nacl,
			       int typeOut, int rescale);

/******** sampling, slicing, cropping */
/* subset.c */
extern int nrrdSample_nva(void *val, Nrrd *nin, int *coord);
extern int nrrdSample(void *val, Nrrd *nin,
		      ... /* coord0, coord1, .., coord(dim-1) */ );
extern int nrrdSlice(Nrrd *nout, Nrrd *nin, int axis, int pos);
extern int nrrdCrop(Nrrd *nout, Nrrd *nin, int *min, int *max);
extern int nrrdSimpleCrop(Nrrd *nout, Nrrd *nin, int crop);

/******** padding */
/* superset.c */
extern int nrrdSplice(Nrrd *nout, Nrrd *nin, Nrrd *nslice, int axis, int pos);
extern int nrrdPad_nva(Nrrd *nout, Nrrd *nin, int *min, int *max,
		       int boundary, double padValue);
extern int nrrdPad(Nrrd *nout, Nrrd *nin, int *min, int *max, int boundary,
		   ... /* if nrrdBoundaryPad, what value */);
extern int nrrdSimplePad_nva(Nrrd *nout, Nrrd *nin, int pad,
			     int boundary, double padValue);
extern int nrrdSimplePad(Nrrd *nout, Nrrd *nin, int pad, int boundary,
			 ... /* if nrrdBoundaryPad, what value */);
extern int nrrdInset(Nrrd *nout, Nrrd *nin, Nrrd *nsub, int *min);

/******** permuting, shuffling, and all flavors of reshaping */
/* reorder.c */
extern int nrrdInvertPerm(int *invp, int *perm, int n);
extern int nrrdAxesPermute(Nrrd *nout, Nrrd *nin, int *axes);
extern int nrrdAxesSwap(Nrrd *nout, Nrrd *nin, int ax1, int ax2);
extern int nrrdShuffle(Nrrd *nout, Nrrd *nin, int axis, int *perm);
extern int nrrdFlip(Nrrd *nout, Nrrd *nin, int axis);
extern int nrrdJoin(Nrrd *nout, Nrrd **nin, int numNin, int axis, int incrDim);
extern int nrrdReshape(Nrrd *nout, Nrrd *nin, int dim,
		       ... /* sx, sy, .., axis(dim-1) size */ );
extern int nrrdReshape_nva(Nrrd *nout, Nrrd *nin, int dim, int *size);
extern int nrrdAxesInsert(Nrrd *nout, Nrrd *nin, int ax);
extern int nrrdAxesDelete(Nrrd *nout, Nrrd *nin, int ax);
extern int nrrdBlock(Nrrd *nout, Nrrd *nin);
extern int nrrdUnblock(Nrrd *nout, Nrrd *nin, int type);

/******** measuring and projecting */
/* measure.c */
extern nrrd_export void (*nrrdMeasureLine[NRRD_MEASURE_MAX+1])(void *ans,
							       int ansType,
						   void *line, int lineType,
						   int lineLen, 
						   double axMin, double axMax);
extern int nrrdProject(Nrrd *nout, Nrrd *nin, int axis, int measr);

/********* various kinds of histograms */
/* histogram.c */
extern int nrrdHisto(Nrrd *nout, Nrrd *nin, Nrrd *nwght, int bins, int type);
extern int nrrdHistoDraw(Nrrd *nout, Nrrd *nin, int sy,
			 int showLog, double max);
extern int nrrdHistoAxis(Nrrd *nout, Nrrd *nin, int axis, int bins, int type);
extern int nrrdHistoJoint(Nrrd *nout, Nrrd **nin, int numNin,
			  Nrrd *nwght, int *bins, int type, int *clamp);

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
/*
******** nrrdResample_t typedef
**
** type used to hold filter sample locations and weights in
** nrrdSpatialResample(), and to hold the intermediate sampling
** results.  Not as good as templating, but better than hard-coding
** float versus double.  Actually, the difference between float and
** double not exposed in any functions or objects declared in this
** header; it is entirely internal to the operation of
** nrrdSpatialResample().
**
** Choose by setting "#if" arg to 1 (for float) or 0 (for double)
*/
#if 1
typedef float nrrdResample_t;
#  define NRRD_RESAMPLE_FLOAT 1
#else
typedef double nrrdResample_t;
#  define NRRD_RESAMPLE_FLOAT 0
#endif

/* resampleNrrd.c */
extern int nrrdSpatialResample(Nrrd *nout, Nrrd *nin, NrrdResampleInfo *info);
extern int nrrdSimpleResample(Nrrd *nout, Nrrd *nin,
			      NrrdKernel *kernel, double *parm,
			      int *samples, double *scalings);

/******** kernels (interpolation, 1st and 2nd derivatives) */
/* tmfKernel.c
   nrrdKernelTMF[D][C][A] is d<D>_c<C>_<A>ef:
   Dth-derivative, C-order continuous ("smooth"), A-order accurate */
extern nrrd_export NrrdKernel *nrrdKernelTMF[3][4][5];
extern nrrd_export int nrrdKernelTMF_maxD;
extern nrrd_export int nrrdKernelTMF_maxC;
extern nrrd_export int nrrdKernelTMF_maxA;
/* winKernel.c : various kinds of windowed sincs */
extern nrrd_export NrrdKernel *nrrdKernelHann;
extern nrrd_export NrrdKernel *nrrdKernelHannD;
extern nrrd_export NrrdKernel *nrrdKernelHannDD;
extern nrrd_export NrrdKernel *nrrdKernelBlackman;
extern nrrd_export NrrdKernel *nrrdKernelBlackmanD;
extern nrrd_export NrrdKernel *nrrdKernelBlackmanDD;
/* kernel.c */
extern nrrd_export NrrdKernel *nrrdKernelZero, /* zero everywhere */
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
extern int nrrdKernelParse(NrrdKernel **kernelP, double *parm,
			   const char *str);

#ifdef __cplusplus
}
#endif

#endif /* NRRD_HAS_BEEN_INCLUDED */
