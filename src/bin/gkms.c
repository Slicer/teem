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
#include <string.h>

#include <air.h>
#include <biff.h>
#include <nrrd.h>
#include <bane.h>

char *me, key[] = "gkms";

/* ------------------------------------------------------ */
/* ----------------------- HVOL ------------------------- */
/* ------------------------------------------------------ */

int
usageHVol(void) {
  fprintf(stderr, "usage: %s hvol [-s v,g,h] [-p v,g,h] [-d v,g,h]\n", me);
  fprintf(stderr, "       [-h lapl|hess|gmg] <input> <hvolOut>\n");
  fprintf(stderr, "Default values are in {}s\n");
  fprintf(stderr, "  -s v,g,h : Strategies for determining inclusion of values in the histogram\n");
  fprintf(stderr, "             volume.  Each of v,g,h can be ... \n");
  fprintf(stderr, "             \"f\": fraction of maximum range\n");
  fprintf(stderr, "             \"p\": percentile of highest values to exclude\n");
  fprintf(stderr, "             \"a\": absolute specification\n");
  fprintf(stderr, "             \"s\": multiple of standard deviation\n");
  fprintf(stderr, "             {f,p,p}\n");
  fprintf(stderr, "  -p v,g,h : Parameters to control chosen inclusion strategies. Each of v,g,h\n");
  fprintf(stderr, "             should be a scalar value.\n");
  fprintf(stderr, "             {f: 1.0; p: 0.15; a: (no default); s: 70}\n");
  fprintf(stderr, "  -d v,g,h : Dimensions of output histogram volume.  Each of v,g,h should be\n");
  fprintf(stderr, "             an integer\n");
  fprintf(stderr, "             {256,256,256}\n");
  fprintf(stderr, "-h lapl|hess|gmg: how to measure second derivative\n");
  fprintf(stderr, "             \"lapl\": Laplacian\n");
  fprintf(stderr, "             \"hess\": Hessian-based\n");
  fprintf(stderr, "             \"gmg\": gradient of gradient magnitude\n");
  fprintf(stderr, "             {hess}\n");
  fprintf(stderr, "     input : 3-D scalar nrrd being analyzed\n");
  fprintf(stderr, "   hvolOut : 3-D histogram volume to be created\n");
  return 1;
}

