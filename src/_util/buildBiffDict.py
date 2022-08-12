#!/usr/bin/env python
# halt if python2; thanks to https://preview.tinyurl.com/44f2beza
_x,*_y=1,2 # NOTE: A SyntaxError here means you need python3, not python2
del _x, _y

import os
import argparse
import re

verbose = 1
# TEEM_LIB_LIST
tlibs = [ # 'air', 'hest', 'biff',    (these don't use biff)
'nrrd', 'ell', 'moss', 'unrrdu', 'alan', 'tijk', 'gage',
'dye', 'bane', 'limn', 'echo', 'hoover', 'seek', 'ten',
'elf', 'pull', 'coil', 'push', 'mite', 'meet']

def argsCheck(tPath):
    if (not (os.path.isdir(tPath)
            and os.path.isdir(f'{tPath}/arch')
            and os.path.isdir(f'{tPath}/src') )):
        raise Exception(f'Need {tPath} to be dir with "arch" and "src" subdirs')

def doAnnote(oFile, funcName, retQT, annote, wher):
    # bail if static function
    # bail if "nope"
    # remove any # comment in annote
    print(f'{retQT} {funcName} {annote} ({wher})', file=oFile)

def doSrc(oFile, sFile, sFN):
    lines = [line.strip() for line in sFile.readlines()]
    nlen = len(lines)
    idx = 0
    while idx < nlen:
        if (match := re.match(r'(.+?)\/\* (Biff: .+?)\*\/', lines[idx])):
            retQT = match.group(1).strip()
            annote = match.group(2).strip()
            idx += 1
            if not (match := re.match(r'(.+?)\(', lines[idx])):
                raise Exception(f'couldnt parse function name on line {idx+1} of {sFN}')
            funcName = match.group(1)
            doAnnote(oFile, funcName, retQT, annote, f'{sFN}:{idx}')
        idx += 1

def doLib(oFile, tPath, lib):
    sPath = f'{tPath}/src/{lib}'
    # read the CMakeLists.txt file to get list of source files
    with open(f'{sPath}/CMakeLists.txt') as cmfile:
        lines = [line.strip() for line in cmfile.readlines()]
    idx0 = 1 + lines.index(f'set({L.upper()}_SOURCES')
    idx1 = lines.index(')')
    srcs = filter(lambda fn: fn.endswith('.c'), lines[idx0:idx1])
    for FN in srcs:
        if (verbose):
            print(f'... {lib}/{FN}')
        with open(f'{sPath}/{FN}') as sFile:
            doSrc(oFile, sFile, f'{lib}/{FN}')

def parse_args():
    # https://docs.python.org/3/library/argparse.html
    parser = argparse.ArgumentParser(description='Scans "Biff:" annotations, and generates '
                                    'a Python dict representing the same information',
                                     formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument('-v', metavar='verbosity', type=int, default=1, required=False,
                        help='verbosity level (0 for silent)')
    parser.add_argument('teem_path',
                        help='path of Teem checkout with "src" and "arch" subdirs')
    parser.add_argument('out_file',
                        help='output filename to put biff dict in, probably something '
                        'like ../../python/cffi/biffDict.py')
    # we always do all the libraries (that might use biff)
    return parser.parse_args()

if __name__ == '__main__':
    args = parse_args()
    verbose = args.v
    argsCheck(args.teem_path)
    with open(args.out_file, 'w') as oFile:
        for L in tlibs:
            doLib(oFile, args.teem_path, L)