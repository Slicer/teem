/*
  Teem: Tools to process and visualize scientific data and images              
  Copyright (C) 2008, 2007, 2006, 2005  Gordon Kindlmann
  Copyright (C) 2004, 2003, 2002, 2001, 2000, 1999, 1998  University of Utah

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public License
  (LGPL) as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  The terms of redistributing and/or modifying this software also
  include exceptions to the LGPL that facilitate static linking.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with this library; if not, write to Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef MEET_HAS_BEEN_INCLUDED
#define MEET_HAS_BEEN_INCLUDED

#include <stdio.h>
#include <string.h>

#include <teem/air.h>
#include <teem/hest.h>
#include <teem/biff.h>
#include <teem/nrrd.h>
#include <teem/ell.h>
#include <teem/unrrdu.h>
#include <teem/dye.h>
#if defined(TEEM_BUILD_EXPERIMENTAL_LIBS)
#include <teem/moss.h>
#include <teem/alan.h>
#endif
#include <teem/gage.h>
#if defined(TEEM_BUILD_EXPERIMENTAL_LIBS)
#include <teem/bane.h>
#endif
#include <teem/limn.h>
#include <teem/seek.h>
#include <teem/hoover.h>
#include <teem/echo.h>
#include <teem/ten.h>
#include <teem/mite.h>
#if defined(TEEM_BUILD_EXPERIMENTAL_LIBS)
#include <teem/coil.h>
#include <teem/push.h>
#include <teem/pull.h>
#endif

#if defined(_WIN32) && !defined(__CYGWIN__) && !defined(TEEM_STATIC)
#  if defined(TEEM_BUILD) || defined(meet_EXPORTS) || defined(teem_EXPORTS)
#    define MEET_EXPORT extern __declspec(dllexport)
#  else
#    define MEET_EXPORT extern __declspec(dllimport)
#  endif
#else /* TEEM_STATIC || UNIX */
#  define MEET_EXPORT extern
#endif


#ifdef __cplusplus
extern "C" {
#endif

#define MEET meetBiffKey

/* enumall.c */
MEET_EXPORT const char *meetBiffKey;
MEET_EXPORT const airEnum **meetAirEnumAll();
MEET_EXPORT void meetAirEnumAllPrint(FILE *file);

#ifdef __cplusplus
}
#endif

#endif /* MEET_HAS_BEEN_INCLUDED */