int
parseHVol(Nrrd **ninP, char **outNameP, baneHVolParm *hvp,
	  int argc, char *argv[]) {
  char me[]="parseHVol", err[512], incs[3], *inName;
  float incp[3];
  int i, a, dim[3], perm[3], miss=0;

  /* perm[i] is the index in this executable's context for the 
     variable with index i within bane;  bane's ordering for the
     histogram volume is g, h, v; not v, g, h */
  perm[0] = 1;
  perm[1] = 2;
  perm[2] = 0;

  /* set defaults for inclusion strategies */
  incs[0] = 'f'; incs[1] = 'p'; incs[2] = 'p';
  
  /* if these don't get updated we'll know */
  incp[0] = incp[1] = incp[2] = AIR_NAN;
  inName = *outNameP = NULL;
  
  /* set the defaults */
  baneHVolParmGKMSInit(hvp);

  a = 2;
  while (argv[a]) {
    miss = 0;
    if (!strcmp("-s", argv[a])) {
      miss = !argv[a+1]; if (miss) break;
      if (3 != airParseStrC(incs, argv[++a], ",", 3)) {
	sprintf(err, "%s: couldn't parse 3 chars from \"%s\"", me, argv[a]);
	biffSet(key, err); return 1;
      }
      /* printf("%s: HEY incs = %c %c %c\n", me, incs[0], incs[1], incs[2]); */
    }
    else if (!strcmp("-p", argv[a])) {
      miss = !argv[a+1]; if (miss) break;
      if (3 != airParseStrF(incp, argv[++a], ",", 3)) {
	sprintf(err, "%s: couldn't parse 3 floats from \"%s\"", me, argv[a]);
	biffSet(key, err); return 1;
      }
    }
    else if (!strcmp("-d", argv[a])) {
      miss = !argv[a+1]; if (miss) break;
      if (3 != airParseStrI(dim, argv[++a], ",", 3)) {
	sprintf(err, "%s: couldn't parse 3 ints from \"%s\"", me, argv[a]);
	biffSet(key, err); return 1;
      }
      hvp->ax[0].res = dim[perm[0]];
      hvp->ax[1].res = dim[perm[1]];
      hvp->ax[2].res = dim[perm[2]];
    }
    else if (!strcmp("-h", argv[a])) {
      miss = !argv[a+1]; if (miss) break;
      a++;
      if (!strcmp("lapl", argv[a])) {
	hvp->ax[1].measr = baneMeasrLapl;
      }
      else if (!strcmp("hess", argv[a])) {
	hvp->ax[1].measr = baneMeasrHess;
      }
      /*
      else if (!strcmp("gmg", argv[a])) {
	hvp->ax[1].measr = baneMeasrGMG_cd;
      }
      */
      else {
	sprintf(err, "%s: second derivative measure \"%s\" unknown", 
		me, argv[a]);
	return 1;
      }
    }
    else if (!inName) {
      inName = argv[a];
    }
    else if (!*outNameP) {
      *outNameP = argv[a];
    }
    else {
      sprintf(err, "%s: command-line option \"%s\" unknown", me, argv[a]);
      biffSet(key, err); return 1;
    }
    a++;
  }
  if (miss) {
    sprintf(err, "%s: didn't get argument(s) for option %s", me, argv[a]);
    biffSet(key, err); return 1;
  }

  /* figure out inclusion/parameter stuff */
  for (i=0; i<=2; i++) {
    switch (incs[perm[i]]) {
    case 'f':
      hvp->ax[i].inc = baneIncRangeRatio;
      hvp->ax[i].incParm[0] = AIR_EXISTS(incp[perm[i]]) ? incp[perm[i]] : 1.0;
      break;
    case 'p':
      hvp->ax[i].inc = baneIncPercentile;
      hvp->ax[i].incParm[0] = 1024;
      hvp->ax[i].incParm[1] = (AIR_EXISTS(incp[perm[i]]) 
				? incp[perm[i]] 
				: 0.15);
      break;
    case 'a':
      if (2 == i) {
	sprintf(err, "%s: sorry, absolute inclusion specification not supported for data value", me);
	biffSet(key, err); return 1;
      }
      if (!AIR_EXISTS(incp[perm[i]])) {
	sprintf(err, "%s: need to give parameter for absolute inclusion specification", me);
	biffSet(key, err); return 1;
      }
      hvp->ax[i].inc = baneIncAbsolute;
      hvp->ax[i].incParm[0] = i == 0 ? 0 : -incp[perm[i]];
      hvp->ax[i].incParm[1] = incp[perm[i]];
      break;
    case 's':
      hvp->ax[i].inc = baneIncStdv;
      hvp->ax[i].incParm[0] = (AIR_EXISTS(incp[perm[i]]) 
				? incp[perm[i]] 
				: 70);
      break;
    default:
      sprintf(err, "%s: inclusion strategy \"%c\" for axis %d unknown", 
	      me, incs[perm[i]], perm[i]);
      biffSet(key, err); return 1;
      break;
    }
    /* printf("%s: HEY axp[%d].inc = %d\n", me, i, hvp->ax[i].inc); */
  }
  
  if (!(*outNameP)) {
    sprintf(err, "%s: didn't get output file name", me);
    biffSet(key, err); return 1;
  }
    
  /* can we actually read the input? */
  if (!inName) {
    sprintf(err, "%s: didn't get input file name", me);
    biffSet(key, err); return 1;
  }
  if (nrrdLoad(*ninP=nrrdNew(), inName)) {
    sprintf(err, "%s: trouble loading input nrrd \"%s\"", me, inName);
    biffMove(key, err, NRRD); return 1;
  }

  return 0;
}

int
doHVol(int argc, char *argv[]) {
  Nrrd *nin, *nout;
  baneHVolParm *hvp;
  char me[]="doHVol", err[512], *outName = NULL;

  if (!(3 <= argc))
    return usageHVol();
  
  hvp = baneHVolParmNew();
  if (parseHVol(&nin, &outName, hvp, argc, argv)) {
    sprintf(err, "%s: trouble parsing command-line arguments", me);
    biffAdd(key, err); return 2;
  }
  nout = nrrdNew();
  if (baneMakeHVol(nout, nin, hvp)) {
    sprintf(err, "%s: trouble making histogram volume", me);
    biffMove(key, err, BANE); return 1;
  }
  if (nrrdSave(outName, nout, NULL)) {
    sprintf(err, "%s: trouble saving histogram volume", me);
    biffMove(key, err, NRRD); return 1;
  }
  nrrdNuke(nin);
  nrrdNuke(nout);
  hvp = baneHVolParmNix(hvp);
  return 0;
}



/* ------------------------------------------------------ */
/* ----------------------- SCAT ------------------------- */
/* ------------------------------------------------------ */

#define DEF_GAMMA 1.4

int
usageScat(void) {
  fprintf(stderr, "usage: %s scat [-g <gamma>] <hvolIn> <VGout> <VHout>\n", 
	  me);
  fprintf(stderr, "Default values are in {}s\n");
  fprintf(stderr, "  -g gamma : gamma used to brighten scatterplot\n");
  fprintf(stderr, "             (gamma > 1 makes image brighter; gamma < 0 inverts everthing\n");
  fprintf(stderr, "             {%g}\n", DEF_GAMMA);
  fprintf(stderr, "    hvolIn : histogram volume being analyzed\n");
  fprintf(stderr, "VGout VHout: the value-gradient and value-2ndDerivative scatterplot\n");
  fprintf(stderr, "             images to be created (PGM format)\n");
  return 1;
}

