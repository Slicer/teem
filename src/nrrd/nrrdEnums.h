/*
  teem: Gordon Kindlmann's research software
  Copyright (C) 2005  Gordon Kindlmann
  Copyright (C) 2004, 2003, 2002, 2001, 2000, 1999, 1998  University of Utah

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
** For describing the information along one axis of an array.  This is
** most important for clarifying the representation of non-scalar
** data, in order to distinguish between axes that are genuine image
** domain axes, and axes that exist just to store the multiple
** attributes per sample.  One could argue that this information
** should be per-array and not per-axis, but you still have to
** indicate which one of the axes is the attribute axis.  And, if you
** have, say, the gradient of RGB colors, you want the per-pixel 3x3
** array to have those two attribute axes tagged accordingly.
**
** More of these may be added in the future, as when nrrd supports bricking.
**
** NOTE: The nrrdKindSize() function returns the suggested length for these.
**
** Keep in sync:
**   enumsNrrd.c: nrrdKind airEnum
**        axis.c: nrrdKindSize()
**        axis.c: _nrrdKindAltered()
*/
enum {
  nrrdKindUnknown,
  nrrdKindDomain,            /*  1: "Yes, you can resample me" */
  nrrdKindSpace,             /*  2: a spatial domain */
  nrrdKindTime,              /*  3: a temporal domain */
  nrrdKindList,              /*  4: "No, it is goofy to resample me" */
  nrrdKindStub,              /*  5: axis with one sample (a placeholder) */
  nrrdKindScalar,            /*  6: effectively, same as a stub */
  nrrdKindComplex,           /*  7: real and imaginary components */
  nrrdKind2Vector,           /*  8: 2 component vector */
  nrrdKind3Color,            /*  9: ANY 3-component color value */
  nrrdKind4Color,            /* 10: ANY 4-component color value */
  nrrdKind3Vector,           /* 11: 3 component vector */
  nrrdKind3Normal,           /* 12: 3 component vector, assumed normalized */
  nrrdKind4Vector,           /* 13: 4 component vector */
  nrrdKind2DSymMatrix,       /* 14: Mxx Mxy Myy */
  nrrdKind2DMaskedSymMatrix, /* 15: mask Mxx Mxy Myy */
  nrrdKind2DMatrix,          /* 16: Mxx Mxy Myx Myy */
  nrrdKind2DMaskedMatrix,    /* 17: mask Mxx Mxy Myx Myy */
  nrrdKind3DSymMatrix,       /* 18: Mxx Mxy Mxz Myy Myz Mzz */
  nrrdKind3DMaskedSymMatrix, /* 19: mask Mxx Mxy Mxz Myy Myz Mzz */
  nrrdKind3DMatrix,          /* 20: Mxx Mxy Mxz Myx Myy Myz Mzx Mzy Mzz */
  nrrdKind3DMaskedMatrix,    /* 21: mask Mxx Mxy Mxz Myx Myy Myz Mzx Mzy Mzz */
  nrrdKindLast
};
#define NRRD_KIND_MAX           21

