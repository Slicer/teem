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

# https://unix.stackexchange.com/questions/683231/replace-the-keyword-with-the-whole-content-of-another-file
sed -e '/# BIFFDICT/r biffDict.py' -e '//d' pre-teem.py > ../teem.py
