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

AllExterns := PNG ZLIB BZIP2 LEVMAR FFTW3 PTHREAD

## In Teem, an "external" is a non-Teem library that Teem can _optionally_ link with to
## increase its functionality.  They are listed in AllExterns above.  Teem doesn't
## represent inter-external dependencies explicitly, but the ordering in AllExterns above
## MUST match the ordering on the compile link line: PNG precedes ZLIB because libpng
## depends on zlib. In the following "EXT" stands for one of the external names above.
##
## In the bigger world externals are often called "dependencies", but we don't use that
## word here in the realm of makefiles, because Teem doesn't "depend" on the externals in
## the sense of a target depending on pre-requisites in a make rule. Also in the bigger
## world, the externals are things that are chosen and resolved prior to running make, at
## configure time. Teem's GNUmakefiles have no notion of configuration or configure-time.
##
## Instead, Teem externals are selected and "configured" through the environment (in the
## unix sense) during compile-time. External EXT is enabled during "make" by setting the
## environment variable TEEM_EXT (just set it, to anything EXCEPT "0"), either by:
##   export TEEM_EXT; ...; make
## or:
##   TEEM_EXT= make
## or:
##   TEEM_EXT=1 make
## or TEEM_EXT=yesplease or any non-empty string *except* "0": "TEEM_EXT=0" leaves EXT
## disabled.  If external EXT is enabled during make, then the *preprocessor symbol*
## "TEEM_EXT" will be #define'd as "1" during source file compilation, and then Teem code
## that can benefit from EXT, guarded by "#if TEEM_EXT", will be compiled in.
##
## NOTE: "make" can only remember what was built when, not how it was built, so if you
## are using externals, and your build process is split over multiple invocations of
## "make", then the same externals have to be used each time. For example, making "unu"
## will need to -llib link against both Teem libraries like nrrd and the external
## libraries nrrd depends on, so "make"ing of unu has to know about the exact same
## externals as were part of the earlier "make"ing of libnrrd.a (and this fussiness is
## part of why configuration is a thing).
##
## To ensure externals are set the same for every "make": FEEL FREE TO MODIFY THIS FILE
## to change the variable settings below; they are read in every time make runs for Teem.
## Or, source a little shell script that sets the TEEM_ environment variables for you.
## Or, construct an elaborate alias that you use for Teem work, such as GLK's
##   alias tmake="TEEM_PTHREAD= \
##     TEEM_PNG= \
##     TEEM_PNG_IPATH=/opt/homebrew/Cellar/libpng/1.6.50/include \
##     TEEM_PNG_LPATH=/opt/homebrew/Cellar/libpng/1.6.50/lib \
##     TEEM_ZLIB= \
##     TEEM_ZLIB_IPATH=/opt/homebrew/opt/zlib/include \
##     TEEM_ZLIB_LPATH=/opt/homebrew/opt/zlib/lib \
##     TEEM_FFTW3= \
##     TEEM_FFTW3_IPATH=/opt/homebrew/Cellar/fftw/3.3.10_2/include \
##     TEEM_FFTW3_LPATH=/opt/homebrew/Cellar/fftw/3.3.10_2/lib \
##     TEEM_LEVMAR= \
##     TEEM_LEVMAR_IPATH=/Users/me/src/levmar-2.6 \
##     TEEM_LEVMAR_LPATH=/Users/me/src/levmar-2.6 \
##     TEEM_LEVMAR_LMORE=lapack\ blas \
##     make"
##
## How Teem uses externals has been in place since ~2001, but for TeemV2 all the
## GNUmakefiles were re-written.  TeemV2 moved the variables describing each external to
## here (since there are no longer multiple architectures to simultaneously support).
## For each external EXT:
## TEEM_EXT: set (here or in your environment) to enable EXT
## TEEM_EXT_IPATH, TEEM_EXT_LPATH: You very likely have to set these!
##   These variables have their same name from early Teem,
##   BUT NOTE: Now their names are accurate!
##   You no longer start TEEM_EXT_IPATH with "-I" or TEEM_EXT_LPATH with "-L".
##   They are really just the path to where the header or library file is; it is the job
##   of the functions Externs.dashD and Externs.dashI (defined in ../GNUmakefile and
##   used in template.mk) to add the -I and -L to each path as needed
## TEEM_EXT_LMORE: if the lib named in EXT.lname (below) is not the only library that
##   has to linked with to make a complete executable, than name here whatever more
##   libraries are needed for linking e.g:
##   TEEM_LEVMAR_LMORE ?= lapack blas
## EXT.lname: (NOT TO BE MESSED WITH) link with library lib with -llib
## lib.Externs: (NOT TO BE MESSED WITH) how we declare to the build system which Teem
##   library lib needs to know about the extern for the sake of its compilation flags

## PNG: for PNG images, from https://www.libpng.org/pub/png/libpng.html
## Using PNG enables the "png" nrrd format.
##
# TEEM_PNG ?=      # uncomment to enable
TEEM_PNG_IPATH ?=
TEEM_PNG_LPATH ?=
TEEM_PNG_LMORE ?=
## Header file is <png.h>
PNG.lname := png
nrrd.Externs += PNG

## ZLIB: for the zlib library (in gzip and PNG image format) from https://zlib.net/
## Using zlib enables the "gzip" nrrd data encoding
##
TEEM_ZLIB ?=      # uncomment to enable
TEEM_ZLIB_IPATH ?=
TEEM_ZLIB_LPATH ?=
TEEM_ZLIB_LMORE ?=
## Header file is <zlib.h>.
ZLIB.lname := z
nrrd.Externs += ZLIB

## BZIP2: for the bzip2 compression library, from https://sourceware.org/bzip2/
## Using bzip2 enables the "bzip2" nrrd data encoding.
##
# TEEM_BZIP2 ?=       # uncomment to enable
TEEM_BZIP2_IPATH ?=
TEEM_BZIP2_LPATH ?=
TEEM_BZIP2_LMORE ?=
# Header file is <bzlib.h>.
BZIP2.lname := bz2
nrrd.Externs += BZIP2

## LEVMAR: Levenberg-Marquardt from https://users.ics.forth.gr/~lourakis/levmar/
## Used for non-linear fitting in some diffusion model estimation
##
# TEEM_LEVMAR ?=      # uncomment to enable
TEEM_LEVMAR_IPATH ?=
TEEM_LEVMAR_LPATH ?=
TEEM_LEVMAR_LMORE ?=
# may want: TEEM_LEVMAR_LMORE ?= lapack blas
# Header file is <levmar.h>
LEVMAR.lname := levmar
ten.Externs += LEVMAR
elf.Externs += LEVMAR

## FFTW3: FFTW version 3 from https://www.fftw.org/
## Powers nrrdFFT() and "unu fft"
# TEEM_FFTW3 ?=      # uncomment to enable
TEEM_FFTW3_IPATH ?=
TEEM_FFTW3_LPATH ?=
TEEM_FFTW3_LMORE ?=
# Header file is <fftw3.h>
FFTW3.lname := fftw3
nrrd.Externs += FFTW3

## PTHREAD: use pthread-based multi-threading in airThreads, which enables multi-threaded
## volume rendering and other things.  Note that Windows has its own multithreading
## capabilities, which are used in airThread if !TEEM_PTHREAD and we are compiling on
## Windows.
# TEEM_PTHREAD ?=      # uncomment to enable
TEEM_PTHREAD_IPATH ?=
TEEM_PTHREAD_LPATH ?=
TEEM_PTHREAD_LMORE ?=
# Header file is <pthread.h>
PTHREAD.lname := pthread
air.Externs += PTHREAD
