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



Teem is a collection of libraries and programs that I use for my
research.  All of the things below are names of libraries in teem,
many libraries have some associated command-line programs useful for
debugging or accessing the functionality in the library.  Teem
libraries depend on each other in a non-circularly dependent way.
Those things listed *later* depend on a subset of the libraries listed
*earlier*: this is the *reverse* of how they would be listed on the
link ("ld") line.

air:    general utilities which everyone needs
hest:   command-line parsing
biff:   general error reporting utility
ell:    assorted linear algebra stuff
nrrd:	nearly raw raster data
dye:    color conversions (and eventually color maps)
gage:   for measuring things in volumes (currently only scalar volumes)
        (includes "qbert" for generating VGH volumes for simian)
bane:   GK's Master's thesis re-reimplementation
unrrdu:	"unu": powerful command-line interface to nrrd
limn:   graphics related stuff and/or postscript drawing
hoover: multi-threaded volume rendering framework
mite:   "miter": simple volume renderer of scalar fields
ten:    diffusion tensor related functionality
echo:   dumb ray-tracer, pay no attention

Here is a full dependency matrix of the teem libraries/binaries: Use
this to determine exactly which libraries are needed on the link line.

Key:
 L : lib to left uses functions in lib above
 i : lib to left uses only declarations or macros from headers of lib above
(L): lib to left needs lib above indirectly
(i): lib to left indirectly uses only declarations or macros
[L]: lib to left needs lib above indirectly, otherwise only needs headers

Each horizonatal line has some kind of "L" for each library it needs
to link against, which determines what appears on the link line.

                        u              h   
                        n              o   
            h  b     n  r     g  b  l  o  m     e
         a  e  i  e  r  r  d  a  a  i  v  i  t  c
         i  s  f  l  r  d  y  g  n  m  e  t  e  h
         r  t  f  l  d  u  e  e  e  n  r  e  n  o  
         |  |  |  |  |  |  |  |  |  |  |  |  |  |
air    : -
hest   : L  -
biff   : L     -
ell    : L        -
nrrd   : L  i  L     -
unrrdu : L  L  L     L  -
dye    : L     L  i        -
gage   : L (i) L  L  L        -
bane   : L (i) L (L) L        L  -
limn   : L  L  L  L  L              -
hoover : L (L) L [L](L)             L  -
mite   : L [L] L [L] L        L     L  L  -
ten    : L [L] L  L  L     L        L        -  
echo   : L [L] L  L  L              L           -
