#
# Common makefile variables and rules for all teem libraries
#
# This is included by all the individual library/utility Makefiles,
# at the *end* of the Makefile.

# all the architectures currently supported
#KNOWN_ARCH = irix6.n32 irix6.64 linux cygwin
KNOWN_ARCH = irix6.n32 irix6.64 linux

# there is no default architecture
ifndef TEEM_ARCH
  $(warning *)
  $(warning *)
  $(warning *    Env variable TEEM_ARCH not set.)
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
  $(warning *    Env variable TEEM_ARCH = "$(TEEM_ARCH)" unknown)
  $(warning *    Possible settings currently supported:)
  $(warning *    $(KNOWN_ARCH))
  $(warning *)
  $(warning *)
  $(error Make quitting)
endif

# the root of the teem tree, as seen from the library subdirectories
# of the the "src" directory
ifndef TEEM_ROOT
  PREFIX = ../..
else
  PREFIX = $(TEEM_ROOT)
endif

# set directory-related variables: where to install things, as well as
# the -I and -L path flags
IDEST = $(PREFIX)/include
LDEST = $(PREFIX)/$(TEEM_ARCH)/lib
BDEST = $(PREFIX)/$(TEEM_ARCH)/bin
ODEST = $(PREFIX)/$(TEEM_ARCH)/obj
IPATH += -I$(IDEST)
LPATH += -L$(LDEST)

###
### effect the architecture-dependent settings by reading through the
### file specific to the chosen architecture
###
include ../make/$(ARCH).mk
###
###
###

ifdef LIB
  OBJ_PREF = $(ODEST)/$(LIB)
  OBJS = $(addprefix $(OBJ_PREF)/,$(LIBOBJS))
  LIB_BASENAME ?= lib$(LIB)
  _LIB.A = $(LIB_BASENAME).a
  _LIB.S = $(LIB_BASENAME).$(SHEXT)
  LIB.A = $(OBJ_PREF)/$(_LIB.A)
  LIB.S = $(OBJ_PREF)/$(_LIB.S)
endif

# the complete path names for headers and libraries to be installed
INSTALL_HDRS = $(addprefix $(IDEST)/,$(HEADERS))
INSTALL_LIBS = $(addprefix $(LDEST)/,$(_LIB.A) $(_LIB.S))

# the complete path names of the binaries to be installed.
# The fanciness here is that we want to allow the sources for the binaries
# to be in subdirectories of where the library's sources are, but those
# subdirectories can't be part of the installed binary name ...
INSTALL_BINS = $(addprefix $(BDEST)/,$(notdir $(BINS)))
# ... however, we need to set VPATH in order to help make find the 
# sources for the binaries in their subdirectories
VPATH = $(sort $(dir $(BINS)))

#
# flags.
#
ifeq ($(TEEM_LINK_SHARED),true)
  BIN_CFLAGS += $(SHARED_CFLAG)
  ifdef LIB
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

#
# Okay kids, here are the rules.
# 

# "make" ("make all") will make the bins and test bins in the current
# directory (or a subdirectory).  The potentially odd thing is that
# "make install" DOES NOT create the bins and test bins here and then
# install them, it just creates them where they belong.  The
# assumption here is that someone who wants to install the
# install-able things from this library can say "make install", and
# not "make; make install", while someone debugging/developing a
# library can say "make".
all: $(LIB.A) $(LIB.S) $(TEST_BINS) $(BINS)

# "make install" installs the headers, libraries, and binaries, but
# does not build the bins or test bins in the current directory
install: $(INSTALL_HDRS) $(INSTALL_LIBS) $(INSTALL_BINS)

# "make clean" removes what's created by "make" ("make all")
clean:
	$(RM) $(OBJS) $(LIB.A) $(LIB.S) $(TEST_BINS) $(BINS) $(OTHER_CLEAN)

# "make uninstall" removes what's created by "make install"
uninstall:
	$(if $(HEADERS), $(RM) $(foreach h, $(HEADERS), $(IDEST)/$(h)))
	$(if $(LIB), $(RM) $(LDEST)/$(_LIB.A) $(LDEST)/$(_LIB.S))
	$(if $(BINS), $(RM) $(foreach b, $(BINS), $(BDEST)/$(b)))

# "make destroy" does as you'd expect
destroy: clean uninstall

# all objects depend on all headers
$(OBJS): $(HEADERS) $(PRIV_HEADERS)

# NB: .o's are never created in the same directory as the source
$(OBJ_PREF)/%.o: %.c
	$(CC) $(CFLAGS) -I. $(IPATH) -c $< -o $@

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
	$(CC) $(CFLAGS) $(BIN_CFLAGS) -I. $(IPATH) -o $@ $< \
	$(THISLIB_LPATH) $(LPATH) $(BINLIBS)

# This rule is to satisfy the target $(INSTALL_HDRS)
$(IDEST)/%: %
	$(INSTALL) -m 644 $< $(IDEST)

# This rule is to satisfy the target $(INSTALL_LIBS)
$(LDEST)/%: $(OBJ_PREF)/%
	$(INSTALL) -m 755 $< $(LDEST)

# This rule is to satisfy the target $(INSTALL_BINS)
# The binaries which are to be installed should link against the
# already-installed libraries, and use the already installed headers.
#
# NB: VPATH is used to locate the prerequisite %.c in a subdirectory
#
$(BDEST)/%: %.c $(INSTALL_LIBS) $(INSTALL_HDRS)
	$(CC) $(CFLAGS) $(BIN_CFLAGS) $(IPATH) -o $@ $< $(LPATH) $(BINLIBS)
	$(CHMOD) 755 $@
