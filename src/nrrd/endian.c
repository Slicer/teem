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

int
nrrdStr2Endian(char *str) {
  char *c, *tmp;
  int ret;

  ret = nrrdEndianUnknown;
  if (str) {
    if (tmp = strdup(str)) {
      c = tmp;
      while (*c) {
	*c = tolower(*c);
	c++;
      }
      if (!strncmp("big", tmp, 3)) {
	ret = nrrdEndianBig;
      }
      else if (!strncmp("little", tmp, 6)) {
	ret = nrrdEndianLittle;
      }
      free(tmp);
    }
  }  
  return ret;
}

int
nrrdMyEndian(void) {
  short word;
  char *byte;

  word = 0x0001;
  byte = (char *)&word;
  return(*byte ? nrrdEndianLittle : nrrdEndianBig);
}

void
_nrrdSwapShortEndian(void *_data, NRRD_BIG_INT N) {
  short *data, s, fix;
  NRRD_BIG_INT I;
  
  if (_data) {
    data = (short *)_data;
    for (I=0; I<=N-1; I++) {
      s = data[I];
      fix =  (s & 0x00FF);
      fix = ((s & 0xFF00) >> 0x08) | (fix << 0x08);
      data[I] = fix;
    }
  }
}

void
_nrrdSwapWordEndian(void *_data, NRRD_BIG_INT N) {
  int *data, w, fix;
  NRRD_BIG_INT I;

  if (_data) {
    data = (int *)_data;
    for (I=0; I<=N-1; I++) {
      w = data[I];
      fix =  (w & 0x000000FF);
      fix = ((w & 0x0000FF00) >> 0x08) | (fix << 0x08);
      fix = ((w & 0x00FF0000) >> 0x10) | (fix << 0x08);
      fix = ((w & 0xFF000000) >> 0x18) | (fix << 0x08);
      data[I] = fix;
    }
  }
}

void
_nrrdSwapLongWordEndian(void *_data, NRRD_BIG_INT N) {
  unsigned long long *data, l, fix;
  NRRD_BIG_INT I;

  if (_data) {
    data = (unsigned long long  *)_data;
    for (I=0; I<=N-1; I++) {
      l = data[I];
      fix =  (l & 0x00000000000000FF);
      fix = ((l & 0x000000000000FF00) >> 0x08) | (fix << 0x08);
      fix = ((l & 0x0000000000FF0000) >> 0x10) | (fix << 0x08);
      fix = ((l & 0x00000000FF000000) >> 0x18) | (fix << 0x08);
      fix = ((l & 0x000000FF00000000) >> 0x20) | (fix << 0x08);
      fix = ((l & 0x0000FF0000000000) >> 0x28) | (fix << 0x08);
      fix = ((l & 0x00FF000000000000) >> 0x30) | (fix << 0x08);
      fix = ((l & 0xFF00000000000000) >> 0x38) | (fix << 0x08);
      data[I] = fix;
    }
  }
}

void
_nrrdNoopEndian(void *_data, NRRD_BIG_INT N) {
  
}

void
(*_nrrdEndianSwapper[])(void *, NRRD_BIG_INT) = {
  _nrrdNoopEndian,         /*  0: nobody knows! */
  _nrrdNoopEndian,         /*  1:   signed 1-byte integer */
  _nrrdNoopEndian,         /*  2: unsigned 1-byte integer */
  _nrrdSwapShortEndian,    /*  3:   signed 2-byte integer */
  _nrrdSwapShortEndian,    /*  4: unsigned 2-byte integer */
  _nrrdSwapWordEndian,     /*  5:   signed 4-byte integer */
  _nrrdSwapWordEndian,     /*  6: unsigned 4-byte integer */
  _nrrdSwapLongWordEndian, /*  7:   signed 8-byte integer */
  _nrrdSwapLongWordEndian, /*  8: unsigned 8-byte integer */
  _nrrdSwapWordEndian,     /*  9:          4-byte floating point */
  _nrrdSwapLongWordEndian, /* 10:          8-byte floating point */
  _nrrdNoopEndian,         /* HEY! PUNT: 11:        16-byte floating point */
  _nrrdNoopEndian          /* HEY! PUNT: 12: size user defined at run time */
};

void
nrrdSwapEndian(Nrrd *nrrd) {
  void (*swapper)(void *, NRRD_BIG_INT);
  
  if (nrrd 
      && nrrd->data 
      && nrrdTypeUnknown < nrrd->type 
      && nrrd->type < nrrdTypeLast) {
    swapper = _nrrdEndianSwapper[nrrd->type];
    swapper(nrrd->data, nrrd->num);
  }
}



