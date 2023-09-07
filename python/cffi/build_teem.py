#!/usr/bin/env python
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
Once CMake has created the libteem shared library (teem-install/lib/libteem.{so,dylib}
for install directory teem-install), you use this program to generate the "_teem"
module that wraps around libteem. Run "build_teem.py --help" for more info.
GLK welcomes suggestions on how to refactor to avoid too-many-branches and too-many-statements
warnings from from pylint.
"""
import argparse
import re
import sys

import exult

# halt if python2; thanks to https://stackoverflow.com/a/65407535/1465384
_x, *_y = 1, 2  # NOTE: A SyntaxError means you need Python3, not Python2
del _x, _y

# learned:
# The C parser used by CFFI is meagre, and can generate obscure error message
# e.g. unrrdu.h use to declare all the unrrdu_fooCmd structs, with meta-macro use
#   UNRRDU_MAP(UNRRDU_DECLARE)
# and this led to error:
#   File ". . ./site-packages/pycparser/plyparser.py", line 67, in _parse_error
#     raise ParseError("%s: %s" % (coord, msg))
#   pycparser.plyparser.ParseError: <cdef source string>: At end of input
# (but this was made moot by moving this macro use to a private header)

# global control on verbosity level
VERB = 0


def proc_line(line: str) -> str:
    """
    Initial processing of single line of a header file, returning empty string for any line
    that doesn't help cffi's cdef, and transforming the ones that do.
    """
    empty = False
    macro_start = r'^#define +\S+ *\([^\)]+\) +\S*?\([^\)]+?\).*?$'
    define_a = r'^#define +\S+ +\(.*?\)$'
    define_b = r'^#define +\S+ +\([^\(\)]*?\([^\(\)]*?\)[^\(\)]*?\)$'
    # drop the include guards       and    drop any include directives
    if line.find('HAS_BEEN_INCLUDED') >= 0 or re.match('^# *include ', line):
        empty = True
    # drop any #defines of strings
    elif re.match(r'^#define +\S+ +"[^"]+"', line):
        if VERB >= 2:
            print(f'dropping #define of string "{line}"')
        empty = True
    elif re.match(r'^#define +\S+ +\'.\'', line):
        if VERB >= 2:
            print(f'dropping #define of character "{line}"')
        empty = True
    # drop #defines of (some) floating-point values
    elif re.match(r'^#define +\S+ +[0-9]+\.[0-9]+', line):
        if VERB >= 2:
            print(f'dropping #define of float "{line}"')
        empty = True
    # drop #defines of (some) non-numeric things (e.g. #define NRRD nrrdBiffKey)
    elif re.match(r'^#define +\S+ +[a-zA-Z_]+$', line):
        if VERB >= 2:
            print(f'dropping #define of symbol "{line}"')
        empty = True
    # drop one-line macro #defines
    # (multi-line macro #defines handled by unmacro)
    elif re.match(macro_start, line) and not line.endswith('\\'):
        if VERB >= 2:
            print(f'dropping one-line #define macro "{line}"')
        empty = True
    # drop #defines of (some) parenthesized expressions
    elif re.match(define_a, line) or re.match(define_b, line):
        if VERB >= 2:
            print(f'dropping (other) #define "{line}"')
        empty = True
    elif re.match('^#define', line):
        if '(' in line:
            if VERB >= 2:
                print(f'dropping other (other) #define "{line}"')
            empty = True
        elif VERB >= 3:
            print(f'KEEPing #define "{line}"')
    if empty:
        ret = ''
    else:
        # transform AIR_EXPORT, BIFF_EXPORT, etc into extern
        ret = re.sub(r'^[A-Z]+_EXPORT ', 'extern ', line)
    return ret


def unmacro(lines: list[str]) -> list[str]:
    """
    Returns a copy of input list of strings lines, except with multi-line #defines removed.
    """
    olines = []
    copying = True
    for line in lines:
        if re.match(r'^#define +.*\\$', line):
            # start of multi-line macro
            if VERB >= 2:
                print(f'start of multi-line macro: "{line}"')
            copying = False
            continue
        if not copying:
            if re.match(r'.*?\\$', line):
                if VERB >= 2:
                    print(f'        ... more of macro: "{line}"')
            else:  # after starting macro, got to line not ending with '\'
                if VERB >= 2:
                    print(f'         ... end of macro: "{line}"')
                copying = True
            continue
        if copying:
            olines.append(line)
    return olines


def drop_at_match(rgx: str, num: int, lines: list[str]) -> int:
    """From list of strings lines, drop num lines starting with first match to rgx"""
    idx = -1
    for idx, line in enumerate(lines):
        if re.match(rgx, line):
            break
    if -1 == idx:
        raise Exception(f'found regex "{rgx}" nowhere in lines')
    for _ in range(num):
        lines.pop(idx)
    return idx


def drop1(bye: str, lines: list[str]) -> None:
    """From list of strings lines, drop first occurance of bye"""
    lines.pop(lines.index(bye))


def drop_at(bye: str, num: int, lines: list[str]) -> int:
    """From list of strings lines, drop num lines starting with first occurance of bye"""
    idx = lines.index(bye)
    for _ in range(num):
        lines.pop(idx)
    return idx


def drop_at_all(bye: str, num: int, lines: list[str]) -> None:
    """From list of strings lines, drop sets of num lines, each starting with bye"""
    while (idx := lines.index(bye) if bye in lines else -1) >= 0:
        for _ in range(num):
            lines.pop(idx)


def proc_hdr(fout, fin, hname: str) -> None:  # out, fin: files
    """
    Write to file out the results of processing header file hfin
    """
    if VERB:
        print(f'proc_hdr({hname}) ...')
    # read all lines from hfin, strip newlines (and trailing whitespace)
    lines = [line.rstrip() for line in fin.readlines()]
    # remove multiline macros
    lines = unmacro(lines)
    # initial pass of processing lines individually, and keeping only good ones
    lines = list(filter(None, [proc_line(line) for line in lines]))
    # remove sets of 3 lines, starting with '#ifdef __cplusplus'
    drop_at_all('#ifdef __cplusplus', 3, lines)
    # remove sets of 9 lines that define <LIB>_EXPORT
    drop_at_all('#if defined(_WIN32) && !defined(__CYGWIN__) && !defined(TEEM_STATIC)', 9, lines)
    if hname == 'air.h':  # handling specific to air.h
        # air.h has a set of 15 lines around airLLong, airULLong typedefs
        # this currently signals it's start, though (HEY) seems fragile
        idx = drop_at(
            '#if defined(_WIN32) && !defined(__CYGWIN__) && !defined(__MINGW32__)', 15, lines
        )
        # restore definitions that seem to work for mac/linux
        lines.insert(idx, 'typedef signed long long airLLong;')
        lines.insert(idx, 'typedef unsigned long long airULLong;')
        # air.h's inclusion of teem/airExistsConf.h moot for cdef
        drop_at('#if !defined(TEEM_NON_CMAKE)', 3, lines)
        # drop AIR_EXISTS definition
        drop_at(
            '#if defined(_WIN32) || defined(__ECC) || '
            'defined(AIR_EXISTS_MACRO_FAILS) /* NrrdIO-hack-002 */',
            5,
            lines,
        )
    elif hname == 'biff.h':
        # drop the attribute directives about biff like printf
        drop_at_all('#ifdef __GNUC__', 3, lines)
    elif hname == 'nrrd.h':
        # drop control of nrrdResample_t (no effect on API)
        drop_at('#if 0 /* float == nrrdResample_t; */', 9, lines)
    elif hname == 'alan.h':
        idx = drop_at('#if 1 /* float == alan_t */', 9, lines)
        lines.insert(idx, 'typedef float alan_t;')
    elif hname == 'bane.h':
        lines.remove('BANE_GKMS_MAP(BANE_GKMS_DECLARE)')
    elif hname == 'limn.h':
        lines.remove('LIMN_MAP(LIMN_DECLARE)')
    elif hname == 'echo.h':
        idx = drop_at('#if 1 /* float == echoPos_t */', 7, lines)
        lines.insert(idx, 'typedef float echoPos_t;')
        idx = drop_at('#if 1 /* float == echoCol_t */', 7, lines)
        lines.insert(idx, 'typedef float echoCol_t;')
        # unmacro removed the multi-line ECHO_OBJECT_MATTER macro, but its contents are needed
        # to complete the struct definitions (we're doing the pre-processor work)
        matdef = (
            'unsigned char matter; echoCol_t rgba[4]; '
            'echoCol_t mat[ECHO_MATTER_PARM_NUM]; Nrrd *ntext'
        )
        lines = [line.replace('ECHO_OBJECT_MATTER', matdef) for line in lines]
    elif hname == 'ten.h':
        lines.remove('TEND_MAP(TEND_DECLARE)')
    elif hname == 'pull.h':
        # here, there really are #define controls that do change what's visible in the structs
        # so we have to implement the action of the pre-processor
        pcntl = [{'id': 'HINTER'}, {'id': 'TANCOVAR'}, {'id': 'PHIST'}]
        for pcc in pcntl:
            pcc['on'] = any(re.match(f"^#define PULL_{pcc['id']} *1$", line) for line in lines)
            if not pcc['on'] and not any(
                re.match(f"^#define PULL_{pcc['id']} *0$", line) for line in lines
            ):
                raise Exception(f"did not see #define PULL_{pcc['id']} in expected form in pull.h")
            drop_at_match(f"^#define PULL_{pcc['id']}", 1, lines)
        olines = []
        copying = True
        for line in lines:
            ifs = [line == f"#if PULL_{pcc['id']}" for pcc in pcntl]
            if any(ifs):
                # line is start of an #if/#endif pair we recognize
                copying = pcntl[ifs.index(True)]['on']
                olines.append(f'/* {line} */')
                continue
            if line == '#endif':
                # line is end of an #if/#endif pair (hopefully not nested!)
                copying = True
                olines.append(f'/* {line} */')
                continue
            # else not delimiting an #if/#endif pair: either outside or inside one
            if copying:
                olines.append(line)
        lines = olines
    elif hname == 'coil.h':
        idx = drop_at('#if 1 /* float == coil_t */', 9, lines)
        lines.insert(idx, 'typedef float coil_t;')
    elif hname == 'mite.h':
        idx = drop_at('#if 0 /* float == mite_t */', 10, lines)
        lines.insert(idx, 'typedef double mite_t;')
    elif hname == 'meet.h':
        # ideally these would be handled as delimiting pairs, but oh well
        drop_at_all('#if defined(TEEM_BUILD_EXPERIMENTAL_LIBS)', 1, lines)
        drop_at_all('#endif', 1, lines)
    # end of per-library-specific stuff
    for line in lines:
        fout.write(f'{line}\n')


def cdef_write(cdef_path: str, hdr_path: str, libs: list[str]) -> None:
    """
    creates cdef/cdef_LIB.h for all LIB in this Teem installation
    """
    for lib in libs:
        if VERB:
            print(f'#################### writing cdef/cdef_{lib}.h ...')
        with open(f'{cdef_path}/cdef_{lib}.h', 'w', encoding='utf-8') as out:
            out.write(
                f"""
