#!/usr/bin/env bash
#
# Teem: Tools to process and visualize scientific data and images
# Copyright (C) 2009--2024  University of Chicago
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
#
# You should have received a copy of the GNU Lesser General Public License along with
# this library; if not, write to Free Software Foundation, Inc., 51 Franklin Street,
# Fifth Floor, Boston, MA 02110-1301 USA
#

# 00=FIRST script to run to generate circ.txt and pointy.txt data

set -o errexit
set -o nounset
shopt -s expand_aliases
JUNK=""
function junk { JUNK="$JUNK $@"; }
function cleanup { rm -rf $JUNK; }
trap cleanup err exit int term
# https://devmanual.gentoo.org/tools-reference/bash/
unset UNRRDU_QUIET_QUIT

N=50

echo 0 1 |
    unu reshape -s 2 |
    unu convert -t double |
    unu 3op x - 2 pi |
    unu resample -s $((N+1)) -k tent -c node -o a; junk a
unu 1op cos -i a -o x; junk x
unu 1op sin -i a -o y; junk y
unu gamma -i x -g 1.0 |  # gamma > 1 flattens right side, makes left side pointy
unu join -i - y -a 0 -incr |
    unu crop -min 0 0 -max M M-1 -o circ.txt

unu 2op spow circ.txt 4 -o pointy.txt
