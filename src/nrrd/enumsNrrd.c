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

#include "nrrd.h"

/*
** Rules of thumb for editing these things.  The airEnum definitions are
** unfortunately EXTREMELY sensitive to small typo errors, and there is
** no good way to detect the errors.  So:
**
** 1) Be awake and undistracted.  Turn down the music.
** 2) When editing the char arrays, make sure that you put commas
**    where you mean them to be.  C's automatic string concatenation 
**    is not your friend here.
** 3) When editing the *StrEqv and *ValEqv arrays, make absolutely
**    sure that both are changed in parallel.  Use only one enum value
**    per line; putting all equivalents on that line, and make sure that
**    there is one line in both *StrEqv and *ValEqv for all the possible
**    enum values, and that there are as many elements in each line.
** 4) Make sure that additions here are reflected in nrrdEnums.h and
**    vice versa.
*/

/* ------------------------ nrrdFormat ------------------------- */

char
_nrrdFormatStr[NRRD_FORMAT_MAX+1][AIR_STRLEN_SMALL] = {
  "(unknown_format)",
  "nrrd",
  "pnm",
  "png",
  "vtk",
  "text"
};

char
_nrrdFormatDesc[NRRD_FORMAT_MAX+1][AIR_STRLEN_MED] = {
  "unknown_format",
  "native format for nearly raw raster data",
  "Portable aNy Map: includes PGM for grayscale and PPM for color",
  "Portable Network Graphics: lossless compression of 8- and 16-bit data",
  "Visualization ToolKit STRUCTURED_POINTS data",
  "white-space-delimited ascii encoding of floats"
};

char
_nrrdFormatStrEqv[][AIR_STRLEN_SMALL] = {
  "nrrd",
  "pnm",
  "png",
  "vtk",
  "table", "text", "txt",
  ""
};

int
_nrrdFormatValEqv[] = {
  nrrdFormatNRRD,
  nrrdFormatPNM,
  nrrdFormatPNG,
  nrrdFormatVTK,
  nrrdFormatTable, nrrdFormatTable, nrrdFormatTable
};

airEnum
_nrrdFormat = {
  "format",
  NRRD_FORMAT_MAX,
  _nrrdFormatStr,  NULL,
  _nrrdFormatDesc,
  _nrrdFormatStrEqv, _nrrdFormatValEqv,
  AIR_FALSE
};
airEnum *
nrrdFormat = &_nrrdFormat;

/* ------------------------ nrrdBoundary ------------------------- */

char
_nrrdBoundaryStr[NRRD_BOUNDARY_MAX+1][AIR_STRLEN_SMALL] = {
  "(unknown_boundary)",
  "pad",
  "bleed",
  "wrap",
  "weight"
};

char
_nrrdBoundaryDesc[NRRD_BOUNDARY_MAX+1][AIR_STRLEN_MED] = {
  "unknown boundary behavior",
  "pad with some specified value",
  "copy values from edge outward as needed",
  "wrap around to other end of axis",
  "re-weight (by normalization) samples within axis range"
};

airEnum
_nrrdBoundary = {
  "boundary behavior",
  NRRD_BOUNDARY_MAX,
  _nrrdBoundaryStr, NULL,
  _nrrdBoundaryDesc,
  NULL, NULL,
  AIR_FALSE
};
airEnum *
nrrdBoundary = &_nrrdBoundary;

/* ------------------------ nrrdMagic ------------------------- */

char
_nrrdMagicStr[NRRD_MAGIC_MAX+1][AIR_STRLEN_SMALL] = {
  "(unknown_magic)",
  "NRRD00.01",
  "NRRD0001",
  "P2",
  "P3",
  "P5",
  "P6",
  "\211PNG",
  "# vtk DataFile Version 1.0",
  "# vtk DataFile Version 2.0",
};

