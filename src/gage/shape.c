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

void
gageShapeSet(gageShape *shp, Nrrd *nin, gageKind *kind) {
  int i, bd;
  NrrdAxis *ax[3];

  if (!( shp && nin && kind ))
    return;

  bd = kind->baseDim;
  ax[0] = &(nin->axis[bd+0]);
  ax[1] = &(nin->axis[bd+1]);
  ax[2] = &(nin->axis[bd+2]);

  shp->sx = ax[0]->size;
  shp->sy = ax[1]->size;
  shp->sz = ax[2]->size;
  shp->xs = ax[0]->spacing;
  shp->ys = ax[1]->spacing;
  shp->zs = ax[2]->spacing;
  for (i=0; i<GAGE_KERNEL_NUM; i++) {
    switch (i) {
    case gageKernel00:
    case gageKernel10:
    case gageKernel20:
      /* interpolation requires no re-weighting for non-unit spacing */
      shp->fwScale[i][0] = 1.0;
      shp->fwScale[i][1] = 1.0;
      shp->fwScale[i][2] = 1.0;
      break;
    case gageKernel11:
    case gageKernel21:
      shp->fwScale[i][0] = 1.0/(shp->xs);
      shp->fwScale[i][1] = 1.0/(shp->ys);
      shp->fwScale[i][2] = 1.0/(shp->zs);
      break;
    case gageKernel22:
      shp->fwScale[i][0] = 1.0/((shp->xs)*(shp->xs));
      shp->fwScale[i][1] = 1.0/((shp->ys)*(shp->ys));
      shp->fwScale[i][2] = 1.0/((shp->zs)*(shp->zs));
      break;
    }
  }

  /* equality of axis centers is checked by gageVolumeCheck() */
  shp->center = (nrrdCenterUnknown == ax[0]->center
		 ? gageDefCenter
		 : ax[0]->center);

  return;
}

void
gageShapeReset(gageShape *shp) {
  int i;
  
  if (!shp)
    return;

  shp->sx = shp->sy = shp->sz = -1;
  shp->center = nrrdCenterUnknown;
  shp->xs = shp->ys = shp->zs = AIR_NAN;
  for (i=gageKernelUnknown+1; i<gageKernelLast; i++) {
    /* valgrind complained about AIR_NAN at -O2 */
    shp->fwScale[i][0] = shp->fwScale[i][1] = shp->fwScale[i][2] = airNaN();
  }

  return;
}

int
gageShapeEqual(gageShape *shp1, char *_name1,
	       gageShape *shp2, char *_name2) {
  char me[]="_gageShapeEqual", err[AIR_STRLEN_MED],
    *name1, *name2, what[] = "???";

  name1 = _name1 ? _name1 : what;
  name2 = _name2 ? _name2 : what;
  if (!( shp1->sx == shp2->sx &&
	 shp1->sy == shp2->sy &&
	 shp1->sz == shp2->sz )) {
    sprintf(err, "%s: dimensions of %s (%d,%d,%d) != %s's (%d,%d,%d)", me,
	    name1, shp1->sx, shp1->sy, shp1->sz,
	    name2, shp2->sx, shp2->sy, shp2->sz);
    biffAdd(GAGE, err); return 0;
  }
  if (!( shp1->xs == shp2->xs &&
	 shp1->ys == shp2->ys &&
	 shp1->zs == shp2->zs )) {
    sprintf(err, "%s: spacings of %s (%g,%g,%g) != %s's (%g,%g,%g)", me,
	    name1, shp1->xs, shp1->ys, shp1->zs,
	    name2, shp2->xs, shp2->ys, shp2->zs);
    biffAdd(GAGE, err); return 0;
  }
  if (!( shp1->center == shp2->center )) {
    sprintf(err, "%s: centering of %s (%s) != %s's (%s)", me,
	    name1, airEnumStr(nrrdCenter, shp1->center),
	    name2, airEnumStr(nrrdCenter, shp2->center));
    biffAdd(GAGE, err); return 0;
  }

  return 1;
}

int
gageVolumeCheck (Nrrd *nin, gageKind *kind) {
  char me[]="gageVolumeCheck", err[AIR_STRLEN_MED];
  double xs, ys, zs;
  int bd;

  if (nrrdCheck(nin)) {
    sprintf(err, "%s: basic nrrd validity check failed", me);
    biffMove(GAGE, err, NRRD); return 1;
  }
  if (nrrdTypeBlock == nin->type) {
    sprintf(err, "%s: need a non-block type nrrd", me);
    biffAdd(GAGE, err); return 1;
  }
  bd = kind->baseDim;
  if (3 + bd != nin->dim) {
    sprintf(err, "%s: nrrd should have dimension %d, not %d",
	    me, 3 + bd, nin->dim);
    biffAdd(GAGE, err); return 1;
  }
  xs = nin->axis[bd+0].spacing;
  ys = nin->axis[bd+1].spacing;
  zs = nin->axis[bd+2].spacing;
  if (!( AIR_EXISTS(xs) && AIR_EXISTS(ys) && AIR_EXISTS(zs) )) {
    sprintf(err, "%s: spacings for axes %d,%d,%d don't all exist",
	    me, bd+0, bd+1, bd+2);
    biffAdd(GAGE, err); return 1;
  }
  if (!( xs != 0 && ys != 0 && zs != 0 )) {
    sprintf(err, "%s: spacings (%g,%g,%g) for axes %d,%d,%d not all non-zero",
	    me, xs, ys, zs, bd+0, bd+1, bd+2);
    biffAdd(GAGE, err); return 1;
  }
  if (!( nin->axis[bd+0].center == nin->axis[bd+1].center &&
	 nin->axis[bd+0].center == nin->axis[bd+2].center )) {
    sprintf(err, "%s: axes %d,%d,%d centerings (%s,%s,%s) not equal", me,
	    bd+0, bd+1, bd+2,
	    airEnumStr(nrrdCenter, nin->axis[bd+0].center),
	    airEnumStr(nrrdCenter, nin->axis[bd+1].center),
	    airEnumStr(nrrdCenter, nin->axis[bd+2].center));
    biffAdd(GAGE, err); return 1;
  }
  return 0;
}
