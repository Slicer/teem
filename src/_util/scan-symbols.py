#!/usr/bin/env python
# halt if python2; thanks to https://preview.tinyurl.com/44f2beza
_x, *_y = 1, 2   # NOTE: A SyntaxError here means you need python3, not python2
del _x, _y

import os
import sys
import argparse
import subprocess
import re
from enum import Enum

# hacky script by GLK, started to check consistency of symbols in libraries and
# declarations in headers, but also the "biff auto-scan"; do read
# teem/src/biff/README.txt for more about Biff annotations
# Example usage:
#   python3 scan-symbols.py ~/teem-src nrrd
# to run scan on nrrd library, or
#   python3 scan-symbols.py ~/teem-src nrrd -biff 1
# to also do the biff auto-scan
# (btw there's nothing more "auto" in the "biff auto-scan" than any other scanning
# that this or any other script does, but GLK started goofily calling it the
# "biff auto-scan" in commit messages so now the name is stuck)
#
# See teem/src/biff/README.txt for /* Biff: */ annotation format docs

# to be run on all libraries (TEEM_LIB_LIST) prior to release
# air hest biff nrrd ell moss unrrdu alan tijk gage dye bane limn echo hoover seek ten elf pull coil push mite meet

# TODO: refactor so that lib can be given as "ALL";
# hard now because of use of globals (duh)

# for functions, what kind are they (not really a technical term)
class kind(Enum):
    PUBLIC = 1   # declared in lib.h (and available in library)
    PRIVATE = 2   # declared in privateLib.h (also in library)
    STATIC = 3   # not declared in a header since it is static


kindStr = {kind.PUBLIC: 'public', kind.PRIVATE: 'private', kind.STATIC: 'static'}

# for interpreting "nm" output
if sys.platform == 'darwin':  # Macif sys.platform == 'darwin':  # Mac
    dropUnder = True
elif sys.platform == 'linux':
    dropUnder = False
else:
    raise Exception('Sorry, currently only know how work on Mac and Linux')

verbose = 1
LIB = None   # which library is being scanned
archDir = None
libDir = None
srcLines = {}   # maps from filename to (list of) lines of code, either from disk or modified
modified = {}   # maps from filename to how many srcLines have been modified


def getLines(fileName):
    if fileName in srcLines:
        if verbose > 2:
            print(f'(re-using read of {fileName})')
        lines = srcLines[fileName]
    else:
        if verbose > 2:
            print(f'(reading {fileName} for first time)')
        with open(fileName) as CF:
            lines = [L.rstrip() for L in CF.readlines()]
        srcLines[fileName] = lines
        modified[fileName] = 0
    return lines