int
parseScat(Nrrd **ninP, char **outVGNameP, char **outVHNameP, float *gammaP,
	  int argc, char *argv[]) {
  char me[]="parseScat", err[512], *inName;
  int a, miss=0;

  inName = *outVGNameP = *outVHNameP = NULL;
  *gammaP = DEF_GAMMA;
  
  a = 2;
  while (argv[a]) {
    miss = 0;
    if (!strcmp("-g", argv[a])) {
      miss = !argv[a+1]; if (miss) break;
      if (1 != sscanf(argv[++a], "%f", gammaP)) {
	sprintf(err, "%s: couldn't parse \"%s\" as float", me, argv[a]);
	biffSet(key, err); return 1;
      }
    }
    else if (!inName) {
      inName = argv[a];
    }
    else if (!*outVGNameP) {
      *outVGNameP = argv[a];
    }
    else if (!*outVHNameP) {
      *outVHNameP = argv[a];
    }
    else {
      sprintf(err, "%s: command-line option \"%s\" unknown", me, argv[a]);
      biffSet(key, err); return 1;
    }
    a++;
  }
  if (miss) {
    sprintf(err, "%s: didn't get argument(s) for option %s", me, argv[a]);
    biffSet(key, err); return 1;
  }

  if (!(*gammaP)) {
    sprintf(err, "%s: need a non-zero gamma", me);
    biffSet(key, err); return 1;
  }
  if (!inName) {
    sprintf(err, "%s: didn't get input file name", me);
    biffSet(key, err); return 1;
  }
  if (!(*outVGNameP) && !(*outVHNameP)) {
    sprintf(err, "%s: didn't get 2 output fle names", me);
    biffSet(key, err); return 1;
  }

  if (nrrdLoad(*ninP=nrrdNew(), inName)) {
    sprintf(err, "%s: couldn't open input nrrd \"%s\"", me, inName);
    biffMove(key, err, NRRD); return 1;
  }

  return 0;
}

int
doScat(int argc, char *argv[]) {
  Nrrd *hvol, *nvgRaw, *nvhRaw, *nvg=NULL, *nvh=NULL;
  char me[]="doScat", err[512], *outVGName, *outVHName;
  float gamma;
  int E;
  
  if (!(3 <= argc))
    return usageScat();

  if (parseScat(&hvol, &outVGName, &outVHName, &gamma, argc, argv)) {
    sprintf(err, "%s: trouble parsing command-line arguments", me);
    biffAdd(key, err); return 2;
  }
  if (!baneValidHVol(hvol)) {
    sprintf(err, "%s: didn't seem to get a histogram volume", me);
    biffMove(key, err, BANE); return 1;
  }
  E = 0;

  if (baneRawScatterplots(nvgRaw = nrrdNew(), nvhRaw = nrrdNew(), 
			  hvol, AIR_TRUE)) {
    sprintf(err, "%s: trouble creating raw scatterplots", me);
    biffMove(key, err, BANE); return 1;
  }

  nrrdMinMaxSet(nvgRaw);
  if (!E) E |= nrrdArithGamma(nvgRaw, nvgRaw, gamma, nvgRaw->min, nvgRaw->max);
  nrrdMinMaxSet(nvhRaw);
  if (!E) E |= nrrdArithGamma(nvhRaw, nvhRaw, gamma, nvhRaw->min, nvhRaw->max);
  if (E) {
    sprintf(err, "%s: trouble doing gamma %g", me, gamma);
    biffMove(key, err, NRRD); return 1;
  }

  /* create 8-bit versions */
  if (!E) E |= nrrdQuantize(nvg = nrrdNew(), nvgRaw, 8);
  if (!E) E |= nrrdQuantize(nvh = nrrdNew(), nvhRaw, 8);
  if (E) {
    sprintf(err, "%s: couldn't create 8-bit scatterplots", me);
    biffMove(key, err, NRRD); return 1;
  }

  /* save */
  if (!E) E |= nrrdSave(outVGName, nvg, NULL);
  if (!E) E |= nrrdSave(outVHName, nvh, NULL);
  if (E) {
    sprintf(err, "%s: trouble saving scatterplot images", me);
    biffMove(key, err, NRRD); return 1;
  }

  nrrdNuke(hvol);
  nrrdNuke(nvgRaw);
  nrrdNuke(nvhRaw);
  nrrdNuke(nvg);
  nrrdNuke(nvh);
  return 0;
}



/* ------------------------------------------------------ */
/* ----------------------- INFO ------------------------- */
/* ------------------------------------------------------ */

int
usageInfo(void) {
  fprintf(stderr, "usage: %s info [-d 1|2] [-m <measr>] <input> <output>\n", 
	  me);
  fprintf(stderr, "Default values are in {}s\n");
  fprintf(stderr, "    -d 1|2 : dimension of histo-info to be created\n");
  fprintf(stderr, "             {1}\n");
  fprintf(stderr, "  -m measr : how to collapse axes of the histogram volume\n");
  fprintf(stderr, "             \"mean\": average (as done in GK's published research)\n");
  fprintf(stderr, "             \"median\": value at 50%% percentile\n");
  fprintf(stderr, "             \"mode\": most common value\n");
  fprintf(stderr, "             {mean}\n");
  fprintf(stderr, "     input : histogram volume being analyzed\n");
  fprintf(stderr, "    output : histo-info to be created\n");
  return 1;
}

