/*
  teem: Gordon Kindlmann's research software
  Copyright (C) 2004, 2003, 2002, 2001, 2000, 1999, 1998 University of Utah

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
*/

#include <teemEndian.h>
#include <teemQnanhibit.h>
#include <teemBigbitfield.h>

/*
** _airFloat, _airDouble
**
** these unions facilitate converting amonst
** i: unsigned integral type
** c: (sign,exp,frac) triples of unsigned integral components
** v: the floating point numbers these bit-patterns represent
*/
typedef union {
  unsigned int i;
  struct {
#if TEEM_ENDIAN == 1234
    unsigned int frac : 23;
    unsigned int exp : 8;
    unsigned int sign : 1;
#else
    unsigned int sign : 1;
    unsigned int exp : 8;
    unsigned int frac : 23;
#endif
  } c;
  float v;
} _airFloat;

typedef union {
  airULLong i;
#if TEEM_BIGBITFIELD == 1
  /* #ifndef __sparc */
  struct {
# if TEEM_ENDIAN == 1234
    airULLong frac : 52;
    unsigned int exp : 11;
    unsigned int sign : 1;
# else
    unsigned int sign : 1;
    unsigned int exp : 11;
    airULLong frac : 52;
# endif
  } c;
#endif
  /* these next two members are used for printing in airFPFprintf_d */
  struct { /* access to whole double as two unsigned ints */
#if TEEM_ENDIAN == 1234
    unsigned int half0 : 32;
    unsigned int half1 : 32;
#else
    unsigned int half1 : 32;
    unsigned int half0 : 32;
#endif
  } h;
  struct { /* access to fraction with two unsigned ints */
#if TEEM_ENDIAN == 1234
    unsigned int frac0 : 32;
    unsigned int frac1 : 20;
    unsigned int exp : 11;
    unsigned int sign : 1;
#else
    unsigned int sign : 1;
    unsigned int exp : 11;
    unsigned int frac1 : 20;
    unsigned int frac0 : 32;
#endif
  } c2;
  double v;
} _airDouble;