/*
******** nrrdAxisInfo enum
**
** the different pieces of per-axis information recorded in a nrrd
*/
enum {
  nrrdAxisInfoUnknown,
  nrrdAxisInfoSize,                   /*  1: number of samples along axis */
#define NRRD_AXIS_INFO_SIZE_BIT      (1<< 1)
  nrrdAxisInfoSpacing,                /*  2: spacing between samples */
#define NRRD_AXIS_INFO_SPACING_BIT   (1<< 2)
  nrrdAxisInfoThickness,              /*  3: thickness of sample region */
#define NRRD_AXIS_INFO_THICKNESS_BIT (1<< 3)
  nrrdAxisInfoMin,                    /*  4: min pos. assoc. w/ 1st sample */
#define NRRD_AXIS_INFO_MIN_BIT       (1<< 4) 
  nrrdAxisInfoMax,                    /*  5: max pos. assoc. w/ last sample */
#define NRRD_AXIS_INFO_MAX_BIT       (1<< 5)
  nrrdAxisInfoSpaceDirection,         /*  6: inter-sample vector in "space" */
#define NRRD_AXIS_INFO_SPACEDIRECTION_BIT (1<< 6)
  nrrdAxisInfoCenter,                 /*  7: cell vs. node */
#define NRRD_AXIS_INFO_CENTER_BIT    (1<< 7)
  nrrdAxisInfoKind,                   /*  8: from the nrrdKind* enum */
#define NRRD_AXIS_INFO_KIND_BIT      (1<< 8)
  nrrdAxisInfoLabel,                  /*  9: string describing the axis */
#define NRRD_AXIS_INFO_LABEL_BIT     (1<< 9)
  nrrdAxisInfoUnits,                  /* 10: from the nrrdUnit* enum */
#define NRRD_AXIS_INFO_UNITS_BIT     (1<<10)
  nrrdAxisInfoLast
};
#define NRRD_AXIS_INFO_MAX               10
#define NRRD_AXIS_INFO_ALL  \
    ((1<<1)|(1<<2)|(1<<3)|(1<<4)|(1<<5)|(1<<6)|(1<<7)|(1<<8)|(1<<9)|(1<<10))
#define NRRD_AXIS_INFO_NONE 0

/*
******** nrrdBasicInfo enum
**
** the non-per-axis (or per-array) pieces of information that could
** meaningfully be copied between nrrds (hence the void *data is not
** included).
**
** "Basic" is named after the "basic field specifications" described
** in the NRRD file format definition
*/
enum {
  nrrdBasicInfoUnknown,
  nrrdBasicInfoData,                        /*  1 */
#define NRRD_BASIC_INFO_DATA_BIT           (1<< 1)
  nrrdBasicInfoType,                        /*  2 */
#define NRRD_BASIC_INFO_TYPE_BIT           (1<< 2)
  nrrdBasicInfoBlocksize,                   /*  3 */
#define NRRD_BASIC_INFO_BLOCKSIZE_BIT      (1<< 3)
  nrrdBasicInfoDimension,                   /*  4 */
#define NRRD_BASIC_INFO_DIMENSION_BIT      (1<< 4)
  nrrdBasicInfoContent,                     /*  5 */
#define NRRD_BASIC_INFO_CONTENT_BIT        (1<< 5)
  nrrdBasicInfoSampleUnits,                 /*  6 */
#define NRRD_BASIC_INFO_SAMPLEUNITS_BIT    (1<< 6)
  nrrdBasicInfoSpace,                       /*  7 */
#define NRRD_BASIC_INFO_SPACE_BIT          (1<< 7)
  nrrdBasicInfoSpaceDimension,              /*  8 */
#define NRRD_BASIC_INFO_SPACEDIMENSION_BIT (1<< 8)
  nrrdBasicInfoSpaceUnits,                  /*  9 */
#define NRRD_BASIC_INFO_SPACEUNITS_BIT     (1<< 9)
  nrrdBasicInfoSpaceOrigin,                 /* 10 */
#define NRRD_BASIC_INFO_SPACEORIGIN_BIT    (1<<10)
  nrrdBasicInfoOldMin,                      /* 11 */
#define NRRD_BASIC_INFO_OLDMIN_BIT         (1<<11)
  nrrdBasicInfoOldMax,                      /* 12 */ 
#define NRRD_BASIC_INFO_OLDMAX_BIT         (1<<12)
  nrrdBasicInfoComments,                    /* 13 */
#define NRRD_BASIC_INFO_COMMENTS_BIT       (1<<13)
  nrrdBasicInfoKeyValuePairs,               /* 14 */
#define NRRD_BASIC_INFO_KEYVALUEPAIRS_BIT  (1<<14)
  nrrdBasicInfoLast
};
#define NRRD_BASIC_INFO_MAX                    14
#define NRRD_BASIC_INFO_ALL  \
    ((1<<1)|(1<<2)|(1<<3)|(1<<4)|(1<<5)|(1<<6)|(1<<7)|(1<<8)|(1<<9)|(1<<10)\
     |(1<<11)|(1<<12)|(1<<13)|(1<<14))
