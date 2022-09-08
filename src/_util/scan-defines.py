#!/usr/bin/env python
"""
a hacky script by GLK for making sure Teem's header files only #define
names that make sense for their filename.  Example usage:
  python3 scan-defines.py ~/teem-install
"""

import os
import argparse
import re

# halt if python2; thanks to https://preview.tinyurl.com/44f2beza
_x,*_y=1,2 # NOTE: A SyntaxError here means you need python3, not python2
del _x, _y

VERBOSE = 1
H_PATH = None
BAD = False

def scan(libname_up, file, filename):
    """scans lines in given file (named filename, in library (upper-case name) libname_up)
    looking for #defines that do not start with LIB or _LIB"""
    global BAD
    lines = [line.rstrip() for line in file.readlines()]
    for line in filter(lambda L: L.startswith('#define'), lines):
        if VERBOSE > 1:
            print(f'see line |{line}|')
        if not (line.startswith(f'#define {libname_up}')
                or line.startswith(f'#define _{libname_up}')):
            # we allow some exceptions ...
            if 'airExistsConf.h' == filename and '#define airExistsConf_h' == line:
                # GLK not a fan of this, but whatever
                continue
            if 'air.h' == filename and re.match(r'#define TEEM_VERSION', line):
                # air.h is the right place for these TEEM defines
                continue
            if 'gage.h' == filename and re.match(r'#define gage.+?Of', line):
                # acceptable hack for preserving gage access to functions moved to air
                continue
            print(f'bad line: |{line}| in "{filename}"')
            print(f'          is not a {libname_up} name')
            BAD = True

def parse_args():
    """create parser for command-line, and use it"""
    # https://docs.python.org/3/library/argparse.html
    parser = argparse.ArgumentParser(description='Utility for seeing if #defines in installed '
                                     'Teem header files are limiting themselves to the '
                                     'corresponding Teem library names.')
    parser.add_argument('-v', metavar='verbosity', type=int, default=1, required=False,
                        help='verbosity level (0 for silent)')
    parser.add_argument('install_path',
                        help='path into which CMake has install Teem (should have '
                        '\"include\" and \"lib\" subdirectories)')
    return parser.parse_args()

if __name__ == '__main__':
    ARGS = parse_args()
    VERBOSE = ARGS.v
    try:
        os.chdir(ARGS.install_path)
    except:
        print(f'\nError: Given Teem install path "{ARGS.install_path}" does seem to exist.\n')
        raise
    try:
        os.chdir('include/teem')
    except:
        print(f'\nError: Given Teem install path "{ARGS.install_path}" '
              'does not have "include/teem" sub-dir.\n')
        raise
    H_PATH = ARGS.install_path + '/include/teem'
    HDRS = os.listdir('.')
    if not HDRS:
        raise Exception(f'No headers found in {ARGS.install_path}/include/teem')
    if VERBOSE:
        print(f'=== working on header files: {HDRS}')
    for FN in HDRS:
        if os.path.isdir(FN):
            raise Exception(f'Unexpected directory "{FN}" in include dir {H_PATH}')
        if not FN.endswith('.h'):
            raise Exception(f'file "{FN}" not ending with ".h"')
        match = re.match(r'([^A-Z]+).*\.h', FN)
        if not match:
            raise Exception(f'confusing filename "{FN}"')
        LIB = match.group(1).upper()
        if VERBOSE:
            print(f'=== header file {FN} --> looking for {LIB} #defines')
        with open(FN, 'r', encoding='utf-8') as hfile:
            scan(LIB, hfile, FN)
    if BAD:
        raise Exception('One or more bad #defines seen')
    # else
    if VERBOSE:
        print('all good')
