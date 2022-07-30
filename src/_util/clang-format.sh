#!/usr/bin/env bash
set -o errexit
set -o nounset
shopt -s expand_aliases
JUNK=""
function junk { JUNK="$JUNK $@"; }
function cleanup { rm -rf $JUNK; }
trap cleanup err exit int term
# https://devmanual.gentoo.org/tools-reference/bash/
unset UNRRDU_QUIET_QUIT

shopt -s nullglob
TODO=$(echo *.h *.c)
for F in $TODO; do
    echo ================= $F
    clang-format -style=file < $F > /tmp/tmp
    diff $F /tmp/tmp ||:
    # vdiff $F /tmp/tmp
    cp /tmp/tmp $F
done
