#!/usr/bin/env python
# halt if python2; thanks to https://preview.tinyurl.com/44f2beza
_x,*_y=1,2 # NOTE: A SyntaxError here means you need python3, not python2
del _x, _y

import os
import sys
import argparse
import subprocess
import re

# hacky script by GLK, started to check consistency of symbols in libraries and
# declarations in headers, but also does "biff auto-scan". Example usage:
#   python3 scan-symbols.py ~/teem-src nrrd
# to run scan on nrrd library, or
#   python3 scan-symbols.py ~/teem-src nrrd -biff
# to also do the biff auto-scan
# (there's nothing more "auto" in the biff auto-scan than any other scanning that
# this script does, but GLK started calling it biff auto-scan in commit messages
# so the name is stuck)

# FIX: "HEY: some air .h declares airArrayStructCB(*a,  but not in lib"
# TODO: still with curious symbols: air biff nrrd gage

verbose = 1
archDir = None
libDir = None
srcLines = {} # cache of lines of source files

# All the types (and type qualifiers) we try to parse away
# The list has to be sorted (from long to short) because things like 'int'
# will also match 'pullPoint'
allTypes = sorted(
    ['const', 'unsigned',
    'int', 'void', 'double', 'float', 'char', 'short', 'size_t', 'FILE',
    # manually generated list of Teem-derived types
    'airLLong', 'airULLong', 'airArray', 'airEnum', 'airHeap', 'airFloat',
    'airRandMTState', 'airThread', 'airThreadMutex',
    'airThreadCond', 'airThreadBarrier',
    'biffMsg',
    'hestCB', 'hestParm', 'hestOpt',
    'gzFile',
    'NrrdEncoding', 'NrrdKernel', 'NrrdFormat', 'Nrrd', 'NrrdRange', 'NrrdIoState',
    'NrrdIter', 'NrrdResampleContext', 'NrrdDeringContext', 'NrrdBoundarySpec',
    'NrrdResampleInfo', 'NrrdKernelSpec',
    'unrrduCmd',
    'alanContext',
    'mossSampler',
    'tijk_type', 'tijk_refine_rank1_parm', 'tijk_refine_rankk_parm',
    'tijk_approx_heur_parm',
    'gageItemSpec', 'gageScl3PFilter_t', 'gageKind', 'gageItemPack', 'gageShape',
    'gagePerVolume', 'gageOptimSigContext', 'gageStackBlurParm', 'gageContext',
    'dyeColor', 'dyeConverter',
    'baneRange', 'baneInc', 'baneClip', 'baneMeasr', 'baneHVolParm',
    'limnLight', 'limnCamera', 'limnWindow', 'limnObject', 'limnPolyData',
    'limnSplineTypeSpec', 'limnSpline', 'limnSplineTypeSpec', 'limnPoints', 'limnCBFPath',
    'echoRTParm', 'echoGlobalState', 'echoThreadState', 'echoScene', 'echoObject',
    '_echoRayIntxUV_t', '_echoIntxColor_t',
    'hooverContext', 'hooverRenderBegin_t', 'hooverThreadBegin_t', 'hooverRenderEnd_t',
    'hooverRayBegin_t', 'hooverSample_t', 'hooverRayEnd_t', 'hooverThreadEnd_t',
    'seekContext',
    'tenGradientParm', 'tenInterpParm', 'tenGlyphParm', 'tenEstimateContext',
    'tenEvecRGBParm', 'tenFiberSingle', 'tenFiberContext', 'tenFiberMulti', 'tenModel',
    'tenEMBimodalParm', 'tenExperSpec',
    'elfMaximaContext',
    'pullEnergy', 'pullEnergySpec', 'pullVolume', 'pullInfoSpec', 'pullContext',
    'pullTrace', 'pullTraceMulti', 'pullTask', 'pullBin', 'pullPoint',
    'coilKind', 'coilMethod', 'coilContext',
    'pushContext', 'pushEnergy', 'pushEnergySpec', 'pushBin', 'pushTask', 'pushPoint',
    'miteUser', 'miteShadeSpec', 'miteThread',
    'meetPullVol', 'meetPullInfo',
    ], key=len, reverse=True)

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
    runthis('make', False)
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
            print(f'HEY: curious symbol "{L}" in {currObj}.c')
            continue;
        if dropUnder: # (Mac)  (----- 1 -----)(- 2 -)  ( 3)
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

# starts with '  *const nrrdKernel'
def kernelLineProc(L):
    L0 = L
    if (match := re.match(r'.+?(/\*.+?)$', L)):
        # if there is a start of a multi-line comment, remove it
        L = L.replace(match.group(1), '').rstrip()
    if ',' == L[-1]:
        # uncommented part of kernel list
        L = L[:-1] + ';'
    if ';' != L[-1]:
        raise Exception(f'kernel def "{L0}" of unexpected form')
    return L

