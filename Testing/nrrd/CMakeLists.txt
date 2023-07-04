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

add_executable(test_trand trand.c)
target_link_libraries(test_trand teem)
add_test(NAME trand COMMAND $<TARGET_FILE:test_trand>)

add_executable(test_tload tload.c)
target_link_libraries(test_tload teem)
add_test(NAME tload COMMAND $<TARGET_FILE:test_tload>)

add_executable(test_tskip tskip.c)
target_link_libraries(test_tskip teem)
# Note the different file names; tests are run in parallel
add_test(NAME tskip11p COMMAND $<TARGET_FILE:test_tskip> -s 101 102 103 -p 66 81     -o tsA.raw tsA.nhdr)
add_test(NAME tskip11n COMMAND $<TARGET_FILE:test_tskip> -s 101 102 103 -p 66 81 -ns -o tsB.raw tsB.nhdr)
add_test(NAME tskip01p COMMAND $<TARGET_FILE:test_tskip> -s 101 102 103 -p 0 99      -o tsC.raw tsC.nhdr)
add_test(NAME tskip01n COMMAND $<TARGET_FILE:test_tskip> -s 101 102 103 -p 0 99 -ns  -o tsD.raw tsD.nhdr)
add_test(NAME tskip10p COMMAND $<TARGET_FILE:test_tskip> -s 101 102 103 -p 77 0      -o tsE.raw tsE.nhdr)
add_test(NAME tskip10n COMMAND $<TARGET_FILE:test_tskip> -s 101 102 103 -p 77 0 -ns  -o tsF.raw tsF.nhdr)

add_executable(test_sanity sanity.c)
target_link_libraries(test_sanity teem)
add_test(NAME sanity COMMAND $<TARGET_FILE:test_sanity>)

add_executable(test_macros macros.c)
target_link_libraries(test_macros teem)
add_test(NAME macros COMMAND $<TARGET_FILE:test_macros>)

add_executable(test_bspec tbspec.c)
target_link_libraries(test_bspec teem)
add_test(NAME bspec COMMAND $<TARGET_FILE:test_bspec> -bs bleed wrap pad:42)