int
parseInfo(Nrrd **ninP, char **outNameP, int *dimP, int *measrP,
	  int argc, char *argv[]) {
  char me[]="parseInfo", err[512], *inName;
  int a, miss=0, M;

  a = 2;
  *dimP = 1;
  *measrP = nrrdMeasureHistoMean;

  inName = *outNameP = NULL;
  while (argv[a]) {
    miss = 0;
    if (!strcmp("-d", argv[a])) {
      miss = !argv[a+1]; if (miss) break;
      if (1 != sscanf(argv[++a], "%d", dimP)) {
	sprintf(err, "%s: couldn't parse \"%s\" as int", me, argv[a]);
	biffSet(key, err); return 1;
      }
      if (!(*dimP == 1 || *dimP == 2)) {
	sprintf(err, "%s: can only do dimension 1 or 2 (not %d)", me, *dimP);
	biffSet(key, err); return 1;
      }
    }
    else if (!strcmp("-m", argv[a])) {
      miss = !argv[a+1]; if (miss) break;
      ++a;
      M = 0;
      /*
      if (!M) { if (!strcmp(argv[a], "min")) M = nrrdMeasureHistoMin; }
      if (!M) { if (!strcmp(argv[a], "max")) M = nrrdMeasureHistoMax; }
      */
      if (!M) { if (!strcmp(argv[a], "mean")) M = nrrdMeasureHistoMean; }
      if (!M) { if (!strcmp(argv[a], "median")) M = nrrdMeasureHistoMedian; }
      if (!M) { if (!strcmp(argv[a], "mode")) M = nrrdMeasureHistoMode; }
      if (!M) {
	sprintf(err, "%s: didn't recognize measurement \"%s\"", me, argv[a]);
	biffSet(key, err); return 1;
      }
      *measrP = M;
    }
    else if (!inName) {
      inName = argv[a];
    }
    else if (!*outNameP) {
      *outNameP = argv[a];
    }
    else {
      sprintf(err, "%s: command-line option \"%s\" unknown", me, argv[a]);
      biffSet(key, err); return 1;
    }
    a++;
  }
  if (miss) {
    sprintf(err, "%s: didn't get argument(s) for option %s", me, argv[a]);
    biffSet(key, err); return 1;
  }
  if (!inName) {
    sprintf(err, "%s: didn't get input file name", me);
    biffSet(key, err); return 1;
  }
  if (nrrdLoad(*ninP=nrrdNew(), inName)) {
    sprintf(err, "%s: trouble loading histogram volume \"%s\"", me, inName);
    biffMove(key, err, NRRD); return 1;
  }
  if (!*outNameP) {
    sprintf(err, "%s: didn't get output fle name", me);
    biffSet(key, err); return 1;
  }
  return 0;
}

int
doInfo(int argc, char *argv[]) {
  char me[]="doInfo", err[512], *outName;
  Nrrd *hvol, *info;
  int dim, measr;

  if (!(3 <= argc))
    return usageInfo();
  
  if (parseInfo(&hvol, &outName, &dim, &measr, argc, argv)) {
    sprintf(err, "%s: trouble parsing command line\n", me);
    biffAdd(key, err); return 2;
  }
  if (baneOpacInfo(info = nrrdNew(), hvol, dim, measr)) {
    sprintf(err, "%s: trouble calculating %dD histo-info", me, dim);
    biffMove(key, err, BANE); return 1;
  }
  if (nrrdSave(outName, info, NULL)) {
    sprintf(err, "%s: trouble saving %dD histo-info", me, dim);
    biffMove(key, err, NRRD); return 1;
  }
  
  return 0;
}


/* ------------------------------------------------------ */
/* ----------------------- PVG -------------------------- */
/* ------------------------------------------------------ */

#define DEF_GTHR_SCALE 0.04
#define DEF_BR 0
#define DEF_BG 0
#define DEF_BB 0
#define DEF_ZR 255
#define DEF_ZG 255
#define DEF_ZB 255
#define PVG_HISTEQ_BINS 2048

unsigned char don[] = {DEF_BR, DEF_BG, DEF_BB, /* (0) background */
		       0, 107, 255,   /* (1) start: blue */
		       51, 104, 255,
		       103, 117, 255,
		       123, 124, 255,
		       141, 130, 255,
		       156, 132, 255,
		       166, 131, 245,
		       174, 131, 231,
		       181, 130, 216,
		       187, 130, 201,
		       DEF_ZR, DEF_ZR, DEF_ZR, /* (11) middle */
		       DEF_ZR, DEF_ZR, DEF_ZR, /* (12) middle */
		       187, 130, 201,
		       192, 129, 186,
		       197, 128, 172,
		       200, 128, 158,
		       204, 127, 142,
		       210, 126, 113,
		       212, 126, 98,
		       213, 126, 84,
		       216, 126, 49,
		       220, 133, 0};  /* (22) end: orange */


