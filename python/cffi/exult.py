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
exult.py: CFFI *EX*tension module *U*tilities for *L*ibraries depending on *T*eem.
When creating CFFI extension modules for libraries that depend on Teem (and for
Teem itself), some wrapping of the extension module helps it be more Pythonic,
and more useful. The two main kinds of wrapping are:
* for functions that use biff, wrap it in something that detects the biff error,
  and turn the biff error message into a Python exception
* for airEnums, make a Python object with useful int<->string conversion methods
This functionality used to be in teem.py (the wrapper around the _teem CFFI
extension module), but then other Teem-based extension modules would have to
copy-paste it. Now, anything wrapping a CFFI extension module with .lib elements
that are biff-using functions or airEnums (including teem.py itself), can use
exult.py to generate wrappers for those objects.
"""

import math as _math  # for isnan test that may appear in _BIFFDICT
import sys as _sys

# halt if python2; thanks to https://preview.tinyurl.com/44f2beza
_x, *_y = 1, 2  # NOTE: A SyntaxError here means you need python3, not python2
del _x, _y


class Tenum:
    """Helper/wrapper around (pointers to) airEnums (part of Teem's "air" library).
    This provides convenient ways to convert between integer enum values and real
    Python strings. The C airEnum underlying the Python Tenum foo is still available
    as foo().
    """

    def __init__(self, aenm, _name, xmdl):
        """Constructor takes a Teem airEnum pointer (const airEnum *const)."""
        # xmdl: what extension module is this enum coming from; it it is not _teem itself, then
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
