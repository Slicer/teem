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

# halt if python2; thanks to https://preview.tinyurl.com/44f2beza
_x,*_y=1,2 # NOTE: A SyntaxError here means you need python3, not python2
del _x, _y

# For more about teem.py, its functionality, and how it is created,
# see teem/python/cffi/README.md

import math # for math.isnan test
import sys as _sys
if _sys.platform == 'darwin':  # mac
    _lpathVN = 'DYLD_LIBRARY_PATH'
    _shext = 'dylib'
else:
    _lpathVN = 'LD_LIBRARY_PATH'
    _shext = 'so'

try:
    import _teem
except ModuleNotFoundError:
    print('\n*** teem.py: failed to load shared library wrapper module "_teem", from')
    print('*** a shared object file named something like _teem.cpython-platform.so.')
    print('*** Make sure you first ran: "python3 build_teem.py" to build that,')
    print(f'*** and/or try setting the {_lpathVN} environment variable so that')
    print(f'*** the underlying libteem.{_shext} shared library can be found.\n')
    raise

# The value of this ffi, as opposed to "from cffi import FFI; ffi = FFI()" is that it knows
# about the various typedefs that were learned to build the CFFI wrapper, which may in turn
# be useful for setting up calls into libteem
ffi = _teem.ffi
# enable access to original un-wrapped things, straight from cffi
lib = _teem.lib
# for slight convenience, e.g. when calling nrrdLoad with NULL (default) NrrdIoState
NULL = ffi.NULL
# NOTE that "ffi", "lib" and "NULL" are currently the only things "exported" by this module
# that are not either a CFFI-generated wrapper for a C symbol in the libteem library, or an
# extra Python wrapper around that wrapper (as with biff-using functions, and teemEnums)

# this is an experiment
class teemEnum:
    """A helper/wrapper around airEnums (or pointers to them) in Teem, which
    provides convenient ways to convert between integer enum values and real
    Python strings. The C airEnum underlying the Python teemEnum foo is still
    available as both foo.ae and foo().
    """
    def __init__(self, ae):
        """Constructor takes a Teem airEnum pointer (const airEnum *const)."""
        self.ae = ae
        if not str(ae).startswith("<cdata 'airEnum *' "):
            raise TypeError(f'passed argument {ae} does not seem to be a Teem airEnum pointer')
        self.name = _teem.ffi.string(self.ae.name).decode('ascii')
        # looking at airEnum struct definition in air.h
        self.vals = list(range(1, self.ae.M + 1))
        if self.ae.val:
            self.vals = [self.ae.val[i] for i in self.vals]
    def __call__(self):
        """Returns (a pointer to) the underlying Teem airEnum."""
        return self.ae
    def __iter__(self):
        """Provides a way to iterate through the valid values of the enum"""
        return iter(self.vals)
    def str(self, v: int):
        """Converts from integer enum value v to string identifier
        (wraps airEnumStr())"""
        return _teem.ffi.string(_teem.lib.airEnumStr(self.ae, v)).decode('ascii')
    def desc(self, v: int):
        """Converts from integer value v to description string
        (wraps airEnumDesc())"""
        return _teem.ffi.string(_teem.lib.airEnumDesc(self.ae, v)).decode('ascii')
    def val(self, s: str):
        """Converts from string s to integer enum value
        (wraps airEnumVal())"""
        return _teem.lib.airEnumVal(self.ae, s.encode('ascii'))
    def unknown(self):
        """Returns value representing unknown
        (wraps airEnumUnknown())"""
        return _teem.lib.airEnumUnknown(self.ae)

# The following dictionary is for all of Teem, including functions from the
# "experimental" libraries; it is no problem if the libteem in use does not
# actually contain the experimental libs.
# BIFFDICT

# generates a biff-checking wrapper around function func
def _biffer(func, funcName: str, rvtf, mubi: int, bkey, fnln: str):
    def wrapper(*args):
        # pass all args to underlying C function; get return value rv
        rv = func(*args)
        # we have to get biff error if the returned value indicates error (rvtf(rv))
        # and, either: this function definitely uses biff (0 == mubi)
        #          or: (this function maybe uses biff and) "useBiff" args[mubi-1] is True
        if rvtf(rv) and (0 == mubi or args[mubi-1]):
            err = _teem.lib.biffGetDone(bkey)
            estr = ffi.string(err).decode('ascii').rstrip()
            _teem.lib.free(err)
            raise RuntimeError(f'return value {rv} from C function "{funcName}" ({fnln}):\n{estr}')
        return rv
    wrapper.__name__ = funcName
    wrapper.__doc__ = f"""
error-checking wrapper around C function "{funcName}" ({fnln}):
{func.__doc__}
"""
    return wrapper

# This traverses the actual symbols in the libteem used
for _symName in dir(_teem.lib):
    if 'free' == _symName:
        # don't export C runtime's free(), though we use it above in the biff wrapper
        continue
    _sym = getattr(_teem.lib, _symName)
    # Create a python object in this module for the library symbol _sym
    _exp = None
    # The exported symbol _sym is ...
    if not _symName in _biffDict:
        # ... either a function known to not use biff, or, not a function
        if str(_sym).startswith("<cdata 'airEnum *' "):
            # _sym is name of an airEnum, wrap it as such
            _exp = teemEnum(_sym)
        else:
            # straight copy of reference to _sym
            _exp = _sym
    else:
        # ... or a Python wrapper around a function known to use biff.
        (_rvtf, _mubi, _bkey, _fnln) = _biffDict[_symName]
        _exp = _biffer(_sym, _symName, _rvtf, _mubi, _bkey, _fnln)
    globals()[_symName] = _exp
