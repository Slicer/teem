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


#include "ten.h"

tenGlyphParm *
tenGlyphParmNew() {
  tenGlyphParm *parm;

  parm = calloc(1, sizeof(tenGlyphParm));
  if (parm) {
    parm->anisoThresh = parm->anisoType = AIR_NAN;
    parm->vThreshVol = NULL;
    parm->dwiThresh = parm->vThresh = parm->useColor = AIR_NAN;
    parm->thresh = parm->cscale = AIR_NAN;
  }
  return parm;
}

tenGlyphParm *
tenGlyphParmNix(tenGlyphParm *parm) {

  if (parm) {
    free(parm);
  }
  return NULL;
}
