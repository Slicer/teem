#
# teem: Gordon Kindlmann's research software
# Copyright (C) 2002, 2001, 2000, 1999, 1998 University of Utah
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
#

#### Name of the library goes here
####
####
L := nrrd
####
####
####

# boilerplate: default targets and include tricks
ifeq (,$($(L).SEEN))
$(L).SEEN := true
TEEM_ROOT ?= ../..
TEEM_SRC ?= ..
ifeq (,$(DEF_TARGETS))
dev     : $(L)/dev
install : $(L)/install
usable  : $(L)/usable
clean   : $(L)/clean
clobber : $(L)/clobber
nothing :
DEF_TARGETS = true
endif
ifeq (,$(INCLUDED))
include ../GNUmakefile
endif

#### Describe library here
#### (hest needed for callback declaration)
#### 
$(L).NEED_LIBS = biff hest air
$(L).PUBLIC_HEADERS = nrrd.h nrrdDefines.h nrrdMacros.h nrrdEnums.h
$(L).PRIVATE_HEADERS = privateNrrd.h
$(L).OBJS = \
	accessors.o  arith.o        arraysNrrd.o  apply1D.o      \
	axes.o       comment.o      convertNrrd.o defaultsNrrd.o \
	endianNrrd.o enumsNrrd.o    filt.o        hestNrrd.o     \
	histogram.o  iter.o         kernel.o      map.o          \
	measure.o    methodsNrrd.o  parseNrrd.o   read.o         \
	reorder.o    resampleNrrd.o simple.o      subset.o       \
	superset.o   write.o 
$(L).TESTS = test/trand test/ax test/io test/minmax test/tkernel \
	test/typestest test/tline test/tread
####
####
####

# boilerplate: declare rules for this library
include $(TEEM_ROOT)/src/make/template.mk
endif
