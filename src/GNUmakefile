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

####
#### top-level GNUmakefile: Master makefile for teem
####
#### NOTE: You should probably be using CMake instead of this GNU Makefile!  CMake builds
#### Teem by creating one monolithic library (libteem) and the executables (e.g. unu)
#### that link with that, which is very likely what you want.
####
#### This GNUmakefile logic was first written by GLK in ~2001, before CMake was useful or
#### popular. It builds Teem one library at a time (not a single monolithic library like
#### from CMake), and handles the per-library "test" programs (distinct from the nascent
#### efforts at using CTest).  Because this remains useful for debugging and developing
#### Teem, it persists, and GLK uses it.
####
#### For TeemV2, all the GNUMakefile machinery was simplified to remove any notion of
#### "architecture" and the environment variable "TEEM_ARCH" that declared it.  It was
#### never exactly the CPU architecture, but rather some mix of the architecture and the
#### OS around it.  And, it only made sense for settings where one (network) file system
#### was being used on a variety of different machines.  This was the case in where Teem
#### was first developed at University of Utah (Scientific Computing and Imaging group):
#### SGI, Linux (32-bit x86, ia64 and amd64), and Solaris hosts all talked on the same
#### filesystem.  Development builds needed to put binaries in different places depending
#### on platform, and this was determined by environment variable TEEM_ARCH, which in
#### turn chose an architecture-specific subdirectory of teem/src/arch to put things
#### in. Even if these kinds of heterogeneous environments still exist, Teem development
#### no longer happens there.  Also in the interests of simplicity: everything about
#### shared libraries was dropped.  The only shared library that matters is libteem.so or
#### libteem.dylib, and making that is better handled by CMake than by anything here.
####
#### This simplification meant removing:
#### (from everywhere)
#### env var TEEM_ARCH
#### env var TEEM_DEST (because now any "install" should be done via CMake)
#### make vars ARCH, SUBARCH
#### everything about shared libraries: TEEM_LINK_SHARED, SHEXT
#### as well as variables TEEM_EXT_IPATH and TEEM_EXT_LPATH (for all EXT externals
#### PNG ZLIB BZIP2 PTHREAD LEVMAR FFTW3, e.g TEEM_ZLIB_IPATH, TEEM_ZLIB_LPATH);
#### these have become EXT.DASHI and EXT.DASHL set in make/externals.mk
####
#### file errorCheck.mk gone (so simple, nothing left to error check)
#### make var KNOWN_ARCH
#### make funcs checkArchSet, checkArchValid,
####   checkArchLinux, checkArchLinux64, checkArchNetbsd64, checkArchDarwin
####
#### With this simplification, these files in make/ stopped being used:
####   aix.mk cygwin.mk darwin.mk irix6.mk linux.mk netbsd.mk solaris.mk win32.mk
#### and are thus removed.  Instead, a new single make/arch.mk contains the
#### information that used to be there, or at least variables from which to hang
#### this info.

ifndef Top.Included
Top.Included := yes

## ".SUFFIXES :" speeds debugging with "make -d" (and probably make in general) by
## eliminating the list of filename extensions (.y, .l, etc) checked by implicit pattern
## rules.
##
.SUFFIXES :

# avoids "make -d" perseverating with "Trying implicit prerequisite `....w'."
# (.w files are for Knuth's cweave/ctangle)
MAKEFLAGS += -r

## TeemRoot: a relative path to directory containing "src", "include", "built" subdirs
## Whether make started on this GNUmakefile, or that of a subdirectory, determines who
## gets to set TeemRoot first. Same for TeemSrc, a relative path to the "src" subdir
##
TeemRoot ?= ..
TeemSrc ?= .

## Enstate the architecture-dependent settings by reading through the file specific to
## the chosen architecture
##
## We assume that arch.mk will finish with these all being set:
## CC, LD, AR, ARFLAGS, RANLIB, RM, CP, CHMOD, CFLAGS, BIN_CFLAGS
##
include $(TeemSrc)/make/arch.mk

