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
******** nrrdIoState* enum
** 
** the various things it makes sense to get and set in nrrdIoState struct
** via nrrdIoStateGet and nrrdIoStateSet
*/
enum {
  nrrdIoStateUnknown,
  nrrdIoStateDetachedHeader,
  nrrdIoStateBareText,
  nrrdIoStateCharsPerLine,
  nrrdIoStateValsPerLine,
  nrrdIoStateSkipData,
  nrrdIoStateKeepNrrdDataFileOpen,
  nrrdIoStateZlibLevel,
  nrrdIoStateZlibStrategy,
  nrrdIoStateBzip2BlockSize,
  nrrdIoStateLast
};

/*
******** nrrdFormatType enum
**
** the different file formats which nrrd supports
*/
enum {
  nrrdFormatTypeUnknown,
  nrrdFormatTypeNRRD,   /* 1: basic nrrd format (associated with any of
                           the magics starting with "NRRD") */
  nrrdFormatTypePNM,    /* 2: PNM image */
  nrrdFormatTypePNG,    /* 3: PNG image */
  nrrdFormatTypeVTK,    /* 4: VTK Structured Points datasets (v1.0 and 2.0) */
  nrrdFormatTypeText,   /* 5: bare ASCII text for 2D arrays */
  nrrdFormatTypeEPS,    /* 6: Encapsulated PostScript (write-only) */
  nrrdFormatTypeLast
};
#define NRRD_FORMAT_TYPE_MAX    6

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
******** nrrdType enum
**
** all the different types, identified by integer
**
** 18 July 03: After some consternation, I decided to set
** nrrdTypeUnknown and nrrdTypeDefault to the same thing, with the
** reasoning that the only times that nrrdTypeDefault is used is when
** controlling an *output* type (the type of "nout"), or rather,
** choosing not to control an output type.  As output types must be
** known, there is no confusion between being unset/unknown (invalid)
** and being simply default.
*/
enum {
  nrrdTypeUnknown=0,     /*  0: signifies "type is unset/unknown" */
  nrrdTypeDefault=0,     /*  0: signifies "determine output type for me" */
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
******** nrrdEncodingType enum
**
** how data might be encoded into a bytestream
*/
enum {
  nrrdEncodingTypeUnknown,
  nrrdEncodingTypeRaw,      /* 1: same as memory layout (modulo endianness) */
  nrrdEncodingTypeAscii,    /* 2: decimal values are spelled out in ascii */
  nrrdEncodingTypeHex,      /* 3: hexidecimal (two chars per byte) */
  nrrdEncodingTypeGzip,     /* 4: gzip'ed raw data */
  nrrdEncodingTypeBzip2,    /* 5: bzip2'ed raw data */
  nrrdEncodingTypeLast
};
#define NRRD_ENCODING_TYPE_MAX 5

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
******** nrrdKind enum
**
** The very cautious (and last?) step nrrd takes towards sementics.
**
** More of these may be added in the future, as when nrrd supports bricking.
**
** NB: The nrrdKindSize() function returns the suggested length for these.
*/
enum {
  nrrdKindUnknown,
  nrrdKindDomain,            /*  1: "Yes, you can resample me" */
  nrrdKindList,              /*  2: "No, it is goofy to resample me" */
  nrrdKindStub,              /*  3: axis with one sample (a placeholder) */
  nrrdKindScalar,            /*  4: effectively, same as a stub */
  nrrdKindComplex,           /*  5: real and imaginary components */
  nrrdKind2Vector,           /*  6: 2 component vector */
  nrrdKind3Color,            /*  7: ANY 3-component color value */
  nrrdKind4Color,            /*  8: ANY 4-component color value */
  nrrdKind3Vector,           /*  9: 3 component vector */
  nrrdKind3Normal,           /* 10: 3 component vector, assumed normalized */
  nrrdKind4Vector,           /* 11: 4 component vector */
  nrrdKind2DSymTensor,       /* 12: Txx Txy Tyy */
  nrrdKind2DMaskedSymTensor, /* 13: mask Txx Txy Tyy */
  nrrdKind2DTensor,          /* 14: Txx Txy Tyx Tyy */
  nrrdKind2DMaskedTensor,    /* 15: mask Txx Txy Tyx Tyy */
  nrrdKind3DSymTensor,       /* 16: Txx Txy Txz Tyy Tyz Tzz */
  nrrdKind3DMaskedSymTensor, /* 17: mask Txx Txy Txz Tyy Tyz Tzz */
  nrrdKind3DTensor,          /* 18: Txx Txy Txz Tyx Tyy Tyz Tzx Tzy Tzz */
  nrrdKind3DMaskedTensor,    /* 19: mask Txx Txy Txz Tyx Tyy Tyz Tzx Tzy Tzz */
  nrrdKindLast
};
#define NRRD_KIND_MAX           19

/*
******** nrrdAxisInfo enum
**
** the different pieces of per-axis information recorded in a nrrd
*/
enum {
  nrrdAxisInfoUnknown,
  nrrdAxisInfoSize,                 /* 1: number of samples along axis */
#define NRRD_AXIS_INFO_SIZE_BIT    (1<<1)
  nrrdAxisInfoSpacing,              /* 2: spacing between samples */
#define NRRD_AXIS_INFO_SPACING_BIT (1<<2)
  nrrdAxisInfoMin,                  /* 3: minimum pos. assoc. w/ 1st sample */
#define NRRD_AXIS_INFO_MIN_BIT     (1<<3) 
  nrrdAxisInfoMax,                  /* 4: maximum pos. assoc. w/ last sample */
#define NRRD_AXIS_INFO_MAX_BIT     (1<<4)
  nrrdAxisInfoCenter,               /* 5: cell vs. node */
#define NRRD_AXIS_INFO_CENTER_BIT  (1<<5)
  nrrdAxisInfoKind,                 /* 6: from the nrrdKind* enum */
#define NRRD_AXIS_INFO_KIND_BIT    (1<<6)
  nrrdAxisInfoLabel,                /* 7: string describing the axis */
#define NRRD_AXIS_INFO_LABEL_BIT   (1<<7)
  nrrdAxisInfoUnit,                 /* 8: string identifying units */
#define NRRD_AXIS_INFO_UNIT_BIT    (1<<8)
  nrrdAxisInfoLast
};
#define NRRD_AXIS_INFO_MAX             8
#define NRRD_AXIS_INFO_ALL  \
    ((1<<1)|(1<<2)|(1<<3)|(1<<4)|(1<<5)|(1<<6)|(1<<7)|(1<<8))
#define NRRD_AXIS_INFO_NONE 0

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
**
** other things which must be kept in sync:
** arraysNrrd.c: 
**    _nrrdFieldValidInImage[]
**    _nrrdFieldOnePerAxis[]
**    _nrrdFieldValidInText[]
**    _nrrdFieldRequired[]
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
  nrrdField_kinds,           /* 12 */
  nrrdField_labels,          /* 13 */
  nrrdField_units,           /* 14 */
  nrrdField_min,             /* 15 */
  nrrdField_max,             /* 16 */
  nrrdField_old_min,         /* 17 */
  nrrdField_old_max,         /* 18 */
  nrrdField_data_file,       /* 19 */
  nrrdField_endian,          /* 20 */
  nrrdField_encoding,        /* 21 */
  nrrdField_line_skip,       /* 22 */
  nrrdField_byte_skip,       /* 23 */
  nrrdField_keyvalue,        /* 24 */
  nrrdField_last
};
#define NRRD_FIELD_MAX          24

