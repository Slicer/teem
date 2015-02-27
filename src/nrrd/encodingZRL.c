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

  AIR_UNUSED(nio);
  unsigned char *output_buffer = (unsigned char *) data;
  size_t toread = elementNum*nrrdElementSize(nrrd);
  printf("!%s: looking for %u values (%u bytes) of type %s\n", me,
         (unsigned int)elementNum, (unsigned int)toread, airEnumStr(nrrdType, nrrd->type));
  int cc, dd;
  unsigned int j = 0;
  while (j < toread) {
    if ((cc = fgetc(file)) == 0) {
      if ((dd = fgetc(file)) == 0) {
        j += dd + fgetc(file)*256;
      } else {
        j += (unsigned char)dd;
      }
    } else {
      output_buffer[j] = (unsigned char)cc;
      j++;
    }
  }

  return 0;
}

static int
_nrrdEncodingZRL_write(FILE *file, const void *data, size_t elementNum,
                       const Nrrd *nrrd, NrrdIoState *nio) {
  static const char me[]="_nrrdEncodingZRL_write";

  AIR_UNUSED(file);
  AIR_UNUSED(data);
  AIR_UNUSED(elementNum);
  AIR_UNUSED(nrrd);
  AIR_UNUSED(nio);
  biffAddf(NRRD, "%s: sorry, currently a read-only encoding", me);

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
