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

TEEM_LIB = gage
HEADERS = gage.h
PRIV_HEADERS = private.h
TEEM_LIB_OBJS = \
	defaults.o misc.o print.o filter.o methods.o general.o update.o \
	scl.o sclfilter.o sclprint.o sclanswer.o \
	vec.o vecprint.o
TEST_BINS = test/gantry test/copy
TEST_BINS = test/gantry
TEEM_BINS = bin/vprobe bin/qbert 
TEEM_BIN_LIBS = -lgage -lnrrd -lbiff -lell -lhest -lair -lm

include ../make/teem.mk