## Now that CMake-based compilation is the default, the (GNUmake) non-CMake building has
## to a little extra work to keep working: this extra -DTEEM_NON_CMAKE flag says to $(CC)
## "we're not building with cmake" (so e.g. don't look for teem/airExistsConf.h)
CFLAGS += -DTEEM_NON_CMAKE

## information about optional external libraries to link with
##
include $(TeemSrc)/make/externals.mk

## Libs: all the teem libraries we'll try to build
## DepNums: indices numbering all the libs, in *Dep*endency order
## RevNums: indices for numbering libs in (reverse) order for linking
## DepLibs, RevLibs: strings of N/L (number/library) pairs, in either order
## (TEEM_LIB_LIST)
##
Libs := air hest biff nrrd ell moss unrrdu alan tijk gage dye bane limn echo hoover seek ten elf pull coil push mite meet
DepNums := 0 1 2 3 4 5 6 7 8 9 A B C D E F G H I J K L M
ifneq ($(words $(Libs)),$(words $(DepNums)))
  $(error |Libs| == $(words $(Libs)) != $(words $(DepNums)) == |DepNums|)
endif
ifneq ($(DepNums),$(sort $(DepNums)))
  $(error Not already sorted? DepNums == $(DepNums))
endif
# https://stackoverflow.com/questions/52674/simplest-way-to-reverse-the-order-of-strings-in-a-make-variable
reverse = $(if $(1),$(call reverse,$(wordlist 2,$(words $(1)),$(1)))) $(firstword $(1))
RevNums := $(strip $(call reverse,$(DepNums)))
DepLibs := $(join $(DepNums),$(Libs:%=/%))
RevLibs := $(join $(RevNums),$(Libs:%=/%))

## Modes: the purpose of running make:
##   bild = compiling/building/making something from source files
##   test = (depends on bild) builds intra-library "test" programs
##   clean = removing whatever bild and test does
## Declaring all these modes as phony saves make the effort of trying implicit rules to
## update them.
.PHONY : bild test clean
Modes := bild test clean
# expands to: teem.bild teem.test teem.clean air.bild air.test air.clean ... bin.bild bin.test bin.clean
# (even though bin.test is never actually a target)
.PHONY : $(foreach L,teem $(Libs) bin,$(Modes:%=$(L).%))

## Top-level default rules.  If make started with this file, then these will be the first
## rules that make sees, so that "make" defaults to "make teem.bild" If make started in
## subdirectory for library L, then default target "bild" will depend on L.bild, and
## "clean" will depend on L.clean.
##
ifndef DefaultTargets
  DefaultTargets := yes
  bild  : teem.bild
  # kind of a crazy (test are for helping write one library) but allowed
  test  : teem.test
  clean : teem.clean
endif

## Top-level rules, available regardless of where make started:
## teem.M (for mode M) depends on X.M for all libs X and on bin.M
##
teem.bild  : $(addsuffix .bild,$(Libs) bin)
teem.test  : $(addsuffix .test,$(Libs)) # there is no bin.test
teem.clean : $(addsuffix .clean,$(Libs) bin)

## if.missing(path): returns "missing" if path does not exist
##
if.missing = $(if $(wildcard $(1)),,missing)

## create.if.missing(dir): creates directory dir if it doesn't exist,
## and gives a warning to that effect
##
create.if.missing = \
  $(if $(call if.missing,$(1)),\
    $(warning *** WARNING ***: Creating needed directory $(1)) \
      $(shell mkdir $(1)))

## Set variables connected to directories where things go
## ObjPath: where the .o object files go
## HdrPath: where .h header files go (for #include <>)
## LibPath: where the .a library files go
## BinPath: where executables go
ObjPath := $(TeemRoot)/built/obj
HdrPath := $(TeemRoot)/built/include
LibPath := $(TeemRoot)/built/lib
BinPath := $(TeemRoot)/built/bin
$(foreach dir,$(ObjPath) $(HdrPath) $(HdrPath)/teem $(LibPath) $(BinPath),\
   $(call create.if.missing,$(dir)))
