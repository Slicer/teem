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

#ifndef NRRD_ENUMS_HAS_BEEN_INCLUDED
#define NRRD_ENUMS_HAS_BEEN_INCLUDED
#ifdef __cplusplus
extern "C" {
#endif

/*
******** nrrdFormat enum
**
** the different file formats which nrrd supports
*/
typedef enum {
  nrrdFormatUnknown,
  nrrdFormatNRRD,       /* 1: "native" nrrd format */
  nrrdFormatPNM,        /* 2: PNM image */
  nrrdFormatTable,      /* 3: bare ASCII table */
  nrrdFormatLast
} nrrdFormat;
#define NRRD_FORMAT_MAX    3
#define NRRD_FORMAT_DEFAULT nrrdFormatNRRD

/*
******** nrrdBoundary enum
**
** when resampling, how to deal with the ends of a scanline
*/
typedef enum {
  nrrdBoundaryUnknown,
  nrrdBoundaryPad,      /* 1: fill with some user-specified value */
  nrrdBoundaryBleed,    /* 2: copy the last/first value out as needed */
  nrrdBoundaryWrap,     /* 3: wrap-around */
  nrrdBoundaryWeight,   /* 4: normalize the weighting on the existing samples;
			   ONLY sensible for a strictly positive kernel
			   which integrates to unity (as in blurring) */
  nrrdBoundaryLast
} nrrdBoundary;
#define NRRD_BOUNDARY_MAX  4

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
  nrrdMagicUnknown,
  nrrdMagicNRRD0001,     /* 1: currently, the only "native" nrrd header */
  nrrdMagicP2,           /* 2: ascii PGM */
  nrrdMagicP3,           /* 3: ascii PPM */
  nrrdMagicP5,           /* 4: binary PGM */
  nrrdMagicP6,           /* 5: binary PPM */
  nrrdMagicLast
} nrrdMagic;
#define NRRD_MAGIC_MAX      5

/*
******** nrrdType enum
**
** all the different types, identified by integer
**
** nrrdTypeBlock must precede nrrdTypeLast
*/
typedef enum {
  nrrdTypeUnknown,
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
  nrrdTypeBlock,         /* 11: size user defined at run time */
  nrrdTypeLast
} nrrdType;
#define NRRD_TYPE_MAX       11 /* this has to agree with nrrdTypeBlock */
#define NRRD_TYPE_SIZE_MAX 8   /* sizeof() for largest scalar type supported */


/*
******** nrrdEncoding enum
**
** how data might be encoded into a bytestream
*/
typedef enum {
  nrrdEncodingUnknown,
  nrrdEncodingRaw,        /* 1: same as memory layout (module endian) */
  nrrdEncodingAscii,      /* 2: decimal values are spelled out in ascii */
  nrrdEncodingLast
} nrrdEncoding;
#define NRRD_ENCODING_MAX    2
#define NRRD_ENCODING_DEFAULT nrrdEncodingRaw

/*
******** nrrdMeasr enum
**
** ways to "measure" some portion of the array
** NEEDS TO BE IN SYNC WITH nrrdMeasr array in measr.c
*/
typedef enum {
  nrrdMeasureUnknown,
  nrrdMeasureMin,            /* 1: smallest value */
  nrrdMeasureMax,            /* 2: biggest value */
  nrrdMeasureMean,           /* 3: average of values */
  nrrdMeasureMedian,         /* 4: value at 50th percentile */
  nrrdMeasureMode,           /* 5: most common value */
  nrrdMeasureProduct,        /* 6: product of all values */
  nrrdMeasureSum,            /* 7: sum of all values */
  nrrdMeasureL1,             /* 8 */
  nrrdMeasureL2,             /* 9 */
  nrrdMeasureLinf,           /* 10 */
  /* 
  ** these nrrduMeasureHisto* measures interpret the array as a histogram
  ** of some implied value distribution
  */
  nrrdMeasureHistoMin,       /* 11 */
  nrrdMeasureHistoMax,       /* 12 */
  nrrdMeasureHistoMean,      /* 13 */
  nrrdMeasureHistoMedian,    /* 14 */
  nrrdMeasureHistoMode,      /* 15 */
  nrrdMeasureHistoProduct,   /* 16 */
  nrrdMeasureHistoSum,       /* 17 */
  nrrdMeasureHistoVariance,  /* 18 */
  nrrdMeasureLast
} nrrdMeasure;
#define NRRD_MEASURE_MAX        18

