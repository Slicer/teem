#!/usr/bin/env bash
set -o errexit
set -o nounset

if [ ! -f biffDict.py ]; then
    echo "recreating biffDict.py ..."
    cd ../../../src/_util/
    python3 buildBiffDict.py ../..
    cd -
    echo "... done"
fi

sed -e '/# BIFFDICT/r biffDict.py' -e '//d' pre-teem.py > ../teem.py
