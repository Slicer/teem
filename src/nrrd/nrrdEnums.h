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

#ifndef NRRD_ENUMS_HAS_BEEN_INCLUDED
#define NRRD_ENUMS_HAS_BEEN_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

/*******
******** NONE of these enums should have values set explicitly in their
******** definition.  The values should simply start at 0 (for Unknown)
******** and increase one integer per value.  The _nrrdCheckEnums()
******** sanity check assumes this, and there is no reason to use 
******** explicit values for any of the enums.
*******/

/*
******** nrrdFormat enum
**
** the different file formats which nrrd supports
*/
enum {
  nrrdFormatUnknown,
  nrrdFormatNRRD,       /* 1: basic nrrd format (associated with both
			   magic nrrdMagicOldNRRD and nrrdMagicNRRD0001 */
  nrrdFormatPNM,        /* 2: PNM image */
  nrrdFormatPNG,        /* 3: PNG image */
  nrrdFormatTable,      /* 4: bare ASCII table */
  nrrdFormatLast
};
#define NRRD_FORMAT_MAX    4

/*
******** nrrdBoundary enum
**
** when resampling, how to deal with the ends of a scanline
*/
enum {
  nrrdBoundaryUnknown,
  nrrdBoundaryPad,      /* 1: fill with some user-specified value */
  nrrdBoundaryBleed,    /* 2: copy the last/first value out as needed */
  nrrdBoundaryWrap,     /* 3: wrap-around */
  nrrdBoundaryWeight,   /* 4: normalize the weighting on the existing samples;
			   ONLY sensible for a strictly positive kernel
			   which integrates to unity (as in blurring) */
  nrrdBoundaryLast
};
#define NRRD_BOUNDARY_MAX  4

/*
******** nrrdMagic enum
**
** the different "magic numbers" that nrrd knows about.  Can only deal
** with a magic on a line of its own, with a carraige return.  
** WARNING: the PNM format does not _require_ a carraige return after
** the magic, but everything seems to do it anyway
*/
enum {
  nrrdMagicUnknown,
  nrrdMagicOldNRRD,      /* 1: "NRRD00.01" (pre teem 1.4) */
  nrrdMagicNRRD0001,     /* 2: "NRRD0001" */
  nrrdMagicP2,           /* 3: "P2": ascii PGM */
  nrrdMagicP3,           /* 4: "P3": ascii PPM */
  nrrdMagicP5,           /* 5: "P5": binary PGM */
  nrrdMagicP6,           /* 6: "P6": binary PPM */
  nrrdMagicPNG,          /* 7: "\211PNG": PNG */
  nrrdMagicLast
};
#define NRRD_MAGIC_MAX      7

/*
******** nrrdType enum
**
** all the different types, identified by integer
*/
enum {
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
  nrrdTypeBlock,         /* 11: size user defined at run time; MUST BE LAST */
  nrrdTypeLast
};
#define NRRD_TYPE_MAX       11
#define NRRD_TYPE_SIZE_MAX   8    /* max(sizeof()) over all scalar types */
#define NRRD_TYPE_BIGGEST double  /* this should be a basic C type which
				     requires for storage the maximum size
				     of all the basic C types */

/*
******** nrrdEncoding enum
**
** how data might be encoded into a bytestream
*/
enum {
  nrrdEncodingUnknown,
  nrrdEncodingRaw,        /* 1: same as memory layout (modulo endianness) */
  nrrdEncodingAscii,      /* 2: decimal values are spelled out in ascii */
  nrrdEncodingHex,        /* 3: hexidecimal (two chars per byte) */
  nrrdEncodingGzip,       /* 4: gzip'ed raw data */
  nrrdEncodingBzip2,      /* 5: bzip2'ed raw data */
  nrrdEncodingLast
};
#define NRRD_ENCODING_MAX    5

/*
******** nrrdZlibStrategy enum
**
** how gzipped data is compressed
*/
enum {
  nrrdZlibStrategyUnknown,
  nrrdZlibStrategyDefault,   /* 1: default (Huffman + string match) */
  nrrdZlibStrategyHuffman,   /* 2: Huffman only */
  nrrdZlibStrategyFiltered,  /* 3: specialized for filtered data */
  nrrdZlibStrategyLast
};
#define NRRD_ZLIB_STRATEGY_MAX  3

