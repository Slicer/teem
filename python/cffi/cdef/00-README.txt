The *.h header files in here document Teem's C API for the sake of Python's
CFFI. They are created by ../build_teem.py (run periodically to stay in synch
with Teem source), and read by ../exult.py, which passes their contents to
cffi.ffi.cdef(), so that CFFI can compile its bridge between Python and C.

They differ from the real teem/*.h header files because CFFI's reader does not
handle #include, most #define's, or most anything else that hinges on the
pre-processor.

There is no value in editing these files by hand - it will only cause confusion
by having a mismatch between the real headers and these for declaring the
Python bindings.
