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

#include "ten.h"
#include "tenPrivate.h"

/*
** the volume fits inside [-vhlen[0],vhlen[0]] x [-vhlen[1],vhlen[1]] x [-vhlen[2],vhlen[2]]
*/
/*
void
_tenLearnLengths(double vhlen[3], int center, Nrrd *dtvol) {
  double maxLen;
  int numSamples[3], numElements[3];

  numSamples[0] = dtvol->axis[1].size;
  numSamples[1] = dtvol->axis[2].size;
  numSamples[2] = dtvol->axis[3].size;
  if (nrrdCenterCell == center) {
    numElements[0] = numSamples[0];
    numElements[1] = numSamples[1];
    numElements[2] = numSamples[2];
  } else {
    numElements[0] = numSamples[0]-1;
    numElements[1] = numSamples[1]-1;
    numElements[2] = numSamples[2]-1;
  }
  vhlen[0] = numElements[0]*dtvol->axis[1].spacing;
  vhlen[1] = numElements[1]*dtvol->axis[1].spacing;
  vhlen[2] = numElements[2]*dtvol->axis[1].spacing;
  maxLen = AIR_MAX(vhlen[0], vhlen[1]);
  maxLen = AIR_MAX(vhlen[2], maxLen);
  vhlen[0] /= maxLen;
  vhlen[1] /= maxLen;
  vhlen[2] /= maxLen;
}
*/

tenFiberContext *
tenFiberContextNew(Nrrd *dtvol) {
  char me[]="tenFiberContextNew", err[AIR_STRLEN_MED];
  tenFiberContext *tfx;
  gagePerVolume *pvl;
  NrrdKernel *kernel;
  double kparm[NRRD_KERNEL_PARMS_NUM];
  
  tfx = calloc(1, sizeof(tenFiberContext));
  if (tfx) {
    if (tenTensorCheck(dtvol, nrrdTypeUnknown, AIR_TRUE)) {
      sprintf(err, "%s: didn't get a tensor volume", me);
      biffAdd(TEN, err); return NULL;
    }
    tfx->dtvol = dtvol;
    tfx->ksp = nrrdKernelSpecNew();
    tfx->gtx = gageContextNew();
    if ( !(pvl = gagePerVolumeNew(dtvol, tenGageKind)) 
	 || (gagePerVolumeAttach(tfx->gtx, pvl)) ) {
      sprintf(err, "%s: gage trouble", me);
      biffMove(TEN, err, GAGE); return NULL;
    }
    if (nrrdKernelParse(&kernel, kparm, tenDefFiberKernel)) {
      sprintf(err, "%s: couldn't parse tenDefFiberKernel \"%s\"",
	      me,  tenDefFiberKernel);
      biffMove(TEN, err, NRRD); return NULL;
    }
    if (tenFiberKernelSet(tfx, kernel, kparm)) {
      sprintf(err, "%s: couldn't set default kernel", me);
      biffAdd(TEN, err); return NULL;
    }
    tfx->type = tenFiberTypeUnknown;
    tfx->step = tenDefFiberStep;
    tfx->maxHalfLen = tenDefFiberMaxHalfLen;

    tfx->dten = gageAnswerPointer(tfx->gtx->pvl[0], tenGageTensor);
    tfx->evec = gageAnswerPointer(tfx->gtx->pvl[0], tenGageEvec);
  }
  return tfx;
}

int
tenFiberTypeSet(tenFiberContext *tfx, int type) {
  char me[]="tenFiberTypeSet", err[AIR_STRLEN_MED];
  gagePerVolume *pvl;
  int qse;

  if (!(tfx)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(TEN, err); return 1;
  }
  pvl = tfx->gtx->pvl[0];
  qse = 0;
  switch(type) {
  case tenFiberTypeEvec1:
    qse = gageQuerySet(pvl, ( (1 << tenGageEvec) 
			      | (1 << tenGageAniso) ));
    break;
  case tenFiberTypeTensorLine:
    qse = gageQuerySet(pvl, ( (1 << tenGageTensor)
			      | (1 << tenGageEvec) 
			      | (1 << tenGageAniso) ));
    break;
  case tenFiberTypePureLine:
    qse = gageQuerySet(pvl, ( (1 << tenGageTensor)
			      | (1 << tenGageAniso) ));
    break;
  case tenFiberTypeZhukov:
    sprintf(err, "%s: sorry, not Zhukov oriented tensors implemented", me);
    biffAdd(TEN, err); return 1;
    break;
  default:
    sprintf(err, "%s: fiber type %d not recognized", me, type);
    biffAdd(TEN, err); return 1;
    break;
  }
  if (qse) {
    sprintf(err, "%s: problem setting query", me);
    biffMove(TEN, err, GAGE); return 1;
  }
  tfx->type = type;
  return 0;
}

int
tenFiberKernelSet(tenFiberContext *tfx,
		  NrrdKernel *kern,
		  double parm[NRRD_KERNEL_PARMS_NUM]) {
  char me[]="tenFiberKernelSet", err[AIR_STRLEN_MED];

  if (!(tfx && kern)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(TEN, err); return 1;
  }
  nrrdKernelSpecSet(tfx->ksp, kern, parm);
  if (gageKernelSet(tfx->gtx, gageKernel00,
		    tfx->ksp->kernel, tfx->ksp->parm)) {
    sprintf(err, "%s: problem setting kernel", me);
    biffMove(TEN, err, GAGE); return 1;
  }
  
  return 0;
}

int
tenFiberUpdate(tenFiberContext *tfx) {
  char me[]="tenFiberUpdate", err[AIR_STRLEN_MED];

  if (!tfx) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(TEN, err); return 1;
  }
  if (tenFiberTypeUnknown == tfx->type) {
    sprintf(err, "%s: fiber type not set", me);
    biffAdd(TEN, err); return 1;
  }
  if (gageUpdate(tfx->gtx)) {
    sprintf(err, "%s: trouble", me);
    biffMove(TEN, err, GAGE); return 1;
  }
  return 0;
}

tenFiberContext *
tenFiberContextNix(tenFiberContext *tfx) {
  
  if (tfx) {
    tfx->ksp = nrrdKernelSpecNix(tfx->ksp);
    tfx->gtx = gageContextNix(tfx->gtx);
    free(tfx);
  }
  return NULL;
}

