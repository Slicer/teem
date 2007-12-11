/*
  Teem: Tools to process and visualize scientific data and images
  Copyright (C) 2006, 2005  Gordon Kindlmann
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


#include "pull.h"
#include "privatePull.h"

gageKind *
pullGageKindParse(const char *_str) {
  char me[]="pullGageKindParse", err[BIFF_STRLEN], *str;
  gageKind *ret;
  airArray *mop;
  
  if (!_str) {
    sprintf(err, "%s: got NULL pointer", me);
    return NULL;
  }
  mop = airMopNew();
  str = airStrdup(_str);
  airMopAdd(mop, str, airFree, airMopAlways);
  airToLower(str);
  if (!strcmp(gageKindScl->name, str)) {
    ret = gageKindScl;
  } else if (!strcmp(gageKindVec->name, str)) {
    ret = gageKindVec;
  } else if (!strcmp(tenGageKind->name, str)) {
    ret = tenGageKind;
  } else /* not allowing DWIs for now */ {
    sprintf(err, "%s: not \"%s\", \"%s\", or \"%s\"", me,
            gageKindScl->name, gageKindVec->name, tenGageKind->name);
    airMopError(mop); return NULL;
  }

  airMopOkay(mop);
  return ret;
}

pullVolume *
pullVolumeNew() {
  pullVolume *vol;

  vol = AIR_CAST(pullVolume *, calloc(1, sizeof(pullVolume)));
  if (vol) {
    vol->name = NULL;
    vol->kind = NULL;
    vol->ninSingle = NULL;
    vol->ninScale = NULL;
    vol->scaleNum = 0;
    vol->scaleMin = AIR_NAN;
    vol->scaleMax = AIR_NAN;
    vol->ksp00 = nrrdKernelSpecNew();
    vol->ksp11 = nrrdKernelSpecNew();
    vol->ksp22 = nrrdKernelSpecNew();
    vol->kspSS = nrrdKernelSpecNew();
    vol->gctx = gageContextNew();
    vol->gpvl = NULL;
  }
  return vol;
}

pullVolume *
pullVolumeNix(pullVolume *vol) {

  if (vol) {
    vol->name = airFree(vol->name);
    vol->ksp00 = nrrdKernelSpecNix(vol->ksp00);
    vol->ksp11 = nrrdKernelSpecNix(vol->ksp11);
    vol->ksp22 = nrrdKernelSpecNix(vol->ksp22);
    vol->kspSS = nrrdKernelSpecNix(vol->kspSS);
    vol->gctx = gageContextNix(vol->gctx);
    airFree(vol);
  }
  return NULL;
}

int
_pullVolumeSet(pullVolume *vol, char *name,
               const Nrrd *ninSingle,
               const Nrrd *const *ninScale, unsigned int ninNum,
               const gageKind *kind, 
               double scaleMin, double scaleMax,
               const NrrdKernelSpec *ksp00,
               const NrrdKernelSpec *ksp11,
               const NrrdKernelSpec *ksp22,
               const NrrdKernelSpec *kspSS) {
  char me[]="_pullVolumeSet", err[BIFF_STRLEN];
  int E;

  if (!( vol && (ninSingle || ninScale) && kind 
         && airStrlen(name) && ksp00 && ksp11 && ksp22 )) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(PULL, err); return 1;
  }
  if (ninScale && !(ninNum >= 2)) {
    sprintf(err, "%s: need at least 2 volumes (not %u)", me, ninNum);
    biffAdd(PULL, err); return 1;
  }
  if (!(vol->gctx)) {
    sprintf(err, "%s: got NULL vol->gageContext", me);
    biffAdd(PULL, err); return 1;
  }
  gageParmSet(vol->gctx, gageParmRequireAllSpacings, AIR_TRUE);
  gageParmSet(vol->gctx, gageParmRenormalize, AIR_FALSE);
  gageParmSet(vol->gctx, gageParmCheckIntegrals, AIR_TRUE);
  E = 0;
  if (!E) E |= !(vol->gpvl = gagePerVolumeNew(vol->gctx, (ninSingle
                                                          ? ninSingle
                                                          : ninScale[0]),
                                              kind));
  if (!E) E |= gageKernelSet(vol->gctx, gageKernel00,
                             ksp00->kernel, ksp00->parm);
  if (!E) E |= gageKernelSet(vol->gctx, gageKernel11,
                             ksp11->kernel, ksp11->parm); 
  if (!E) E |= gageKernelSet(vol->gctx, gageKernel22,
                             ksp22->kernel, ksp22->parm);
  if (ninScale) {
    gagePerVolume **pvlSS;
    if (!kspSS) {
      sprintf(err, "%s: got NULL kspSS", me);
      biffAdd(PULL, err); return 1;
    }
    gageParmSet(vol->gctx, gageParmStackUse, AIR_TRUE);
    gageParmSet(vol->gctx, gageParmStackRenormalize, AIR_TRUE);
    if (!E) E |= gageStackPerVolumeNew(vol->gctx, &pvlSS,
                                       ninScale, ninNum, kind);
    if (!E) E |= gageStackPerVolumeAttach(vol->gctx, vol->gpvl, pvlSS, ninNum,
                                          scaleMin, scaleMax);
    if (!E) E |= gageKernelSet(vol->gctx, gageKernelStack,
                               kspSS->kernel, kspSS->parm);
  } else {
    if (!E) E |= gagePerVolumeAttach(vol->gctx, vol->gpvl);
  }
  if (E) {
    sprintf(err, "%s: trouble", me);
    biffMove(PULL, err, GAGE); return 1;
  }

  vol->name = airStrdup(name); /* HEY: error checking? */
  vol->kind = kind;
  nrrdKernelSpecSet(vol->ksp00, ksp00->kernel, ksp00->parm);
  nrrdKernelSpecSet(vol->ksp11, ksp11->kernel, ksp11->parm);
  nrrdKernelSpecSet(vol->ksp22, ksp22->kernel, ksp22->parm);
  if (ninScale) {
    vol->ninSingle = NULL;
    vol->ninScale = ninScale;
    vol->scaleNum = ninNum;
    vol->scaleMin = scaleMin;
    vol->scaleMax = scaleMax;
    nrrdKernelSpecSet(vol->kspSS, kspSS->kernel, kspSS->parm);
  } else {
    vol->ninSingle = ninSingle;
    vol->ninScale = NULL;
    vol->scaleNum = 0;
    vol->scaleMin = AIR_NAN;
    vol->scaleMax = AIR_NAN;
    vol->kspSS = NULL;
  }
  
  gageQueryReset(vol->gctx, vol->gpvl);
  /* the query is the single thing remaining unset in the gageContext */

  return 0;
}