# All the types (and type qualifiers) we try to parse away
# The list has to be sorted (from long to short) because things like 'int'
# will also match 'pullPoint'
allTypes = sorted(
    [
        'const',
        'unsigned',
        'static',
        'int',
        'void',
        'double',
        'float',
        'char',
        'short',
        'size_t',
        'FILE',
        # manually generated list of Teem-derived types
        'airLLong',
        'airULLong',
        'airArray',
        'airEnum',
        'airHeap',
        'airFloat',
        'airRandMTState',
        'airThread',
        'airThreadMutex',
        'airThreadCond',
        'airThreadBarrier',
        'biffMsg',
        'hestCB',
        'hestParm',
        'hestOpt',
        'gzFile',
        'NrrdEncoding',
        'NrrdKernel',
        'NrrdFormat',
        'Nrrd',
        'NrrdRange',
        'NrrdIoState',
        'NrrdIter',
        'NrrdResampleContext',
        'NrrdDeringContext',
        'NrrdBoundarySpec',
        'NrrdResampleInfo',
        'NrrdKernelSpec',
        'unrrduCmd',
        'alanContext',
        'mossSampler',
        'tijk_type',
        'tijk_refine_rank1_parm',
        'tijk_refine_rankk_parm',
        'tijk_approx_heur_parm',
        'gageItemSpec',
        'gageScl3PFilter_t',
        'gageKind',
        'gageItemPack',
        'gageShape',
        'gagePerVolume',
        'gageOptimSigContext',
        'gageStackBlurParm',
        'gageContext',
        'dyeColor',
        'dyeConverter',
        'baneRange',
        'baneInc',
        'baneClip',
        'baneMeasr',
        'baneHVolParm',
        'limnLight',
        'limnCamera',
        'limnWindow',
        'limnObject',
        'limnPolyData',
        'limnSplineTypeSpec',
        'limnSpline',
        'limnSplineTypeSpec',
        'limnPoints',
        'limnCBFPath',
        'echoRTParm',
        'echoGlobalState',
        'echoThreadState',
        'echoScene',
        'echoObject',
        '_echoRayIntxUV_t',
        '_echoIntxColor_t',
        'hooverContext',
        'hooverRenderBegin_t',
        'hooverThreadBegin_t',
        'hooverRenderEnd_t',
        'hooverRayBegin_t',
        'hooverSample_t',
        'hooverRayEnd_t',
        'hooverThreadEnd_t',
        'seekContext',
        'tenGradientParm',
        'tenInterpParm',
        'tenGlyphParm',
        'tenEstimateContext',
        'tenEvecRGBParm',
        'tenFiberSingle',
        'tenFiberContext',
        'tenFiberMulti',
        'tenModel',
        'tenEMBimodalParm',
        'tenExperSpec',
        'elfMaximaContext',
        'pullEnergy',
        'pullEnergySpec',
        'pullVolume',
        'pullInfoSpec',
        'pullContext',
        'pullTrace',
        'pullTraceMulti',
        'pullTask',
        'pullBin',
        'pullPoint',
        'coilKind',
        'coilMethod',
        'coilContext',
        'pushContext',
        'pushEnergy',
        'pushEnergySpec',
        'pushBin',
        'pushTask',
        'pushPoint',
        'miteUser',
        'miteShadeSpec',
        'miteThread',
        'meetPullVol',
        'meetPullInfo',
    ],
    key=len,
    reverse=True,
)

# what to do for biffLevel 2 and up:
#   (in describing annotations, "~same" means an existing annotation is either exactly
#   the same as annote, or differs only in Biff?->Biff: or in addition of a comment)
# - if there is nothing after return type, add annotation
# - if existing annotation ~same: leave it
# - if existing comment not ~same as annote: complain
#      and if >2: over-write it
def biffAnnotate(biffLevel, fileName, lineNum, annote, funcName):
    # (not doing anything to avoid being called twice on same function)
    ncmt = f'/* {annote} */'   # new annotation comment
    # source file lines must have been read in
    L = srcLines[fileName][lineNum]
    origL = L
    # the goal now is to get L to have the new annotation, modulo '?' -> ':', and
    # possibly comment within the annotation
    wut = f'{fileName}:{lineNum+1} (for {funcName})'
    # print(f'!{wut} old line "{L}" -vs- new comment "{ncmt}"')
    match = re.match(r'.+?(\/\*.*\*\/)', L)
    if not match:
        # no existing comment (whether or not it has an annotation)
        # make sure that a function returning void is not using biff
        doesBiff = 'Biff? nope' != annote and 'Biff? (private) nope' != annote
        # qualifier and return type; distinguishing between returning
        # void (should not use biff) vs returning void* (fine to use biff)
        qts = L.strip().replace('void *', 'void*').split(' ')
        if (
            'void' in qts
            and doesBiff
            and '_nrrdErrorHandlerPNG'
            != funcName  # except: uses biff w/ PNG's setjmp/longjmp error handler
            and '_nrrdWarningHandlerPNG' != funcName
        ):   # except: uses biff but without setjmp/longjmp, is that ok??
            raise Exception(f'{wut} returns "{qts}" but uses biff (annote={annote})?')
        # else no surprises in whether function uses biff
        # so add annotation when it makes sense to do so:
        # function is not returning void, and,
        # function is not static, or, (it is static and) function does use biff
        #   (static functions not using biff is kind of the norm),
        if not 'void' in qts and (not 'static' in qts or doesBiff):
            L += ' ' + ncmt
        # print(f'! no old comment, so now L={L}')
    else:
        ocmt = match.group(1)
        if not ocmt.startswith('/* Biff'):
            if biffLevel < 3:
                print(
                    f'{wut} has comment "{ocmt}" that isn\'t a Biff annotation, '
                    f'refusing to touch it at level {biffLevel}'
                )
                return
            else:
                print(f'\nNOTE: {wut} deleting old comment "{ocmt}"')
                L = L.replace(ocmt, ncmt)
        # else existing comment starts like an annotation
        elif ocmt == ncmt:
            # nice, already exactly the same
            pass
        elif ocmt == ncmt.replace('/* Biff?', '/* Biff:'):
            # ah, they differed only in '?' -> ':'
            pass
        elif match := re.match(r'\/\*(.+?)(#.*)\*\/', ocmt):
            # existing comment has an annotation comment
            onote = match.group(1).strip()
            if onote == annote:
                # ah, except for annotation comment they're same
                pass
            elif onote == annote.replace('Biff?', 'Biff:'):
                # ah, they differed only in '?' -> ':'
                pass
            else:
                # there was an annotation comment, but actual annotation is different
                if biffLevel < 3:
                    print(
                        f'{fileName}:{lineNum+1} (for {funcName}) has comment "{ocmt}" which itself has an '
                        f'annotation comment "{match.group(2)}", but old annotation "{onote}" '
                        f'really differs from new "{annote}"; won\'t change it at level {biffLevel}'
                    )
                    return
                # else we go ahead and over-write old comment, INCLUDING THE annotation comment
                print(
                    f'\nNOTE: {fileName}:{lineNum+1} (for {funcName}) deleting old commented annotation "{ocmt}"'
                )
                L = L.replace(ocmt, ncmt)
        else:
            # had an old comment that started like an annotation, but its
            # not the new annotation in any recognizable form
            if biffLevel < 3:
                print(
                    f'{wut} has comment "{ocmt}" that maybe is a Biff annotation, '
                    f'refusing to touch it at level {biffLevel}'
                )
                return
            else:
                print(f'\nNOTE: {wut} deleting old comment "{ocmt}"')
                L = L.replace(ocmt, ncmt)
    if L == origL:
        # ah, so we had nothing to do
        return
    else:
        # we are modifying the line
        # print(f'! {wut} |{origL}| -> |{L}|')
        srcLines[fileName][lineNum] = L
        modified[fileName] += 1


