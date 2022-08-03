#!/usr/bin/env python
# halt if python2; thanks to https://preview.tinyurl.com/44f2beza
_x,*_y=1,2 # NOTE: A SyntaxError here means you need python3, not python2
del _x, _y

# This is a script for generating ../teem.py, the wrapper module around ../_teem.py
# (the CFFI-based module around the CMake-compiled libteem shared library)

import argparse
import re
from os import path

sPath = None
verbose = 0

# (TEEM_LIB_LIST)
libs = ['air', 'hest', 'biff', 'nrrd', 'ell', 'moss', 'unrrdu', 'alan', 'tijk',
        'gage', 'dye', 'bane', 'limn', 'echo', 'hoover', 'seek', 'ten', 'elf',
        'pull', 'coil', 'push', 'mite', 'meet']

def check_path():
    if (not path.isdir(sPath) or not path.isdir(f'{sPath}/src')):
        raise Exception(f'Teem source path "{sPath}" not a directory with "src" subdir')
    missingSrc = [L for L in filter(lambda L: not path.isdir(f'{sPath}/src/{L}'), libs)]
    if (len(missingSrc)):
        raise Exception(f'missing source dir for {missingSrc} library')

def blistAddFile(bfile, FN, sfile):
    lines = [line.rstrip() for line in sfile.readlines()]
    idx = 0
    while idx < len(lines):
        T = lines[idx-1]
        L = lines[idx]
        # find lines that look like start of a non-static function definition
        if (not T.startswith('static')
            and re.match('^[_a-zA-Z0-9]+?\(', L)):
            print(f'   {idx} | {T} | {L}')
        idx += 1

def blistAddLib(bfile, L, cmfile):
    lines = [line.strip() for line in cmfile.readlines()]
    idx0 = 1 + lines.index(f'set({L.upper()}_SOURCES')
    idx1 = lines.index(')')
    srcs = filter(lambda fn: fn.endswith('.c'), lines[idx0:idx1])
    for FN in srcs:
        if (verbose):
            print(f'... {FN}')
        with open(f'{sPath}/src/{L}/{FN}') as sfile:
            blistAddFile(bfile, FN, sfile)

def tpgo():
    check_path()
    with open('biffList.txt', 'w') as bfile:
        for L in libs:
            if ('air' == L): continue # doesn't use biff
            if ('hest' == L): continue # doesn't use biff
            if ('biff' == L): continue # doesn't use biff (!)
            if (verbose):
                print(f'========== {L} ==========')
            with open(f'{sPath}/src/{L}/CMakeLists.txt') as cmfile:
                blistAddLib(bfile, L, cmfile)

def parse_args():
    # https://docs.python.org/3/library/argparse.html
    parser = argparse.ArgumentParser(description='Utility for generating ../teem.py, '
                                     'the wrapper around ../_teem.py (the CFFI-based module'
                                     'around the CMake-compiled libteem shared library)')
    parser.add_argument('-v', metavar='verbosity', type=int, default=1, required=False,
                        help='verbosity level (0 for silent)')
    parser.add_argument('source_path',
                        help='path of the Teem *source* (not the install dir)')
    return parser.parse_args()

if __name__ == '__main__':
    args = parse_args()
    verbose = args.v
    sPath = args.source_path
    tpgo()
