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

#### Library name
####
####
L := air
####
####
####

# boilerplate: default targets and include tricks
TEEM_ROOT ?= ../..
TEEM_SRC ?= ..
ifeq (,$(DEF_TARGETS))
DEF_TARGETS = true
dev     : $(L)/dev
clean   : $(L)/clean
include ../GNUmakefile
endif
ifeq (,$($(L).SEEN))
$(L).SEEN := true

#### Describe library here
####
####
$(L).NEED =
$(L).PUBLIC_HEADERS = air.h
$(L).PRIVATE_HEADERS = privateAir.h
$(L).OBJS = 754.o randMT.o randJSF.o array.o miscAir.o parseAir.o math.o \
	endianAir.o mop.o enum.o sane.o string.o threadAir.o heap.o
$(L).TESTS = test/floatprint test/doubleprint test/tok \
	test/tmop test/tline test/fp test/trand test/trandJSF test/tmisc \
	test/bessy test/tarr test/texp test/logrice test/tprint
####
####
####

# boilerplate: declare rules for this library
include $(TEEM_SRC)/make/template.mk
endif
ifeq (,$(INCLUDED))
  include $(TEEM_SRC)/bin/GNUmakefile
endif