int
usagePvg(void) {
  fprintf(stderr, "usage: %s pvg [-s <sigma>] [-g <gthresh>] [-b r,g,b]\n",me);
  fprintf(stderr, "       [-z r,g,b] <input> <output>\n");
  fprintf(stderr, "Default values are in {}s\n");
  fprintf(stderr, "  -s sigma : scaling in position calculation\n");
  fprintf(stderr, "             {(automatically calculated)}\n");
  fprintf(stderr, "-g gthresh : minimum significant gradient mag\n");
  fprintf(stderr, "             {%g * maxgrad}\n", DEF_GTHR_SCALE);
  fprintf(stderr, "  -b r,g,b : background color of p(v,g) image\n");
  fprintf(stderr, "             {%d,%d,%d}\n", DEF_BR, DEF_BG, DEF_BB);
  fprintf(stderr, "  -z r,g,b : color of zero in p(v,g) image\n");
  fprintf(stderr, "             {%d,%d,%d}\n", DEF_ZR, DEF_ZG, DEF_ZB);
  fprintf(stderr, "     input : 2D histo-info being analyzed\n");
  fprintf(stderr, "    output : PPM to be created\n");
  return 1;
}

int
parsePvg(Nrrd **ninP, char **outNameP, float *sigmaP, float *gthreshP,
	 int argc, char *argv[]) {
  char me[]="parsePvg", err[512], *inName;
  int a, rgb[3], miss=0;

  if (!(4 <= argc)) {
    return usagePvg();
  }
  inName = *outNameP = NULL;
  *sigmaP = *gthreshP = AIR_NAN;

  a = 2;
  while (argv[a]) {
    miss = 0;
    if (!strcmp("-s", argv[a])) {
      miss = !argv[a+1]; if (miss) break;
      if (1 != sscanf(argv[++a], "%f", sigmaP)) {
	sprintf(err, "%s: couldn't parse sigma \"%s\" as float", me, argv[a]);
	biffSet(key, err); return 1;
      }
    }
    else if (!strcmp("-g", argv[a])) {
      miss = !argv[a+1]; if (miss) break;
      if (1 != sscanf(argv[++a], "%f", gthreshP)) {
	sprintf(err, "%s: couldn't parse gthresh \"%s\" as float", 
		me, argv[a]);
	biffSet(key, err); return 1;
      }
    }
    else if (!strcmp("-b", argv[a])) {
      miss = !argv[a+1]; if (miss) break;
      if (3 != airParseStrI(rgb, argv[++a], ",", 3)) {
	sprintf(err, "%s: couldn't parse 3 ints from background \"%s\"",
		me, argv[a]);
	biffSet(key, err); return 1;
      }
      don[0] = rgb[0]; don[1] = rgb[1]; don[2] = rgb[2];
    }
    else if (!strcmp("-z", argv[a])) {
      miss = !argv[a+1]; if (miss) break;
      if (3 != airParseStrI(rgb, argv[++a], ",", 3)) {
	sprintf(err, "%s: couldn't parse 3 ints from background \"%s\"",
		me, argv[a]);
	biffSet(key, err); return 1;
      }
      don[0+3*11] = don[0+3*12] = rgb[0];
      don[1+3*11] = don[1+3*12] = rgb[1];
      don[2+3*11] = don[2+3*12] = rgb[2];
    }
    else if (!inName) {
      inName = argv[a];
    }
    else if (!*outNameP) {
      *outNameP = argv[a];
    }
    else {
      sprintf(err, "%s: don't understand option \"%s\"", me, argv[a]);
      biffSet(key, err); return 1;
    }
    a++;
  }
  if (miss) {
    sprintf(err, "%s: didn't get argument(s) for option %s", me, argv[a]);
    biffSet(key, err); return 1;
  }

  if (nrrdLoad(*ninP=nrrdNew(), inName)) {
    sprintf(err, "%s: trouble loading 2D histo-info \"%s\"", me, inName);
    biffMove(key, err, NRRD); return 1;
  }
  if (!baneValidInfo(*ninP, 2)) {
    sprintf(err, "%s: didn't get valid 2D histo-info", me);
    biffMove(key, err, BANE); return 1;
  }
  if (!AIR_EXISTS(*sigmaP)) {
    if (baneSigmaCalc(sigmaP, *ninP)) {
      sprintf(err, "%s: trouble calculating sigma", me);
      biffMove(key, err, NRRD); return 1;
    }
    fprintf(stderr, "%s: calculated sigma: %g\n", me, *sigmaP);
  }
  if (!AIR_EXISTS(*gthreshP)) {
    *gthreshP = DEF_GTHR_SCALE*(*ninP)->axis[2].max;
    fprintf(stderr, "%s: calculated gthresh: %g\n", me, *gthreshP);
  }

  return 0;
}