/*
******** nrrdMeasure enum
**
** ways to "measure" some portion of the array
** NEEDS TO BE IN SYNC WITH nrrdMeasr array in measr.c
*/
enum {
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
  nrrdMeasureVariance,       /* 11 */
  nrrdMeasureSD,             /* 12: standard deviation */
  /* 
  ** the nrrduMeasureHisto... measures interpret the array as a
  ** histogram of some implied value distribution
  */
  nrrdMeasureHistoMin,       /* 13 */
  nrrdMeasureHistoMax,       /* 14 */
  nrrdMeasureHistoMean,      /* 15 */
  nrrdMeasureHistoMedian,    /* 16 */
  nrrdMeasureHistoMode,      /* 17 */
  nrrdMeasureHistoProduct,   /* 18 */
  nrrdMeasureHistoSum,       /* 19 */
  nrrdMeasureHistoVariance,  /* 20 */
  nrrdMeasureLast
};
#define NRRD_MEASURE_MAX        20

/*
******** nrrdCenter enum
**
** node-centered vs. cell-centered
*/
enum {
  nrrdCenterUnknown,
  nrrdCenterNode,            /* 1: samples at corners of things
				(how "voxels" are usually imagined)
				|\______/|\______/|\______/|
				X        X        X        X   */
  nrrdCenterCell,            /* 2: samples at middles of things
				(characteristic of histogram bins)
				 \___|___/\___|___/\___|___/
				     X        X        X       */
  nrrdCenterLast
};
#define NRRD_CENTER_MAX         2

/*
******** nrrdAxesInfo enum
**
** the different pieces of per-axis information recorded in a nrrd
*/
enum {
  nrrdAxesInfoUnknown,
  nrrdAxesInfoSize,                /* 1: number of samples along axis */
#define NRRD_AXESINFO_SIZE_BIT    (1<<1)
  nrrdAxesInfoSpacing,             /* 2: spacing between samples */
#define NRRD_AXESINFO_SPACING_BIT (1<<2)
  nrrdAxesInfoMin,                 /* 3: minimum pos. assoc. w/ first sample */
#define NRRD_AXESINFO_MIN_BIT     (1<<3) 
  nrrdAxesInfoMax,                 /* 4: maximum pos. assoc. w/ last sample */
#define NRRD_AXESINFO_MAX_BIT     (1<<4)
  nrrdAxesInfoCenter,              /* 5: cell vs. node */
#define NRRD_AXESINFO_CENTER_BIT  (1<<5)
  nrrdAxesInfoLabel,               /* 6: string describing the axis */
#define NRRD_AXESINFO_LABEL_BIT   (1<<6)
  nrrdAxesInfoUnit,                /* 7: string identifying units */
#define NRRD_AXESINFO_UNIT_BIT    (1<<7)
  nrrdAxesInfoLast
};
#define NRRD_AXESINFO_MAX             7
#define NRRD_AXESINFO_ALL  ((1<<1)|(1<<2)|(1<<3)|(1<<4)|(1<<5)|(1<<6)|(1<<7))
#define NRRD_AXESINFO_NONE 0

/*
** the "endian" enum is actually in the air library, but it is very
** convenient to have it incorporated into the nrrd enum framework for
** the purposes of string<-->int conversion.  Unfortunately, the
** little and big values are 1234 and 4321 respectively, so
** NRRD_ENDIAN_MAX is not actually the highest valid value, but only
** an indicator of how many valid values there are.
*/
#define NRRD_ENDIAN_MAX 2

/*
******** nrrdField enum
**
** the various fields we can parse in a NRRD header
*/
enum {
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
  nrrdField_units,           /* 13 */
  nrrdField_min,             /* 14 */
  nrrdField_max,             /* 15 */
  nrrdField_old_min,         /* 16 */
  nrrdField_old_max,         /* 17 */
  nrrdField_data_file,       /* 18 */
  nrrdField_endian,          /* 19 */
  nrrdField_encoding,        /* 20 */
  nrrdField_line_skip,       /* 21 */
  nrrdField_byte_skip,       /* 22 */
  nrrdField_last
};
#define NRRD_FIELD_MAX          22

