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
#### contain the name of the library for which we're creating rules
#### (this is set by the library GNUmakefile)
####
####

## Apparently unfortunately necessary cleverness.  In a rule, the
## contexts of the target and the prerequisite are immediate, the
## contexts of the commands are deferred; there is no getting around
## this.  Thus, if the commands to satisfy $(L)/clean include $(RM)
## $(call OBJS.DEV,$(L)), then this will remove the object files for
## library $(L), but the value of $(L) that is used is the one in
## effect with WHEN THE COMMAND IS RUN, not the one when the rule was
## read by make.  Not useful.
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
## The real value of the .%.usable target, in fact, is to have
## something off which to hang this assignment, so that, for instance,
## the right MORE_CFLAGS is used when compiling sources for a library
## that we need (say, air), but which isn't in fact that top-level
## target (say, nrrd).
##
$(L)/% : _L := $(L)
$(TEEM_SRC)/.$(L).usable : _L := $(L)

## These approximate the messages I had in the previous version of the
## teem makefiles, giving some progress indication in terms of what
## library we're working on.  Since we want this to be printed out
## before we do the work of making the target, we need a double colon
## rule preceeding all the others.  But it 
##

## $(L)/usable are the things that other teem libraries may rely on
##
$(L)/usable : $(TEEM_SRC)/.$(L).usable

## The prequisites of .$(L).usable are the .usables of all our
## prerequisites, and our own libs and headers, if either:
## 1) any of the libs and headers are missing, or
## 2) a prerequisite .usable is newer than ours.
## Naming our libs and headers should effectively trigger an install.
##
used := $(call USED.INST,$(L))
me := $(TEEM_SRC)/.$(L).usable
need := $(call NEED.USABLE,$(L))
$(TEEM_SRC)/.$(L).usable : $(call NEED.USABLE,$(L)) \
$(if $(call MISSING,$(used)),$(used),$(if $(call NEWER.THAN,$(me),$(need)),$(used)))

## $(L)/install depends on usable prerequisite libraries and $(L)'s
## installed libs and headers.
##
$(L)/install : $(call NEED.USABLE,$(L)) \
  $(call LIBS.INST,$(L)) $(call HDRS.INST,$(L))

## $(L)/dev depends on usable prerequisites and $(L)'s local
## development builds of the libs and tests
##
$(L)/dev : $(call NEED.USABLE,$(L)) \
  $(call LIBS.DEV,$(L)) $(call TESTS.DEV,$(L))

## $(L)/clean undoes $(L)/dev.
##
$(L)/clean :
	$(RM) $(call OBJS.DEV,$(_L)) $(call LIBS.DEV,$(_L)) \
	  $(call TESTS.DEV,$(_L))

## $(L)/clobber undoes $(L)/install.
##
$(L)/clobber : $(L)/clean
	$(RM) $(call LIBS.INST,$(_L)) $(call HDRS.INST,$(_L)) \
	  $(TEEM_SRC)/.$(_L).usable


## The objects of a lib depend on usable prerequisite libraries (for
## their headers specifically), and on our own headers.
##
$(call OBJS.DEV,$(L)) : $(call NEED.USABLE,$(L)) $(call HDRS.DEV,$(L))

## Development tests depend on usable prerequiste libraries, and the
## development libs (header dep. through objects, source dep. below)
##
$(call TESTS.DEV,$(L)) : $(call NEED.USABLE,$(L)) $(call LIBS.DEV,$(L))

## How to create development static and shared libs (LIBS.DEV) from
## the objects on which they depend.
##
$(ODEST)/lib$(L).a : $(call OBJS.DEV,$(L))
	$(AR) $(ARFLAGS) $@ $^
ifdef SHEXT
$(ODEST)/lib$(L).$(SHEXT) : $(call OBJS.DEV,$(L))
	$(LD) -o $@ $(LDFLAGS) $(LPATH) $^
endif

## MAYBEBANNER(L)(obj) returns "echo ..." to show a library banner 
## progress indicator, but only if obj is the first object in $(L).OBJS.
## This mimics the behavior under the old recursive teem makefile.
##
MAYBEBANNER.$(L) = $(if $(filter $(notdir $(1:.c=.o)),$(word 1,$($(_L).OBJS))),$(call BANNER,$(_L)))

## How to compile a .o file. We're giving a pattern rule constrained
## to the objects we know we need to make for this library.  Or, we
## could use vpath to locate the sources in the library subdirectory,
## but why start cheating now.
##
$(call OBJS.DEV,$(L)) : $(ODEST)/%.o : $(TEEM_SRC)/$(L)/%.c
	@$(call MAYBEBANNER.$(_L),$<)
	$(P) $(CC) $(CFLAGS) $(call MORE_CFLAGS,$(_L)) $(IPATH) -c $< -o $@

## How to make development tests.  It doesn't actually matter in this
## case where the source files are, we just put the executable in the
## same place.
##
$(call TESTS.DEV,$(L)) : % : %.c
	$(P) $(CC) $(CFLAGS) $(BIN_CFLAGS) $(call MORE_CFLAGS,$(_L)) \
	  $(IPATH) -o $@ $< -L$(ODEST) -l$(_L) \
	  $(LPATH) $(call NEED.LIBLINKS,$(_L)) -lz -lm

## Since the installed libs and headers are the "usables", we have to
## touch .$(_L).usable every time any one of them is updated (since we
## don't always know which ones are going to be updated)

## How to install a libs (LIB.INST), static and shared: This really
## should be in the top-level GNUmakefile, since there is really
## nothing library specific about this, but it looked funny to just
## have one rule there and all the rest in here.  In order to prevent
## redefining rules for the same target ($(LDEST)/%.a), we
## artificially make the rule specific to the library with a static
## pattern rule.
##
$(LDEST)/lib$(L).a : $(LDEST)/%.a : $(ODEST)/%.a
	$(CP) $< $@; $(CHMOD) 644 $@
	$(if $(SIGH),$(SLEEP) $(SIGH); touch $@)
	$(if $(SIGH),$(SLEEP) $(SIGH))
	touch $(TEEM_SRC)/.$(_L).usable
ifdef SHEXT
$(LDEST)/lib$(L).$SHEXT) : $(LDEST)/%.$(SHEXT) : $(ODEST)/%.$(SHEXT)
	$(CP) $< $@; $(CHMOD) 755 $@
	$(if $(SIGH),$(SLEEP) $(SIGH); touch $@)
	$(if $(SIGH),$(SLEEP) $(SIGH))
	touch $(TEEM_SRC)/.$(_L).usable
endif

## How to install headers: another instance where vpath could simplify
## things, but why bother.
##
$(call HDRS.INST,$(L)) : $(IDEST)/%.h : $(TEEM_SRC)/$(L)/%.h
	$(CP) $< $@; $(CHMOD) 644 $@
	$(if $(SIGH),$(SLEEP) $(SIGH); touch $@)
	$(if $(SIGH),$(SLEEP) $(SIGH))
	touch $(TEEM_SRC)/.$(_L).usable