void
pvgMap(unsigned char *rgb, float min, float sml, float max, float p) {
  float cf, cw;
  int ci;

  if (!AIR_EXISTS(p)) {
    rgb[0] = don[0];
    rgb[1] = don[1];
    rgb[2] = don[2];
    return;
  }
  if (p > sml) {
    cf = AIR_AFFINE(sml, p, max, 11.5, 21.999);
  }
  else {
    cf = AIR_AFFINE(min, p, sml, 1, 11.5);
  }
  ci = cf;
  cw = cf - ci;
  rgb[0] = AIR_AFFINE(0, cw, 1, don[0 + 3*ci], don[0 + 3*(ci+1)]);
  rgb[1] = AIR_AFFINE(0, cw, 1, don[1 + 3*ci], don[1 + 3*(ci+1)]);
  rgb[2] = AIR_AFFINE(0, cw, 1, don[2 + 3*ci], don[2 + 3*(ci+1)]);
}

int
doPvg(int argc, char *argv[]) {
  Nrrd *info, *nposA, *nposB, *nppm;
  char me[]="doPvg", err[512], *outName;
  float sigma, gthresh, p, *pos, min, max, sml=0;
  int i, v, g, sv, sg, smlI=0;
  unsigned char *ppm, *rgb;
  
  if (parsePvg(&info, &outName, &sigma, &gthresh, argc, argv)) {
    sprintf(err, "%s: trouble parsing command line", me);
    biffAdd(key, err); return 2;
  }
  if (banePosCalc(nposA=nrrdNew(), sigma, gthresh, info)) {
    sprintf(err, "%s: trouble doing position calculation", me);
    biffMove(key, err, BANE); return 1;
  }
  sv = nposA->axis[0].size;
  sg = nposA->axis[1].size;
  pos = nposA->data;

  /* find min, max, sml, smlI */
  min = max = AIR_NAN;
  for (i=0; i<=nrrdElementNumber(nposA)-1; i++) {
    p = pos[i];
    if (!AIR_EXISTS(p))
      continue;
    if (!AIR_EXISTS(min)) {
      min = max = p;
      sml = AIR_ABS(p);
    }
    min = AIR_MIN(p, min);
    max = AIR_MAX(p, max);
    if (AIR_ABS(p) < sml) {
      sml = AIR_ABS(p);
      smlI = i;
    }
  }
  if (!AIR_EXISTS(min)) {
    sprintf(err, "%s: didn't see any real data in position array", me);
    biffSet(key, err); return 1;
  }
  /*
  printf("%s: pos range: [%g,%g(%d),%g]\n", 
	 me, min, pos[smlI], smlI, max);
  */
  if (nrrdHistoEq(nposB=nrrdNew(), nposA, NULL, PVG_HISTEQ_BINS, 3)) {
    sprintf(err, "%s: trouble doing hist-eq on p(v,g)", me);
    biffMove(key, err, NRRD); return 1;
  }
  if (nrrdPPM(nppm=nrrdNew(), sv, sg)) {
    sprintf(err, "%s: couldn't create %dx%d PPM", me, sv, sg);
    biffMove(key, err, NRRD); return 1;
  }
  ppm = nppm->data;
  /* this is why we had to find sml: to keep track of where it shifted to
     as a result of histogram equalization */
  pos = nposB->data;
  sml = pos[smlI];
  for (g=0; g<=sg-1; g++) {
    for (v=0; v<=sv-1; v++) {
      i = v + sv*g;
      rgb = ppm + 3*(v + sv*(sg-1-g));
      p = pos[i];
      pvgMap(rgb, min, sml, max, p);
    }
  }
  if (nrrdSave(outName, nppm, NULL)) {
    sprintf(err, "%s: trouble saving output to \"%s\"", me, outName);
    biffMove(key, err, NRRD); return 1;
  }

  nrrdNuke(info);
  nrrdNuke(nposA);
  nrrdNuke(nposB);
  nrrdNuke(nppm);
  return 0;
}

/* ------------------------------------------------------ */
/* ----------------------- OPAC ------------------------- */
/* ------------------------------------------------------ */

#define DEF_C 0.0
#define DEF_W 1.0
#define DEF_S 1.0
#define DEF_A 1.0
#define DEF_MEDRAD 0

int
usageOpac(void) {
  fprintf(stderr, "usage: %s [-b <bemph>] [-f c,w,s,a] [-s <sigma>]\n", me);
  fprintf(stderr, "       [-g <gthresh>] [-m <radius>] <input> <output>\n");
  fprintf(stderr, "Default values are in {}s\n");
  fprintf(stderr, "  -b bemph : nrrd containing data for boundary emphasis function b(x)\n");
  fprintf(stderr, "             (no default)\n");
  fprintf(stderr, "-f c,w,s,a : alternate way of specifying b(x)\n");
  fprintf(stderr, "             c: where to center support\n");
  fprintf(stderr, "             w: full-width half-max of support\n");
  fprintf(stderr, "             s: shape of support (0.0: box, 1.0: tent)\n");
  fprintf(stderr, "             a: height of function (max opacity)\n");
  fprintf(stderr, "             {%g,%g,%g,%g}\n", DEF_C, DEF_W, DEF_S, DEF_A);
  fprintf(stderr, "  -s sigma : scaling in position calculation\n");
  fprintf(stderr, "             larger values ---> narrower opacity functions\n");
  fprintf(stderr, "             {(automatically calculated)}\n");
  fprintf(stderr, "-g gthresh : minimum significant gradient mag\n");
  fprintf(stderr, "             {%g * maxgrad}\n", DEF_GTHR_SCALE);
  fprintf(stderr, " -m radius : radius of median filtering, 0 for none\n");
  fprintf(stderr, "             {%d}\n", DEF_MEDRAD);
  fprintf(stderr, "     input : 1D or 2D histo-info being analyzed\n");
  fprintf(stderr, "    output : opacity function to be created\n");
  return 1;
}