/* 
******** nrrdHasNonExist* enum
**
** oh look, I'm violating my rules outline above for how the enum values
** should be ordered.  The reason for this is that its just too bizarro to
** have the logical value of both nrrdHasNonExistFalse and nrrdHasNonExistTrue
** to be (in C) true.  For instance, nrrdHasNonExist() should be able to 
** return a value from this enum which also functions in a C expressions as
** the expected boolean value.  If for some reason (outide the action of
** nrrdHasNonExist(), nrrdHasNonExistUnknown is interpreted as true, that's
** probably harmlessly conservative.  Time will tell.
*/
enum {
  nrrdHasNonExistFalse,     /* 0: no non-existent values were seen */
  nrrdHasNonExistTrue,      /* 1: some non-existent values were seen */
  nrrdHasNonExistOnly,      /* 2: NOTHING BUT non-existant values were seen */
  nrrdHasNonExistUnknown,   /* 3 */
  nrrdHasNonExistLast
};
#define NRRD_HAS_NON_EXIST_MAX 3

/* ---- BEGIN non-NrrdIO */

/*
******** nrrdMeasure enum
**
** ways to "measure" some portion of the array
** NEEDS TO BE IN SYNC WITH:
** - nrrdMeasure airEnum in enumsNrrd.c
** - nrrdMeasureLine function array in measure.c
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
  nrrdMeasureSkew,           /* 13: skew */
  /* 
  ** the nrrduMeasureHisto... measures interpret the array as a
  ** histogram of some implied value distribution
  */
  nrrdMeasureHistoMin,       /* 14 */
  nrrdMeasureHistoMax,       /* 15 */
  nrrdMeasureHistoMean,      /* 16 */
  nrrdMeasureHistoMedian,    /* 17 */
  nrrdMeasureHistoMode,      /* 18 */
  nrrdMeasureHistoProduct,   /* 19 */
  nrrdMeasureHistoSum,       /* 20 */
  nrrdMeasureHistoL2,        /* 21 */
  nrrdMeasureHistoVariance,  /* 22 */
  nrrdMeasureHistoSD,        /* 23 */
  nrrdMeasureLast
};
#define NRRD_MEASURE_MAX        23
#define NRRD_MEASURE_DESC \
   "Possibilities include:\n " \
   "\b\bo \"min\", \"max\", \"mean\", \"median\", \"mode\", \"variance\", " \
     "\"skew\"\n (self-explanatory)\n " \
   "\b\bo \"sd\": standard deviation\n " \
   "\b\bo \"product\", \"sum\": product or sum of all values\n " \
   "\b\bo \"L1\", \"L2\", \"Linf\": different norms\n " \
   "\b\bo \"histo-min\",  \"histo-max\", \"histo-mean\", " \
     "\"histo-median\", \"histo-mode\", \"histo-product\", \"histo-l2\", " \
     "\"histo-sum\", \"histo-variance\", \"histo-sd\": same measures, " \
     "but for situations " \
     "where we're given not the original values, but a histogram of them."
  

