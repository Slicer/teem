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
  "(unknown format)",
  "nrrd",
  "pnm",
  "table"
};

char
_nrrdEnumBoundaryStr[NRRD_BOUNDARY_MAX+1][NRRD_STRLEN_SMALL] = {
  "(unknown boundary)",
  "pad",
  "bleed",
  "wrap",
  "weight"
};

char
_nrrdEnumMagicStr[NRRD_MAGIC_MAX+1][NRRD_STRLEN_SMALL] = {
  "(unknown magic)",
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
  "(unknown type)",
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
  "(unknown encoding)",
  "raw",
  "ascii"
};

char
_nrrdEnumMeasureStr[NRRD_MEASURE_MAX+1][NRRD_STRLEN_SMALL] = {
  "(unknown measure)",
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
_nrrdEnumMinMaxStr[NRRD_MINMAX_MAX+1][NRRD_STRLEN_SMALL] = {
  "(unknown minmax)",
  "search",
  "search+set",
  "use",
  "instead-use"
};

char
_nrrdEnumCenterStr[NRRD_CENTER_MAX+1][NRRD_STRLEN_SMALL] = {
  "(unknown center)",
  "node",
  "cell"
};

char
_nrrdEnumAxesInfoStr[NRRD_AXESINFO_MAX+1][NRRD_STRLEN_SMALL] = {
  "(unknown axes info)",
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
_nrrdEnumEnumStr[NRRD_ENUM_MAX+1][NRRD_STRLEN_SMALL] = {
  "(unknown enum)",
  "format",
  "boundary",
  "magic",
  "type",
  "encoding",
  "measure",
  "minmax",
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
  NRRD_MINMAX_MAX,
  NRRD_CENTER_MAX,
  NRRD_AXESINFO_MAX,
  NRRD_ENDIAN_MAX,
  NRRD_FIELD_MAX
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
  _nrrdEnumMinMaxStr,
  _nrrdEnumCenterStr,
  _nrrdEnumAxesInfoStr,
  NULL,  /* _nrrdEnumEndianStr, */
  _nrrdEnumFieldStr
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

/*
** _nrrdEnumTypeStrToVal
**
** takes a given string and returns the integral type
*/
int
_nrrdEnumTypeStrToVal(char *str) {
  char type[][NRRD_STRLEN_SMALL]  = {
    "char", "signed char",
    "uchar", "unsigned char",
    "short", "short int", "signed short", "signed short int",
    "ushort", "unsigned short", "unsigned short int",
    "int", "signed int",
    "unsigned int",
    "long long", "long long int", "signed long long", "signed long long int",
    "unsigned long long", "unsigned long long int",
    "float",
    "double",
    /* "long double", */
    "block"};
  int value[] = {
    1, 1,
    2, 2,
    3, 3, 3, 3,
    4, 4, 4,
    5, 5,
    6,
    7, 7, 7, 7,
    8, 8,
    9,
    10,
    /* 11,  12, 0}; */
    11,
    0};

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

  enstr = (char (*)[NRRD_STRLEN_SMALL])(_nrrdEnumAllStr[whichEnum]);
  max = _nrrdEnumAllMax[whichEnum];
  switch (whichEnum) {
  case nrrdEnumFormat:
  case nrrdEnumBoundary:
  case nrrdEnumEncoding:
  case nrrdEnumMeasure:
  case nrrdEnumMinMax:
  case nrrdEnumCenter:
    airToLower(str);
  case nrrdEnumMagic:
  case nrrdEnumField:
    /* if the loop goes to completion, we return 0 */
    for (ret=max; ret>=0; ret--) {
      if (!strcmp(str, enstr[ret])) {
	break;
      }
    }
    break;
  case nrrdEnumType:
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

int 
nrrdTypeSize[NRRD_TYPE_MAX+1] = {
  -1, /* unknown */
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
  -1  /* effectively unknown; user has to set explicitly */
};

int 
nrrdEncodingEndianMatters[NRRD_ENCODING_MAX+1] = {
  0,
  1,
  0
};

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

int
_nrrdFieldValidInTable[NRRD_FIELD_MAX+1] = {
  0, /* nrrdField_unknown */
  1, /* nrrdField_comment */
  1, /* nrrdField_content */
  0, /* nrrdField_number */
  1, /* nrrdField_type */
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

