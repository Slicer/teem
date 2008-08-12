=============== 
  Teem: Tools to process and visualize scientific data and images
  Copyright (C) 2008, 2007, 2006, 2005  Gordon Kindlmann
  Copyright (C) 2004, 2003, 2002, 2001, 2000, 1999, 1998  University of Utah

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  The terms of redistributing and/or modifying this software also
  include exceptions to the LGPL that facilitate static linking.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

=============== License information

See above.  This preamble appears on all .c, .h, and .mk files. Full
text of the GNU Lesser General Public License should be in the file
"LICENSE.txt" in the "src" directory.  See the web page at
<http://teem.sourceforge.net/lgpl.html> relative to this file for my
understanding of exactly what the LGPL means for people wishing to use
any of the teem libraries in their own programs.

=============== How to compile

Use CMake to compile Teem.  CMake is available from:

http://www.cmake.org/

The information below refers to the old home-grown bizarro
GNUmakefiles that Gordon originally wrote.  These may still work,
but they are not supported.

1: set the environment variable TEEM_ARCH to one of the following:
   "irix6.n32": for irix in n32 mode
   "irix6.64": for irix in 64-bit mode
   "linux.32": for 32-bit linux boxes
   "linux.ia64": for 64-bit Itanium (Intel) linux
   "linux.amd64": for 64-bit Athlon (AMD) linux
   "solaris": for solaris boxes
   "cygwin": if you have cygwin (www.cygwin.com) on a windows box
   "darwin.32": on a 32-bit Mac OS X box
   "darwin.64": on a 64-bit Mac OS X box

2: "cd src"

3: "make" 
   Or more accurately, "gmake", or "/usr/local/gnu/bin/make", or
   however it is that you invoke a GNU make.  This MUST be GNU make.
   I am in fact using features unique to GNU make.

4: "../${TEEM_ARCH}/bin/nrrdSanity"
   This runs a little program which performs a sanity-check on the
   nrrd library; specifically, all the assumptions about type sizes,
   endianness, and such that are set at compile time.  If it doesn't
   start by saying "nrrd sanity check passed", then email me; there
   are serious problems.

=============== Directory Structure

src/
  With one subdirectory for each of the teem libraries, all the 
  source for the libraries is in here.  There is also a "src/make/"
  directory which is just for makefile (.mk) files.
include/
  Some short header files that are used to verify the correct
  setting of compiler variables, such as TEEM_ENDIAN.
include/teem/
  The include (.h) files for all the libraries (such as nrrd.h)
  get put here (but don't originate from here).  
irix6.64/
irix6.n32/
linux.32/
linux.64/
solaris/
cygwin/
darwin.32/
darwin.64/
  The architecture-dependent directories, with a name which exactly
  matches valid settings for the environment variable TEEM_ARCH.
  Within these directories there are:
    lib/  all libraries put both their static/archive (.a) and 
          shared/dynamic (.so) library files here (such as libnrrd.a)
    bin/  all libraries put their binaries here, hopefully in a way which
          doesn't cause name clashes
    obj/  make puts all the .o files in here, for all libraries. When
          compiling "dev", it also puts libraries here, so that "tests"
          can link against them there

See the README.txt in the "src" directory for library-specific
information.

=============== Code aesthetics

There are some matters of coding style which I try to hold myself too.
They are listed here primarily for my benefit, but I would hope that 
other people follow them as well if they contribute to teem.

aspects of GNU style (http://www.gnu.org/prep/standards.html) which I like:
(but don't necessarily always follow)
- avoid arbitrary limits on (memory) sizes of things (this is very hard)
- be robust about handling of non-ASCII input where ASCII is expected
- be super careful about handling of erroneous system call return,
  and always err on the side of being anal in matters of error detection
  and reporting.
- check every single malloc/calloc for NULL return
- make sure all symbols visible in the library
  start with "<lib>" or "_<lib>" where <lib> is the library name
- for expressions split on multiple lines, split before an operator, not after
- use parens on multi-line expressions to make them tidy
- Function comments should be in complete sentences, with two spaces after "."
- Try to avoid assignments inside if-conditional.

other:
- make sure that error detection code is as seperate as possible
  from code which gets something done (a sort of preamble of asserts)
- Spell-check all comments.
- No constants embedding in code- either use #defines or enums.
- to distinguish between a continuous (float) variable and one (int) which
  represents the same quantity, but discretized, suffix the int with "i" or
  "Idx":  "u" vs. "uIdx" or "u" vs. "ui"
- Try to avoid making local copies of variables in structs, just for
  the sake of code brevity when those variables won't change during
  the scope of the function.
- use "return" correctly: no parens!
- don't give biff carriage returns
- always use "biff" for error handling stuff
- if a pointer should be initialized to NULL, then set it to NULL;
  Don't assume that a pointer in a struct will be NULL following a calloc.
