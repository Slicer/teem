/*
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
*/

#include "nrrd.h"
#include "privateNrrd.h"

/*
** these aren't "const"s because the user should be able to change
** default behavior- until a more sophisticated mechanism for this
** kind of control is developed, it seems simple and usable enough to
** have this be global state which we agree to treat nicely, as in,
** threads shouldn't be changing these willy-nilly.
**
** What IS a "default"?  A default is the assertion of a certain
** choice in situations where the user hasn't set it explicitly, but
** COULD.  The pad value in resampling is a good example: it is set by
** a constructor to nrrdDefRsmpPadValue, but the user can also set it
** explicitly.
*/

int nrrdDefWrtEncoding = nrrdEncodingRaw;
int nrrdDefWrtSeperateHeader = AIR_FALSE;
int nrrdDefWrtBareTable = AIR_TRUE;
int nrrdDefWrtCharsPerLine = 75;
int nrrdDefWrtValsPerLine = 8;
int nrrdDefRsmpBoundary = nrrdBoundaryBleed;
int nrrdDefRsmpType = nrrdTypeUnknown;  /* sometimes means "same as input" */
double nrrdDefRsmpScale = 1.0;    /* these two should probably be the same */
double nrrdDefKernelParm0 = 1.0; 
int nrrdDefRsmpRenormalize = AIR_TRUE;
int nrrdDefRsmpClamp = AIR_TRUE;
double nrrdDefRsmpPadValue = 0.0;
int nrrdDefCenter = nrrdCenterNode;
double nrrdDefSpacing = 1.0;

/* these aren't really "defaults" because there's no other channel for
   specifying this information.  It is just global state.  Obviously,
   like defaults, they are not thread-safe if different threads ever
   set them differently. */
int nrrdStateVerboseIO = 1;
int nrrdStateClever8BitMinMax = AIR_TRUE;
int nrrdStateMeasureType = nrrdTypeFloat;
int nrrdStateMeasureModeBins = 1024;
int nrrdStateMeasureHistoType = nrrdTypeFloat;
int nrrdStateAlwaysSetContent = AIR_TRUE;
char nrrdStateUnknownContent[AIR_STRLEN_SMALL] = NRRD_UNKNOWN;
int nrrdStateDisallowFixedPointNonExist = AIR_TRUE;

/* should the acceptance (or not) of malformed NRRD header fields 
   embedded in PNM or table comments be controlled here? */

/* Are there other assumptions currently built into nrrd which could
   stand to be user-controllable? */
