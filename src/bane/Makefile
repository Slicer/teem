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
LIB = bane
HEADERS = bane.h
LIBOBJS = arrays.o methods.o range.o measr.o inc.o clip.o \
	hvol.o trnsf.o trex.o probe.o
#TEST_BINS = test/apply test/gkms test/tinfo test/sigma test/pos \
#	test/opac test/pvg test/tprobe
TEST_BINS = test/tprobe
BINS = 
BINLIBS = -lbane -lnrrd -lbiff -lair -lm

include ../make/Common.mk
