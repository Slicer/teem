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

####
#### top-level GNUmakefile: Master makefile for teem
####

## ".SUFFIXES :" speeds debugging with make -d (and probably make in
## general) by eliminating the list of suffixes checked by implicit
## pattern rules.  The rest of the rules are to tell make to forget
## about trying to automatically update the files that we "include"
##
.SUFFIXES :
% : %,v
% : RCS/%,v
% : RCS/%
% : s.%
% : SCCS/s.%
%.c : %.w     # this doesn't seem to work, unfortunately

## TEEM_ROOT: a relative path to the directory which contains the
## "src", "include", and all the architecture-specific directories
## (which in turn contain "bin", "lib", "obj", and "purify"). Whether
## make started on this makefile or on an individual library's
## makefile determine who gets to set TEEM_ROOT first.  Same for
## TEEM_SRC, a relative path to the "src" directory
##
TEEM_ROOT ?= ..
TEEM_SRC ?= .

## Including errorCheck.mk:
## - Ensures that TEEM_ARCH is set, and is set to something known
## - Sets ARCH and SUBARCH.
## - Ensures that SHEXT is set if TEEM_LINK_SHARED was requested
## - Ensures that PURIFY is set if TEEM_PURIFY was requested
##
include $(TEEM_SRC)/make/errorCheck.mk

## LIBS: all the teem libraries we'll try to build
## NUMS: "numbering" of all the libraries in link order
## NUM.LIBS: used for craziness below
##
LIBS = air hest biff ell nrrd unrrdu dye gage bane limn hoover mite ten echo
NUMS =  D   C    B    A   9     8     7   6     5    4     3    2    1   0
NUM.LIBS = $(join $(NUMS),$(LIBS:%=.%))

## MODES: the different kinds of builds that we support.  Declaring
## these modes (for all libraries) as phony saves make the effort of
## trying implicit rules to update them.
##
MODES = install usable dev clean clobber
.PHONY : $(foreach LIB,teem $(LIBS),$(MODES:%=$(LIB)/%))

## Top-level/default rules.  If make started with this file, then
## these will be the first rules that make sees, so that "make"
## defaults to "make X.install" for *every* library X.  If make
## started in a library subdirectory, these won't be seen at all.
##
ifeq (,$(DEF_TARGETS))
  install : teem/install
  dev     : teem/dev
  usable  : teem/usable
  clean   : teem/clean
  clobber : teem/clobber
  DEF_TARGETS := true
endif

## Top-level rules, available regardless of where make started
##
teem/install : $(addsuffix /install,$(LIBS) bin)
teem/dev     : $(addsuffix /dev,$(LIBS) bin)
teem/usable  : $(addsuffix /usable,$(LIBS) bin)
teem/clean   : $(addsuffix /clean,$(LIBS) bin)
teem/clobber : $(addsuffix /clobber,$(LIBS) bin)
nothing : ;

## Set directory-related variables: where to install things, as well
## as the directories used in conjunction with the -I and -L path
## flags for cc and ld
##
IDEST = $(TEEM_ROOT)/include
LDEST = $(TEEM_ROOT)/$(TEEM_ARCH)/lib
BDEST = $(TEEM_ROOT)/$(TEEM_ARCH)/bin
ODEST = $(TEEM_ROOT)/$(TEEM_ARCH)/obj
PCACHE = $(TEEM_ROOT)/$(TEEM_ARCH)/purify
IPATH += -I$(IDEST)
LPATH += -L$(LDEST)

## Before we read in the architecture-dependent stuff, take a stab at
## defining the various programs we'll need, and some of their flags.
## If these are not over-written, we assume that they'll work.
##
CC = cc
LD = ld
AR = ar
ARFLAGS = ru
RM = rm -f
CP = cp
CHMOD = chmod
SLEEP = sleep

## Enstate the architecture-dependent settings by reading through the
## file specific to the chosen architecture
##
include $(TEEM_SRC)/make/$(ARCH).mk

## By giving a list of library extensions we care about, LIBEXTS determines
## what kinds of libraries are build (just static, or both static and
## shared)
## 
ifdef SHEXT
  LIBEXTS = $(SHEXT)
endif
LIBEXTS += a

#######################################
## Flags
##
ifeq (true,$(TEEM_LINK_SHARED))
  BIN_CFLAGS += $(SHARED_CFLAG)
else
  BIN_CFLAGS += $(STATIC_CFLAG)
endif

CFLAGS += $(OPT_CFLAG) $(ARCH_CFLAG)
LDFLAGS += $(ARCH_LDFLAG) $(SHARED_LDFLAG)

## The AIR_SIZE_T_FMT macros requires knowing TEEM_32BIT for all
## source files wishing to use it, which might as well be all teem
## source files.
##
CFLAGS += -DTEEM_32BIT=$(TEEM_32BIT)

## SGI's C pre-processor errors aren't fatal by defailt
##
CFLAGS += $(CPP_ERROR_DIE)

## If using purify, set P to all the purify stuff that will prefix
## compile and link commands
## 
ifeq (true,$(TEEM_PURIFY))
  P = $(PURIFY) \
    -inuse-at-exit=yes -add-suppression-files=$(PCACHE)/.purify \
    -always-use-cache-dir -cache-dir=$(PCACHE)
endif

#######################################
## "Functions"
## The bread and butter of how template.mk works.

