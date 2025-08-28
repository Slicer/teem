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

# boiler-ish-plate: default targets, include, and guarding per-"lib" rules
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

#### By the ordering and guarding of includes, this should ALWAYS be the last "library"
#### makefile to be parsed, regardless of the directory in which make started.

# Bins: all the command-line executables associated with teem
#
Bins := airSanity nrrdSanity overrgb talkweb unu cubic qbert ilk \
  emap gkms ungantry spots tend miter mrender pprobe vprobe gprobe \
  deconv puller

# BinsBild: full paths to all built and "installed" binaries
#
#$(warning BinsBild := $(foreach bin,$(Bins),$(BinPath)/$(bin)))
BinsBild := $(foreach bin,$(Bins),$(BinPath)/$(bin))

# Entry-point targets for binaries.
#
#$(warning bin.bild: $(BinsBild))
bin.bild: $(BinsBild)
bin.clean:
	$(RM) $(BinsBild)
ifdef LITTER
	$(RM) -r $(foreach bin,$(BinsBild),$(bin)$(LITTER))
endif

# Each bin needs to know the libraries it immediately depends on.  We
# do *not* need to manually find the transitive closure here.  What is
# listed here is just the lib that are #include <teem/lib.h> at the
# start of the bin.c (perhaps this could be automated) though for
# those that use meet, that is the only one listed (since meet depends
# on all others).
airSanity.Depends  := air
nrrdSanity.Depends := nrrd biff
overrgb.Depends    := nrrd hest biff air
talkweb.Depends    := nrrd biff hest air
unu.Depends        := unrrdu
cubic.Depends      := ell
qbert.Depends      := bane gage nrrd hest air
ilk.Depends        := moss unrrdu
emap.Depends       := limn nrrd ell biff hest air
gkms.Depends       := bane
ungantry.Depends   := gage nrrd biff hest air
spots.Depends      := ten alan nrrd hest
tend.Depends       := ten
miter.Depends      := mite hoover limn nrrd ell biff air
mrender.Depends    := meet
pprobe.Depends     := meet
vprobe.Depends     := meet
gprobe.Depends     := meet
deconv.Depends     := meet
puller.Depends     := meet

# Binary B depends on building the libraries it depends on. We could use the air.bild,
# biff.bild, etc phony targets that represent the completed building of those libraries,
# and let make figure out the transitive closure of those, to know the complete set of
# libraries that B depends on.  However, since phony targets are always out of date, the
# binaries would get recompiled with every run (e.g. airSanity gets recompiled every time
# because it would depends on air.bild but air.bild is always out of date).  Thus, we
# rely on the pre-computed per-library transitive closure stored in $(L).MeNeed to:
# - set $(B).needlib = transitive closure of all libraries depended on
# - set $(B).needfile = .h and .a files associated with all the above libraries
# - state dependency of $(BinPath)/$(B) on $(B).needfile
# We can put this inside a multi-line "function" that we then $(eval) However, everything
# except the $(1) argument (the binary name) to the function has to have delayed
# evaluation via $$, or else it won't work (specifically, eval will try to immediately
# evaluate $($(1).needfile) throughout the body of the function, but it won't have a
# value yet, so the dependency on $($(1).needfile) will be empty)
define bin-need
$(1).needlib  := $$(sort $$(foreach lib,$$($(1).Depends),$$($$(lib).MeNeed)))
$(1).needfile := $$(foreach lib,$$($(1).needlib),$$(call LibFile,$$(lib)) $$(call HdrFile,$$(lib)))
$$(BinPath)/$(1) : $$($(1).needfile)
endef

# Run bin-need for each binary B:
$(foreach bin,$(Bins),$(eval $(call bin-need,$(bin))))

# How to make binary B from B.c
$(BinPath)/% : $(TeemSrc)/bin/%.c
	$(CC) $(CFLAGS) $(BIN_CFLAGS) $(dashI) -o $@ $< \
	  $(dashL) $(call llink,$($(notdir $@).needlib)) \
	  $(call Externs.dashL,$($(notdir $@).needlib)) $(call Externs.llink,$($(notdir $@).needlib)) -lm

# we're not a library; template.mk doesn't apply to us
endif # ifndef bin.Included