/* oh look, I'm violating my rules outline above for how the enum values
** should be ordered.  The reason for this is that its just too bizarro
** to have the logical value of both nrrdNonExistFalse and nrrdNonExistTrue
** to be (in C) true.  For instance, nrrdHasNonExist() should be able to
** return a value from this enum which also functions in a C expressions
** as the expected boolean value.  If someone makes the mistake of testing
** one of these values as a C truth value, they will probably be harmlessly
** conservative in thinking that non-existant values do exist.
*/
enum {
  nrrdNonExistFalse,      /* 0 */
  nrrdNonExistTrue,       /* 1 */
  nrrdNonExistUnknown,    /* 2 */
  nrrdNonExistLast
};
#define NRRD_NON_EXIST_MAX   2

/*
******** nrrdUnaryOp enum
**
** for unary operations on nrrds
*/
enum {
  nrrdUnaryOpUnknown,
  nrrdUnaryOpNegative,   /*  1 */
  nrrdUnaryOpReciprocal, /*  2 */
  nrrdUnaryOpSin,        /*  3 */
  nrrdUnaryOpCos,        /*  4 */
  nrrdUnaryOpTan,        /*  5 */
  nrrdUnaryOpAsin,       /*  6 */
  nrrdUnaryOpAcos,       /*  7 */
  nrrdUnaryOpAtan,       /*  8 */
  nrrdUnaryOpExp,        /*  9 */
  nrrdUnaryOpLog,        /* 10 */
  nrrdUnaryOpLog10,      /* 11 */
  nrrdUnaryOpSqrt,       /* 12 */
  nrrdUnaryOpCeil,       /* 13 */
  nrrdUnaryOpFloor,      /* 14 */
  nrrdUnaryOpRoundUp,    /* 15 */
  nrrdUnaryOpRoundDown,  /* 16 */
  nrrdUnaryOpAbs,        /* 17 */
  nrrdUnaryOpSgn,        /* 18 */
  nrrdUnaryOpExists,     /* 19 */
  nrrdUnaryOpLast
};
#define NRRD_UNARY_OP_MAX   19


/*
******** nrrdBinaryOp enum
**
** for binary operations on nrrds
*/
enum {
  nrrdBinaryOpUnknown,
  nrrdBinaryOpAdd,        /*  1 */
  nrrdBinaryOpSubtract,   /*  2 */
  nrrdBinaryOpMultiply,   /*  3 */
  nrrdBinaryOpDivide,     /*  4 */
  nrrdBinaryOpPow,        /*  5 */
  nrrdBinaryOpMod,        /*  6 */
  nrrdBinaryOpFmod,       /*  7 */
  nrrdBinaryOpAtan2,      /*  8 */
  nrrdBinaryOpMin,        /*  9 */
  nrrdBinaryOpMax,        /* 10 */
  nrrdBinaryOpLT,         /* 11 */
  nrrdBinaryOpLTE,        /* 12 */
  nrrdBinaryOpGT,         /* 13 */
  nrrdBinaryOpGTE,        /* 14 */
  nrrdBinaryOpCompare,    /* 15 */
  nrrdBinaryOpEqual,      /* 16 */
  nrrdBinaryOpNotEqual,   /* 17 */
  nrrdBinaryOpLast
};
#define NRRD_BINARY_OP_MAX   17

/*
******** nrrdTernaryOp
**
** for ternary operations on nrrds
*/
enum {
  nrrdTernaryOpUnknown,
  nrrdTernaryOpClamp,     /* 1 */
  nrrdTernaryOpLerp,      /* 2 */
  nrrdTernaryOpExists,    /* 3 */
  nrrdTernaryOpInside,    /* 4 */
  nrrdTernaryOpBetween,   /* 5 */
  nrrdTernaryOpLast
};
#define NRRD_TERNARY_OP_MAX  5

#ifdef __cplusplus
}
#endif

#endif /* NRRD_ENUMS_HAS_BEEN_INCLUDED */
