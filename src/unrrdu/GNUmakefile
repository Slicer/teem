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
L := unrrdu
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
$(L).Depends = moss nrrd biff hest air
$(L).PublicHdr = unrrdu.h
$(L).PrivateHdr = privateUnrrdu.h
$(L).Obj = flotsam.o
# The source file xxx.c for "unu xxx" must define a unuCmd "unu_xxxCmd".
# This applies to every .o file listed here.
$(L).Obj += $(patsubst %.c,%.o, \
    1op.c 2op.c 3op.c aabplot.c about.c acrop.c affine.c axdelete.c axinfo.c \
    axinsert.c axmerge.c axsplit.c basinfo.c block.c built.c ccadj.c ccfind.c \
    ccmerge.c ccsettle.c cksum.c cmedian.c convert.c crop.c data.c dering.c \
    dhisto.c dice.c diff.c dist.c dnorm.c env.c fft.c flip.c gamma.c grid.c hack.c \
    head.c heq.c histax.c histo.c i2w.c ilk.c imap.c inset.c jhisto.c join.c lut.c \
    lut2.c make.c minmax.c mlut.c mrmap.c ninspect.c pad.c permute.c project.c \
    quantize.c resample.c reshape.c rmap.c save.c shuffle.c slice.c splice.c \
    sselect.c subst.c swap.c tile.c unblock.c uncmt.c undos.c unorient.c \
    unquantize.c untile.c vidicon.c w2i.c \
  )
$(L).Test =
####
####
####

# boilerplate: enable "make test/T", template lib L rules, finish include guard
$(foreach T,$($(L).Test),$(eval test/$(T) : ../$(L)/test/$(T)))
include $(TeemSrc)/make/template.mk
endif # ifndef $(L).Included
