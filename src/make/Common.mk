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

# learned: you get very confusing errors if one the filenames in 
# $(LIB_OBJS) ends with ".c"



#
# Common makefile variables and rules for all teem libraries
#
# This is included by all the individual library/utility Makefiles,
# at the *end* of that Makefile.

# all the architectures currently supported
KNOWN_ARCH = irix6.n32 irix6.64 linux cygwin solaris

# there is no default architecture
ifndef TEEM_ARCH
  $(warning *)
  $(warning *)
  $(warning *    Environment variable TEEM_ARCH not set.)
  $(warning *    Possible settings currently supported:)
  $(warning *    $(KNOWN_ARCH))
  $(warning *)
  $(warning *)
  $(error Make quitting)
endif

# the architecture name may have two parts, ARCH and SUBARCH,
# seperated by one period
ARCH = $(basename $(TEEM_ARCH))
SUBARCH = $(patsubst .%,%,$(suffix $(TEEM_ARCH)))

# verify that we can recognize the architecture setting
ifeq (,$(strip $(findstring $(TEEM_ARCH),$(KNOWN_ARCH))))
  $(warning *)
  $(warning *)
  $(warning *    Environment variable TEEM_ARCH = "$(TEEM_ARCH)" unknown)
  $(warning *    Possible settings currently supported:)
  $(warning *    $(KNOWN_ARCH))
  $(warning *)
  $(warning *)
  $(error Make quitting)
endif

# The root of the teem tree, as seen from the library subdirectories
# of the the "src" directory.  TEEM_ROOT is the directory which
# contains the "src", "include", and all the architecture-specific
# directories (which in turn contain "bin", "lib", "obj", and "purify")
ifndef TEEM_ROOT
  PREFIX = ../..
else
  PREFIX = $(TEEM_ROOT)
endif

# set directory-related variables: where to install things, as well as
# the directories used in conjunction with the -I and -L path flags
# for cc and ld
IDEST = $(PREFIX)/include
LDEST = $(PREFIX)/$(TEEM_ARCH)/lib
BDEST = $(PREFIX)/$(TEEM_ARCH)/bin
ODEST = $(PREFIX)/$(TEEM_ARCH)/obj
PCACHE = $(PREFIX)/$(TEEM_ARCH)/purify
IPATH += -I$(IDEST)
LPATH += -L$(LDEST)

# before we read in the architecture-dependent stuff, take a stab at
# defining the various programs we'll need, by assuming that they're
# already in the path AND (and this is a huge assumption) that they
# are the correct make and model
CC = cc
LD = ld
AR = ar
RM = rm -f
CP = cp
CHMOD = chmod
PURIFY = purify

###
### effect the architecture-dependent settings by reading through the
### file specific to the chosen architecture
###
include $(PREFIX)/src/make/$(ARCH).mk
###
###
###

# if we need to create a new library, then we set up some variables
# relating to the library name, location, and required objects:
# OBJ_PREF OBJS LIB.A _LIB.A LIB.S _LIB.S
#
ifdef LIB
  OBJ_PREF = $(ODEST)/$(LIB)
  OBJS = $(addprefix $(OBJ_PREF)/,$(LIB_OBJS))
  LIB_BASENAME ?= lib$(LIB)
  _LIB.A = $(LIB_BASENAME).a
  LIB.A = $(OBJ_PREF)/$(_LIB.A)
  # if SHEXT is not defined to any non-zero length string, then these
  # variables are not set, and no shared libraries are created at all
  ifdef SHEXT
    _LIB.S = $(LIB_BASENAME).$(SHEXT)
    LIB.S = $(OBJ_PREF)/$(_LIB.S)
  endif
endif

ifdef BIN
  _BIN = $(notdir $(BIN))
  _BLIB = $(patsubst %/,%,$(dir $(BIN)))
  ifdef OBJ_PREF
    ifneq ($(OBJ_PREF),$(ODEST)/$(_BLIB))
      $(error BIN $(BIN) must be in same path as LIB $(LIB))
    endif
  endif
  OBJ_PREF = $(ODEST)/$(_BLIB)
  OBJS = $(addprefix $(OBJ_PREF)/,$(BIN_OBJS))
  # The non-installed binary will be called $(_BIN) and will be in the
  # main source directory (wherever this .mk file was sourced from).
  # The installed binary goes in the usual place (BDEST)
