/*
  Teem: Tools to process and visualize scientific data and images             .
  Copyright (C) 2013, 2012, 2011, 2010, 2009  University of Chicago
  Copyright (C) 2008, 2007, 2006, 2005  Gordon Kindlmann
  Copyright (C) 2004, 2003, 2002, 2001, 2000, 1999, 1998  University of Utah

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public License
  (LGPL) as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  The terms of redistributing and/or modifying this software also
  include exceptions to the LGPL that facilitate static linking.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with this library; if not, write to Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "nrrd.h"
#include "privateNrrd.h"

static int
_nrrdEncodingZRL_available(void) {

  return AIR_TRUE;
}

static int
_nrrdEncodingZRL_read(FILE *file, void *data, size_t elementNum,
                      Nrrd *nrrd, NrrdIoState *nio) {
  static const char me[]="_nrrdEncodingZRL_read";
  size_t ret, bsize;
  int car;
  char *data_c;
  size_t elementSize, maxChunkSize, remainderValue, chunkSize;
  size_t retTmp;
  char stmp[3][AIR_STRLEN_SMALL];

  AIR_UNUSED(nio);
  /* HEY: below is copy/paste from encodingGzip */
  bsize = nrrdElementSize(nrrd)*elementNum;
  ret = 0;
  data_c = (char *)data;
  elementSize = nrrdElementSize(nrrd);
  maxChunkSize = 1024 * 1024 * 1024 / elementSize;
  while(ret < elementNum) {
    remainderValue = elementNum-ret;
    if (remainderValue < maxChunkSize) {
      chunkSize = remainderValue;
    } else {
      chunkSize = maxChunkSize;
    }
    retTmp =
      fread(&(data_c[ret*elementSize]), elementSize, chunkSize, file);
    ret += retTmp;
    if (retTmp != chunkSize) {
      biffAddf(NRRD, "%s: fread got only %s %s-sized things, not %s "
               "(%g%% of expected)", me,
               airSprintSize_t(stmp[0], ret),
               airSprintSize_t(stmp[1], nrrdElementSize(nrrd)),
               airSprintSize_t(stmp[2], elementNum),
               100.0*AIR_CAST(double, ret)/AIR_CAST(double, elementNum));
      return 1;
    }
  }

  car = fgetc(file);
  if (EOF != car) {
    if (1 <= nrrdStateVerboseIO) {
      fprintf(stderr, "%s: WARNING: finished reading raw data, "
              "but file not at EOF\n", me);
    }
    ungetc(car, file);
  }

  return 0;
}

static int
_nrrdEncodingZRL_write(FILE *file, const void *data, size_t elementNum,
                       const Nrrd *nrrd, NrrdIoState *nio) {
  static const char me[]="_nrrdEncodingZRL_write";
  size_t ret, bsize;
  const char *data_c;
  size_t elementSize, maxChunkSize, remainderValue, chunkSize;
  size_t retTmp;
  char stmp[3][AIR_STRLEN_SMALL];

  AIR_UNUSED(nio);
  /* HEY: below is copy/paste from encodingGzip */

  bsize = nrrdElementSize(nrrd)*elementNum;

  /* HEY: There's a bug in fread/fwrite in gcc 4.2.1 (with SnowLeopard).
     When it reads/writes a >=2GB data array, it pretends to succeed
     (i.e. the return value is the right number) but it hasn't
     actually read/written the data.  The work-around is to loop
     over the data, reading/writing 1GB (or smaller) chunks.         */
  ret = 0;
  data_c = AIR_CAST(const char *, data);
  elementSize = nrrdElementSize(nrrd);
  maxChunkSize = 1024 * 1024 * 1024 / elementSize;
  while(ret < elementNum) {
    remainderValue = elementNum-ret;
    if (remainderValue < maxChunkSize) {
      chunkSize = remainderValue;
    } else {
      chunkSize = maxChunkSize;
    }
    retTmp =
      fwrite(&(data_c[ret*elementSize]), elementSize, chunkSize, file);
    ret += retTmp;
    if (retTmp != chunkSize) {
      biffAddf(NRRD, "%s: fwrite wrote only %s %s-sized things, not %s "
               "(%g%% of expected)", me,
               airSprintSize_t(stmp[0], ret),
               airSprintSize_t(stmp[1], nrrdElementSize(nrrd)),
               airSprintSize_t(stmp[2], elementNum),
               100.0*AIR_CAST(double, ret)/AIR_CAST(double, elementNum));
      return 1;
    }
  }

  fflush(file);
  return 0;
}

const NrrdEncoding
_nrrdEncodingZRL = {
  "zrl",      /* name */
  "zrl",      /* suffix */
  AIR_TRUE,   /* endianMatters */
  AIR_FALSE,   /* isCompression: HEY this is a hack: this IS certainly a
                  compression. However, with compressed encodings the nrrd
                  format has no way of specifying whether a byteskip
                  between be outside the encoding (in the uncompressed
                  data) vs inside the encoding (within the compuressed
                  data).  To date the convention has been that byte skip is
                  done *inside* compressions, but for the ZRL-encoded data
                  as currently generated, the relevant byte skipping is
                  certainly *outside* the compression.  Thus we claim
                  ignorance about how ZRL is a compression, so that byte
                  skipping can be used. */
  _nrrdEncodingZRL_available,
  _nrrdEncodingZRL_read,
  _nrrdEncodingZRL_write
};

const NrrdEncoding *const
nrrdEncodingZRL = &_nrrdEncodingZRL;
