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


#include "bane.h"

int baneProbeDebug = 0;

/*
  baneProbeVal,             0: data value (*float)
  baneProbeGradVec,         1: gradient vector, un-normalized (float[3])
  baneProbeGradMag,         2: gradient magnitude (*float)
  baneProbeNormal,          3: gradient vector, normalized (float[3])
  baneProbeHess,            4: Hessian (float[9])
  baneProbeHessEval,        5: Hessian's eigenvalues (float[3])
  baneProbeHessEvec,        6: Hessian's eigenvectors (float[9])
  baneProbe2ndDD,           7: 2nd dir.deriv along gradient (*float)
  baneProbeCurvVecs,        8: principle curvature directions (float[6])
  baneProbeK1K2,            9: principle curvature magnitudes (float[2])
  baneProbeShapeIndex,      10: Koenderink's shape index, (his "S")
  baneProbeCurvedness       11: L2 norm of K1, K2 (not Koen.'s "C")

 #define BANE_PROBE_VAL        (1<<0)
 #define BANE_PROBE_GRADVEC    (1<<1)
 #define BANE_PROBE_GRADMAG    (1<<2)
 #define BANE_PROBE_NORMAL     (1<<3)
 #define BANE_PROBE_HESS       (1<<4)
 #define BANE_PROBE_HESSEVAL   (1<<5)
 #define BANE_PROBE_HESSEVEC   (1<<6)
 #define BANE_PROBE_2NDDD      (1<<7)
 #define BANE_PROBE_CURVVECS   (1<<8)
 #define BANE_PROBE_K1K2       (1<<9)
 #define BANE_PROBE_SHAPEINDEX (1<<10)
 #define BANE_PROBE_CURVEDNESS (1<<11)
*/

int
baneProbeAnsLen[BANE_PROBE_MAX+1] = {
  1, 3, 1, 3, 9,  3,  9,  1,  6,  2,  1,  1
};

int
baneProbeAnsOffset[BANE_PROBE_MAX+1] = {
  0, 1, 4, 5, 8, 17, 20, 29, 30, 36, 38, 39
};

int
baneProbePrereq[BANE_PROBE_MAX+1] = {
  0,
  0,
  BANE_PROBE_GRADVEC,
  BANE_PROBE_GRADVEC | BANE_PROBE_GRADMAG,
  0,
  BANE_PROBE_HESS,
  BANE_PROBE_HESS | BANE_PROBE_HESSEVAL,
  BANE_PROBE_GRADVEC | BANE_PROBE_GRADMAG | BANE_PROBE_NORMAL| BANE_PROBE_HESS,
  BANE_PROBE_GRADVEC | BANE_PROBE_GRADMAG | BANE_PROBE_HESS,
  BANE_PROBE_GRADVEC | BANE_PROBE_GRADMAG | BANE_PROBE_HESS,
  BANE_PROBE_GRADVEC | BANE_PROBE_GRADMAG | BANE_PROBE_HESS,
  BANE_PROBE_GRADVEC | BANE_PROBE_GRADMAG | BANE_PROBE_HESS
};

int
_baneProbeDeriv[BANE_PROBE_MAX+1] = {
  0, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2
};

#define DITCH(ptr) if ((ptr)) free((ptr)), (ptr)=NULL

#define _baneZeroNudge(g) (g < 0.0001 ? 0 : g)

