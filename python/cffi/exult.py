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
exult.py: CFFI *EX*tension module *U*tilities for *L*ibraries depending on *T*eem. Contains two
kinds of things related to CFFI extension modules for libraries that depend on Teem (including
Teem itself): creating extension modules, and using them.

(1) Compiling extension modules involves checks on paths, and knowing which Teem libs
are dependencies of a given single Teem lib, in order to know which cdef headers to read in.
This functionality used to be in build_teem.py (which generates the _teem CFFI extension module),
but it is useful for the building of other Teem-using extension modules.

(2) Using extension modules that depend on Teem benefits from some wrapping:
* for functions that use biff, wrap it in something that detects the biff error,
  and turn the biff error message into a Python exception
* for airEnums, make a Python object with useful int<->string conversion methods
This functionality used to be in teem.py (the wrapper around the _teem CFFI extension module),
but then other Teem-based extension modules would have to copy-paste it. Now, anything wrapping
a CFFI extension module with .lib elements that are biff-using functions or airEnums (including
teem.py itself), can use exult.py to generate wrappers for those objects.

The use cases for (1) and (2) above are in fact distinct: (1) is done once on a machine, (2) is
repeatedly done with every program execution.  Still, for now it is nice to keep information
about Teem localized to this single file.
"""

# import math as _math  # for isnan test that may appear in _BIFFDICT
import sys as _sys
import os as _os
import re as _re
import subprocess as _subprocess

import cffi as _cffi

# TODO: revisit if it makes sense to have these underscores

# halt if python2; thanks to https://preview.tinyurl.com/44f2beza
_x, *_y = 1, 2  # NOTE: A SyntaxError here means you need python3, not python2
del _x, _y

# info about all the Teem libraries (TEEM_LIB_LIST)
_tlibs = {
    'air': {'expr': False, 'deps': []},  # (don't need airExistsConf.h)
    'hest': {'expr': False, 'deps': ['air']},
    'biff': {'expr': False, 'deps': ['air']},
    'nrrd': {
        'expr': False,
        'deps': ['biff', 'hest', 'air'],
        'hdrs': ['nrrdEnums.h', 'nrrdDefines.h'],
    },
    'ell': {
        'expr': False,
        'deps': ['nrrd', 'biff', 'air'],
        # ellMacros.h does not add to ell API
    },
    'moss': {
        'expr': False,
        'deps': ['ell', 'nrrd', 'biff', 'hest', 'air'],
    },
    'unrrdu': {
        'expr': False,
        'deps': ['nrrd', 'hest', 'biff', 'air'],
        # moss needed for linking (because of unu ilk) but not for declaring unrrdu API
    },
    'alan': {
        'expr': True,
        'deps': ['ell', 'nrrd', 'biff', 'air'],
    },
    'tijk': {
        'expr': True,
        'deps': ['ell', 'nrrd', 'air'],
    },
    'gage': {
        'expr': False,
        'deps': ['ell', 'nrrd', 'biff', 'air'],
    },
    'dye': {
        'expr': False,
        'deps': ['ell', 'biff', 'air'],
        # may not actually need ell; implementation of dye needs ellMacros.h
    },
    'bane': {
        'expr': True,
        'deps': ['gage', 'unrrdu', 'nrrd', 'biff', 'air'],
    },
    'limn': {
        'expr': False,
        'deps': ['gage', 'ell', 'unrrdu', 'nrrd', 'biff', 'hest', 'air'],
    },
    'echo': {
        'expr': False,
        'deps': ['limn', 'ell', 'nrrd', 'biff', 'air'],
    },
    'hoover': {
        'expr': False,
        'deps': ['limn', 'ell', 'nrrd', 'biff', 'air'],
    },
    'seek': {
        'expr': False,
        'deps': ['gage', 'limn', 'ell', 'nrrd', 'biff', 'hest', 'air'],
    },
    'ten': {
        'expr': False,
        'deps': ['echo', 'limn', 'gage', 'dye', 'unrrdu', 'ell', 'nrrd', 'biff', 'air'],
    },
    'elf': {
        'expr': True,
        'deps': ['ten', 'tijk', 'limn', 'ell', 'nrrd', 'air'],
    },
    'pull': {
        'expr': False,
        'deps': ['ten', 'limn', 'gage', 'ell', 'nrrd', 'biff', 'hest', 'air'],
    },
    'coil': {
        'expr': True,
        'deps': ['ten', 'ell', 'nrrd', 'biff', 'air'],
    },
    'push': {
        'expr': True,
        'deps': ['ten', 'gage', 'ell', 'nrrd', 'biff', 'air'],
    },
    'mite': {
        'expr': False,
        'deps': ['ten', 'hoover', 'limn', 'gage', 'ell', 'nrrd', 'biff', 'air'],
    },
    'meet': {
        'expr': False,
        'deps': [
            'air',
            'hest',
            'biff',
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
        ],
    },
}

### (1) (1) (1) (1) (1) (1) (1) (1) (1) (1) (1) (1) (1) (1) (1) (1) (1) (1) (1) (1)
### (1) (1) (1) (1) (1) (1) (1) (1) (1) (1) (1) (1) (1) (1) (1) (1) (1) (1) (1) (1)
###
### Things useful for *compiling* Teem-based extension modules


def tlib_all() -> list[str]:
    """
    Returns list of all Teem libraries in dependency order
    """
    return list(_tlibs.keys())


def tlib_experimental(lib: str) -> bool:
    """
    Answers if a given Teem library is "experimental"
    """
    try:
        info = _tlibs[lib]
    except Exception as exc:
        raise RuntimeError(f'{lib} is not a known Teem library') from exc
    return info['expr']


def tlib_depends(lib: str) -> list[str]:
    """
    Computes dependency expansion of given Teem library
    """
    try:
        info = _tlibs[lib]
    except Exception as exc:
        raise RuntimeError(f'{lib} is not a known Teem library') from exc
    # iteratively find all dependencies and dependencies of dependencies, etc
    oldd = set()   # all previously dependencies known
    newd = set([lib]) | set(info['deps'])   # newly discovered dependencies
    while oldd != newd:
        # while new dependencies were just discovered
        tmpd = set()
        for nlb in newd:
            tmpd = tmpd | set([lib]) | set(_tlibs[nlb]['deps'])
        oldd = newd
        newd = tmpd
    tla = tlib_all()   # linear array of all libs in dependency order
    # return dependencies sorted in dependency order
    return sorted(list(newd), key=tla.index)


def tlib_headers(lib: str) -> list[str]:
    """
    Returns list of headers (installed by CMake's "make install" the declares the API of given
    Teem library. This is really only the business of things (like build_teem.py) that do the
    one-time generation of cdef headers to be later consumed by CFFI, rather than things that
    use Teem via its python wrappers, but it is nice to have the info about Teem libraries
    centralized to this file. For example this handles how the nrrd library needs nrrdDefines.h
    and nrrdEnums.h as well as nrrd.h.
    """
    try:
        info = _tlibs[lib]
    except Exception as exc:
        raise RuntimeError(f'{lib} is not a known Teem library') from exc
    ret = info['hdrs'].copy() if 'hdrs' in info else []
    ret += [f'{lib}.h']
    return ret


def check_path_tlib(path_tlib: str) -> None:
    """
    Sanity checks on Teem install "lib" path lib_path.
    May throw various exceptions but returns nothing.
    """
    if _sys.platform == 'darwin':  # Mac
        shext = 'dylib'
    elif _sys.platform == 'linux':
        shext = 'so'
    else:
        raise Exception(
            'Sorry, currently only know how work on Mac and Linux (not {_sys.platform})'
        )
    lib_fnames = _os.listdir(path_tlib)
    if not lib_fnames:
        raise Exception(f'Teem library dir {path_tlib} seems empty')
    ltname = f'libteem.{shext}'
    if not ltname in lib_fnames:
        raise Exception(
            f'Teem library dir {path_tlib} contents {lib_fnames} do not seem to include '
            f'required {ltname} shared library, which means running '
            'cffi.FFI().compile() later will not produce a working wrapper, even if '
            'it finishes without error.'
        )


def check_path_thdr(path_thdr: str):
    """
    Main purpose is to do sanity check on Teem include path path_thdr.
    Having done that work, we can also return information learned along the way:
    (exper, have_libs) where exper indicates if this was run on an "experimental"
    Teem build, and have_libs is the list of libraries for which the .h headers are present
    """
    itpath = path_thdr + '/teem'
    if not _os.path.isdir(itpath):
        raise Exception(f'Need {itpath} to be directory')
    all_libs = tlib_all()
    base_libs = list(filter(lambda L: not tlib_experimental(L), all_libs))
    expr_libs = list(filter(tlib_experimental, all_libs))
    base_hdrs = sum([tlib_headers(L) for L in base_libs], [])
    expr_hdrs = sum([tlib_headers(L) for L in expr_libs], [])
    missing_hdrs = list(filter(lambda F: not _os.path.isfile(f'{itpath}/{F}'), base_hdrs))
    if missing_hdrs:
        raise Exception(
            f'Missing header(s) {" ".join(missing_hdrs)} in {itpath} '
            'for one or more of the core Teem libs'
        )
    have_libs = base_libs
    missing_expr_hdrs = list(filter(lambda F: not _os.path.isfile(f'{itpath}/{F}'), expr_hdrs))
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
    else:
        # it is Experimental; reform the header list in dependency order (above)
        have_libs = all_libs
    return (not missing_expr_hdrs, have_libs)


def check_path_tinst(path: str):
    """
    Checks that the given path really is a path to a CMake-based Teem installation
    """
    path = path.rstrip('/')
    if not _os.path.isdir(path):
        path = _os.path.expanduser(path)
        if not _os.path.isdir(path):
            raise Exception(f'Given path {path} is not a directory')
    path_thdr = path + '/include'
    path_tlib = path + '/lib'
    if not _os.path.isdir(path_thdr) or not _os.path.isdir(path_tlib):
        raise Exception(
            f'Need both {path_thdr} and {path_tlib} to be subdirs of teem install dir {path}'
        )
    check_path_tlib(path_tlib)
    (exper, have_libs) = check_path_thdr(path_thdr)
    return (path_thdr, path_tlib, have_libs, exper)


def scan_cdef(hfilename: str) -> list[str]:
    """
    Reads header file, looking for a region demarcated by "exult.cdef begin" and "exult.cdef
    end", and processes the lines in that region to anticipate the limitations of the
    CFFI.cdef() parser, and returns list of strings that should be pass-able to it.
    """
    if not _os.path.isfile(hfilename):
        raise Exception(f'header filename {hfilename} not a file')
    result = _subprocess.run(
        ['unu', 'uncmt', '-xc', hfilename, '-'], check=True, stdout=_subprocess.PIPE
    )
    ilines = result.stdout.decode('utf-8').splitlines()
    using = True
    olines = []
    for linen in ilines:
        line = linen.strip()   # strip left and right whitespace
        if not line:
            continue
        if line.startswith('#'):
            # if even CFFI.cdef() can handle certain simple #defines, we don't try
            if line.startswith('#ifdef') or line.startswith('#ifndef'):
                print(f'|{line}| is #ifdef')
                using = False
            elif line.startswith('#endif'):
                using = True
            continue
        if not using:
            continue
        if match := _re.match(r'(__attribute__\(.*\))', line):
            line = line.replace(match.group(1), '')
        olines.append(line)
    return olines


class Tffi:
    """
    Helps create and use an instance of CFFI's "FFI" object, when creating an extension module
    for Teem itself, or a library that uses Teem. In particular this manages the many arguments
    to the .set_source() method, as well as calling .compile().  The basic steps are:
    (1) tffi = Tffi(...) # instantiate the class
    tffi.desc(...) # optional: describe the non-Teem library of the extension module
    (2) tffi.cdef() # do the cdef declarations
    (3) tffi.set_source() # set up and make call to ffi.set_source()
    (4) tffi.compile(): # run the compilation
    self.step remembers the step number just done
    """

    def __init__(self, path_tsrc: str, path_tinst: str, top_tlib: str, verb: int = 0):
        """
        Creates a Tffi from the given arguments:
        :param str path_tsrc: path into local checkout of Teem source (needed to get to the
            python/cffi subdirectory with files that are not currently copied uppon Teem install)
        :param str path_tinst: path into wherever CMake's "make install" put things
            (like the "include" and "lib" subdirectory)
        :param str top_tlib: If creating the Teem extension module, pass 'teem', else name the
            top-most Teem library (depending on the most other Teem libraries) on which your new
            library's extension module depends.
        :param int verb: verbosity level
        :return: a new instance of the Tffi class, which contains a new CFFI FFI instance.
        """
        self.verb = verb
        if not _os.path.isdir(path_tsrc):
            path_tsrc = _os.path.expanduser(path_tsrc)
            if not _os.path.isdir(path_tsrc):
                raise Exception(
                    f'Need path {path_tsrc} into Teem source checkout to be a directory'
                )
        self.path_tsrc = path_tsrc
        self.path_cdef = path_tsrc + '/python/cffi/cdef'
        if not _os.path.isdir(self.path_cdef):
            raise Exception(
                f'Missing directory with per-Teem-library cdef headers {self.path_cdef}'
            )
        # This does a lot error checking
        (self.path_thdr, self.path_tlib, self.have_tlibs, self.exper) = check_path_tinst(
            path_tinst
        )
        self.path_tinst = path_tinst
        # initialize other members
        self.ipath = ''
        self.lpath = ''
        self.cdf = ''
        self.eca = []
        self.ela = []
        self.source_args = None
        self.path_out = None
        if 'teem' == top_tlib:
            # we are creating the Teem extension module
            self.isteem = True
            self.name = 'teem'
            self.top_tlib = 'meet'
            # we keep the experimental-ness value now in self.exper
        else:
            if not top_tlib in self.have_tlibs:
                raise Exception(
                    f'Requested top lib {top_tlib} not in this Teem build: {self.have_tlibs}'
                )
            self.isteem = None
            self.name = None
            self.top_tlib = top_tlib
            # we set exper according to whether requested library is "experimental"
            self.exper = tlib_experimental(top_tlib)
        # create the instance, but don't do anything with it; that depends on other methods
        self.ffi = _cffi.FFI()
        self.step = 1   # for tracking correct ordering of method calls

    def desc(
        self, name: str, ipath: str, lpath: str, cdf: str = '', eca: list = [], ela: list = []
    ):
        """
        To create an extension module for a non-Teem library that depends on Teem,
        describe it here.
        :param str name: name of the new non-Teem library
        :param str ipath: path to headers for you library, to give to -I when compiling
        :param str lpath: path to your compile library, to give to -L when compiling
        :param str cdf: string to pass to ffi.cdef()
        :param list eca: for the extra_compile_args parameter to ffi.compile()
        :param list ela: for the extra_link_args parameter to ffi.compile()
        """
        if 1 != self.step:
            raise Exception('Describing library only possible right after Tffi creation')
        if self.isteem:
            raise Exception(
                "Can't use .desc when making Teem module "
                "(as implied by top_tlib='teem' arg to init)"
            )
        if 'teem' == name or name in self.have_tlibs:
            raise Exception('Need non-Teem name for non-Teem library')
        if name.startswith('_'):
            raise Exception(
                'Name "{_name}" should not start with "_"; that will be added as needed later'
            )
        if not _os.path.isdir(ipath):
            raise Exception(f'Need ipath {ipath} to be a directory')
        if not _os.path.isdir(lpath):
            raise Exception(f'Need lpath {ipath} to be a directory')
        self.name = name
        self.ipath = ipath
        self.lpath = lpath
        self.cdf = cdf
        self.eca = eca
        self.ela = ela
        # leave self.step at 1

    def cdef(self):
        """
        Make calls to ffi.cdef() to declare to CFFI what should be in the extension module
        (the members of the module's .lib)
        """
        if 1 != self.step:
            raise Exception('Expected .cdef() only right after Tffi creation and optional .desc()')
        # want free() available in for freeing biff messages
        self.ffi.cdef('extern void free(void *);')
        # read in the relevant Teem cdef/ headers
        for lib in tlib_depends(self.top_tlib):
            if self.verb:
                print(f'Tffi.cdef: reading {self.path_cdef}/cdef_{lib}.h ...')
            with open(f'{self.path_cdef}/cdef_{lib}.h', 'r', encoding='utf-8') as file:
                self.ffi.cdef(file.read())
        if self.cdf:
            if self.verb:
                print(f'Tffi.cdef: calling cdef(self.cdf = "{self.cdf}")')
            self.ffi.cdef(self.cdf)
        if not self.isteem:
            lines = scan_cdef(f'{self.ipath}/{self.name}.h')
            if self.verb:
                print('Tffi.cdef: calling cdef on:')
                for line in lines:
                    print(f'...{line}')
            self.ffi.cdef('\n'.join(lines))
        self.step = 2

    def set_source(self):
        """
        Sets up arguments to ffi.set_source() and calls it
        """
        if 2 != self.step:
            raise Exception('Expected .set_source() only right after .cdef()')
        self.source_args = {
            'libraries': ([] if self.isteem else [self.name]) + ['teem'],
            'include_dirs': ([] if self.isteem else [self.ipath]) + [self.path_thdr],
            'library_dirs': ([] if self.isteem else [self.lpath]) + [self.path_tlib],
            'extra_compile_args': (
                (['-DTEEM_BUILD_EXPERIMENTAL_LIBS'] if self.exper else []) + self.eca
            ),
            'extra_link_args': self.ela,
            # On linux, path <dir> here is passed to -Wl,--enable-new-dtags,-R<dir>;
            # "readelf -d ....so | grep PATH" should show <dir> and "ldd .....so" should show
            # where dependencies were found.
            # On Mac, path <dir> here is passed to -Wl,-rpath,<dir>, and you can see that
            # from "otool -l ....so", in the LC_RPATH sections.
            'runtime_library_dirs': [
                _os.path.abspath(dir)
                for dir in ([] if self.isteem else [self.lpath]) + [self.path_tlib]
            ],
            # keep asserts()
            # https://docs.python.org/3/distutils/apiref.html#distutils.core.Extension
            'undef_macros': ['NDEBUG'],
        }
        arg1 = f'_{self.name}'   # HERE is where we add the leading underscore
        arg2 = f'#include <teem/{self.top_tlib}.h>' + (
            '' if self.isteem else f'\n#include <{self.name}.h>'
        )
        if self.verb:
            print('Tffi.set_source: calling ffi.set_source with ...')
            print(f"'{arg1}',")
            print(f"'{arg2}',")
            for key, val in self.source_args.items():
                print(f' {key} = {val}')
        self.ffi.set_source(
            arg1,
            arg2,
            **self.source_args,
        )
        self.step = 3
        return self.source_args

    def compile(self):
        """Finally call ffi.compile()"""
        if self.verb:
            print('Tffi.compile: compiling ... ')
            if 'meet' == self.top_tlib:
                print('     (compiling bindings to all of Teem is slow)')
        self.path_out = self.ffi.compile(verbose=(self.verb > 0))
        if self.verb:
            print(f'Tffi.compile: compiling _{self.name} done; created:\n{self.path_out}')
        return self.path_out


### (2) (2) (2) (2) (2) (2) (2) (2) (2) (2) (2) (2) (2) (2) (2) (2) (2) (2) (2) (2)
### (2) (2) (2) (2) (2) (2) (2) (2) (2) (2) (2) (2) (2) (2) (2) (2) (2) (2) (2) (2)
###
### Things useful for *wrapping* Teem-using extension module


class Tenum:
    """Helper/wrapper around (pointers to) airEnums (part of Teem's "air" library).
    This provides convenient ways to convert between integer enum values and real
    Python strings. The C airEnum underlying the Python Tenum foo is still available
    as foo().
    """

    def __init__(self, aenm, _name, xmdl):
        """Constructor takes a Teem airEnum pointer (const airEnum *const)."""
        # xmdl: what extension module is this enum coming from; if it is not _teem itself, then
        # we want to know the module so as to avoid errors (when calling .ffi.string) like:
        # "TypeError: initializer for ctype 'airEnum *' appears indeed to be 'airEnum *', but
        # the types are different (check that you are not e.g. mixing up different ffi instances)"
        # TODO: check that xmdl has:
        # xmdl.ffi, xmdl.ffi.string
        # xmdl.lib,
        # xmdl.lib.airEnumStr
        # xmdl.lib.airEnumDesc
        # xmdl.lib.airEnumVal
        # xmdl.lib.airEnumValCheck
        if not str(aenm).startswith("<cdata 'airEnum *' "):
            raise TypeError(f'passed argument {aenm} does not seem to be an airEnum pointer')
        self.aenm = aenm
        self.xmdl = xmdl
        self.name = self.xmdl.ffi.string(self.aenm.name).decode('ascii')
        self._name = _name  # the variable name for the airEnum in libteem
        # following definition of airEnum struct in air.h
        self.vals = list(range(1, self.aenm.M + 1))
        if self.aenm.val:
            self.vals = [self.aenm.val[i] for i in self.vals]

    def __call__(self):
        """Returns (a pointer to) the underlying airEnum."""
        return self.aenm

    def __iter__(self):
        """Provides a way to iterate through the valid values of the enum"""
        return iter(self.vals)

    def valid(self, ios) -> bool:  # ios = int or string
        """Answers whether given int is a valid value of enum, or whether given string
        is a valid string in enum, depending on incoming type.
        (wraps airEnumValCheck() and airEnumVal())"""
        if isinstance(ios, int):
            return not self.xmdl.lib.airEnumValCheck(self.aenm, ios)
        if isinstance(ios, str):
            return self.unknown() != self.val(ios)
        # else
        raise TypeError(f'Need an int or str argument (not {type(ios)})')

    def str(self, val: int, picky=False) -> str:
        """Converts from integer enum value val to string identifier
        (wraps airEnumStr())"""
        assert isinstance(val, int), f'Need an int argument (not {type(val)})'
        if picky and not self.valid(val):
            raise ValueError(f'{val} not a valid {self._name} ("{self.name}") enum value')
        # else
        return self.xmdl.ffi.string(self.xmdl.lib.airEnumStr(self.aenm, val)).decode('ascii')

    def desc(self, val: int) -> str:
        """Converts from integer value val to description string
        (wraps airEnumDesc())"""
        assert isinstance(val, int), f'Need an int argument (not {type(val)})'
        return self.xmdl.ffi.string(self.xmdl.lib.airEnumDesc(self.aenm, val)).decode('ascii')

    def val(self, sss: str, picky=False) -> int:
        """Converts from string sss to integer enum value
        (wraps airEnumVal())"""
        assert isinstance(sss, str), f'Need an string argument (not {type(sss)})'
        ret = self.xmdl.lib.airEnumVal(self.aenm, sss.encode('ascii'))
        if picky and ret == self.unknown():
            raise ValueError(f'"{sss}" not parsable as {self._name} ("{self.name}") enum value')
        # else
        return ret

    def unknown(self) -> int:
        """Returns value representing unknown
        (wraps airEnumUnknown())"""
        return self.xmdl.lib.airEnumUnknown(self.aenm)
