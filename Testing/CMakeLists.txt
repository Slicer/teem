#
# Teem: Tools to process and visualize scientific data and images              
# Copyright (C) 2008, 2007, 2006, 2005  Gordon Kindlmann
# Copyright (C) 2004, 2003, 2002, 2001, 2000, 1999, 1998  University of Utah
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public License
# (LGPL) as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
# The terms of redistributing and/or modifying this software also
# include exceptions to the LGPL that facilitate static linking.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this library; if not, write to Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
#

PROJECT(TEEMTesting)

IF(Teem_BINARY_DIR)
  # We are building inside the tree
  IF(NOT EXECUTABLE_OUTPUT_PATH)
    SET (EXECUTABLE_OUTPUT_PATH ${Teem_BINARY_DIR} CACHE PATH
         "Single output directory for building all testing executables.")
    SET(C_TEST_PATH ${Teem_BINARY_DIR})
  ENDIF(NOT EXECUTABLE_OUTPUT_PATH)
ENDIF(Teem_BINARY_DIR)

ADD_TEST(nrrd.Sanity ${EXECUTABLE_OUTPUT_PATH}/nrrdSanity)

ADD_SUBDIRECTORY(nrrd)
ADD_SUBDIRECTORY(biff)
