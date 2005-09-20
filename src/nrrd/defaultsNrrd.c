/*
  Teem: Tools to process and visualize scientific data and images
  Copyright (C) 2005  Gordon Kindlmann
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
** a constructor to nrrdDefaultResamplePadValue, but the user can also set it
** explicitly.
*/

const NrrdEncoding *nrrdDefaultWriteEncoding = &_nrrdEncodingRaw;
int nrrdDefaultWriteBareText = AIR_TRUE;
unsigned int nrrdDefaultWriteCharsPerLine = 75;
unsigned int nrrdDefaultWriteValsPerLine = 8;
/* ---- BEGIN non-NrrdIO */
int nrrdDefaultResampleBoundary = nrrdBoundaryBleed;
int nrrdDefaultResampleType = nrrdTypeDefault;
int nrrdDefaultResampleRenormalize = AIR_TRUE;
int nrrdDefaultResampleRound = AIR_TRUE;
int nrrdDefaultResampleClamp = AIR_TRUE;
int nrrdDefaultResampleCheap = AIR_FALSE;
double nrrdDefaultResamplePadValue = 0.0;
double nrrdDefaultKernelParm0 = 1.0; 
/* ---- END non-NrrdIO */
int nrrdDefaultCenter = nrrdCenterCell;
double nrrdDefaultSpacing = 1.0;

/* these aren't really "defaults" because there's no other channel for
   specifying this information.  It is just global state.  Obviously,
   like defaults, they are not thread-safe if different threads ever
   set them differently. */
int nrrdStateVerboseIO = 1; /* NrrdIO-hack-003 */
int nrrdStateKeyValuePairsPropagate = AIR_FALSE;
/* ---- BEGIN non-NrrdIO */
int nrrdStateBlind8BitRange = AIR_TRUE;
int nrrdStateMeasureType = nrrdTypeFloat;
int nrrdStateMeasureModeBins = 1024;
int nrrdStateMeasureHistoType = nrrdTypeFloat;
int nrrdStateDisallowIntegerNonExist = AIR_TRUE;
/* ---- END non-NrrdIO */
int nrrdStateAlwaysSetContent = AIR_TRUE;
int nrrdStateDisableContent = AIR_FALSE;
char *nrrdStateUnknownContent = NRRD_UNKNOWN;
int nrrdStateGrayscaleImage3D = AIR_FALSE;
/* there is no sane reason to change this initialization */
int nrrdStateKeyValueReturnInternalPointers = AIR_FALSE;
/* Making the default for this be AIR_TRUE means that nrrd is not only
   completely conservative about updating kind, but purposely stupid.
   Nrrd is only going to implement the most converative kind of logic
   anyway, based on existing sementics nailed down by the format spec. */
int nrrdStateKindNoop = AIR_FALSE;

/* should the acceptance (or not) of malformed NRRD header fields 
   embedded in PNM or text comments be controlled here? */

/* Are there other assumptions currently built into nrrd which could
   stand to be user-controllable? */