#define NRRD_BASIC_INFO_SPACE (NRRD_BASIC_INFO_SPACE_BIT \
                               | NRRD_BASIC_INFO_SPACEDIMENSION_BIT \
                               | NRRD_BASIC_INFO_SPACEUNITS_BIT \
                               | NRRD_BASIC_INFO_SPACEORIGIN_BIT)
#define NRRD_BASIC_INFO_NONE 0

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
** parseNrrd.c:
**    _nrrdReadNrrdParseInfo[]
** enumsNrrd.c:
**    nrrdField definition
** simple.c:
**    _nrrdFieldCheck[]
** write.c:
**    _nrrdFieldInteresting()
**    _nrrdSprintFieldInfo()
** to some extent, in this file:
**    nrrdAxisInfo and nrrdBasicInfo enums
** axis.c (for per-axis info):
**    _nrrdAxisInfoCopy()
** methodsNrrd.c:
**    lots of functions, but you knew that ...
*/
enum {
  nrrdField_unknown,
  nrrdField_comment,          /*  1 */
  nrrdField_content,          /*  2 */
  nrrdField_number,           /*  3 */
  nrrdField_type,             /*  4 */
  nrrdField_block_size,       /*  5 */
  nrrdField_dimension,        /*  6 */
  nrrdField_space,            /*  7 */
  nrrdField_space_dimension,  /*  8 */
  nrrdField_sizes,            /*  9 ----- begin per-axis ----- */
  nrrdField_spacings,         /* 10 */
  nrrdField_thicknesses,      /* 11 */
  nrrdField_axis_mins,        /* 12 */
  nrrdField_axis_maxs,        /* 13 */
  nrrdField_space_directions, /* 14 */
  nrrdField_centers,          /* 15 */
  nrrdField_kinds,            /* 16 */
  nrrdField_labels,           /* 17 */
  nrrdField_units,            /* 18 ------ end per-axis ------ */
  nrrdField_min,              /* 19 */
  nrrdField_max,              /* 20 */
  nrrdField_old_min,          /* 21 */
  nrrdField_old_max,          /* 22 */
  nrrdField_endian,           /* 23 */
  nrrdField_encoding,         /* 24 */
  nrrdField_line_skip,        /* 25 */
  nrrdField_byte_skip,        /* 26 */
  nrrdField_keyvalue,         /* 27 */
  nrrdField_sample_units,     /* 28 */
  nrrdField_space_units,      /* 29 */
  nrrdField_space_origin,     /* 30 */
  nrrdField_data_file,        /* 31 */
  nrrdField_last
};
#define NRRD_FIELD_MAX           31

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

