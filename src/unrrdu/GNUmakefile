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
L := unrrdu
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
####
####
$(L).NEED_LIBS = nrrd biff hest air
$(L).PUBLIC_HEADERS = unrrdu.h
$(L).PRIVATE_HEADERS = privateUnrrdu.h
$(L).OBJS = flotsam.o
#
# The source for unu command xxx must define a unuCmd "unu_xxxCmd".
# This applies to every .o file below.
#
$(L).OBJS  += flip.o slice.o convert.o crop.o pad.o permute.o histo.o \
	resample.o cmedian.o reshape.o shuffle.o quantize.o unquantize.o \
	block.o unblock.o project.o swap.o join.o dhisto.o jhisto.o dice.o \
	heq.o histax.o gamma.o make.o 1op.o 2op.o 3op.o lut.o rmap.o imap.o \
	save.o 
####
####
####

# boilerplate: declare rules for this library
include $(TEEM_ROOT)/src/make/template.mk
endif