int
parseOpac(Nrrd **infoP, Nrrd **nbP, int *medradP, 
	  float *sigmaP, float *gthreshP, char **outNameP,
	  int argc, char *argv[]) {
  Nrrd *nmax;
  char me[]="parseOpac", err[512], *nbName, *inName;
  float cwsa[4], *b, off;
  int a, dim, miss=0;
  
  if (!(4 <= argc)) {
    return usageOpac();
  }

  *nbP = NULL;
  cwsa[0] = AIR_NAN;
  *sigmaP = *gthreshP = AIR_NAN;
  inName = nbName = *outNameP = NULL;
  *medradP = DEF_MEDRAD;
  a = 2;
  while (argv[a]) {
    miss = 0;
    if (!strcmp("-b", argv[a])) {
      miss = !argv[a+1]; if (miss) break;
      nbName = argv[++a];
      cwsa[0] = AIR_NAN;
    }
    else if (!strcmp("-f", argv[a])) {
      miss = !argv[a+1]; if (miss) break;
      if (4 != airParseStrF(cwsa, argv[++a], ",", 4)) {
	sprintf(err, "%s: couldn't parse 3 floats from \"%s\"", me, argv[a]);
	biffSet(key, err); return 1;
      }
      nbName = NULL;
    }
    else if (!strcmp("-s", argv[a])) {
      miss = !argv[a+1]; if (miss) break;
      if (1 != sscanf(argv[++a], "%f", sigmaP)) {
	sprintf(err, "%s: couldn't parse sigma \"%s\" as float", me, argv[a]);
	biffSet(key, err); return 1;
      }
    }
    else if (!strcmp("-g", argv[a])) {
      miss = !argv[a+1]; if (miss) break;
      if (1 != sscanf(argv[++a], "%f", gthreshP)) {
	sprintf(err, "%s: couldn't parse gthresh \"%s\" as float", 
		me, argv[a]);
	biffSet(key, err); return 1;
      }
    }
    else if (!strcmp("-m", argv[a])) {
      miss = !argv[a+1]; if (miss) break;
      if (1 != sscanf(argv[++a], "%d", medradP)) {
	sprintf(err, "%s: couldn't parse median radius \"%s\" as int",
		me, argv[a]);
	biffSet(key, err); return 1;
      }
    }
    else if (!inName) {
      inName = argv[a];
    }
    else if (!*outNameP) {
      *outNameP = argv[a];
    }
    else {
      sprintf(err, "%s: command-line option \"%s\" unknown", me, argv[a]);
      biffSet(key, err); return 1;
    }
    ++a;
  }
  if (miss) {
    sprintf(err, "%s: didn't get argument(s) for option %s", me, argv[a]);
    biffSet(key, err); return 1;
  }
  if (nbName) {
    if (nrrdLoad(*nbP=nrrdNew(), nbName)) {
      sprintf(err, "%s: couldn't open b(x) nrrd \"%s\"", me, nbName);
      biffMove(key, err, NRRD); return 1;
    }
  }
  else {
    if (!AIR_EXISTS(cwsa[0])) {
      cwsa[0] = DEF_C;
      cwsa[1] = DEF_W;
      cwsa[2] = DEF_S;
      cwsa[3] = DEF_A;
    }
    if (nrrdAlloc(*nbP=nrrdNew(), nrrdTypeFloat, 2, 2, 4)) {
      sprintf(err, "%s: trouble creating b(x) nrrd", me);
      biffMove(key, err, NRRD); return 1;
    }
    b = (*nbP)->data;
    off = AIR_AFFINE(0.0, cwsa[2], 1.0, 0.0, cwsa[1]/2);
    b[0 + 2*0] = cwsa[0] - cwsa[1]/2 - off;
    b[0 + 2*1] = cwsa[0] - cwsa[1]/2 + off;
    b[0 + 2*2] = cwsa[0] + cwsa[1]/2 - off;
    b[0 + 2*3] = cwsa[0] + cwsa[1]/2 + off;
    b[1 + 2*0] = 0.0;
    b[1 + 2*1] = cwsa[3];
    b[1 + 2*2] = cwsa[3];
    b[1 + 2*3] = 0.0;
  }

  if (!inName) {
    sprintf(err, "%s: didn't get input 2D histo-info file name", me);
    biffSet(key, err); return 1;
  }
  else if (nrrdLoad(*infoP=nrrdNew(), inName)) {
    sprintf(err, "%s: couldn't open input histo-info nrrd \"%s\"", 
	    me, inName);
    biffMove(key, err, NRRD); return 1;
  }
  dim = baneValidInfo(*infoP, AIR_FALSE);
  if (!dim) {
    sprintf(err, "%s: didn't get a valid histo-info", me);
    biffMove(key, err, BANE); return 1;
  }
  if (!*outNameP) {
    sprintf(err, "%s: didn't gout output opacity function file name", me);
    biffSet(key, err); return 1;
  }
  if (!AIR_EXISTS(*sigmaP)) {
    if (baneSigmaCalc(sigmaP, *infoP)) {
      sprintf(err, "%s: trouble calculating sigma", me);
      biffMove(key, err, NRRD); return 1;
    }
    fprintf(stderr, "%s: calculated sigma: %g\n", me, *sigmaP);
  }
  if (!AIR_EXISTS(*gthreshP)) {
    if (2 == dim) {
      *gthreshP = DEF_GTHR_SCALE*(*infoP)->axis[2].max;
    }
    else {
      if (nrrdProject(nmax = nrrdNew(), *infoP, 1, nrrdMeasureMax)) {
	sprintf(err, "%s: couldn't do max projection of 1D histo-info", me);
	biffMove(key, err, NRRD); return 1;
      }
      *gthreshP = nrrdFLookup[nmax->type](nmax->data, 0);
      *gthreshP *= DEF_GTHR_SCALE;
      nrrdNuke(nmax);
    }
    fprintf(stderr, "%s: calculated gthresh: %g\n", me, *gthreshP);
  }

  return 0;
}

