=============== 

  teem: Gordon Kindlmann's research software
  Copyright (C) 2002, 2001, 2000, 1999, 1998 University of Utah

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

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
"LICENSE.txt" in the same directory as this file.  See the end of this
file for my understanding of exactly what the LGPL means for people
wishing to use any of the teem libraries in their own programs.

=============== How to compile

1: set the environment variable TEEM_ARCH to one of the following:
   "irix6.n32": for irix in n32 mode
   "irix6.64": for irix in 64-bit mode
   "linux": for linux boxes
   "solaris": for solaris boxes
   "cygwin": if you have cygwin (www.cygwin.com) on a windows box

2: "cd src"

3: "make" 
   Or more accurately, "gmake", or "/usr/local/gnu/bin/make", or
   however it is that you invoke a GNU make.  This MUST be GNU make.
   I am in fact using features unique to GNU make.

4: "../${TEEM_ARCH}/bin/nrrdSanity"
   This runs a little program which performs a sanity-check on the
   nrrd library; specifically, all the assumptions about type sizes,
   endienness, and such that are set at compile time.  If it doesn't
   say "nrrd sanity check passed", then email me; there are serious
   problems.

If you want only the nrrd library and the related utilities (unrrdu)
then type: "make just-nrrd".

What?  No configure or auto-conf script?  That's right.  Because the
architecture specific stuff is all set with a file in the "src/make"
directory, and because this never "installs" anything to a location
outside this directory tree, and because this is nearly all very
vanilla ANSI C, I don't consider those tools necessary.

However, you may need to alter the appropriate architecture-specific
".mk" file in the "src/make" directory.  This is unlikely.  If you
feel there is a bug in those files, please email me at
gk_AT_cs.utah.edu

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

Much of teem would be better re-written in C++.  But not the majority
of it.  C is a small language, and a simple language, and I know
exactly what I'm doing with it.  Lots of teem is either so simple
(like bane) or so low-level-ish (like nrrd), that the benefits of
using a much more powerful language like C++ do not outweigh the
benefits of me writing the software I need, now, in a way that I
understand, now, so that I can graduate, soon.  C++ has its time and
place.  In later times, I may find myself in those places.

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

---> After you have untarred the teem tree, the ONLY directory which
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

=============== Gordon's understanding of the LGPL

I'm using the space here to record my understanding of the LGPL and
its consequences, after a careful reading of it.  This is as much for
my own good as for others interested in using my libraries.

The notion of a "derived work" is central to copyright law, because
copyright concerns itself not just with a work, but with anything
which is a modified version of the original work, or a translated
version of the work, or which is (or contains) a significant portion
of the work, and so on.

If you link my library with your program, the resulting binary is
derived from my library, in the copyright sense, because the binary
contains translated portions of my code.  Interestingly, the GNU folks
make no distinction between static and dynamic linking INSOFSAR AS THE
DEFINITION OF A DERIVED WORK (as applied to software) IS CONCERNED.
From the Preamble of the LGPL: "When a program is linked with a
library, whether statically or using a shared library, the combination
of the two is legally speaking a combined work, a derivative of the
oringal library."

The reason that the LGPL is a longer and more complicated document
than the GPL is that the LGPL makes a distinction between different
kinds of derived works that the GPL does not make.  Section 2(b) of
the GPL kicks in for ANY derived work that you distribute or publish,
requiring that the derived work be licensed under the GPL.  Read the
GPL for exactly what that entails; it involves distribution of your
program's source code.  The LGPL also says that it applies to all
derived works, but in a way that depends on what the derived work is:

1) If the derived work involves a modification of the source of my
library, then Section 2(c) of the LGPL kicks in as soon as you
distribute the derived work, requiring that the derived
work be licensed under the LGPL.

2) If the derived work is a binary created by linking against my
library, then Section 6 of the LGPL kicks in, saying that the derived
work can be distributed "under the terms of your choice, provided that
...", and goes on to describe the responsibilities of someone
distributing the derived work.

The point of all this is that those responsibilities, which I'll
describe below, amount to something LESS than the full LGPL.  Those
responsibilities are mandated and defined by the LGPL (since the LGPL
governs the distribution of any derived work), but they are not the
same as the LGPL, in contrast to what is required when the derived
work involves modification of my source.

So, if you distribute a program that links against teem, without
modifying teem, (I believe) Section 6 of the LGPL requires you to do
four things:

1) Give "prominant notice" that your program uses teem, and that teem
is covered by LGPL.  I'm not sure, but I take this to mean that you
have to mention teem and LGPL somewhere relatively soon in any
high-level description of your program which occurs in a central
README or similar documentation.

2) Supply a copy of the LGPL.  That's the LICENSE.txt file in the
same directory as this file.

3) If your program displays any copyright notices, then you need to
include teem's copyright notice as well, which (I belive) is simply:
  Copyright (C) 2002, 2001, 2000, 1999, 1998 University of Utah

4) This is the interesting one.  You need to make it possible for
users of your program to modify my teem libraries, and use those
modified teem libraries in your program.  The principle of copyleft is
that you pass on to others the same freedoms as were available to you.
The five possible ways to facilitate this, listed in Section 6, can (I
believe) be summarized as two alternatives:

  A) If you statically link against teem libraries, you need to
  accompany the distribution of your program with:
  - all source files for the required teem libraries, and
  - all the OBJECT files (or source files) for YOUR program.
  This way someone could re-link your various object files together
  with modified teem libraries to produce their own version of your
  program.  You don't have to release any source for your program if
  you don't want to.  (This much is from Section 6(a).)  If you don't
  supply these materials alongside the distribution of your program,
  then they need to be conveyed or made available by some other means.
  (This is Sections 6(c)-6(e))

  B) If you only dynamically link against teem shared libraries,
  you're done!  The nature of shared libraries means that your program
  will slurp up the teem libraries (original or modified) at run-time.
  Section 6(b) doesn't even mention that you need to supply the source
  for my teem libraries, but it would probably be a good idea.  (This
  is Section 6(b); perhaps something else I missed mandates teem
  source redistribution.)

I mentioned above that the LGPL does not distinguish between static
and dynamic linking insofar as the definition of a derivative work is
concerned.  Section 6 of LGPL says that the distinction between static
and dynamic DOES matter in the context of how to comply with LGPL when
you're releasing a program that uses teem without modifying teem.  And
this is how I imagine most everyone will use teem-- just linking
against teem as I've distributed it.  It seems to me that the linking
had better be with the shared library versions of teem, since it will
probably be easier than taking the route described in 4(A) above.  Of
course, you should read the license itself in order to make sure of
all these details.

I feel that the four points of compliance, described above, are
entirely fair and easy to deal with, especially if you take the shared
library route.  An explanation of why I chose a copyleft (GNU) license
instead of non-copyleft (X11, "BSD-style") will wait for another time.
