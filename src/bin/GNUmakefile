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

#### Name of the library goes here.
#### 
#### 
L := bin
####
#### We're not a library at all actually ...
####

# boilerplate: default targets (except usable) and include tricks
ifeq (,$($(L).SEEN))
$(L).SEEN := true
TEEM_ROOT ?= ../..
TEEM_SRC ?= ..
ifeq (,$(DEF_TARGETS))
dev     : $(L)/dev
install : $(L)/install
clean   : $(L)/clean
clobber : $(L)/clobber
DEF_TARGETS = true
endif
ifeq (,$(INCLUDED))
include ../GNUmakefile
endif

## BINS: all the command-line executables associated with teem
## 
BINS = airSanity affine cubic nrrdSanity overrgb idx2pos \
  pos2idx vprobe unu qbert mrender miter gkms gantry

## _BIND: list of development locations for given bins
## _BINI: list of install locations for given bins
## _BINDI: list of install and development locations for given bins
##
_BIND = $(foreach bin,$(1),$(TEEM_SRC)/bin/$(bin))
_BINI = $(foreach bin,$(1),$(BDEST)/$(bin))
_BINDI = $(foreach bin,$(1),$(TEEM_SRC)/bin/$(bin) $(BDEST)/$(bin))

## BINS.{DEV,INST}: full paths to development and install versions
## 
BINS.DEV = $(call _BIND,$(BINS))
BINS.INST = $(call _BINI,$(BINS))


## print progress info only if this is a single-job build
##
#  bin/install ::
#	@echo ">>>>>>>>>>>>>>>>>>>>" bin "<<<<<<<<<<<<<<<<<<<<"

## Dntry-point targets for binaries.  There is no "usable" for
## binaries, since they aren't libraries.
##
bin/dev: $(BINS.DEV)
bin/install: $(BINS.INST)
bin/clean: 
	$(RM) $(BINS.DEV)
bin/clobber: bin/clean
	$(RM) $(BINS.INST)

## All the prerequisite usables of the bins, but needn't be
## recursively expanded.  These include things they only need the
## header of (for macros, like with ell), as well as the things to
## link against.
##
airSanity.NEED  = air
affine.NEED     = air
cubic.NEED      = ell
nrrdSanity.NEED = nrrd biff
idx2pos.NEED    = nrrd hest air
pos2idx.NEED    = nrrd hest air
overrgb.NEED    = nrrd hest biff air
qbert.NEED      = gage nrrd hest air
vprobe.NEED     = gage nrrd biff hest air
gkms.NEED       = bane nrrd biff air
unu.NEED        = unrrdu nrrd biff hest air
mrender.NEED    = hoover limn gage nrrd biff hest air
miter.NEED      = mite hoover limn nrrd ell biff air
gantry.NEED     = gage nrrd biff hest air

## TOP.DEP(BIN) : most dependent library (furthest from air) needed for BIN
## TOP.DEP.IS(LIB) : which bins have TOP.DEP of LIB
##
TOP.DEP = $(word 1,$(call NEED.LIBS,$(1)))
TOP.DEP.IS = $(foreach bin,$(BINS),$(if $(strip $(subst $(1),,$(call TOP.DEP,$(bin)))),,$(bin)))

## We need some way of expressing the fact that a given binary
## (install and dev) depends on the usable of its top dependency.
## Rather than going bin by bin, we can make a shorter list by doing
## this in terms of the libraries which are TOP.DEPs for one or more
## bins.  The addition of new bins with different TOP.DEPs will
## require supplementing these rules!!!  (do "make clobber; make
## bin/install" from teem/src to see)
##
$(call _BINDI,$(call TOP.DEP.IS,air)) : air/usable
$(call _BINDI,$(call TOP.DEP.IS,ell)) : ell/usable
$(call _BINDI,$(call TOP.DEP.IS,nrrd)) : nrrd/usable
$(call _BINDI,$(call TOP.DEP.IS,unrrdu)) : unrrdu/usable
$(call _BINDI,$(call TOP.DEP.IS,gage)) : gage/usable
$(call _BINDI,$(call TOP.DEP.IS,hoover)) : hoover/usable
$(call _BINDI,$(call TOP.DEP.IS,mite)) : mite/usable
$(call _BINDI,$(call TOP.DEP.IS,bane)) : bane/usable

## How to make BINS.DEV and BINS.INST: both kinds link against the
## same (installed) libraries and include the same (installed)
## headers, so the commands aren't actually any different ...
## 
$(TEEM_SRC)/bin/% $(BDEST)/% : $(TEEM_SRC)/bin/%.c
	$(P) $(CC) $(CFLAGS) $(BIN_CFLAGS) $(IPATH) -o $@ $< \
	  $(LPATH) $(call NEED.LIBLINKS,$(notdir $@)) -lz -lm

# we're not a library, template.mk doesn't apply to us
endif