# wrapper around subprocess.run
def runthis(cmdStr, capOut):
    if verbose:
        print(f' ... running "{cmdStr}"')
    spr = subprocess.run(cmdStr.split(' '), check=True, capture_output=capOut)
    ret = spr if capOut else None
    return ret


########## The DEFINITION (lib.a) side of the declaration-vs-definition correspondence:
# compile the "lib" library, run nm on it
# and build up a list of the actual, defined, symbols in it
def symbList(lib, firstClean):
    if verbose:
        print(f'========== recompiling, scanning definitions in {lib}.a ... ')
    if firstClean:
        runthis('make clean', False)
    runthis('make', False)
    runthis('make install', False)
    nmOut = runthis(f'nm {archDir}/lib/lib{lib}.a', True).stdout.decode('UTF-8').splitlines()
    nmOut.pop(0)   # first line is empty (at least on Mac)
    # symb accumulates a dict mapping from symbol name to little description:
    # 'type': of the symbol, in the nm sense (not the C sense)
    # 'file': the .c filename that contributed to this library "archive element"
    symb = {}
    currFile = None
    for L in nmOut:
        if match := re.match(r'[^\()]+\(([^\)]+).o\):$', L):
            currFile = match.group(1) + '.c'
            if verbose > 1:
                print(f'   ... {currFile}')
            continue
        if not len(L):
            # blank lines delimit archive elements
            currFile = None
            continue
        if re.match(r' + U ', L) or re.match(r'[0-9a-fA-F]+ (?:d|s) ', L):
            # if undefined: totally irrelevant
            # static (non-external) text, data, or other; not on our radar
            continue
        # t: static/local function "text" (not externally available in library)
        # T: non-static function "text"
        # D: global non-function variable "data"
        # S: const global (non)variable
        if not re.match(r'[0-9a-fA-F]+ [tTDS] ', L):
            # flag this but don't raise an Exception; sometimes these are ok
            print(f'HEY: curious symbol "{L}" in {currFile}')
            continue
        if dropUnder:   # (Mac)  (----- 1 -----)(- 2 -)  ( 3)
            match = re.match(r'([0-9a-fA-F]+ )([tTDS]) _(.*)$', L)
            if not match:
                raise Exception(f'malformed (no leading underscore) "{L}" in {currFile}')
        else:   # not trying to drop leading underscore (not on Mac)
            #                  (----- 1 -----)(- 2 -) ( 3)
            match = re.match(r'([0-9a-fA-F]+ )([tTDS]) (.*)$', L)
            if not match:
                raise Exception(f'malformed "{L}" in {currFile}')
        st = match.group(2)   # symbol type
        sn = match.group(3)   # symbol name
        # subsuming role of old _util/release-nm-check.csh
        if st != 't':   # it is something that should be prefixed by library name
            if not (sn.startswith(lib) or sn.startswith(f'_{lib}')):   # but it doesn't
                raise Exception(
                    f'symbol {sn} (from {currFile}) does not start with {lib} or _{lib}'
                )
            # and, how it starts with library name should be parsable
            nn = sn[1:] if '_' == sn[0] else sn
            if not (match := re.match(r'([a-z]+)[_A-Z0-9]', nn)):
                raise Exception(f"can't parse library name from symbol name {sn}")
            mib = match.group(1)
            if not lib == mib:
                if not ('tend' == mib and 'ten' == lib):   # we allow this to sneak by (HEY)
                    raise Exception(
                        f'symbol name {sn} implies library name "{match.group(1)}" != "{lib}"'
                    )
        # all done, record the symbol description
        symb[sn] = {'type': st, 'file': currFile}
    # print(symb)
    return symb


