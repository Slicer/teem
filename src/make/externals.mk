#
# Teem: Tools to process and visualize scientific data and images
# Copyright (C) 2009--2025  University of Chicago
# Copyright (C) 2005--2008  Gordon Kindlmann
# Copyright (C) 1998--2004  University of Utah
#
# This library is free software; you can redistribute it and/or modify it under the terms
# of the GNU Lesser General Public License (LGPL) as published by the Free Software
# Foundation; either version 2.1 of the License, or (at your option) any later version.
# The terms of redistributing and/or modifying this software also include exceptions to
# the LGPL that facilitate static linking.
#
# This library is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
# PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
# You should have received a copy of the GNU Lesser General Public License
# along with this library; if not, see <https://www.gnu.org/licenses/>.
#

## AllExterns: list of all the identifiers for the various external libraries that we can
## _optionally_ link against.  Teem has no notion of depending on these in the makefile
## sense.  Teem doesn't try to represent inter-external dependencies (e.g. PNG on ZLIB)
## explicitly, but the ordering of the xterns below HAS TO reflect the ordering on the
## link line (so PNG preceeds ZLIB)
##
## External EXT is enabled during make by setting the shell variable TEEM_EXT (just set
## it, to anything EXCEPT "0"), either by "export TEEM_EXT; ...; make" or by
## "TEEM_EXT= make" or "TEEM_EXT=1 make"
## (but not: "TEEM_EXT=0 make", which leaves EXT disabled).
## If external EXT is enabled during make, then *preprocessor symbol* TEEM_EXT will be
## effectively #define'd as "1" during source file compilation.
##
## TeemV2 renamed:
## TEEM_EXT_IPATH --> TEEM_EXT_DASHI
## TEEM_EXT_LPATH --> TEEM_EXT_DASHL
## and these variables can now be set here (since there are no longer multiple
## architectures to simultaneously support). Being perfectly consistent with the
## of the V2 reworking of GNUmake stuff would have used EXT.dashI and EXT.dashL
## but the shell sometimes gets confused by variables with a "." inside
##
AllExterns = PNG ZLIB BZIP2 PTHREAD LEVMAR FFTW3

## PNG: for PNG images, from https://www.libpng.org/pub/png/libpng.html
## Using PNG enables the "png" nrrd format.
## Header file is <png.h>
PNG.llink = -lpng
nrrd.Externs += PNG
TEEM_PNG_DASHI ?=
TEEM_PNG_DASHL ?=

## ZLIB: for the zlib library (in gzip and PNG image format) from https://zlib.net/
## Using zlib enables the "gzip" nrrd data encoding
## Header file is <zlib.h>.
ZLIB.llink = -lz
nrrd.Externs += ZLIB
TEEM_ZLIB_DASHI ?=
TEEM_ZLIB_DASHL ?=

## BZIP2: for the bzip2 compression library, from https://sourceware.org/bzip2/
## Using bzip2 enables the "bzip2" nrrd data encoding.
## Header file is <bzlib.h>.
BZIP2.llink = -lbz2
nrrd.Externs += BZIP2
TEEM_BZIP2_DASHI ?=
TEEM_BZIP2_DASHL ?=

## PTHREAD: use pthread-based multi-threading in airThreads.  Note that Windows has its
## own multithreading capabilities, which is used in airThread if !TEEM_PTHREAD, and we
## are on Windows.
## Header file is <pthread.h>
PTHREAD.llink = -lpthread
air.Externs += PTHREAD
TEEM_PTHREAD_DASHI ?=
TEEM_PTHREAD_DASHL ?=

## LEVMAR: Levenberg-Marquardt from https://users.ics.forth.gr/~lourakis/levmar/
## Header file is <levmar.h>
LEVMAR.llink = -llevmar
ten.Externs += LEVMAR
elf.Externs += LEVMAR
TEEM_LEVMAR_DASHI ?=
TEEM_LEVMAR_DASHL ?=

## FFTW3: FFTW version 3 from https://www.fftw.org/
## Header file is <fftw3.h>
FFTW3.llink = -lfftw3
nrrd.Externs += FFTW3
TEEM_FFTW3_DASHI ?=
TEEM_FFTW3_DASHL ?=
