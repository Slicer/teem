#!/usr/bin/env python3
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

# HEY TODO: make pylint happier!

# halt if python2; thanks to https://preview.tinyurl.com/44f2beza
import re
import argparse
import os

_x, *_y = 1, 2  # NOTE: A SyntaxError here means you need python3, not python2
del _x, _y


VERB = 1
# TEEM_LIB_LIST
TLIBS = [  # 'air', 'hest', 'biff',  (these do not use biff)
    'nrrd',
    'ell',
    'moss',
    'unrrdu',
    'alan',
    'tijk',
    'gage',
    'dye',
    'bane',
    'limn',
    'echo',
    'hoover',
    'seek',
    'ten',
    'elf',
    'pull',
    'coil',
    'push',
    'mite',
    'meet',
]


def argsCheck(oPath: str, tPath: str) -> None:
    """Checks command-line args"""
    if not os.path.isdir(oPath):
        raise Exception(f'Need output {oPath} to be directory')
    if not (
        os.path.isdir(tPath) and os.path.isdir(f'{tPath}/arch') and os.path.isdir(f'{tPath}/src')
    ):
        raise Exception(f'Need Teem source {tPath} to be dir with "arch" and "src" subdirs')


# the task is: given the string representation of the error return test value
# tv as it came out of the "Biff:"" annotation from the .c file, convert it to
# another string representation of a Python boolean expression that is true if
# the actual function return value {rvName} is equal to tv.
# This assumes that the user of our output has:
# - imported _teem (for enum values)
# - imported math as _math (for isnan)


# def rvtest(typ, tv, rvName):
#    ret = None
#    if 'int' in typ:
#        try:
#            vv = int(tv)
#            ret = f'{vv} == {rvName}'
#        except ValueError:  # int() conversion failed
#            # going to take wild-ass assumption that this is an enum (e.g. hooverErrInit)
#            ret = f'{"_teem.lib." + tv} == {rvName}'
#            # HEY you know we ourselves could import _teem here and check this assumption ...
#    elif 'NULL' == tv:
#        ret = f'_teem.ffi.NULL == {rvName}'
#    elif 'AIR_NAN' == tv:
#        ret = f'_math.isnan({rvName})'
#    else:
#        # this function is super adhoc-y, and will definitely require future expansion
#        raise Exception(f"sorry don't yet know how to handle typ={typ}, tv={tv}")
#    return ret


def doAnnote(oFile, funcName, retQT, annoteCmt, fnln):
    qts = retQT.split(' ')
    # remove any comment within annotation
    if '#' in annoteCmt:
        annote = annoteCmt.split('#')[0].strip()
    else:
        annote = annoteCmt
    anlist = annote[6:].split(' ')  # remove "Biff: ", and split into list
    # For Python-wrapping the Teem API, we're ignoring some things:
    if 'static' in qts:
        # function not accessible anyway in the libteem library
        return
    if 'nope' in anlist:
        # function doesn't use biff, so nothing for wrapper to do w.r.t biff
        return
    if '(private)' in anlist:
        # wrapper is around public API, so nothing for this either
        # (this covers some of the more far-out uses of Biff annotations, so
        # including private functions will require more work on rvtest() above)
        return
    if 1 != len(anlist):
        raise Exception(f'got multiple words {anlist} (from "{annoteCmt}") but expected 1')
    anval = anlist[0]
    if anval.startswith('maybe:'):
        mlist = anval.split(':')  # mlist[0]='maybe:'
        mubi = mlist[1]  # for Maybe, useBiff index (1-based)
        anval = mlist[2]  # returned value(s) to check
    else:
        mubi = 0
    if not (match := re.match(r'_*(.+?)[^a-z]', funcName)):
        raise Exception(f'couldn\'t extract library name from function name "{funcName}"')
    bkey = match.group(1)
    if not bkey in TLIBS:
        raise Exception(
            f'apparent library name prefix "{bkey}" of function "{funcName}" not in Teem library list {TLIBS}'
        )
    # NOTE: The code above is useful logic for anything seeking to use the Biff annotations,
    # and wanting some self-contained repackaging of their info, so we store our processing of
    # it in a simple .csv file that can be parsed and used for the Python/CFFI wrappers, as
    # well as maybe eventually for other languages (e.g. Julia):
    print(f'{funcName},{" ".join(qts)},{anval},{mubi},{bkey},{fnln}', file=oFile)
    # old code:
    ##  This code could
    ##  be later expanded to generate analogous dictionaries useful for
    ## other wrappings (e.g. Julia).
    ## set up, generated by us:
    ##    biffDict['func'] = (rvtf, mubi, bkey, fnln)
    ## later as used by teem.py:
    ##    rv = _teem.lib.func(*args)
    ##    if ['func'] in biffDict:
    ##       (_rvtf, _mubi, _bkey, _fnln) = _BIFFDICT['func']
    ## and in wrapper function for 'func':
    ##    if _rvtf(rv) and (0 == _mubi or args[_mubi-1]):
    ##       biffGetDone(bkey)
    # rvtf = '(lambda rv: ' + ' or '.join([rvtest(qts, V, 'rv') for V in anval.split('|')]) + ')'
    # if '(lambda rv: 1 == rv)' == rvtf:
    #    rvtf = '_equals1'
    # elif '(lambda rv: _math.isnan(rv))' == rvtf:
    #    rvtf = '_math.isnan'
    ## finally, write dict entry for handling errors in funcName
    # print(f"    '{funcName}': ({rvtf}, {mubi}, b'{bkey}', '{fnln}'),", file=oFile)


def doSrc(oFile, sFile, sFN):
    lines = [line.strip() for line in sFile.readlines()]
    nlen = len(lines)
    idx = 0
    while idx < nlen:
        # scan lines of source file, looking for Biff: annotations
        if match := re.match(r'(.+?)\/\* (Biff: .+?)\*\/', lines[idx]):
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
    idx0 = 1 + lines.index(f'set({lib.upper()}_SOURCES')
    idx1 = lines.index(')')
    srcs = filter(lambda fn: fn.endswith('.c'), lines[idx0:idx1])
    for FN in srcs:
        if VERB > 1:
            print(f'... {lib}/{FN}')
        with open(f'{sPath}/{FN}') as sFile:
            doSrc(oFile, sFile, f'{lib}/{FN}')


def parse_args():
    # https://docs.python.org/3/library/argparse.html
    parser = argparse.ArgumentParser(
        description='Scans "Biff:" annotations, and generates '
        '.csv files that represent the same information',
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )
    # formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument(
        '-v',
        metavar='verbosity',
        type=int,
        default=1,
        required=False,
        help='verbosity level (0 for silent)',
    )
    parser.add_argument(
        '-o',
        metavar='outdir',
        default='../../python/cffi/biffdata',
        help='output directory in which to put per-library .csv files',
    )
    parser.add_argument(
        'teem_source', help='path of Teem source checkout with "src" and "arch" subdirs'
    )
    # we always do all the libraries (that might use biff)
    # regardless of "experimental"
    return parser.parse_args()


if __name__ == '__main__':
    args = parse_args()
    VERB = args.v
    argsCheck(args.o, args.teem_source)
    for lib in TLIBS:
        if VERB:
            print(f'processing library {lib} ...')
        ofn = args.o + f'/{lib}.csv'
        with open(ofn, 'w') as oFile:
            doLib(oFile, args.teem_source, lib)
        if VERB:
            print(f' ... wrote {ofn}')
