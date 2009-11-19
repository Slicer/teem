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

#include "pull.h"
#include "privatePull.h"

const char *
_pullValStr[] = {
  "(unknown pullVal)",
  "sl",
  "slgradvec",
};

const int
_pullValVal[] = {
  pullValUnknown,
  pullValSlice,
  pullValSliceGradVec,
};

const char *
_pullValStrEqv[] = {
  "sl",
  "slgradvec", "slgv",
  ""
};

int
_pullValValEqv[] = {
  pullValSlice,
  pullValSliceGradVec, pullValSliceGradVec,
};

const airEnum
_pullVal = {
  "pullVal",
  PULL_VAL_ITEM_MAX,
  _pullValStr, _pullValVal,
  NULL,
  _pullValStrEqv, _pullValValEqv,
  AIR_FALSE
};
const airEnum *const
pullVal = &_pullVal;

/*
** HEY!! copy and paste from teem/src/mite/kindnot.c HEY!!
**
** again, this is not a true gageKind- mainly because these items may
** depend on items in different gageKinds (scalar and vector).  So,
** the prerequisites here are all blank.  Go look in pullQueryAdd()
** to see these items' true prereqs
*/
gageItemEntry
_pullValTable[PULL_VAL_ITEM_MAX+1] = {
  /* enum value        len,deriv, prereqs,  parent item, index, needData*/
  {pullValUnknown,       0,  0,   {0},              0,   0,     AIR_FALSE},
  {pullValSlice,         1,  0,   {0},              0,   0,     AIR_FALSE},
  {pullValSliceGradVec,  3,  0,   {0},              0,   0,     AIR_FALSE},
};

gageKind
_pullValGageKind = {
  AIR_FALSE,
  "pull",
  &_pullVal,
  0,
  0,
  PULL_VAL_ITEM_MAX,
  _pullValTable,
  NULL,
  NULL,
  NULL,
  NULL, NULL, NULL, NULL,
  NULL
};

gageKind *
pullValGageKind = &_pullValGageKind;
