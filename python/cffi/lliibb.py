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
import sys as _sys
import argparse as _argparse

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
            raise TypeError(
                f'passed argument {aenm} does not seem to be an airEnum pointer'
            )
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

    def vals(self):
        """Provides list of valid values"""
        return self._vals.copy()

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

    def str(self, val: int, picky=False, excls=ValueError) -> str:
        """Converts from integer enum value val to string identifier.
        If picky, then failure to parse string generates an exception,
        of class excls (defaults to ValueError) (wraps airEnumStr())"""
        assert isinstance(val, int), f'Need an int argument (not {type(val)})'
        if picky and not self.valid(val):
            raise excls(f'{val} not a valid {self._name} ("{self.name}") enum value')
        # else
        return _lliibb.ffi.string(_lliibb.lib.airEnumStr(self.aenm, val)).decode('utf8')

    def strs(self):
        """Provides a list of strings for the valid values"""
        return [self.str(v) for v in self.vals()]

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
            raise ValueError(
                f'"{sss}" not parsable as {self._name} ("{self.name}") enum value'
            )
        # else
        return ret

    def unknown(self) -> int:
        """Returns value representing unknown
        (wraps airEnumUnknown())"""
        return _lliibb.lib.airEnumUnknown(self.aenm)


class TenumVal:
    """Represents one value in a Tenum, in a way that can be both an int and a string,
    and that remembers which Tenum it is part of"""

    def __init__(self, tenum, ios):
        """Create enum value from Tenum and either int or string value"""
        if not isinstance(tenum, Tenum):
            raise ValueError(f'Given {tenum=} not actually a Tenum')
        if not tenum.valid(ios):
            raise ValueError(f'Given {ios=} not member of enum {tenum.name}')
        self.tenum = tenum
        self.val = ios if isinstance(ios, int) else tenum.val(ios)

    def __str__(self):
        """Return own value as string"""
        return self.tenum.str(self.val)

    def __int__(self):
        """Return own value as int"""
        return self.val

    def __repr__(self):
        """Full string representation of own value
        (though currently nothing knows how to parse this)"""
        return f'{self.tenum.str(self.val)}:({self.tenum.name} enum)'

    def __eq__(self, other):
        """Test equality between two things"""
        if isinstance(other, TenumVal):
            if self.tenum != other.tenum:
                return False
            # else other same kind of tenum
            return self.val == other.val
        # else other not a tenum
        if not self.tenum.valid(other):
            raise ValueError(f'Given {other=} not member of enum {self.tenum.name}')
        oval = other if isinstance(other, int) else self.tenum.val(other)
        return self.val == oval


class TenumValParseAction(_argparse.Action):
    """Custom argparse action for parsing a TenumVal from the command-line"""

    def __init__(self, option_strings, dest, tenum, **kwargs):
        super().__init__(option_strings, dest, **kwargs)
        # Store the enum we want to parse
        self.tenum = tenum

    def __call__(self, _parser, namespace, string, _option_string=None):
        # parse value from string
        val = self.tenum.val(string, True, _argparse.ArgumentTypeError)
        tval = TenumVal(self.tenum, val)
        # Set the parsed result in the namespace
        setattr(namespace, self.dest, tval)


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


