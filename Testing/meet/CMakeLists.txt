#
# Teem: Tools to process and visualize scientific data and images
# Copyright (C) 2009--2019  University of Chicago
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

add_executable(test_enmall enmall.c)
target_link_libraries(test_enmall teem)
add_test(NAME enmall COMMAND $<TARGET_FILE:test_enmall>)

add_executable(test_kernall kernall.c)
target_link_libraries(test_kernall teem)
add_test(NAME kernall COMMAND $<TARGET_FILE:test_kernall>)

add_executable(test_buildinfo buildinfo.c)
target_link_libraries(test_buildinfo teem)
add_test(NAME buildinfo COMMAND $<TARGET_FILE:test_buildinfo>)

add_executable(test_probeSS probeSS.c)
target_link_libraries(test_probeSS teem)
add_test(NAME probeSS_box01 COMMAND $<TARGET_FILE:test_probeSS> -k box -supp 1.0 -pnum 1500)
add_test(NAME probeSS_cos01 COMMAND $<TARGET_FILE:test_probeSS> -k cos -supp 1.0 -pnum 1500)
add_test(NAME probeSS_cos02 COMMAND $<TARGET_FILE:test_probeSS> -k cos -supp 2.0 -pnum 1200)
add_test(NAME probeSS_cos04 COMMAND $<TARGET_FILE:test_probeSS> -k cos -supp 4.0 -pnum 1000)
add_test(NAME probeSS_cos10 COMMAND $<TARGET_FILE:test_probeSS> -k cos -supp 9.0 -pnum 800)
add_test(NAME probeSS_ctmr02 COMMAND $<TARGET_FILE:test_probeSS> -k ctmr -supp 2.0 -pnum 1300)
add_test(NAME probeSS_ctmr04 COMMAND $<TARGET_FILE:test_probeSS> -k ctmr -supp 4.0 -pnum 1300)
add_test(NAME probeSS_ctmr10 COMMAND $<TARGET_FILE:test_probeSS> -k ctmr -supp 9.0 -pnum 1300)