dashI += -I$(HdrPath)
# we still need this for the teem*.h headers (such as teemPng.h)
dashI += -I$(TeemRoot)/include
dashL += -L$(LibPath)

#######################################
## "Functions": The bread and butter of how template.mk works. Each of these can be
## $(call)ed with a library name, as the one and only argument, in order to get a list of
## files or flags related to library.
##
## NOTE: In all of these, "File" could be singular or plural
## SrcHdrFile(L): paths to all (public and private) headers in souce for lib L
## HdrFile(Ls): paths to all public headers "installed" for lib L in Ls
## ObjFile(L): paths to all object files to combine to make library L
## LibFile(L): path to .a library file "installed" for library L
## TestFile(L): paths to debugging tests for library L, built in-place
##
SrcHdrFile = $(addprefix $(TeemSrc)/$(1)/,$($(1).PublicHdr) $($(1).PrivateHdr))
HdrFile = $(foreach lib,$(1),$(addprefix $(HdrPath)/teem/,$($(lib).PublicHdr)))
ObjFile = $(addprefix $(ObjPath)/,$($(1).Obj))
LibFile = $(LibPath)/lib$(1).a
TestFile = $(addprefix $(TeemSrc)/$(1)/test/,$($(1).Test))

## Need(L): All the libraries that L needs. L's GNUmakefile declares its immediate
## dependencies in $(L).Depends, but Need(L) is the recursive-ish expansion.
## MeNeed(L): L and Need(L), in dependency order
##
## Yes, the whole point of "make" is to keep track of dependencies, so this would seem to
## be a little pointless. But the issue is linking with -llib for every library lib: how
## do you assemble the complete link command?  It needs to be the transitive closure of
## the libs depended on.  I (GLK) can't figure out how to do this implicitly via make,
## hence the stuff below.  Because I can't figure out how to do fixed-point determination
## as part of a recursive function call, I'm only doing a few levels of explicit
## prerequisite expansion. Each level is done by "dmnl" (discover more needed libs).  The
## number of levels is more than enough for current Teem; adding more levels later is
## trivial.
##
## Since we rely on $(sort) to remove redundancies, we need a way of putting the
## libraries back in dependency order (!= lexical order).  So, we prefix the needed
## library names with 0/,1/,...,9/,A/,B/,... (via $(join)) to create DepLibs (above),
## re-sort them, and then lose the prefix with $(notdir).  This is done by "DepOrder". On
## the link line, however, the ordering needs to be reversed, this is done by "RevOrder".
##
dmnl = $(sort $(foreach lib,$(1),$(lib) $($(lib).Depends)))
DepOrder = $(notdir $(sort $(foreach lib,$(1),$(filter %/$(lib),$(DepLibs)))))
RevOrder = $(notdir $(sort $(foreach lib,$(1),$(filter %/$(lib),$(RevLibs)))))
Need = $(call DepOrder,\
$(call dmnl,\
$(call dmnl,\
$(call dmnl,\
$(foreach lib,$(1),$($(lib).Depends))))))
MeNeed = $(call DepOrder,$(1) $(call Need,$(1)))

## More utilities
## llink(LIBS): "-lL" for all L in LIBS, in correct link order
## banner(L): progress indication for compiling library L; used in template.mk.
##
llink = $(foreach L,$(call RevOrder,$(1)),-l$(L))
banner = echo -e "\n-------" $(1) "-------" $(1) "-------" $(1) "-------" $(1) "-------\n"

