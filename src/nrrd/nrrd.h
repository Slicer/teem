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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <errno.h>

#include <air.h>
#include <biff.h>

#include "nrrdDefines.h"
#include "nrrdAccessors.h"

#define NRRD "nrrd"

/*
******** Nrrd struct
**
** The struct used to wrap around the raw data array
*/
typedef struct {
  /* 
  ** NECESSARY information describing the main array, its
  ** representation, and its storage.  This is generally set at the
  ** same time that either the nrrd is created, or at the time that an
  ** existing nrrd is wrapped around an array 
  */
  void *data;                    /* the data in memory */
  NRRD_BIG_INT num;              /* number of elements */
  int type;                      /* a value from the nrrdType enum */
  int dim;                       /* what is dimension of data */
  int encoding;                  /* how the data will be read from or 
				    should be written to file */

  /* 
  ** Information about the individual axes of the data's domain 
  */
  int size[NRRD_MAX_DIM];        /* number of elements along each axis */
  double spacing[NRRD_MAX_DIM];  /* if non-NaN, distance between samples */
  double axisMin[NRRD_MAX_DIM];  /* if non-NaN, value associated with
				    lowest index */
  double axisMax[NRRD_MAX_DIM];  /* if non-NaN, value associated with
				    highest index.  Obviously one can
				    set "spacing" to something incompatible
				    with axisMin, axisMax.  No clear policy 
				    on this one yet */
  char label[NRRD_MAX_DIM][NRRD_SMALL_STRLEN];  
                                 /* short info string for each axis */

  /* 
  ** Information of dubious standing- descriptive of whole array, but
  ** not necessary (meaningful only for some uses of a nrrd), but basic
  ** enough to be part of the basic nrrd type
  */
  char content[NRRD_MED_STRLEN]; /* briefly, just what the hell is this data */
  int blockSize;                 /* for nrrdTypeBlock array, block byte size */
  double min, max,               /* if non-NaN, extremal values for array */
    oldMin, oldMax;              /* if non-NaN, and if nrrd is of integral
				    type, extremal values for the array
				    BEFORE it was quantized */
  void *ptr;                     /* generic pointer which is NEVER read or
				    set by nrrd library, but which may come
				    in handy for the user; obviously its
				    maintainence is completely up to user */

  /* 
  ** Comments.  Read from and written to header.
  ** --> This is the ONLY part of the struct <--
  ** -->   which is dynamically allocated.   <--
  ** (that is, besides the void *data pointer)
  */
  int numComments;
  char **comment;

  /*
  ** Transient I/O information: information which potentially modifies
  ** how data will be read or written, but which has no relevance
  ** after that time, and never any relevance for nrrds which aren't
  ** born of or headed for the filesystem 
  */
  char dir[NRRD_BIG_STRLEN],     /* allows us to remember the directory
				    from whence this nrrd was "open"ed,
				    or to whence this nrrd is "save"ed,
				    so as to facilitate games with data
				    files relative to header files */
    name[NRRD_MED_STRLEN];       /* when "save"ing a nrrd into seperate 
				    header and data, the name of the data 
				    file as it will be recorded in the 
				    header */
  FILE *dataFile;                /* if non-NULL, where the data is to
				    be read from or written to.  If
				    NULL, data will be read from
				    current file */
  int lineSkip,                  /* if dataFile non-NULL, the number of lines
				    in dataFile that should be skipped over
				    (so as to bypass another form of ASCII
				    header preceeding raw data) */
    byteSkip,                    /* exactly like lineSkip, but bytes 
				    instead of lines.  First the lines are
				    skipped, then the bytes */
    fileEndian;                  /* endien-ness of the data written to file,
				    (for those encodings for which it 
				    matters) */
  
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
  float (*eval)(float x, float *param);   /* evaluate the kernel once */
  void (*evalVec)(float *f, float *x,     /* evaluate many times */
		  int N, float *param);   
  double (*evalD)(double x, float *param);/* evaluate once, double precision */
  void (*evalVecD)(double *f, double *x,  /* evaluate many times, double */
		   int N, float *param);
} nrrdKernel;