/*
******** nrrdMinMax enum
** 
** behaviors for dealing with the "min" and "max" fields in the nrrd,
** and the min and max values in a nrrd
*/
typedef enum {
  nrrdMinMaxUnknown,
  nrrdMinMaxSearch,          /* 1: find the actual min/max of the
				data, and use it for compuation, but
				don't set them in nrrd */
  nrrdMinMaxSearchSet,       /* 2: like nrrdMinMaxCalc, but DO record
				the min/max in the nrrd */
  nrrdMinMaxUse,             /* 3: trust and use the min/max values 
				already stored in the nrrd */
  nrrdMinMaxInsteadUse,      /* 4: trust and use the min/max values
				specified by some alternate means */
  nrrdMinMaxLast
} nrrdMinMax;
#define NRRD_MINMAX_MAX         4

/*
******** nrrdCenter enum
**
** node-centered vs. cell-centered
*/
typedef enum {
  nrrdCenterUnknown,
  nrrdCenterNode,            /* 1: samples on corners of square support */
  nrrdCenterCell,            /* 2: samples in middle of square support */
  nrrdCenterLast
} nrrdCenter;
#define NRRD_CENTER_MAX         2

typedef enum {
  nrrdAxesInfoUnknown,
  nrrdAxesInfoSize,            /* 1: number of samples along axis */
#define NRRD_AXESINFO_SIZE    (1<<1)
  nrrdAxesInfoSpacing,         /* 2: spacing between samples */
#define NRRD_AXESINFO_SPACING (1<<2)
  nrrdAxesInfoMin,             /* 3: minimum pos. assoc. w/ first sample */
#define NRRD_AXESINFO_AMIN    (1<<3)
  nrrdAxesInfoMax,             /* 4: maximum pos. assoc. w/ last sample */
#define NRRD_AXESINFO_AMAX    (1<<4)
#define NRRD_AXESINFO_MINMAX ((1<<3)|(1<<4))
  nrrdAxesInfoCenter,          /* 5: cell vs. node */
#define NRRD_AXESINFO_CENTER  (1<<5)
  nrrdAxesInfoLabel,           /* 6: string describing the axis */
#define NRRD_AXESINFO_LABEL   (1<<6)
  nrrdAxesInfoLast
} nrrdAxesInfo;
#define NRRD_AXESINFO_MAX         6
#define NRRD_AXESINFO_NONE        0
#define NRRD_AXESINFO_ALL         ((1<<1)|(1<<2)|(1<<3)|(1<<4)|(1<<5)|(1<<6))

/*
** This is somewhat silly- the "endian" enum is actually in air
** but its very convenient to have it incorporated into the
** nrrd enum framework
*/
#define NRRD_ENDIAN_MAX 2

/*
******** nrrdField enum
**
** the various fields we can parse in a NRRD header
*/
typedef enum {
  nrrdField_unknown,
  nrrdField_comment,         /*  1 */
  nrrdField_content,         /*  2 */
  nrrdField_number,          /*  3 */
  nrrdField_type,            /*  4 */
  nrrdField_block_size,      /*  5 */
  nrrdField_dimension,       /*  6 */
  nrrdField_sizes,           /*  7 */
  nrrdField_spacings,        /*  8 */
  nrrdField_axis_mins,       /*  9 */
  nrrdField_axis_maxs,       /* 10 */
  nrrdField_centers,         /* 11 */
  nrrdField_labels,          /* 12 */
  nrrdField_min,             /* 13 */
  nrrdField_max,             /* 14 */
  nrrdField_old_min,         /* 15 */
  nrrdField_old_max,         /* 16 */
  nrrdField_data_file,       /* 17 */
  nrrdField_endian,          /* 18 */
  nrrdField_encoding,        /* 19 */
  nrrdField_line_skip,       /* 20 */
  nrrdField_byte_skip,       /* 21 */
  nrrdField_last
} nrrdField;
#define NRRD_FIELD_MAX          21

/*
******** nrrdEnum enum
**
** all the enums above (and then some)
*/
typedef enum {
  nrrdEnumUnknown,
  nrrdEnumFormat,            /*  1 */
  nrrdEnumBoundary,          /*  2 */
  nrrdEnumMagic,             /*  3 */
  nrrdEnumType,              /*  4 */
  nrrdEnumEncoding,          /*  5 */
  nrrdEnumMeasure,           /*  6 */
  nrrdEnumMinMax,            /*  7 */
  nrrdEnumCenter,            /*  8 */
  nrrdEnumAxesInfo,          /*  9 */
  nrrdEnumEndian,            /* 10 */
  nrrdEnumField,             /* 11 */
  nrrdEnumLast
} nrrdEnum;
#define NRRD_ENUM_MAX           11

/* extern C */
#ifdef __cplusplus
}
#endif
#endif /* NRRD_ENUMS_HAS_BEEN_INCLUDED */



