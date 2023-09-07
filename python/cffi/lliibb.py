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
lliibb.py: A convenience wrapper around the _lliibb extension module, which in turn links into
the underlying liblliibb.{so,dylib} shared library. The main utility of lliibb.py is wrapping
calls into Teem functions that use biff, so that if the function has an error, the biff error
message is converted into a Python exception.  We also introduce the Tenum object for wrapping
an airEnum, and maybe eventually other ways of making pythonic interfaces to lliibb
functionality.  See teem/python/cffi/README.md.

teem/python/cffi/exult.py was likely used to both compile the _lliibb extension module (the
shared library), and to generate this wrapper, which is the result of simple text
transformations of the template wrapper in teem/python/cffi/LLIIBB.py
"""

import math as _math   # # likely used in _BIFF_DICT, below, for testing function return values

# halt if python2; thanks to https://stackoverflow.com/a/65407535/1465384
_x, *_y = 1, 2  # NOTE: A SyntaxError means you need Python3, not Python2
del _x, _y


def string(bstr):
    """Convenience utility for going from C char* bytes to Python string:
    string(B) is just _lliibb.ffi.string(B).decode('ascii')"""
    return _lliibb.ffi.string(bstr).decode('ascii')


class Tenum:
    """Helper/wrapper around (pointers to) airEnums (part of Teem's "air" library).
    This provides convenient ways to convert between integer enum values and real Python
    strings. The C airEnum underlying the Python Tenum foo is still available as foo().
    """

    def __init__(self, aenm, _name):
        """Constructor takes a Teem airEnum pointer (const airEnum *const)."""
        if not str(aenm).startswith("<cdata 'airEnum *' "):
            raise TypeError(f'passed argument {aenm} does not seem to be an airEnum pointer')
        self.aenm = aenm
        self.name = string(self.aenm.name)
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
            return not _lliibb.lib.airEnumValCheck(self.aenm, ios)
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
        return string(_lliibb.lib.airEnumStr(self.aenm, val))

    def desc(self, val: int) -> str:
        """Converts from integer value val to description string
        (wraps airEnumDesc())"""
        assert isinstance(val, int), f'Need an int argument (not {type(val)})'
        return string(_lliibb.lib.airEnumDesc(self.aenm, val))

    def val(self, sss: str, picky=False) -> int:
        """Converts from string sss to integer enum value
        (wraps airEnumVal())"""
        assert isinstance(sss, str), f'Need an string argument (not {type(sss)})'
        ret = _lliibb.lib.airEnumVal(self.aenm, sss.encode('ascii'))
        if picky and ret == self.unknown():
            raise ValueError(f'"{sss}" not parsable as {self._name} ("{self.name}") enum value')
        # else
        return ret

    def unknown(self) -> int:
        """Returns value representing unknown
        (wraps airEnumUnknown())"""
        return _lliibb.lib.airEnumUnknown(self.aenm)


def _equals_one(val):   # likely used in _BIFF_DICT, below
    """Returns True iff given val equals 1"""
    return val == 1


def _equals_null(val):   # likely used in _BIFF_DICT, below
    """Returns True iff given val equals NULL"""
    return val == NULL   # NULL is set at very end of this file


_BIFF_DICT = {  # contents here are filled in by teem/python/cffi/exult.py Tffi.wrap()
    'key': 'val',  # INSERT_BIFFDICT here
}


def _biffer(func, func_name: str, blob):
    """
    generates function wrappers that turn C biff errors into Python exceptions
    """
    (
        rvtf,  # C-function return value test function
        mubi,  # Maybe useBiff index (1-based) into function arguments
        bkey,  # bytes for biff key to retrieve biff error
        fnln,  # filename and line number of C function
    ) = blob

    def wrapper(*args):
        """
        function wrapper that turns C biff errors into Python exceptions
        """
        ret_val = func(*args)
        # we have to get biff error if rvtf(ret_val) indicates error, and,
        # either: this function definitely uses biff (0 == mubi)
        #     or: (this function maybe uses biff and) "useBiff" args[mubi-1] is True
        if rvtf(ret_val) and (0 == mubi or args[mubi - 1]):
            err = _lliibb.lib.biffGetDone(bkey)
            estr = string(err).rstrip()
            _lliibb.lib.free(err)
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


def export_lliibb():
    """
    Exports things from _lliibb.lib, adding biff wrappers to functions where possible.
    """
    for sym_name in dir(_lliibb.lib):
        if 'free' == sym_name:
            # don't export C runtime's free(), though we use it above in the biff wrapper
            continue
        sym = getattr(_lliibb.lib, sym_name)
        # Create a python object in this module for the library symbol sym
        xprt = None
        # The exported symbol xprt will be ...
        if not sym_name in _BIFF_DICT:
            # ... either: not a function, or a function known to not use biff
            if str(sym).startswith("<cdata 'airEnum *' "):
                # _sym is name of an airEnum, wrap it as such
                xprt = Tenum(sym, sym_name)
            else:
                # straight copy of (reference to) sym
                xprt = sym
        else:
            # ... or: a Python wrapper around a function known to use biff.
            xprt = _biffer(sym, sym_name, _BIFF_DICT[sym_name])
        # can't do "if not xprt:" because, e.g. AIR_FALSE is 0 but needs to be exported
        if xprt is None:
            raise Exception(f"didn't handle symbol {sym_name}")
        globals()[sym_name] = xprt


if 'lliibb' == __name__:  # being imported
    try:
        import _lliibb
    except ModuleNotFoundError:
        print('\n*** lliibb.py: failed to "import _lliibb", the _lliibb extension ')
        print('*** module stored in a file named something like: ')
        print('*** _lliibb.cpython-platform.so.')
        print('*** Is there a build_lliibb.py script you can run to recompile it?\n')
        raise
    # The value of this ffi, as opposed to "from cffi import FFI; ffi = FFI()" is that it knows
    # about the various typedefs that were learned to build the CFFI wrapper, which may in turn
    # be useful for setting up calls into liblliibb
    ffi = _lliibb.ffi
    # enable access to original un-wrapped things, straight from cffi
    lib = _lliibb.lib
    # for slight convenience, e.g. when calling nrrdLoad with NULL (default) NrrdIoState
    NULL = _lliibb.ffi.NULL
    # now export/wrap everything
    export_lliibb()
