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

/* ------------------------ nrrdFormat ------------------------- */

char
_nrrdFormatStr[NRRD_FORMAT_MAX+1][AIR_STRLEN_SMALL] = {
  "(unknown_format)",
  "nrrd",
  "pnm",
  "table"
};

char
_nrrdFormatStrEqv[][AIR_STRLEN_SMALL] = {
  "nrrd",
  "pnm",
  "table", "text", "txt",
  ""
};

int
_nrrdFormatValEqv[] = {
  nrrdFormatNRRD,
  nrrdFormatPNM,
  nrrdFormatTable, nrrdFormatTable, nrrdFormatTable
};

airEnum
nrrdFormat = {
  "format",
  NRRD_FORMAT_MAX,
  _nrrdFormatStr,  NULL,
  _nrrdFormatStrEqv, _nrrdFormatValEqv,
  AIR_FALSE
};

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
nrrdBoundary = {
  "boundary behavior",
  NRRD_BOUNDARY_MAX,
  _nrrdBoundaryStr, NULL,
  NULL, NULL,
  AIR_FALSE
};

/* ------------------------ nrrdMagic ------------------------- */

char
_nrrdMagicStr[NRRD_MAGIC_MAX+1][AIR_STRLEN_SMALL] = {
  "(unknown_magic)",
  "NRRD00.01",
  "NRRD00.01", /* "NRRD0001", */
  "P2",
  "P3",
  "P5",
  "P6"
};

airEnum
nrrdMagic = {
  "magic",
  NRRD_MAGIC_MAX+1,
  _nrrdMagicStr, NULL,
  NULL, NULL,
  AIR_TRUE
};

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
  "long long", "long long int", "signed long long",
               "signed long long int", "int64", "int64_t", 
  "unsigned long long", "unsigned long long int", "uint64", "uint64_t", 
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
  ntLL, ntLL, ntLL, ntLL, ntLL, ntLL, 
  ntUL, ntUL, ntUL, ntUL, 
  ntFL,
  ntDB,
  ntBL
};

airEnum
nrrdType = {
  "type",
  NRRD_TYPE_MAX,
  _nrrdTypeStr, NULL,
  _nrrdTypeStrEqv, _nrrdTypeValEqv,
  AIR_TRUE
};

/* ------------------------ nrrdEncoding ------------------------- */

char
_nrrdEncodingStr[NRRD_ENCODING_MAX+1][AIR_STRLEN_SMALL] = {
  "(unknown_encoding)",
  "raw",
  "ascii"
};

airEnum
nrrdEncoding = {
  "encoding",
  NRRD_ENCODING_MAX,
  _nrrdEncodingStr, NULL,
  NULL, NULL,
  AIR_FALSE
};

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
nrrdMeasure = {
  "measure",
  NRRD_MEASURE_MAX,
  _nrrdMeasureStr, NULL,
  NULL, NULL,
  AIR_FALSE
};

/* ------------------------ nrrdCenter ------------------------- */

char
_nrrdCenterStr[NRRD_CENTER_MAX+1][AIR_STRLEN_SMALL] = {
  "(unknown_center)",
  "node",
  "cell"
};

airEnum
nrrdCenter = {
  "centering",
  NRRD_CENTER_MAX,
  _nrrdCenterStr, NULL,
  NULL, NULL,
  AIR_FALSE
};

/* ------------------------ nrrdAxisInfo ------------------------- */

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
nrrdAxesInfo = {
  "axes_info",
  NRRD_AXESINFO_MAX,
  _nrrdAxesInfoStr, NULL,
  NULL, NULL,
  AIR_TRUE
};
  
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

#define nfComment  nrrdField_comment
#define nfContent  nrrdField_content
#define nfNumber   nrrdField_number
#define nfType     nrrdField_type
#define nfBsize    nrrdField_block_size
#define nfDim      nrrdField_dimension
#define nfSizes    nrrdField_sizes
#define nfSpacings nrrdField_spacings
#define nfAMins    nrrdField_axis_mins
#define nfAMaxs    nrrdField_axis_maxs
#define nfCenters  nrrdField_centers
#define nfLabels   nrrdField_labels
#define nfMin      nrrdField_min
#define nfMax      nrrdField_max
#define nfOMin     nrrdField_old_min
#define nfOMax     nrrdField_old_max
#define nfDataFile nrrdField_data_file
#define nfEndian   nrrdField_endian
#define nfEncoding nrrdField_encoding
#define nfLineSkip nrrdField_line_skip
#define nfByteSkip nrrdField_byte_skip

char
_nrrdFieldStrEqv[][AIR_STRLEN_SMALL]  = {
  "#",
  "content",
  "number",
  "type",
  "block size", "blocksize",
  "dimension", "dim",
  "sizes",
  "spacings",
  "axis mins", "axismins",
  "axis maxs", "axismaxs",
  "centers",
  "labels",
  "min",
  "max",
  "old min", "oldmin",
  "old max", "oldmax",
  "data file", "datafile",
  "endian", "endianness",
  "encoding",
  "line skip", "lineskip",
  "byte skip", "byteskip",
  ""
};

int
_nrrdFieldValEqv[] = {
  nfComment,
  nfContent,
  nfNumber,
  nfType,
  nfBsize, nfBsize,
  nfDim, nfDim,
  nfSizes,
  nfSpacings,
  nfAMins, nfAMins,
  nfAMaxs, nfAMaxs,
  nfCenters,
  nfLabels,
  nfMin,
  nfMax,
  nfOMin,
  nfOMax,
  nfDataFile, nfDataFile,
  nfEndian, nfEndian,
  nfEncoding, 
  nfLineSkip, nfLineSkip,
  nfByteSkip, nfByteSkip,
};

airEnum
nrrdField = {
  "nrrd_field",
  NRRD_FIELD_MAX,
  _nrrdFieldStr, NULL,
  _nrrdFieldStrEqv, _nrrdFieldValEqv, 
  AIR_TRUE
};

/* ------------------------ nrrdNonExist ------------------------- */

/*
char
_nrrdNonExistStr[NRRD_NON_EXIST_MAX+1][AIR_STRLEN_SMALL] = {
  "(unknown)",
  "true",
  "false"
};
*/