def declList(lib):
    pubH = f'{lib}.h'
    prvH = f'private{lib.title()}.h'
    if (os.path.isfile(prvH)):
        hdrs = [pubH, prvH]
    else:
        hdrs = [pubH]
    if (verbose):
        print(f"========== scanning {lib} headers {hdrs} ... ")
    decl = {}
    for HN in hdrs:
        pub = (0 == hdrs.index(HN))
        externStr = f'{lib.upper()}_EXPORT ' if pub else 'extern '
        with open(HN) as HF:
            lines = [line.rstrip() for line in HF.readlines()]
        # remove 'extern "C" {' in from list for private
        if not pub: lines.remove('extern "C" {')
        for L in lines:
            origL = L
            # special handling of inside of list of nrrdKernels
            if 'nrrd.h' == HN and L.startswith('  *const nrrdKernel'):
                L = 'NRRD_EXPORT const NrrdKernel' + kernelLineProc(L[1:])
            # does it looks like the start of a declaration?
            if L.startswith(externStr):
                # remove LIB_EXPORT or extern prefix
                L = L.removeprefix(externStr)
                if L == 'const NrrdKernel':
                    # its the start of a kernel list; each handled separately above
                    continue
                # else work on isolating the symbol name
                # remove types
                #print(f'foo0 |{L}|')
                for QT in allTypes:
                    #preL = L
                    L = L.replace(QT+' ', '')
                    #if (L != preL):
                    #    print(f'   {QT} : |{preL}| -> |{L}|')
                #print(f'foo1 |{L}|')
                if (match := re.match(r'.+?(/\*.+?\*/)', L)):
                    # if there a single-line self-contained comment, remove it
                    L = L.replace(match.group(1), '').rstrip()
                #print(f'foo2 |{L}|')
                while (match := re.match(r'.+?(\[[^\[\]]+?\])', L)):
                    # remove arrays
                    L = L.replace(match.group(1), '[]').rstrip()
                #print(f'foo3 |{L}|')
                if (match := re.match(r'.+(\([^\(\)]+)$', L)):
                    # if start of multi-line function declaration, simplify it
                    #print(f'start of multi-line func decl |{L}|')
                    L = L.replace(match.group(1), '();')
                #print(f'foo4 |{L}|')
                while (match := re.match(r'.*?\(.+?(\*\(.+?\)\(.+?\))', L)):
                    # remove function args like *(*threadBody)(void *), for airThreadStart
                    L = L.replace(match.group(1), 'XX').rstrip()
                #print(f'foo5 |{L}|')
                while (match := re.match(r'.+?(\([^\(\)]+\))', L)):
                    # if single-line function declaration, simplfy it
                    #print(f'single-line func decl |{L}|')
                    L = L.replace(match.group(1), '()')
                #print(f'foo5b|{L}|')
                if (match := re.match(r'.+?(\([A-Z]+_ARGS\(\)\))', L)):
                    # echo uses macros to fill out arguments in function declarations
                    L = L.replace(match.group(1), '()')
                #print(f'foo6 |{L}|')
                if (match := re.match(r'.*?(\(\*[^ \)]+\))', L)):
                    # remove indication of being a function pointer
                    L = L.replace(match.group(1), match.group(1)[2:-1])
                #print(f'foo7 |{L}|')
                if L.endswith('()'):
                    L += ';'
                #print(f'foo8 |{L}|')
                if (match := re.match(r'.+(\([^\(\)]+)$', L)):
                    # another whack at this, for airArrayPointerCB
                    # if start of multi-line function declaration, simplify it
                    L = L.replace(match.group(1), '();')
                #print(f'foo9 |{L}|')
                L = L.replace('()(),', '();') # ugh, hacky for airArrayStructCB
                L = L.removeprefix('*').removeprefix('*')
                #print(f'fooA |{L}|')
            else:
                # it doesn't look like either a #define or a declaration, move on to next line
                continue
            # else it was a declaration, and we think we have the name isolated
            if L.endswith('[][][];'):
                decl[L[:-7]] = 'D'
            elif L.endswith('[][]();'):
                decl[L[:-7]] = 'D'
            elif L.endswith('[]();'):
                decl[L[:-5]] = 'D'
            elif L.endswith('[][];'):
                decl[L[:-5]] = 'D'
            elif L.endswith('[];'):
                decl[L[:-3]] = 'D'
            elif L.endswith('();'):
                decl[L[:-3]] = 'T'
            elif L.startswith('gageScl3PFilter'):
                decl[L[:-1]] = 'T' # there's a function typedef
            elif (re.match(r'hoover[\w]+Begin;', L) or re.match(r'hoover[\w]+End;', L)) or ('hooverStubSample;' == L):
                decl[L[:-1]] = 'T' # there's a function typedef
            elif L.endswith(';'):
                decl[L[:-1]] = 'D'
            else:
                raise Exception(f'confused about |{L}| from |{origL}|')
    return decl

