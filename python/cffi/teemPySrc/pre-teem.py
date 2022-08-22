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
# NOTE that now "ffi" and "NULL" are only things "exported" by this module that are not a
# CFFI-generated wrapper for a C symbol in the libteem library.

# The following dictionary is for all of Teem, including functions from the
# "experimental" libraries; it is no problem if the libteem in use does not
# actually contain the experimental libs.
# BIFFDICT

# experimental helper/wrapper around airEnums.
# The underlying cffi object for airEnum foo is available is foo.ae
class airEnum:
    def __init__(self, ae):
        self.ae = ae
        self.name = _teem.ffi.string(self.ae.name).decode('ascii')
        # looking at airEnum struct definition in air.h
        self.vals = list(range(1, self.ae.M + 1))
        if self.ae.val:
            self.vals = [self.ae.val[i] for i in self.vals]
    def __iter__(self):
        return iter(self.vals)
    def str(self, v):
        return _teem.ffi.string(_teem.lib.airEnumStr(self.ae, v)).decode('ascii')
    def desc(self, v):
        return _teem.ffi.string(_teem.lib.airEnumDesc(self.ae, v)).decode('ascii')
    def val(self, s):
        return _teem.lib.airEnumVal(self.ae, s.encode('ascii'))

# This traverses the actual symbols in the libteem used
for _sym in dir(_teem.lib):
    if 'free' == _sym:
        # don't export C runtime's free(), though we use it below for biff
        continue
    # Create a python object in this module for the library symbol _sym.
    # The dir() above returns a list of names of symbols, not a list of symbols,
    # so we are in the unfortunate business of generating the text of python
    # code that is later passed to exec().  The exported symbol _sym is ...
    if not _sym in _biffDict:
        # ... either a function known to not use biff, or, not a function
        # (hacky way to learn about a object we can only refer to by name)
        exec(f'_is_airEnum = "airEnum *" in str(_teem.lib.{_sym})')
        if not _is_airEnum:
            # straight renaming of _sym
            _code = f'{_sym} = _teem.lib.{_sym}'
        else:
            # _sym is name of an airEnum, wrap it as such
            _code = f'{_sym} = airEnum(_teem.lib.{_sym})'
    else:
        # ... or a Python wrapper around a function known to use biff.
        (_rvte, _bkey, _fnln) = _biffDict[_sym]
        _code = f"""
def {_sym}(*args):
    # pass all args to underlying C function; get return value rv
    rv = _teem.lib.{_sym}(*args)
    # evaluate return value test expression ('rv' and 'args' hardcoded)
    if {_rvte}:
        err = _teem.lib.biffGetDone(b'{_bkey}')
        estr = ffi.string(err).decode('ascii')
        _teem.lib.free(err)
        raise RuntimeError(f'return value {{rv}} from C function {_sym} ({_fnln}):\\n'+estr)
    return rv
{_sym}.__doc__ = f'error-checking wrapper around C function {_sym} ({_fnln}):\\n\\n'+_teem.lib.{_sym}.__doc__
"""
    # now evaluate the Python code in _code
    #print(_code)
    exec(_code)
