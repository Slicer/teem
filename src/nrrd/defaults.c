/*
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
*/


#include "nrrd.h"
#include "private.h"

/*
** these aren't "const" ints because the user should be able to change
** default behavior- until a more sophisticated mechanism for this
** kind of control is developed, it seems simple and usable enough to
** have this be global state which we agree to treat nicely, as in,
** threads shouldn't be changing these willy-nilly.  
**
** What IS a "default"?  A default is the assertion of a certain
** choice in situations where the user hasn't set it explicitly, but
** COULD, using some other mechanism.  The pad value in resampling is
** a good example: it is set by a constructor to nrrdDefRsmpPadValue,
** but the user can set it explicitly.
*/

int nrrdDefWrtEncoding = nrrdEncodingRaw;
int nrrdDefWrtSeperateHeader = AIR_FALSE;
int nrrdDefWrtBareTable = AIR_FALSE;
int nrrdDefWrtCharsPerLine = 75;
int nrrdDefWrtValsPerLine = 8;
int nrrdDefRsmpBoundary = nrrdBoundaryBleed;
int nrrdDefRsmpType = nrrdTypeUnknown;  /* means "same as input" */
double nrrdDefRsmpScale = 1.0;
int nrrdDefRsmpRenormalize = AIR_TRUE;
double nrrdDefRsmpPadValue = 0.0;
int nrrdDefCenter = nrrdCenterNode;

/* these aren't really "defaults" because there's no other channel for
   specifying this information.  It is just global state.  Obviously,
   they are not thread-safe if different threads ever set them
   differently. */
int nrrdStateMeasureType = nrrdTypeFloat;
int nrrdStateMeasureHistoType = nrrdTypeFloat;
int nrrdStateVerboseIO = 4;

/* these are just plain hacks.  They are not threadsafe.  They
   are not documented.  They are also the hallmark of a feeble mind. */
int nrrdHackHasNonExist = AIR_FALSE;

/* should the behavior of nrrdMinMaxFind() on 8-bit data be controlled
   here? */

/* should the acceptance (or not) of malformed NRRD header fields 
   embedded in PNM or table comments be controlled here? */

/* Are there other assumptions currently built into nrrd which could
   stand to be user-controllable? */
