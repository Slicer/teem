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

airEnum
_nrrdBoundary = {
  "boundary behavior",
  NRRD_BOUNDARY_MAX,
  _nrrdBoundaryStr, NULL,
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

airEnum
_nrrdMagic = {
  "magic",
  NRRD_MAGIC_MAX+1,
  _nrrdMagicStr, NULL,
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
  "histo-variance"
};

airEnum
_nrrdMeasure = {
  "measure",
  NRRD_MEASURE_MAX,
  _nrrdMeasureStr, NULL,
  NULL, NULL,
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

airEnum
_nrrdCenter_enum = {
  "centering",
  NRRD_CENTER_MAX,
  _nrrdCenterStr, NULL,
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
  "label"
};

airEnum
_nrrdAxesInfo = {
  "axes_info",
  NRRD_AXESINFO_MAX,
  _nrrdAxesInfoStr, NULL,
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
#define nuSqt nrrdUnaryOpSqrt
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
  "sqrt",
  "ceil",
  "floor",
  "roundup",
  "rounddown",
  "abs",
  "sgn",
  "exists"
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
  "sqrt",
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
  nuSqt,
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
  "neq"
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
  nbNeq, nbNeq, nbNeq
};

airEnum
_nrrdBinaryOp_enum = {
  "binary op",
  NRRD_BINARY_OP_MAX,
  _nrrdBinaryOpStr, NULL,
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
  "inside",
  "between"
};

char
_nrrdTernaryOpStrEqv[][AIR_STRLEN_SMALL] = {
  "clamp",
  "lerp",
  "exists",
  "inside",
  "between", "tween", "btw",
  ""
};

int
_nrrdTernaryOpValEqv[] = {
  nrrdTernaryOpClamp,
  nrrdTernaryOpLerp,
  nrrdTernaryOpExists,
  nrrdTernaryOpInside,
  nrrdTernaryOpBetween, nrrdTernaryOpBetween, nrrdTernaryOpBetween
};

airEnum
_nrrdTernaryOp_enum = {
  "ternary op",
  NRRD_TERNARY_OP_MAX,
  _nrrdTernaryOpStr, NULL,
  _nrrdTernaryOpStrEqv, _nrrdTernaryOpValEqv,
  AIR_FALSE
};
airEnum *
nrrdTernaryOp = &_nrrdTernaryOp_enum;

