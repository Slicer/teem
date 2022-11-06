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
A python module to wrap the CFFI-generated _teem module (which links into the libteem
shared library). Main utility is converting calls into Teem functions that generated
a biff error message into a Python exception containing the error message.  Also
introduces the Tenum object for wrapping an airEnum, and eventually other ways of
making pythonic interfaces to Teem functionality.  See teem/python/cffi/README.md.
"""

# pylint can't see inside CFFI-generated modules, and
# long BIFFDICT lines don't need to be human-friendly
# pylint: disable=c-extension-no-member, line-too-long

import math as _math  # for isnan test that may appear in _BIFFDICT
import sys as _sys

# halt if python2; thanks to https://preview.tinyurl.com/44f2beza
_x, *_y = 1, 2  # NOTE: A SyntaxError here means you need python3, not python2
del _x, _y


class Tenum:
    """A helper/wrapper around airEnums (or pointers to them) in Teem, which provides
    convenient ways to convert between integer enum values and real Python strings.
    The C airEnum underlying the Python Tenum foo is still available as foo().
    """

    def __init__(self, aenm, _name):
        """Constructor takes a Teem airEnum pointer (const airEnum *const)."""
        if not str(aenm).startswith("<cdata 'airEnum *' "):
            raise TypeError(f'passed argument {aenm} does not seem to be a Teem airEnum pointer')
        self.aenm = aenm
        self.name = _teem.ffi.string(self.aenm.name).decode('ascii')
        self._name = _name  # the variable name for the airEnum in libteem
        # following definition of airEnum struct in air.h
        self.vals = list(range(1, self.aenm.M + 1))
        if self.aenm.val:
            self.vals = [self.aenm.val[i] for i in self.vals]

    def __call__(self):
        """Returns (a pointer to) the underlying Teem airEnum."""
        return self.aenm

    def __iter__(self):
        """Provides a way to iterate through the valid values of the enum"""
        return iter(self.vals)

    def valid(self, ios) -> bool:  # ios = int or string
        """Answers whether given int is a valid value of enum, or whether given string
        is a valid string in enum, depending on incoming type.
        (wraps airEnumValCheck() and airEnumVal())"""
        if isinstance(ios, int):
            return not _teem.lib.airEnumValCheck(self.aenm, ios)
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
        return _teem.ffi.string(_teem.lib.airEnumStr(self.aenm, val)).decode('ascii')

    def desc(self, val: int) -> str:
        """Converts from integer value val to description string
        (wraps airEnumDesc())"""
        assert isinstance(val, int), f'Need an int argument (not {type(val)})'
        return _teem.ffi.string(_teem.lib.airEnumDesc(self.aenm, val)).decode('ascii')

    def val(self, sss: str, picky=False) -> int:
        """Converts from string sss to integer enum value
        (wraps airEnumVal())"""
        assert isinstance(sss, str), f'Need an string argument (not {type(sss)})'
        ret = _teem.lib.airEnumVal(self.aenm, sss.encode('ascii'))
        if picky and ret == self.unknown():
            raise ValueError(f'"{sss}" not parsable as {self._name} ("{self.name}") enum value')
        # else
        return ret

    def unknown(self) -> int:
        """Returns value representing unknown
        (wraps airEnumUnknown())"""
        return _teem.lib.airEnumUnknown(self.aenm)


# The following dictionary is for all of Teem, including functions from the
# "experimental" libraries; it is no problem if the libteem in use does not
# actually contain the experimental libs.
# BIFFDICT

# generates a biff-checking wrapper around function func


def _biffer(func, func_name: str, rvtf, mubi: int, bkey, fnln: str):
    def wrapper(*args):
        # pass all args to underlying C function; get return value
        ret_val = func(*args)
        # we have to get biff error if rvtf(ret_val) == ret_valindicates error
        # and, either: this function definitely uses biff (0 == mubi)
        #          or: (this function maybe uses biff and) "useBiff" args[mubi-1] is True
        if rvtf(ret_val) and (0 == mubi or args[mubi - 1]):
            err = _teem.lib.biffGetDone(bkey)
            estr = ffi.string(err).decode('ascii').rstrip()
            _teem.lib.free(err)
            raise RuntimeError(
                f'return value {ret_val} from C function "{func_name}" ({fnln}):\n{estr}'
            )
        return ret_val

    wrapper.__name__ = func_name
    wrapper.__doc__ = f"""
error-checking wrapper around C function "{func_name}" ({fnln}):
{func.__doc__}
"""
    return wrapper


def export_teem():
    """Exports things from _teem.lib, adding biff wrappers to functions where possible."""
    for sym_name in dir(_teem.lib):
        if 'free' == sym_name:
            # don't export C runtime's free(), though we use it above in the biff wrapper
            continue
        sym = getattr(_teem.lib, sym_name)
        # Create a python object in this module for the library symbol sym
        exp = 'unset'
        # The exported symbol _sym is ...
        if not sym_name in _BIFFDICT:
            # ... either a function known to not use biff, or, not a function
            if str(sym).startswith("<cdata 'airEnum *' "):
                # _sym is name of an airEnum, wrap it as such
                exp = Tenum(sym, sym_name)
            else:
                # straight copy of (reference to) sym
                exp = sym
        else:
            # ... or a Python wrapper around a function known to use biff.
            (rvtf, mubi, bkey, fnln) = _BIFFDICT[sym_name]
            exp = _biffer(sym, sym_name, rvtf, mubi, bkey, fnln)
        # can't do "if not exp:" because, e.g. AIR_FALSE is 0 but needs to be exported
        if 'unset' == exp:
            raise Exception(f"didn't handle symbol {sym_name}")
        globals()[sym_name] = exp


if 'teem' == __name__:  # being imported
    if _sys.platform == 'darwin':  # mac
        _LPVNM = 'DYLD_LIBRARY_PATH'
        _SHEXT = 'dylib'
    else:
        _LPVNM = 'LD_LIBRARY_PATH'
        _SHEXT = 'so'
    try:
        import _teem
    except ModuleNotFoundError:
        print('\n*** teem.py: failed to load shared library wrapper module "_teem", from')
        print('*** a shared object file named something like _teem.cpython-platform.so.')
        print('*** Make sure you first ran: "python3 build_teem.py" to build that,')
        print(f'*** and/or try setting the {_LPVNM} environment variable so that')
        print(f'*** the underlying libteem.{_SHEXT} shared library can be found.\n')
        raise
    # The value of this ffi, as opposed to "from cffi import FFI; ffi = FFI()" is that it knows
    # about the various typedefs that were learned to build the CFFI wrapper, which may in turn
    # be useful for setting up calls into libteem
    ffi = _teem.ffi
    # enable access to original un-wrapped things, straight from cffi
    lib = _teem.lib
    # for slight convenience, e.g. when calling nrrdLoad with NULL (default) NrrdIoState
    NULL = _teem.ffi.NULL
    export_teem()
