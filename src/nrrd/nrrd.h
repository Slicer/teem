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

#ifndef NRRD_HAS_BEEN_INCLUDED
#define NRRD_HAS_BEEN_INCLUDED

#include <errno.h>

/* ---- BEGIN non-NrrdIO */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <float.h>

#include <teem/air.h>
#include <teem/hest.h>
#include <teem/biff.h>

#include "nrrdDefines.h"
#include "nrrdMacros.h"
#include "nrrdEnums.h"
/* ---- END non-NrrdIO */

#ifdef __cplusplus
extern "C" {
#endif

#define NRRD nrrdBiffKey

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
  double oldMin, oldMax;         /* if non-NaN, and if nrrd is of integral
				    type, extremal values for the array
				    BEFORE it was quantized */
  void *ptr;                     /* never read or set by nrrd; use/abuse
				    as you see fit */

  /* 
  ** Comments.  Read from, and written to, header.
  ** The comment array "cmt" is NOT NULL-terminated.
  ** The number of comments is cmtArr->len.
  */
  char **cmt;
  airArray *cmtArr;

  /*
  ** Key-value pairs.
  */
  char **kvp;
  airArray *kvpArr;
} Nrrd;

struct NrrdIoState_t;
struct NrrdEncoding_t;

/*
******** NrrdFormat
**
** All information and behavior relevent to one datafile format
*/
typedef struct {
  char name[AIR_STRLEN_SMALL];    /* short identifying string */
  int isImage,    /* this format is intended solely for "2D" images, which
		     controls the invocation of _nrrdReshapeUpGrayscale()
		     if nrrdStateGrayscaleImage3D */
    readable,     /* we can read as well as write this format */
    usesDIO;      /* this format can use Direct IO */

  /* tests if this format is currently available in this build */
  int (*available)(void);

  /* (for writing) returns non-zero if a given filename could likely be
     represented by this format */
  int (*nameLooksLike)(const char *filename);

  /* (for writing) returns non-zero if a given nrrd/encoding pair will fit
     in this format */
  int (*fitsInto)(const Nrrd *nrrd, const struct NrrdEncoding_t *encoding, 
		   int useBiff);

  /* (for reading) returns non-zero if what has been read in so far 
     is recognized as the beginning of this format */
  int (*contentStartsLike)(struct NrrdIoState_t *nio);

  /* reader and writer */
  int (*read)(FILE *file, Nrrd *nrrd, struct NrrdIoState_t *nio);
  int (*write)(FILE *file, const Nrrd *nrrd, struct NrrdIoState_t *nio);
} NrrdFormat;

/*
******** NrrdEncoding
**
** All information and behavior relevent to one way of encoding data
**
** The data readers are responsible for memory allocation.
** This is necessitated by the memory restrictions of direct I/O
*/
typedef struct NrrdEncoding_t {
  char name[AIR_STRLEN_SMALL],    /* short identifying string */
    suffix[AIR_STRLEN_SMALL];     /* costumary filename suffix */
  int endianMatters,
    isCompression;
  int (*available)(void);
  int (*read)(Nrrd *nrrd, struct NrrdIoState_t *nio);
  int (*write)(const Nrrd *nrrd, struct NrrdIoState_t *nio);
} NrrdEncoding;

