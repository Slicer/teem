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
HEADERS = nrrd.h nrrdDefines.h nrrdAccessors.h
LIBOBJS = types.o arrays.o accessors.o io.o methods.o \
	subset.o reorder.o measr.o histogram.o filt.o \
	convert.o endian.o kernel.o
#TEST_BINS = test/subtest test/iotest test/typestest
TEST_BINS = test/rsmp test/ktest test/shuff
BINS = 
BINLIBS = -lnrrd -lbiff -lair -lm

include ../make/Common.mk
