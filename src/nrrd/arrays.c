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


#include "nrrd.h"

/* for all the arrays that everyone needs.  See nrrd.h for documentation */

char
_nrrdEnumFormatStr[NRRD_FORMAT_MAX+1][NRRD_STRLEN_SMALL] = {
  "(unknown_format)",
  "nrrd",
  "pnm",
  "table"
};

char
_nrrdEnumBoundaryStr[NRRD_BOUNDARY_MAX+1][NRRD_STRLEN_SMALL] = {
  "(unknown_boundary)",
  "pad",
  "bleed",
  "wrap",
  "weight"
};

char
_nrrdEnumMagicStr[NRRD_MAGIC_MAX+1][NRRD_STRLEN_SMALL] = {
  "(unknown_magic)",
  "NRRD00.01",
  /* "NRRD0001", */
  "NRRD00.01",
  "P2",
  "P3",
  "P5",
  "P6"
};

char 
_nrrdEnumTypeStr[NRRD_TYPE_MAX+1][NRRD_STRLEN_SMALL] = {
  "(unknown_type)",
  "signed char",
  "unsigned char",
  "short",
  "unsigned short",
  "int",
  "unsigned int",
  "long long",
  "unsigned long long",
  "float",
  "double",
  /* "long double", */
  "block"
};

char
_nrrdEnumEncodingStr[NRRD_ENCODING_MAX+1][NRRD_STRLEN_SMALL] = {
  "(unknown_encoding)",
  "raw",
  "ascii"
};

char
_nrrdEnumMeasureStr[NRRD_MEASURE_MAX+1][NRRD_STRLEN_SMALL] = {
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
  "histo-min",
  "histo-max",
  "histo-mean",
  "histo-median",
  "histo-mode",
  "histo-product",
  "histo-sum",
  "histo-variance"
};

char
_nrrdEnumCenterStr[NRRD_CENTER_MAX+1][NRRD_STRLEN_SMALL] = {
  "(unknown_center)",
  "node",
  "cell"
};

char
_nrrdEnumAxesInfoStr[NRRD_AXESINFO_MAX+1][NRRD_STRLEN_SMALL] = {
  "(unknown_axes_info)",
  "size",
  "spacing",
  "min",
  "max",
  "center",
  "label"
};

/*
** there is no 
** "char _nrrdEnumEndianStr[NRRD_ENDIAN_MAX+1][NRRD_STRLEN_SMALL]"
** because this is taken care of in air
*/

