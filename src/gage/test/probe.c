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


#include <stdio.h>
#include <hest.h>
#include <nrrd.h>
#include "../gage.h"

/* I'm cheating here: I don't want gage to depend on ten */
#define PROBE_MAT2LIST(l, m) ( \
   (l)[1] = (m)[0],          \
   (l)[2] = (m)[3],          \
   (l)[3] = (m)[6],          \
   (l)[4] = (m)[4],          \
   (l)[5] = (m)[7],          \
   (l)[6] = (m)[8] )

int
probeParseNrrd(void *ptr, char *str, char err[AIR_STRLEN_HUGE]) {
  char me[] = "probeParseNrrd", *nerr;
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
  airMopOkay(mop);
  return 0;
}

hestCB probeNrrdHestCB = {
  sizeof(Nrrd *),
  "nrrd",
  probeParseNrrd,
  (airMopper)nrrdNuke
}; 

int
probeParseKind(void *ptr, char *str, char err[AIR_STRLEN_HUGE]) {
  char me[] = "probeParseKind";
  gageKind **kindP;
  
  if (!(ptr && str)) {
    sprintf(err, "%s: got NULL pointer", me);
    return 1;
  }
  kindP = ptr;
  airToLower(str);
  if (!strcmp("scalar", str)) {
    *kindP = gageKindScl;
  } else if (!strcmp("vector", str)) {
    *kindP = gageKindVec;
  } else {
    sprintf(err, "%s: not \"scalar\" or \"vector\"", me);
    return 1;
  }
    
  return 0;
}

hestCB probeKindHestCB = {
  sizeof(gageKind *),
  "kind",
  probeParseKind,
  NULL
}; 

/*
** probeNrrdKernel
** 
** this is what will be parsed from the command-line: a kernel and its
** parameter list
*/
typedef struct {
  NrrdKernel *k;
  double kparm[NRRD_KERNEL_PARMS_NUM];
} probeNrrdKernel;

int
probeParseKernel(void *ptr, char *str, char err[AIR_STRLEN_HUGE]) {
  probeNrrdKernel *ker;
  char me[]="probeParseKernel", *nerr;

  if (!(ptr && str)) {
    sprintf(err, "%s: got NULL pointer", me);
    return 1;
  }
  ker = ptr;
  if (nrrdKernelParse(&(ker->k), ker->kparm, str)) {
    nerr = biffGetDone(NRRD);
    strncpy(err, nerr, AIR_STRLEN_HUGE-1);
    free(nerr);
    return 1;
  }
  return 0;
}

hestCB probeKernelHestCB = {
  sizeof(probeNrrdKernel),
  "kernel specification",
  probeParseKernel,
  NULL
};

char *probeInfo = ("Shows off the functionality of the gage library. "
		   "Uses gageProbe() to query scalar or vector volumes "
		   "to learn various measured or derived quantities. ");

