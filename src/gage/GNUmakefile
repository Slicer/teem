#
# Teem: Tools to process and visualize scientific data and images
# Copyright (C) 2006, 2005  Gordon Kindlmann
# Copyright (C) 2004, 2003, 2002, 2001, 2000, 1999, 1998  University of Utah
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

#### Library name
####
####
L := gage
####
####
####

# boilerplate: default targets and include tricks
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

#### Describe library here
####
####
$(L).NEED = ell nrrd biff air
$(L).PUBLIC_HEADERS = gage.h
$(L).PRIVATE_HEADERS = privateGage.h
$(L).OBJS = defaultsGage.o miscGage.o scl.o kind.o \
        shape.o pvl.o update.o deconv.o \
	print.o sclanswer.o sclprint.o sclfilter.o \
	vecGage.o vecprint.o st.o filter.o ctx.o 
$(L).TESTS = test/ctfix test/demo
####
####
####

# boilerplate: declare rules for this library
include $(TEEM_SRC)/make/template.mk
endif
ifeq (,$(INCLUDED))
  include $(TEEM_SRC)/bin/GNUmakefile
endif