char
_nrrdMagicDesc[NRRD_MAGIC_MAX+1][AIR_STRLEN_MED] = {
  "unknown magic",
  "old style of NRRD magic",
  "NRRD magic, version 1",
  "ascii grayscale (PGM) image",
  "ascii color (PPM) image",
  "binary grayscale (PGM) image",
  "binary color (PPM) image",
  "Portable Network Graphics (PNG)",
  "VTK magic, version 1.0",
  "VTK magic, version 2.0"
};

airEnum
_nrrdMagic = {
  "magic",
  NRRD_MAGIC_MAX+1,
  _nrrdMagicStr, NULL,
  _nrrdMagicDesc,
  NULL, NULL,
  AIR_TRUE
};
airEnum *
nrrdMagic = &_nrrdMagic;

/* ------------------------ nrrdType ------------------------- */

char 
_nrrdTypeStr[NRRD_TYPE_MAX+1][AIR_STRLEN_SMALL] = {
  "(unknown_type)",
  "signed char",
  "unsigned char",
  "short",
  "unsigned short",
  "int",
  "unsigned int",
  "long long int",
  "unsigned long long int",
  "float",
  "double",
  "block"
};

char 
_nrrdTypeDesc[NRRD_TYPE_MAX+1][AIR_STRLEN_MED] = {
  "unknown type",
  "signed 1-byte integer",
  "unsigned 1-byte integer",
  "signed 2-byte integer",
  "unsigned 2-byte integer",
  "signed 4-byte integer",
  "unsigned 4-byte integer",
  "signed 8-byte integer",
  "unsigned 8-byte integer",
  "4-byte floating point",
  "8-byte floating point",
  "size user-defined at run-time"
};

#define ntCH nrrdTypeChar
#define ntUC nrrdTypeUChar
#define ntSH nrrdTypeShort
#define ntUS nrrdTypeUShort
#define ntIN nrrdTypeInt
#define ntUI nrrdTypeUInt
#define ntLL nrrdTypeLLong
#define ntUL nrrdTypeULLong
#define ntFL nrrdTypeFloat
#define ntDB nrrdTypeDouble
#define ntBL nrrdTypeBlock

char
_nrrdTypeStrEqv[][AIR_STRLEN_SMALL] = {
  "signed char", /* but NOT just "char" */ "int8", "int8_t",
  "uchar", "unsigned char", "uint8", "uint8_t", 
  "short", "short int", "signed short", "signed short int", "int16", "int16_t",
  "ushort", "unsigned short", "unsigned short int", "uint16", "uint16_t", 
  "int", "signed int", "int32", "int32_t", 
  "uint", "unsigned int", "uint32", "uint32_t",
  "longlong", "long long", "long long int", "signed long long",
               "signed long long int", "int64", "int64_t", 
  "ulonglong", "unsigned long long", "unsigned long long int",
               "uint64", "uint64_t", 
  "float",
  "double",
  "block",
  ""
};

int
_nrrdTypeValEqv[] = {
  ntCH, ntCH, ntCH,
  ntUC, ntUC, ntUC, ntUC,
  ntSH, ntSH, ntSH, ntSH, ntSH, ntSH,
  ntUS, ntUS, ntUS, ntUS, ntUS,
  ntIN, ntIN, ntIN, ntIN,
  ntUI, ntUI, ntUI, ntUI, 
  ntLL, ntLL, ntLL, ntLL, ntLL, ntLL, ntLL, 
  ntUL, ntUL, ntUL, ntUL, ntUL, 
  ntFL,
  ntDB,
  ntBL
};

airEnum
_nrrdType = {
  "type",
  NRRD_TYPE_MAX,
  _nrrdTypeStr, NULL,
  _nrrdTypeDesc,
  _nrrdTypeStrEqv, _nrrdTypeValEqv,
  AIR_FALSE
};
airEnum *
nrrdType = &_nrrdType;

/* ------------------------ nrrdEncoding ------------------------- */

char
_nrrdEncodingStr[NRRD_ENCODING_MAX+1][AIR_STRLEN_SMALL] = {
  "(unknown_encoding)",
  "raw",
  "ascii",
  "hex",
  "gz",
  "bz2"
};

