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

#include "gage.h"
#include "privateGage.h"

/*
** _gageNeedDSet()
**
** goes through pervolumes to find collective needD[], and checks
** to see if this has changed
**
** Sets: ctx->needD[]
*/
int
_gageNeedDSet(gageContext *ctx) {
  int needD[3], i;
  
  needD[0] = needD[1] = needD[2] = AIR_FALSE;
  for (i=0; i<ctx->numPvl; i++) {
    needD[0] |= ctx->pvl[i]->needD[0];
    needD[1] |= ctx->pvl[i]->needD[1];
    needD[2] |= ctx->pvl[i]->needD[2];
  }
  /*
  if (!( needD[0] || needD[1] || needD[2] )) {
    sprintf(err, "%s: none of the pervolumes need any measurements", me);
    biffAdd(GAGE, err); return 1;
  }
  */
  if ( (needD[0] != ctx->needD[0]) ||
       (needD[1] != ctx->needD[1]) ||
       (needD[2] != ctx->needD[2]) ) {
    ctx->needD[0] = needD[0];
    ctx->needD[1] = needD[1];
    ctx->needD[2] = needD[2];
    ctx->flag[gageFlagNeedD] = AIR_TRUE;
  }
  return 0;
}

/*
** _gageNeedKSet()
**
** looks at needD[] and k3pack to determine needK[], and sees if
** the new value differs from the one already in the context
**
** Sets: ctx->needK[]
*/
void
_gageNeedKSet(gageContext *ctx) {
  int k, needK[GAGE_KERNEL_NUM], change;
  
  for (k=0; k<GAGE_KERNEL_NUM; k++) {
    needK[k] = AIR_FALSE;
  }
  if (ctx->needD[0]) {
    needK[gageKernel00] = AIR_TRUE;
  }
  if (ctx->needD[1]) {
    needK[gageKernel11] = AIR_TRUE;
    if (ctx->k3pack) {
      needK[gageKernel00] = AIR_TRUE;
    } else {
      needK[gageKernel10] = AIR_TRUE;
    }  
  }
  if (ctx->needD[2]) {
    needK[gageKernel22] = AIR_TRUE;
    if (ctx->k3pack) {
      needK[gageKernel00] = AIR_TRUE;
      needK[gageKernel11] = AIR_TRUE;
    } else {
      needK[gageKernel20] = AIR_TRUE;
      needK[gageKernel21] = AIR_TRUE;
    }  
  }
  change = AIR_FALSE;
  for (k=0; k<GAGE_KERNEL_NUM; k++) {
    change |= (needK[k] != ctx->needK[k]);
  }
  if (change) {
    for (k=0; k<GAGE_KERNEL_NUM; k++) {
      ctx->needK[k] = needK[k];
    }
    ctx->flag[gageFlagNeedK] = AIR_TRUE;
  }
}

/*
** _gageNeedPadSet()
**
** determines padding required from needK[] and current kernel set
** 
** Sets: ctx->needPad
*/
int
_gageNeedPadSet(gageContext *ctx) {
  char me[]="_gageNeedPadSet", err[AIR_STRLEN_MED];
  int k, fr, needPad;
  double maxRad;

  maxRad = 0;
  for (k=0; k<GAGE_KERNEL_NUM; k++) {
    if (ctx->needK[k]) {
      if (!ctx->k[k]) {
	sprintf(err, "%s: need kernel %s but it hasn't been set", 
		me, airEnumStr(gageKernel, k));
	biffAdd(GAGE, err); return 1;
      }
      maxRad = AIR_MAX(maxRad, ctx->k[k]->support(ctx->kparm[k]));
    }
  }
  fr = AIR_ROUNDUP(maxRad);
  /* In case either kernels have tiny supports (less than 0.5), or if
     we in fact don't need any kernels, then we need to do this to 
     ensure that we generate a valid (trivial) padding */
  fr = AIR_MAX(fr, 1);
  needPad = fr - 1;
  if (needPad != ctx->needPad) {
    ctx->needPad = needPad;
    ctx->flag[gageFlagNeedPad] = AIR_TRUE;
  }

  return 0;
}

