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
#### template.mk: Defines rules which have the same structure for each
#### library, but which refer to the specific consituents and
#### prerequisites of the library.  The variable L is assumed to
#### contain the name of the library for which we're creating rules;
#### L is an immediate set by the library GNUmakefile.
####
####

## Either to avoid extra calls to need, or extra uses of call
## $(L).need: recursively expanded version of $(L).NEED
## $(L).need.usable: X/usable for all X needed for L
## $(L).{hdrs,libs}.inst: full path to installed headers/libs for L
## Keep in mind that these will be set for all libraries before ANY
##   command's rules are executed
##
$(L).need := $(call need,$(L))
$(L).meneed := $(L) $($(L).need)
$(L).need.usable := $(call usable,$($(L).need))
$(L).need.hdrs.inst := $(call hdrs.inst,$($(L).need))
$(L).need.libs.inst := $(call libs.inst,$($(L).need))
$(L).need.links := $(call link,$($(L).need))
$(L).hdrs.inst := $(call hdrs.inst,$(L))
$(L).libs.inst := $(call libs.inst,$(L))
$(L).hdrs.dev := $(call hdrs.dev,$(L))
$(L).libs.dev := $(call libs.dev,$(L))
$(L).objs.dev := $(call objs.dev,$(L))
$(L).tests.dev := $(call tests.dev,$(L))
$(L).more.cflags := $(call more.cflags,$(L))
$(L).ext.ipath := $(call ext.ipath,$($(L).meneed))
$(L).ext.lpath := $(call ext.lpath,$($(L).meneed))
$(L).ext.link := $(call ext.link,$($(L).meneed))
$(L).ext.Dflag := $(call ext.Dflag,$($(L).meneed))

## In a rule, the contexts of the target and the prerequisite are
## immediate, the contexts of the commands are deferred; there is no
## getting around this.  Thus, if the commands to satisfy $(L)/clean
## include $(RM) $(call OBJS.DEV,$(L)), then this will remove the
## object files for library $(L), but the value of $(L) that is used
## is the one in effect with WHEN THE COMMAND IS RUN, not the one when
## the rule was read by make.  Not useful.
##
## For all the "entry-point" targets, we enstate a pattern-specific
## immediate variable value _L.  This bridges the immediate and
## deferred stages by remembering the value of L at the time the rule
## was read, so that it can be used in the deferred context of the
## rule's commands.  Fortunately, the sementics of pattern-specific
## variables mean that the value of _L will be set the same when
## satisfying all prerequisites of $(L)/%, which is exactly what we
## want.
##
$(L)/% : _L := $(L)
$($(L).hdrs.inst) : _L := $(L)
$($(L).objs.dev) : _L := $(L)

## The prequisites of .$(L).usable are the .usable's of all our
## prerequisites, and our own libs and headers, if either:
## 1) any of the libs and headers are missing, or
## 2) a prerequisite .usable is newer than ours.
## Naming our libs and headers should effectively trigger an install.
##

ifneq (undefined,$(origin TEEM_USABLE))
$(IDEST)/.$(L).hdr: $(call if.missing,$($(L).hdrs.inst))
$(LDEST)/.$(L).lib: $(call newer.than,$($(L).libs.inst),$($(L).need.hdrs.inst))
endif

## $(L)/install depends on usable prerequisite libraries and $(L)'s
## installed libs and headers.
##
$(L)/install : $(call used,$($(L).need)) \
  $($(L).libs.inst) $($(L).hdrs.inst)

## $(L)/dev depends on usable prerequisites and $(L)'s local
## development builds of the libs and tests
##
$(L)/dev : $(call used,$($(L).need)) \
  $($(L).libs.dev) $($(L).tests.dev)

## $(L)/clean undoes $(L)/dev.
##
$(L)/clean :
	$(RM) $($(_L).objs.dev) $($(_L).libs.dev) $($(_L).tests.dev)

## $(L)/clobber undoes $(L)/install.
##
$(L)/clobber : $(L)/clean
	$(RM) $($(_L).libs.inst) $($(_L).hdrs.inst)
	$(RM) $(IDEST)/.$(_L).hdr $(LDEST)/.$(_L).lib

