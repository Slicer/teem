The "teem" source tree
"To one day be a powerful swarm of useful libraries and utilities"

=============== 

  The contents of this file are subject to the University of Utah Public
  License (the "License"); you may not use this file except in
  compliance with the License.
  
  Software distributed under the License is distributed on an "AS IS"
  basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See
  the License for the specific language governing rights and limitations
  under the License.

  The Original Source Code is "teem", released March 23, 2001.
  
  The Original Source Code was developed by the University of Utah.
  Portions created by UNIVERSITY are Copyright (C) 2001, 1998 University
  of Utah. All Rights Reserved.

=============== License information

See above.  This preamble appears on all .c, .h, and .mk files. Full
text of the License should be in the file "LICENSE.txt" in the same
directory as this file.

=============== How to compile

1: set the environment variable TEEM_ARCH to one of the following:
   "irix6.n32": for irix in n32 mode
   "irix6.64": for irix in 64-bit mode
   "linux": for linux boxes
   "solarix": for solarix boxes
   (cygwin and hpux support are in the works)

2: "cd src"

3: "make" 
   Or more accurately, "gmake", or "/usr/local/gnu/bin/make".
   This MUST be GNU make.  I am in fact using features unique to GNU make.

What?  No configure or auto-conf script?  That's right.  Because the
architecture specific stuff is all set with a file in the "src/make"
directory, and because this never "installs" anything to a location
outside this directory tree, and because this is all very vanilla ANSI
C, I don't consider those tools necessary.

However, you may need to alter the appropriate architecture-specific
".mk" file in the "src/make" directory.  If you feel there is a bug
in those files, please email me at gk_AT_cs.utah.edu

=============== General Info

These libraries and utilities are written by Gordon Kindlmann in
support of his research.  Other people have also found them useful.

I'm using CVS so that I have more discipline about how I write my own
code, and also to facilitate other people getting it and contributing
bug fixes.

I consider the software in this CVS tree to be relatively stable and
well-tested, unlike the many other libraries which I'm currently
working on.  While I am open to the idea of adding new wrappers,
extensions, or libraries to teem, please keep in mind that they will
undergo my scrutiny before they will be added to the official CVS
tree.

I'm not especially averse to C++, I just think its a bit of a
disaster.  Bill Joy agrees with me on this one.  As do other people I
respect.  C++ has its time and place, no doubt.  I'm just generally not
in those times or places.  My loss, I'm sure.

=============== Directory Structure

src/
  With one subdirectory for each of the teem libraries, all the 
  source for the libraries is in here.  There is also a "src/make/"
  directory which is just for makefile (.mk) files.
include/
  the include (.h) files for all the libraries (such as nrrd.h)
  get put here (but don't originate from here)
    include/teem
      include/teem/need: header files which help verifying that 
      certain compiler variables are set
irix6.64/
irix6.n32/
linux/
solaris/
cygwin/
  The architecture-dependent directories, with a name which exactly
  matches valid settings for the environment variable TEEM_ARCH.
  Within these directories there are:
    lib/  all libraries put both their static/archive (.a) and 
          shared/dynamic (.so) library files here (such as libnrrd.a)
    bin/  all libraries put their binaries here, hopefully in a way which
          doesn't cause name clashes
    obj/  this has further subdirectories for each library, into which 
          "make" puts all the .o files.  The object files in these
          directories (such as "obj/nrrd") are assumed to be suitable
          for making both .a archive libraries as well as .so (or such)
          shared libraries.  This may mean that all object files will
          be compiled as position-independent code, even if that incurs
          a slight performance penalty.

---> Upon getting a copy of the CVS tree, the ONLY directory which
---> should contain anything is the "src" directory (except for "teem"
---> in include)- the compilations of the various libraries will put
---> header, library, and object files as needed in all the
---> appropriate directories.

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
  start with "<lib>" or "_<lib>" where <lib> is the library name or
  some obvious but non-trivial shortening of it
- for expressions split on multiple lines, split before an operator, not after
- use parens on multi-line expressions to make them tidy
- Function comments should be in complete sentences, with two spaces after "."
- Try to avoid assignments inside if-conditional.

other:
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