endif

# the complete path names for installed headers and libraries
INSTALL_HDRS = $(addprefix $(IDEST)/,$(HEADERS))
INSTALL_LIBS = $(addprefix $(LDEST)/,$(_LIB.A) $(_LIB.S))

# the complete path name for the "single" binary
INSTALL_BIN = $(addprefix $(BDEST)/,$(_BIN))

# the complete path names of the binaries to be installed.
# The fanciness here is that we want to allow the sources for the binaries
# to be in subdirectories of where the library's sources are, but those
# subdirectories can't be part of the installed binary name ...
INSTALL_BINS = $(addprefix $(BDEST)/,$(notdir $(BINS)))
# ... however, we need to set VPATH in order to help "make" find the 
# sources for the binaries in their subdirectories
VPATH = $(sort $(dir $(BINS)))
# Thus, bane can have its single binary "gkms" built from "bane/bin/gkms.c",
# but it will be installed as "gkms" in the architecture's bin directory

#
# flags.
#
ifeq ($(TEEM_LINK_SHARED),true)
  BIN_CFLAGS += $(SHARED_CFLAG)
  ifdef LIB
    ifndef SHEXT
      $(warning *)
      $(warning *)
      $(warning *    Can't do shared library linking with SHEXT unset)
      $(warning *    See architecture-specific .mk file.)
      $(warning *)
      $(warning *)
      $(error Make quitting)
    endif
    THISLIB = $(LIB.S)
    THISLIB_LPATH = -L$(OBJ_PREF)
  endif
else
  BIN_CFLAGS += $(STATIC_CFLAG)
  ifdef LIB
    THISLIB = $(LIB.A)
    THISLIB_LPATH = -L$(OBJ_PREF)
  endif
endif  
CFLAGS += $(OPT_CFLAG) $(ARCH_CFLAG)
LDFLAGS += $(ARCH_LDFLAG) $(SHARED_LDFLAG)
ARFLAGS = ru

# for things like endianness, TEEM_X is set in the archicture-specific
# makefile, and NEED_X is set in the Makefile for the library which
# needs that info.  Meanwhile, teem/need/x.h in teem's top-level
# include directory contains C-preprocessor code to make sure that the
# variable has been set and set to something reasonable.
#
# Right now X is either "ENDIAN", "QNANHIBIT", or "DIO".
#
ifeq ($(NEED_ENDIAN),true)
  CFLAGS += -DTEEM_ENDIAN=$(TEEM_ENDIAN)
  SET_DIE = 1
endif
ifeq ($(NEED_QNANHIBIT),true)
  CFLAGS += -DTEEM_QNANHIBIT=$(TEEM_QNANHIBIT)
  SET_DIE = 1
endif
ifeq ($(NEED_DIO),true)
  CFLAGS += -DTEEM_DIO=$(TEEM_DIO)
  SET_DIE = 1
endif

# the SET_DIE silliness is because with some cc compilers (cough, SGI),
# C-preprocessor "error"s aren't fatal unless you specifically ask
# them to be.  Please.
#
ifeq ($(SET_DIE),1)
  CFLAGS += $(CPP_ERROR_DIE)
endif

#
# are we using purify?
# 
ifeq ($(TEEM_PURIFY),true)
  ifndef PURIFY
    $(warning *)
    $(warning *)
    $(warning *    Purification requested, but env variable PURIFY not set)
    $(warning *    Edit make/$(ARCH).mk in teem root directory)
    $(warning *)
    $(warning *)
    $(error Make quitting)
  endif
  POPTS = -inuse-at-exit=yes -suppression-filenames=$(PCACHE)/.purify
#  POPTS += -always-use-cache-dir -cache-dir=$(PCACHE)
  P = $(PURIFY) $(POPTS)
endif

#
# Okay kids, here are the rules.
# 

# "make" ("make all") will make the bins and test bins in the current
# directory (or a subdirectory).  The potentially odd thing is that
# "make install" DOES NOT create the bins and test bins here and then
# install them, it just creates them where they belong.  The
# assumption here is that someone who wants to install the
# install-able things from this library can say "make install" (and
# not "make; make install") while someone debugging/developing a
# library can say "make"
all: $(LIB.A) $(LIB.S) $(TEST_BINS) $(BINS) $(_BIN)
testbins: $(TEST_BINS)
bins: $(BINS)

# "make install" installs the headers, libraries, and binaries, but
# does not build the bins or test bins in the current directory
install: $(INSTALL_LIBS) $(INSTALL_BINS) $(INSTALL_HDRS) $(INSTALL_BIN)

# "make clean" removes what's created by "make" ("make all")
clean:
	$(RM) $(OBJS) $(LIB.A) $(LIB.S) $(TEST_BINS) $(BINS) \
	  $(OTHER_CLEAN) $(BIN)

# "make uninstall" removes what's created by "make install"
uninstall:
	$(if $(HEADERS), $(RM) $(foreach h, $(HEADERS), $(IDEST)/$(h)))
	$(if $(LIB), $(RM) $(LDEST)/$(_LIB.A))
	$(if $(LIB.S), $(RM) $(LDEST)/$(_LIB.S))
	$(if $(BINS), $(RM) $(foreach b, $(BINS), $(BDEST)/$(b)))
	$(if $(BIN), $(RM) $(BDEST)/$(BIN))

# "make destroy" does as you'd expect
destroy: clean uninstall

# all objects depend on all headers
$(OBJS): $(HEADERS) $(PRIV_HEADERS)

# NB: .o files are NEVER created in the same directory as the source
$(OBJ_PREF)/%.o: %.c
	$(P) $(CC) $(CFLAGS) -I. $(IPATH) -c $< -o $@

# the libraries are dependent on the respective object files
$(LIB.A): $(OBJS)
	$(AR) $(ARFLAGS) $(LIB.A) $(OBJS)
$(LIB.S): $(OBJS)
	$(LD) -o $(LIB.S) $(LDFLAGS) $(LPATH) $(OBJS) $(LDLIBS)

# This rule is for binaries and test binaries which are NOT being
# installed.  Such binaries depend on the library file $(THISLIB)
# (which will be defined ifdef LIB, and will probably be set to
# lib$(LIB).a or lib$(LIB).$(SHEXT), according to
# $(TEEM_LINK_SHARED)).  We link against the copy of the library
# living in the object directory (with $(THISLIB_LPATH) preceeding
# $(LPATH)), so that the binaries can be use to debug the library
# before installing the library in the proper lib directory.  The
# style of linking is still controlled by $(TEEM_LINK_SHARED).
#
# NB: This creates the executable in the same directory as the .c file
#
%: %.c $(THISLIB)
	$(P) $(CC) $(CFLAGS) $(BIN_CFLAGS) -I. $(IPATH) -o $@ $< \
	   $(THISLIB_LPATH) $(LPATH) \
	   $(BIN_LIBS)

$(_BIN): $(OBJS)
	$(P) $(CC) $(CFLAGS) $(BIN_CFLAGS) -I. $(IPATH) -o $@ $(OBJS) \
	   $(LPATH) $(BIN_LIBS)

# This rule is to satisfy the target $(INSTALL_HDRS)
$(IDEST)/%: %
	$(CP) $< $(IDEST)
	$(CHMOD) 644 $(IDEST)/$<

# This rule is to satisfy the target $(INSTALL_LIBS)
$(LDEST)/%: $(OBJ_PREF)/%
	$(CP) $< $(LDEST)
	$(CHMOD) 755 $(LDEST)/$<

# This rule is to satisfy the target $(INSTALL_BIN)
$(BDEST)/$(_BIN): $(_BIN)
	$(CP) $(_BIN)$(DOTEXE) $(BDEST)
	$(CHMOD) 755 $(BDEST)/$(_BIN)$(DOTEXE)

# This rule is to satisfy the target $(INSTALL_BINS)
# The binaries which are to be installed should link against the
# already-installed libraries, and use the already installed headers.
#
# NB: VPATH is used to locate the prerequisite %.c in a subdirectory
#
$(BDEST)/%: %.c $(INSTALL_LIBS) $(INSTALL_HDRS)
	$(P) $(CC) $(CFLAGS) $(BIN_CFLAGS) $(IPATH) -o $@ $< \
	   $(LPATH) $(BIN_LIBS)
	$(CHMOD) 755 $@$(DOTEXE)

