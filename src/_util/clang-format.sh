#!/usr/bin/env bash
set -o errexit
set -o nounset
shopt -s expand_aliases

shopt -s nullglob
TODO=$(echo *.h *.c)
if [ "$TODO" = "" ]; then
    echo "Error: this expects to be be run in a directory with .c and .h files"
    exit 1
fi

echo "Running over files: $TODO"
for F in $TODO; do
    echo ================= $F
    clang-format -style=file < $F > /tmp/tmp
    set +o errexit
    diff $F /tmp/tmp
    RET=$?
    set -o errexit
    if [ $RET -ne 0 ]; then
        mv /tmp/tmp $F
    fi
done
