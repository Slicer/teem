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
nrrdMagic2Str[][NRRD_SMALL_STRLEN] = {
  "(unknown magic)",
  NRRD_HEADER,
  "P1",
  "P2",
  "P3",
  "P4",
  "P5",
  "P6"
};

char 
nrrdType2Str[][NRRD_SMALL_STRLEN] = {
  "(unknown type)",
  "char",
  "unsigned char",
  "short",
  "unsigned short",
  "int",
  "unsigned int",
  "long long",
  "unsigned long long",
  "float",
  "double",
  "long double",
  "block"
};

char
nrrdType2Conv[][NRRD_SMALL_STRLEN] = {
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
  "%Lf",
  "%*d"  /* what else? */
};

int 
nrrdTypeSize[] = {
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
  16, /* long double */
  -1  /* effectively unknown; user has to set explicitly */
};

char 
nrrdEncoding2Str[][NRRD_SMALL_STRLEN] = {
  "(unknown encoding)",
  "raw",
  "zlib",
  "ascii",
  "hex",
  "base85",
  "user"
  "",
};

char
nrrdEndian2Str[][NRRD_SMALL_STRLEN] = {
  "(unknown endian)",
  "little",
  "big"
};

int 
nrrdEncodingEndianMatters[] = {
  0,
  1,
  0,
  0,
  0,
  0,
  0
};

