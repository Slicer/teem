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
# You should have received a copy of the GNU Lesser General Public License
# along with this library; if not, see <https://www.gnu.org/licenses/>.
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

# why changing N can significantly change the fit:
#    because the nrp parms that make sense for a small number of points don't work great
#    for a huge number of points

## Good debugging test case: N=18 is a bad fit, N=19 is a perfect fit
## BUT THEN the improved delta_t fixed this; so both are equally good!
## likely due to initial arc-length parameterization being bad, and nrp stuck in local minima
#N=18
#echo "-0.5 0.5
# 2.0  0.5
#-0.5  0.0
# 0.5 -0.5" |./lpu cbfit -i - -synthn $N -sup 1 -syntho xy-inn-$N.txt

## This is demo of why we might want a step that optimizes tangent directions
## after the parameterization has been found
#N=42
#echo "-1 -1
# 1 1.5
#-1 1.5
# 1 -1" | ./lpu cbfit -i - -synthn $N -sup 3 -syntho xy-inn-$N.txt

# This is demo of why we might want a step that optimizes tangent directions
# after the parameterization has been found
N=60
echo "1 -1
 1 3
-3 -1
 1 -1" | ./lpu cbfit -i - -synthn $N -v 0 -syntho - | unu crop -min 0 0 -max M M-1 -o xy-inn-$N.txt

junk xy-inn-$N.txt

CMD="./lpu cbfit -i xy-inn-$N.txt -scl 0 -fs 0 -1 -v 1 -psi 1000000000 -nim 4000 -deltathr 0.000000000001"
echo "====== $CMD"
eval $CMD > log.txt
cat log.txt; junk log.txt
tail -n 4 log.txt | ./lpu cbfit -i - -synthn $N -sup 1 -syntho xy-out-$N.txt
junk xy-out-$N.txt

BIN=500
for X in inn out; do
    unu jhisto -i xy-$X-$N.txt -min -1.1 1.1 -max 1.1 -1.1 -b $BIN $BIN |
        unu quantize -b 8 -max 1 -o xy-$X.png
done

unu join -i xy-{out,inn,out}.png -a 0 -incr |
    unu resample -s = x2 x2 -k box -o tmp.png

open tmp.png
