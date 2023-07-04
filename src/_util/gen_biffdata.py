#!/usr/bin/env python3
#
# Teem: Tools to process and visualize scientific data and images
# Copyright (C) 2009--2023  University of Chicago
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

"""
Processes Teem source .c files to produce data (in csv format) about biff usage, to be used for
wrappers around a Teem extension module. Right now the only consumer of these files is the
teem.py Python wrapper, but the generated .csv files are language-agnostic.
"""

import re
import argparse
import os

# halt if python2; thanks to https://preview.tinyurl.com/44f2beza
_x, *_y = 1, 2  # NOTE: A SyntaxError here means you need python3, not python2
del _x, _y


VERB = 1
# TEEM_LIB_LIST
TLIBS = [  # 'air', 'hest', 'biff',  (these libraries cannot not use biff, by their nature)
    # the following lists ALL the other Teem libraries. It may be that some
    # (like elf, tijk, unrrdu) do not use biff, but that is something we discover
    # now as part of our operation, rather than decreeing from the outset.
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


def proc_annote(function: str, qualtype: str, annotecomment: str) -> str:
    """
    Processes the annotation found for one function to generate one output csv string
    :param str function: the name of the function
    :param str qualtype: the qualifier(s) and return type of the function
    :param str annotecomment: whole annotation, maybe with comment, starting with "Biff: "
    """
    # NOTE: The code below is useful logic for anything seeking to use the Biff annotations,
    # and wanting some self-contained repackaging of their info, so we store our processing of
    # it in a simple .csv file that can be parsed and used for the Python/CFFI wrappers, as
    # well as maybe eventually for other languages (e.g. Julia):
    # mubi = maybe useBiff 1-based index into function parms, or 0 for always uses biff on error
    if 'HEADER' == function:
        #            0         1         2           3      4
        return 'func_name,return_type,error_val(s),mubi,biff_key'
        # but NOTE that this does not include final entry for filename:linenumber
    # else used as normall
    qtlist = qualtype.split(' ')   # list made from qualifer and type
    if not annotecomment.startswith('Biff: '):
        raise Exception(
            f'for function "{function}" annotation "{annotecomment}" does not start with "Biff: "'
        )
    # remove any comment within annotation
    if '#' in annotecomment:
        # drop the comment
        annote = annotecomment.split('#')[0].strip()
    else:
        annote = annotecomment
    anlist = annote[6:].split(' ')  # split whatever follows "Biff: " into list
    # For Python-wrapping the Teem API, we're ignoring some things:
    if 'static' in qtlist:
        # function not accessible anyway in the libteem library
        return ''
    if 'nope' in anlist:
        # function doesn't use biff, so nothing for wrapper to do w.r.t biff
        return ''
    if '(private)' in anlist:
        # intended wrapper is around public API, so nothing for this either. This covers some
        # of the more far-out uses of Biff annotations, so including private functions will
        # require more work on the Tffi.rvt_expr method in exult.py
        return ''
    if 1 != len(anlist):
        raise Exception(f'got multiple words {anlist} (from "{annotecomment}") but expected 1')
    errval = anlist[0]
    if errval.startswith('maybe:'):
        # function may or may not use biff in case of error, depending on value of useBiff param
        mlist = errval.split(':')  # mlist[0]='maybe:'
        mubi = mlist[1]  # for *M*aybe, *u*se*B*iff *i*ndex (ONE-based number of function params)
        errval = mlist[2]  # returned value(s) to check
    else:
        # function always uses biff in case of error
        mubi = 0
    if not (match := re.match(r'_*(.+?)[^a-z]', function)):
        raise Exception(f'couldn\'t extract biff key prefix from function name "{function}"')
    biffkey = match.group(1)
    if not biffkey in TLIBS:
        raise Exception(
            f'apparent library name prefix "{biffkey}" of function "{function}" '
            f'not in Teem library list {TLIBS}'
        )
    #           0            1         2       3       4
    return f'{function},{qualtype},{errval},{mubi},{biffkey}'


def proc_src(file, filename):
    """
    Process all the "Biff:" annotations found in given file (with given filename),
    and return a list of results.
    As a fun extra bonus, this checks that a biff-using function names itself with
    a static const char me[] definition that actually matches the function name,
    and describes any issues as a WARNING (this found many many such glitches)
    """
    olines = []
    ilines = [line.strip() for line in file.readlines()]
    ilnum = len(ilines)
    for (lidx, iline) in enumerate(ilines):
        # scan lines of source file, looking for Biff: annotations
        if not (match := re.match(r'(.+?)\/\* (Biff: .+?)\*\/', iline)):
            continue
        # So now: lines[lidx] aka "line {lidx+1}" has a Biff annotation, and
        # lines[lidx+1] aka "line {lidx+2}" is 1st line of function definition
        # lines[lidx+2] aka "line {lidx+3}" OR LATER is line that might define me[]
        line = ilines[lidx + 1]   # line that should have function name
        qualtype = match.group(1).strip()   # function return qualifier and type
        annote = match.group(2).strip()
        if not (match := re.match(r'(.+?)\(', line)):
            raise Exception(
                f"couldn't parse function name on line {lidx+2} of {filename}: |{line}|"
            )
        function = match.group(1)
        if oline := proc_annote(function, qualtype, annote):
            # add on the final filename:linenumber field
            olines += [oline + ',' + f'{filename}:{lidx+2}']
        # We've finished adding information about just-seen Biff annotation.
        # Now: fun extra bonus: see if me[] definition actually matches function name
        # epobidx = line index with end paren open brace ") {"; next line should have "me[]"
        # this is initialized to so first line looked at is "line" above
        epobidx = lidx + 1
        me_is_param = False
        while epobidx < ilnum:
            if re.match(r'.*const char \*me,', ilines[epobidx]):
                # oh ok, "me" is already a function parameter (as in some unrrdu-related
                # functions) so it will not also define static const char me[], so bail
                me_is_param = True
                break
            if re.match(r'.*\) {$', ilines[epobidx]):
                # great; we found the line with ") {"; we're done
                break
            # else still looking for ") {"
            epobidx += 1
        if me_is_param:
            # nothing to do for fun extra bonus; move along to look for next Biff annotation
            continue
        # else we try to work on fun extra bonus
        if not epobidx + 1 < ilnum:
            raise Exception(
                f'{filename}:{lidx+1} starts function {function} definition but '
                'never saw ") {" with line after for me[]'
            )
        # "static const char me[]" should appear on line after ") {"
        line = ilines[epobidx + 1]   # (now) line that should have me[]
        if match := re.match(r' *static const char me\[\] = "(.*?)"', line):
            if function != match.group(1):
                print(
                    f'\nWARNING: {filename}:{lidx+2} function {function} '
                    f'has different me[]="{match.group(1)}"\n'
                )
        elif match := re.match(r' *static const char .*?me\[\]', line):
            print(
                f'\nWARNING: {filename}:{lidx+3} function {function} '
                f'weird line with me[] |{line}|\n'
            )
        elif not (annote.startswith('Biff: nope') or annote.startswith('Biff: (private) nope')):
            print(
                f'\nWARNING: {filename}:{lidx+3} function {function} with annote=|{annote}| '
                f'does not seem to have me[] defined in |{line}|\n'
            )
    return olines


def proc_lib(path_teem: str, lib: str) -> list[str]:
    """
    From Teem source checkout at path_teem, for Teem library lib, generate lines of csv data
    about Biff annotations
    """
    path_srcs = f'{path_teem}/src/{lib}'
    # read the CMakeLists.txt file to get list of source files
    with open(f'{path_srcs}/CMakeLists.txt', 'r', encoding='utf8') as cmfile:
        ilines = [line.strip() for line in cmfile.readlines()]
    idx0 = ilines.index(f'set({lib.upper()}_SOURCES')
    idx1 = ilines.index(')')
    filenames = filter(lambda fn: fn.endswith('.c'), ilines[idx0 + 1 : idx1])
    olines = []
    for filename in filenames:
        if VERB > 1:
            print(f'... {lib}/{filename}')
        with open(f'{path_srcs}/{filename}', 'r', encoding='utf8') as file:
            olines += proc_src(file, f'{lib}/{filename}')
    if olines:
        olines.insert(0, proc_annote('HEADER', '', '') + ',filename:linenumber')
    return olines


def parse_args():
    """
    Create and run command-line option parser
    """
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


def check_args(args) -> None:
    """Checks command-line args"""
    if not os.path.isdir(args.o):
        raise Exception(f'Need output {args.o} to be directory')
    teemsrc = args.teem_source
    if not (
        os.path.isdir(teemsrc)
        and os.path.isdir(f'{teemsrc}/arch')
        and os.path.isdir(f'{teemsrc}/src')
    ):
        raise Exception(f'Need Teem source {teemsrc} to be dir with "arch" and "src" subdirs')
    return args


if __name__ == '__main__':
    ARGS = check_args(parse_args())
    VERB = ARGS.v
    for LIB in TLIBS:
        if VERB:
            print(f'processing library {LIB} ...')
        # bd = biffdata
        if not (bdlines := proc_lib(ARGS.teem_source, LIB)):
            if VERB:
                print(' ... (no Biff annotations found)')
            continue
        if VERB > 1:
            print(f'library {LIB} lines: {bdlines}')
        ofilename = ARGS.o + f'/{LIB}.csv'
        with open(ofilename, 'w', encoding='utf8') as ofile:
            for bdline in bdlines:
                ofile.write(f'{bdline}\n')
        if VERB:
            print(f' ... wrote {ofilename}')
