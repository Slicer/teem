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
   (cygwin support is not currently complete)

2: cd src

3: make (actually, gmake: this MUST be GNU make)

=============== General Info

These libraries and utilities are written by Gordon Kindlmann in
support of his research.  Other people have also found them useful.

I'm using CVS so that I have more discipline about how I write my own
code, and also to facilitate other people getting it and contributing
bug fixes.

The software in this CVS tree is stable and well-tested.  There are
plenty of libraries I'm working on which are either completely in
flux, or which are so embryonic, that are not in a distributable
state, and are thus not in the CVS tree.  New wrappers or extensions
to my software can be added after I've made sure that they are
consistent with the design ideas I've had in mind for my software.  I
want to encourage the usage and extension of my code, but as long as
this CVS tree is for the software I consider "mine", then new
libraries and directories will be added here at my discretion.

=============== Directory Structure

src/
  With one subdirectory for each of the teem libraries, all the 
  source for the libraries is in here.  There is also a "src/make/"
  directory which is just for makefile (.mk) files.
include/
  the include (.h) files for all the libraries (such as nrrd.h)
irix6.64/
irix6.n32/
linux/
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
          be compiled as position-independent code.

Upon getting a copy of the CVS tree, the only directory which should
contain anything is the "src" directory- the compiles of the libraries
will put files as needed in all the other directories.

See the README.txt in the "src" directory for library-specific information.

=============== Code aesthetics

There are some matters of coding style which I try to hold myself too.
They are listed here primarily for my benefit, but I would hope that 
other people follow them as well.

aspects of GNU style (http://www.gnu.org/prep/standards.html) which I like:
- avoid arbitrary limits on (memory) sizes of things
- be robust about handling of non-ASCII input where ASCII is expected
- be super careful about handling of erroneous system call return,
  and always err on the side of being anal in matters of error detection
  and reporting.
- check every single malloc, calloc for NULL return
- make sure all symbols visible in the library
  start with "<lib>" or "_<lib>"; <lib> = library name
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
- migrate general utility things from nrrd to air
- always use "biff" for error handling stuff
- if a pointer should be initialized to NULL, then set it to NULL;
  Don't assume that a pointer in a struct will be NULL following a calloc.