# in reading .h file, line L looks like a nrrdKernel declaration
def kernelLineProc(L):
    L0 = L
    if match := re.match(r'.+?(/\*.+?)$', L):
        # if there is a start of a multi-line comment, remove it
        L = L.replace(match.group(1), '').rstrip()
    if ',' == L[-1]:
        # uncommented part of kernel list
        L = L[:-1] + ';'
    if ';' != L[-1]:
        raise Exception(f'kernel def "{L0}" of unexpected form')
    return L


########## The DECLARATION (lib.h) side of the declaration-vs-definition correspondence:
# parse header file lib.h (and maybe privateLib.h) and build up names of things
# it declares that we expect to find defined the library
# (the result is another list of symbols, like symbList above returns, but those are
# the real deal, and these are just potentially empty declarations)
def declList(lib):
    pubH = f'{lib}.h'
    prvH = f'private{lib.title()}.h'
    if os.path.isfile(prvH):
        hdrs = [pubH, prvH]
    else:
        hdrs = [pubH]
    if verbose:
        print(f'========== scanning declarations in {lib} headers {hdrs} ... ')
    # the decl dict accumulates a mapping from symbol name to little description
    # 'type': the expected type of the symbol in the library,
    #         as an nm-style letter: 'D' or 'T'
    # 'public': True if in lib.h, False if in privateLib.h
    decl = {}
    for HN in hdrs:
        public = 0 == hdrs.index(HN)
        with open(HN) as HF:
            lines = [line.rstrip() for line in HF.readlines()]
        # remove 'extern "C" {' from the lines (really only an issue for privateLib.h)
        lines.remove('extern "C" {')
        # how thing intended for linkable visibility are are announced
        externStr = f'{LIB}_EXPORT ' if public else 'extern '
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
                # else (hackly) work on isolating the symbol name
                # print(f'foo0 |{L}|')
                for QT in allTypes:
                    # remove all qualifiers and types we know about
                    # preL = L
                    L = L.replace(QT + ' ', '')
                    # if (L != preL):
                    #    print(f'   {QT} : |{preL}| -> |{L}|')
                # print(f'foo1 |{L}|')
                if match := re.match(r'.+?(/\*.+?\*/)', L):
                    # if there a single-line self-contained comment, remove it
                    L = L.replace(match.group(1), '').rstrip()
                # print(f'foo2 |{L}|')
                while match := re.match(r'.+?(\[[^\[\]]+?\])', L):
                    # remove arrays
                    L = L.replace(match.group(1), '[]').rstrip()
                # print(f'foo3 |{L}|')
                if match := re.match(r'.+(\([^\(\)]+)$', L):
                    # if start of multi-line function declaration, simplify it
                    # print(f'start of multi-line func decl |{L}|')
                    L = L.replace(match.group(1), '();')
                # print(f'foo4 |{L}|')
                while match := re.match(r'.*?\(.+?(\*\(.+?\)\(.+?\))', L):
                    # remove function args like *(*threadBody)(void *), for airThreadStart
                    L = L.replace(match.group(1), 'XX').rstrip()
                # print(f'foo5 |{L}|')
                while match := re.match(r'.+?(\([^\(\)]+\))', L):
                    # if single-line function declaration, simplfy it
                    # print(f'single-line func decl |{L}|')
                    L = L.replace(match.group(1), '()')
                # print(f'foo5b|{L}|')
                if match := re.match(r'.+?(\([A-Z]+_ARGS\(\)\))', L):
                    # echo uses macros to fill out arguments in function declarations
                    L = L.replace(match.group(1), '()')
                # print(f'foo6 |{L}|')
                if match := re.match(r'.*?(\(\*[^ \)]+\))', L):
                    # remove indication of being a function pointer
                    L = L.replace(match.group(1), match.group(1)[2:-1])
                # print(f'foo7 |{L}|')
                if L.endswith('()'):
                    L += ';'
                # print(f'foo8 |{L}|')
                if match := re.match(r'.+(\([^\(\)]+)$', L):
                    # another whack at this, for airArrayPointerCB
                    # if start of multi-line function declaration, simplify it
                    L = L.replace(match.group(1), '();')
                # print(f'foo9 |{L}|')
                if L.startswith('airArrayStructCB'):   # wow, super-hacky!
                    L = 'airArrayStructCB();'
                L = L.removeprefix('*').removeprefix('*')
                # print(f'fooA |{L}|')
            else:
                # it doesn't look like either a #define or a declaration, move on to next line
                continue
            # else it was a declaration, and we think we have the name isolated
            def desc(K):
                return {
                    'type': K,
                    'kind': kind.PUBLIC if f'{lib}.h' == HN else kind.PRIVATE
                    # can't be STATIC because then it won't be in header (enforced elsewhere)
                }

            if L.endswith('[][][];'):
                decl[L[:-7]] = desc('D')
            elif L.endswith('[][]();'):
                decl[L[:-7]] = desc('D')
            elif L.endswith('[]();'):
                decl[L[:-5]] = desc('D')
            elif L.endswith('[][];'):
                decl[L[:-5]] = desc('D')
            elif L.endswith('[];'):
                decl[L[:-3]] = desc('D')
            elif L.endswith('();'):
                decl[L[:-3]] = desc('T')
            elif L.startswith('gageScl3PFilter'):
                decl[L[:-1]] = desc('T')   # there's a function typedef
            elif (re.match(r'hoover[\w]+Begin;', L) or re.match(r'hoover[\w]+End;', L)) or (
                'hooverStubSample;' == L
            ):
                decl[L[:-1]] = desc('T')   # there's a function typedef
            elif L.endswith(';'):
                decl[L[:-1]] = desc('D')
            else:
                raise Exception(f'confused about |{L}| from |{origL}|')
    # print(decl)
    return decl


