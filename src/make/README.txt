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

(these reflect the simplification of non-CMake building done for TeemV2)

=========================
User-set environment variables which effect global things:
=========================

TEEM_ROOT: the top-level "teem" directory, under which are the directories where object,
  library, and include files will be installed.  If not set, the top-level directory is
  taken to be "../..", when inside the source directory for the individual libraries

=========================
The variables that can be set by the individual library Makefile's
=========================

LIB: the name of the library being compiled.  If this isn't set, the
  assumption is that there is no new library to compile, but simply
  a set of binaries which depend on other libraries

LIB_BASENAME: the base name of the archive and shared library files;
  by default this is set to "lib$(LIB)", but setting this allows one
  to over-ride that.

HEADERS: the "public" .h files for this library; these will be installed

PRIV_HEADERS: .h files needed for this library, but not installed

LIBOBJS: the .o files (created from .c files) which comprise this library

TEST_BINS: executables which are used to debug a library, but which will
  not be installed

BINS: executables which will be installed

BINLIBS: all the libraries (-l<name> ...) against which $(BINS) and
  $(TEST_BINS) collectively depend, for example "-lnrrd -lbiff -lair"
  is used for unrrdu stuff.  Unfortunately, warning messages of the
  sort " ... not used for resolving any symbol" are to be expected due
  to the current simplistic nature of the Teem makefiles

IPATH, LDPATH: the "-I<dir>" and "-L<dir>" flags for the compiler and linker.
  Values set here will be suffixed by the common makefile

LDLIBS: when making shared libraries, it is sometimes necessary to
  link against other libraries.  $(LDLIBS) is the last argument to the
  $(LD) call which creates a shared library, and should be used like
  $(BINLIBS). $(LPATH) can be set with the "-L<dir." flags non-Teem
  libraries, the Teem library flags will be suffixed on.
