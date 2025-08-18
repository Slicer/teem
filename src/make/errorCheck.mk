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
#### errorCheck.mk: checks on validity of the variables that make uses
####

## Other error checking ...
##
checkShext = $(if $(filter undefined,$(origin TEEM_LINK_SHARED)),,\
$(if $(TEEM_SHEXT),,\
$(warning *)\
$(warning *)\
$(warning * Cannot do shared library linking with TEEM_SHEXT unset)\
$(warning * Set it in teem/make/arch.mk or as environment var)\
$(warning *)\
$(warning *)\
$(error Make quitting)))

checkTeemDest = $(if $(TEEM_DEST),\
$(if $(filter-out /%,$(TEEM_DEST)),\
$(warning *)\
$(warning *)\
$(warning * TEEM_DEST must be an absolute path (not $(TEEM_DEST)))\
$(warning *)\
$(warning *)\
$(error Make quitting)\
),)