/*
******** nrrdSpace* enum
**
** Identifies the space in which which the origin and direction
** vectors have their coordinates measured.  When a direction is named
** here (like "Left" or "Anterior"), that implies a basis vector that
** points in that direction, along which that coordinate becomes *larger*
** (this is the opposite of MetaIO, for example).
**
** All of these spaces have a well-defined expected dimension, as
** determined by nrrdSpaceDimension(), and setting a nrrd to be in
** such a space, by nrrdSpaceSet(), will automatically set nrrd->spaceDim.
**
** The first six spaces here are PATIENT-ORIENTED spaces, which are
** properly speaking aligned with the patient, and not the scanner
** itself.  But nrrdSpaceScannerXYZ and nrrdSpaceScannerXYZTime are
** DEVICE-ORIENTED spaces, irrespective of the patient, used in a
** previous version of the DICOM standard.  When the two spaces are
** lined up with normal patient orientation in the scanner,
** nrrdSpaceScannerXYZ is the same as nrrdSpaceLeftPosteriorSuperior.
** To quote Part 3 (Information Object Definitions) of the DICOM spec
** (page 275): "If a patient lies parallel to the ground, face-up on
** the table, with his feet-to-head direction same as the
** front-to-back direction of the imaging equipment, the direction of
** the axes of this patient based coordinate system and the equipment
** based coordinate system in previous versions of this Standard will
** coincide."
**
** Keep in sync:
**   enumsNrrd.c: nrrdSpace airEnum
**      simple.c: int nrrdSpaceDimension(int space)
*/
enum {
  nrrdSpaceUnknown,
  nrrdSpaceRightAnteriorSuperior,     /*  1: NIFTI-1 (right-handed) */
  nrrdSpaceLeftAnteriorSuperior,      /*  2: standard Analyze (left-handed) */
  nrrdSpaceLeftPosteriorSuperior,     /*  3: DICOM 3.0 (right-handed) */
  nrrdSpaceRightAnteriorSuperiorTime, /*  4: */
  nrrdSpaceLeftAnteriorSuperiorTime,  /*  5: */
  nrrdSpaceLeftPosteriorSuperiorTime, /*  6: */
  nrrdSpaceScannerXYZ,                /*  7: ACR/NEMA 2.0 (pre-DICOM 3.0) */
  nrrdSpaceScannerXYZTime,            /*  8: */
  nrrdSpace3DRightHanded,             /*  9: */
  nrrdSpace3DLeftHanded,              /* 10: */
  nrrdSpace3DRightHandedTime,         /* 11: */
  nrrdSpace3DLeftHandedTime,          /* 12: */
  nrrdSpaceLast
};
#define NRRD_SPACE_MAX                   12

/*
******** nrrdSpacingStatus* enum
**
** a way of describing how spacing information is known or not known for a 
** given axis, as determined by nrrdSpacingCalculate
*/
enum {
  nrrdSpacingStatusUnknown,           /* 0: nobody knows,
                                         or invalid axis choice */
  nrrdSpacingStatusNone,              /* 1: neither axis->spacing nor
                                         axis->spaceDirection is set */
  nrrdSpacingStatusScalarNoSpace,     /* 2: axis->spacing set,
                                         w/out space info */
  nrrdSpacingStatusScalarWithSpace,   /* 3: axis->spacing set, but there *is*
                                         space info, which means the spacing
                                         does *not* live in the surrounding
                                         space */
  nrrdSpacingStatusVector,            /* 4: axis->spaceDirection set, and 
                                         measured according to surrounding
                                         space */
  nrrdSpacingStatusLast
};

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
  nrrdMeasureLineSlope,      /* 14: slope of line of best fit */
  nrrdMeasureLineIntercept,  /* 15: y-intercept of line of best fit */
  nrrdMeasureLineError,      /* 16: error of line fitting */
  /* 
  ** the nrrduMeasureHisto... measures interpret the array as a
  ** histogram of some implied value distribution
  */
  nrrdMeasureHistoMin,       /* 17 */
  nrrdMeasureHistoMax,       /* 18 */
  nrrdMeasureHistoMean,      /* 19 */
  nrrdMeasureHistoMedian,    /* 20 */
  nrrdMeasureHistoMode,      /* 21 */
  nrrdMeasureHistoProduct,   /* 22 */
  nrrdMeasureHistoSum,       /* 23 */
  nrrdMeasureHistoL2,        /* 24 */
  nrrdMeasureHistoVariance,  /* 25 */
  nrrdMeasureHistoSD,        /* 26 */
  nrrdMeasureLast
};
#define NRRD_MEASURE_MAX        26
#define NRRD_MEASURE_DESC \
   "Possibilities include:\n " \
   "\b\bo \"min\", \"max\", \"mean\", \"median\", \"mode\", \"variance\", " \
     "\"skew\"\n (self-explanatory)\n " \
   "\b\bo \"intc\", \"slope\", \"error\": " \
     "intercept, slope, and error from line fitting\n " \
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
** [0,255] (uchar) or [SCHAR_MIN,SCHAR_MAX] (signed char)
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
  nrrdBinaryOpIf,         /* 20 */
  nrrdBinaryOpLast
};
#define NRRD_BINARY_OP_MAX   20

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