/*
******** nrrdResampleInfo struct
**
*/
typedef struct {
  nrrdKernel *kernel[NRRD_MAX_DIM];       /* kernels from nrrd, or something
					     supplied by the user */
  int samples[NRRD_MAX_DIM];              /* number of samples */
  float param[NRRD_MAX_DIM][NRRD_MAX_KERNEL_PARAMS], /* kernel arguments */
    min[NRRD_MAX_DIM],
    max[NRRD_MAX_DIM];           /* range, in index space, along which to
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

/*
******** nrrdBoundary enum
**
** when resampling, how to deal with the ends of a scanline
*/
typedef enum {
  nrrdBoundaryUnknown,  /* 0: who knows? */
  nrrdBoundaryPad,      /* 1: fill with some user-specified value */
  nrrdBoundaryBleed,    /* 2: copy the last/first value out as needed */
  nrrdBoundaryWrap,     /* 3: wrap-around */
  nrrdBoundaryWeight,   /* 4: normalize the weighting on the existing samples;
			   ONLY sensible for a strictly positive kernel
			   which integrates to unity (as in blurring) */
  nrrdBoundaryLast
} nrrdBoundary;

/*
******** nrrdMagic enum
**
** the different "magic numbers" that nrrd knows about.  Not as useful
** as you might want- the readers (io.c) can (currently) only deal
** with a magic which is on a line on its own, with a carraige return.
** This is the case for the nrrd magic, and for the PNMs written by xv
** and every other paint program I've run into.
** (HOWEVER: PNMs do not _require_ a carriage return after the magic) 
*/
typedef enum {
  nrrdMagicUnknown,      /* 0: nobody knows! */
  nrrdMagicNrrd0001,     /* 1: currently, the only "native" nrrd header */
  nrrdMagicP1,           /* 2: ascii PBM */
  nrrdMagicP2,           /* 3: ascii PGM */
  nrrdMagicP3,           /* 4: ascii PPM */
  nrrdMagicP4,           /* 5: binary PBM */
  nrrdMagicP5,           /* 6: binary PGM */
  nrrdMagicP6,           /* 7: binary PPM */
  nrrdMagicLast          /* 8: after the last valid magic */
} nrrdMagicType;

/*
******** nrrdType enum
**
** all the different types, identified by integer
** must be in sync with NRRD_MAX_TYPE_SIZE in nrrdDefines.h
** nrrdTypeBlock must precede nrrdTypeLast
*/
typedef enum {
  nrrdTypeUnknown,       /*  0: nobody knows! */
  nrrdTypeChar,          /*  1:   signed 1-byte integer */
  nrrdTypeUChar,         /*  2: unsigned 1-byte integer */
  nrrdTypeShort,         /*  3:   signed 2-byte integer */
  nrrdTypeUShort,        /*  4: unsigned 2-byte integer */
  nrrdTypeInt,           /*  5:   signed 4-byte integer */
  nrrdTypeUInt,          /*  6: unsigned 4-byte integer */
  nrrdTypeLLong,         /*  7:   signed 8-byte integer */
  nrrdTypeULLong,        /*  8: unsigned 8-byte integer */
  nrrdTypeFloat,         /*  9:          4-byte floating point */
  nrrdTypeDouble,        /* 10:          8-byte floating point */
  nrrdTypeLDouble,       /* 11:         16-byte floating point */
  nrrdTypeBlock,         /* 12: size user defined at run time */
  nrrdTypeLast           /* 13: after the last valid type */
} nrrdType;

/*
******** nrrdEncoding enum
**
** how data might be encoded into a bytestream
*/
typedef enum {
  nrrdEncodingUnknown,    /* 0: nobody knows */
  nrrdEncodingRaw,        /* 1: file reflects memory (fread/fwrite) */
  nrrdEncodingZlib,       /* 2: using the zlib compression library */
  nrrdEncodingAscii,      /* 3: decimal values are spelled out in ascii */
  nrrdEncodingHex,        /* 4: hexadecimal encoding (two chars per byte) */
  nrrdEncodingBase85,     /* 5: using base-85 (as in Postscript Level 2) */
  nrrdEncodingUser,       /* 6: something the user choses and sets */
  nrrdEncodingLast        /* 7: after last valid one */
} nrrdEncoding;

/*
******** nrrdMeasr enum
**
** ways to "measure" some portion of the array
** NEEDS TO BE IN SYNC WITH nrrdMeasr array in measr.c
*/
#define NRRD_MEASR_MAX 18
typedef enum {
  nrrdMeasrUnknown,          /* 0: nobody knows */
  nrrdMeasrMin,              /* 1: smallest value */
  nrrdMeasrMax,              /* 2: biggest value */
  nrrdMeasrProduct,          /* 3: product of all values */
  nrrdMeasrSum,              /* 4: sum of all values */
  nrrdMeasrMean,             /* 5: average of values */
  nrrdMeasrMedian,           /* 6: value at 50th percentile */
  nrrdMeasrMode,             /* 7: most common value */
  nrrdMeasrL1,               /* 8 */
  nrrdMeasrL2,               /* 9 */
  nrrdMeasrLinf,             /* 10 */
  /* 
  ** these nrrdMeasrHisto* measures interpret the array as a histogram
  ** of some implied value distribution
  */
  nrrdMeasrHistoMin,         /* 11 */
  nrrdMeasrHistoMax,         /* 12 */
  nrrdMeasrHistoProduct,     /* 13 */
  nrrdMeasrHistoSum,         /* 14 */
  nrrdMeasrHistoMean,        /* 15 */
  nrrdMeasrHistoMedian,      /* 16 */
  nrrdMeasrHistoMode,        /* 17 */
  nrrdMeasrHistoVariance,    /* 18 */
  nrrdMeasrLast
} nrrdMeasrs;

/*
******** nrrdEndian enum
**
** for identifying how a file was written to disk, for those encodings
** where the data on disk is dependent on the endianness of the
** architecture.  There could potentially be more endiannesses
** at some future date.
*/
typedef enum {
  nrrdEndianUnknown,         /* 0: nobody knows */
  nrrdEndianLittle,          /* 1: Intel and friends */
  nrrdEndianBig,             /* 2: the rest */
  nrrdEndianLast
} nrrdEndians;

/*
** these arrays are all in arrays.c
*/

/*
******** nrrdMagic2Str[][]
**
** the actual strings for each magic
*/
extern char nrrdMagic2Str[][NRRD_SMALL_STRLEN];

/*
******** nrrdType2Str[][]
**
** strings for each type, except the non-type nrrdTypeLast.
** These are written in the NRRD header; it is only for the sake
** of clarity and simplicity that they also happen to be valid C types.
** There is more flexibility in interpreting the type read from the NRRD
** header, see nrrdStr2Type.
** (Besides the enum above, the actual C types used internally 
** are in nrrdDefines.h)
*/
extern char nrrdType2Str[][NRRD_SMALL_STRLEN];

/*
******** nrrdType2Conv[][]
**
** conversion sequence for each type, (like "%d" for nrrdTypeInt)
*/
extern char nrrdType2Conv[][NRRD_SMALL_STRLEN];

/*
******** nrrdTypeSize[]
**
** expected sizes of the types, except for the non-type nrrdTypeLast
*/
extern int nrrdTypeSize[];

/*
******** nrrdEncoding2Str[][]
**
** strings for each encoding type, except the non-type nrrdEncodeLast
*/
extern char nrrdEncoding2Str[][NRRD_SMALL_STRLEN];

/*
******** nrrdEndian2Str[][]
**
** strings for each endianness type
*/
extern char nrrdEndian2Str[][NRRD_SMALL_STRLEN];

/*
******** nrrdEncodingEndianMatters[]
**
** indexed by encoding type, non-zero iff encoding exposes endianness
*/
extern int nrrdEncodingEndianMatters[];

/******** type related */
/* (types.c) */
/* (tested in typestest.c) */
extern int nrrdStr2Type(char *str);

/******** endian related */
/* (endian.c) */
extern int nrrdStr2Endian(char *str);
extern int nrrdMyEndian();
extern void nrrdSwapEndian();

/******** making and destroying nrrds and basic info within */
/* (methods.c) */
extern Nrrd *nrrdNew(void);
extern void nrrdInit(Nrrd *nrrd);
extern Nrrd *nrrdNix(Nrrd *nrrd);
extern Nrrd *nrrdUnwrap(Nrrd *nrrd);  /* same as nrrdNix() */
extern void nrrdEmpty(Nrrd *nrrd);
extern Nrrd *nrrdNuke(Nrrd *nrrd);
extern void nrrdWrap(Nrrd *nrrd, void *data, 
		     NRRD_BIG_INT num, int type, int dim);
extern Nrrd *nrrdNewWrap(void *data, NRRD_BIG_INT num, int type, int dim);
extern void nrrdSetInfo(Nrrd *nrrd, NRRD_BIG_INT num, int type, int dim);
extern Nrrd *nrrdNewSetInfo(NRRD_BIG_INT num, int type, int dim);
extern int nrrdAlloc(Nrrd *nrrd, NRRD_BIG_INT num, int type, int dim);
extern Nrrd *nrrdNewAlloc(NRRD_BIG_INT num, int type, int dim);
extern Nrrd *nrrdNewPPM(int sx, int sy);
extern Nrrd *nrrdNewPGM(int sx, int sy);
extern void nrrdAddComment(Nrrd *nrrd, char *cmt);
extern void nrrdClearComments(Nrrd *nrrd);
extern int nrrdScanComments(Nrrd *nrrd, char *key, char **valP);
extern void nrrdDescribe(FILE *file, Nrrd *nrrd);
extern int nrrdCheck(Nrrd *nrrd);
extern int nrrdRange(double *minP, double *maxP, Nrrd *nrrd);
extern int nrrdCopy(Nrrd *nout, Nrrd *nin);
extern Nrrd *nrrdNewCopy(Nrrd *nin);
extern int nrrdSameSize(Nrrd *n1, Nrrd *n2);
extern nrrdResampleInfo *nrrdResampleInfoNew(void);
extern nrrdResampleInfo *nrrdResampleInfoNix(nrrdResampleInfo *info);
extern int nrrdElementSize(Nrrd *nrrd);

/******** getting information to and from files */
/* io.c */
extern int nrrdLoad(char *name, Nrrd *nrrd);
extern Nrrd *nrrdNewLoad(char *name);
extern int nrrdSave(char *name, Nrrd *nrrd);
extern int nrrdOneLine(FILE *file, char *line, int size);
typedef int (*NrrdReadDataType)(FILE *file, Nrrd *nrrd);
typedef int (*NrrdWriteDataType)(FILE *file, Nrrd *nrrd);
extern int nrrdRead(FILE *file, Nrrd *nrrd);
extern Nrrd *nrrdNewRead(FILE *file);
extern int nrrdReadMagic(FILE *file);
extern int nrrdReadHeader(FILE *file, Nrrd *nrrd); 
extern Nrrd *nrrdNewReadHeader(FILE *file); 
extern int nrrdReadData(FILE *file, Nrrd *nrrd);
extern int nrrdReadDataRaw(FILE *file, Nrrd *nrrd);
extern int nrrdReadDataZlib(FILE *file, Nrrd *nrrd);
extern int nrrdReadDataAscii(FILE *file, Nrrd *nrrd);
extern int nrrdReadDataHex(FILE *file, Nrrd *nrrd);
extern int nrrdReadDataBase85(FILE *file, Nrrd *nrrd);
extern int (*nrrdReadDataFptr[])(FILE *, Nrrd *);
extern int nrrdWrite(FILE *file, Nrrd *nrrd);
extern int nrrdWriteHeader(FILE *file, Nrrd *nrrd);
extern int nrrdWriteData(FILE *file, Nrrd *nrrd);
extern int nrrdWriteDataRaw(FILE *file, Nrrd *nrrd);
extern int nrrdWriteDataZlib(FILE *file, Nrrd *nrrd);
extern int nrrdWriteDataAscii(FILE *file, Nrrd *nrrd);
extern int nrrdWriteDataHex(FILE *file, Nrrd *nrrd);
extern int nrrdWriteDataBase85(FILE *file, Nrrd *nrrd);
extern int (*nrrdWriteDataFptr[])(FILE *, Nrrd *);
extern int nrrdValidPNM(Nrrd *pnm, int useBiff);
extern int nrrdReadPNMHeader(FILE *file, Nrrd *nrrd, int magic);
extern int nrrdWritePNM(FILE *file, Nrrd *nrrd);

/* getting value into and out of an array of general type, and
   other simplistic functionality pseudo-parameterized by type */
/* accessors.c */
extern int    (*nrrdILoad[13])(void *v);
extern float  (*nrrdFLoad[13])(void *v);
extern double (*nrrdDLoad[13])(void *v);
extern int    (*nrrdIStore[13])(void *v, int j);
extern float  (*nrrdFStore[13])(void *v, float f);
extern double (*nrrdDStore[13])(void *v, double d);
extern int    (*nrrdILookup[13])(void *v, NRRD_BIG_INT I);
extern float  (*nrrdFLookup[13])(void *v, NRRD_BIG_INT I);
extern double (*nrrdDLookup[13])(void *v, NRRD_BIG_INT I);
extern int    (*nrrdIInsert[13])(void *v, NRRD_BIG_INT I, int j);
extern float  (*nrrdFInsert[13])(void *v, NRRD_BIG_INT I, float f);
extern double (*nrrdDInsert[13])(void *v, NRRD_BIG_INT I, double d);
extern int    (*nrrdSprint[13])(char *, void *);
extern int    (*nrrdFprint[13])(FILE *, void *);
extern float  (*nrrdFClamp[13])(float);
extern double (*nrrdDClamp[13])(double);
extern void   (*nrrdMinMax[13])(void *, void *, NRRD_BIG_INT, void *);

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
extern int nrrdShuffle(Nrrd *nout, Nrrd *nin, int axis, int *perm);
extern int nrrdJoin(Nrrd *nout, Nrrd **nin, int num, int axis, int incrDim);
extern int nrrdFlip(Nrrd *nout, Nrrd *nin, int axis);

/******** measuring and projecting */
/* measr.c */
extern void (*nrrdMeasr[NRRD_MEASR_MAX+1])(void *, int, int, float, float,
					   void *, int);
extern int nrrdMeasureAxis(Nrrd *nout, Nrrd *nin, int axis, int measr);

/********* HISTOGRAMS!!! */
/* histogram.c */
extern int nrrdHistoAxis(Nrrd *nout, Nrrd *nin, int axis, int bins);
extern int nrrdHistoMulti(Nrrd *nout, Nrrd **nin, 
			  int num, int *bin, 
			  float *min, float *max, int *clamp);
extern int nrrdHisto(Nrrd *nout, Nrrd *nin, int bins);
extern int nrrdHistoDraw(Nrrd *nout, Nrrd *nin, int sy);
extern int nrrdHistoEq(Nrrd *nin, Nrrd **nhistP, int bins, int smart);

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

/******** conversions */
/* convert.c */
extern int nrrdConvert(Nrrd *nout, Nrrd *nin, int type);
extern int nrrdQuantize(Nrrd *nout, Nrrd *nin, float min, float max, int bits);


/* extern C */
#ifdef __cplusplus
}
#endif

#endif /* NRRD_HAS_BEEN_INCLUDED */



