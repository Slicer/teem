===============
Teem: Tools to process and visualize scientific data and images
Copyright (C) 2009--2025  University of Chicago
Copyright (C) 2005--2008  Gordon Kindlmann
Copyright (C) 1998--2004  University of Utah

This library is free software; you can redistribute it and/or modify it under the terms
of the GNU Lesser General Public License (LGPL) as published by the Free Software
Foundation; either version 2.1 of the License, or (at your option) any later version.
The terms of redistributing and/or modifying this software also include exceptions to
the LGPL that facilitate static linking.

This library is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
You should have received a copy of the GNU Lesser General Public License
along with this library; if not, see <https://www.gnu.org/licenses/>.

=============== How to connect with other Teem users.

In preparation for the Teem v2 release, there's a new Discord server:

  https://discord.gg/xBBqZGXkF7

Join!

=============== License information

See above.  This preamble should appear on all released source files. Full text
of the Simple Library Usage License (SLUL) should be in the file "LICENSE.txt".
The SLUL is the GNU Lesser General Public License v2.1, plus an exception:
statically-linked binaries that link with Teem can be destributed under the
terms of your choice, with very modest provisions.

=============== How to compile

Use CMake to compile Teem.  CMake is available from:

http://www.cmake.org/

An example sequence of commands to build Teem and the "unu" utility
(with static-linking):

  svn co https://svn.code.sf.net/p/teem/code/teem/trunk teem-src
  TEEM_SRC=$(cd teem-src && pwd)
  TEEM_INSTALL=$(mkdir teem-install && cd teem-install && pwd)
  mkdir teem-build && cd teem-build
  cmake \
    -D BUILD_SHARED_LIBS=ON -D BUILD_TESTING=OFF \
    -D CMAKE_BUILD_TYPE=Release \
    -D Teem_USE_FFTW3=OFF -D Teem_USE_LEVMAR=OFF \
    -D Teem_USE_BZIP2=ON -D Teem_USE_PNG=ON \
    -D Teem_USE_ZLIB=ON -D Teem_USE_PTHREAD=ON \
    -D CMAKE_INSTALL_PREFIX=$TEEM_INSTALL \
    ../teem-src
  make install

If you're squeamish about your command-line tools relying on shared libraries,
use BUILD_SHARED_LIBS=OFF. btw Teem's CMake sets the rpath in the command-line
tools to be relative, so the install directory can be moved elsewhere, as long
as the `bin` and `lib` directories stay side-by-side.

Using BUILD_SHARED_LIBS=ON makes a libteem shared library, which enables the
python/CFFI wrapping, which you do with this (with a python3 that can
"import cffi"):

  cd $TEEM_SRC/python/cffi && python3 build_teem.py $TEEM_INSTALL

Then you can, in python3:

  import teem

and have fast compiled access to the entire Teem API, with biff error messages
turned into Python exceptions.

=============== Directory Structure

* CMake/
  Files related to compiling Teem with CMake

* src/
  With one subdirectory for each of the teem libraries (air, biff, hest, nrrd, etc);
  All the source for the libraries is in here.  See library listing below.
  The src/CODING.txt file documents Teem coding conventions.
  * src/make
    Files related to compiling Teem with src/GNUmakefile, the old way
    of making Teem prior to CMake.  This is still unofficially in use.
  * src/bin
    Source files for Teem command-line tools, including "unu" and "tend"

* UseTeemCMakeDemo/
  An stand-alone example of how to use the Teem::Teem package, created by CMake
  (via the new CMake files re-written Sept 2025)

* include/
  Some short header files that are used to check the setting of compiler
  variables while building Teem, but not used by any downstream user.
  Currently just teemPng.h, which ensures that support for zlib compression
  is being compiled in if png format is being compiled in.

* python/
  For python wrappings
  * python/cffi
    Bindings for python via CFFI, for all of Teem (in teem.py), as well as a
    way (exult.py) to create bindings for other C libraries that depend on Teem
    (created in August 2022)
  * python/ctypes
    Bindings for python via ctypes; currently out of date because it
    depended on the moribund gccxml