/*
******** NrrdIoState struct
**
** Everything transient relating to how the nrrd is read and written.
** Once the nrrd has been read or written, this information is moot,
** except that after reading, it is a potentially useful record of what
** it took to read in a nrrd, and it is the mechanism for hacks like
** keepNrrdDataFileOpen
*/
typedef struct NrrdIoState_t {
  char *path,               /* allows us to remember the directory
			       from whence this nrrd was "load"ed, or
			       to whence this nrrd is "save"ed, MINUS the
			       trailing "/", so as to facilitate games with
			       header-relative data files */
    *base,                  /* when "save"ing a nrrd into seperate
			       header and data, the name of the header
			       file (e.g. "output.nhdr") MINUS the ".nhdr".
			       This is  massaged to produce a header-
			       relative data filename.  */
    *dataFN,                /* ON READ: no semantics 
			       ON WRITE: name to be saved in the "data file"
			       field of the nrrd, either verbatim from
			       "unu make -h", or, internally, created based
			       on detached header path and name */
    *line;                  /* buffer for saving one line from file */
  
  int lineLen,              /* allocated size of line, including the
			       last character for \0 */
    pos;                    /* line[pos] is beginning of stuff which
			       still has yet to be parsed */

  FILE *dataFile;           /* if non-NULL, where the data is to be
			       read from or written to.  If NULL, data
			       will be read from current file */

  int endian,               /* endian-ness of the data in file, for
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
    seen[NRRD_FIELD_MAX+1], /* for error checking in header parsing */
    detachedHeader,         /* ON READ+WRITE: nrrd is split into distinct
			       header and data (for nrrd format only) */
    bareText,               /* when writing a plain text file, is there any
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
    skipData,               /* if non-zero (all formats):
			       ON READ: don't allocate memory for, and don't
			       read in, the data portion of the file (but we
			       do verify that for nrrds, detached datafiles
			       can be opened).  Note: Does NOT imply 
			       keepNrrdDataFileOpen.  Warning: resulting
			       nrrd struct will have "data" pointer NULL.
			       ON WRITE: don't write data portion of file
			       (for nrrds, don't even try to open detached
			       datafiles).  Warning: can result in broken
			       noncomformant files.
			       (be careful with this) */
    keepNrrdDataFileOpen,   /* ON READ: don't close nio->dataFile when
			       you otherwise would, when reading the
			       nrrd format Probably used in conjunction with
			       skipData.  (currently for "unu data")
			       ON WRITE: no semantics */
    zlibLevel,              /* zlib compression level (0-9, -1 for
			       default[6], 0 for no compression). */
    zlibStrategy,           /* zlib compression strategy, can be one
			       of the nrrdZlibStrategy enums, default is
			       nrrdZlibStrategyDefault. */
    bzip2BlockSize;         /* block size used for compression, 
			       roughly equivalent to better but slower
			       (1-9, -1 for default[9]). */
  void *oldData;            /* ON READ: data pointer that may have already been
			       allocated for the right size to hold the data */
  size_t oldDataSize;       /* ON READ: size of data pointed to by oldData */
  /* format and encoding.  These are initialized to nrrdFormatUnknown
     and nrrdEncodingUnknown, respectively. USE THESE VALUES for 
     any kind of initialization or flagging; DO NOT USE NULL */
  const NrrdFormat *format;
  const NrrdEncoding *encoding;
} NrrdIoState;

/* ---- BEGIN non-NrrdIO */

/*
******** NrrdRange
**
** information about a range of values, used as both a description
** of an existing nrrd, or as input to functions like nrrdQuantize
** (in which case the given min,max may not correspond to the actual
** min,max of the nrrd in question).
**
** This information has been removed from the Nrrd struct (as of teem1.6) 
** and put into this seperate entity because:
** 1) when intended to be descriptive of a nrrd, it can't be guaranteed
** to be true across nrrd calls
** 2) when used as input parameters (e.g. to nrrdQuantize), its not
** data-flow friendly (you can't modify input)
*/
typedef struct {
  double min, max;  /* if non-NaN, nominally: extremal values for array, but
		       practically: the min and max values to use for nrrd
		       calls for which a min and max values are used */
  int hasNonExist;  /* from the nrrdHasNonExist* enum values */
} NrrdRange;

/*
******** NrrdKernel struct
**
** these are essentially the methods of the various kernels implemented.
**
** Nrrd's use of this sort of kernel always assumes support symmetric
** around zero, but does not assume anything about even- or oddness
**
** It is a strong but very simplifying assumption that the parameter
** array ("parm") is always type double.  There is essentially no
** value in allowing flexibility between float and double, and much
** teem code assumes that it will always be type double.
*/
typedef struct {
  /* terse string representation of kernel function, irrespective of
     the parameter vector */
  char name[AIR_STRLEN_SMALL];
  
  /* number of parameters needed (# elements in parm[] used) */
  int numParm;  

  /* smallest x (x > 0) such that k(y) = 0 for all y > x, y < -x */
  double (*support)(const double *parm); 

  /* integral of kernel from -support to +support */
  double (*integral)(const double *parm);

  /* evaluate once, single precision */
  float (*eval1_f)(float x, const double *parm);
  
  /* evaluate N times, single precision */
  void (*evalN_f)(float *f, const float *x, size_t N, const double *parm);   

  /* evaluate once, double precision */
  double (*eval1_d)(double x, const double *parm);

  /* eval. N times, double precision */
  void (*evalN_d)(double *f, const double *x, size_t N, const double *parm);
} NrrdKernel;

/*
******** NrrdKernelSpec struct
** 
** for those times when it makes most sense to directly associate a
** NrrdKernel with its parameter vector (that is, a full kernel
** "spec"ification), basically: using hest.
*/
typedef struct {
  const NrrdKernel *kernel;
  double parm[NRRD_KERNEL_PARMS_NUM];
} NrrdKernelSpec;