/*
******** nrrdBlind8BitRange
**
** whether or not to blindly say that the range of 8-bit data is
** [0-255] (uchar) or [SCHAR_MIN-SCHAR_MAX] (signed char)
*/
enum {
  nrrdBlind8BitRangeUnknown,   /* 0 */
  nrrdBlind8BitRangeTrue,      /* 1: blindly use the widest extrema (e.g.,
				  [0-255] for uchar, regardless of what's
				  really present in the data values */
  nrrdBlind8BitRangeFalse,     /* 2: use the exact value range in the data */
  nrrdBlind8BitRangeState,     /* 3: defer to nrrdStateBlind8BitMinMax */
  nrrdBlind8BitRangeLast
};
#define NRRD_BLIND_8BIT_RANGE_MAX 3
  
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
  nrrdUnaryOpLog2,       /* 11 */
  nrrdUnaryOpLog10,      /* 12 */
  nrrdUnaryOpLog1p,      /* 13 */
  nrrdUnaryOpSqrt,       /* 14 */
  nrrdUnaryOpCbrt,       /* 15 */
  nrrdUnaryOpErf,        /* 16 */
  nrrdUnaryOpCeil,       /* 17 */
  nrrdUnaryOpFloor,      /* 18 */
  nrrdUnaryOpRoundUp,    /* 19 */
  nrrdUnaryOpRoundDown,  /* 20 */
  nrrdUnaryOpAbs,        /* 21 */
  nrrdUnaryOpSgn,        /* 22 */
  nrrdUnaryOpExists,     /* 23 */
  nrrdUnaryOpRand,       /* 24 */
  nrrdUnaryOpLast
};
#define NRRD_UNARY_OP_MAX   24

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
  nrrdBinaryOpSgnPow,     /*  6 */
  nrrdBinaryOpMod,        /*  7 */
  nrrdBinaryOpFmod,       /*  8 */
  nrrdBinaryOpAtan2,      /*  9 */
  nrrdBinaryOpMin,        /* 10 */
  nrrdBinaryOpMax,        /* 11 */
  nrrdBinaryOpLT,         /* 12 */
  nrrdBinaryOpLTE,        /* 13 */
  nrrdBinaryOpGT,         /* 14 */
  nrrdBinaryOpGTE,        /* 15 */
  nrrdBinaryOpCompare,    /* 16 */
  nrrdBinaryOpEqual,      /* 17 */
  nrrdBinaryOpNotEqual,   /* 18 */
  nrrdBinaryOpExists,     /* 19 */
  nrrdBinaryOpLast
};
#define NRRD_BINARY_OP_MAX   19

/*
******** nrrdTernaryOp
**
** for ternary operations on nrrds
*/
enum {
  nrrdTernaryOpUnknown,
  nrrdTernaryOpAdd,      /*  1 */
  nrrdTernaryOpMultiply, /*  2 */
  nrrdTernaryOpMin,      /*  3 */
  nrrdTernaryOpMax,      /*  4 */
  nrrdTernaryOpClamp,    /*  5 */
  nrrdTernaryOpIfElse,   /*  6 */
  nrrdTernaryOpLerp,     /*  7 */
  nrrdTernaryOpExists,   /*  8 */
  nrrdTernaryOpInOpen,   /*  9 */
  nrrdTernaryOpInClosed, /* 10 */
  nrrdTernaryOpLast
};
#define NRRD_TERNARY_OP_MAX 10

/* ---- END non-NrrdIO */

#ifdef __cplusplus
}
#endif

#endif /* NRRD_ENUMS_HAS_BEEN_INCLUDED */