# TODO: make this an inner function
def usesBiff(str, idx, fname):
    ss = str.lstrip()
    # Now that the biffMsg functions were moved to privateBiff.h,
    # these really are the only functions to look for. "startswith"
    # will detect both, e.g., biffAdd() and the more common biffAddf()
    # Also note: we really can get by with such simple string processing
    # because clang-format has normalized things so completely
    if ss.startswith('biffMaybeAdd'):
        wen = 'maybe'
        # HEY this really assumes biffMaybeAddf, not biffMaybeAdd
        # in fact currently biffMaybeAdd is never actually used in Teem,
        # which suggests that it should be removed...
        rgx = r'biff\w+\(useBiff, (\w+),'
    elif ss.startswith('biffAdd') or ss.startswith('biffMove'):
        wen = 'yes'
        rgx = r'biff\w+\((\w+),'
    elif ss.startswith('biff'):
        raise Exception(f'confusing biff @ line {idx+1} of {fname}: |{ss}|')
    else:
        wen = False
    if wen:
        if not (match := re.match(rgx, ss)):
            raise Exception(f'unparsable biff call @ line {idx+1} of {fname}: |{ss}|')
        key = match.group(1)
        if LIB != key:
            print(f'HEY {fname}:{idx+1} uses biff key "{key}" != "{LIB}"')
        # we've made an effort to ensure that the biff key is the library name
        # and, symbList made sure that the library name is parsable from function name
    return wen