/*
******** NrrdResampleInfo struct
**
** a struct to contain the many parameters needed for nrrdSpatialResample()
*/
typedef struct {
  const NrrdKernel 
    *kernel[NRRD_DIM_MAX];   /* which kernel to use on each axis; use NULL to
				say no resampling whatsoever on this axis */
  int samples[NRRD_DIM_MAX]; /* number of samples per axis */
  double parm[NRRD_DIM_MAX][NRRD_KERNEL_PARMS_NUM], /* kernel arguments */
    min[NRRD_DIM_MAX],
    max[NRRD_DIM_MAX];       /* min[i] and max[i] are the range, in WORLD
				space, along which to resample
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
  const Nrrd *nrrd;            /* read-only nrrd to get values from */
  Nrrd *ownNrrd;               /* another nrrd to get values from, which we
			          do "own", and do delete on nrrdIterNix */
  double val;                  /* single fixed value */
  int size;                    /* type size */
  char *data;                  /* where to get the next value */
  size_t left;                 /* number of values beyond what "data"
				  currently points to */
  double (*load)(const void*); /* how to get a value out of "data" */
} NrrdIter;

/* ---- END non-NrrdIO */

/******** defaults (nrrdDef..) and state (nrrdState..) */
/* defaultsNrrd.c */
TEEM_API const NrrdEncoding *nrrdDefWriteEncoding;
TEEM_API int nrrdDefWriteBareText;
TEEM_API int nrrdDefWriteCharsPerLine;
TEEM_API int nrrdDefWriteValsPerLine;
/* ---- BEGIN non-NrrdIO */
TEEM_API int nrrdDefRsmpBoundary;
TEEM_API int nrrdDefRsmpType;
TEEM_API double nrrdDefRsmpScale;
TEEM_API int nrrdDefRsmpRenormalize;
TEEM_API int nrrdDefRsmpRound;
TEEM_API int nrrdDefRsmpClamp;
TEEM_API double nrrdDefRsmpPadValue;
TEEM_API double nrrdDefKernelParm0;
/* ---- END non-NrrdIO */
TEEM_API int nrrdDefCenter;
TEEM_API double nrrdDefSpacing;
TEEM_API int nrrdStateVerboseIO;
/* ---- BEGIN non-NrrdIO */
TEEM_API int nrrdStateBlind8BitRange;
TEEM_API int nrrdStateMeasureType;
TEEM_API int nrrdStateMeasureModeBins;
TEEM_API int nrrdStateMeasureHistoType;
TEEM_API int nrrdStateDisallowIntegerNonExist;
/* ---- END non-NrrdIO */
TEEM_API int nrrdStateAlwaysSetContent;
TEEM_API int nrrdStateDisableContent;
TEEM_API char *nrrdStateUnknownContent;
TEEM_API int nrrdStateGrayscaleImage3D;
TEEM_API int nrrdStateKeyValueReturnInternalPointers;
/* ---- BEGIN non-NrrdIO */
TEEM_API void nrrdDefGetenv(void);
TEEM_API void nrrdStateGetenv(void);
/* ---- END non-NrrdIO */

/******** all the airEnums used through-out nrrd */
/* (the actual C enums are in nrrdEnums.h) */
/* enumsNrrd.c */
TEEM_API airEnum *nrrdFormatType;
TEEM_API airEnum *nrrdType;
TEEM_API airEnum *nrrdEncodingType;
TEEM_API airEnum *nrrdCenter;
TEEM_API airEnum *nrrdAxisInfo;
TEEM_API airEnum *nrrdField;
/* ---- BEGIN non-NrrdIO */
TEEM_API airEnum *nrrdBoundary;
TEEM_API airEnum *nrrdMeasure;
TEEM_API airEnum *nrrdUnaryOp;
TEEM_API airEnum *nrrdBinaryOp;
TEEM_API airEnum *nrrdTernaryOp;
/* ---- END non-NrrdIO */

/******** arrays of things (poor-man's functions/predicates) */
/* arraysNrrd.c */
TEEM_API char nrrdTypePrintfStr[][AIR_STRLEN_SMALL];
TEEM_API int nrrdTypeSize[];
TEEM_API double nrrdTypeMin[];
TEEM_API double nrrdTypeMax[];
TEEM_API int nrrdTypeIsIntegral[];
TEEM_API int nrrdTypeIsUnsigned[];
TEEM_API double nrrdTypeNumberOfValues[];

/******** pseudo-constructors, pseudo-destructors, and such */
/* methodsNrrd.c */
TEEM_API NrrdIoState *nrrdIoStateNew(void);
TEEM_API void nrrdIoStateInit(NrrdIoState *io);
TEEM_API NrrdIoState *nrrdIoStateNix(NrrdIoState *io);
/* ---- BEGIN non-NrrdIO */
TEEM_API NrrdResampleInfo *nrrdResampleInfoNew(void);
TEEM_API NrrdResampleInfo *nrrdResampleInfoNix(NrrdResampleInfo *info);
TEEM_API NrrdKernelSpec *nrrdKernelSpecNew();
TEEM_API void nrrdKernelSpecSet(NrrdKernelSpec *ksp, const NrrdKernel *k,
				double kparm[NRRD_KERNEL_PARMS_NUM]);