def usesBiff(str, idx, fname):
    ss = str.lstrip()
    ret = ''
    # TODO: make sure that these really are the only biff calls in use
    # by removing the rest of them and seeing if anything breaks
    if ss.startswith('biffMaybeMovef') or ss.startswith('biffMaybeAdd'):
        ret = 'maybe'
    elif ss.startswith('biffAddf') or ss.startswith('biffMovef'):
        ret = 'yes'
    elif ss.startswith('biff'):
        print(f'confusing biff @ line {idx} of {fname}: |{ss}|')
    return ret

def biffScan(funcName, obj):
    fileName = f'{obj}.c'
    # get lines of fileName
    if fileName in srcLines:
        if verbose > 2:
            print(f'(re-using read of {fileName}')
        lines = srcLines[fileName]
    else:
        if verbose > 2:
            print(f'(reading {fileName} for first time)')
        with open(fileName) as CF:
            lines = [L.rstrip() for L in CF.readlines()]
        srcLines[fileName] = lines
    # dIdx = index of start of definition (but return type on previous line!)
    dIdx = -1
    nlin = len(lines)
    for idx in range(nlin):
        L = lines[idx]
        if L.startswith(funcName+'('):
            if (-1 == dIdx):
                dIdx = idx
            else:
                print(f'WHOA: bailing since two lines seem to define {funcName}:\n' +
                      (' %4d: %s\n' % (dIdx, lines[dIdx])) +
                      (' %4d: %s' % (idx, lines[idx])))
                return None
    if (-1 == dIdx):
        print(f'--> Sorry, could not find {funcName} defined in {fileName}')
        return
    # else we think we found it
    if verbose > 1:
        print(f'found {funcName} on line {dIdx} of {fileName}: |{lines[dIdx]}|')
    idx = dIdx
    brets = [] # will stay empty if don't see biff usage
    while idx < nlin and '}' != lines[idx]:
        if (bu := usesBiff(lines[idx], idx, fileName)):
            bline = lines[idx]
            bIdx = idx
            while not (RL := lines[idx].lstrip()).startswith('return'):
                idx += 1
            #print(f'{bu}: ({bIdx}) {bline} --> ({idx}) {RL}')
            match = re.match(r'return (.+);', RL)
            if not match:
                raise Exception(f'confusing return line {idx} of {fileName}: |{linex[idx]}|')
            uRV = (bu, match.group(1))
            if not uRV in brets: brets.append(uRV)
        idx += 1
    if brets: # apparently using biff
        # the most common case is using biffAddf/biffMovef, with return 1
        # if not that, print it out:
        if (brets != [('yes','1')]):
            print(f'--> ({obj}.c:{dIdx}) {lines[dIdx]} -> {brets}')


def parse_args():
    # https://docs.python.org/3/library/argparse.html
    parser = argparse.ArgumentParser(description='Utility for seeing if the symbols '
                                     'externally available in a library really are '
                                     'declared as such, and that things declared in '
                                     'header files are actually defined.')
    parser.add_argument('-v', metavar='verbosity', type=int, default=1, required=False,
                        help='verbosity level (0 for silent)')
    parser.add_argument('-c', action='store_true',
                        help='Do a "make clean" before make')
    parser.add_argument('-biff', action='store_true',
                        help='also run "biff auto-scan"')
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
        print('========== found (in lib) symbols:', symb)
    decl = declList(args.lib)
    if (verbose > 1):
        print('========== found (in .h) declarations:', decl)
    toBS = [] # do the declaration-vs-definition stuff first, but remember things to biffScan
    for N in symb:
        symbT = symb[N]['type']
        if 'D' == symbT: print(f'--> {args.lib} lib has global variable {N}')
        if args.biff and 'T' == symbT and not (args.lib in ['air', 'biff', 'hest']):
            toBS.append((N, symb[N]['object'])) # will biffScan this next
        if N in decl:
            declT = decl[N]
            if declT == symbT:
                if verbose > 1: print(f'agree on {N}')
            else:
                if not ('S' == symbT and 'D' == declT):
                    print(f"disagree on {N} type (nm {symbT} vs .h {declT})")
        else:
            if ('unrrdu' == args.lib and re.match(r'unrrdu_\w+Cmd', N)) \
                or ('ten' == args.lib and re.match(r'tend_\w+Cmd', N)) \
                or ('bane' == args.lib and re.match(r'baneGkms_\w+Cmd', N)) \
                or ('limn' == args.lib and re.match(r'limnpu_\w+Cmd', N)) \
                :
                # actually it (probably!) is declared in a private header, via inscrutable macro
                continue
            if ('ten' == args.lib and re.match('_tenQGL_', N)):
                # is not declared in privateTen.h, but used by some ten/test demos
                continue
            print(f'HEY: lib{args.lib} {symbT} symbol {N} not declared')
    for N in decl:
        if not N in symb:
            print(f'HEY: some {args.lib} .h declares {N} but not in lib')
    if args.biff:
        if (verbose):
            print(f"========== biff auto-scan ... ")
        for bs in toBS:
            biffScan(bs[0], bs[1])
