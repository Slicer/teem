#
# The contents of this file are subject to the University of Utah Public
# License (the "License"); you may not use this file except in
# compliance with the License.
#
# Software distributed under the License is distributed on an "AS IS"
# basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See
# the License for the specific language governing rights and limitations
# under the License.
#
# The Original Source Code is "teem", released March 23, 2001.
#  
# The Original Source Code was developed by the University of Utah.
# Portions created by UNIVERSITY are Copyright (C) 2001, 1998 University
# of Utah. All Rights Reserved.
#
#
LIB = nrrd
HEADERS = nrrd.h nrrdDefines.h nrrdMacros.h nrrdEnums.h
PRIV_HEADERS = private.h
LIB_OBJS = \
	accessors.o  arith.o      arrays.o     axes.o       \
        comment.o    convert.o    defaults.o   endian.o     \
	filt.o       histogram.o  kernel.o     map.o        \
	measure.o    methods.o    parse.o      read.o       \
	reorder.o    rsmp.o       simple.o     subset.o     \
        write.o

#TEST_BINS = test/enum2str test/str2enum 
TEST_BINS = test/shuff test/ax test/io test/minmax test/tkernel
BINS = bin/nrrdSanity
BIN_LIBS = -lnrrd -lbiff -lair -lm

include ../make/Common.mk