TEEM_API void nrrdKernelParmSet(const NrrdKernel **kP,
				double kparm[NRRD_KERNEL_PARMS_NUM],
				NrrdKernelSpec *ksp);
TEEM_API NrrdKernelSpec *nrrdKernelSpecNix(NrrdKernelSpec *ksp);
/* ---- END non-NrrdIO */
TEEM_API void nrrdInit(Nrrd *nrrd);
TEEM_API Nrrd *nrrdNew(void);
TEEM_API Nrrd *nrrdNix(Nrrd *nrrd);
TEEM_API Nrrd *nrrdEmpty(Nrrd *nrrd);
TEEM_API Nrrd *nrrdNuke(Nrrd *nrrd);
TEEM_API int nrrdWrap_nva(Nrrd *nrrd, void *data, int type,
			  int dim, const int *size);
TEEM_API int nrrdWrap(Nrrd *nrrd, void *data, int type, int dim,
		      ... /* sx, sy, .., axis(dim-1) size */);
TEEM_API int nrrdCopy(Nrrd *nout, const Nrrd *nin);
TEEM_API int nrrdAlloc_nva(Nrrd *nrrd, int type, int dim, const int *size);
TEEM_API int nrrdAlloc(Nrrd *nrrd, int type, int dim,
		       ... /* sx, sy, .., axis(dim-1) size */);
TEEM_API int nrrdMaybeAlloc_nva(Nrrd *nrrd, int type, int dim,
				const int *size);
TEEM_API int nrrdMaybeAlloc(Nrrd *nrrd, int type, int dim,
			    ... /* sx, sy, .., axis(dim-1) size */);
TEEM_API int nrrdPPM(Nrrd *, int sx, int sy);
TEEM_API int nrrdPGM(Nrrd *, int sx, int sy);

/******** axis info related */
/* axis.c */
TEEM_API int nrrdAxisInfoCopy(Nrrd *nout, const Nrrd *nin,
			      const int *axmap, int bitflag);
TEEM_API void nrrdAxisInfoSet_nva(Nrrd *nin, int axInfo, const void *info);
TEEM_API void nrrdAxisInfoSet(Nrrd *nin, int axInfo,
			      ... /* const void* */);
TEEM_API void nrrdAxisInfoGet_nva(const Nrrd *nrrd, int axInfo, void *info);
TEEM_API void nrrdAxisInfoGet(const Nrrd *nrrd, int axInfo,
			      ... /* void* */);
TEEM_API double nrrdAxisPos(const Nrrd *nrrd, int ax, double idx);
TEEM_API double nrrdAxisIdx(const Nrrd *nrrd, int ax, double pos);
TEEM_API void nrrdAxisPosRange(double *loP, double *hiP,
			       const Nrrd *nrrd, int ax,
			       double loIdx, double hiIdx);
TEEM_API void nrrdAxisIdxRange(double *loP, double *hiP,
			       const Nrrd *nrrd, int ax,
			       double loPos, double hiPos);
TEEM_API void nrrdAxisSpacingSet(Nrrd *nrrd, int ax);
TEEM_API void nrrdAxisMinMaxSet(Nrrd *nrrd, int ax, int defCenter);

/******** simple things */
/* simple.c */
TEEM_API const char *nrrdBiffKey;
TEEM_API const Nrrd **nrrdCNPP(Nrrd **nin, int N);
TEEM_API int nrrdPeripheralInit(Nrrd *nrrd);
TEEM_API int nrrdPeripheralCopy(Nrrd *nout, const Nrrd *nin);
TEEM_API int nrrdContentSet(Nrrd *nout, const char *func,
			    const Nrrd *nin, const char *format,
			    ... /* printf-style arg list */ );
TEEM_API void nrrdDescribe(FILE *file, const Nrrd *nrrd);
TEEM_API int nrrdCheck(const Nrrd *nrrd);
TEEM_API int nrrdElementSize(const Nrrd *nrrd);
TEEM_API size_t nrrdElementNumber(const Nrrd *nrrd);
TEEM_API int nrrdSanity(void);
TEEM_API int nrrdSameSize(const Nrrd *n1, const Nrrd *n2, int useBiff);

/******** comments related */
/* comment.c */
TEEM_API int nrrdCommentAdd(Nrrd *nrrd, const char *str);
TEEM_API void nrrdCommentClear(Nrrd *nrrd);
TEEM_API int nrrdCommentCopy(Nrrd *nout, const Nrrd *nin);