/* ---- BEGIN non-NrrdIO */
void
nrrdDefaultGetenv(void) {
  char *envS;
  int valI;
  unsigned int valUI;
  double valD;
  
  /* these two pre-date Def --> Default rename; 
     old "_DEF_" string is recognized */
  if (((envS = getenv("NRRD_DEF_WRITE_BARE_TEXT"))
       || (envS = getenv("NRRD_DEFAULT_WRITE_BARE_TEXT")))
      && (valI = airEnumVal(airBool, envS))) {
    nrrdDefaultWriteBareText = valI;
  }
  if (((envS = getenv("NRRD_DEF_CENTER"))
       || (envS = getenv("NRRD_DEFAULT_CENTER")))
      && (valI = airEnumVal(nrrdCenter, envS))) {
    nrrdDefaultCenter = valI;
  }

  /* the introduction of these post-date the Def --> Default rename */
  if ((envS = getenv("NRRD_DEFAULT_WRITE_CHARS_PER_LINE"))
      && (1 == sscanf(envS, "%u", &valUI))) {
    nrrdDefaultWriteCharsPerLine = valUI;
  }
  if ((envS = getenv("NRRD_DEFAULT_WRITE_VALS_PER_LINE"))
      && (1 == sscanf(envS, "%u", &valUI))) {
    nrrdDefaultWriteValsPerLine = valUI;
  }
  if ((envS = getenv("NRRD_DEFAULT_RESAMPLE_BOUNDARY"))
      && (valI = airEnumVal(nrrdBoundary, envS))) {
    nrrdDefaultResampleBoundary = valI;
  }
  if ((envS = getenv("NRRD_DEFAULT_RESAMPLE_TYPE"))
      && (valI = airEnumVal(nrrdType, envS))) {
    nrrdDefaultResampleType = valI;
  }
  if ((envS = getenv("NRRD_DEFAULT_RESAMPLE_RENORMALIZE"))
      && (-1 != (valI = airEnumVal(airBool, envS)))) {
    nrrdDefaultResampleRenormalize = valI;
  }
  if ((envS = getenv("NRRD_DEFAULT_RESAMPLE_ROUND"))
      && (-1 != (valI = airEnumVal(airBool, envS)))) {
    nrrdDefaultResampleRound = valI;
  }
  if ((envS = getenv("NRRD_DEFAULT_RESAMPLE_CLAMP"))
      && (-1 != (valI = airEnumVal(airBool, envS)))) {
    nrrdDefaultResampleClamp = valI;
  }
  if ((envS = getenv("NRRD_DEFAULT_RESAMPLE_CHEAP"))
      && (-1 != (valI = airEnumVal(airBool, envS)))) {
    nrrdDefaultResampleCheap = valI;
  }
  if ((envS = getenv("NRRD_DEFAULT_RESAMPLE_PAD_VALUE"))
      && (1 == sscanf(envS, "%lf", &valD))) {
    nrrdDefaultResamplePadValue = valD;
  }
  if ((envS = getenv("NRRD_DEFAULT_KERNEL_PARM0"))
      && (1 == sscanf(envS, "%lf", &valD))) {
    nrrdDefaultKernelParm0 = valD;
  }
  if ((envS = getenv("NRRD_DEFAULT_SPACING"))
      && (1 == sscanf(envS, "%lf", &valD))) {
    nrrdDefaultSpacing = valD;
  }

  return;
}

void
nrrdStateGetenv(void) {
  char *envS;
  int valI;
  
  if ((envS = getenv("NRRD_STATE_KIND_NOOP"))
      && (-1 != (valI = airEnumVal(airBool, envS)))) {
    nrrdStateKindNoop = valI;
  }
  if ((envS = getenv("NRRD_STATE_VERBOSE_IO"))
      && (1 == sscanf(envS, "%d", &valI))) {
    nrrdStateVerboseIO = valI;
  }
  if ((envS = getenv("NRRD_STATE_KEYVALUEPAIRS_PROPAGATE"))
      && (-1 != (valI = airEnumVal(airBool, envS)))) {
    nrrdStateKeyValuePairsPropagate = valI;
  }
  if ((envS = getenv("NRRD_STATE_BLIND_8_BIT_RANGE"))
      && (-1 != (valI = airEnumVal(airBool, envS)))) {
    nrrdStateBlind8BitRange = valI;
  }
  if ((envS = getenv("NRRD_STATE_ALWAYS_SET_CONTENT"))
      && (-1 != (valI = airEnumVal(airBool, envS)))) {
    nrrdStateAlwaysSetContent = valI;
  }
  if ((envS = getenv("NRRD_STATE_DISABLE_CONTENT"))
      && (-1 != (valI = airEnumVal(airBool, envS)))) {
    nrrdStateDisableContent = valI;
  }
  if ((envS = getenv("NRRD_STATE_MEASURE_TYPE"))
      && (nrrdTypeUnknown != (valI = airEnumVal(nrrdType, envS)))) {
    nrrdStateMeasureType = valI;
  }
  if ((envS = getenv("NRRD_STATE_MEASURE_MODE_BINS"))
      && (1 == sscanf(envS, "%d", &valI))) {
    nrrdStateMeasureModeBins = valI;
  }
  if ((envS = getenv("NRRD_STATE_MEASURE_HISTO_TYPE"))
      && (nrrdTypeUnknown != (valI = airEnumVal(nrrdType, envS)))) {
    nrrdStateMeasureHistoType = valI;
  }
  return;
}

/* ---- END non-NrrdIO */
