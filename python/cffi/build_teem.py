#!/usr/bin/env python
# halt if python2; thanks to https://preview.tinyurl.com/44f2beza
_x,*_y=1,2 # NOTE: A SyntaxError here means you need python3, not python2
del _x, _y

# Once CMake has created the libteem shared library (teem-install/lib/libteem.{so,dylib}
# for install directory teem-install), you use this program to generate the "_teem.py"
# module that wraps around libteem.
#
# Run "build_teem.py --help" for more info.
#
# With _teem.py, you should then be able to use the pre-generated teem.py to wrap around
# _teem.py; teem.py turns biff errors into exceptions.

# learned:
# The C parser used by CFFI is modest, and can generate obscure error message
# e.g. unrrdu.h use to declare all the unrrdu_fooCmd structs, with meta-macro use
#   UNRRDU_MAP(UNRRDU_DECLARE)
# and this led to error:
#   File ". . ./site-packages/pycparser/plyparser.py", line 67, in _parse_error
#     raise ParseError("%s: %s" % (coord, msg))
#   pycparser.plyparser.ParseError: <cdef source string>: At end of input
# (but this was made moot by moving this macro use to a private header)

from cffi import FFI
from os import path #, chdir
# from pathlib import Path
from sys import platform
import argparse
import re

verbose = 0
haveExpr = False

if platform == 'darwin':  # Mac
    shext = 'dylib'
elif platform == 'linux':
    shext = 'so'
else:
    raise Exception('Sorry, currently only know how work on Mac and Linux')

# Array of dicts (object literals) to list all the Teem libraries
# air hest biff nrrd ell moss unrrdu alan tijk gage dye bane limn echo hoover seek ten elf pull coil push mite meet
# (TEEM_LIB_LIST)
libs = [
    {'name': 'air',    'expr': False}, # (don't need airExistsConf.h)
    {'name': 'hest',   'expr': False},
    {'name': 'biff',   'expr': False},
    {'name': 'nrrd',   'expr': False}, # also need: nrrdEnums.h nrrdDefines.h
    {'name': 'ell',    'expr': False}, # (don't need ellMacros.h)
    {'name': 'unrrdu', 'expr': False},
    {'name': 'alan',   'expr': True},
    {'name': 'moss',   'expr': False},
    {'name': 'tijk',   'expr': True},
    {'name': 'gage',   'expr': False},
    {'name': 'dye',    'expr': False},
    {'name': 'bane',   'expr': True},
    {'name': 'limn',   'expr': False},
    {'name': 'echo',   'expr': False},
    {'name': 'hoover', 'expr': False},
    {'name': 'seek',   'expr': False},
    {'name': 'ten',    'expr': False},
    {'name': 'elf',    'expr': True},
    {'name': 'pull',   'expr': False},
    {'name': 'coil',   'expr': True},
    {'name': 'push',   'expr': True},
    {'name': 'mite',   'expr': False},
    {'name': 'meet',   'expr': False},
]

