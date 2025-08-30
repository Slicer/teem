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

AllExterns := PNG ZLIB BZIP2 PTHREAD LEVMAR FFTW3

## In Teem, an "external" is a non-Teem library (external) libraries that Teem can
## _optionally_ link against.  Teem has no notion of depending on these in the makefile
## sense.  Teem doesn't try to represent inter-external dependencies (e.g. PNG on ZLIB)
## explicitly, but the ordering of the externals in AllExterns above HAS TO reflect the
## ordering on the link line (so PNG preceeds ZLIB)
##
## There is no configure-time setting of what externals to use (as in CMake).  External
## EXT is enabled during *make* by setting the shell variable TEEM_EXT (just set it, to
## anything EXCEPT "0"), either by:
##   export TEEM_EXT; ...; make
## or:
##   TEEM_EXT= make
## or:
##   TEEM_EXT=1 make
## (but not: "TEEM_EXT=0 make", which leaves EXT disabled).
## If external EXT is enabled during make, then *preprocessor symbol* TEEM_EXT will be
## effectively #define'd as "1" during source file compilation.
##
## NOTE: make can only remember what was built when, not how it was built, so if you are
## using externals, and your build process is split over multiple invocations of "make",
## then the same externals have to be used each time (e.g. making "unu" will need to
## -llib link again the external libraries, so that "make" has to know about the same
## externals used by the compiled code in libnrrd.a
##
## Here is one example how one might compile with PNG and ZLIB:
##   TEEM_PNG= \
##   TEEM_PNG_IPATH=/opt/homebrew/Cellar/libpng/1.6.50/include \
##   TEEM_PNG_LPATH=/opt/homebrew/Cellar/libpng/1.6.50/lib \
##   TEEM_ZLIB= \
##   TEEM_ZLIB_IPATH=/opt/homebrew/opt/zlib/include \
##   TEEM_ZLIB_LPATH=/opt/homebrew/opt/zlib/lib \
##   make
##
## TeemV2 renamed moved the variables describing each external to here (since there are
## no longer multiple architectures to simultaneously support). For each EXT:
## TEEM_EXT_IPATH, TEEM_EXT_LPATH: You very likely have to set these!
##   These variables have their same name from early Teem,
##   BUT NOTE: Now their names are accurate!
##   You no longer start TEEM_EXT_IPATH with "-I" or TEEM_EXT_LPATH with "-L".
##   They are really just the path to where the header or library file is; it is the job
##   of the functions Externs.dashD and Externs.dashI (defined in ../GNUmakefile and
##   used in template.mk) to add the -I and -L to each path as needed
## TEEM_EXT_LNAME: the lib in "-llib" option to link with the library. Usually doesn't
##   have to change, but you may have to be change if the library depends on other libraries
##   e.g. TEEM_LEVMAR_LNAME ?= levmar lapack blas
## lib.Externs: NOT TO BE MESSED WITH: how we declare to the build system which Teem
##   library lib needs to know about the extern for the sake of its compilation flags

## PNG: for PNG images, from https://www.libpng.org/pub/png/libpng.html
## Using PNG enables the "png" nrrd format.
TEEM_PNG_IPATH ?=
TEEM_PNG_LPATH ?=
## Header file is <png.h>
TEEM_PNG_LNAME ?= png
nrrd.Externs += PNG

## ZLIB: for the zlib library (in gzip and PNG image format) from https://zlib.net/
## Using zlib enables the "gzip" nrrd data encoding
## Header file is <zlib.h>.
TEEM_ZLIB_IPATH ?=
TEEM_ZLIB_LPATH ?=
TEEM_ZLIB_LNAME ?= z
nrrd.Externs += ZLIB

## BZIP2: for the bzip2 compression library, from https://sourceware.org/bzip2/
## Using bzip2 enables the "bzip2" nrrd data encoding.
TEEM_BZIP2_IPATH ?=
TEEM_BZIP2_LPATH ?=
## Header file is <bzlib.h>.
TEEM_BZIP2_LNAME ?= bz2
nrrd.Externs += BZIP2

## PTHREAD: use pthread-based multi-threading in airThreads.  Note that Windows has its
## own multithreading capabilities, which is used in airThread if !TEEM_PTHREAD, and we
## are on Windows.
TEEM_PTHREAD_IPATH ?=
TEEM_PTHREAD_LPATH ?=
## Header file is <pthread.h>
TEEM_PTHREAD_LNAME ?= pthread
air.Externs += PTHREAD

## LEVMAR: Levenberg-Marquardt from https://users.ics.forth.gr/~lourakis/levmar/
TEEM_LEVMAR_IPATH ?=
TEEM_LEVMAR_LPATH ?=
## Header file is <levmar.h>
TEEM_LEVMAR_LNAME ?= levmar
ten.Externs += LEVMAR
elf.Externs += LEVMAR

## FFTW3: FFTW version 3 from https://www.fftw.org/
TEEM_FFTW3_IPATH ?=
TEEM_FFTW3_LPATH ?=
## Header file is <fftw3.h>
TEEM_FFTW3_LNAME ?= fftw3
nrrd.Externs += FFTW3
