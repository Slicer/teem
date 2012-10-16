#
# Teem: Tools to process and visualize scientific data and images              
# Copyright (C) 2012, 2011, 2010, 2009  University of Chicago
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

ADD_EXECUTABLE(test_enmall enmall.c)
TARGET_LINK_LIBRARIES(test_enmall teem)
ADD_TEST(enmall ${EXECUTABLE_OUTPUT_PATH}/test_enmall)

ADD_EXECUTABLE(test_kernall kernall.c)
TARGET_LINK_LIBRARIES(test_kernall teem)
ADD_TEST(kernall ${EXECUTABLE_OUTPUT_PATH}/test_kernall)

ADD_EXECUTABLE(test_buildinfo buildinfo.c)
TARGET_LINK_LIBRARIES(test_buildinfo teem)
ADD_TEST(buildinfo ${EXECUTABLE_OUTPUT_PATH}/test_buildinfo)

ADD_EXECUTABLE(test_probeSS probeSS.c)
TARGET_LINK_LIBRARIES(test_probeSS teem)
ADD_TEST(probeSS_01 ${EXECUTABLE_OUTPUT_PATH}/test_probeSS -supp 1.0 -pnum 800)
ADD_TEST(probeSS_02 ${EXECUTABLE_OUTPUT_PATH}/test_probeSS -supp 2.0 -pnum 800)
ADD_TEST(probeSS_04 ${EXECUTABLE_OUTPUT_PATH}/test_probeSS -supp 4.0 -pnum 800)
ADD_TEST(probeSS_10 ${EXECUTABLE_OUTPUT_PATH}/test_probeSS -supp 9.0 -pnum 800)