## Each of these can be $(call)ed with a library name as the only
## argument, in order to get a list of files or flags related to
## library.  
##
## {LIBS,HDRS}.INST: installed librs and headers
## USED.INST: whats "used" in a library, installed libraries and headers
## {LIBS,OBJS}.DEV: libraries and object files for non-install linking
## TESTS.DEV: tests built in-place, for debugging
## HDRS.DEV : the local (original) copies of public and private headers
##
LIBS.INST = $(foreach ext,$(LIBEXTS),$(LDEST)/lib$(1).$(ext))
USED.INST = $(call LIBS.INST,$(1)) $(call HDRS.INST,$(1))
HDRS.INST = $(addprefix $(IDEST)/,$($(1).PUBLIC_HEADERS))
LIBS.DEV = $(foreach ext,$(LIBEXTS),$(ODEST)/lib$(1).$(ext))
OBJS.DEV = $(addprefix $(ODEST)/,$($(1).OBJS))
TESTS.DEV = $(addprefix $(TEEM_SRC)/$(1)/,$($(1).TESTS))
HDRS.DEV = $(addprefix $(TEEM_SRC)/$(1)/,$($(1).PUBLIC_HEADERS)) \
  $(addprefix $(TEEM_SRC)/$(1)/,$($(1).PRIVATE_HEADERS))

## $(call NEED.LIBS,L): pseudo-recursive expansion of all the
## libraries L depends on, either directly or indirectly.  9 levels
## deep of recursion is about 7 levels overkill for current teem, and
## is apt to be plenty for any future teem.  make won't allow direct
## or indirect recursion in variable names.
## 
## Since we rely on $(sort) to remove redundancies, we need a way of
## putting the libraries back in dependency order (!= lexical order).
## So, we prefix the library names with [0..D] (via $(join), to create
## NUM.LIBS, and finally _ND.NLS), re-sort them, and then lose the
## prefix (to create NEED.LIBS).
## 
_ND.0 = $($(1).NEED)
_ND.1 = $(sort $(foreach LIB,$(call _ND.0,$(1)),$(LIB) $(call _ND.0,$(LIB))))
_ND.2 = $(sort $(foreach LIB,$(call _ND.1,$(1)),$(LIB) $(call _ND.0,$(LIB))))
_ND.3 = $(sort $(foreach LIB,$(call _ND.2,$(1)),$(LIB) $(call _ND.0,$(LIB))))
_ND.4 = $(sort $(foreach LIB,$(call _ND.3,$(1)),$(LIB) $(call _ND.0,$(LIB))))
_ND.5 = $(sort $(foreach LIB,$(call _ND.4,$(1)),$(LIB) $(call _ND.0,$(LIB))))
_ND.6 = $(sort $(foreach LIB,$(call _ND.5,$(1)),$(LIB) $(call _ND.0,$(LIB))))
_ND.7 = $(sort $(foreach LIB,$(call _ND.6,$(1)),$(LIB) $(call _ND.0,$(LIB))))
_ND.8 = $(sort $(foreach LIB,$(call _ND.7,$(1)),$(LIB) $(call _ND.0,$(LIB))))
_ND.9 = $(sort $(foreach LIB,$(call _ND.8,$(1)),$(LIB) $(call _ND.0,$(LIB))))
_ND.NLS = $(foreach LIB,$(call _ND.9,$(1)),$(filter %.$(LIB),$(NUM.LIBS)))
NEED.LIBS = $(patsubst .%,%,$(suffix $(sort $(call _ND.NLS,$(1)))))

## NEED.USABLE: .X.usable for all needed X
## NEED.LIBLINKS: -lX for all needed X
##
NEED.USABLE = $(foreach LIB,$(call NEED.LIBS,$(1)),$(TEEM_SRC)/.$(LIB).usable)
NEED.LIBLINKS = $(foreach lib,$(call NEED.LIBS,$(1)),-l$(lib))

## For XXX things like endianness, TEEM_XXX is set in the
## architecture-specific makefile, and $(L).NEED_XXX is set in the
## Makefile for the library which needs that info.  Meanwhile,
## teemXxx.h in teem's top-level include directory contains
## C-preprocessor code to make sure that the variable has been set,
## and set to something reasonable.
##
## Right now XXX is either "ENDIAN", "QNANHIBIT", "DIO"
##
MORE_CFLAGS = $(if $($(1).NEED_ENDIAN),-DTEEM_ENDIAN=$(TEEM_ENDIAN))\
 $(if $($(1).NEED_DIO),-DTEEM_DIO=$(TEEM_DIO))\
 $(if $($(1).NEED_QNANHIBIT),-DTEEM_QNANHIBIT=$(TEEM_QNANHIBIT))

WHATS.USED.EXISTS = \
$(if $(strip $(foreach x,$(call USED.INST,$(1)),$(if $(wildcard $(x)),,no))),$(call USED.INST,$(1)))

#######################################
## Read in the makefiles for all the libraries.  In order to prevent
## recursive inclusion, we set INCLUDED. The library makefiles won't
## include this file if INCLUDED has been set.  If make started in a
## library directory, then its include guards will prevent us from
## reading its body a second time. Note: "include" is a directive, not
## a function, which eliminates the possibility of iterating through
## the libraries, reading the make file, and then setting variables
## based on what was just read.
##
## The L.SAVE is so that if L was set when we got to here (because it
## was set by a library makefile that included us), then we'll
## remember that value and set it back when we're done.  Otherwise the
## library makefiles have to set L twice, before and after
## (conditionally) including this makefile.
##
ifneq (,$(L))
  ifeq (,$(L.SAVE))
    L.SAVE := $(L)
  endif
endif
INCLUDED = true
include $(foreach LIB,$(LIBS),$(TEEM_SRC)/$(LIB)/GNUmakefile)
include $(TEEM_SRC)/bin/GNUmakefile
ifneq (,$(L.SAVE))
  L := $(L.SAVE)
endif