def check_path(iPath, lPath):
    global haveExpr
    if (not path.isdir(iPath) or
        not path.isdir(lPath)):
        raise Exception(f'Need both {iPath} and {lPath} to be subdirs of teem install dir')
    itPath = iPath + '/teem'
    if (not path.isdir(itPath)):
        raise Exception(f'Need {itPath} to be directory')
    baseLibNames = [L['name'] for L in filter(lambda L: not L['expr'], libs)]
    exprLibNames = [L['name'] for L in filter(lambda L:     L['expr'], libs)]
    baseHdrs = [f'{LN}.h' for LN in baseLibNames]
    if ('nrrd.h' in baseHdrs):
        # other headers are needed to define nrrd library API
        baseHdrs.insert(baseHdrs.index('nrrd.h'), 'nrrdDefines.h')
        baseHdrs.insert(baseHdrs.index('nrrd.h'), 'nrrdEnums.h')
    exprHdrs = [f'{LN}.h' for LN in exprLibNames]
    missingHdrs = [H for H in filter(lambda F: not path.isfile(f'{itPath}/{F}'), baseHdrs)]
    if (len(missingHdrs)):
        raise Exception(f"Missing header(s) {' '.join(missingHdrs)} in {itPath} "
                        + "for one or more of the core Teem libs")
    missingExprHdrs = [H for H in filter(lambda F: not path.isfile(f'{itPath}/{F}'), exprHdrs)]
    haveHdrs = baseHdrs
    if (len(missingExprHdrs)):
        # missing one or more of the non-core "Experimental" header files
        if (len(missingExprHdrs) < len(exprHdrs)):
            raise Exception("Missing some (but not all) non-core header(s) "
                            + f"{' '.join(missingExprHdrs)} in {itPath} for one or more of the "
                            + "core Teem libs")
        # else len(missingExprHdrs) == len(exprHdrs)) aka all missing, ok, so not Experimental
        # (haveExpr initialized to False above)
        if (verbose):
            print('(Teem build does *not* appear include "Experimental" libraries)')
    else:
        # it is Experimental; reform the header list in dependency order (above)
        haveExpr = True
        haveHdrs = [f"{L['name']}.h" for L in libs]
        # HEY stupid copy-and-paste
        if ('nrrd.h' in haveHdrs):
            # other headers are needed to define nrrd library API
            haveHdrs.insert(haveHdrs.index('nrrd.h'), 'nrrdDefines.h')
            haveHdrs.insert(haveHdrs.index('nrrd.h'), 'nrrdEnums.h')
        if (verbose):
            print('(Teem build includes "Experimental" libraries)')
    return haveHdrs

def procLine(L):
    # drop the include guards
    if L.find('HAS_BEEN_INCLUDED') >= 0:
        return False
    # drop any include directives
    if re.match('^# *include ', L):
        return False
    # drop any #defines of strings
    if (re.match(r'^#define +\S+ +"[^"]+"', L)):
        if (verbose >= 2): print(f'dropping #define of string "{L}"')
        return False
    if (re.match(r'^#define +\S+ +\'.\'', L)):
        if (verbose >= 2): print(f'dropping #define of character "{L}"')
        return False
    # drop #defines of (some) floating-point values
    if re.match(r'^#define +\S+ +[0-9]+\.[0-9]+', L):
        if (verbose >= 2): print(f'dropping #define of float "{L}"')
        return False
    # drop #defines of (some) non-numeric things (e.g. #define NRRD nrrdBiffKey)
    if re.match(r'^#define +\S+ +[a-zA-Z_]+$', L):
        if (verbose >= 2): print(f'dropping #define of symbol "{L}"')
        return False
    # drop one-line macro #defines
    # (multi-line macro #defines handled by unmacro)
    if re.match(r'^#define +\S+ *\([^\)]+\) +\S*?\([^\)]+?\).*?$', L) and not L.endswith('\\'):
        if (verbose >= 2): print(f'dropping one-line #define macro "{L}"')
        return False
    # drop #defines of (some) parenthesized expressions
    if (   re.match(r'^#define +\S+ +\(.*?\)$', L)
        or re.match(r'^#define +\S+ +\([^\(\)]*?\([^\(\)]*?\)[^\(\)]*?\)$', L)):
        if (verbose >= 2): print(f'dropping (other) #define "{L}"')
        return False
    if (re.match(f'^#define', L)):
        if ('(' in L):
            if (verbose >= 2): print(f'dropping other (other) #define "{L}"')
            return False
        if (verbose >= 3): print(f'KEEPing #define "{L}"')
    # transform AIR_EXPORT, BIFF_EXPORT, etc into extern
    L = re.sub(r'^[A-Z]+_EXPORT ', 'extern ', L)
    return L

def unmacro(lines):
    olines = []
    copying = True
    for L in lines:
        if re.match(r'^#define +.*\\$', L):
            # start of multi-line macro
            if (verbose >= 2):     print(f'start of multi-line macro: "{L}"')
            copying = False
            continue
        elif not copying:
            if re.match(r'.*?\\$', L):
                if (verbose >= 2): print(f'        ... more of macro: "{L}"')
                pass
            else: # after starting macro, got to line not ending with '\'
                if (verbose >= 2): print(f'         ... end of macro: "{L}"')
                copying = True
            continue
        if copying:
            olines.append(L)
    return olines

def dropAtMatch(rgx, N, lines):
    found = False
    for idx in range(len(lines)):
        if re.match(rgx, lines[idx]):
            found = True
            break
    if (not found):
        raise Exception(f'found regex "{rgx}" nowhere in lines')
    for _ in range(N):
        lines.pop(idx)
    return idx

