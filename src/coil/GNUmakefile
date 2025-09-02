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

#### Library name
####
####
L := coil
####
####
####

# boilerplate: set top-level paths and targets if not set, include top-level GNUmakefile,
# and guard reading of per-library variables and (via template.mk) rules
TeemRoot ?= ../..
TeemSrc ?= ..
ifndef DefaultTargets
  DefaultTargets := yes
  bild  : $(L).bild
  test  : $(L).test
  clean : $(L).clean
endif # ifndef DefaultTargets
include $(TeemSrc)/GNUmakefile
ifndef $(L).Included
$(L).Included := yes

#### Describe library L here
####
####
# (ell for macros)
$(L).Depends = ten ell nrrd biff air
$(L).PublicHdr = coil.h
$(L).PrivateHdr =
$(L).Obj = $(patsubst %.c,%.o, \
    coreCoil.c defaultsCoil.c enumsCoil.c methodsCoil.c realmethods.c scalarCoil.c \
    tensorCoil.c \
  )
$(L).Test = coiler
####
####
####

# boilerplate: enable "make test/T", template lib L rules, finish include guard
$(foreach T,$($(L).Test),$(eval test/$(T) : ../$(L)/test/$(T)))
include $(TeemSrc)/make/template.mk
endif # ifndef $(L).Included
