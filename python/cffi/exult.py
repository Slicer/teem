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
import cffi as _cffi

# halt if python2; thanks to https://preview.tinyurl.com/44f2beza
_x, *_y = 1, 2  # NOTE: A SyntaxError here means you need python3, not python2
del _x, _y

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


def lib_all() -> list[str]:
    """
    Returns list of all Teem libraries in dependency order
    """
    return list(_tlibs.keys())


def lib_experimental(lib: str) -> bool:
    """
    Answers if a given Teem library is "experimental"
    """
    try:
        info = _tlibs[lib]
    except Exception as exc:
        raise RuntimeError(f'{lib} is not a known Teem library') from exc
    return info['expr']


def lib_depends(lib: str) -> list[str]:
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
    tla = lib_all()   # linear array of all libs in dependency order
    # return dependencies sorted in dependency order
    return sorted(list(newd), key=tla.index)


def lib_headers(lib: str) -> list[str]:
    """
    Returns list of headers (installed by CMake's "make install" the declares the API of given
    Teem library. This is really only the business of things (like build_teem.py) that do the
    one-time generation of cdef headers to be later consumed by CFFI, rather than things that
    use Teem via its python wrappers, but it is nice to have the info about Teem libraries
    centralized to this file.
    """
    try:
        info = _tlibs[lib]
    except Exception as exc:
        raise RuntimeError(f'{lib} is not a known Teem library') from exc
    ret = info['hdrs'].copy() if 'hdrs' in info else []
    ret += [f'{lib}.h']
    return ret


def check_lib_path(lib_path: str) -> None:
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
    lib_fnames = _os.listdir(lib_path)
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


def check_hdr_path(hdr_path: str):
    """
    Sanity check on include path hdr_path.
    Returns (exper, have_libs) where exper indicates if this was run on an "experimental"
    Teem build, and have_libs is the list of libraries for which the .h headers are present
    """
    itpath = hdr_path + '/teem'
    if not _os.path.isdir(itpath):
        raise Exception(f'Need {itpath} to be directory')
    all_libs = lib_all()
    base_libs = list(filter(lambda L: not lib_experimental(L), all_libs))
    expr_libs = list(filter(lib_experimental, all_libs))
    base_hdrs = sum([lib_headers(L) for L in base_libs], [])
    expr_hdrs = sum([lib_headers(L) for L in expr_libs], [])
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


def check_path(path: str):
    """
    Calls check_lib_path and check_hdr_path on given install path "path"
    """
    path = path.rstrip('/')
    hdr_path = path + '/include'
    lib_path = path + '/lib'
    if not _os.path.isdir(hdr_path) or not _os.path.isdir(lib_path):
        raise Exception(f'Need both {hdr_path} and {lib_path} to be subdirs of teem install dir')
    check_lib_path(lib_path)
    (exper, have_libs) = check_hdr_path(hdr_path)
    return (hdr_path, lib_path, have_libs, exper)


def ffi_builder(cdef_path: str, hdr_path: str, lib_path: str, top_lib: str, verb: bool):
    """
    Sets up a builder instance of cffi.FFI() by feeding it all the cdef headers needed for
    library top_lib in Teem, and creates an args dict for builder.set_source. The last steps
    for the caller to do are call builder.set_source and .compile.
    """
    # yes, this is probably redundant with something the caller has already done,
    # but it would be more annoying to pass back (to here) this information, and
    # we need to do error checking anyway
    (exper, have_libs) = check_hdr_path(hdr_path)
    if not top_lib in have_libs:
        raise Exception(f'Requested lib {top_lib} not part of this Teem build: {have_libs}')
    # if 'meet' == top_lib, we're probably being called from build_teem.py, so we use the
    # learned value of exper as part of setting up source_args. Else:
    if 'meet' != top_lib:
        # we set exper according to whether requested library is "experimental"
        exper = lib_experimental(top_lib)
    ffibld = _cffi.FFI()
    # so that teem.py can call free() as part of biff error handling
    ffibld.cdef('extern void free(void *);')
    for lib in lib_depends(top_lib):
        if verb:
            print(f'#################### reading cdef_{lib}.h ...')
        with open(f'{cdef_path}/cdef_{lib}.h', 'r', encoding='utf-8') as file:
            ffibld.cdef(file.read())
    # NOTE that the caller (if not build_teem.py) will need to add to the arrays:
    # 'libraries': with name of new teem-using library X
    # 'runtime_library_dirs': with path to shared library of X.{so,dylib}
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
        'runtime_library_dirs': [_os.path.abspath(lib_path)],
        # keep asserts()
        # https://docs.python.org/3/distutils/apiref.html#distutils.core.Extension
        'undef_macros': ['NDEBUG'],
    }
    return (ffibld, source_args)


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
