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

TEEM_BIN = unrrdu/unu
PRIV_HEADERS = private.h
TEEM_BIN_OBJS = unu.o
#
# every unu command source file must define:
# char *xxxName
#      the short string used to invoke the command
# char *xxxInfo
#      approx. one line description of the function
# int xxxMain(int argc, char **argv, char *argv0)
#      the function to call to do the command
# This applies to every .o file below!
#
TEEM_BIN_OBJS += flip.o slice.o convert.o crop.o pad.o permute.o histo.o \
	resample.o cmedian.o reshape.o shuffle.o quantize.o block.o \
	unblock.o project.o swap.o join.o dhisto.o jhisto.o dice.o heq.o \
	histax.o gamma.o make.o 1op.o 2op.o 3op.o lut.o rmap.o imap.o \
	save.o 
TEEM_BIN_LIBS = -lnrrd -lbiff -lhest -lair -lm

include ../make/Common.mk
