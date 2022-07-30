#!/usr/bin/env python
# halt if python2; thanks to https://preview.tinyurl.com/44f2beza
_x,*_y=1,2 # NOTE: A SyntaxError here means you need python3, not python2
del _x, _y

from os import path
from sys import platform
import argparse
import subprocess
import re

verbose = 1

# the variable _ is (totally against python conventions) standing for some particular!
# for interpreting "nm" output
if platform == 'darwin':  # Mac
    _ = '_'
elif platform == 'linux':
    _ = ''
else:
    raise Exception('Sorry, currently only know how work on Mac and Linux')

def sscan(tpath, lib):
    if (not (path.isdir(tPath)
            and path.isdir(f'{tPath}/{arch}')
            and path.isdir(f'{tPath}/{src}') )):
        raise Exception(f'Need {tPath} to dir with "arch" and "src" subdirs')

def parse_args():
    # https://docs.python.org/3/library/argparse.html
    parser = argparse.ArgumentParser(description='Utility for seeing if the symbols '
                                     'externally available in a library really are '
                                     'declared as such.')
    parser.add_argument('-v', metavar='verbosity', type=int, default=1, required=False,
                        help='verbosity level (0 for silent)')
    parser.add_argument('teem_path',
                        help='path of Teem checkout with "src" and "arch" subdirs')
    parser.add_argument('lib',
                        help='which single library to scan')
    return parser.parse_args()

if __name__ == '__main__':
    args = parse_args()
    verbose = args.v
    sscan(args.teem_path, args.lib)