/* NOTE: This file is a *very* hacked up version of the original
teem/{lib}.h, generated by build_teem.py to declare the {lib} API to
CFFI, within its many limitations, specifically lacking a C pre-processor
(so no #include directives, and only certain #defines). */
 """
            )
            for idx, hdr in enumerate(exult.tlib_headers(lib)):
                if idx:
                    out.write('\n\n')
                out.write(f'/* =========== {hdr} =========== */\n')
                with open(f'{hdr_path}/teem/{hdr}', 'r', encoding='utf-8') as hfin:
                    proc_hdr(out, hfin, hdr)


def parse_args():
    """
    Set up and run argparse command-line parser
    """
    # https://docs.python.org/3/library/argparse.html
    parser = argparse.ArgumentParser(
        description='Utility for compiling CFFI-based python3 extension module around '
        'Teem shared library'
    )
    parser.add_argument(
        '-v',
        metavar='verbosity',
        type=int,
        default=1,
        required=False,
        help='verbosity level (0 for silent)',
    )
    parser.add_argument(
        '-gch',
        action='store_true',
        default=False,
        required=False,
        help='Generate (possibly over-writing) the ./cdef/cdef_*.h headers and then '
        'quit. Otherwise (by default), existing headers are read and used to create the '
        '_teem extension module. This is mainly used during Teem development and prior '
        "to Teem releases, but isn't needed for users to create their own _teem "
        'extension module.',
    )
    on_mac = sys.platform.startswith('darwin')
    parser.add_argument(
        '-int',
        action='store_true',
        default=False,
        help=(
            'On Macs only: '
            f"{'' if on_mac else '(and this is not a Mac, so it will have no effect) '}"
            'after creating cffi extension library, store in it the explicit '
            'path libteem.dylib, using install_name_tool.'
        ),
    )
    parser.add_argument(
        '-wrap',
        action='store_true',
        default=False,
        required=False,
        help='Do not run .cdef(), .set_source(), or .compile() steps in CFFI wrapper to generate '
        'the _teem extension module; ONLY run .wrap() to generate the teem.py wrapper around an '
        'already-compiled _teem extension module. Without -wrap, the _teem extension module is '
        'compiled anew (which is slow), and then the teem.py wrapper is made around it.',
    )
    parser.add_argument(
        'install_path',
        help='path into which CMake has installed Teem (should have '
        '"include" and "lib" subdirectories)',
    )
    return parser.parse_args()


if __name__ == '__main__':
    ARGS = parse_args()
    VERB = ARGS.v
    if ARGS.gch:
        (_hdr_path, _, _have_libs, _) = exult.check_path_tinst(ARGS.install_path)
        cdef_write('./cdef', _hdr_path, _have_libs)
    else:
        ffi = exult.Tffi('../..', ARGS.install_path, 'teem', VERB)
        if not ARGS.wrap:
            ffi.cdef()
            ffi.set_source()
            ffi.compile(run_int=ARGS.int)
        ffi.wrap('teem.py')