int
doOpac(int argc, char *argv[]) {
  Nrrd *nb=NULL, *npos=NULL, *ninfo=NULL, *nopac, *nmflt;
  float sigma, gthresh;
  char me[]="doOpac", err[512], *outName;
  int medrad;
  
  if (parseOpac(&ninfo, &nb, &medrad, &sigma, &gthresh, &outName, 
		argc, argv)) {
    sprintf(err, "%s: trouble parsing command line", me);
    biffAdd(key, err); return 2;
  }
  if (banePosCalc(npos = nrrdNew(), sigma, gthresh, ninfo)) {
    sprintf(err, "%s: couldn't calculate position", me);
    biffMove(key, err, BANE); return 1;
  }
  if (baneOpacCalc(nopac = nrrdNew(), nb, npos)) {
    sprintf(err, "%s: couldn't calculate opacity", me);
    biffMove(key, err, BANE); return 1;
  }
  if (medrad) {
    if (nrrdCheapMedian(nmflt=nrrdNew(), nopac, medrad, 1.0, 2048)) {
      sprintf(err, "%s: trouble median filtering", me);
      biffMove(key, err, NRRD); return 1;
    }
    nrrdNuke(nopac);
    nopac = nmflt;
  }
  if (nrrdSave(outName, nopac, NULL)) {
    sprintf(err, "%s: trouble saving opacity", me);
    biffMove(key, err, NRRD); return 1;
  }
  nrrdNuke(ninfo);
  nrrdNuke(npos);
  nrrdNuke(nb);
  return 0;
}

/* ------------------------------------------------------ */
/* ------------------------------------------------------ */
/* ------------------------------------------------------ */

int
usage(void) {
  fprintf(stderr, "usage: %s <command> ...\n", me);
  fprintf(stderr, "command is one of:\n");
  fprintf(stderr, "   hvol : make histogram volume\n");
  fprintf(stderr, "   scat : make v-g and v-h scatterplots\n");
  fprintf(stderr, "   info : make projections needed for opacity function generation\n");
  fprintf(stderr, "    pvg : make colormapped plots of p(v,g)\n");
  fprintf(stderr, "   opac : make opacity functions\n");
  fprintf(stderr, "Usage information can be seen by running just \"%s <command>\"\n", me);
  fprintf(stderr, "For all commands, input and output filenames can be \"-\" to signify stdin\n");
  fprintf(stderr, "and stdout (respectively), where unambiguous.\n");
  return 1;
}

int
main(int argc, char *argv[]) {
  char *cmdS, *err;
  int ret;
  int (*doit)(int argc, char *argv[]);
  int (*useit)(void);

  me = argv[0];
  if (!(2 <= argc))
    return usage();
  cmdS = argv[1];
  
  if (!strcmp("hvol", cmdS)) {
    doit = doHVol;
    useit = usageHVol;
  }
  else if (!strcmp("scat", cmdS)) {
    doit = doScat;
    useit = usageScat;
  }
  else if (!strcmp("info", cmdS)) {
    doit = doInfo;
    useit = usageInfo;
  }
  else if (!strcmp("pvg", cmdS)) {
    doit = doPvg;
    useit = usagePvg;
  }
  else if (!strcmp("opac", cmdS)) {
    doit = doOpac;
    useit = usageOpac;
  }
  else {
    fprintf(stderr, "%s: command \"%s\" not recognized.\n", me, cmdS);
    return usage();
  }
  ret = doit(argc, argv);
  if (ret) {
    if (biffCheck(key)) {
      fprintf(stderr, "%s: error:\n%s", me, err=biffGet(key));
      free(err);
    }
    if (2 == ret) {
      fprintf(stderr, "\n");
      return useit();
    }
    else {
      return 1;
    }
  }

  return 0;
}
