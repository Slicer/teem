/*
  teem: Gordon Kindlmann's research software
  Copyright (C) 2004, 2003, 2002, 2001, 2000, 1999, 1998 University of Utah

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


#include "limn.h"

int
limnObjDescribe(FILE *file, limnObj *obj) {
  int j, i, vi;
  limnFace *f;
  limnEdge *e;
  limnPoint *p;
  limnPart *r;
  
  fprintf(file, "parts: %d\n", obj->rA->len);
  for (j=0; j<=obj->rA->len-1; j++) {
    r = &(obj->r[j]);
    fprintf(file, "%d | points: %d\n", j, r->pNum);
    for (i=0; i<=r->pNum-1; i++) {
      p = &(obj->p[r->pBase + i]);
      fprintf(file, "%d | %d(%d): w=(%g,%g,%g)\tv=(%g,%g,%g)\ts(%g,%g,%g)\n", 
	      j, i, r->pBase + i, 
	      p->w[0], p->w[1], p->w[2],
	      p->v[0], p->v[1], p->v[2],
	      p->s[0], p->s[1], p->s[2]);
    }
    fprintf(file, "%d | edges: %d\n", j, r->eNum);
    for (i=0; i<=r->eNum-1; i++) {
      e = &(obj->e[r->eBase + i]);
      fprintf(file, "%d | %d(%d): vert(%d,%d), face(%d,%d)\n", 
	      j, i, r->eBase + i, e->v0, e->v1, e->f0, e->f1);
    }
    fprintf(file, "%d | faces: %d\n", j, r->fNum);
    for (i=0; i<=r->fNum-1; i++) {
      f = &(obj->f[r->fBase + i]);
      fprintf(file, "%d | %d(%d): [", j, i, r->fBase + i);
      for (vi=0; vi<=f->vNum-1; vi++) {
	fprintf(file, "%d", obj->v[f->vBase + vi]);
	if (vi < f->vNum-1)
	  fprintf(file, ",");
      }
      fprintf(file, "]; wn = (%g,%g,%g)\n", f->wn[0], f->wn[1], f->wn[2]);
    }
  }

  return 0;
}

int
limnObjOFFWrite(FILE *file, limnObj *obj) {
  char me[]="limnObjOFFWrite", err[AIR_STRLEN_MED];
  int ii, vi;
  limnPoint *p;
  limnFace *f;
  
  if (!( obj && file )) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(LIMN, err); return 1;
  }
  fprintf(file, "OFF\n");
  fprintf(file, "%d %d -1\n", obj->pA->len, obj->fA->len);
  for (ii=0; ii<obj->pA->len; ii++) {
    p = obj->p + ii;
    fprintf(file, "%g %g %g",
	    p->w[0]/p->w[3], p->w[1]/p->w[3], p->w[2]/p->w[3]);
    if (p->sp) {
      /* its a non-default color */
      fprintf(file, " %g %g %g", obj->s[p->sp].rgba[0],
	      obj->s[p->sp].rgba[1], obj->s[p->sp].rgba[2]);
    }
    fprintf(file, "\n");
  }
  for (ii=0; ii<obj->fA->len; ii++) {
    f = obj->f + ii;
    fprintf(file, "%d", f->vNum);
    for (vi=0; vi<f->vNum; vi++) {
      fprintf(file, " %d", obj->v[vi + f->vBase]);
    }
    if (f->sp) {
      fprintf(file, " %g %g %g", obj->s[f->sp].rgba[0],
	      obj->s[f->sp].rgba[1], obj->s[f->sp].rgba[2]);
    }
    fprintf(file, "\n");
  }
  return 0;
}

int
limnObjOFFRead(limnObj *obj, FILE *file) {
  char me[]="limnObjOFFRead", err[AIR_STRLEN_MED];
  double vert[6];
  char line[AIR_STRLEN_LARGE];  /* HEY: bad bad Gordon */
  int si, lret, nvert, nface, ii, got, ibuff[512];  /* HEY: bad bad Gordon */
  float fbuff[512];  /* HEY: bad bad Gordon */

  if (!( obj && file )) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(LIMN, err); return 1;
  }
  got = 0;
  do {
    if (!airOneLine(file, line, AIR_STRLEN_LARGE)) {
      sprintf(err, "%s: hit EOF before getting #vert #face #edge line", me);
      biffAdd(LIMN, err); return 1;
    }
    got = airParseStrI(ibuff, line, AIR_WHITESPACE, 3);
  } while (3 != got);
  nvert = ibuff[0];
  nface = ibuff[1];
  limnObjPartStart(obj);
  for (ii=0; ii<nvert; ii++) {
    do {
      lret = airOneLine(file, line, AIR_STRLEN_LARGE);
    } while (1 == lret);
    if (!lret) {
      sprintf(err, "%s: hit EOF trying to read vert %d (of %d)",
	      me, ii, nvert);
      biffAdd(LIMN, err); return 1;
    }
    if (3 != airParseStrD(vert, line, AIR_WHITESPACE, 3)) {
      sprintf(err, "%s: couldn't parse 3 doubles from \"%s\" "
	      "for vert %d (of %d)",
	      me, line, ii, nvert);
      biffAdd(LIMN, err); return 1;
    }
    if (6 == airParseStrD(vert, line, AIR_WHITESPACE, 6)) {
      /* we could also parse an RGB color */
      si = limnObjSPAdd(obj);
      ELL_4V_SET(obj->s[si].rgba, vert[3], vert[4], vert[5], 1);
    } else {
      si = 0;
    }
    limnObjPointAdd(obj, si,  vert[0], vert[1], vert[2]);
  }
  for (ii=0; ii<nface; ii++) {
    do {
      lret = airOneLine(file, line, AIR_STRLEN_LARGE);
    } while (1 == lret);
    if (!lret) {
      sprintf(err, "%s: hit EOF trying to read face %d (of %d)",
	      me, ii, nface);
      biffAdd(LIMN, err); return 1;
    }
    if (1 != sscanf(line, "%d", &nvert)) {
      sprintf(err, "%s: can't get first int (#verts) from \"%s\" "
	      "for face %d (of %d)",
	      me, line, ii, nface);
      biffAdd(LIMN, err); return 1;
    }
    if (nvert+1 != airParseStrI(ibuff, line, AIR_WHITESPACE, nvert+1)) {
      sprintf(err, "%s: couldn't parse %d ints from \"%s\" "
	      "for face %d (of %d)",
	      me, nvert+1, line, ii, nface);
      biffAdd(LIMN, err); return 1;
    }
    if (nvert+1+3 == airParseStrF(fbuff, line, AIR_WHITESPACE, nvert+1+3)) {
      /* could also parse color */
      si = limnObjSPAdd(obj);
      ELL_4V_SET(obj->s[si].rgba,
		 fbuff[nvert+1+0], fbuff[nvert+1+1], fbuff[nvert+1+2], 1);
    } else {
      si = 0;
    }
    limnObjFaceAdd(obj, si, nvert, ibuff+1);
  }
  limnObjPartFinish(obj);
  
  return 0;
}