/*
** _gageHavePadSet()
**
** sets havePad based on noRepadWhenSmaller and needPad, and everything
** else which immediately and solely depends on havePad
**
** Sets ctx->havePad, as well as ctx->fr, ctx->fd, as well as allocating
** ctx->fsl, ctx->fw, and ctx->off, as well as allocating 
** pvl->iv3, pvl->iv2, and pvl->iv1 in all attached pervolumes
*/
int
_gageHavePadSet(gageContext *ctx) {
  char me[]="_gageHavePadSet", err[AIR_STRLEN_MED];
  int havePad, fd, i;
  gagePerVolume *pvl;
  
  if (ctx->noRepadWhenSmaller) {
    havePad = AIR_MAX(ctx->needPad, ctx->havePad);
  } else {
    havePad = ctx->needPad;
  }
  if (havePad != ctx->havePad) {
    ctx->havePad = havePad;
    ctx->fr = havePad + 1;
    fd = ctx->fd = 2*ctx->fr;
    airFree(ctx->fsl);
    airFree(ctx->fw);
    airFree(ctx->off);
    ctx->fsl = calloc(fd*3, sizeof(gage_t));
    ctx->fw = calloc(fd*3*GAGE_KERNEL_NUM, sizeof(gage_t));
    ctx->off = calloc(fd*fd*fd, sizeof(unsigned int));
    if (!(ctx->fsl && ctx->fw && ctx->off)) {
      sprintf(err, "%s: couldn't allocate filter caches for fd=%d", me, fd);
      biffAdd(GAGE, err); return 1;
    }
    for (i=0; i<ctx->numPvl; i++) {
      pvl = ctx->pvl[i];
      airFree(pvl->iv3);
      airFree(pvl->iv2);
      airFree(pvl->iv1);
      pvl->iv3 = calloc(fd*fd*fd*pvl->kind->valLen, sizeof(gage_t));
      pvl->iv2 = calloc(fd*fd*pvl->kind->valLen, sizeof(gage_t));
      pvl->iv1 = calloc(fd*pvl->kind->valLen, sizeof(gage_t));
      if (!(ctx->pvl[i]->iv3 && ctx->pvl[i]->iv2 && ctx->pvl[i]->iv1)) {
	sprintf(err, "%s: couldn't allocate pvl[%d]'s value caches for fd=%d",
		me, i, fd);
	biffAdd(GAGE, err); return 1;
      }
    }
    ctx->flag[gageFlagHavePad] = AIR_TRUE;
  }

  return 0;
}

/*
** _gageSpacingSet()
**
** This is kind of an odd function- (1) its has an error checking
** role, in making sure that all the pervolumes have a nin, and that
** all nins match in size and spacing, and (2) the role of setting
** information immediately and solely dependent on that size and
** spacing, namely ctx->{sx,sy,sz,fwScale}.  The error checking is
** delayed until now because we can't do it until we know that all
** nins have been supposedly set in all pervolumes.
**
** Sets: ctx->xs, ctx->ys, ctx->zs, ctx->fwScale[] (values)
*/
int
_gageSpacingSet(gageContext *ctx) {
  char me[]="_gageSpacingSet", err[AIR_STRLEN_MED];
  int i, sx=0, sy=0, sz=0, bd;
  double xs=1, ys=1, zs=1;
  gagePerVolume *pvl;

  for (i=0; i<ctx->numPvl; i++) {
    pvl = ctx->pvl[i];
    bd = pvl->kind->baseDim;
    if (!pvl->nin) {
      sprintf(err, "%s: no volume set in pervolume %d", me, i);
      biffAdd(GAGE, err); return 1;
    }
    if (0 == i) {
      sx = pvl->nin->axis[bd+0].size;
      sy = pvl->nin->axis[bd+1].size;
      sz = pvl->nin->axis[bd+2].size;
      xs = pvl->nin->axis[bd+0].spacing;
      ys = pvl->nin->axis[bd+1].spacing;
      zs = pvl->nin->axis[bd+2].spacing;
    }
    else {
      if (!( sx == pvl->nin->axis[bd+0].size &&
	     sy == pvl->nin->axis[bd+1].size &&
	     sz == pvl->nin->axis[bd+2].size )) {
	sprintf(err, "%s: dimensions of volume %d (%d,%d,%d) != "
		"those of volume 0 (%d,%d,%d)",
		me, i, sx, sy, sz,
		pvl->nin->axis[bd+0].size,
		pvl->nin->axis[bd+1].size,
		pvl->nin->axis[bd+2].size);
	biffAdd(GAGE, err); return 1;
      }
      if (!( xs == pvl->nin->axis[bd+0].spacing &&
	     ys == pvl->nin->axis[bd+1].spacing &&
	     zs == pvl->nin->axis[bd+2].spacing )) {
	sprintf(err, "%s: spacings of volume %d (%g,%g,%g) != "
		"those of volume 0 (%g,%g,%g)",
		me, i, xs, ys, zs,
		pvl->nin->axis[bd+0].spacing,
		pvl->nin->axis[bd+1].spacing,
		pvl->nin->axis[bd+2].spacing);
	biffAdd(GAGE, err); return 1;
      }
    }
  }
  if (ctx->numPvl) {
    ctx->xs = xs;
    ctx->ys = ys;
    ctx->zs = zs;
    for (i=0; i<GAGE_KERNEL_NUM; i++) {
      switch (i) {
      case gageKernel00:
      case gageKernel10:
      case gageKernel20:
	/* interpolation requires no re-weighting for non-unit spacing */
	ctx->fwScale[i][0] = 1.0;
	ctx->fwScale[i][1] = 1.0;
	ctx->fwScale[i][2] = 1.0;
	break;
      case gageKernel11:
      case gageKernel21:
	ctx->fwScale[i][0] = 1.0/xs;
	ctx->fwScale[i][1] = 1.0/ys;
	ctx->fwScale[i][2] = 1.0/zs;
	break;
      case gageKernel22:
	ctx->fwScale[i][0] = 1.0/(xs*xs);
	ctx->fwScale[i][1] = 1.0/(ys*ys);
	ctx->fwScale[i][2] = 1.0/(zs*zs);
	break;
      }
    }
  }
  return 0;
}