########## Home of the "biff auto-scan"
# read in .c fileName, looking for lines of C code defining (one function) funcName,
# and figure out how it is using biff.
# Return a tupe (line#, annote): line# is the 0-based index of where annotation annote should go,
# where annote does NOT have wrapping /*  */
def biffScan(funcName, fileName, funcKind):
    # print('!', funcName, fileName, funcKind)
    # get lines of fileName
    lines = getLines(fileName)
    # dIdx = index of start of definition (but return type on previous line!)
    dIdx = -1
    nlin = len(lines)
    useBiffIdx = 0   # 1-based number of "useBiff" parameter
    for idx in range(nlin):
        L = lines[idx]
        # this is assuming the results of clang-format, that in the function definition,
        # the function name is at the start of the line
        if L.startswith(funcName + '('):
            if -1 == dIdx:
                dIdx = idx
            else:
                print(
                    f'WHOA: bailing; two lines in {fileName} seem to define {funcName}:\n'
                    + (' %4d: %s\n' % (dIdx, lines[dIdx]))
                    + (' %4d: %s' % (idx, lines[idx]))
                )
                return None
    if -1 == dIdx:
        if kind.STATIC != funcKind:
            # it is very common for static functions to be defined by macros;
            # so common that it would be annoying to handle possibilities here.
            # So, we only complain not being able to find definitions of declared
            # functions (at the risk of losing chance to find biff usage errors
            # in static functions, such as functions not defined in macros, but
            # inside a clang-off/clang-on bracket)
            print(f'--> Sorry, could not (simplistically) find {funcName} defined in {fileName}')
        return None
    # else we think we found it
    if verbose > 2:
        print(f'found {funcName} on line {dIdx+1} of {fileName}: |{lines[dIdx]}|')
    idx = dIdx
    # sometimes the start of the function declaration is multiple lines; we know
    # we're at the end of the intro when we see a '{' ending the line
    intro = lines[dIdx]
    while not intro.endswith('{'):
        idx += 1
        intro += ' ' + lines[idx].lstrip()
    if verbose > 2 and intro != lines[dIdx]:
        print(f'  full intro: |{intro}|')
    brets = []   # will stay empty if don't see biff usage
    # now scan the body of the function, ending with line '}'
    while idx < nlin and '}' != lines[idx]:
        if bu := usesBiff(lines[idx], idx, fileName):
            bline = lines[idx]
            bIdx = idx
            # saw a line using biff, now look for next "return"
            found = False
            while not found:
                idx += 1
                RL = lines[idx].lstrip()   # maybe a line with a return
                # accept returns in comments because sometimes functions (like tenFiberStopSet)
                # need to use a goto end for clean finishing (like finishing var-args)
                found = RL.startswith('return ') or RL.startswith('/* return ')
            if verbose > 2:
                print(f'{bu}: ({bIdx}) {bline} --> ({idx}) {RL}')
            match = re.match(r'.*?return (.+);', RL)
            if not match:
                raise Exception(f'confusing return line {idx} of {fileName}: |{lines[idx]}|')
            uRV = (bu, match.group(1))
            if not uRV in brets:
                # haven't yet recorded this "return" value RV after this biff usage bu
                brets.append(uRV)
        idx += 1
        if idx == nlin:
            raise Exception(f'hit end of file {fileName} looking for }} ending {funcName} defn')
    # print(f'!({fileName} : {dIdx} {kindStr[funcKind]}) {lines[dIdx-1]} {intro}\n  -> {brets}')
    if brets:
        # if there was any biff usage (and its fine if there is not)
        if len(brets) > 1:
            # make sure uses are either all 'yes' or all 'maybe'
            uu = list(set([uRV[0] for uRV in brets]))
            if (len(uu)) > 1:
                raise Exception(
                    f'function {funcName} in {fileName}:{dIdx} uses a combination of biffAdd/Move and biffMaybeAdd/Move'
                )
        # the most common case is using biffAddf/biffMovef (bu = 'yes'), with return 1
        # if not that, print it out (or always print it with high enough verbosity):
        if len(brets) > 1:
            print('****\n**** really? multiple different returns in:')
            print(
                f'--> ({fileName} : {dIdx} {kindStr[funcKind]}) {lines[dIdx-1]} {intro}\n  -> {brets}'
            )
        if 'maybe' == brets[0][0]:
            # figure out which of the function parameters (1-based numbering) is called "useBiff"
            if not (match := re.match(r'.+?\((.+?)\)', intro)):
                raise Exception(
                    f"can't parse parameters from declaration start {intro} in {fileName}:{dIdx}"
                )
            # parse parameters into list of (lists of words)
            parms = [P.strip().split(' ') for P in match.group(1).split(',')]
            # look for useBiff
            useb = [('useBiff' in PL) for PL in parms]
            try:
                useBiffIdx = useb.index(True)
            except ValueError:
                raise Exception(
                    f'{funcName} uses biffMaybe but don\'t see "useBiff" in '
                    f'start of function declaration "{intro}"'
                )
            # make sure there's only one useBiff
            useb.pop(useBiffIdx)
            if [False] != list(set(useb)):
                raise Exception(
                    f'{funcName} seems to have multiple "useBiff" parms '
                    f'in its declaration "{intro}"'
                )
            # make useBiffIdx 1-based
            useBiffIdx += 1
        if len(brets) > 2:
            # of course there's no reason why a function can't have multiple error return values
            # (in fact that is a pretty standard way of communicating how things went south)
            # but currently the documentation of Biff: annotation in teem/src/biff/README.txt
            # only covers two return values
            raise Exception(f'Have {len(brets)} > 2 different error return values')
        if 2 == len(brets) and 'maybe' == brets[0][0]:
            # another limitation of the Biff: annotation format, than plausible function behavior
            raise Exception(
                f'Cannot currently handle "maybe" with 2 different error return values'
            )
    # create and return the annotation that captures what we know:
    annote = 'Biff? '
    if kind.PRIVATE == funcKind:
        annote += f'({kindStr[funcKind]}) '
    if not brets:
        annote += 'nope'
    # else there is biff usage
    elif 'yes' == brets[0][0]:
        if 1 == len(brets):
            annote += brets[0][1]
        else:
            # here is where multiple return values would be handled
            annote += f'{brets[0][1]}|{brets[1][1]}'
    elif 'maybe' == brets[0][0]:
        annote += f'maybe:{useBiffIdx}:{brets[0][1]}'
    else:
        raise Exception(f'Sorry very confused about brets={brets}')
    if not (
        'Biff? 1' == annote
        or 'Biff? (private) 1' == annote
        or 'Biff? nope' == annote
        or 'Biff? (private) nope' == annote
    ):
        # interesting enough to print out
        print(f'{fileName}:{dIdx-1} /* {annote} */ <-- {intro}')
    # print(f'! returning annote={annote}')
    return (dIdx - 1, annote)   # RETURN


