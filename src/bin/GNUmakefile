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
TEEM_ROOT ?= ../..
TEEM_SRC ?= ..
ifeq (,$(DEF_TARGETS))
DEF_TARGETS = true
dev     : $(L)/dev
install : $(L)/install
clean   : $(L)/clean
clobber : $(L)/clobber
include ../GNUmakefile
endif
ifeq (,$($(L).SEEN))
$(L).SEEN := true

#### By the ordering and guarding of includes, this will ALWAYS be the 
#### last "library" makefile to be parsed, regardless of the directory
#### in which make started.

## BINS: all the command-line executables associated with teem
## 
BINS = airSanity affine cubic nrrdSanity overrgb idx2pos \
  pos2idx unu vprobe qbert mrender miter gkms ungantry

## _binD(bins): list of development locations for given bins
## _binI(bins): list of install locations for given bins
## _binDI(bins): list of install and development locations for given bins
##
_binD = $(foreach bin,$(1),$(TEEM_SRC)/bin/$(bin))
_binI = $(foreach bin,$(1),$(BDEST)/$(bin))
_binDI = $(foreach bin,$(1),$(TEEM_SRC)/bin/$(bin) $(BDEST)/$(bin))

## BINS.{DEV,INST}: full paths to development and install versions
## 
BINS.DEV := $(call _binD,$(BINS))
BINS.INST := $(call _binI,$(BINS))


## Entry-point targets for binaries.  There is no "usable" for
## binaries, since they aren't libraries.
##
bin/dev: $(BINS.DEV)
bin/install: $(BINS.INST)
bin/clean: 
	$(RM) $(addsuffix $(DOTEXE),$(BINS.DEV))
bin/clobber: bin/clean
	$(RM) $(addsuffix $(DOTEXE),$(BINS.INST))

## All the prerequisite usables of the bins, recursivly expanded.  The
## calls to need() will work correctly because we've gotten here only
## after going through all the other library makesfiles.  The
## arguments to need() are libraries for which we need a header and/or
## libraries we need to link against.
##
airSanity.need  = $(call meneed,air)
affine.need     = $(call meneed,air)
cubic.need      = $(call meneed,ell)
nrrdSanity.need = $(call meneed,nrrd biff)
idx2pos.need    = $(call meneed,nrrd hest air)
pos2idx.need    = $(call meneed,nrrd hest air)
overrgb.need    = $(call meneed,nrrd hest biff air)
qbert.need      = $(call meneed,gage nrrd hest air)
vprobe.need     = $(call meneed,gage nrrd biff hest air)
gkms.need       = $(call meneed,bane nrrd biff air)
unu.need        = $(call meneed,unrrdu nrrd biff hest air)
mrender.need    = $(call meneed,hoover limn gage nrrd biff hest air)
miter.need      = $(call meneed,mite hoover limn nrrd ell biff air)
ungantry.need   = $(call meneed,gage nrrd biff hest air)

## top.dep(BIN) : most dependent library (furthest from air) needed for BIN
## top.dep.is(LIB) : which bins have TOP.DEP of LIB
##
top.dep = $(word 1,$(call linkorder,$($(1).need)))
top.dep.is = $(foreach bin,$(BINS),$(if $(strip $(subst $(1),,$(call top.dep,$(bin)))),,$(bin)))

## We need some way of expressing the fact that a given binary
## (install and dev) depends on the usable of its top dependency.
## Rather than going bin by bin, we can make a shorter list by doing
## this in terms of the libraries which are top.deps for one or more
## bins.  The addition of new bins with different top.deps will
## require supplementing these rules!!!  (do "make clobber; make
## bin/install" from teem/src to see)
##
## L.meneed is set by template.mk by inclusion from L/GNUmakefile
##
$(call _binDI,$(call top.dep.is,air))    : $(call used,$(air.meneed))
$(call _binDI,$(call top.dep.is,ell))    : $(call used,$(ell.meneed))
$(call _binDI,$(call top.dep.is,nrrd))   : $(call used,$(nrrd.meneed))
$(call _binDI,$(call top.dep.is,unrrdu)) : $(call used,$(unrrdu.meneed))
$(call _binDI,$(call top.dep.is,gage))   : $(call used,$(gage.meneed))
$(call _binDI,$(call top.dep.is,hoover)) : $(call used,$(hoover.meneed))
$(call _binDI,$(call top.dep.is,mite))   : $(call used,$(mite.meneed))
$(call _binDI,$(call top.dep.is,bane))   : $(call used,$(bane.meneed))

## How to make bins.dev and bins.inst: both kinds link against the
## same (installed) libraries and include the same (installed)
## headers, so the commands aren't actually any different ...
## 
$(TEEM_SRC)/bin/% $(BDEST)/% : $(TEEM_SRC)/bin/%.c
	$(P) $(CC) $(CFLAGS) $(BIN_CFLAGS) $(IPATH) -o $@ $< \
	  $(LPATH) $(call link,$($(notdir $@).need)) \
	  $(call xtern.Lpath,$($(notdir $@).need)) $(call xtern.link,$($(notdir $@).need)) -lm

# we're not a library; template.mk doesn't apply to us
endif
