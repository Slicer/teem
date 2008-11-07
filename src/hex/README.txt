/*
  Copyright (C) 2004, 2003, 2002 University of Utah

  This software,  is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

-----------
enhex/dehex
-----------

enhex and dehex are a stand-alone hex encoder/decoder pair.
They convert between data in raw form (paying no regard to the
endianness), and convert it to hexadecimal form.  

To compile:

  cc -o enhex enhex.c
  cc -o dehex dehex.c

I wrote these as freely distributable (non-copyleft) programs for
doing the hex encoding which is an optional encoding in the nrrd
library in Teem.  The (convoluted) reason is that the only encodings
that are *required* of non-Teem nrrd readers and writers are raw and
ascii.  All other encodings should be able to be handled by
stand-alone tools (such as gzip/gunzip for zlib compression, and
bzip/bunzip2 for bzip compress).  Yet, a google search didn't reveal
simple tools for raw--hex conversion, so I wrote them.  Nothing fancy.

Gordon Kindlmann
