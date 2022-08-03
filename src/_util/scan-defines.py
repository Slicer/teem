#!/usr/bin/env python
# halt if python2; thanks to https://preview.tinyurl.com/44f2beza
_x,*_y=1,2 # NOTE: A SyntaxError here means you need python3, not python2
del _x, _y

# a hacky script by GLK for making sure Teem's header files only #define
# names that make sense for their filename.  Example usage:
#   python3 scan-defines.py ~/teem-install

import os
import argparse
import re

verbose = 1
hPath = None
bad = False

def scan(LIB, file, FN):
    global bad
    lines = [line.rstrip() for line in file.readlines()]
    for L in filter(lambda L: L.startswith('#define'), lines):
        if (verbose > 1):
            print(f'see line |{L}|')
        if not (L.startswith(f'#define {LIB}') or L.startswith(f'#define _{LIB}')):
            # we allow some exceptions ...
            if 'airExistsConf.h' == FN and '#define airExistsConf_h' == L:
                # GLK not a fan of this, but whatever
                continue
            if 'air.h' == FN and re.match(r'#define TEEM_VERSION', L):
                # air.h is the right place for these TEEM defines
                continue
            if 'gage.h' == FN and re.match(r'#define gage.+?Of', L):
                # acceptable hack for preserving gage access to functions moved to air
                continue
            print(f'bad line: |{L}| in "{FN}"')
            print(f'          is not a {LIB} name')
            bad = True

def parse_args():
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
    args = parse_args()
    verbose = args.v
    try:
        os.chdir(args.install_path)
    except:
        print(f'\nError: Given Teem install path "{args.install_path}" does seem to exist.\n')
        raise
    try:
        os.chdir('include/teem')
    except:
        print(f'\nError: Given Teem install path "{args.install_path}" does not have "include/teem" sub-dir.\n')
        raise
    hPath = args.install_path + '/include/teem'
    hdrs = os.listdir('.')
    if not hdrs:
        raise Exception(f'No headers found in {args.install_path}/include/teem')
    if verbose:
        print(f'=== working on header files: {hdrs}')
    for FN in hdrs:
        if os.path.isdir(FN):
            raise Exception(f'Unexpected directory "{FN}" in include dir {hPath}')
        if not FN.endswith('.h'):
            raise Exception(f'file "{FN}" not ending with ".h"')
        match = re.match(r'([^A-Z]+).*\.h', FN)
        if not match:
            raise Exception(f'confusing filename "{FN}"')
        LIB = match.group(1).upper()
        if verbose:
            print(f'=== header file {FN} --> looking for {LIB} #defines')
        with open(FN, 'r') as file:
            scan(LIB, file, FN)
    if bad:
        raise Exception('One or more bad #defines seen')
    # else
    if verbose:
        print(f'all good')