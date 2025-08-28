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
$(L).Depends = ten limn gage ell nrrd biff hest air
$(L).PublicHdr = pull.h
$(L).PrivateHdr = privatePull.h
$(L).Obj = defaultsPull.o energy.o infoPull.o volumePull.o taskPull.o  \
    binningPull.o corePull.o contextPull.o actionPull.o constraints.o \
    pointPull.o popcntl.o ccPull.o parmPull.o initPull.o enumsPull.o trace.o
$(L).Test = eparse circ
####
####
####

# boilerplate: declare rules for L via template, finish include guard
.PHONY: $(TeemSrc)/make/template.mk
include $(TeemSrc)/make/template.mk
endif # ifndef $(L).Included
