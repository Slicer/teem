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

add_executable(test_sbp tsbp.c)
target_link_libraries(test_sbp teem)
add_test(NAME stackBlurParm COMMAND $<TARGET_FILE:test_sbp> -sbp 0-5-4.2)

add_executable(test_probeScl probeScl.c)
target_link_libraries(test_probeScl teem)
add_test(NAME probeScl COMMAND $<TARGET_FILE:test_probeScl>)

add_executable(test_probePolynomial probePolynomial.c)
target_link_libraries(test_probePolynomial teem)
add_test(NAME probePolynomial COMMAND $<TARGET_FILE:test_probePolynomial>)

add_executable(test_probeMulti probeMulti.c)
target_link_libraries(test_probeMulti teem)
add_test(NAME probeMulti COMMAND $<TARGET_FILE:test_probeMulti>)
