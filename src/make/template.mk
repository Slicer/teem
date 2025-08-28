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
#### template.mk: Defines rules which have the same structure for each library, but which
#### refer to the specific constituents and prerequisites of the library.  The rules
#### defined here are effectively templated on the name of the library.  The variable L
#### is assumed to contain the name of the library for which we're creating rules; L is
#### an immediate set by the library GNUmakefile.
####

## Avoid redundant expensive calls evaluating and saving these once
##
$(L).Need := $(call Need,$(L))
$(L).MeNeed := $(call MeNeed,$(L))
$(L).MeNeedFile := $(foreach lib,$($(L).MeNeed),$(call LibFile,$(lib)) $(call HdrFile,$(lib)))
#$(warning template($(L)): $(L).Need = |$($(L).Need)|)
#$(warning template($(L)): $(L).MeNeed = |$($(L).MeNeed)|)
#$(warning template($(L)): $(L).MeNeedFile = |$($(L).MeNeedFile)|)

## In a rule, the contexts of the target and the prerequisite are immediate, the contexts
## of the commands are deferred; there is no getting around this.  Thus, if the commands
## to satisfy $(L)/clean include $(RM) $(call foo,$(L)), the value of $(L) passed to foo
## is the one in effect with WHEN THE COMMAND IS RUN, not the one when the rule was read
## by make.  Not useful.
##
## For all the phony entry-point targets, we enstate a pattern-specific immediate
## variable value _L.  This bridges the immediate and deferred stages by remembering the
## value of L at the time the rule was read, so that it can be used in the deferred
## context of the rule's commands when run.  Fortunately, the sementics of
## pattern-specific variables mean that the value of _L will be set the same when
## satisfying all prerequisites of $(L).%, which is exactly what we want.
##
$(L).% : _L := $(L)

## added this to enable _L when the target is not explicitly a library but, for example,
## an individually named binary, which will depend on the .a library file.  This may
## subsume the definition above, but it doesn't hurt to have both.
##
$(call LibFile,$(L)) : _L := $(L)
$(call HdrFile,$(L)) : _L := $(L)
$(call ObjFile,$(L)) : _L := $(L)
$(call TestFile,$(L)): _L := $(L)

## Here are the actual rules for building!

## $(L).bild depends on the library and header files, in their built location, for me (L)
## and for everything I (transitively) depend on.  Rules below will describe how my
## library and header files are created; prior libraries running this template.mk will do
## the same for them.
##
#$(warning $(L).bild : $($(L).MeNeedFile))
$(L).bild : $($(L).MeNeedFile)

## $(L).test depends on all the per-lib test programs
## which in turn depend on first building L (and everything that it depends on),
#$(warning $(L).test : $(call TestFile,$(L)))
$(L).test : $(call TestFile,$(L))
#$(warning $(call TestFile,$(L)) : $($(L).MeNeedFile)
$(call TestFile,$(L)) : $($(L).MeNeedFile)

## $(L).clean undoes $(L).bild and $(L).test
##
$(L).clean :
	$(RM) $(call ObjFile,$(_L)) $(call LibFile,$(_L)) $(call HdrFile,$(_L)) $(call TestFile,$(_L))
ifdef LITTER
	$(RM) -r $(foreach bin,$(call TestFile,$(_L)),$(bin)$(LITTER))
endif

## The objects of a lib depend on the headers of the libraries we
## depend on (both directly and indirectly, or else ABI mismatches,
## no?), and on our own headers.
##
#$(warning [ObjFile(L)] $(call ObjFile,$(L)) : ...)
#$(warning [ObjFile(L)] ... : $(call HdrFile,$($(L).Need)) $(call SrcHdrFile,$(L)))
$(call ObjFile,$(L)) : $(call HdrFile,$($(L).Need)) $(call SrcHdrFile,$(L))

## (L).maybebanner(obj) returns "echo ..." to show a library banner progress indicator,
## but only if obj is the first object in $(L).OBJS.
##
$(L).maybebanner = $(if $(filter $(notdir $(1:.c=.o)),\
                                 $(word 1,$($(_L).Obj))),\
  $(call banner,$(_L)))

## How to compile a .o file. We're giving a pattern rule constrained to the objects we
## know we need to make for this library.  Or, we could use vpath to locate the sources
## in the library subdirectory, but why start cheating now.
##
#$(warning $(call ObjFile,$(L)) : $(ObjPath)/%.o : $(TeemSrc)/$(L)/%.c)
$(call ObjFile,$(L)) : $(ObjPath)/%.o : $(TeemSrc)/$(L)/%.c
	@$(call $(_L).maybebanner,$<)
	$(CC) $(CFLAGS) $(dashI) $(call Externs.dashD,$(_L)) $(call Externs.dashI,$(_L)) \
	  -c $< -o $@
#$(warning CC -c $< -o $@)

## How to create static lib from the constituent object files
##
#$(warning $(call LibFile,$(L)) : $(call ObjFile,$(L)))
$(call LibFile,$(L)) : $(call ObjFile,$(L))
	$(AR) $(ARFLAGS) $@ $^
	$(if $(RANLIB),$(RANLIB) $@,)
#$(warning $(AR) $(ARFLAGS) $@ $^)

## How to "build" headers: just copy them
##
#$(warning $(call HdrFile,$(L)) : $(HdrPath)/teem/%.h : $(TeemSrc)/$(L)/%.h)
$(call HdrFile,$(L)) : $(HdrPath)/teem/%.h : $(TeemSrc)/$(L)/%.h
	$(CP) $< $@; $(CHMOD) 644 $@

## How to build development tests.  It doesn't actually matter in this case where the
## source files are, we just put the executable in the same place.
##
#$(warning $(call TestFile,$(L)) : % : %.c)
$(call TestFile,$(L)) : % : %.c
	$(CC) $(CFLAGS) $(BIN_CFLAGS) \
	  $(dashI) -o $@ $< \
	  $(dashL) $(call llink,$($(_L).MeNeed)) \
	  $(call Externs.dashL,$($(_L).MeNeed)) \
	  $(call Externs.llink,$($(_L).MeNeed)) -lm
