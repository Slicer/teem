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

# 03 = FOURTH stage of testing- another test limnCbfSingle, via lpu cbfit -fs,
# but with completely free-form input

set -o errexit
set -o nounset
shopt -s expand_aliases
JUNK=""
function junk { JUNK="$JUNK $@"; }
function cleanup { rm -rf $JUNK; }
trap cleanup err exit int term
# https://devmanual.gentoo.org/tools-reference/bash/
unset UNRRDU_QUIET_QUIT

# HEY changing this from 200 to 400 to 800 significantly changes the fit; why?
N=200

echo "-0.7 0.7
1.4 0.7
0.7 0.7
0.7 -0.7" | unu 2op x - 1 | unu 2op + - 0.0 | ./lpu cbfit -i - -synthn $N -sup 1 -syntho xy-0.txt
junk xy-0.txt

./lpu cbfit -i xy-0.txt -scl 0 -psi 10000 -fs 0 -1 -v 3 -eps 0.001 2>&1 > log.txt
cat log.txt; junk log.txt
tail -n 4 log.txt | ./lpu cbfit -i - -synthn $N -sup 1 -syntho xy-1.txt
junk xy-1.txt

BIN=500
for I in 0 1; do
    unu jhisto -i xy-$I.txt -min -1.1 1.1 -max 1.1 -1.1 -b $BIN $BIN |
        unu quantize -b 8 -max 1 -o xy-$I.png
done

unu join -i xy-{1,0,1}.png -a 0 -incr |
    unu resample -s = x2 x2 -k box -o tmp.png

open tmp.png
