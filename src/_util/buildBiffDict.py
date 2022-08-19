#!/usr/bin/env python
# halt if python2; thanks to https://preview.tinyurl.com/44f2beza
_x,*_y=1,2 # NOTE: A SyntaxError here means you need python3, not python2
del _x, _y

import os
import argparse
import re

verbose = 1
# TEEM_LIB_LIST
tlibs = [ # 'air', 'hest', 'biff',  (these do not use biff)
'nrrd', 'ell', 'moss', 'unrrdu', 'alan', 'tijk', 'gage',
'dye', 'bane', 'limn', 'echo', 'hoover', 'seek', 'ten',
'elf', 'pull', 'coil', 'push', 'mite', 'meet']

def argsCheck(tPath):
    if (not (os.path.isdir(tPath)
            and os.path.isdir(f'{tPath}/arch')
            and os.path.isdir(f'{tPath}/src') )):
        raise Exception(f'Need {tPath} to be dir with "arch" and "src" subdirs')

# the task is: given the string representation of the error return test value
# tv as it came out of the "Biff:"" annotation from the .c file, convert it to
# another string representation of a Python boolean expression that is true if
# the actual function return value rv is equal to tv.
# This assumes that the user of our output has:
# - imported _teem (for enum values)
# - imported math (for isnan)
def rvtest(typ, tv):
    ret = None
    if 'int' in typ:
        try:
            vv = int(tv)
            ret = f'({vv} == rv)'
        except ValueError: # int() conversion failed
            # going to take wild-ass guess that this is an enum (e.g. hooverErrInit)
            ret = f'({"_teem.lib." + tv} == rv)'
            # HEY you know we could import _teem here and check ...
    elif 'NULL' == tv:
        ret = f'(_teem.ffi.NULL == rv)'
    elif 'AIR_NAN' == tv:
        ret = f'math.isnan(rv)'
    else:
        # this function is super adhoc-y, and will definitely require future expansion
        raise Exception(f'sorry don\'t yet know how to handle typ={typ}, tv={tv}')
    return ret


def doAnnote(oFile, funcName, retQT, annoteCmt, fnln):
    qts = retQT.split(' ')
    # remove any comment within annotation
    if '#' in annoteCmt:
        annote = annoteCmt.split('#')[0].strip()
    else:
        annote = annoteCmt
    anlist = annote[6:].split(' ') # remove "Biff: ", and split into list
    # For Python-wrapping the Teem API, we're ignoring some things:
    if 'static' in qts:
        # function not accessible anyway in the libteem library
        return
    if 'nope' in anlist:
        # function doesn't use biff, so nothing for wrapper to do w.r.t biff
        return
    if '(private)' in anlist:
        # wrapper is around public API, so nothing for this either
        # (this contains some of the more far-out uses of Biff annotations,
        # so changing this will require more work on rvtest() above )
        return
    if not (match := re.match(r'_*(.+?)[^a-z]', funcName)):
        raise Exception(f'couldn\'t extract library name from function name "{funcName}"')
    bkey = match.group(1)
    if 1 != len(anlist):
        raise Exception(f'got multiple words {anlist} (from "{annoteCmt}") but expected 1')
    anval = anlist[0]
    if anval.startswith('maybe:'):
        mlist = anval.split(':') # mlist[0]='maybe:'
        mubi = mlist[1] # for Maybe, useBiff index (1-based)
        anval = mlist[2] # returned value(s) to check
    else:
        mubi = 0
    # how this is going to be used
    # set up: biffDict['func'] = (mubi, rvtf, bkey, fnln)
    # later:
    # rv = _teem.lib.func(*args)
    # if ['func'] in biffDict:
    #    (mubi, rvtf, bkey, fnln) = biffDict['func']
    #    if (0 == mubi or args[mubi-1]) and rvtf(rv):
    #       biffGetDone(bkey)
    # so need to generate text of (maybe lambda) function rvtf
    rvtf = 'lambda rv: ' + ' or '.join([rvtest(qts, V) for V in anval.split('|')])
    if 'lambda rv: (1 == rv)' == rvtf:
        rvtf = '_equalsOne'
    #print(f'{mubi} |{rvtf}| {qts} "{annote}" {anval}')
    # write dict entry for handling errors in funcName
    print(f'    \'{funcName}\': ({mubi}, \'{rvtf}\', \'{bkey}\', \'{fnln}\'),', file=oFile)

def doSrc(oFile, sFile, sFN):
    lines = [line.strip() for line in sFile.readlines()]
    nlen = len(lines)
    idx = 0
    while idx < nlen:
        # scan lines of soure file, looking for Biff: annotations
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
        if (verbose > 1):
            print(f'... {lib}/{FN}')
        with open(f'{sPath}/{FN}') as sFile:
            doSrc(oFile, sFile, f'{lib}/{FN}')

def parse_args():
    # https://docs.python.org/3/library/argparse.html
    parser = argparse.ArgumentParser(description='Scans "Biff:" annotations, and generates '
                                    'a Python dict representing the same information',
                                    formatter_class=argparse.ArgumentDefaultsHelpFormatter)
                                    # formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument('-v', metavar='verbosity', type=int, default=1, required=False,
                        help='verbosity level (0 for silent)')
    parser.add_argument('-o', metavar='outfile', default='../../python/cffi/teemPyGen/biffDict.py',
                        help='output filename to put biff dict in')
    parser.add_argument('teem_path',
                        help='path of Teem checkout with "src" and "arch" subdirs')
    # we always do all the libraries (that might use biff)
    # regardless of "experimental"
    return parser.parse_args()

if __name__ == '__main__':
    args = parse_args()
    verbose = args.v
    argsCheck(args.teem_path)
    with open(args.o, 'w') as oFile:
        print('def _equalsOne(rv):', file=oFile)
        print('    return rv == 1\n', file=oFile)
        print('_biffDict = {', file=oFile)
        for L in tlibs:
            doLib(oFile, args.teem_path, L)
        print('}', file=oFile)
