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

# boilerplate: start include guard based on containing dir, then
# set library name L, define default targets, include top-level GNUmakefile
pardir := $(notdir $(patsubst %/,%,$(dir $(abspath $(lastword $(MAKEFILE_LIST))))))
ifndef $(pardir).Included
######## HERE is where L is set to library name
L := $(pardir)
$(L).Included := yes
TeemRoot ?= ../..
TeemSrc ?= ..
ifndef DefaultTargets
  DefaultTargets := yes
  bild  : $(L).bild
  test  : $(L).test
  clean : $(L).clean
endif # ifndef DefaultTargets
.PHONY: $(TeemSrc)/GNUmakefile
include $(TeemSrc)/GNUmakefile

#### Describe library L here
####
####
# hest needed for callback declaration
$(L).Depends = biff hest air
$(L).PublicHdr = nrrd.h nrrdDefines.h nrrdMacros.h nrrdEnums.h
$(L).PrivateHdr = privateNrrd.h
$(L).Obj = \
	accessors.o  arith.o     arraysNrrd.o   apply1D.o apply2D.o \
	axis.o       comment.o   convertNrrd.o  defaultsNrrd.o   \
	deringNrrd.o   endianNrrd.o   enumsNrrd.o   filt.o   gzio.o  \
	hestNrrd.o   histogram.o iter.o         kernel.o   	 \
	map.o        measure.o   methodsNrrd.o  parseNrrd.o      \
	read.o       write.o        reorder.o   resampleNrrd.o \
	simple.o     subset.o     superset.o  tmfKernel.o      \
	winKernel.o  bsplKernel.o  ccmethods.o  cc.o        range.o  \
        encoding.o   encodingRaw.o  encodingAscii.o  encodingHex.o \
	encodingGzip.o   encodingBzip2.o  encodingZRL.o \
	format.o     formatNRRD.o     formatPNM.o      formatPNG.o \
	formatVTK.o      formatText.o     formatEPS.o      \
	keyvalue.o  resampleContext.o  fftNrrd.o
$(L).Test = tread trand ax io strio texp minmax tkernel typestest \
	tline genvol quadvol convo kv reuse histrad otsu \
	dnorm morph phrnd
####
####
####

# boilerplate: declare rules for L via template, finish include guard
.PHONY: $(TeemSrc)/make/template.mk
include $(TeemSrc)/make/template.mk
endif # ifndef $(L).Included