#define _baneOneClamp(g) (g < 1 ? g : 1)

  
/*
******** baneProbe()
**
** to query derived quantities at various non-integral coordinates
** in three-dimensional volumes
**
** employs some tricks with static vars to try to be fast, but these
** can also produce unexpected results if not understood:
** ...
**
*/
void
baneProbe(float *_ans, Nrrd *nin, int query,
	  baneProbeK3Pack *pack,
	  float x, float y, float z) {
  char me[]="baneProbe";
  int i, j, k,                  /* swiss-army ints */
    tmpI,                       /* a temporary integer */
    xi, yi, zi;                 /* indices of current voxels lowest corner */
  float tmpF, tmpG, xf, yf, zf;

  static float *ans=NULL,       /* last value of _ans, also the thing which
				   controls caching of values */
    *val=NULL,                  /* value cache */
    *fi2=NULL, *fi1=NULL,     /* filtering result cache */
    *fsl=NULL,                  /* filtering sample locations (for x,y,z) */
    *fslx, *fsly, *fslz,        /* pointers into fsl */
    *fw0=NULL, *fw1=NULL, *fw2=NULL, /* filter weights (for x, y, and z) */
    *fw0x, *fw0y, *fw0z,        /* pointers into fw0 */
    *fw1x, *fw1y, *fw1z,        /* pointers into fw1 */
    *fw2x, *fw2y, *fw2z,        /* pointers info fw2 */
    *grad, *hess,               /* madness */
    *tmpP, 
    xs, ys, zs,                 /* scaling along the axis */
    (*lup)(void *v, NRRD_BIG_INT I);  /* nrrdFLookup[] element */
  static int 
    idx,                        /* volume index for voxel at probe */
    lidx,                       /* volume index for lower corner of 
				   neighborhood around probe */
    tquery,                     /* bit mask for query plus all the things
				   which we have to measure in order to find
				   what was asked for */
    sx, sy, sz,                 /* volume dimensions */
    o2[8],                      /* for current sx, sy values, offsets to get
				   to other 2^3 corners of a voxel */
    o4[64],                     /* offsets for 4^3 samples */
    fr, fd;                     /* maximum filter radius and diameter among
				   k0, k1, k2 (determines val[] dimensions) */
  static void *data;            /* copy of nin->data */
  static char *ptr;             /* address of data[lidx] */
  
  /* if !_ans, all cached things should be reset to initial conditions */
  if (!_ans) {
    ans = NULL;
    DITCH(val); DITCH(fi2); DITCH(fi1); DITCH(fsl);
    DITCH(fw0); DITCH(fw1); DITCH(fw2);
    fslx = fsly = fslz = NULL;
    fw0x = fw0y = fw0z = NULL;
    fw1x = fw1y = fw1z = NULL;
    fw2x = fw2y = fw2z = NULL;
    return;
  }

  /* if _ans has changed, learn the new values for everything */
  if (ans != _ans) {
    ans = _ans;
    data = nin->data;
    lup = nrrdFLookup[nin->type];
    sx = nin->size[0];
    sy = nin->size[1];
    sz = nin->size[2];
    xs = nin->spacing[0]; xs = AIR_EXISTS(xs) ? xs : 1.0;
    ys = nin->spacing[1]; ys = AIR_EXISTS(ys) ? ys : 1.0;
    zs = nin->spacing[2]; zs = AIR_EXISTS(zs) ? zs : 1.0;
    if (baneProbeDebug) {
      printf("%s: sx,sy,sz = (%d,%d,%d); xs,ys,zs = (%g,%g,%g)\n",
	     me, sx, sy, sz, xs, ys, zs);
    }
    for (k=0; k<=1; k++)
      for (j=0; j<=1; j++)
	for (i=0; i<=1; i++)
	  o2[i+2*(j+2*k)] = i + sx*(j + sy*k);
    for (k=0; k<=3; k++)
      for (j=0; j<=3; j++)
	for (i=0; i<=3; i++)
	  o4[i+4*(j+4*k)] = i + sx*(j + sy*k);
    tquery = 0;
    for (i=0; i<=BANE_PROBE_MAX; i++) {
      if ((query >> i) & 1) {
	tquery |= (1 << i) | baneProbePrereq[i];
      }
    }
    tmpF = pack->k0->support(pack->param0); 
    tmpF = AIR_MAX(tmpF, pack->k1->support(pack->param1));
    tmpF = AIR_MAX(tmpF, pack->k2->support(pack->param2));
    fr = AIR_ROUNDUP(tmpF);
    if (baneProbeDebug) {
      printf("%s: tmpF=%g --> fr = %d\n", me, tmpF, fr);
    }
    if (0 == fr) {
      fprintf(stderr, "%s: calculated fr == 0!!!\n", me);
      return;
    }
    fd = fr*2;
    DITCH(val); DITCH(fi2); DITCH(fi1); DITCH(fsl);
    DITCH(fw0); DITCH(fw1); DITCH(fw2); 
    val = calloc(fd*fd*fd, sizeof(float));
    fi2 = calloc(fd*fd, sizeof(float));
    fi1 = calloc(fd, sizeof(float));
    fsl = calloc(3*fd, sizeof(float));
    fw0 = calloc(3*fd, sizeof(float));
    fw1 = calloc(3*fd, sizeof(float));
    fw2 = calloc(3*fd, sizeof(float));
    fslz = (fsly = (fslx = fsl) + fd) + fd;
    fw0z = (fw0y = (fw0x = fw0) + fd) + fd;
    fw1z = (fw1y = (fw1x = fw1) + fd) + fd;
    fw2z = (fw2y = (fw2x = fw2) + fd) + fd;
    if (!(val && fi2 && fi1 && fsl && fw0 && fw1 && fw2)) {
      fprintf(stderr, "%s: couldn't allocate various caches\n", me);
      return;
    }
  }

  /* where the hell are we? */
  xi = x; xi -= xi == sx-1; xf = x - xi;
  yi = y; yi -= yi == sy-1; yf = y - yi;
  zi = z; zi -= zi == sz-1; zf = z - zi;
  tmpI = xi + sx*(yi + sy*zi);
  if (baneProbeDebug) {
    printf("%s: x, y, z = (%g,%g,%g)\n", me, x, y, z);
    printf("%s: xi, yi, zi = (%d,%d,%d), xf,yf,zf = (%g,%g,%g)\n",
	   me, xi, yi, zi, xf, yf, zf);
  }

  /* have we changed which voxel we're inside? */
  if (idx != tmpI) {
    /* are we still within the volume minus margins? */
    if (xi < fr-1  || yi < fr-1  || zi < fr-1 ||
	xi > sx-fr || yi > sy-fr || zi > sz-fr) {
      if (baneProbeDebug) {
	fprintf(stderr, "%s: location (%g,%g,%g) -> index (%d,%d,%d) "
		"not within %d-margin of %dx%dx%d volume\n",
		me, x, y, z, xi, yi, zi, fr, sx, sy, sz);
      }
      for (i=0; i<=BANE_PROBE_MAX; i++) {
	if ((query >> i) & 1) {
	  memset(ans + baneProbeAnsOffset[i], 0, 
		 sizeof(float)*baneProbeAnsLen[i]);
	}
      }
      return;
    }
    /* else we are somewhere in the volume, fill the val cache */
    idx = tmpI;
    lidx = xi-fr+1 + sx*(yi-fr+1 + sy*(zi-fr+1));
    ptr = (char*)data + lidx*nrrdTypeSize[nin->type];
    switch (fd) {
    case 2:
      for (i=0; i<=7; i++) {
	val[i] = lup(ptr, o2[i]);
      }
      if (baneProbeDebug) {
	printf("val[]:\n");
	printf("% 10.4f   % 10.4f\n", val[6], val[7]);
	printf("   % 10.4f   % 10.4f\n\n", val[4], val[5]);
	printf("% 10.4f   % 10.4f\n", val[2], val[3]);
	printf("   % 10.4f   % 10.4f\n", val[0], val[1]);
      }
      break;
    case 4:
      for (i=0; i<=63; i++)
	val[i] = lup(ptr, o4[i]); {
      }
      if (baneProbeDebug) {
	printf("val[]:\n");
	for (i=3; i>=0; i--) {
	  printf("% 10.4f   % 10.4f   % 10.4f   % 10.4f\n", 
		 val[12+16*i], val[13+16*i], val[14+16*i], val[15+16*i]);
	  printf("   % 10.4f  %c% 10.4f   % 10.4f%c   % 10.4f\n", 
		 val[ 8+16*i], (i==1||i==2)?'\\':' ',
		 val[ 9+16*i], val[10+16*i], (i==1||i==2)?'\\':' ',
		 val[11+16*i]);
	  printf("      % 10.4f  %c% 10.4f   % 10.4f%c   % 10.4f\n", 
		 val[ 4+16*i], (i==1||i==2)?'\\':' ',
		 val[ 5+16*i], val[ 6+16*i], (i==1||i==2)?'\\':' ',
		 val[ 7+16*i]);
	  printf("         % 10.4f   % 10.4f   % 10.4f   % 10.4f\n", 
		 val[ 0+16*i], val[ 1+16*i], val[ 2+16*i], val[ 3+16*i]);
	  if (i) printf("\n");
	}
      }
      break;
    default:
      /* filter diameter is bigger than 4 */
      for (k=0; k<=fd-1; k++) {
	for (j=0; j<=fd-1; j++) {
	  for (i=0; i<=fd-1; i++) {
	    val[i] = lup(ptr, i + sx*(j + sy*k));
	  }
	}
      }
      break;
    }
  }
  
  /* we have the relevant values in val[], now get values for fsl[], 
     fw0[], fw1[], fw2[] arrays based on subvoxel position (xf, yf, zf) */
  switch (fd) {
  case 2:
    fslx[0] = -xf; fslx[1] = -xf+1;
    fsly[0] = -yf; fsly[1] = -yf+1;
    fslz[0] = -zf; fslz[1] = -zf+1;
    if (baneProbeDebug) {
      printf("fsl:\n");
      printf("x[]: % 10.4f   % 10.4f\n", fslx[0], fslx[1]);
      printf("y[]: % 10.4f   % 10.4f\n", fsly[0], fsly[1]);
      printf("z[]: % 10.4f   % 10.4f\n", fslz[0], fslz[1]);
    }
    break;
  case 4:
    fslx[0] = -xf-1; fslx[1] = -xf; fslx[2] = -xf+1; fslx[3] = -xf+2;
    fsly[0] = -yf-1; fsly[1] = -yf; fsly[2] = -yf+1; fsly[3] = -yf+2;
    fslz[0] = -zf-1; fslz[1] = -zf; fslz[2] = -zf+1; fslz[3] = -zf+2;
    if (baneProbeDebug) {
      printf("fsl:\n");
      printf("x[]:% 10.4f  % 10.4f  % 10.4f  % 10.4f\n", 
	     fslx[0], fslx[1], fslx[2], fslx[3]);
      printf("y[]:% 10.4f  % 10.4f  % 10.4f  % 10.4f\n", 
	     fsly[0], fsly[1], fsly[2], fsly[3]);
      printf("z[]:% 10.4f  % 10.4f  % 10.4f  % 10.4f\n", 
	     fslz[0], fslz[1], fslz[2], fslz[3]);
    }
    break;
  default:
    /* filter diameter is bigger than 4 */
    for (i=0; i<=fd-1; i++) {
      fslx[i] = -xf-fr+1+i;
      fsly[i] = -yf-fr+1+i;
      fslz[i] = -zf-fr+1+i;
    }
    break;
  }
  /* does evaluations for x, y, z all at once to minimize calling overhead */
  pack->k0->evalVec(fw0, fsl, 3*fd, pack->param0);
  pack->k1->evalVec(fw1, fsl, 3*fd, pack->param1);
  pack->k2->evalVec(fw2, fsl, 3*fd, pack->param2);
  /* fix scalings for anisotropic voxels */
  for (i=0; i<=fd-1; i++) {
    fw1x[i] /= xs;  fw2x[i] /= xs*xs;
    fw1y[i] /= ys;  fw2y[i] /= ys*ys;
    fw1z[i] /= zs;  fw2z[i] /= zs*zs;
  }
  if (baneProbeDebug) {
    printf("fw0:\n");
    switch (fd) {
    case 2:
      printf("x[]: % 10.4f   % 10.4f\n", fw0x[0], fw0x[1]);
      printf("y[]: % 10.4f   % 10.4f\n", fw0y[0], fw0y[1]);
      printf("z[]: % 10.4f   % 10.4f\n", fw0z[0], fw0z[1]);
      break;
    case 4:
      printf("x[]:% 10.4f  % 10.4f  % 10.4f  % 10.4f\n", 
	     fw0x[0], fw0x[1], fw0x[2], fw0x[3]);
      printf("y[]:% 10.4f  % 10.4f  % 10.4f  % 10.4f\n", 
	     fw0y[0], fw0y[1], fw0y[2], fw0y[3]);
      printf("z[]:% 10.4f  % 10.4f  % 10.4f  % 10.4f\n", 
	     fw0z[0], fw0z[1], fw0z[2], fw0z[3]);
      break;
    default:
      break;
    }
  }

  grad = ans + baneProbeAnsOffset[baneProbeGradVec];
  hess = ans + baneProbeAnsOffset[baneProbeHess];
  switch (fd) {
  case 2:
#define D2(a,b) ((a)[0]*(b)[0]+(a)[1]*(b)[1])
    /* x0 */
    fi2[0] = D2(fw0x, val + 0*2);           
    fi2[1] = D2(fw0x, val + 1*2);
    fi2[2] = D2(fw0x, val + 2*2);
    fi2[3] = D2(fw0x, val + 3*2);
    /* x0y0 */
    fi1[0] = D2(fw0y, fi2 + 0*2);           
    fi1[1] = D2(fw0y, fi2 + 1*2);
    *ans = D2(fw0z, fi1);                   /* f */
    grad[2] = D2(fw1z, fi1);                /* g_z */
    hess[8] = D2(fw2z, fi1);                /* h_zz */
    /* x0y1 */
    fi1[0] = D2(fw1y, fi2 + 0*2);           
    fi1[1] = D2(fw1y, fi2 + 1*2);   
    grad[1] = D2(fw0z, fi1);                /* g_y */
    hess[5] = hess[7] = D2(fw1z, fi1);      /* h_yz */
    /* x0y2 */
    fi1[0] = D2(fw2y, fi2 + 0*2);           
    fi1[1] = D2(fw2y, fi2 + 1*2);
    hess[4] = D2(fw0z, fi1);                /* h_yy */
    /* x1 */
    fi2[0] = D2(fw1x, val + 0*2);           
    fi2[1] = D2(fw1x, val + 1*2);
    fi2[2] = D2(fw1x, val + 2*2);
    fi2[3] = D2(fw1x, val + 3*2);
    /* x1y0 */
    fi1[0] = D2(fw0y, fi2 + 0*2);           
    fi1[1] = D2(fw0y, fi2 + 1*2);
    grad[0] = D2(fw0z, fi1);                /* g_x */
    hess[2] = hess[6] = D2(fw1z, fi1);      /* h_xz */
    /* x1y1 */
    fi1[0] = D2(fw1y, fi2 + 0*2);           
    fi1[1] = D2(fw1y, fi2 + 1*2);
    hess[1] = hess[3] = D2(fw0z, fi1);      /* h_xy */
    /* x2 */
    fi2[0] = D2(fw2x, val + 0*2);           
    fi2[1] = D2(fw2x, val + 1*2);
    fi2[2] = D2(fw2x, val + 2*2);
    fi2[3] = D2(fw2x, val + 3*2);
    /* x2y0 */
    fi1[0] = D2(fw0y, fi2 + 0*2);           
    fi1[1] = D2(fw0y, fi2 + 1*2);
    hess[0] = D2(fw0z, fi1);                /* h_xx */
    break;
  case 4:
    /* If you're reading this code and you want to understand what the 
       heck is going on here, you really ought to come and talk to me.
       It is just not meant to be human readable. */
#define D4(a,b) ((a)[0]*(b)[0]+(a)[1]*(b)[1]+(a)[2]*(b)[2]+(a)[3]*(b)[3])
    /* x0 */
    fi2[ 0] = D4(fw0x, val+ 0*4);  fi2[ 1] = D4(fw0x, val+ 1*4);
    fi2[ 2] = D4(fw0x, val+ 2*4);  fi2[ 3] = D4(fw0x, val+ 3*4);
    fi2[ 4] = D4(fw0x, val+ 4*4);  fi2[ 5] = D4(fw0x, val+ 5*4);
    fi2[ 6] = D4(fw0x, val+ 6*4);  fi2[ 7] = D4(fw0x, val+ 7*4);
    fi2[ 8] = D4(fw0x, val+ 8*4);  fi2[ 9] = D4(fw0x, val+ 9*4);
    fi2[10] = D4(fw0x, val+10*4);  fi2[11] = D4(fw0x, val+11*4);
    fi2[12] = D4(fw0x, val+12*4);  fi2[13] = D4(fw0x, val+13*4);
    fi2[14] = D4(fw0x, val+14*4);  fi2[15] = D4(fw0x, val+15*4);
    /* x0y0 */
    fi1[ 0] = D4(fw0y, fi2+ 0*4);  fi1[ 1] = D4(fw0y, fi2+ 1*4);
    fi1[ 2] = D4(fw0y, fi2+ 2*4);  fi1[ 3] = D4(fw0y, fi2+ 3*4);
    *ans = D4(fw0z, fi1);                   /* f */
    grad[2] = D4(fw1z, fi1);                /* g_z */
    hess[8] = D4(fw2z, fi1);                /* h_zz */
    /* x0y1 */
    fi1[ 0] = D4(fw1y, fi2+ 0*4);  fi1[ 1] = D4(fw1y, fi2+ 1*4);
    fi1[ 2] = D4(fw1y, fi2+ 2*4);  fi1[ 3] = D4(fw1y, fi2+ 3*4);
    grad[1] = D4(fw0z, fi1);                /* g_y */
    hess[5] = hess[7] = D4(fw1z, fi1);      /* h_yz */
    /* x0y2 */
    fi1[ 0] = D4(fw2y, fi2+ 0*4);  fi1[ 1] = D4(fw2y, fi2+ 1*4);
    fi1[ 2] = D4(fw2y, fi2+ 2*4);  fi1[ 3] = D4(fw2y, fi2+ 3*4);
    hess[4] = D4(fw0z, fi1);                /* h_yy */
    /* x1 */
    fi2[ 0] = D4(fw1x, val+ 0*4);  fi2[ 1] = D4(fw1x, val+ 1*4);
    fi2[ 2] = D4(fw1x, val+ 2*4);  fi2[ 3] = D4(fw1x, val+ 3*4);
    fi2[ 4] = D4(fw1x, val+ 4*4);  fi2[ 5] = D4(fw1x, val+ 5*4);
    fi2[ 6] = D4(fw1x, val+ 6*4);  fi2[ 7] = D4(fw1x, val+ 7*4);
    fi2[ 8] = D4(fw1x, val+ 8*4);  fi2[ 9] = D4(fw1x, val+ 9*4);
    fi2[10] = D4(fw1x, val+10*4);  fi2[11] = D4(fw1x, val+11*4);
    fi2[12] = D4(fw1x, val+12*4);  fi2[13] = D4(fw1x, val+13*4);
    fi2[14] = D4(fw1x, val+14*4);  fi2[15] = D4(fw1x, val+15*4);
    /* x1y0 */
    fi1[ 0] = D4(fw0y, fi2+ 0*4);  fi1[ 1] = D4(fw0y, fi2+ 1*4);
    fi1[ 2] = D4(fw0y, fi2+ 2*4);  fi1[ 3] = D4(fw0y, fi2+ 3*4);
    grad[0] = D4(fw0z, fi1);                /* g_x */
    hess[2] = hess[6] = D4(fw1z, fi1);      /* h_xz */
    /* x1y1 */
    fi1[ 0] = D4(fw1y, fi2+ 0*4);  fi1[ 1] = D4(fw1y, fi2+ 1*4);
    fi1[ 2] = D4(fw1y, fi2+ 2*4);  fi1[ 3] = D4(fw1y, fi2+ 3*4);
    hess[1] = hess[3] = D4(fw0z, fi1);      /* h_xy */
    /* x2 (damn h_xx) */
    fi2[ 0] = D4(fw2x, val+ 0*4);  fi2[ 1] = D4(fw2x, val+ 1*4);
    fi2[ 2] = D4(fw2x, val+ 2*4);  fi2[ 3] = D4(fw2x, val+ 3*4);
    fi2[ 4] = D4(fw2x, val+ 4*4);  fi2[ 5] = D4(fw2x, val+ 5*4);
    fi2[ 6] = D4(fw2x, val+ 6*4);  fi2[ 7] = D4(fw2x, val+ 7*4);
    fi2[ 8] = D4(fw2x, val+ 8*4);  fi2[ 9] = D4(fw2x, val+ 9*4);
    fi2[10] = D4(fw2x, val+10*4);  fi2[11] = D4(fw2x, val+11*4);
    fi2[12] = D4(fw2x, val+12*4);  fi2[13] = D4(fw2x, val+13*4);
    fi2[14] = D4(fw2x, val+14*4);  fi2[15] = D4(fw2x, val+15*4);
    /* x2y0 */
    fi1[ 0] = D4(fw0y, fi2+ 0*4);  fi1[ 1] = D4(fw0y, fi2+ 1*4);
    fi1[ 2] = D4(fw0y, fi2+ 2*4);  fi1[ 3] = D4(fw0y, fi2+ 3*4);
    hess[0] = D4(fw0z, fi1);                /* h_xx */
    break;
  default:
    printf("%s: NOT IMPLEMENTED\n", me);
    break;
  }

  tmpI = tquery;
  /* 0: always do baneProbeVal */
  tmpI >>= 1;
  /* 1: always do baneProbeGradVec */
  tmpI >>= 1;
  /* 2: ??? baneProbeGradMag */
  if (tmpI & 1) {
    tmpF = sqrt(grad[0]*grad[0] + grad[1]*grad[1] + grad[2]*grad[2]);
    ans[baneProbeAnsOffset[baneProbeGradMag]] = tmpF;
  }
  tmpI >>= 1;
  /* 3: ??? baneProbeNormal */
  if (tmpI & 1) {
    /* hack: from baneProbePrereq we know that tmpF is still grad mag */
    tmpG = _baneZeroNudge(tmpF);
    tmpP = ans + baneProbeAnsOffset[baneProbeNormal];
    if (tmpG) {
      *tmpP++ = grad[0]/tmpG;
      *tmpP++ = grad[1]/tmpG;
      *tmpP++ = grad[2]/tmpG;
    }
    else {
      *tmpP++ = 0;
      *tmpP++ = 0;
      *tmpP++ = 0;
    }
  }
  tmpI >>= 1;
  /* 4: always do baneProbeHess */
  tmpI >>= 1;
  /* 5: ??? baneProbeHessEval */
  if (tmpI & 1) {

  }
  tmpI >>= 1;
  /* 6: ??? baneProbeHessEvec */
  if (tmpI & 1) {

  }
  tmpI >>= 1;
  /* 7: ??? baneProbe2ndDD */
  if (tmpI & 1) {
    /* hack: from baneProbePrereq we know that tmpF is still grad mag */
    tmpP = ans + baneProbeAnsOffset[baneProbeNormal];
    ans[baneProbeAnsOffset[baneProbe2ndDD]] = _baneOneClamp(tmpF)*
      (tmpP[0]*(tmpP[0]*hess[0] + tmpP[1]*hess[1] + tmpP[2]*hess[2]) +
       tmpP[1]*(tmpP[0]*hess[3] + tmpP[1]*hess[4] + tmpP[2]*hess[5]) +
       tmpP[2]*(tmpP[0]*hess[6] + tmpP[1]*hess[7] + tmpP[2]*hess[8]));
  }
}


/*
  baneProbeVal,            0 f
  baneProbeGradVec,        1 dx, dy, dz
  baneProbeGradMag,        2 
  baneProbeNormal,         3 
  baneProbeHess,           4 dxx, dxy, dxz, dyy, dyz, dzz
  baneProbeHessEval,       5
  baneProbeHessEvec,       6 
  baneProbe2ndDD,          7
  baneProbeCurvVecs,       8
  baneProbeK1K2,           9
  baneProbeShapeIndex,     10
  baneProbeCurvedness      11

*/