char
_nrrdEnumFieldStr[NRRD_FIELD_MAX+1][NRRD_STRLEN_SMALL] = {
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

char
_nrrdEnumNonExistStr[NRRD_NON_EXIST_MAX+1][NRRD_STRLEN_SMALL] = {
  "(unknown)",
  "true",
  "false"
};


char
_nrrdEnumEnumStr[NRRD_ENUM_MAX+1][NRRD_STRLEN_SMALL] = {
  "(unknown_enum)",
  "format",
  "boundary",
  "magic",
  "type",
  "encoding",
  "measure",
  "center",
  "axes info",
  "endian",
  "field"
};

int
_nrrdEnumAllMax[NRRD_ENUM_MAX+1] = {
  -1,
  NRRD_FORMAT_MAX,
  NRRD_BOUNDARY_MAX,
  NRRD_MAGIC_MAX,
  NRRD_TYPE_MAX,
  NRRD_ENCODING_MAX,
  NRRD_MEASURE_MAX,
  NRRD_CENTER_MAX,
  NRRD_AXESINFO_MAX,
  NRRD_ENDIAN_MAX,
  NRRD_FIELD_MAX,
  NRRD_NON_EXIST_MAX
};

void *
_nrrdEnumAllStr[NRRD_ENUM_MAX+1] = {
  NULL,
  _nrrdEnumFormatStr,
  _nrrdEnumBoundaryStr,
  _nrrdEnumMagicStr,
  _nrrdEnumTypeStr,
  _nrrdEnumEncodingStr,
  _nrrdEnumMeasureStr,
  _nrrdEnumCenterStr,
  _nrrdEnumAxesInfoStr,
  NULL,  /* _nrrdEnumEndianStr, alors, il n'existe pas */
  _nrrdEnumFieldStr,
  _nrrdEnumNonExistStr
};

char *
nrrdEnumValToStr(int whichEnum, int val) {
  char (*enstr)[NRRD_STRLEN_SMALL];

  if (nrrdEnumEndian == whichEnum) {
    return airEndianToStr(val);
  }
  
  if (!AIR_BETWEEN(nrrdEnumUnknown, whichEnum, nrrdEnumLast))
    return _nrrdEnumEnumStr[nrrdEnumUnknown];

  enstr = (char (*)[NRRD_STRLEN_SMALL])(_nrrdEnumAllStr[whichEnum]);

  if (!AIR_BETWEEN(0, val, _nrrdEnumAllMax[whichEnum]+1))
    return enstr[0];

  return enstr[val];
}

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

/*
** _nrrdEnumFieldStrToVal
**
** takes a given string and returns the integral enum value for the 
** field in a nrrd header
**
** note that as called by nrrdEnumStrToVal(), the given string has
** already been sent through airToLower()
*/
int
_nrrdEnumFieldStrToVal(char *str) {
  char field[][NRRD_STRLEN_SMALL]  = {
    "Ernesto \"Che\" Guevara",
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
    "endian",
    "encoding",
    "lineskip", "lineskip",
    "byteskip", "byteskip"};
  int value[] = {
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
    nfEndian,
    nfEncoding, 
    nfLineSkip, nfLineSkip,
    nfByteSkip, nfByteSkip,
    0};  /* a sentinel for for-loop below */

  int i;

  for (i=0; value[i]; i++) {
    if (!strcmp(field[i], str)) {
      return value[i];
    }
  }
  return nrrdField_unknown;
}

#define ntC   nrrdTypeChar
#define ntUC  nrrdTypeUChar
#define ntS   nrrdTypeShort
#define ntUS  nrrdTypeUShort
#define ntI   nrrdTypeInt
#define ntUI  nrrdTypeUInt
#define ntLL  nrrdTypeLLong
#define ntULL nrrdTypeULLong
#define ntF   nrrdTypeFloat
#define ntD   nrrdTypeDouble
#define ntB   nrrdTypeBlock

/*
** _nrrdEnumTypeStrToVal
**
** takes a given string and returns the integral type
**
** note that as called by nrrdEnumStrToVal(), the given string has
** already been sent through airToLower()
*/
int
_nrrdEnumTypeStrToVal(char *str) {
  char type[][NRRD_STRLEN_SMALL]  = {
    "char", "signed char", "int8", "int8_t",
    "uchar", "unsigned char", "uint8", "uint8_t", 
    "short", "short int", "signed short", "signed short int", "int16", "int16_t",
    "ushort", "unsigned short", "unsigned short int", "uint16", "uint16_t", 
    "int", "signed int", "int32", "int32_t", 
    "uint", "unsigned int", "uint32", "uint32_t",
    "long long", "long long int", "signed long long", "signed long long int", "int64", "int64_t", 
    "unsigned long long", "unsigned long long int", "uint64", "uint64_t", 
    "float",
    "double",
    /* "long double", */
    "block"};
  int value[] = {
    ntC, ntC, ntC, ntC, 
    ntUC, ntUC, ntUC, ntUC,
    ntS, ntS, ntS, ntS, ntS, ntS, 
    ntUS, ntUS, ntUS, ntUS, ntUS,
    ntI, ntI, ntI, ntI, 
    ntUI, ntUI, ntUI, ntUI, 
    ntLL, ntLL, ntLL, ntLL, ntLL, ntLL, 
    ntULL, ntULL, ntULL, ntULL, 
    ntF,
    ntD,
    ntB,
    0};  /* a sentinel for for-loop below */

  int i;

  for (i=0; value[i]; i++) {
    if (!strcmp(type[i], str)) {
      return value[i];
    }
  }
  return nrrdTypeUnknown;
}

int
nrrdEnumStrToVal(int whichEnum, char *_str) {
  char me[]="nrrdEnumStrToVal", (*enstr)[NRRD_STRLEN_SMALL], *str;
  int ret, max;
  
  if (nrrdEnumEndian == whichEnum) {
    return airStrToEndian(_str);
  }
  
  if (!(_str && AIR_BETWEEN(nrrdEnumUnknown, whichEnum, nrrdEnumLast)))
    return 0;

  str = airStrdup(_str);
  if (!str)
    return 0;

  enstr = (char (*)[NRRD_STRLEN_SMALL])(_nrrdEnumAllStr[whichEnum]);
  max = _nrrdEnumAllMax[whichEnum];
  switch (whichEnum) {
    /* first, all the case-insensitive enums */
  case nrrdEnumFormat:
  case nrrdEnumBoundary:
  case nrrdEnumEncoding:
  case nrrdEnumMeasure:
  case nrrdEnumCenter:
    airToLower(str);

    /* then, all the case-sensitive enums */
  case nrrdEnumMagic:
    /* if the loop goes to completion, we return 0 */
    for (ret=max; ret>=0; ret--) {
      if (!strcmp(str, enstr[ret])) {
	break;
      }
    }
    ret = AIR_MAX(0, ret);
    break;

    /* then, the special-case enums (both case sensitive and not) */
  case nrrdEnumField:
    airToLower(str);
    ret = _nrrdEnumFieldStrToVal(str);
    break;
  case nrrdEnumType:
    airToLower(str);
    ret = _nrrdEnumTypeStrToVal(str);
    break;
  default:
    fprintf(stderr, "%s: WARNING: whichEnum %d not handled\n", me, whichEnum);
    ret = 0;
    break;
  }

  free(str);
  return ret;
}

char
nrrdTypeConv[NRRD_TYPE_MAX+1][NRRD_STRLEN_SMALL] = {
  "%*d",  /* what else? sscanf: skip, printf: use "minimum precision" */
  "%d",
  "%u",
  "%hd",
  "%hu",
  "%d",
  "%u",
  "%lld",
  "%llu",
  "%f",
  "%lf",
  /* "%Lf", */
  "%*d"  /* what else? */
};

/*
** the setting of NRRD_BIGGEST_TYPE has to be in accordance with this
*/
int 
nrrdTypeSize[NRRD_TYPE_MAX+1] = {
  0, /* unknown */
  1,  /* char */
  1,  /* unsigned char */
  2,  /* short */
  2,  /* unsigned short */
  4,  /* int */
  4,  /* unsigned int */
  8,  /* long long */
  8,  /* unsigned long long */
  4,  /* float */
  8,  /* double */
  /* 16,  long double */
  0  /* effectively unknown; user has to set explicitly */
};


/*
** _nrrdFieldValidInPNM[]
**
** these fields are valid embedded in PNM comments
** This does NOT include the fields who's values are constrained
** the PNM format/magic itself.
*/
int
_nrrdFieldValidInPNM[NRRD_FIELD_MAX+1] = {
  0, /* nrrdField_unknown */
  1, /* nrrdField_comment */
  1, /* nrrdField_content */
  0, /* nrrdField_number */
  0, /* nrrdField_type */
  0, /* nrrdField_block_size */
  0, /* nrrdField_dimension */
  0, /* nrrdField_sizes */
  1, /* nrrdField_spacings */
  1, /* nrrdField_axis_mins */
  1, /* nrrdField_axis_maxs */
  1, /* nrrdField_centers */
  1, /* nrrdField_labels */
  1, /* nrrdField_min */
  1, /* nrrdField_max */
  1, /* nrrdField_old_min */
  1, /* nrrdField_old_max */
  0, /* nrrdField_data_file */
  0, /* nrrdField_endian */
  0, /* nrrdField_encoding */
  0, /* nrrdField_line_skip */
  0  /* nrrdField_byte_skip */
};

/*
** _nrrdFieldValidInTable[]
** 
** these fields are valid embedded in table comments
** This does NOT include the fields who's values are constrained
** the table format itself.
*/
int
_nrrdFieldValidInTable[NRRD_FIELD_MAX+1] = {
  0, /* nrrdField_unknown */
  1, /* nrrdField_comment */
  1, /* nrrdField_content */
  0, /* nrrdField_number */
  0, /* nrrdField_type: decided AGAINST table holding general type */
  0, /* nrrdField_block_size */
  0, /* nrrdField_dimension */
  0, /* nrrdField_sizes */
  1, /* nrrdField_spacings */
  1, /* nrrdField_axis_mins */
  1, /* nrrdField_axis_maxs */
  1, /* nrrdField_centers */
  1, /* nrrdField_labels */
  1, /* nrrdField_min */
  1, /* nrrdField_max */
  1, /* nrrdField_old_min */
  1, /* nrrdField_old_max */
  0, /* nrrdField_data_file */
  0, /* nrrdField_endian */
  0, /* nrrdField_encoding */
  0, /* nrrdField_line_skip */
  0  /* nrrdField_byte_skip */
};

/*
** _nrrdFieldRequired[]
**
** regardless of whether its a nrrd, PNM, or table, these things
** need to be conveyed, either explicity or implicitly
*/
int
_nrrdFieldRequired[NRRD_FIELD_MAX+1] = {
  0, /* "Ernesto \"Che\" Guevara" */
  0, /* "#" */
  0, /* "content" */
  0, /* "number" */
  1, /* "type" */
  0, /* "block size" */
  1, /* "dimension" */
  1, /* "sizes" */
  0, /* "spacings" */
  0, /* "axis mins" */
  0, /* "axis maxs" */
  0, /* "centers" */
  0, /* "labels" */
  0, /* "min" */
  0, /* "max" */
  0, /* "old min" */
  0, /* "old max" */
  0, /* "data file" */
  0, /* "endian" */
  1, /* "encoding" */
  0, /* "line skip" */
  0  /* "byte skip" */
};

/*
******** nrrdEncodingEndianMatters[]
** 
** tells if given encoding exposes endianness of architecture
*/
int 
nrrdEncodingEndianMatters[NRRD_ENCODING_MAX+1] = {
  0,   /* unknown */
  1,   /* raw */
  0    /* ascii */
};

/*
** _nrrdFormatUsesDIO[]
**
** whether or not try using direct I/O for a given format
*/
int
_nrrdFormatUsesDIO[NRRD_FORMAT_MAX+1] = {
  0,   /* nrrdFormatUnknown */
  1,   /* nrrdFormatNRRD */
  0,   /* nrrdFormatPNM */
  0    /* nrrdFormatTable */
};
