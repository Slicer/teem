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

LIB = nrrd
HEADERS = nrrd.h nrrdDefines.h nrrdMacros.h nrrdEnums.h
PRIV_HEADERS = private.h
LIB_OBJS = \
	accessors.o  arith.o      arrays.o     apply.o      \
	axes.o       comment.o    convert.o    defaults.o   \
	endian.o     enums.o      filt.o       histogram.o  \
        kernel.o     map.o        measure.o    methods.o    \
	parse.o      read.o       reorder.o    rsmp.o       \
	simple.o     subset.o     superset.o   write.o

#TEST_BINS = test/enum2str test/str2enum 
TEST_BINS = test/shuff test/ax test/io test/minmax test/tkernel
BINS = bin/nrrdSanity
BIN_LIBS = -lnrrd -lbiff -lair -lm

include ../make/Common.mk
