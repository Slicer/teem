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

"""
Once CMake has created the libteem shared library (teem-install/lib/libteem.{so,dylib}
for install directory teem-install), you use this program to generate the "_teem"
module that wraps around libteem. Run "build_teem.py --help" for more info.
GLK welcomes suggestions on how to refactor to avoid too-many-branches and too-many-statements
warnings from from pylint.
"""
import os
import sys
import argparse
import re
import cffi

# halt if python2; thanks to https://preview.tinyurl.com/44f2beza
_x, *_y = 1, 2  # NOTE: A SyntaxError here means you need python3, not python2
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

# Array of dicts (object literals) to list all the Teem libraries
# air hest biff nrrd ell moss unrrdu alan tijk gage dye bane
# limn echo hoover seek ten elf pull coil push mite meet
# (TEEM_LIB_LIST)
LIBS = [
    {'name': 'air', 'expr': False},  # (don't need airExistsConf.h)
    {'name': 'hest', 'expr': False},
    {'name': 'biff', 'expr': False},
    {'name': 'nrrd', 'expr': False},  # also need: nrrdEnums.h nrrdDefines.h
    {'name': 'ell', 'expr': False},  # (don't need ellMacros.h)
    {'name': 'moss', 'expr': False},
    {'name': 'unrrdu', 'expr': False},
    {'name': 'alan', 'expr': True},
    {'name': 'tijk', 'expr': True},
    {'name': 'gage', 'expr': False},
    {'name': 'dye', 'expr': False},
    {'name': 'bane', 'expr': True},
    {'name': 'limn', 'expr': False},
    {'name': 'echo', 'expr': False},
    {'name': 'hoover', 'expr': False},
    {'name': 'seek', 'expr': False},
    {'name': 'ten', 'expr': False},
    {'name': 'elf', 'expr': True},
    {'name': 'pull', 'expr': False},
    {'name': 'coil', 'expr': True},
    {'name': 'push', 'expr': True},
    {'name': 'mite', 'expr': False},
    {'name': 'meet', 'expr': False},
]


def add_extra_nrrd_h(hdrs: list[str]) -> list[str]:
    """Supplements list of header files with extra headers for nrrd"""
    idx = hdrs.index('nrrd.h')
    if idx >= 0:
        # other headers are needed to define nrrd library API
        hdrs.insert(idx, 'nrrdDefines.h')
        hdrs.insert(idx, 'nrrdEnums.h')
    return hdrs


def check_hdr_path(hdr_path: str):
    """
    Sanity check on include path hdr_path.
    Returns (exper, have_hdrs) where exper indicates if this was run on an "experimental"
    Teem build, and have_hdrs is the list of .h headers to process.
    """
    itpath = hdr_path + '/teem'
    if not os.path.isdir(itpath):
        raise Exception(f'Need {itpath} to be directory')
    base_lib_names = [L['name'] for L in filter(lambda L: not L['expr'], LIBS)]
    base_hdrs = add_extra_nrrd_h([f'{LN}.h' for LN in base_lib_names])
    expr_lib_names = [L['name'] for L in filter(lambda L: L['expr'], LIBS)]
    expr_hdrs = [f'{LN}.h' for LN in expr_lib_names]
    missing_hdrs = list(filter(lambda F: not os.path.isfile(f'{itpath}/{F}'), base_hdrs))
    if missing_hdrs:
        raise Exception(
            f'Missing header(s) {" ".join(missing_hdrs)} in {itpath} '
            'for one or more of the core Teem libs'
        )
    missing_expr_hdrs = list(filter(lambda F: not os.path.isfile(f'{itpath}/{F}'), expr_hdrs))
    have_hdrs = base_hdrs
    if missing_expr_hdrs:
        # missing one or more of the non-core "Experimental" header files
        if len(missing_expr_hdrs) < len(expr_hdrs):
            raise Exception(
                'Missing some (but not all) non-core header(s) '
                f'{" ".join(missing_expr_hdrs)} in {itpath} for one or more of the '
                'core Teem libs'
            )
        # else len(missing_expr_hdrs) == len(expr_hdrs)) aka all missing, ok, so
        # not Experimental
        if VERB:
            print('(Teem build does *not* appear include "Experimental" libraries)')
    else:
        # it is Experimental; reform the header list in dependency order (above)
        have_hdrs = add_extra_nrrd_h([f"{L['name']}.h" for L in LIBS])
        if VERB:
            print('(Teem build includes "Experimental" libraries)')
    return (not missing_expr_hdrs, have_hdrs)


