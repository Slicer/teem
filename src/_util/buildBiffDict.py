#!/usr/bin/env python
#
# Teem: Tools to process and visualize scientific data and images
# Copyright (C) 2009--2022  University of Chicago
# Copyright (C) 2008, 2007, 2006, 2005  Gordon Kindlmann
# Copyright (C) 2004, 2003, 2002, 2001, 2000, 1999, 1998  University of Utah
#
# This library is free software; you can redistribute it and/or modify it under the terms
# of the GNU Lesser General Public License (LGPL) as published by the Free Software
# Foundation; either version 2.1 of the License, or (at your option) any later version.
# The terms of redistributing and/or modifying this software also include exceptions to
# the LGPL that facilitate static linking.
#
# This library is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
# PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License along with
# this library; if not, write to Free Software Foundation, Inc., 51 Franklin Street,
# Fifth Floor, Boston, MA 02110-1301 USA

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
# HEY HACK: see note below about hard-coding 'rv' variable name
def rvtest(typ, tv):
    ret = None
    if 'int' in typ:
        try:
            vv = int(tv)
            ret = f'{vv} == rv'
        except ValueError: # int() conversion failed
            # going to take wild-ass assumption that this is an enum (e.g. hooverErrInit)
            ret = f'{"_teem.lib." + tv} == rv'
            # HEY you know we could import _teem here and check this assumption ...
    elif 'NULL' == tv:
        ret = f'_teem.ffi.NULL == rv'
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
    if 1 != len(anlist):
        raise Exception(f'got multiple words {anlist} (from "{annoteCmt}") but expected 1')
    anval = anlist[0]
    if anval.startswith('maybe:'):
        mlist = anval.split(':') # mlist[0]='maybe:'
        mubi = mlist[1] # for Maybe, useBiff index (1-based)
        anval = mlist[2] # returned value(s) to check
    else:
        mubi = 0
    if not (match := re.match(r'_*(.+?)[^a-z]', funcName)):
        raise Exception(f'couldn\'t extract library name from function name "{funcName}"')
    bkey = match.group(1)
    if not bkey in tlibs:
        raise Exception(f'apparent library name prefix "{bkey}" of function "{funcName}" not in Teem library list {tlibs}')
    # ** NOTE: this is all very specific to the Python CFFI wrapper in
    # ** teem/python/cffi/teem.py. But the code above is useful logic
    # ** for anything seeking to use the Biff annotations, and wanting
    # ** some self-contained repackaging of their info. This code could
    # ** be later expanded to generate analogous dictionaries useful for
    # ** other wrappings (e.g. Julia).
    # How this is going to be used in teem.py:
    # set up, generated by us:
    #    biffDict['func'] = (rvteg, bkey, fnln)
    # later:
    #    rv = _teem.lib.func(*args)
    #    if ['func'] in biffDict:
    #       (_rvte, _bkey, _fnln) = _biffDict['func']
    #       generated code for 'func' wrapper:
    #       ... "if {_rvte}:" ...
    #            biffGetDone(bkey)
    # so, need to generate text of expression involving
    # return value 'rv', and maybe argument list 'args',
    # that can be evaluated to know if a biff error message needs retreiving
    # The "return value test expression" is "rvte"
    # ** HEY HACK! we are hard-coding the variable names 'rv' and 'args' !
    # ** to be cleaner, we'd generate text of a lambda function taking those
    # ** variable names as arguments, but that seems needlessly baroque given
    # ** the current circumstances.
    rvte = ' or '.join([rvtest(qts, V) for V in anval.split('|')])
    if mubi:
        rvte = f'(args[{mubi}] and ({rvte}))'
    else:
        rvte = f'({rvte})'
    # finally, write dict entry for handling errors in funcName
    print(f'    \'{funcName}\': (\'{rvte}\', \'{bkey}\', \'{fnln}\'),', file=oFile)

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
    parser.add_argument('-o', metavar='outfile', default='../../python/cffi/teemPySrc/biffDict.py',
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
        print('# (the following generated by teem/src/_util/buildBiffDict.py)', file=oFile)
        print('_biffDict = {', file=oFile)
        for L in tlibs:
            doLib(oFile, args.teem_path, L)
        print('}', file=oFile)