# check the two command-line arguments to this script
def argsCheck(tPath, lib):
    global archDir, libDir
    if not (
        os.path.isdir(tPath) and os.path.isdir(f'{tPath}/arch') and os.path.isdir(f'{tPath}/src')
    ):
        raise Exception(f'Need {tPath} to be dir with "arch" and "src" subdirs')
    if not os.path.isdir(f'{tPath}/src/{lib}'):
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


def parse_args():
    # https://docs.python.org/3/library/argparse.html
    parser = argparse.ArgumentParser(
        description='Utility for (1) consistency between symbols '
        'being defined and available for linking in a library, '
        'and being declared in header files, and (2) with "-biff" '
        'the "biff auto-scan", which scrutinizes functions\' biff '
        'usage, and annotates functions to document this',
        formatter_class=argparse.RawTextHelpFormatter,
    )
    parser.add_argument(
        '-v',
        metavar='verbosity',
        type=int,
        default=1,
        required=False,
        help='verbosity level (0 for silent)',
    )
    parser.add_argument('-c', action='store_true', help='Do a "make clean" before make')
    parser.add_argument(
        '-biff',
        metavar='level',
        type=int,
        default=0,
        help='also run "biff auto-scan", at some level:\n'
        '1: do scan to flag issues in code, but no new annotations\n'
        '2: add annotations where there are not, but dont overwrite '
        'any existing wrong annotations or comments\n'
        '3: create all anntations (including over-writing comments '
        'and wrong annotations)\n'
        'In fact no original source files are over-written: the result '
        'of annotating functions in foo.c is a new file foo-annote.c',
    )
    parser.add_argument('teem_path', help='path of Teem checkout with "src" and "arch" subdirs')
    parser.add_argument('lib', help='which single library to scan')
    return parser.parse_args()