/******** key/value pairs */
/* keyvalue.c */
TEEM_API int nrrdKeyValueSize(const Nrrd *nrrd);
TEEM_API int nrrdKeyValueAdd(Nrrd *nrrd, const char *key, const char *value);
TEEM_API char *nrrdKeyValueGet(const Nrrd *nrrd, const char *key);
TEEM_API void nrrdKeyValueIndex(const Nrrd *nrrd, 
			      char **keyP, char **valueP, int ki);
TEEM_API int nrrdKeyValueErase(Nrrd *nrrd, const char *key);
TEEM_API void nrrdKeyValueClear(Nrrd *nrrd);
TEEM_API int nrrdKeyValueCopy(Nrrd *nout, const Nrrd *nin);

/******** endian related */
/* endianNrrd.c */
TEEM_API void nrrdSwapEndian(Nrrd *nrrd);

/******** getting information to and from files */
/* formatXXX.c */
TEEM_API const NrrdFormat *const nrrdFormatNRRD;
TEEM_API const NrrdFormat *const nrrdFormatPNM;
TEEM_API const NrrdFormat *const nrrdFormatPNG;
TEEM_API const NrrdFormat *const nrrdFormatVTK;
TEEM_API const NrrdFormat *const nrrdFormatText;
TEEM_API const NrrdFormat *const nrrdFormatEPS;
/* format.c */
TEEM_API const NrrdFormat *const nrrdFormatUnknown;
TEEM_API const NrrdFormat *
  const nrrdFormatArray[NRRD_FORMAT_TYPE_MAX+1];
/* encodingXXX.c */
TEEM_API const NrrdEncoding *const nrrdEncodingRaw;
TEEM_API const NrrdEncoding *const nrrdEncodingAscii;
TEEM_API const NrrdEncoding *const nrrdEncodingHex;
TEEM_API const NrrdEncoding *const nrrdEncodingGzip;
TEEM_API const NrrdEncoding *const nrrdEncodingBzip2;
/* encoding.c */
TEEM_API const NrrdEncoding *const nrrdEncodingUnknown;
TEEM_API const NrrdEncoding *
  const nrrdEncodingArray[NRRD_ENCODING_TYPE_MAX+1];
/* read.c */
TEEM_API int nrrdLineSkip(NrrdIoState *io);
TEEM_API int nrrdByteSkip(Nrrd *nrrd, NrrdIoState *io);
TEEM_API int nrrdLoad(Nrrd *nrrd, const char *filename, NrrdIoState *io);
TEEM_API int nrrdRead(Nrrd *nrrd, FILE *file, NrrdIoState *io);
/* write.c */
TEEM_API int nrrdIoStateSet(NrrdIoState *io, int parm, int value);
TEEM_API int nrrdIoStateSetEncoding(NrrdIoState *io,
				    const NrrdEncoding *encoding);
TEEM_API int nrrdIoStateSetFormat(NrrdIoState *io, 
				  const NrrdFormat *format);
TEEM_API int nrrdIoStateGet(NrrdIoState *io, int parm);
TEEM_API const NrrdEncoding *nrrdIoStateGetEncoding(NrrdIoState *io);
TEEM_API const NrrdFormat *nrrdIoStateBetFormat(NrrdIoState *io);
TEEM_API int nrrdSave(const char *filename, const Nrrd *nrrd, NrrdIoState *io);
TEEM_API int nrrdWrite(FILE *file, const Nrrd *nrrd, NrrdIoState *io);

/******** getting value into and out of an array of general type, and
   all other simplistic functionality pseudo-parameterized by type */
/* accessors.c */
TEEM_API int    (*nrrdILoad[NRRD_TYPE_MAX+1])(const void *v);
TEEM_API float  (*nrrdFLoad[NRRD_TYPE_MAX+1])(const void *v);
TEEM_API double (*nrrdDLoad[NRRD_TYPE_MAX+1])(const void *v);
TEEM_API int    (*nrrdIStore[NRRD_TYPE_MAX+1])(void *v, int j);
TEEM_API float  (*nrrdFStore[NRRD_TYPE_MAX+1])(void *v, float f);
TEEM_API double (*nrrdDStore[NRRD_TYPE_MAX+1])(void *v, double d);
TEEM_API int    (*nrrdILookup[NRRD_TYPE_MAX+1])(const void *v, size_t I);
TEEM_API float  (*nrrdFLookup[NRRD_TYPE_MAX+1])(const void *v, size_t I);
TEEM_API double (*nrrdDLookup[NRRD_TYPE_MAX+1])(const void *v, size_t I);
TEEM_API int    (*nrrdIInsert[NRRD_TYPE_MAX+1])(void *v, size_t I, int j);
TEEM_API float  (*nrrdFInsert[NRRD_TYPE_MAX+1])(void *v, size_t I, float f);
TEEM_API double (*nrrdDInsert[NRRD_TYPE_MAX+1])(void *v, size_t I, double d);
TEEM_API int    (*nrrdSprint[NRRD_TYPE_MAX+1])(char *, const void *);
/* ---- BEGIN non-NrrdIO */
TEEM_API int    (*nrrdFprint[NRRD_TYPE_MAX+1])(FILE *, const void *);
TEEM_API float  (*nrrdFClamp[NRRD_TYPE_MAX+1])(float);
TEEM_API double (*nrrdDClamp[NRRD_TYPE_MAX+1])(double);
TEEM_API void (*nrrdMinMaxExactFind[NRRD_TYPE_MAX+1])(void *minP, void *maxP,
						      int *hasNonExistP,
						      const Nrrd *nrrd);
