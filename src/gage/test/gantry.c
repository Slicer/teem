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

#include <air.h>
#include <hest.h>
#include <nrrd.h>
#include <gage.h>

int
gantryParseNrrd(void *ptr, char *str, char err[AIR_STRLEN_HUGE]) {
  char me[] = "gantryParseNrrd", *nerr;
  Nrrd **nrrdP;
  airArray *mop;
  
  if (!(ptr && str)) {
    sprintf(err, "%s: got NULL pointer", me);
    return 1;
  }
  nrrdP = ptr;
  mop = airMopInit();
  *nrrdP = nrrdNew();
  airMopAdd(mop, *nrrdP, (airMopper)nrrdNuke, airMopOnError);
  if (nrrdLoad(*nrrdP, str)) {
    airMopAdd(mop, nerr = biffGetDone(NRRD), airFree, airMopOnError);
    strncpy(err, nerr, AIR_STRLEN_HUGE-1);
    airMopError(mop);
    return 1;
  }
  if (3 != (*nrrdP)->dim) {
    sprintf(err, "%s: need a 3-D nrrd (not %d)", me, (*nrrdP)->dim);
    airMopError(mop);
    return 1;
  }
  airMopOkay(mop);
  return 0;
}

hestCB gantryNrrdHestCB = {
  sizeof(Nrrd *),
  "nrrd",
  gantryParseNrrd,
  (airMopper)nrrdNuke
}; 

/*
** gantryNrrdKernel
** 
** this is what will be parsed from the command-line: a kernel and its
** parameter list
*/
typedef struct {
  NrrdKernel *kernel;
  double parm[NRRD_KERNEL_PARMS_NUM];
} gantryNrrdKernel;

int
gantryParseKernel(void *ptr, char *str, char err[AIR_STRLEN_HUGE]) {
  gantryNrrdKernel *ker;
  char me[]="gantryParseKernel", *nerr;

  if (!(ptr && str)) {
    sprintf(err, "%s: got NULL pointer", me);
    return 1;
  }
  ker = ptr;
  if (nrrdKernelParse(&(ker->kernel), ker->parm, str)) {
    nerr = biffGetDone(NRRD);
    strncpy(err, nerr, AIR_STRLEN_HUGE-1);
    free(nerr);
    return 1;
  }
  return 0;
}

hestCB gantryKernelHestCB = {
  sizeof(gantryNrrdKernel),
  "kernel specification",
  gantryParseKernel,
  NULL
};

char info[]="Gantry tilt be gone!  This program is actually of limited "
"utility: it can only change the tilt by shearing with the "
"X and Z axis fixed, by some angle \"around\" the X axis, assuming "
"that (X,Y,Z) is a right-handed frame. ";