def check_lib_path(lib_path: str) -> None:
    """
    Sanity checks on libaray lib_path.
    May throw various exceptions but returns nothing.
    """
    if sys.platform == 'darwin':  # Mac
        shext = 'dylib'
    elif sys.platform == 'linux':
        shext = 'so'
    else:
        raise Exception(
            'Sorry, currently only know how work on Mac and Linux ' '(not {sys.platform})'
        )
    lib_fnames = os.listdir(lib_path)
    if not lib_fnames:
        raise Exception(f'Teem library dir {lib_path} seems empty')
    ltname = f'libteem.{shext}'
    if not ltname in lib_fnames:
        raise Exception(
            f'Teem library dir {lib_path} contents {lib_fnames} do not seem to include '
            f'required {ltname} shared library, which means running '
            'cffi.FFI().compile() later will not produce a working wrapper, even if '
            'it finishes without error.'
        )


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
            '#if defined(_WIN32) && !defined(__CYGWIN__) ' '&& !defined(__MINGW32__)', 15, lines
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


def build(path: str):
    """
    Main function: creates cdef_teem.h then feeds it to cffi.FFI() to compile the
    _teem Python module that links into the libteem shared library.
    """
    path = path.rstrip('/')
    hdr_path = path + '/include'
    lib_path = path + '/lib'
    if not os.path.isdir(hdr_path) or not os.path.isdir(lib_path):
        raise Exception(f'Need both {hdr_path} and {lib_path} to be subdirs of teem install dir')
    check_lib_path(lib_path)
    (exper, hdrs) = check_hdr_path(hdr_path)
    if VERB:
        print('#################### writing cdef_teem.h ...')
    with open('cdef_teem.h', 'w', encoding='utf-8') as out:
        out.write(
            """
/* NOTE: This file (cdef_teem.h) is generated by build_teem.py.
 * It is NOT usable as a single "teem.h" header for all of Teem, because of
 * the many hacky transformations done to work with the limitations of the
 * CFFI C parser, specifically, lacking C pre-processor (e.g., all #include
 * directives have been removed, and lots of other #defines are gone).
 * The top-level header for all of Teem is teem/meet.h */
 """
        )
        for hdr in hdrs:
            out.write(f'/* =========== {hdr} =========== */\n')
            with open(f'{hdr_path}/teem/{hdr}', 'r', encoding='utf-8') as hfin:
                proc_hdr(out, hfin, hdr)
            out.write('\n\n')
    ffibld = cffi.FFI()
    # so that teem.py can call free() as part of biff error handling
    ffibld.cdef('extern void free(void *);')
    if VERB:
        print('#################### reading cdef_teem.h ...')
    with open('cdef_teem.h', 'r', encoding='utf-8') as file:
        ffibld.cdef(file.read())
    source_args = {
        'libraries': ['teem'],
        'include_dirs': [hdr_path],
        'library_dirs': [lib_path],
        'extra_compile_args': ['-DTEEM_BUILD_EXPERIMENTAL_LIBS'] if exper else None,
        # On linux, path <dir> here is passed to -Wl,--enable-new-dtags,-R<dir>;
        # "readelf -d ....so | grep PATH" should show <dir> and "ldd .....so" should show
        # where dependencies were found.
        # On Mac, path <dir> here is passed to -Wl,-rpath,<dir>, and you can see that
        # from "otool -l ....so", in the LC_RPATH sections.
        'runtime_library_dirs': [os.path.abspath(lib_path)],
        # keep asserts()
        # https://docs.python.org/3/distutils/apiref.html#distutils.core.Extension
        'undef_macros': ['NDEBUG'],
    }
    if VERB:
        print('#################### calling set_source with ...')
        for key, val in source_args.items():
            print(f'   {key} = {val}')
    ffibld.set_source(
        '_teem', '#include <teem/meet.h>', **source_args  # this is effectively teem.h
    )
    if VERB:
        print('#################### compiling _teem (slow!) ...')
    out_path = ffibld.compile(verbose=(VERB > 0))
    if VERB:
        print(f'#################### ... compiling _teem done; created:\n{out_path}')
    # should have now created a new _teem.<platform>.so shared library
    # so should be able to, on Mac, (e.g.) "otool -L _teem.cpython-39-darwin.so"
    # or, on linux, (e.g.) "ldd _teem.cpython-38-x86_64-linux-gnu.so"
    # to confirm that this want to dynamically link to the libteem shared library
    # (something about the mac build process allows this to work even if -Wl,-rpath
    # is not passed to set_source, but this seems necessary on linux)


def parse_args():
    """
    Set up and run argparse command-line parser
    """
    # https://docs.python.org/3/library/argparse.html
    parser = argparse.ArgumentParser(
        description='Utility for compiling CFFI-based ' 'python3 module around Teem shared library'
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
        'install_path',
        help='path into which CMake has install Teem (should have '
        '"include" and "lib" subdirectories)',
    )
    return parser.parse_args()


if __name__ == '__main__':
    args = parse_args()
    VERB = args.v
    build(args.install_path)