char
_nrrdEncodingDesc[NRRD_ENCODING_MAX+1][AIR_STRLEN_MED] = {
  "unknown encoding",
  "file is byte-for-byte same as memory representation",
  "values written out in ASCII",
  "case-insenstive hexadecimal encoding (2 chars / byte)",
  "gzip compression of binary encoding",
  "bzip2 compression of binary encoding"
};

char
_nrrdEncodingStrEqv[][AIR_STRLEN_SMALL] = {
  "(unknown_encoding)",
  "raw",
  "txt", "text", "ascii",
  "hex",
  "gz", "gzip",
  "bz2", "bzip2",
  ""
};

int
_nrrdEncodingValEqv[] = {
  nrrdEncodingUnknown,
  nrrdEncodingRaw,
  nrrdEncodingAscii, nrrdEncodingAscii, nrrdEncodingAscii,
  nrrdEncodingHex,
  nrrdEncodingGzip, nrrdEncodingGzip,
  nrrdEncodingBzip2, nrrdEncodingBzip2,
  nrrdEncodingLast
};

airEnum
_nrrdEncoding = {
  "encoding",
  NRRD_ENCODING_MAX,
  _nrrdEncodingStr, NULL,
  _nrrdEncodingDesc,
  _nrrdEncodingStrEqv, _nrrdEncodingValEqv,
  AIR_FALSE
};
airEnum *
nrrdEncoding = &_nrrdEncoding;

/* ------------------------ nrrdMeasure ------------------------- */

char
_nrrdMeasureStr[NRRD_MEASURE_MAX+1][AIR_STRLEN_SMALL] = {
  "(unknown_measure)",
  "min",
  "max",
  "mean",
  "median",
  "mode",
  "product",
  "sum",
  "L1",
  "L2",
  "Linf",
  "variance",
  "SD",
  "histo-min",
  "histo-max",
  "histo-mean",
  "histo-median",
  "histo-mode",
  "histo-product",
  "histo-sum",
  "histo-l2",
  "histo-variance"
};

char
_nrrdMeasureDesc[NRRD_MEASURE_MAX+1][AIR_STRLEN_MED] = {
  "unknown measure",
  "minimum of values",
  "maximum of values",
  "mean of values",
  "median of values",
  "mode of values",
  "product of values",
  "sum of values",
  "L1 norm of values",
  "L2 norm of values",
  "Linf norm of values",
  "variance of values",
  "SD of values",
  "minimum of histogrammed values",
  "maximum of histogrammed values",
  "mean of histogrammed values",
  "median of histogrammed values",
  "mode of histogrammed values",
  "product of histogrammed values",
  "sum of histogrammed values",
  "L2 norm of histogrammed values",
  "variance of histogrammed values"
};

char
_nrrdMeasureStrEqv[][AIR_STRLEN_SMALL] = {
  "(unknown_measure)",
  "min",
  "max",
  "mean",
  "median",
  "mode",
  "product", "prod",
  "sum",
  "L1",
  "L2",
  "Linf",
  "variance", "var",
  "SD",
  "histo-min",
  "histo-max",
  "histo-mean",
  "histo-median",
  "histo-mode",
  "histo-product",
  "histo-sum",
  "histo-l2",
  "histo-variance", "histo-var"
};

int
_nrrdMeasureValEqv[] = {
  nrrdMeasureUnknown,
  nrrdMeasureMin,
  nrrdMeasureMax,
  nrrdMeasureMean,
  nrrdMeasureMedian,
  nrrdMeasureMode,
  nrrdMeasureProduct, nrrdMeasureProduct,
  nrrdMeasureSum,
  nrrdMeasureL1,
  nrrdMeasureL2,
  nrrdMeasureLinf,
  nrrdMeasureVariance, nrrdMeasureVariance,
  nrrdMeasureSD,
  nrrdMeasureHistoMin,
  nrrdMeasureHistoMax,
  nrrdMeasureHistoMean,
  nrrdMeasureHistoMedian,
  nrrdMeasureHistoMode,
  nrrdMeasureHistoProduct,
  nrrdMeasureHistoSum,
  nrrdMeasureHistoL2,
  nrrdMeasureHistoVariance, nrrdMeasureHistoVariance
};

