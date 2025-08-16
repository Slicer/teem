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

if false; then
N=199
echo "-1 -1
-1 1
1 1
0 0" | ./lpu cbfit -i - -synthn $N  -syntho 0.txt
echo "0 0
-0.5 -0.5
1 -1
1 1" | ./lpu cbfit -i - -synthn $N  -syntho 1.txt
cat 0.txt 1.txt | uniq > xy-inn.txt
junk {0,1}.txt xy-inn.txt
IN=xy-inn.txt
fi

IN=xy-inn-60.txt
#IN=pointy.txt
#IN=circ.txt

rm -f tmp.png

SCL=3
EPS=0.08
rm -f log.txt
# CMD="./lpu cbfit -i $IN             -scl   0  -v 0 -eps $EPS -roll 0"
# CMD="./lpu cbfit -i $IN       -ca 0 -scl   0  -v 0 -eps $EPS -roll 0"
# CMD="./lpu cbfit -i $IN             -scl $SCL -v 0 -eps $EPS -roll 0"
# CMD="./lpu cbfit -i $IN       -ca 0 -scl $SCL -v 0 -eps $EPS -roll 0"
# CMD="./lpu cbfit -i $IN -loop       -scl   0  -v 0 -eps $EPS -roll 0"
# CMD="./lpu cbfit -i $IN -loop -ca 0 -scl   0  -v 0 -eps $EPS -roll 0"
# CMD="./lpu cbfit -i $IN -loop       -scl   0  -v 0 -eps $EPS -roll 5"
# CMD="./lpu cbfit -i $IN -loop -ca 0 -scl   0  -v 0 -eps $EPS -roll 5"
# CMD="./lpu cbfit -i $IN -loop       -scl $SCL -v 0 -eps $EPS -roll 0"
# CMD="./lpu cbfit -i $IN -loop -ca 0 -scl $SCL -v 0 -eps $EPS -roll 0"
# CMD="./lpu cbfit -i $IN -loop       -scl $SCL -v 0 -eps $EPS -roll 5"
  CMD="./lpu cbfit -i $IN -loop -ca 0 -scl $SCL -v 0 -eps $EPS -roll 5"
eval $CMD  2>&1 > log.txt ||:
echo "====== $CMD"
cat log.txt # ; junk log.txt

OUT=xy-out.txt
echo "====== RESULTS: --> $OUT"
grep "^seg" log.txt | xargs -n 12 echo | cut -d' ' -f 2,3,4,5,6,7,8,9
grep "^seg" log.txt | xargs -n 12 echo | cut -d' ' -f 2,3,4,5,6,7,8,9 |
    ./lpu cbfit -i - -loop -synthn 900 -sup 1 -syntho $OUT
junk $OUT


BIN=660
MM="-min -1.1 1.1 -max 1.1 -1.1"
unu jhisto -i  $IN $MM -b $BIN $BIN | unu quantize -b 8 -max 1 -o xy-inn.png
unu jhisto -i $OUT $MM -b $BIN $BIN | unu quantize -b 8 -max 1 -o xy-out.png


unu join -i xy-{out,inn,out}.png -a 0 -incr |
    unu resample -s = x2 x2 -k box -o tmp.png

unu cksum tmp.png # with corners, is it stable w.r.t. -roll ?

open tmp.png
