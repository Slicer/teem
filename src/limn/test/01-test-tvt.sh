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

# 01=SECOND script to run to test limnCbfTVT via lpu cbfit -tvt

set -o errexit
set -o nounset
shopt -s expand_aliases
JUNK=""
function junk { JUNK="$JUNK $@"; }
function cleanup { rm -rf $JUNK; }
trap cleanup err exit int term
# https://devmanual.gentoo.org/tools-reference/bash/
unset UNRRDU_QUIET_QUIT

IN=circ.txt
#IN=pointy.txt
N=$(cat $IN | wc -l | xargs echo)
VVOUT=out-vv.txt
LTOUT=out-rt.txt
RTOUT=out-lt.txt
BIN=670

rm -f $VVOUT; touch $VVOUT; junk $VVOUT
rm -f $LTOUT; touch $LTOUT; junk $LTOUT
rm -f $RTOUT; touch $RTOUT; junk $RTOUT
for I in $(seq 0 $((N-1))); do
    # 16-fold (!) TEST:
    # 8: without -loop, versus with -loop
    # 4: LO=HI=0 (or LO=HI=5 in loop), versus some interval around I
    # 2: oneside (4th arg to -tvt) 0 versus 1
    # 1: -scl 0 versus >0
    LO=0 # $((I-4))
    HI=0 # $((I+4))
    CMD="./lpu cbfit -i $IN -tvt $LO $HI $I 0 -scl 0 -eps 1 -v 0"
    echo $CMD
    rm -f log.txt
    (eval $CMD 2>&1) > log.txt
    grep "lt =" log.txt | cut -d' ' -f 3,4 >> $LTOUT
    grep "vv =" log.txt | cut -d' ' -f 3,4 >> $VVOUT
    grep "rt =" log.txt | cut -d' ' -f 3,4 >> $RTOUT
done
rm -f log.txt

MMB="-min -1.1 1.1 -max 1.1 -1.1 -b $BIN $BIN"
unu jhisto -i $IN    $MMB -t float | unu axinsert -a 0 -s 3 |
    unu resample -s = x1 x1 -k gauss:1.3,6 | unu quantize -b 8 | unu 2op x - 0.5 -o in.png
junk in.png
unu jhisto -i $VVOUT $MMB | unu quantize -b 8 -max 1 -o vv.png
junk vv.png

TSCL=0.02
unu 2op x $LTOUT $TSCL | unu 2op + - $VVOUT | unu jhisto $MMB | unu quantize -b 8 -max 1 -o lt.png
unu 2op x $RTOUT $TSCL | unu 2op + - $VVOUT | unu jhisto $MMB | unu quantize -b 8 -max 1 -o rt.png
junk {l,r}t.png

# in: gray fuzzy blob
# lt: blue    vv: green    rt: red  points
unu join -i rt.png vv.png lt.png -a 0 -incr |
    unu 2op max in.png - |
    unu resample -s = x2 x2 -k box -o tvt.png

open tvt.png
