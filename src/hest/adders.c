/*
  Teem: Tools to process and visualize scientific data and images
  Copyright (C) 2009--2023  University of Chicago
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

  You should have received a copy of the GNU Lesser General Public License along with
  this library; if not, write to Free Software Foundation, Inc., 51 Franklin Street,
  Fifth Floor, Boston, MA 02110-1301 USA
*/

#include "hest.h"
#include "privateHest.h"

unsigned int
hestOptAddFlag(hestOpt **optP, const char *flag, int *valueP, const char *info) {

  return hestOptAdd_nva(optP, flag, NULL /* name */, airTypeInt /* actually moot */,
                        0 /* min */, 0 /* max */, valueP, NULL /* default */, info, NULL,
                        NULL, NULL);
}

/*
hestOptSetXX(hestOpt *opt, )
1<T>
1v<T>
2<T>
3<T>
4<T>
N<T>
Nv<T>  need sawP

<T>=
Bool,
Int,
UInt,
LongInt,
ULongInt,
Size_t,
Float,
Double,
Char,
String,
Enum,  need Enum
Other,  need CB
*/