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

# 02=THIRD tests, of limnCbfSingle, via lpu cbfit -fs

set -o errexit
set -o nounset
shopt -s expand_aliases
JUNK=""
function junk { JUNK="$JUNK $@"; }
function cleanup { rm -rf $JUNK; }
trap cleanup err exit int term
# https://devmanual.gentoo.org/tools-reference/bash/
unset UNRRDU_QUIET_QUIT

IN=pointy.txt
#IN=circ.txt
N=$(cat $IN | wc -l | xargs echo)
BIN=900

MMB="-min -1.1 1.1 -max 1.1 -1.1 -b $BIN $BIN"

unu jhisto -i $IN    $MMB -t float |
    unu resample -s x1 x1 -k gauss:1.3,6 | unu quantize -b 8 | unu 2op x - 0.8 -o in.png
junk in.png

rm -f test-??.png

for LO in $(seq 0 $((N-1))); do
    echo $LO
    HI=$((LO+7))
    LOO=$(printf %02d $LO)
    CMD="./lpu cbfit -i $IN -loop -scl 0 -psi 1000 -fs $LO $HI -v 0 -eps 0.01"
    echo "==================== $LO $HI --> test-$LOO.png : $CMD"
    eval $CMD 2>&1 > log-$LOO.txt
    # cat log-$LOO.txt
    junk log-$LOO.txt
    tail -n 4 log-$LOO.txt |
        ./lpu cbfit -i - -synthn $N -syntho out.txt 2>&1 | grep -v _hestDefaults
    # lots of extraneous printf thwart piping
    unu jhisto -i out.txt $MMB | unu quantize -b 8 -max 1 -o out.png
    #  in: green
    # out: magenta
    unu join -i out.png in.png out.png -a 0 -incr -o test-$LOO.png
done