* built/
  The non-CMake GNUmake system puts things here:
  * include/
    the headers
  * lib/
    all libraries put their static/archive (.a) ibrary files here
  * bin/
    all command-line tools (like unu) go here
  * obj/
    make puts all the .o files in here, for all libraries, hopefully with no
    name clashes.

* tests/
  * ctest/
    Tests run by CTest.  More should be added.
  * python/
    Future home of tests that access Teem via Python/CFFI bindings

* data/
  Small reference datasets; more may be added for testing

* matlab/
  Matlab bindings written in 2005, not touched since then; do they still work?

=============== Teem libraries

Teem is a coordinated collection of libraries, with a stable dependency graph.
Below is a listing of the libraries (with indication of the libraries upon
which it depends).  (TEEM_LIB_LIST)

* air: Basic utility functions, used throughout Teem

* hest: Command-line parsing (air)

* biff: Accumulation of error messages (air)

* nrrd: Nearly Raw Raster Data- library for raster data manipulation, and
support for NRRD file format (biff, hest, air)

* ell: Linear algebra: operations on vectors, matrices and quaternions, and
solving cubic polynomials. (nrrd, biff, air)

* moss: Processing of 2D multi-channel images (ell, nrrd, biff, hest, air)

* unrrdu: internals of "unu" command-line tool, and some machinery used in
other multi-command tools (like "tend") (moss, nrrd, biff, hest, air). "unu
ilk" is new home for "ilk" image transformer built on moss

* alan: Reaction-diffusion textures (nrrd, ell, biff, air)

* tijk: Spherical harmonics and higher-order tensors (ell, nrrd, air)

* gage: Convolution-based measurement of 3D fields, or 4D scale-space (ell,
nrrd, biff, air)

* dye: Color spaces and conversion (ell, biff, air)

* bane: Implementation of Semi-Automatic Generation of Transfer Functions
(gage, unrrdu, nrrd, biff, air)

* limn: Basics of computer graphics, including polygonal data representation
and manipulation (ell, unrrdu, nrrd, biff, hest, air)

* echo: Simple ray-tracer, written for class (limn, ell, nrrd, biff, air)

* hoover: Framework for multi-thread ray-casting volume renderer (limn, ell,
nrrd, biff, air)

* seek: Extraction of polygonal features from volume data, including Marching
Cubes and ridge surfaces (limn, gage, ell, nrrd, biff, hest, air)

* ten: Visualization and analysis of diffusion imaging and diffusion tensor
fields (echo, limn, gage, unrrdu, ell, nrrd, biff, air)

* elf: Visualization/processing of high-angular resolution diffusion imaging
(ten, tijk, limn, ell, nrrd, air)

* pull: Particle systems for image feature sampling in 3D or 4D scale-space
(ten, limn, gage, ell, nrrd, biff, hest, air)

* coil: Non-linear image filtering (ten, ell, nrrd, biff, air)

* push: Original implmentation of glyph packing for DTI (ten, gage, ell, nrrd,
biff, air)

* mite: Hoover-based volume rendering with gage-based transfer functions (ten,
hoover, limn, gage, ell, nrrd, biff, air)

* meet: Uniform API to things common to all Teem libraries (mite, push, coil,
pull, elf, ten, seek, hoover, echo, limn, bane, dye, gage, tijk, moss, alan,
unrrdu, ell, nrrd, biff, hest, air)

=============== Teem comand-line tools

The easiest way to access the functionality in Teem is with its command-line
tools.  Originally intended only as demos for the Teem libraries and using
their APIs, the command-line tools have become a significant way of getting
real work done.  Source for the tools is in teem/src/bin.  The most commonly
used tools are:

* unu: uses the "nrrd" library; a fast way to do raster data processing and
visualization. What the rest of the world does in numpy, you maybe able to do
on the command-line with unu. Type "unu" to see the commands; type "unu all" to
see extra hidden commands (like "unu undos" for converting text files from
Windows to normal)

* tend: uses the "ten" library; for DW-MRI and DTI processing

* gprobe (and vprobe, pprobe): uses the "gage" library, allows measuring
gage items in scalar, vector, tensor, and DW-MRI volumes.

* miter: uses the "mite" library; a flexible volume renderer

* overrgb: for compositing an RGBA image over some background