int
main(int argc, char *argv[]) {
  hestParm *hparm;
  hestOpt *hopt = NULL;
  gageContext *ctx;
  gagePerVolume *pvl;
  gageSclAnswer *san;
  Nrrd *nin, *npad, *nout;
  char *me, *herr, *outS;
  float angle;
  double xs, ys, zs, y, z;
  int sx, sy, sz, E, xi, yi, zi, needPad;
  gantryNrrdKernel gantric;
  void *out;
  double (*insert)(void *v, nrrdBigInt I, double d);
  
  me = argv[0];
  hparm = hestParmNew();
  hparm->elideSingleOtherType = AIR_TRUE;

  hestOptAdd(&hopt, "i", "nin", airTypeOther, 1, 1, &nin, NULL,
	     "input volume, in nrrd format",
	     NULL, NULL, &gantryNrrdHestCB);
  hestOptAdd(&hopt, "a", "angle", airTypeFloat, 1, 1, &angle, NULL,
	     "angle, in degrees, of the gantry tilt around the X axis. "
	     "This is opposite of the amount of tweak we apply.");
  hestOptAdd(&hopt, "k", "kern", airTypeOther, 1, 1, &gantric,
	     "tent",
	     "The kernel to use for resampling.  Chances are, there "
	     "is no justification for anything more than \"tent\".  "
	     "Possibilities include:\n "
	     "\b\bo \"box\": nearest neighbor interpolation\n "
	     "\b\bo \"tent\": linear interpolation\n "
	     "\b\bo \"cubic:B,C\": Mitchell/Netravali BC-family of "
	     "cubics:\n "
	     "\t\t\"cubic:1,0\": B-spline; maximal blurring\n "
	     "\t\t\"cubic:0,0.5\": Catmull-Rom; good interpolating kernel\n "
	     "\b\bo \"quartic:A\": 1-parameter family of "
	     "interpolating quartics (\"quartic:0.0834\" is most accurate)\n "
	     "\b\bo \"gauss:S,C\": Gaussian blurring, with standard deviation "
	     "S and cut-off at C standard deviations",
	     NULL, NULL, &gantryKernelHestCB);
  hestOptAdd(&hopt, "o", "output", airTypeString, 1, 1, &outS, NULL,
	     "output volume in nrrd format");
  if (hestOptCheck(hopt, &herr)) { printf("%s\n", herr); exit(1); }
  
  if (1 == argc) {
    hestInfo(stderr, me, info, hparm);
    hestUsage(stderr, hopt, me, hparm);
    hestGlossary(stderr, hopt, hparm);
    hparm = hestParmFree(hparm);
    hopt = hestOptFree(hopt);
    exit(1);
  }
  if (hestParse(hopt, argc-1, argv+1, &herr, hparm)) {
    fprintf(stderr, "ERROR: %s\n", herr); free(herr);
    hestUsage(stderr, hopt, me, hparm);
    hestGlossary(stderr, hopt, hparm);
    hparm = hestParmFree(hparm);
    hopt = hestOptFree(hopt);
    exit(1);
  }

  sx = nin->axis[0].size;
  sy = nin->axis[1].size;
  sz = nin->axis[2].size;
  xs = nin->axis[0].spacing;
  ys = nin->axis[1].spacing;
  zs = nin->axis[2].spacing;
  if (!(AIR_EXISTS(xs) && AIR_EXISTS(ys) && AIR_EXISTS(zs))) {
    fprintf(stderr, "%s: all axis spacings must exist in input nrrd", me);
    exit(1);
  }
  printf("%s: input and output have dimensions %d %d %d\n",
	 me, sx, sy, sz);
  
  /* start by just copying the nrrd; then we'll meddle with the values */
  if (nrrdCopy(nout = nrrdNew(), nin)) {
    fprintf(stderr, "%s: trouble:\n%s\n", me, biffGet(NRRD));
    exit(1);
  }
  out = nout->data;
  insert = nrrdDInsert[nout->type];

  ctx = gageContextNew();
  gageValSet(ctx, gageValVerbose, 1);
  gageValSet(ctx, gageValRenormalize, AIR_TRUE);
  gageValSet(ctx, gageValCheckIntegrals, AIR_TRUE);
  if (gageKernelSet(ctx, gageKernel00, gantric.kernel, gantric.parm)) {
    fprintf(stderr, "%s: trouble:\n%s\n", me, biffGet(GAGE));
    exit(1);
  }  
  needPad = gageValGet(ctx, gageValNeedPad);
  if (nrrdSimplePad(npad=nrrdNew(), nin, needPad, nrrdBoundaryBleed)) {
    fprintf(stderr, "%s: trouble:\n%s\n", me, biffGet(NRRD));
    exit(1);
  }
  E = 0;
  if (!E) E |= !(pvl = gagePerVolumeNew(needPad, gageKindScalar));
  if (!E) san = (gageSclAnswer *)pvl->ans;
  if (!E) E |= gageVolumeSet(ctx, pvl, npad, needPad);
  if (!E) E |= gageQuerySet(pvl, 1 << gageSclValue);
  if (!E) E |= gageUpdate(ctx, pvl);
  if (E) {
    fprintf(stderr, "%s: trouble:\n%s\n", me, biffGet(GAGE));
    exit(1);
  }
  gageValSet(ctx, gageValVerbose, 0);
  
  for (zi=0; zi<sz; zi++) {
    for (yi=0; yi<sy; yi++) {
      for (xi=0; xi<sx; xi++) {
	
	/* convert to world space, use angle to determine new
	   world space position, convert back to index space,
	   clamp z to find inside old volume */
	
	y = (yi - sy/2.0)*ys;
	z = (zi*zs + y*sin(-angle*3.141592653/180.0))/zs;
	z = AIR_CLAMP(0, z, sz-1);
	gageProbe(ctx, pvl, xi, yi, z);
	insert(out, xi + sx*(yi + sy*zi), *(san->val));
      }
    }
  }

  if (nrrdSave(outS, nout, NULL)) {
    fprintf(stderr, "%s: trouble:\n%s\n", me, biffGet(NRRD));
    exit(1);
  }
  ctx = gageContextNix(ctx);
  hparm = hestParmFree(hparm);
  hopt = hestOptFree(hopt);
  pvl = gagePerVolumeNix(pvl);
  nrrdNuke(nout);
  nrrdNuke(npad);
  
  exit(0);
}
