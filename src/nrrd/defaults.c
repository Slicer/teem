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

int nrrdDefWrtFormat = nrrdFormatNRRD;
int nrrdDefWrtEncoding = nrrdEncodingRaw;
int nrrdDefWrtSeperateHeader = AIR_FALSE;
int nrrdDefWrtBareTable = AIR_TRUE;
int nrrdDefRsmpBoundary = nrrdBoundaryBleed;
int nrrdDefRsmpType = nrrdTypeUnknown;  /* means "same as input" */
double nrrdDefRsmpScale = 1.0;
int nrrdDefRsmpRenormalize = AIR_TRUE;
double nrrdDefRsmpPadValue = 0.0;
int nrrdDefCenter = nrrdCenterNode;
int nrrdDefIOCharsPerLine = 73;
int nrrdDefIOValsPerLine = 6;

/* should the behavior of nrrdMinMaxFind() on 8-bit data be
   controlled here?  Are there other assumptions currently
   built into nrrd which could stand to be user-controllable? */
