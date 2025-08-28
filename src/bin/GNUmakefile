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

# boilerplate: default targets, include, and guarding per-"lib" rules
ifndef bin.Included
TeemRoot ?= ../..
TeemSrc ?= ..
ifndef DefaultTargets
  DefaultTargets := yes
  bild  : bin.bild
  clean : bin.clean
endif
.PHONY: $(TeemSrc)/GNUmakefile
include $(TeemSrc)/GNUmakefile

#### By the ordering and guarding of includes, this will ALWAYS be the last "library"
#### makefile to be parsed, regardless of the directory in which make started.

## Bins: all the command-line executables associated with teem
##
Bins := airSanity nrrdSanity overrgb talkweb unu cubic qbert ilk \
  emap gkms ungantry spots tend miter mrender pprobe vprobe gprobe \
  deconv puller

## BinsBild: full paths to all built and "installed" binaries
##
#$(warning BinsBild := $(foreach bin,$(Bins),$(BinPath)/$(bin)))
BinsBild := $(foreach bin,$(Bins),$(BinPath)/$(bin))

## Entry-point targets for binaries.
##
#$(warning bin.bild: $(BinsBild))
bin.bild: $(BinsBild)
bin.clean:
	$(RM) $(BinsBild)
ifdef LITTER
	$(RM) -r $(foreach bin,$(BinsBild),$(bin)$(LITTER))
endif

## Each bin needs to know the libraries it immediately depends on.
## We do *not* need to manually find the transitive closure here
##
# HEY should be generating this by some sort of grep on each bin.c
airSanity.Depends  := air
nrrdSanity.Depends := nrrd biff
overrgb.Depends    := nrrd hest biff air
talkweb.Depends    := nrrd biff hest air
unu.Depends        := unrrdu nrrd biff hest air
cubic.Depends      := ell
qbert.Depends      := bane gage nrrd hest air
ilk.Depends        := moss unrrdu nrrd ell biff hest air
emap.Depends       := limn nrrd ell biff hest air
gkms.Depends       := bane nrrd biff air
ungantry.Depends   := gage nrrd biff hest air
spots.Depends      := alan nrrd ell biff hest air
tend.Depends       := ten limn gage dye unrrdu nrrd ell biff air
miter.Depends      := mite hoover limn nrrd ell biff air
mrender.Depends    := meet
pprobe.Depends     := meet
vprobe.Depends     := meet
gprobe.Depends     := meet
deconv.Depends     := meet
puller.Depends     := meet

# Binary B depends on building the libraries it depends on (and we let
# make figure out the transitive closure), but/and,
# when linking binary B we need to know all (the transitive closure of)
# the libraries that B depends on, and we only want to compute that once,
# so we save it in a target-specific variable B.needs
# We express both inside a multi-line function:
#$(warning $(BinPath)/$1 : $(foreach lib,$($(1).Depends),$(lib).bild))
#$(warning $(BinPath)/$1 : $1.needs := $(sort $(foreach lib,$($(1).Depends),$($(lib).MeNeed))))
define bin-deps
$(BinPath)/$1 : $(foreach lib,$($(1).Depends),$(lib).bild)
$(BinPath)/$1 : $1.needs := $(sort $(foreach lib,$($(1).Depends),$($(lib).MeNeed)))
endef

# For each binary B, state B's library dependencies, and compute B.needs
$(foreach bin,$(Bins),$(eval $(call bin-deps,$(bin))))

## How to make binary B from B.c
##
$(BinPath)/% : $(TeemSrc)/bin/%.c
	$(CC) $(CFLAGS) $(BIN_CFLAGS) $(dashI) -o $@ $< \
	  $(dashL) $(call llink,$($(notdir $@).needs)) \
	  $(call Externs.dashL,$($(notdir $@).needs)) $(call Externs.llink,$($(notdir $@).needs)) -lm

# we're not a library; template.mk doesn't apply to us
endif # ifndef bin.Included