TEEM_API int (*nrrdValCompare[NRRD_TYPE_MAX+1])(const void *, const void *);
/* ---- END non-NrrdIO */


/******** permuting, shuffling, and all flavors of reshaping */
/* reorder.c */
TEEM_API int nrrdAxesInsert(Nrrd *nout, const Nrrd *nin, int ax);
/* ---- BEGIN non-NrrdIO */
TEEM_API int nrrdInvertPerm(int *invp, const int *perm, int n);
TEEM_API int nrrdAxesPermute(Nrrd *nout, const Nrrd *nin, const int *axes);
TEEM_API int nrrdAxesSwap(Nrrd *nout, const Nrrd *nin, int ax1, int ax2);
TEEM_API int nrrdShuffle(Nrrd *nout, const Nrrd *nin, int axis,
			 const int *perm);
TEEM_API int nrrdFlip(Nrrd *nout, const Nrrd *nin, int axis);
TEEM_API int nrrdJoin(Nrrd *nout, const Nrrd *const *nin, int numNin, 
		      int axis, int incrDim);
TEEM_API int nrrdReshape(Nrrd *nout, const Nrrd *nin, int dim,
			 ... /* sx, sy, .., axis(dim-1) size */ );
TEEM_API int nrrdReshape_nva(Nrrd *nout, const Nrrd *nin,
			     int dim, const int *size);
TEEM_API int nrrdAxesSplit(Nrrd *nout, const Nrrd *nin, int ax,
			   int sizeFast, int sizeSlow);
TEEM_API int nrrdAxesDelete(Nrrd *nout, const Nrrd *nin, int ax);
TEEM_API int nrrdAxesMerge(Nrrd *nout, const Nrrd *nin, int ax);
TEEM_API int nrrdBlock(Nrrd *nout, const Nrrd *nin);
TEEM_API int nrrdUnblock(Nrrd *nout, const Nrrd *nin, int type);

/******** things useful with hest */
/* hestNrrd.c */
TEEM_API hestCB *nrrdHestNrrd;
TEEM_API hestCB *nrrdHestKernelSpec;
TEEM_API hestCB *nrrdHestIter;

/******** nrrd value iterator gadget */
/* iter.c */
TEEM_API NrrdIter *nrrdIterNew(void);
TEEM_API void nrrdIterSetValue(NrrdIter *iter, double val);
TEEM_API void nrrdIterSetNrrd(NrrdIter *iter, const Nrrd *nrrd);
TEEM_API void nrrdIterSetOwnNrrd(NrrdIter *iter, Nrrd *nrrd);
TEEM_API double nrrdIterValue(NrrdIter *iter);
TEEM_API char *nrrdIterContent(NrrdIter *iter);
TEEM_API NrrdIter *nrrdIterNix(NrrdIter *iter);

/******** expressing the range of values in a nrrd */
/* range.c */
TEEM_API NrrdRange *nrrdRangeNew(double min, double max);
TEEM_API NrrdRange *nrrdRangeCopy(const NrrdRange *range);
TEEM_API NrrdRange *nrrdRangeNix(NrrdRange *range);
TEEM_API void nrrdRangeReset(NrrdRange *range);
TEEM_API void nrrdRangeSet(NrrdRange *range,
			   const Nrrd *nrrd, int blind8BitRange);
TEEM_API void nrrdRangeSafeSet(NrrdRange *range,
			       const Nrrd *nrrd, int blind8BitRange);
TEEM_API NrrdRange *nrrdRangeNewSet(const Nrrd *nrrd, int blind8BitRange);
TEEM_API int nrrdHasNonExist(const Nrrd *nrrd);

/******** some of the point-wise value remapping, conversion, and such */
/* map.c */
TEEM_API int nrrdConvert(Nrrd *nout, const Nrrd *nin, int type);
TEEM_API int nrrdQuantize(Nrrd *nout, const Nrrd *nin,
			  const NrrdRange *range, int bits);