/*
** the effect is to give pctx ownership of the vol
*/
int
pullVolumeSingleSet(pullContext *pctx, pullVolume *vol,
                    char *name, const Nrrd *nin,
                    const gageKind *kind, 
                    const NrrdKernelSpec *ksp00,
                    const NrrdKernelSpec *ksp11,
                    const NrrdKernelSpec *ksp22) {
  char me[]="pullVolumeSingleSet", err[BIFF_STRLEN];

  if (_pullVolumeSet(vol, name, nin, NULL, 0, 
                     kind, AIR_NAN, AIR_NAN, 
                     ksp00, ksp11, ksp22, NULL)) {
    sprintf(err, "%s: trouble", me);
    biffAdd(PULL, err); return 1;
  }
  /* add this volume to context */
  pctx->vol[pctx->volNum++] = vol;
  return 0;
}

/*
** the effect is to give pctx ownership of the vol
*/
int
pullVolumeStackSet(pullContext *pctx, pullVolume *vol,
                   char *name,
                   const Nrrd *const *nin, unsigned int ninNum,
                   const gageKind *kind, 
                   double scaleMin, double scaleMax,
                   const NrrdKernelSpec *ksp00,
                   const NrrdKernelSpec *ksp11,
                   const NrrdKernelSpec *ksp22,
                   const NrrdKernelSpec *kspSS) {
  char me[]="pullVolumeStackSet", err[BIFF_STRLEN];

  if (_pullVolumeSet(vol, name, NULL, nin, ninNum,
                     kind, scaleMin, scaleMax,
                     ksp00, ksp11, ksp22, kspSS)) {
    sprintf(err, "%s: trouble", me);
    biffAdd(PULL, err); return 1;
  }
  /* add this volume to context */
  pctx->vol[pctx->volNum++] = vol;
  return 0;
}

/*
** this is only used to create pullVolumes for the pullTasks
**
** DOES use biff
*/
pullVolume *
_pullVolumeCopy(pullVolume *volOrig) {
  char me[]="pullVolumeCopy", err[BIFF_STRLEN];
  pullVolume *volNew;

  volNew = pullVolumeNew();
  if (_pullVolumeSet(volNew, volOrig->name, 
                     volOrig->ninSingle,
                     volOrig->ninScale, volOrig->scaleNum,
                     volOrig->gpvl->kind,
                     volOrig->scaleMin, volOrig->scaleMax,
                     volOrig->ksp00, volOrig->ksp11,
                     volOrig->ksp22, volOrig->kspSS)) {
    sprintf(err, "%s: trouble creating new volume", me);
    biffAdd(PULL, err); return NULL;
  }
  if (gageUpdate(volNew->gctx)) {
    sprintf(err, "%s: trouble with new volume gctx", me);
    biffMove(PULL, err, GAGE); return NULL;
  }
  return volNew;
}
