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
L := nrrd
####
####
####

# boilerplate: set top-level paths and targets if not set, include top-level GNUmakefile,
# and guard reading of per-library variables and (via template.mk) rules
TeemRoot ?= ../..
TeemSrc ?= ..
ifndef DefaultTargets
  DefaultTargets := yes
  bild  : $(L).bild
  test  : $(L).test
  clean : $(L).clean
endif # ifndef DefaultTargets
include $(TeemSrc)/GNUmakefile
ifndef $(L).Included
$(L).Included := yes

#### Describe library L here
####
####
# hest needed for callback declaration
$(L).Depends = biff hest air
$(L).PublicHdr = nrrd.h nrrdDefines.h nrrdMacros.h nrrdEnums.h
$(L).PrivateHdr = privateNrrd.h
$(L).Obj = $(patsubst %.c,%.o, \
    accessors.c apply1D.c apply2D.c arith.c arraysNrrd.c axis.c bsplKernel.c cc.c \
    ccmethods.c comment.c convertNrrd.c defaultsNrrd.c deringNrrd.c encoding.c \
    encodingAscii.c encodingBzip2.c encodingGzip.c encodingHex.c encodingRaw.c \
    encodingZRL.c endianNrrd.c enumsNrrd.c fftNrrd.c filt.c format.c formatEPS.c \
    formatNRRD.c formatPNG.c formatPNM.c formatText.c formatVTK.c gzio.c hestNrrd.c \
    histogram.c iter.c kernel.c keyvalue.c map.c measure.c methodsNrrd.c \
    parseNrrd.c range.c read.c reorder.c resampleContext.c resampleNrrd.c simple.c \
    subset.c superset.c tmfKernel.c winKernel.c write.c \
  )
$(L).Test = tread trand ax io strio texp minmax tkernel typestest \
	tline genvol quadvol convo kv reuse histrad otsu \
	dnorm morph phrnd
####
####
####

# boilerplate: enable "make test/T", template lib L rules, finish include guard
$(foreach T,$($(L).Test),$(eval test/$(T) : ../$(L)/test/$(T)))
include $(TeemSrc)/make/template.mk
endif # ifndef $(L).Included