/*
** _gageNpadSet()
**
** brings all npads up to date with nins, padder, and havePad.  If
** padder or havePad changed, then all npads have to created/replaced.
** But if neither padder nor havePad changed, then only some npads have
** to be created/replaced.  pvl->flagNin is the way to see if a nin changed.
**
** Sets: pvl->npad
*/
int
_gageNpadSet(gageContext *ctx) {
  char me[]="_gageNpadSet", err[AIR_STRLEN_MED];
  int i, padall, bd, ninSize[NRRD_DIM_MAX], npadSize[NRRD_DIM_MAX];
  void *padInfo;
  gagePerVolume *pvl;

  padall = (ctx->flag[gageFlagPadder] && ctx->flag[gageFlagHavePad]);
  for (i=0; i<ctx->numPvl; i++) {
    pvl = ctx->pvl[i];
    if (padall || pvl->flagNin) {
      if (ctx->nixer) {
	padInfo = (ctx->nixer == _gageStandardNixer
		   ? NULL
		   : pvl->padInfo);
	ctx->nixer(pvl->npad, pvl->kind, padInfo);
	pvl->npad = NULL;
      }
      if (ctx->padder) {
	padInfo = (ctx->padder == _gageStandardPadder
		   ? NULL
		   : pvl->padInfo);
	pvl->npad = nrrdNew();       /* doesn't use biff */
	if (ctx->padder(pvl->npad, pvl->nin, pvl->kind,
			ctx->havePad, padInfo)) {
	  sprintf(err, "%s: trouble in padder callback", me);
	  biffAdd(GAGE, err); return 1;
	}
	/* trust no one: make sure that the padder did the right
	   amount of padding */
	if (pvl->nin->dim != pvl->npad->dim) {
	  sprintf(err, "%s: whoa: padder made %d-dim out of %d-dim nrrd", 
		  me, pvl->npad->dim, pvl->nin->dim);
	  biffAdd(GAGE, err); return 1;
	}
	bd = pvl->kind->baseDim;
	nrrdAxesGet_nva(pvl->nin, nrrdAxesInfoSize, ninSize);
	nrrdAxesGet_nva(pvl->npad, nrrdAxesInfoSize, npadSize);
	if (!( (ninSize[bd+0] + 2*ctx->havePad == npadSize[bd+0]) &&
	       (ninSize[bd+1] + 2*ctx->havePad == npadSize[bd+1]) &&
	       (ninSize[bd+2] + 2*ctx->havePad == npadSize[bd+2]) )) {
	  sprintf(err, "%s: padder padded sizes (%d,%d,%d) to (%d,%d,%d) "
		  "instead of (%d,%d,%d) (havePad = %d)", me,
		  ninSize[bd+0], ninSize[bd+1], ninSize[bd+2], 
		  npadSize[bd+0], npadSize[bd+1], npadSize[bd+2],
		  ninSize[bd+0] + 2*ctx->havePad,
		  ninSize[bd+1] + 2*ctx->havePad,
		  ninSize[bd+2] + 2*ctx->havePad, ctx->havePad);
	  biffAdd(GAGE, err); return 1;
	}
	ctx->pvlFlag[gageFlagNpad] = AIR_TRUE;
      }
      pvl->flagNin = AIR_FALSE;
    }
  }
  return 0;
}