airEnum
_nrrdMeasure = {
  "measure",
  NRRD_MEASURE_MAX,
  _nrrdMeasureStr, NULL,
  _nrrdMeasureDesc,
  _nrrdMeasureStrEqv, _nrrdMeasureValEqv, 
  AIR_FALSE
};
airEnum *
nrrdMeasure = &_nrrdMeasure;

/* ------------------------ nrrdCenter ------------------------- */

char
_nrrdCenterStr[NRRD_CENTER_MAX+1][AIR_STRLEN_SMALL] = {
  "(unknown_center)",
  "node",
  "cell"
};

char
_nrrdCenterDesc[NRRD_CENTER_MAX+1][AIR_STRLEN_MED] = {
  "unknown centering",
  "samples are at boundaries between elements along axis",
  "samples are at centers of elements along axis",
};

airEnum
_nrrdCenter_enum = {
  "centering",
  NRRD_CENTER_MAX,
  _nrrdCenterStr, NULL,
  _nrrdCenterDesc,
  NULL, NULL,
  AIR_FALSE
};
airEnum *
nrrdCenter = &_nrrdCenter_enum;

/* ------------------------ nrrdAxesInfo ------------------------- */

char
_nrrdAxesInfoStr[NRRD_AXESINFO_MAX+1][AIR_STRLEN_SMALL] = {
  "(unknown_axes_info)",
  "size",
  "spacing",
  "min",
  "max",
  "center",
  "label",
  "unit"
};

char
_nrrdAxesInfoDesc[NRRD_AXESINFO_MAX+1][AIR_STRLEN_MED] = {
  "unknown axes info",
  "number of samples along axis",
  "spacing between samples",
  "minimum position in \"world\" space associated with axis",
  "maximum position in \"world\" space associated with axis",
  "centering style for samples along this axis",
  "short description of space or value spanned by axis",
  "units in which sample spacing is measured"
};

airEnum
_nrrdAxesInfo = {
  "axes_info",
  NRRD_AXESINFO_MAX,
  _nrrdAxesInfoStr, NULL,
  _nrrdAxesInfoDesc,
  NULL, NULL,
  AIR_TRUE
};
airEnum *
nrrdAxesInfo = &_nrrdAxesInfo;
  
/* ------------------------ nrrdField ------------------------- */

char
_nrrdFieldStr[NRRD_FIELD_MAX+1][AIR_STRLEN_SMALL] = {
  "Ernesto \"Che\" Guevara",
  "#",
  "content",
  "number",
  "type",
  "block size",
  "dimension",
  "sizes",
  "spacings",
  "axis mins",
  "axis maxs",
  "centers",
  "labels",
  "units",
  "min",
  "max",
  "old min",
  "old max",
  "data file",
  "endian",
  "encoding",
  "line skip",
  "byte skip"
};

char
_nrrdFieldDesc[NRRD_FIELD_MAX+1][AIR_STRLEN_MED] = {
  "unknown field identifier",
  "comment",
  "short description of whole array and/or its provinance",
  "total number of samples in array",
  "type of sample value",
  "number of bytes in one block (for block-type)",
  "number of axes in array",
  "list of number of samples along each axis, aka \"dimensions\" of the array",
  "list of sample spacings along each axis",
  "list of minimum positions associated with each axis",
  "list of maximum positions associated with each axis",
  "list of sample centerings for each axis",
  "list of short descriptions for each axis",
  "list of units in which each axes' spacing is measured",
  "known global minimum value in whole array",
  "known global maximum value in whole array",
  "minimum array value prior to quantization",
  "maximum array value prior to quantization",
  "with detached headers, where is data to be found",
  "endiannes of data as written in file",
  "encoding of data written in file",
  "number of lines to skip prior to byte skip and reading data",
  "number of bytes to skip after line skip and prior to reading data"
};

