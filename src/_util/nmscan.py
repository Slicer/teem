#!/usr/bin/env python
# halt if python2; thanks to https://preview.tinyurl.com/44f2beza
_x,*_y=1,2 # NOTE: A SyntaxError here means you need python3, not python2
del _x, _y

import os
import sys
import argparse
import subprocess
import re

verbose = 1
archDir = None
libDir = None

# the variable _ is (totally against python conventions) standing for some particular!
# for interpreting "nm" output
if sys.platform == 'darwin':  # Mac
    dropUnder = True
elif sys.platform == 'linux':
    dropUnder = False
else:
    raise Exception('Sorry, currently only know how work on Mac and Linux')

def argsCheck(tPath, lib):
    global archDir, libDir
    if (not (os.path.isdir(tPath)
            and os.path.isdir(f'{tPath}/arch')
            and os.path.isdir(f'{tPath}/src') )):
        raise Exception(f'Need {tPath} to be dir with "arch" and "src" subdirs')
    if (not os.path.isdir(f'{tPath}/src/{lib}')):
        raise Exception(f'Do not see library "{lib}" subdir in "src" subdir')
    if not 'TEEM_ARCH' in os.environ:
        raise Exception(f'Environment variable "TEEM_ARCH" not set')
    archEV = os.environ['TEEM_ARCH']
    archDir = f'{tPath}/arch/{archEV}'
    if not os.path.isdir(archDir):
        raise Exception(f'Do not see "{archDir}" subdir for TEEM_ARCH "{archEV}"')
    libDir = f'{tPath}/src/{lib}'
    if not os.path.isdir(libDir):
        raise Exception(f'Do not see "{libDir}" subdir for lib "{lib}"')

def runthis(cmdStr, capOut):
    if (verbose):
        print(f' ... running "{cmdStr}"')
    spr = subprocess.run(cmdStr.split(' '), check=True, capture_output=capOut)
    ret = spr if capOut else None
    return ret

def symbList(lib, firstClean):
    if (verbose):
        print(f'========== recompiling {lib} ... ')
    if firstClean:
        runthis('make clean', False)
    runthis('make install', False)
    nmOut = runthis(f'nm {archDir}/lib/lib{lib}.a', True).stdout.decode('UTF-8').splitlines()
    nmOut.pop(0) # first line is empty (at least on Mac)
    symb = {} # accumulates dict mapping from name to {type, object}
    currObj = None
    for L in nmOut:
        if (match := re.match(r'[^\()]+\(([^\)]+).o\):$', L)):
            currObj = match.group(1)
            if (verbose > 1):
                print(f'   ... {currObj}')
            continue
        if not len(L):
            # blank lines delimit objects
            currSrc = None
            continue
        if re.match(r' + U ', L) or re.match(r'[0-9a-fA-F]+ (?:t|d|s) ', L):
            # static (non-external) text, data, or other; no problem
            continue
        if not re.match(r'[0-9a-fA-F]+ [TDS] ', L):
            # flag this but don't raise an Exception; sometimes these are ok
            print(f'\nHEY: weird symbol "{L}" in {currObj}.c\n')
            continue;
        if dropUnder: #        (----- 1 -----)(- 2 -)  ( 3)
            match = re.match(r'([0-9a-fA-F]+ )([TDS]) _(.*)$', L)
            if not match:
                raise Exception(f'malformed (no leading underscore) "{L}" in {currObj}.c')
        else: # not trying to drop leading underscore (not on Mac)
            #                  (----- 1 -----)(- 2 -) ( 3)
            match = re.match(r'([0-9a-fA-F]+ )([TDS]) (.*)$', L)
            if not match:
                raise Exception(f'malformed "{L}" in {currObj}.c')
        ss = match.group(3)
        # subsuming role of old _util/release-nm-check.csh
        if (not ss.startswith(lib) and not ss.startswith(f'_{lib}')):
            raise Exception(f'symbol {ss} (from {currObj}.c) does not start with {lib} or _{lib}')
        symb[ss] = {'type': match.group(2), 'object': currObj}
    return symb

def declList(lib):
    pubH = f'{lib}.h'
    prvH = f'private{lib.title()}.h'
    if (os.path.isfile(prvH)):
        hdrs = [pubH, prvH]
    else:
        hdrs = [pubH]
    if (verbose):
        print(f"========== scanning {lib} headers {hdrs}... ")
    decl = {}
    for HN in hdrs:
        pub = (0 == hdrs.index(HN))
        want = f'{lib.upper()}_EXPORT ' if pub else 'extern '
        with open(HN) as HF:
            lines = [line.rstrip() for line in HF.readlines()]
            # HEY need special handling of:
            # NRRD_EXPORT NrrdKernel
            #  *const nrrdKernel ..
            # initial grep on whether the line starts like a declaration
            lines = [L for L in filter(lambda L: L.startswith(want), lines)]
            # remove 'extern "C" {' in from list for private
            if not pub: lines.remove('extern "C" {')
            # remove LIB_EXPORT or extern prefix
            lines = [L.removeprefix(want) for L in lines]
            #for L in lines:
            #    print(L)
    return decl


def parse_args():
    # https://docs.python.org/3/library/argparse.html
    parser = argparse.ArgumentParser(description='Utility for seeing if the symbols '
                                     'externally available in a library really are '
                                     'declared as such.')
    parser.add_argument('-v', metavar='verbosity', type=int, default=1, required=False,
                        help='verbosity level (0 for silent)')
    parser.add_argument('-c', action='store_true',
                        help='Do a "make clean" before make')
    parser.add_argument('teem_path',
                        help='path of Teem checkout with "src" and "arch" subdirs')
    parser.add_argument('lib',
                        help='which single library to scan')
    return parser.parse_args()

if __name__ == '__main__':
    args = parse_args()
    verbose = args.v
    argsCheck(args.teem_path, args.lib)
    if (verbose):
        print(f'========== cd {libDir} ... ')
    os.chdir(libDir)
    symb = symbList(args.lib, args.c)
    if (verbose > 1):
        print('========== found symbols:', symb)
    decl = declList(args.lib)
    if (verbose > 1):
        print('========== found declarations:', decl)