/*
** _gageSizesSet()
**
** dumb little function- just sets sx, sy, sz, and off[] values in the
** context based on one of the padded volumes, but only if there are any
**
** Sets: ctx->sx, ctx->sy, ctx->sz, ctx->off[] (values)
*/
void
_gageSizesSet(gageContext *ctx) {
  int i, j, k, bd;
  gagePerVolume *pvl;

  if (ctx->numPvl) {
    pvl = ctx->pvl[0];
    bd = pvl->kind->baseDim;
    ctx->sx = pvl->npad->axis[bd+0].size;
    ctx->sy = pvl->npad->axis[bd+1].size;
    ctx->sz = pvl->npad->axis[bd+2].size;
    for (k=0; k<ctx->fd; k++)
      for (j=0; j<ctx->fd; j++)
	for (i=0; i<ctx->fd; i++)
	  ctx->off[i + ctx->fd*(j + ctx->fd*k)] = i + ctx->sx*(j + ctx->sy*k);
  }
}

/*
******** gageUpdate()
**
** call just before probing begins.
*/
int
gageUpdate(gageContext *ctx) {
  char me[]="gageUpdate", err[AIR_STRLEN_MED];

  if (!( ctx )) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(GAGE, err); return 1;
  }
  if (0 == ctx->numPvl) {
    sprintf(err, "%s: context has no attached pervolumes", me);
    biffAdd(GAGE, err); return 1;
  }
  if (ctx->pvlFlag[gageFlagNeedD]) {
    if (ctx->verbose) {
      fprintf(stderr, "%s: calling _gageNeedDSet()\n", me);
    }
    _gageNeedDSet(ctx);
    ctx->pvlFlag[gageFlagNeedD] = AIR_FALSE;
  }
  if (ctx->flag[gageFlagNeedD] || ctx->flag[gageFlagK3Pack]) {
    if (ctx->verbose) {
      fprintf(stderr, "%s: calling _gageNeedKSet()\n", me);
    }
    _gageNeedKSet(ctx);
    ctx->flag[gageFlagNeedD] = AIR_FALSE;
    ctx->flag[gageFlagK3Pack] = AIR_FALSE;
  }
  if (ctx->flag[gageFlagNeedK] || ctx->flag[gageFlagKernel]) {
    if (ctx->verbose) {
      fprintf(stderr, "%s: calling _gageNeedPadSet()\n", me);
    }
    if (_gageNeedPadSet(ctx)) {
      sprintf(err, "%s: ", me); biffAdd(GAGE, err); return 1;
    }
    ctx->flag[gageFlagNeedK] = AIR_FALSE;
    ctx->flag[gageFlagKernel] = AIR_FALSE;
  }
  if (ctx->flag[gageFlagNRWS] || ctx->flag[gageFlagNeedPad]) {
    if (ctx->verbose) {
      fprintf(stderr, "%s: calling _gageHavePadSet()\n", me);
    }
    if (_gageHavePadSet(ctx)) {
      sprintf(err, "%s: ", me); biffAdd(GAGE, err); return 1;
    }
    ctx->flag[gageFlagNRWS] = AIR_FALSE;
    ctx->flag[gageFlagNeedPad] = AIR_FALSE;
  }
  if (ctx->verbose) 
    fprintf(stderr, "%s: needPad = %d, havePad = %d\n", 
	    me, ctx->needPad, ctx->havePad);
  if (ctx->pvlFlag[gageFlagNin]) {
    if (ctx->verbose) {
      fprintf(stderr, "%s: calling _gageSpacingSet()\n", me);
    }
    if (_gageSpacingSet(ctx)) {
      sprintf(err, "%s: ", me); biffAdd(GAGE, err); return 1;
    }
  }
  if (ctx->pvlFlag[gageFlagNin] ||
      ctx->flag[gageFlagPadder] ||
      ctx->flag[gageFlagHavePad]) {
    if (ctx->verbose) {
      fprintf(stderr, "%s: calling _gageNpadSet()\n", me);
    }
    if (_gageNpadSet(ctx)) {
      sprintf(err, "%s: ", me); biffAdd(GAGE, err); return 1;
    }
    ctx->pvlFlag[gageFlagNin] = AIR_FALSE;
    ctx->flag[gageFlagPadder] = AIR_FALSE;
    ctx->flag[gageFlagHavePad] = AIR_FALSE;
  }
  if (ctx->pvlFlag[gageFlagNpad]) {
    if (ctx->verbose) {
      fprintf(stderr, "%s: calling _gageSizesSet()\n", me);
    }
    _gageSizesSet(ctx);
    ctx->pvlFlag[gageFlagNpad] = AIR_FALSE;
  }

  /* chances are, something above has invalidated the state maintained
     during successive calls to gageProbe() */
  ctx->xf = ctx->yf = ctx->zf = AIR_NAN;
  ctx->bidx = -1;

  return 0;
}

