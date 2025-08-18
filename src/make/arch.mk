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
#### process made for Teem v2.  This new file contains all the architecture-specific
#### details about how to compile (on a non-Windows machine).
#### Whatever is in this file is GLK needed to get Teem compiling; edit as you need.

AR = libtool
ARFLAGS = -static -o
RANLIB = ranlib

LD = cc

## for trying undefined behavior flagging  -fsanitize=undefined
OPT_CFLAG ?= -O3 -g
# CC = scan-build clang # to run static analyzer https://clang.llvm.org/docs/ClangStaticAnalyzer.html
CC = clang
STATIC_CFLAG = -Wl,-prebind
SHARED_CFLAG =
SHARED_LDFLAG = -dynamic -dynamiclib -fno-common
SHARED_INSTALL_NAME = -install_name

# more flags to try:
# -std=c90 -pedantic -Wno-long-long -Wno-overlength-strings -Wstrict-aliasing=2 -Wstrict-overflow=5
# -Weverything -Wno-poison-system-directories -Wno-padded -Wno-format-nonliteral -Wno-float-equal -Wno-reserved-id-macro
ARCH_CFLAG = -W -Wall -Wextra
ARCH_LDFLAG =

# Once in ~2003 when GLK was working on Windows/Cygwin, gnu make stopped working
# reliably because the file creation dates were not correctly tracking that
# outputs were being created after inputs. Use "SIGH = 1" to require that build
# steps happen at a nice slow pace.
SIGH =
