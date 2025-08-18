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

## XTERNS: list of all the identifiers for the various external libraries that we can
## _optionally_ link against.  Teem has no notion of depending on these in the makefile
## sense.  Teem doesn't try to represent inter-external dependencies (e.g. PNG on zlib)
## explicitly, but the ordering of the xterns below has to reflect the ordering on the
## link line (e.g. PNG preceeds ZLIB)
##
## Extern EXT is enabled during make by setting the environment variable TEEM_EXT (just
## set it, not to anything in particular).  If external EXT is enabled during make, then
## TEEM_EXT will be defined as "1" during source file compilation.
##
## For Teem v2, the TEEM_EXT_IPATH and TEEM_EXT_LPATH (e.g TEEM_ZLIB_IPATH and
## TEEM_ZLIB_LPATH) variables are changed to EXT.IPATH and EXT.LPATH, set here (since
## there are no longer multiple architectures to simultaneously support)
##
XTERNS = PNG ZLIB BZIP2 PTHREAD LEVMAR FFTW3

## ZLIB: for the zlib library (in gzip and PNG image format) from https://zlib.net/
## Using zlib enables the "gzip" nrrd data encoding
## Header file is <zlib.h>.
ZLIB.LINK = -lz
ZLIB.IPATH =
ZLIB.LPATH =
nrrd.XTERN += ZLIB

## BZIP2: for the bzip2 compression library, from https://sourceware.org/bzip2/
## Using bzip2 enables the "bzip2" nrrd data encoding.
## Header file is <bzlib.h>.
BZIP2.LINK = -lbz2
BZIP2.IPATH =
BZIP2.LPATH =
nrrd.XTERN += BZIP2

## PNG: for PNG images, from https://www.libpng.org/pub/png/libpng.html
## Using PNG enables the "png" nrrd format.
## Header file is <png.h>
PNG.LINK = -lpng
PNG.IPATH =
PNG.LPATH =
nrrd.XTERN += PNG

## PTHREAD: use pthread-based multi-threading in airThreads.  Note that Windows has its
## own multithreading capabilities, which is used in airThread if !TEEM_PTHREAD, and we
## are on windows.
## Header file is <pthread.h>
PTHREAD.LINK = -lpthread
PTHREAD.IPATH =
PTHREAD.LPATH =
air.XTERN += PTHREAD

## LEVMAR: Levenberg-Marquardt from https://users.ics.forth.gr/~lourakis/levmar/
## Header file is <levmar.h>
LEVMAR.LINK = -llevmar
LEVMAR.IPATH =
LEVMAR.LPATH =
ten.XTERN += LEVMAR
elf.XTERN += LEVMAR

## FFTW3: FFTW version 3 from https://www.fftw.org/
## Header file is <fftw3.h>
FFTW3.LINK = -lfftw3
FFTW3.IPATH =
FFTW3.LPATH =
nrrd.XTERN += FFTW3
