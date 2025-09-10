This is a simple demo of how another C program can use CMake to link
with a CMake-built Teem (it also reflects how I (GLK) am belatedly
understanding how CMake works).

Steps:

1) Set TEEM_DIR to the path to where CMake installed Teem.  There should be
`bin`, `include`, `lib`, subdirectories, and the `lib/cmake` will record how
CMake built Teem.  Then:

  ls $TEEM_DIR

should show `bin`, `include`, `lib`.

2) The following will create a `build` subdirectory right here where testio.c
is, doing the configure and generate steps of CMake:

  cmake -S . -B build \
     -DCMAKE_PREFIX_PATH=$TEEM_DIR \
     -DCMAKE_INSTALL_PREFIX=install

The `.` argument to `cmake -S` is the path to the source directory, and the
`build` argument to `cmake -B` is the path to the new testio build directory.
After the `find_package` in ./CMakeLists.txt runs, CMake will set its own
`Teem_DIR` variable to $TEEM_DIR. The `install` local (relative) path will be
the subdirectory that CMake will install into later.

3) To build in a way that is agnostic w.r.t. build system:

  cmake --build build

which means: run the "build" step (first "--build") in the local `build`
subdirectory (second "build").  Or if you like running make:

  (cd build; make)

4) To install:

  cmake --install build
