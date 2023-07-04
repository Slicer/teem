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

add_executable(test_miscAir miscAir.c)
target_link_libraries(test_miscAir teem)
add_test(NAME misc COMMAND $<TARGET_FILE:test_miscAir>)

add_executable(test_mtrand mtrand.c)
target_link_libraries(test_mtrand teem)
add_test(NAME mtrand COMMAND $<TARGET_FILE:test_mtrand>)

add_executable(test_amath amath.c)
target_link_libraries(test_amath teem)
add_test(NAME amath COMMAND $<TARGET_FILE:test_amath>)

add_executable(test_string string.c)
target_link_libraries(test_string teem)
add_test(NAME string COMMAND $<TARGET_FILE:test_string>)

add_executable(test_strtok strtok.c)
target_link_libraries(test_strtok teem)
add_test(NAME strtok COMMAND $<TARGET_FILE:test_strtok>)

add_executable(test_pptest pptest.c)
target_link_libraries(test_pptest teem)
add_test(NAME pptest COMMAND $<TARGET_FILE:test_pptest>)