char
_nrrdFieldStrEqv[][AIR_STRLEN_SMALL]  = {
  "#",
  "content",
  "number",
  "type",
  "block size", "blocksize",
  "dimension",
  "sizes",
  "spacings",
  "axis mins", "axismins",
  "axis maxs", "axismaxs",
  "centers",
  "labels",
  "units",
  "min",
  "max",
  "old min", "oldmin",
  "old max", "oldmax",
  "data file", "datafile",
  "endian",
  "encoding",
  "line skip", "lineskip",
  "byte skip", "byteskip",
  ""
};

int
_nrrdFieldValEqv[] = {
  nrrdField_comment,
  nrrdField_content,
  nrrdField_number,
  nrrdField_type,
  nrrdField_block_size, nrrdField_block_size,
  nrrdField_dimension,
  nrrdField_sizes,
  nrrdField_spacings,
  nrrdField_axis_mins, nrrdField_axis_mins,
  nrrdField_axis_maxs, nrrdField_axis_maxs,
  nrrdField_centers,
  nrrdField_labels,
  nrrdField_units,
  nrrdField_min,
  nrrdField_max,
  nrrdField_old_min, nrrdField_old_min,
  nrrdField_old_max, nrrdField_old_max,
  nrrdField_data_file, nrrdField_data_file,
  nrrdField_endian,
  nrrdField_encoding,
  nrrdField_line_skip, nrrdField_line_skip,
  nrrdField_byte_skip, nrrdField_byte_skip,
};

airEnum
_nrrdField = {
  "nrrd_field",
  NRRD_FIELD_MAX,
  _nrrdFieldStr, NULL,
  _nrrdFieldDesc,
  _nrrdFieldStrEqv, _nrrdFieldValEqv, 
  AIR_FALSE
};
airEnum *
nrrdField = &_nrrdField;

/* ------------------------ nrrdNonExist ------------------------- */

/*
char
_nrrdNonExistStr[NRRD_NON_EXIST_MAX+1][AIR_STRLEN_SMALL] = {
  "(unknown)",
  "true",
  "false"
};
*/


/* ------------------------ nrrdUnaryOp ---------------------- */

#define nuNeg nrrdUnaryOpNegative
#define nuRcp nrrdUnaryOpReciprocal
#define nuSin nrrdUnaryOpSin
#define nuCos nrrdUnaryOpCos
#define nuTan nrrdUnaryOpTan
#define nuAsn nrrdUnaryOpAsin
#define nuAcs nrrdUnaryOpAcos
#define nuAtn nrrdUnaryOpAtan
#define nuExp nrrdUnaryOpExp
#define nuLge nrrdUnaryOpLog
#define nuLgt nrrdUnaryOpLog10
#define nuL1p nrrdUnaryOpLog1p
#define nuSqt nrrdUnaryOpSqrt
#define nuCbt nrrdUnaryOpCbrt
#define nuErf nrrdUnaryOpErf
#define nuCil nrrdUnaryOpCeil
#define nuFlr nrrdUnaryOpFloor
#define nuRup nrrdUnaryOpRoundUp
#define nuRdn nrrdUnaryOpRoundDown
#define nuAbs nrrdUnaryOpAbs
#define nuSgn nrrdUnaryOpSgn
#define nuExs nrrdUnaryOpExists

char 
_nrrdUnaryOpStr[NRRD_UNARY_OP_MAX+1][AIR_STRLEN_SMALL] = {
  "(unknown_unary_op)",
  "-",
  "r",
  "sin",
  "cos",
  "tan",
  "asin",
  "acos",
  "atan",
  "exp",
  "log",
  "log10",
  "log1p",
  "sqrt",
  "cbrt",
  "erf",
  "ceil",
  "floor",
  "roundup",
  "rounddown",
  "abs",
  "sgn",
  "exists"
};

