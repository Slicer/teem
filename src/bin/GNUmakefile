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

# boilerplate: default targets and include tricks
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

## BINS.{DEV,INST}: full paths to development and install versions
## 
BINS.DEV = $(addprefix $(TEEM_SRC)/bin/,$(BINS))
BINS.INST = $(addprefix $(BDEST)/,$(BINS))

bin/install ::
	@echo ">>>>>>>>>>>>>>>>>>>>" bin "<<<<<<<<<<<<<<<<<<<<"

## Dntry-point targets for binaries.  There is no "usable" for
## binaries, since they aren't libraries.
##
bin/dev: $(BINS.DEV)
bin/install:: $(BINS.INST)
bin/clean: 
	$(RM) $(BINS.DEV)
bin/clobber: bin/clean
	$(RM) $(BINS.INST)

## All the prerequisites of the bins, but needn't be recursively
## expanded.  These include things they only need the header of (for
## macros, like with ell), as well as the things to link against.
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

## How to make BINS.DEV and BINS.INST: both kinds link against the
## same (installed) libraries and include the same (installed)
## headers, so the commands aren't actually any different ...
## 
$(TEEM_SRC)/bin/% $(BDEST)/% : $(TEEM_SRC)/bin/%.c
	$(P) $(CC) $(CFLAGS) $(BIN_CFLAGS) $(IPATH) -o $@ $< \
	  $(LPATH) $(call NEED.LIBLINKS,$(notdir $@)) -lm

# we're not a library, template.mk doesn't apply to us
endif