def drop1(str, lines):
    lines.pop(lines.index(str))

def dropAt(str, N, lines):
    idx = lines.index(str)
    for _ in range(N):
        lines.pop(idx)
    return idx

def dropAtAll(str, N, lines):
    while (idx := lines.index(str) if str in lines else -1) >= 0:
        for _ in range(N):
            lines.pop(idx)

def hdrProc(out, hf, hn):
    if verbose:
        print(f'hdrProc({hn}) ...')
    # read all lines from hf, strip newlines (and trailing whitespace)
    lines = [line.rstrip() for line in hf.readlines()]
    # remove multiline macros
    lines = unmacro(lines)
    # initialize #defines dict
    defs = {}
    # initial pass of processing lines individually, and keeping only good ones
    lines = [L for L in filter(None, [procLine(L) for L in lines])]
    # remove sets of 3 lines, starting with '#ifdef __cplusplus'
    dropAtAll('#ifdef __cplusplus', 3, lines)
    # remove sets of 9 lines that define <LIB>_EXPORT
    dropAtAll('#if defined(_WIN32) && !defined(__CYGWIN__) && !defined(TEEM_STATIC)', 9, lines)
    if (hn == 'air.h'): # handling specific to air.h
        # air.h has a set of 15 lines around airLLong, airULLong typedefs
        # this currently signals it's start, though (HEY) seems fragile
        idx = dropAt('#if defined(_WIN32) && !defined(__CYGWIN__) && !defined(__MINGW32__)', 15, lines)
        # restore definitions that seem to work for mac/linux
        lines.insert(idx, 'typedef signed long long airLLong;')
        lines.insert(idx, 'typedef unsigned long long airULLong;')
        # air.h's inclusion of teem/airExistsConf.h moot for cdef
        dropAt('#if !defined(TEEM_NON_CMAKE)', 3, lines)
        # drop AIR_EXISTS definition
        dropAt('#if defined(_WIN32) || defined(__ECC) || defined(AIR_EXISTS_MACRO_FAILS) /* NrrdIO-hack-002 */',
               5, lines)
    if (hn == 'biff.h'):
        # drop the attribute directives aboout biff like printf
        dropAtAll('#ifdef __GNUC__', 3, lines)
    if (hn == 'nrrd.h'):
        # drop control of nrrdResample_t (no effect on API)
        dropAt('#if 0 /* float == nrrdResample_t; */', 9, lines)
    if (hn == 'alan.h'):
        idx = dropAt('#if 1 /* float == alan_t */', 9, lines)
        lines.insert(idx, 'typedef float alan_t;')
    if (hn == 'bane.h'):
       lines.remove('BANE_GKMS_MAP(BANE_GKMS_DECLARE)')
    if (hn == 'limn.h'):
       lines.remove('LIMN_MAP(LIMN_DECLARE)')
    if (hn == 'echo.h'):
        idx = dropAt('#if 1 /* float == echoPos_t */', 7, lines)
        lines.insert(idx, 'typedef float echoPos_t;')
        idx = dropAt('#if 1 /* float == echoCol_t */', 7, lines)
        lines.insert(idx, 'typedef float echoCol_t;')
        # unmacro removed the multi-line ECHO_OBJECT_MATTER macro, but its contents are needed
        # to complete the struct definitions (we're doing the pre-processor work)
        matdef = 'unsigned char matter; echoCol_t rgba[4]; echoCol_t mat[ECHO_MATTER_PARM_NUM]; Nrrd *ntext'
        lines = [L.replace('ECHO_OBJECT_MATTER', matdef) for L in lines]
    if (hn == 'ten.h'):
        lines.remove('TEND_MAP(TEND_DECLARE)')
    if (hn == 'pull.h'):
        # here, there really are #define controls that do change what's visible in the structs
        # so we have to implement the action of the pre-processor
        cntl = [
            {'id': 'HINTER'},
            {'id': 'TANCOVAR'},
            {'id': 'PHIST'}
        ]
        for cc in cntl:
            cc['on'] = any([re.match(f"^#define PULL_{cc['id']} *1$", L) for L in lines])
            if (not cc['on'] and not any([re.match(f"^#define PULL_{cc['id']} *0$", L) for L in lines])):
                raise Exception(f"did not see #define PULL_{cc['id']} in expected form in pull.h")
            dropAtMatch(f"^#define PULL_{cc['id']}", 1, lines)
        olines = []
        copying = True
        for L in lines:
            ifs = [L == f"#if PULL_{cc['id']}" for cc in cntl]
            if any(ifs):
                # L is start of an #if/#endif pair we recognize
                copying = cntl[ifs.index(True)]['on']
                olines.append(f'/* {L} */')
                continue
            elif L == '#endif':
                # L is end of an #if/#endif pair (hopefully not nested!)
                copying = True;
                olines.append(f'/* {L} */')
                continue
            # else not delimiting an #if/#endif pair: either outside or inside one
            if copying:
                olines.append(L)
        lines = olines
    if (hn == 'coil.h'):
        idx = dropAt('#if 1 /* float == coil_t */', 9, lines)
        lines.insert(idx, 'typedef float coil_t;')
    if (hn == 'mite.h'):
        idx = dropAt('#if 0 /* float == mite_t */', 10, lines)
        lines.insert(idx, 'typedef double mite_t;')
    if (hn == 'meet.h'):
        # ideally these would be handled as delimiting pairs, but oh well
        dropAtAll('#if defined(TEEM_BUILD_EXPERIMENTAL_LIBS)', 1, lines)
        dropAtAll('#endif', 1, lines)
    # end of per-library-specific stuff
    for L in lines:
        out.write(f'{L}\n')