char 
_nrrdUnaryOpDesc[NRRD_UNARY_OP_MAX+1][AIR_STRLEN_MED] = {
  "unknown unary op",
  "negative, additive inverse",
  "reciprocal, multiplicative inverse",
  "sin",
  "cos",
  "tan",
  "arcsin",
  "arccos",
  "arctan",
  "e raised to something",
  "natural (base e) logarithm",
  "base 10 logarithm",
  "ln(1+x), accurate for small x",
  "square root",
  "cube root",
  "error function (integral of gaussian)",
  "smallest integer greater than or equal",
  "largest integer less than or equal",
  "round to closest integer (0.5 rounded to 1)",
  "round to closest integer (0.5 rounded to 0)",
  "absolute value",
  "sign of value (-1, 0, or 1)",
  "value is not infinity or NaN"
};

char
_nrrdUnaryOpStrEqv[][AIR_STRLEN_SMALL] = {
  "-", "neg", "negative", "minus",
  "r", "recip",
  "sin",
  "cos",
  "tan",
  "asin", "arcsin",
  "acos", "arccos",
  "atan", "arctan",
  "exp",
  "ln", "log",
  "log10",
  "ln1p", "log1p",
  "sqrt",
  "cbrt",
  "erf",
  "ceil",
  "floor",
  "roundup", "rup",
  "rounddown", "rdown", "rdn",
  "abs", "fabs",
  "sgn", "sign",
  "exists",
  ""
};

int
_nrrdUnaryOpValEqv[] = {
  nuNeg, nuNeg, nuNeg, nuNeg,
  nuRcp, nuRcp,
  nuSin,
  nuCos,
  nuTan,
  nuAsn, nuAsn,
  nuAcs, nuAcs,
  nuAtn, nuAtn,
  nuExp,
  nuLge, nuLge,
  nuLgt,
  nuL1p, nuL1p,
  nuSqt,
  nuCbt,
  nuErf,
  nuCil,
  nuFlr,
  nuRup, nuRup,
  nuRdn, nuRdn, nuRdn,
  nuAbs, nuAbs,
  nuSgn, nuSgn,
  nuExs
};

airEnum
_nrrdUnaryOp_enum = {
  "unary op",
  NRRD_UNARY_OP_MAX,
  _nrrdUnaryOpStr, NULL,
  _nrrdUnaryOpDesc,
  _nrrdUnaryOpStrEqv, _nrrdUnaryOpValEqv,
  AIR_FALSE
};
airEnum *
nrrdUnaryOp = &_nrrdUnaryOp_enum;

/* ------------------------ nrrdBinaryOp ---------------------- */

char 
_nrrdBinaryOpStr[NRRD_BINARY_OP_MAX+1][AIR_STRLEN_SMALL] = {
  "(unknown_binary_op)",
  "+",
  "-",
  "x",
  "/",
  "^",
  "%",
  "fmod",
  "atan2",
  "min",
  "max",
  "lt",
  "lte",
  "gt",
  "gte",
  "comp",
  "eq",
  "neq",
  "exists"
};

char 
_nrrdBinaryOpDesc[NRRD_BINARY_OP_MAX+1][AIR_STRLEN_MED] = {
  "unknown binary op",
  "add",
  "subtract",
  "multiply",
  "divide",
  "power",
  "integer modulo",
  "fractional modulo",
  "two-argment arctangent based on atan2()",
  "miniumum",
  "maximum",
  "less then",
  "less then or equal",
  "greater than",
  "greater than or equal",
  "compare (resulting in -1, 0, or 1)",
  "equal",
  "not equal",
  "if exists(a), then a, else b"
};

