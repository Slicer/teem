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

#### See NOTE in ../GNUMakefile about the big simplification to the non-CMake build
#### process made for TeemV2.  This new file contains all the architecture-specific
#### details about how to compile (on a non-Windows machine).
#### Whatever is in this file is GLK needed to get Teem compiling; edit as you need.

# yes, we should probably be using ?= instead = for the assingments below, but
# the intent here is to be clear and explicit.  The assignment "AR ?= libtool"
# may not do anything because "AR" may default to "ar" for the sake of
# GNUmake's implicit rules, which is annoying because implicit rules are
# annoying. If you want different values for these variables, then change them.

# CC = scan-build clang # to run static analyzer https://clang.llvm.org/docs/ClangStaticAnalyzer.html
CC := cc
LD := ld
# historically `ar`
AR := libtool
# historically `ru`
ARFLAGS := -static -o
# sometimes needed after `ar` to store index in library
RANLIB :=
RM := rm -f
# if non-empty: also delete X$(LITTER) when deleting executable X with "make clean"
LITTER := .dSYM
CP := cp
CHMOD := chmod

# other $(CC) flags for .c compilation (e.g. optimization and warnings)
CFLAGS = -O3 -g -W -Wall -Wextra

# further $(CC) flags for creating executables when linking with (static) library
BIN_CFLAGS =

# more flags to try:
# -std=c90 -pedantic -Wno-long-long -Wno-overlength-strings -Wstrict-aliasing=2 -Wstrict-overflow=5
# -Weverything -Wno-poison-system-directories -Wno-padded -Wno-format-nonliteral -Wno-float-equal -Wno-reserved-id-macro
## for trying undefined behavior flagging  -fsanitize=undefined
