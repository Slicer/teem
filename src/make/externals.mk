#
# teem: Gordon Kindlmann's research software
# Copyright (C) 2002, 2001, 2000, 1999, 1998 University of Utah
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
#

## EXTERNALS: list of all the identifiers for the various external
## libraries that we can _optionally_ link against.  Teem has no
## notion of depending on these in the makefile sense, and doesn't try
## to represent inter-external dependencies (PNG on zlib) with any
## generality.
##
## Extern EXT is enabled during make by setting the environment variable
## TEEM_EXT (just set it, not to anything in particular).
## If external EXT is enabled during make, then TEEM_EXT will be
## defined during source file compilation.
##
EXTERNALS = ZLIB

## ZLIB: for the zlib library underlying gzip and the PNG image
## format.  Using zlib enables the "gzip" nrrd data encoding.  Header
## file: <zlib.h>
TEEM_ZLIB.IPATH =
TEEM_ZLIB.LPATH =
ZLIB.LINK = -lz
nrrd.EXT = ZLIB
