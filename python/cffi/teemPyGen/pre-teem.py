# halt if python2; thanks to https://preview.tinyurl.com/44f2beza
_x,*_y=1,2 # NOTE: A SyntaxError here means you need python3, not python2
del _x, _y

# This teem.py is a wrapper (created by teem/python/cffi/teemPyGen/go.py) around the
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
    import _teem.lib as _lib
except ModuleNotFoundError:
    print('\n*** teem.py: failed to load shared library wrapper module_teem')
    print('*** Make sure you first ran: "python3 build_teem.py" to build it')
    print(f'*** an/or try setting the {_lpathVN} environment variable so')
    print(f'*** that the libteem.{_shext} shared library can be found.\n')
    raise
from _teem import ffi
NULL = ffi.NULL

# Note that now "ffi" is the only object "exported" by this module that is not a
# CFFI-generated wrapper for a C symbol in the libteem library. The value of
# this ffi, as opposed to "from cffi import FFI; ffi = FFI()" is that it knows
# about the various typedefs that were learned to build the CFFI wrapper, which
# may in turn be useful for setting up calls into libteem

_biffList = [
# BIFF_LIST
]

# dictionary mapping from function name to return value signifying error
_biffing = {}
for _func in _biffList:
    if '*' == _func[0]:
        # if function name starts with '*', then NULL means biff error
        _ff = _func[1:]
        _ee = ffi.NULL
    else:
        # returning 1 means error, 0 means all ok
        _ff = _func
        _ee = 1
    _biffing[_ff] = _ee

for _sym in dir(_lib):
    if 'free' == _sym:
        # don't export C runtime's free()
        continue;
    # create a python object in this module the library symbol _sym ...
    if not _sym in _biffing:
        # ... either a straight renaming of the symbol (a function not
        # known to use biff, or something that's not a function)
        _code = f'{_sym} = _lib.{_sym}'
    else:
        # ... or a Python wrapper around a function known to use biff. The
        # biff key for a given function is (BY CONVENTION ONLY) the string
        # of lower-case letters prior to the first upper-case letter.
        _bkey = _re.findall('^[^A-Z]*', _sym)[0]
        _code = f"""
def {_sym}(*args):
    # pass all the args to the underlying C function
    ret = _lib.{_sym}(*args)
    # have an error if integer return >= 1 or pointer return is NULL
    if {'1 <=' if _biffing[_sym] == 1 else 'ffi.NULL =='} ret:
        err = _lib.biffGetDone(b'{_bkey}')
        estr = ffi.string(err).decode('ascii')
        _lib.free(err)
        raise RuntimeError('from calling C function {_sym}:\\n'+estr)
    return ret
{_sym}.__doc__ = f'error-checking wrapper around C function {_sym}:\\n\\n'+_lib.{_sym}.__doc__
"""
    # now run the Python code in _code
    exec(_code)
