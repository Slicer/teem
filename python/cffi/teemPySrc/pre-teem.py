# halt if python2; thanks to https://preview.tinyurl.com/44f2beza
_x,*_y=1,2 # NOTE: A SyntaxError here means you need python3, not python2
del _x, _y

# TODO:
# - add teem.NULL
# - do more thinking about how to express testing function returns
# - make re-generation of teem.py by via Makefile, or smarter python script
# - fix info below

# This teem.py is a wrapper (created by teem/python/cffi/teemPyGen/GO.sh) around the
# CFFI-generated (created by teem/python/cffi/build_teem.py) "_teem" Python bindings to
# the C shared library libteem.{so,dylib} (creating by CMake's "make install" build of
# Teem). The main value is that for (some) Teem functions using the "biff" error message
# accumulator, this gets those error messages and raises them as exceptions. How to
# interpret return values as error signifiers varies between functions, and GLK has
# contrived a crude but parsable way of embedding this info in the Teem sources.  For
# more about CFFI see https://cffi.readthedocs.io/en/latest/

import re as _re # for extracting biff key name from function name
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
    print('\n*** teem.py: failed to load shared library wrapper module "_teem.py"')
    print('*** Make sure you first ran: "python3 build_teem.py" to build it')
    print(f'*** an/or try setting the {_lpathVN} environment variable so')
    print(f'*** that the libteem.{_shext} shared library can be found.\n')
    raise
from _teem import ffi

# Note that now "ffi" is the only object "exported" by this module that is not a
# CFFI-generated wrapper for a C symbol in the libteem library. The value of
# this ffi, as opposed to "from cffi import FFI; ffi = FFI()" is that it knows
# about the various typedefs that were learned to build the CFFI wrapper, which
# may in turn be useful for setting up calls into libteem

# It is not problem if this dictionary contains functions from the "experimental"
# libraries that are not actually part of the libteem used
# BIFFDICT

# This traverses the actual symbols in the libteem used, which may or may not
# include the extra "experimenal" libraries
for _sym in dir(_teem.lib):
    if 'free' == _sym:
        # don't export C runtime's free()
        continue;
    # create a python object in this module the library symbol _sym ...
    if not _sym in _biffDict:
        # ... either a straight renaming of the symbol (a function not
        # known to use biff, or something that's not a function)
        _code = f'{_sym} = _teem.lib.{_sym}'
    else:
        # ... or a Python wrapper around a function known to use biff.
        (_mubi, _rvtf, _bkey, _fnln) = _biffDict[_sym]
        _code = f"""
def {_sym}(*args):
    # pass all args to underlying C function; get return value rv
    rv = _teem.lib.{_sym}(*args)
    # if an error if we used biff, and, the return value indicates error
    if ({_mubi} == 0 or args[{_mubi-1}]) and ({_rvtf})(rv):
        err = _teem.lib.biffGetDone(b'{_bkey}')
        estr = ffi.string(err).decode('ascii')
        _teem.lib.free(err)
        raise RuntimeError('from calling C function {_sym} ({_fnln}):\\n'+estr)
    return rv
{_sym}.__doc__ = f'error-checking wrapper around C function {_sym} ({_fnln}):\\n\\n'+_teem.lib.{_sym}.__doc__
"""
    # now run the Python code in _code
    print(_code)
    exec(_code)