## The objects of a lib depend on usable prerequisite libraries (for
## their headers specifically), and on our own headers.
##
$($(L).objs.dev) : $(call used.hdrs,$($(L).need)) $($(L).hdrs.dev)

## Development tests depend on usable prerequiste libraries, and the
## development libs (header dep. through objects, source dep. below)
##
$($(L).tests.dev) : $(call used,$($(L).need)) \
  $($(L).hdrs.dev) $($(L).libs.dev)

## How to create development static and shared libs (libs.dev) from
## the objects on which they depend.
##
$(ODEST)/lib$(L).a : $($(L).objs.dev)
	$(AR) $(ARFLAGS) $@ $^
ifdef TEEM_SHEXT
$(ODEST)/lib$(L).$(TEEM_SHEXT) : $($(L).objs.dev)
	$(LD) -o $@ $(LDFLAGS) $(LPATH) $^
endif

## maybebanner(L)(obj) returns "echo ..." to show a library banner 
## progress indicator, but only if obj is the first object in $(L).OBJS.
## This mimics the behavior under the old recursive teem makefile.
##
maybebanner.$(L) = $(if $(filter $(notdir $(1:.c=.o)),\
$(word 1,$($(_L).OBJS))),$(call banner,$(_L)))

## How to compile a .o file. We're giving a pattern rule constrained
## to the objects we know we need to make for this library.  Or, we
## could use vpath to locate the sources in the library subdirectory,
## but why start cheating now.
##
$($(L).objs.dev) : $(ODEST)/%.o : $(TEEM_SRC)/$(L)/%.c
	@$(call maybebanner.$(_L),$<)
	$(P) $(CC) $(CFLAGS) $($(_L).more.cflags) \
	  $($(_L).ext.Dflag) $($(_L).ext.ipath) $(IPATH) -c $< -o $@

## How to make development tests.  It doesn't actually matter in this
## case where the source files are, we just put the executable in the
## same place.
##
$($(L).tests.dev) : % : %.c
	$(P) $(CC) $(CFLAGS) $(BIN_CFLAGS) \
	  $(call more.cflags,$(_L)) $(IPATH) -o $@ $< -L$(ODEST) -l$(_L) \
	  $(LPATH) $($(_L).need.links) $($(_L).ext.lpath) $($(_L).ext.link) -lm

## How to install a libs (libs.inst), static and shared: This really
## should be in the top-level GNUmakefile, since there is really
## nothing library specific about this, but it looked funny to just
## have one rule there and all the rest in here.  In order to prevent
## redefining rules for the same target ($(LDEST)/%.a), we
## artificially make the rule specific to the library with a static
## pattern rule.
##
$(LDEST)/lib$(L).a : $(LDEST)/% : $(ODEST)/%
	$(CP) $< $@; $(CHMOD) 644 $@
	$(if $(SIGH),$(SLEEP) $(SIGH); touch $@)
ifneq (undefined,$(origin TEEM_USABLE))
	$(if $(SIGH),$(SLEEP) $(SIGH))
	touch $(LDEST)/.$(_L).lib
endif
ifdef TEEM_SHEXT
  $(LDEST)/lib$(L).$(TEEM_SHEXT) : $(LDEST)/% : $(ODEST)/%
	$(CP) $< $@; $(CHMOD) 755 $@
	$(if $(SIGH),$(SLEEP) $(SIGH); touch $@)
ifneq (undefined,$(origin TEEM_USABLE))
	$(if $(SIGH),$(SLEEP) $(SIGH))
	touch $(LDEST)/.$(_L).lib
endif
endif

## How to install headers: another instance where vpath could simplify
## things, but why bother.
##
$($(L).hdrs.inst) : $(IDEST)/%.h : $(TEEM_SRC)/$(L)/%.h
	$(CP) $< $@; $(CHMOD) 644 $@
	$(if $(SIGH),$(SLEEP) $(SIGH); touch $@)
ifneq (undefined,$(origin TEEM_USABLE))
	$(if $(SIGH),$(SLEEP) $(SIGH))
	touch $(IDEST)/.$(_L).hdr
endif
