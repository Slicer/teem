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
$(L).Depends = moss nrrd biff hest air
$(L).PublicHdr = unrrdu.h
$(L).PrivateHdr = privateUnrrdu.h
$(L).Obj = flotsam.o
#
# The source for unu command xxx must define a unuCmd "unu_xxxCmd".
# This applies to every .o file below.
#
$(L).Obj  += flip.o slice.o sselect.o convert.o crop.o pad.o permute.o \
	histo.o resample.o cmedian.o reshape.o shuffle.o minmax.o quantize.o \
	unquantize.o project.o swap.o join.o dhisto.o \
	jhisto.o dice.o heq.o histax.o gamma.o make.o 1op.o 2op.o 3op.o \
	lut.o subst.o rmap.o imap.o lut2.o save.o head.o data.o splice.o \
	inset.o axinsert.o axdelete.o axinfo.o ccfind.o ccadj.o ccmerge.o \
	ccsettle.o about.o axsplit.o axmerge.o mlut.o mrmap.o tile.o untile.o \
	unorient.o env.o dist.o affine.o i2w.o w2i.o fft.o acrop.o dering.o \
	diff.o cksum.o dnorm.o vidicon.o undos.o uncmt.o basinfo.o grid.o ilk.o \
	ninspect.o hack.o aabplot.o block.o unblock.o built.o
$(L).Test =
####
####
####

# boilerplate: declare rules for L via template, finish include guard
.PHONY: $(TeemSrc)/make/template.mk
include $(TeemSrc)/make/template.mk
endif # ifndef $(L).Included