## For optional external libraries we can depend on (e.g. png, zlib, levmar)
## Read make/externals.mk for context and full list AllExterns
##
## These two reflect the circumstances of running make, not any library:
## wantsExtern(X): returns X if TEEM_<X> has been set (as a shell variable,
##   but not set to "0"), otherwise nothing
## Externs: list of all wanted externs (a subset of AllExterns)
## forExtern(libs,patt): for all external X in Externs,
##   and for all L in libraries $(libs) that benefit from external X,
##   replace "XXX" in $(patt) with X
## Externs.dashD: the compile-time -D flags needed to enable external
##   library X when compiling objects in libraries $(libs)
## evallist(list): utility to evaluate each element of list as a variable
## (in the following, $(libs) which should probably be MeNeed(L) for some lib L)
## Externs.dashI(libs): the external-specific -I flags needed for compiling
##   objects in libraries $(libs)
##   (using the X.dashI set in make/externals.mk for external X)
## Externs.dashL(libs): the external-specific -L flags needed for compiling
##   executables that uses libraries $(libs)
##   (using the X.dashL set in make/externals.mk for external X)
## Externs.llink(libs): the external-specific -l flags needed to link an
##   executable that uses libraries $(libs)
##   (using the X.llink set in make/externals.mk for external X)
##
# if TEEM_X is undefined, then nothing, else if 0 matches TEEM_X then nothing, else X
wantsExtern = $(if $(findstring undefined,$(origin TEEM_$(1))),\
  ,\
  $(if $(filter 0,$(TEEM_$(1))),\
    ,\
    $(1)))
Externs = $(strip $(foreach X,$(AllExterns),$(call wantsExtern,$(X))))
# foreach X in Externs and L in libs (only using $(L).Externs if defined)
forExtern = $(strip $(foreach X,$(Externs),$(foreach L,$(1),\
  $(if $(findstring undefined,$(origin $(L).Externs)),\
   ,\
   $(if $(filter $(X),$($(L).Externs)),\
     $(subst XXX,$(X),$(2)))))))
Externs.dashD = $(call forExtern,$(1),-DTEEM_XXX=1)
evallist = $(foreach var,$(1),$($(var)))
maybe.prefix = $(if $(2),$(1)$(2))
Externs.dashI = $(call maybe.prefix,-I,$(call evallist,$(call forExtern,$(1),TEEM_XXX_DASHI)))
Externs.dashL = $(call maybe.prefix,-L,$(call evallist,$(call forExtern,$(1),TEEM_XXX_DASHL)))
Externs.llink  = $(call evallist,$(call forExtern,$(1),XXX.llink))

## DEBUG
#X = PNG
#$(warning wantsExtern($(X))=$(call wantsExtern,$(X)))
#$(warning Externs=$(Externs))
#$(warning forExtern(nrrd ten air tijk,-DTEEM_XXX=1): $(call forExtern,nrrd ten air tijk,-DTEEM_XXX=1))
#L = ten
#$(warning Externs.dashI($(L)): $(call Externs.dashI,$(L)))
#$(warning Externs.dashL($(L)): $(call Externs.dashL,$(L)))
#$(warning Externs.llink($(L)): $(call Externs.llink,$(L)))
#$(error STOPPING)
# TEEM_PTHREAD= TEEM_PNG= TEEM_LEVMAR=1 make --warn-undefined-variables

#######################################

## Read in the makefiles for all the libraries, and the bins.  Run-away recursive
## inclusion is prevented by the Top.Included and other guards.  Note: "include" is a
## directive, not a function, which eliminates the possibility of iterating through the
## libraries, reading the make file, and then setting variables based on what was just
## read.
##
## Unfortunate trickiness: Lsave preserves the value of L, in case we're being included
## from a lower-level GNUmakefile which set a value for L.  If we didn't put L back the
## way it was, L would have to be set twice in the lower level.
##
ifdef L
  ifndef Lsave
    # "It's such a fine line, between stupid ...
    Lsave := $(L)
  endif
endif
include $(foreach LIB,$(Libs) bin,$(TeemSrc)/$(LIB)/GNUmakefile)
ifdef Lsave
  # ...  and clever"
  L := $(Lsave)
endif

endif # ifndef Top.Included