int
main(int argc, char *argv[]) {
  gageKind *kind;
  char *me, *outS, *whatS, *herr;
  hestParm *hparm;
  hestOpt *hopt = NULL;
  probeNrrdKernel k00, k11, k22;
  float x, y, z, scale;
  int what, a, idx, ansLen, E, xi, yi, zi, otype,
    six, siy, siz, sox, soy, soz;
  gage_t *answer;
  Nrrd *nin, *nout;
  gageSimple *gsl;
  gageSclAnswer *san;
  gageVecAnswer *van;
  double t0, t1;

  me = argv[0];
  hparm = hestParmNew();
  hparm->elideSingleOtherType = AIR_TRUE;
  hestOptAdd(&hopt, "i", "nin", airTypeOther, 1, 1, &nin, NULL,
	     "input volume",
	     NULL, NULL, &probeNrrdHestCB);
  hestOptAdd(&hopt, "k", "kind", airTypeOther, 1, 1, &kind, NULL,
	     "\"kind\" of volume (\"scalar\" or \"vector\")",
	     NULL, NULL, &probeKindHestCB);
  hestOptAdd(&hopt, "q", "query", airTypeString, 1, 1, &whatS, NULL,
	     "the quantity (scalar, vector, or matrix) to learn by probing");
  hestOptAdd(&hopt, "s", "scale", airTypeFloat, 1, 1, &scale, "1.0",
	     "scaling factor (>1.0 : supersampling)");
  hestOptAdd(&hopt, "00", "kern00", airTypeOther, 1, 1, &k00,
	     "tent", "kernel for gageKernel00",
	     NULL, NULL, &probeKernelHestCB);
  hestOptAdd(&hopt, "11", "kern11", airTypeOther, 1, 1, &k11,
	     "fordif", "kernel for gageKernel11",
	     NULL, NULL, &probeKernelHestCB);
  hestOptAdd(&hopt, "22", "kern22", airTypeOther, 1, 1, &k22,
	     "fordif", "kernel for gageKernel22",
	     NULL, NULL, &probeKernelHestCB);
  hestOptAdd(&hopt, "t", "type", airTypeEnum, 1, 1, &otype, "float",
	     "type of output volume", NULL, nrrdType);
  hestOptAdd(&hopt, "o", "nout", airTypeString, 1, 1, &outS, NULL,
	     "output volume");

  if (argc-1 < hestMinNumArgs(hopt)) {
    hestInfo(stderr, me, probeInfo, hparm);
    hestUsage(stderr, hopt, me, hparm);
    hestGlossary(stderr, hopt, hparm);
    hestOptFree(hopt); hestParmFree(hparm);
    return 1;
  }
  if (hestParse(hopt, argc-1, argv+1, &herr, hparm)) {
    fprintf(stderr, "%s: %s\n", me, herr); free(herr);
    hestUsage(stderr, hopt, me, hparm);
    hestGlossary(stderr, hopt, hparm);
    hestOptFree(hopt); hestParmFree(hparm);
    return 1;
  }
  printf("|%s|\n", whatS);
  what = airEnumVal(kind->enm, whatS);
  if (-1 == what) {
    /* -1 indeed always means "unknown" for any gageKind */
    fprintf(stderr, "%s: couldn't parse \"%s\" as measure of \"%s\" volume\n",
	    me, whatS, kind->name);
    hestUsage(stderr, hopt, me, hparm);
    hestGlossary(stderr, hopt, hparm);
    hestOptFree(hopt); hestParmFree(hparm);
    return 1;
  }

  ansLen = gageSclAnsLength[what];
  printf("%s: ansLen = %d\n", me, ansLen);

  /***
  **** Except for the gageSimpleProbe() call in the inner loop below,
  **** and the gageSimpleNix() call at the very end, all the gage
  **** calls which set up the simple context and state are here.
  ***/
  gsl = gageSimpleNew();
  gageValSet(gsl->ctx, gageValVerbose, 1);
  gageValSet(gsl->ctx, gageValRenormalize, AIR_TRUE);
  gageValSet(gsl->ctx, gageValCheckIntegrals, AIR_TRUE);
  E = 0;
  if (!E) E |= gageSimpleKernelSet(gsl, gageKernel00, k00.k, k00.kparm);
  if (!E) E |= gageSimpleKernelSet(gsl, gageKernel11, k11.k, k11.kparm);
  if (!E) E |= gageSimpleKernelSet(gsl, gageKernel22, k22.k, k22.kparm);
  if (E) {
    fprintf(stderr, "%s: trouble:\n%s\n", me, biffGet(GAGE));
    exit(1);
  }
  gsl->nin = nin;
  gsl->kind = kind;
  gsl->query = 1<<what;
  if (gageSimpleUpdate(gsl)) {
    fprintf(stderr, "%s: trouble:\n%s\n", me, biffGet(GAGE));
    exit(1);
  }
  gsl->pvl->verbose = gageValGet(gsl->ctx, gageValVerbose);
  san = (gageSclAnswer *)(gsl->pvl->ans);
  van = (gageVecAnswer *)(gsl->pvl->ans);
  if (gageKindScl == kind) {
    answer = san->ans + gageSclAnsOffset[what];
  } else if (gageKindVec == kind) {
    answer = van->ans + gageVecAnsOffset[what];
  }
  /***
  **** end gage setup.
  ***/

  six = nin->axis[0].size;
  siy = nin->axis[1].size;
  siz = nin->axis[2].size;
  sox = scale*six;
  soy = scale*siy;
  soz = scale*siz;
  if (ansLen > 1) {
    printf("%s: creating %d x %d x %d x %d output\n", 
	   me, ansLen, sox, soy, soz);
    if (!E) E != nrrdAlloc(nout=nrrdNew(), otype, 4, ansLen, sox, soy, soz);
  } else {
    printf("%s: creating %d x %d x %d output\n", me, sox, soy, soz);
    if (!E) E != nrrdAlloc(nout=nrrdNew(), otype, 3, sox, soy, soz);
  }
  if (E) {
    fprintf(stderr, "%s: trouble:\n%s\n", me, biffGet(NRRD));
    exit(1);
  }
  t0 = airTime();
  fprintf(stderr, "%s: si{x,y,z} = %d, %d, %d\n", me, six, siy, siz);
  fprintf(stderr, "%s: so{x,y,z} = %d, %d, %d\n", me, sox, soy, soz);
  for (zi=0; zi<=soz-1; zi++) {
    printf("%d/%d ", zi, soz-1); fflush(stdout);
    z = AIR_AFFINE(0, zi, soz-1, 0, siz-1);
    for (yi=0; yi<=soy-1; yi++) {
      y = AIR_AFFINE(0, yi, soy-1, 0, siy-1);
      for (xi=0; xi<=sox-1; xi++) {
	x = AIR_AFFINE(0, xi, sox-1, 0, six-1);
	idx = xi + sox*(yi + soy*zi);

	gsl->ctx->verbose = 3*( !xi && !yi && !zi ||
				/* ((100 == xi) && (8 == yi) && (8 == zi)) */
				((61 == xi) && (51 == yi) && (46 == zi))
				/* ((40==xi) && (30==yi) && (62==zi)) || */
				/* ((40==xi) && (30==yi) && (63==zi)) */ ); 

	if (gageSimpleProbe(gsl, x, y, z)) {
	  fprintf(stderr, 
		  "%s: trouble at i=(%d,%d,%d) -> f=(%g,%g,%g):\n%s\n(%d)\n",
		  me, xi, yi, zi, x, y, z, gageErrStr, gageErrNum);
	  exit(1);
	}
	if (1 == ansLen) {
	  nrrdFInsert[nout->type](nout->data, idx,
				  nrrdFClamp[nout->type](*answer));
	} else {
	  for (a=0; a<=ansLen-1; a++) {
	    nrrdFInsert[nout->type](nout->data, a + ansLen*idx, 
				    nrrdFClamp[nout->type](answer[a]));
	  }
	}
      }
    }
  }
  printf("\n");
  t1 = airTime();
  printf("probe rate = %g/sec\n", sox*soy*soz/(t1-t0));
  nrrdSave(outS, nout, NULL);

  nrrdNuke(nin);
  nrrdNuke(nout);
  gageSimpleNix(gsl);
  hestOptFree(hopt); hestParmFree(hparm);
  exit(0);
}