TEEM_API int nrrdUnquantize(Nrrd *nout, const Nrrd *nin, int type);
TEEM_API int nrrdHistoEq(Nrrd *nout, const Nrrd *nin, Nrrd **nhistP,
			 int bins, int smart, float amount);

/******** rest of point-wise value remapping, and "color"mapping */
/* apply1D.c */
TEEM_API int nrrdApply1DLut(Nrrd *nout,
			    const Nrrd *nin, const NrrdRange *range,
			    const Nrrd *nlut, int typeOut, int rescale);
TEEM_API int nrrdApply1DRegMap(Nrrd *nout,
			       const Nrrd *nin, const NrrdRange *range,
			       const Nrrd *nmap, int typeOut, int rescale);
TEEM_API int nrrd1DIrregMapCheck(const Nrrd *nmap);
TEEM_API int nrrd1DIrregAclGenerate(Nrrd *nacl, const Nrrd *nmap, int aclLen);
TEEM_API int nrrd1DIrregAclCheck(const Nrrd *nacl);
TEEM_API int nrrdApply1DIrregMap(Nrrd *nout,
				 const Nrrd *nin, const NrrdRange *range, 
				 const Nrrd *nmap, const Nrrd *nacl,
				 int typeOut, int rescale);

/******** sampling, slicing, cropping */
/* subset.c */
TEEM_API int nrrdSample_nva(void *val, const Nrrd *nin, const int *coord);
TEEM_API int nrrdSample(void *val, const Nrrd *nin,
			... /* coord0, coord1, .., coord(dim-1) */ );
TEEM_API int nrrdSlice(Nrrd *nout, const Nrrd *nin, int axis, int pos);
TEEM_API int nrrdCrop(Nrrd *nout, const Nrrd *nin, int *min, int *max);
TEEM_API int nrrdSimpleCrop(Nrrd *nout, const Nrrd *nin, int crop);

/******** padding */
/* superset.c */
TEEM_API int nrrdSplice(Nrrd *nout, const Nrrd *nin, const Nrrd *nslice,
			int axis, int pos);
TEEM_API int nrrdPad_nva(Nrrd *nout, const Nrrd *nin,
			 const int *min, const int *max,
			 int boundary, double padValue);
TEEM_API int nrrdPad(Nrrd *nout, const Nrrd *nin,
		     const int *min, const int *max, int boundary,
		     ... /* if nrrdBoundaryPad, what value */);
TEEM_API int nrrdSimplePad_nva(Nrrd *nout, const Nrrd *nin, int pad,
			       int boundary, double padValue);
TEEM_API int nrrdSimplePad(Nrrd *nout, const Nrrd *nin, int pad, int boundary,
			   ... /* if nrrdBoundaryPad, what value */);
TEEM_API int nrrdInset(Nrrd *nout, const Nrrd *nin,
		       const Nrrd *nsub, const int *min);

/******** measuring and projecting */
/* measure.c */
TEEM_API void (*nrrdMeasureLine[NRRD_MEASURE_MAX+1])(void *ans, int ansType,
						     const void *line,
						     int lineType, int lineLen,
						     double axMin,
						     double axMax);
TEEM_API int nrrdProject(Nrrd *nout, const Nrrd *nin,
			 int axis, int measr, int type);

/********* various kinds of histograms */
/* histogram.c */
TEEM_API int nrrdHisto(Nrrd *nout, const Nrrd *nin, const NrrdRange *range,
		       const Nrrd *nwght, int bins, int type);
TEEM_API int nrrdHistoDraw(Nrrd *nout, const Nrrd *nin, int sy,
			   int showLog, double max);
TEEM_API int nrrdHistoAxis(Nrrd *nout, const Nrrd *nin, const NrrdRange *range,
			   int axis, int bins, int type);
TEEM_API int nrrdHistoJoint(Nrrd *nout, const Nrrd *const *nin,
			    const NrrdRange *const *range, int numNin,
			    const Nrrd *nwght, const int *bins,
			    int type, const int *clamp);

/******** arithmetic and math on nrrds */
/* arith.c */
TEEM_API int nrrdArithGamma(Nrrd *nout, const Nrrd *nin,
			    const NrrdRange *range, double gamma);
TEEM_API int nrrdArithUnaryOp(Nrrd *nout, int op, const Nrrd *nin);
TEEM_API int nrrdArithBinaryOp(Nrrd *nout, int op,
			       const Nrrd *ninA, const Nrrd *ninB);
TEEM_API int nrrdArithTernaryOp(Nrrd *nout, int op,
				const Nrrd *ninA, const Nrrd *ninB,
				const Nrrd *ninC);
TEEM_API int nrrdArithIterBinaryOp(Nrrd *nout, int op,
				   NrrdIter *inA, NrrdIter *inB);