if __name__ == '__main__':
    args = parse_args()
    verbose = args.v
    argsCheck(args.teem_path, args.lib)
    LIB = args.lib.upper()
    if verbose:
        print(f'========== cd {libDir} ... ')
    os.chdir(libDir)
    symb = symbList(args.lib, args.c)
    if verbose > 1:
        print('========== found (in lib) defined symbols:', symb)
    decl = declList(args.lib)
    if verbose > 1:
        print('========== found (in .h) declarations:', decl)
    toBS = []   # do the declaration-vs-definition stuff first, but remember things to biffScan
    ######### FIRST: go through all the symbols
    # - queue things for biff analysis
    # - are defined symbols actually declared?
    for N in symb:
        symbT = symb[N]['type']
        if 'D' == symbT:
            print(f'NOTE: {args.lib} lib has global variable {N}')
        if 't' == symbT and N in decl:
            raise Exception(f'wut? static function {N} is declared in a {args.lib} header?')
        if args.biff and symbT in 'tT' and not (args.lib in ['air', 'biff', 'hest']):
            # if we're doing the biff auto-scan, and this is a function, and it isn't
            # in a library that can't use biff, then add this to list of things to biffScan
            # (each item in this list is a dumb little tuple)
            tbt = (
                N,  # 0 : name
                symb[N]['file'],  # 1 : which file it's defined in
                decl[N]['kind'] if N in decl else kind.STATIC,  # 2 : kind of function
            )
            # print('!', tbt)
            toBS.append(tbt)
        if N in decl:
            declT = decl[N]['type']
            if declT == symbT or ('S' == symbT and 'D' == declT):
                # global const variables are type 'S' in nm
                if verbose > 2:
                    print(f'agree on {N}')
            else:
                print(f'HEY: decl/defn disagree on type of {N} (nm {symbT} vs .h {declT})')
        else:
            # symbol name N was (apparently) not declared in a header
            if 't' == symbT:
                # um, it's static, of course it's not declared
                continue
            if (
                ('unrrdu' == args.lib and re.match(r'unrrdu_\w+Cmd', N))
                or ('ten' == args.lib and re.match(r'tend_\w+Cmd', N))
                or ('bane' == args.lib and re.match(r'baneGkms_\w+Cmd', N))
                or ('limn' == args.lib and re.match(r'limnPu_\w+Cmd', N))
            ):
                # actually it (probably!) is declared, in a private header, via inscrutable macro
                continue
            if 'ten' == args.lib and re.match('_tenQGL_', N):
                # is not declared in privateTen.h, but used by some ten/test demos; ok
                continue
            print(f'HEY: lib{args.lib} {symbT} symbol {N} not declared')
    for N in decl:
        if not N in symb:
            print(f'HEY: some {args.lib} .h declares {N} but not defined in lib')
    if args.biff:
        if verbose:
            print(f'========== biff auto-scan ... ')
        for bs in toBS:
            bnote = biffScan(*bs)
            if not bnote or 1 == args.biff:
                # just passing through
                continue
            # else we try to do something with the annotation
            (lineNum, annote) = bnote
            funcName = bs[0]
            fileName = bs[1]
            # print(f'!{fileName}:{lineNum+1} --- {annote}')
            biffAnnotate(args.biff, fileName, lineNum, annote, funcName)
        allNF = []
        for MF in [F for F, M in modified.items() if M]:   # HEY better way?
            NF = MF.replace('.c', '-annote.c')
            allNF.append(NF)
            with open(NF, 'w') as fout:
                for L in srcLines[MF]:
                    fout.write(f'{L}\n')
            print(f'wrote {modified[MF]} annotations in {NF}')
        # print(f'writing new files in {libDir}: {allNF}')