# NOTE: this is copy-pasta from GLK's SciVis class code, and the python wrappers there
class _lliibb_Module:
    """An object that exists just to "become" the imported module, an old hack[1,2]
    that is still needed because even though there is now module-level __getattr__[3],
    module-level __setattr__ has been sadly rejected[4,5]. Using __setattr__ here
    solves two problems: (1) supporting true read-only variables, which avoids easy but
    confusing mistakes (should NOT be able to over-write the C functions we need to use,
    and accurately reflecting C names (of enums, functions) matters more than hewing to
    the mere convention that ALL-CAPS Python variables are constants), and, (2) creating
    useful aliases for global variables so that they are actually set in the underlying
    C library instead of just modifying a Python-only variable.
    [1] https://groups.google.com/g/comp.lang.python/c/2H53WSCXhoM
    [2] https://mail.python.org/pipermail/python-ideas/2012-May/014969.html
    [3] https://peps.python.org/pep-0562/
    [4] https://discuss.python.org/t/extend-pep-562-with-setattr-for-modules/25506
    [5] https://discuss.python.org/t/pep-726-module-setattr-and-delattr/32640
    """

    # _done being False indicates to __setattr__ that __init__ is in progress
    _done = False

    def __init__(self):
        """Set up all the extension module wrapping"""
        # we init ourself with the globals() to best emulate being a module
        for kk, vv in globals().items():
            setattr(self, kk, vv)
        # set various things that simplify using CFFI and this module
        # for slight convenience, e.g. when calling nrrdLoad with NULL (default) NrrdIoState
        self.NULL = _lliibb.ffi.NULL
        # The value of this ffi, as opposed to "from cffi import FFI; ffi = FFI()" is that it knows
        # about the various typedefs that were learned to build the CFFI wrapper, which may in turn
        # be useful for setting up calls into liblliibb
        self.ffi = _lliibb.ffi
        # enable access to original un-wrapped things, straight from cffi
        self.lib = _lliibb.lib
        # for non-const things, self._alias maps from exported name to CFFI object
        # in the underlying library
        self._alias = {}
        # go through everything in underlying C library, and process accordingly
        for sym_name in dir(_lliibb.lib):
            if 'free' == sym_name:
                # don't export C runtime's free(), though we use it above in the biff wrapper
                continue
            # sym is the symbol with name sym_name
            # (not __lib_.lib[sym_name] since '_cffi_backend.Lib' object is not subscriptable)
            sym = getattr(_lliibb.lib, sym_name)
            # string useful for distinguishing different kinds of CFFI objects
            strsym = str(sym)
            # The exported symbol xprt will be ...
            if sym_name in _BIFF_DICT:
                # ... or: a Python wrapper around a function known to use biff.
                setattr(self, sym_name, _biffer(sym, sym_name, _BIFF_DICT[sym_name]))
            # else either a function known to not use biff, or not a function,
            elif strsym.startswith("<cdata 'airEnum *' "):
                # _sym is name of an airEnum, wrap it as such
                setattr(self, sym_name, Tenum(sym, sym_name))
            elif (
                # Annoyingly, functions in _lliibb.lib can either look like
                # <cdata 'int(*)(char *, ...)' 0x10af91330> or like
                # <built-in method _lib_Foo of _cffi_backend.Lib object at 0x10b0cd210>
                (strsym.startswith('<cdata') and '(*)(' in strsym)  # some functions
                or strsym.startswith('<built-in method')  # other functions
                # ridiculous list of wacky types of things in Teem
                or strsym.startswith("<cdata 'void(*[")
                or strsym.startswith("<cdata 'char *")
                or strsym.startswith("<cdata 'char[")
                or strsym.startswith("<cdata 'unsigned int *")
                or strsym.startswith("<cdata 'unsigned int(*")
                or strsym.startswith("<cdata 'unsigned int[")
                or strsym.startswith("<cdata 'int[")
                or strsym.startswith("<cdata 'int(*[")
                or strsym.startswith("<cdata 'float(*[")
                or strsym.startswith("<cdata 'double[")
                or strsym.startswith("<cdata 'double(*[")
                or strsym.startswith("<cdata 'size_t[")
                or strsym.startswith("<cdata 'size_t(*[")
                or strsym.startswith("<cdata 'struct ")
                or strsym.startswith("<cdata 'airFloat &'")
                or strsym.startswith("<cdata 'hestCB &'")
                or strsym.startswith("<cdata 'airRandMTState *'")
                or strsym.startswith("<cdata 'hestCB *'")
                or strsym.startswith("<cdata 'unrrduCmd * *'")
                or strsym.startswith("<cdata 'gageItemPack *'")
                or strsym.startswith("<cdata 'NrrdFormat *")
                or strsym.startswith("<cdata 'NrrdKernel *")
                or strsym.startswith("<cdata 'coilKind &'")
                or strsym.startswith("<cdata 'coilKind *")
                or strsym.startswith("<cdata 'coilMethod *")
                or strsym.startswith("<cdata 'pushEnergy *")
                or strsym.startswith("<cdata 'pullEnergy *")
            ):
                # with C strings, it might be cute to instead export a real Python string, but
                # then its value would NOT be useful as is for the underlying C library.
                setattr(self, sym_name, sym)
            else:
                # More special cases; see if sym is an integer constant: enum or #define
                cval = None  # value of symbol as integer const
                try:
                    cval = _lliibb.ffi.integer_const(sym_name)
                except _lliibb.ffi.error:
                    # sym_name wasn't actually an integer const; ignore the complaint.
                    pass
                if cval is sym:
                    # so sym_name *is* an integer const, export that (integer) value
                    setattr(self, sym_name, sym)
                elif (
                    isinstance(sym, int)
                    or isinstance(sym, float)
                    or isinstance(sym, bytes)
                ):
                    # sym_name is a NON-CONST scalar, do not export, instead alias it
                    self._alias[sym_name] = sym_name
                    # HEY in the python wrappers for GLK's SciVis, this aliasing is
                    # non-trivial e.g. `foo.Verbose` aliases _foo.lib.fooVerbose`.
                    # Here, `_alias` instead becomes a badly-named mechanism to indicate
                    # which exports might be mutable
                else:
                    raise ValueError(
                        f'Libary item {sym_name} is something ({strsym}) unexpected; sorry'
                    )
        # done looping through symbols
        # Fake out the name of this class to be name of wannabe module
        self.__class__.__name__ = __name__
        # Prevent further changes
        self._done = True

    def __setattr__(self, name, value):
        """Allow new attributes during __init__, then be read-only except for aliased variables"""
        if not self._done:
            # self.__init__ is still in progress; pass through
            self.__dict__[name] = value
            return
        # else __init__ is done; have turned mostly read-only
        if name in self._alias:
            # the aliases are the one thing we allow changing
            setattr(self.lib, self._alias[name], value)
            # and we're done
            return
        # else try to give informative exceptions to documenting being read-only
        if not name in self.__dict__:
            raise ValueError(
                f'"{name}" not already in {self.__name__} wrapper module '
                'and cannot add new elements.'
            )
        raise ValueError(
            f'Cannot change "{name}" in {self.__name__} wrapper module'
            # not saying this because somethings like _lib_.NULL or .NAN
            # are not actually in the underlying C library
            # 'corresponding element of underlying C library is const.'
        )

    def __getattr__(self, name):
        """Handle requests for attributes that don't really exist;
        currently just the aliased variables"""
        if name in self._alias:
            return getattr(self.lib, self._alias[name])
        # else not an alias
        raise KeyError(f'"{name}" not in {self.__name__} wrapper module')

    def __dir__(self):
        """Directory of self, hacked to include the aliased variables"""
        lst = list(self.__dict__.keys())
        for name in self._alias:
            lst.append(name)
        return lst

    # other utility functions that come in handy
    def str(self, cstr):
        """Utility function from C char* to Python string; nothing more than
        str(cstr) = _lliibb.ffi.string(cstr).decode('utf8')"""
        return _lliibb.ffi.string(cstr).decode('utf8')

    def cs(self, pstr: str):
        """Utility function from Python string to something compatible with C char*.
        Has such a short name ("cs" could stand for "C string" or "char star") to be
        shorter than its simple implementation: cs(pstr) = pstr.encode('utf8')"""
        return pstr.encode('utf8')


if 'lliibb' == __name__:  # being imported
    try:
        import _lliibb
    except ModuleNotFoundError:
        print('\n*** lliibb.py: failed to "import _lliibb", the _lliibb extension ')
        print('*** module stored in a file named something like: ')
        print('*** _lliibb.cpython-platform.so.')
        print('*** Is there a build_lliibb.py script you can run to recompile it?\n')
        raise
    # Finally, the object-instance-becomes-the-module fake-out workaround described in the
    # __lib_Module docstring above and the links therein.
    _sys.modules[__name__] = _lliibb_Module()