TEEM_API int nrrdArithIterTernaryOp(Nrrd *nout, int op,
				    NrrdIter *inA, NrrdIter *inB,
				    NrrdIter *inC);

/******** filtering and re-sampling */
/* filt.c */
TEEM_API int nrrdCheapMedian(Nrrd *nout, const Nrrd *nin,
			     int pad, int mode,
			     int radius, float wght, int bins);

/*
******** nrrdResample_t typedef
**
** type used to hold filter sample locations and weights in
** nrrdSpatialResample(), and to hold the intermediate sampling
** results.  Not as good as templating, but better than hard-coding
** float versus double.  Actually, the difference between float and
** double is not exposed in any functions or objects declared in this
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
TEEM_API int nrrdSpatialResample(Nrrd *nout, const Nrrd *nin,
				 const NrrdResampleInfo *info);
TEEM_API int nrrdSimpleResample(Nrrd *nout, Nrrd *nin,
				const NrrdKernel *kernel, const double *parm,
				const int *samples, const double *scalings);

/******** connected component extraction and manipulation */
/* ccmethods.c */
TEEM_API int nrrdCCValid(const Nrrd *nin);
TEEM_API int nrrdCCSize(Nrrd *nout, const Nrrd *nin);
TEEM_API int nrrdCCMax(const Nrrd *nin);
TEEM_API int nrrdCCNum(const Nrrd *nin);
/* cc.c */
TEEM_API int nrrdCCFind(Nrrd *nout, Nrrd **nvalP, const Nrrd *nin,
			int type, int conny);
TEEM_API int nrrdCCAdjacency(Nrrd *nout, const Nrrd *nin, int conny);
TEEM_API int nrrdCCMerge(Nrrd *nout, const Nrrd *nin, Nrrd *nval,
			 int dir, int maxSize, int maxNeighbor, int conny);
TEEM_API int nrrdCCRevalue (Nrrd *nout, const Nrrd *nin, const Nrrd *nval);
TEEM_API int nrrdCCSettle(Nrrd *nout, Nrrd **nvalP, const Nrrd *nin);
  
/******** kernels (interpolation, 1st and 2nd derivatives) */
/* tmfKernel.c
   nrrdKernelTMF[D+1][C+1][A] is d<D>_c<C>_<A>ef:
   Dth-derivative, C-order continuous ("smooth"), A-order accurate
   (for D and C, index 0 accesses the function for -1) */
TEEM_API NrrdKernel *const nrrdKernelTMF[4][5][5];
TEEM_API int nrrdKernelTMF_maxD;
TEEM_API int nrrdKernelTMF_maxC;
TEEM_API int nrrdKernelTMF_maxA;
/* winKernel.c : various kinds of windowed sincs */
TEEM_API NrrdKernel
  *const nrrdKernelHann,         /* Hann (cosine-bell) windowed sinc */
  *const nrrdKernelHannD,        /* 1st derivative of Hann windowed since */
  *const nrrdKernelHannDD,       /* 2nd derivative */
  *const nrrdKernelBlackman,     /* Blackman windowed sinc */
  *const nrrdKernelBlackmanD,    /* 1st derivative of Blackman windowed sinc */
  *const nrrdKernelBlackmanDD;   /* 2nd derivative */
/* kernel.c */
TEEM_API NrrdKernel
  *const nrrdKernelZero,         /* zero everywhere */
  *const nrrdKernelBox,          /* box filter (nearest neighbor) */
  *const nrrdKernelTent,         /* tent filter (linear interpolation) */
  *const nrrdKernelForwDiff,     /* forward-difference-ish 1st deriv. */
  *const nrrdKernelCentDiff,     /* central-difference-ish 1st deriv. */
  *const nrrdKernelBCCubic,      /* BC family of cubic polynomial splines */
  *const nrrdKernelBCCubicD,     /* 1st deriv. of BC cubic family */
  *const nrrdKernelBCCubicDD,    /* 2nd deriv. of BC cubic family */
  *const nrrdKernelAQuartic,     /* A family of quartic C2 interp. splines */
  *const nrrdKernelAQuarticD,    /* 1st deriv. of A quartic family */
  *const nrrdKernelAQuarticDD,   /* 2nd deriv. of A quartic family */
  *const nrrdKernelGaussian,     /* Gaussian */
  *const nrrdKernelGaussianD,    /* 1st derivative of Gaussian */
  *const nrrdKernelGaussianDD;   /* 2nd derivative of Gaussian */
TEEM_API int nrrdKernelParse(const NrrdKernel **kernelP,
			     double *parm,
			     const char *str);
TEEM_API int nrrdKernelSpecParse(NrrdKernelSpec *ksp, const char *str);

/* ---- END non-NrrdIO */

#ifdef __cplusplus
}
#endif

#endif /* NRRD_HAS_BEEN_INCLUDED */