def build(path):
    path = path.rstrip('/')
    iPath = path + '/include'
    lPath = path + '/lib'
    hdrs = check_path(iPath, lPath)
    if (verbose):
        print("#################### writing cdef_teem.h ...")
    with open('cdef_teem.h', 'w') as out:
        out.write('/* NOTE: This file is automatically generated by build_teem.py.\n')
        out.write(' * It is NOT usable as a single "teem.h" header for all of Teem, because of\n')
        out.write(' * the many hacky transformations done to work with the limitations of the\n')
        out.write(' * CFFI C parser, specifically, lacking C pre-processor (e.g., all #include\n')
        out.write(' * directives have been removed, and lots of other #defines are gone).\n')
        out.write(' * The top-level header for all of Teem is teem/meet.h */\n')
        for hh in hdrs:
            out.write(f'/* =========== {hh} =========== */\n')
            with open(f'{iPath}/teem/{hh}') as hf:
                hdrProc(out, hf, hh)
            out.write(f'\n\n')
    ffibld = FFI()
    ffibld.set_source('_teem',
                      '#include <teem/meet.h>',
                      libraries=['teem'],  # HEY? need png, z, etc?
                      include_dirs=[iPath],
                      library_dirs=[lPath],
                      extra_compile_args=(['-DTEEM_BUILD_EXPERIMENTAL_LIBS'] if haveExpr else None),

                      # HEY?
                      # this module will only be used in this here directory
                      extra_link_args=['-Wl,-rpath,.'],
                      # HEY needed?
                      # https://docs.python.org/3/distutils/apiref.html#distutils.core.Extension
                      undef_macros = [ "NDEBUG" ], # keep asserts()
    )
    ## so that teem.py can call free()
    ffibld.cdef('extern void free(void *);')
    if (verbose):
        print("#################### reading cdef_teem.h ...")
    with open('cdef_teem.h', 'r') as file:
        ffibld.cdef(file.read())
    if (verbose):
        print("#################### compiling (slow!) ...")
    ffibld.compile(verbose=(verbose > 0))
    if (verbose):
        print("#################### ... done.")

def parse_args():
    # https://docs.python.org/3/library/argparse.html
    parser = argparse.ArgumentParser(description='Utility for compiling CFFI-based '
                                     'python3 module around Teem shared library')
    parser.add_argument('-v', metavar='verbosity', type=int, default=1, required=False,
                        help='verbosity level (0 for silent)')
    parser.add_argument('install_path',
                        help='path into which CMake has install Teem (should have '
                        '\"include\" and \"lib\" subdirectories)')
    return parser.parse_args()

if __name__ == '__main__':
    args = parse_args()
    verbose = args.v
    build(args.install_path)
