# CFFI-based Python wrappers for Teem

The goal is to create useful Python wrappers for Teem.

Background:

- The previous Python wrapping method, using [ctypes](https://docs.python.org/3/library/ctypes.html) (in `../ctypes`) is stalled because the automated generation of ctypes description of the API depended on a patched version of [gccxml](https://gccxml.github.io/HTML/Index.html), which is no longer maintained. A new [llvm-based ctypeslib](https://github.com/trolldbois/ctypeslib) exists and is the likely basis of any future ctypes Teem wraper.
- The current approach uses [CFFI](https://cffi.readthedocs.io/), specifically the ["API, out-of-line" mode](https://cffi.readthedocs.io/en/latest/overview.html#other-cffi-modes), which includes a C compilation step to produce a platform-specific bridge between the Python run-time and calls into the Teem library. This should be faster than ctypes, which cffi would call "ABI, in-line"
- Teem is written in C as a collection of libraries (`air`, `biff`, `hest`, `nrrd`, etc), but compiling and installing with CMake produces a single `libteem` library to link with. CFFI can call into either a static or shared library, but additional dependencies (png, zlib) currently -- for the sake of this wrapping -- require a shared library: `libteem.so` or `libteem.dylib` for Linux or Mac, respectively. Windows should be possible but hasn't been tried.
- Teem's `biff` library is for human-readable error message accumulation. Handling an error in a Teem call like `nrrdLoad()` sometimes involves `biff` subsequent calls (like `biffGetDone()`) to retrieve the error message. Work on these Python wrapers has led to a new systematic way of documenting which functions use biff, and which function return values indicate an error. This information is now stored as `/* Biff: */` annotations at the start of function definitions in Teem source code.
- The following description will use `$TEEM_INSTALL` to refer to whatever directory in which (as the result of a `make install`) CMake installed Teem, with a `lib` subdir containing `libteem`, an `include` subdir containing all the Teem headers (`teem/air.h`, `teem/biff.h`, etc), and `bin` for executables (not relevant for this).
- The following description will use `$TEEM_SRC` to refer to whatever directory has the source checkout of Teem, with a `src` subdir for all sources, and a `python` subdir for python wrappings, and `python/cffi` containing this file.

The various files in this directory (`$TEEM_SRC/python/cffi`), where they come from, and their role:

- `teem.py` (distributed with Teem source): enables "import teem" from python3, and provides access to the Teem API, with the added benefit that Biff errors are turned into Python Exceptions. `teem.py` relies on an `import _teem` which dynamically loads and links:
- `_teem.cpython-`_platformspecifics_`.so` (created by user): The `.compile` method of the `cffi.FFI()` ffi builder creates a platform-specific shared library that bridges Python and `libteem`. Linking with `libteem` also involves all the headers in `$TEEM_INSTALL/include/teem`, but the work of describing the Teem API to CFFI in order to mirror it in Python is done by the `cdef_teem.h` header, created by:
- `build_teem.py` (distributed with Teem source, run by user): This script does a lot of fragile hacking on the headers in `$TEEM_INSTALL/include/teem` to produce a statement in `cdef_teem.h` of the Teem API digestable to the meagre C header parser inside `cffi.FFI()`: basically removing all pre-processor logic and all `#define` macros, keeping only the `#define`s around integers (which can be parsed). Running `python3 build_teem.py $TEEM_INSTALL` creates `cdef_teem.h`, `_teem.c`, and finally compiles the `_teem.cpython` shared object (above). Whether `$TEEM_INSTALL` includes the "experimental" Teem libraries will determine the contents of `cdef_teem.h` and `_teem.c`, which is a reason why these are not distributed with source.

The creation of `teem.py` depends on a few things, and this is staged in the `teemPySrc` subdir.

- `biffDict.py` (distributed with Teem source): this is created by `$TEEM_SRC/src/_util/buildBiffDict.py`, which scans the Teem source files for `/* Biff: */` annotations, and summarizes everything in `biffDict.py`. For the curious, the `/* Biff: */` annotations are created by `$TEEM_SRC/src/_util/scan-symbols.py`. The format of these annotations is defined in `$TEEM_SRC/biff/README.txt`.
- `pre-teem.py` (distributed with Teem source): the contents of `teem.py` except for `biffDict.py`
- `GO.sh` (distributed with Teem source): very minimalist way of combining `pre-teem.py` and `biffDict.py` into `teem.py`, but this really should be via a Makefile or by something smarter.