#define nbAdd nrrdBinaryOpAdd
#define nbSub nrrdBinaryOpSubtract
#define nbMul nrrdBinaryOpMultiply
#define nbDiv nrrdBinaryOpDivide
#define nbPow nrrdBinaryOpPow
#define nbMod nrrdBinaryOpMod
#define nbFmd nrrdBinaryOpFmod
#define nbAtn nrrdBinaryOpAtan2
#define nbMin nrrdBinaryOpMin
#define nbMax nrrdBinaryOpMax
#define nbLt  nrrdBinaryOpLT
#define nbLte nrrdBinaryOpLTE
#define nbGt  nrrdBinaryOpGT
#define nbGte nrrdBinaryOpGTE
#define nbCmp nrrdBinaryOpCompare
#define nbEq  nrrdBinaryOpEqual
#define nbNeq nrrdBinaryOpNotEqual
#define nbExt nrrdBinaryOpExists

char
_nrrdBinaryOpStrEqv[][AIR_STRLEN_SMALL] = {
  "+", "plus", "add",
  "-", "minus", "subtract", "sub", 
  "x", "*", "times", "multiply", "product",
  "/", "divide", "quotient",
  "^", "pow", "power",
  "%", "mod", "modulo",
  "fmod",
  "atan2", 
  "min", "minimum",
  "max", "maximum",
  "lt", "<", "less", "lessthan",
  "lte", "<=", "lessthanorequal",
  "gt", ">", "greater", "greaterthan",
  "gte", ">=", "greaterthanorequal",
  "comp", "compare",
  "eq", "==", "equal",
  "neq", "!=", "notequal",
  "exists",
  ""
};

int
_nrrdBinaryOpValEqv[] = {
  nbAdd, nbAdd, nbAdd,
  nbSub, nbSub, nbSub, nbSub, 
  nbMul, nbMul, nbMul, nbMul, nbMul, 
  nbDiv, nbDiv, nbDiv, 
  nbPow, nbPow, nbPow,
  nbMod, nbMod, nbMod, 
  nbFmd,
  nbAtn,
  nbMin, nbMin,
  nbMax, nbMax,
  nbLt, nbLt, nbLt, nbLt,
  nbLte, nbLte, nbLte,
  nbGt, nbGt, nbGt, nbGt,
  nbGte, nbGte, nbGte,
  nbCmp, nbCmp,
  nbEq, nbEq, nbEq,
  nbNeq, nbNeq, nbNeq,
  nbExt
};

airEnum
_nrrdBinaryOp_enum = {
  "binary op",
  NRRD_BINARY_OP_MAX,
  _nrrdBinaryOpStr, NULL,
  _nrrdBinaryOpDesc,
  _nrrdBinaryOpStrEqv, _nrrdBinaryOpValEqv,
  AIR_FALSE
};
airEnum *
nrrdBinaryOp = &_nrrdBinaryOp_enum;

/* ------------------------ nrrdTernaryOp ---------------------- */

char 
_nrrdTernaryOpStr[NRRD_TERNARY_OP_MAX+1][AIR_STRLEN_SMALL] = {
  "(unknown_ternary_op)",
  "clamp",
  "lerp",
  "exists",
  "in_op",
  "in_cl"
};

char 
_nrrdTernaryOpDesc[NRRD_TERNARY_OP_MAX+1][AIR_STRLEN_MED] = {
  "unknown ternary op",
  "clamp 2nd value to closed interval between 1st and 3rd",
  "linearly interpolate between 2nd value (1st = 0.0) and 3rd (1st = 1.0)",
  "if 1st value exists, the 2nd value, else the 3rd",
  "2nd value is inside OPEN interval range between 1st and 3rd",
  "2nd value is inside CLOSED interval range between 1st and 3rd"
};

airEnum
_nrrdTernaryOp_enum = {
  "ternary op",
  NRRD_TERNARY_OP_MAX,
  _nrrdTernaryOpStr, NULL,
  _nrrdTernaryOpDesc,
  NULL, NULL,
  AIR_FALSE
};
airEnum *
nrrdTernaryOp = &_nrrdTernaryOp_enum;

