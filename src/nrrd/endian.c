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

void
_nrrdSwap16Endian(void *_data, nrrdBigInt N) {
  short *data, s, fix;
  nrrdBigInt I;
  
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
_nrrdSwap32Endian(void *_data, nrrdBigInt N) {
  int *data, w, fix;
  nrrdBigInt I;

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
_nrrdSwap64Endian(void *_data, nrrdBigInt N) {
  unsigned long long *data, l, fix;
  nrrdBigInt I;

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
_nrrdNoopEndian(void *_data, nrrdBigInt N) {
  
}

void
_nrrdBlockEndian(void *_data, nrrdBigInt N) {
  char me[]="_nrrdBlockEndian";
  
  fprintf(stderr, "%s: WARNING: can't fix endiannes of nrrd type %s\n", me,
	  nrrdEnumValToStr(nrrdEnumType, nrrdTypeBlock));
}

void
(*_nrrdSwapEndian[])(void *, nrrdBigInt) = {
  _nrrdNoopEndian,         /*  0: nobody knows! */
  _nrrdNoopEndian,         /*  1:   signed 1-byte integer */
  _nrrdNoopEndian,         /*  2: unsigned 1-byte integer */
  _nrrdSwap16Endian,       /*  3:   signed 2-byte integer */
  _nrrdSwap16Endian,       /*  4: unsigned 2-byte integer */
  _nrrdSwap32Endian,       /*  5:   signed 4-byte integer */
  _nrrdSwap32Endian,       /*  6: unsigned 4-byte integer */
  _nrrdSwap64Endian,       /*  7:   signed 8-byte integer */
  _nrrdSwap64Endian,       /*  8: unsigned 8-byte integer */
  _nrrdSwap32Endian,       /*  9:          4-byte floating point */
  _nrrdSwap64Endian,       /* 10:          8-byte floating point */
  /* _nrrdNoopEndian,    HEY! PUNT: 11:        16-byte floating point */
  _nrrdBlockEndian         /* 11: size user defined at run time */
};

void
nrrdSwapEndian(Nrrd *nrrd) {
  
  if (nrrd 
      && nrrd->data 
      && AIR_BETWEEN(nrrdTypeUnknown, nrrd->type, nrrdTypeLast)) {
    _nrrdSwapEndian[nrrd->type](nrrd->data, nrrdElementNumber(nrrd));
  }
  return;
}



