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
L := gage
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
$(L).Depends = ell nrrd biff air
$(L).PublicHdr = gage.h
$(L).PrivateHdr = privateGage.h
# NOT including multiGage.c for now, and scl3pfilterbody.c is just a macro
$(L).Obj = $(patsubst %.c,%.o, \
    ctx.c deconvolve.c defaultsGage.c filter.c kind.c miscGage.c \
    optimsig.c print.c pvl.c scl.c sclanswer.c sclfilter.c \
    sclprint.c shape.c st.c stack.c stackBlur.c twovecGage.c update.c vecGage.c \
    vecprint.c \
  )
$(L).Test = ctfix demo vh aalias indx genoptsig ssc maxes tplot
####
####
####

# boilerplate: enable "make test/T", template lib L rules, finish include guard
$(foreach T,$($(L).Test),$(eval test/$(T) : ../$(L)/test/$(T)))
include $(TeemSrc)/make/template.mk
endif # ifndef $(L).Included
