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
limnObjectDescribe(FILE *file, limnObject *obj) {
  limnFace *face; int si, fii;
  limnEdge *edge; int eii;
  limnVertex *vert; int vii;
  limnPart *part; int partIdx;
  
  fprintf(file, "parts: %d\n", obj->partNum);
  for (partIdx=0; partIdx<obj->partNum; partIdx++) {
    part = obj->part + partIdx;
    fprintf(file, "part %d | verts: %d ========\n", partIdx, part->vertIdxNum);
    for (vii=0; vii<part->vertIdxNum; vii++) {
      vert = obj->vert + part->vertIdx[vii];
      fprintf(file, "part %d | %d(%d): "
	      "w=(%g,%g,%g)\tv=(%g,%g,%g)\ts(%g,%g,%g)\n", 
	      partIdx, vii, part->vertIdx[vii], 
	      vert->world[0], vert->world[1], vert->world[2],
	      vert->view[0], vert->view[1], vert->view[2],
	      vert->screen[0], vert->screen[1], vert->screen[2]);
    }
    fprintf(file, "part %d | edges: %d ========\n", partIdx, part->edgeIdxNum);
    for (eii=0; eii<part->edgeIdxNum; eii++) {
      edge = obj->edge + part->edgeIdx[eii];
      fprintf(file, "part %d | %d(%d): "
	      "vert(%d,%d), face(%d,%d)\n", 
	      partIdx, eii, part->edgeIdx[eii],
	      edge->vertIdxIdx[0], edge->vertIdxIdx[1],
	      edge->faceIdxIdx[0], edge->faceIdxIdx[1]);
    }
    fprintf(file, "part %d | faces: %d ========\n", partIdx, part->faceIdxNum);
    for (fii=0; fii<part->faceIdxNum; fii++) {
      face = obj->face + part->faceIdx[fii];
      fprintf(file, "part %d | %d(%d): [", partIdx, fii, part->faceIdx[fii]);
      for (si=0; si<face->sideNum; si++) {
	fprintf(file, "%d", part->vertIdx[face->vertIdxIdx[si]]);
	if (si < face->sideNum-1)
	  fprintf(file, ",");
      }
      fprintf(file, "]; wn = (%g,%g,%g)\n", face->worldNormal[0],
	      face->worldNormal[1], face->worldNormal[2]);
    }
  }

  return 0;
}

int
limnObjectOFFWrite(FILE *file, limnObject *obj) {
  char me[]="limnObjectOFFWrite", err[AIR_STRLEN_MED];
  int si;
  limnVertex *vert; int vii;
  limnFace *face; int fii;
  limnPart *part; int partIdx;
  
  if (!( obj && file )) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(LIMN, err); return 1;
  }
  fprintf(file, "OFF # created by teem/limn\n");
  /* there will be (obj->partNum - 1) dummy vertices marking
     the boundary between different parts */
  fprintf(file, "%d %d %d\n", obj->vertNum + obj->partNum - 1,
	  obj->faceNum, obj->edgeNum);

  /* write vertices */
  for (partIdx=0; partIdx<obj->partNum; partIdx++) {
    part = obj->part + partIdx;
    for (vii=0; vii<part->vertIdxNum; vii++) {
      vert = obj->vert + part->vertIdx[vii];
      fprintf(file, "%g %g %g",
	      vert->world[0]/vert->world[3],
	      vert->world[1]/vert->world[3],
	      vert->world[2]/vert->world[3]);
      if (vert->lookIdx) {
	/* its a non-default color */
	fprintf(file, " %g %g %g",
		obj->look[vert->lookIdx].rgba[0],
		obj->look[vert->lookIdx].rgba[1],
		obj->look[vert->lookIdx].rgba[2]);
      }
      fprintf(file, "\n");
    }
    /* dummy vertex, but not after last part */
    if (partIdx<obj->partNum-1) {
      fprintf(file, "666 666 666  # end part %d\n", partIdx);
    }
  }

  /* write faces */
  for (partIdx=0; partIdx<obj->partNum; partIdx++) {
    part = obj->part + partIdx;
    for (fii=0; fii<part->faceIdxNum; fii++) {
      face = obj->face + part->faceIdx[fii];
      fprintf(file, "%d", face->sideNum);
      for (si=0; si<face->sideNum; si++) {
	fprintf(file, " %d", part->vertIdx[face->vertIdxIdx[si]] + partIdx);
      }
      if (face->lookIdx) {
	fprintf(file, " %g %g %g",
		obj->look[face->lookIdx].rgba[0],
		obj->look[face->lookIdx].rgba[1],
		obj->look[face->lookIdx].rgba[2]);
      }
      fprintf(file, "\n");
    }
  }

#if 0  /* before the OFF vertex hijack to delineate parts */
  for (ii=0; ii<obj->pA->len; ii++) {
    p = obj->p + ii;
    fprintf(file, "%g %g %g",
	    p->w[0]/p->w[3], p->w[1]/p->w[3], p->w[2]/p->w[3]);
    if (p->lookIdx) {
      /* its a non-default color */
      fprintf(file, " %g %g %g",
	      obj->look[p->lookIdx].rgba[0],
	      obj->look[p->lookIdx].rgba[1],
	      obj->look[p->lookIdx].rgba[2]);
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
      fprintf(file, " %g %g %g",
	      obj->look[f->lookIdx].rgba[0],
	      obj->look[f->lookIdx].rgba[1],
	      obj->look[f->lookIdx].rgba[2]);
    }
    fprintf(file, "\n");
  }
#endif

  return 0;
}

int
limnObjectOFFRead(limnObject *obj, FILE *file) {
  char me[]="limnObjectOFFRead", err[AIR_STRLEN_MED];
  double vert[6];
  char line[AIR_STRLEN_LARGE];  /* HEY: bad Gordon */
  int lookIdx, ri, lret, nvert, nface, ii, got;
  int ibuff[512]; /* HEY: bad Gordon */
  float fbuff[512];  /* HEY: bad bad Gordon */
  int *rlut;
  airArray *mop;

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
  
  mop = airMopNew();
  rlut = (int*)calloc(nvert, sizeof(int));
  airMopAdd(mop, rlut, airFree, airMopAlways);
  ri = -1; /* ssh */
  for (ii=0; ii<nvert; ii++) {
    do {
      lret = airOneLine(file, line, AIR_STRLEN_LARGE);
    } while (1 == lret);
    if (!lret) {
      sprintf(err, "%s: hit EOF trying to read vert %d (of %d)",
	      me, ii, nvert);
      biffAdd(LIMN, err); airMopError(mop); return 1;
    }
    if (3 != airParseStrD(vert, line, AIR_WHITESPACE, 3)) {
      sprintf(err, "%s: couldn't parse 3 doubles from \"%s\" "
	      "for vert %d (of %d)",
	      me, line, ii, nvert);
      biffAdd(LIMN, err); airMopError(mop); return 1;
    }
    if (!ii || (666 == vert[0] && 666 == vert[1] && 666 == vert[2])) {
      ri = limnObjectPartAdd(obj);
    }
    rlut[ii] = ri;
    if (6 == airParseStrD(vert, line, AIR_WHITESPACE, 6)) {
      /* we could also parse an RGB color */
      lookIdx = limnObjectLookAdd(obj);
      ELL_4V_SET(obj->look[lookIdx].rgba, vert[3], vert[4], vert[5], 1);
    } else {
      lookIdx = 0;
    }
    limnObjectVertexAdd(obj, ri, lookIdx, vert[0], vert[1], vert[2]);
  }
  for (ii=0; ii<nface; ii++) {
    do {
      lret = airOneLine(file, line, AIR_STRLEN_LARGE);
    } while (1 == lret);
    if (!lret) {
      sprintf(err, "%s: hit EOF trying to read face %d (of %d)",
	      me, ii, nface);
      biffAdd(LIMN, err); airMopError(mop); return 1;
    }
    if (1 != sscanf(line, "%d", &nvert)) {
      sprintf(err, "%s: can't get first int (#verts) from \"%s\" "
	      "for face %d (of %d)",
	      me, line, ii, nface);
      biffAdd(LIMN, err); airMopError(mop); return 1;
    }
    if (nvert+1 != airParseStrI(ibuff, line, AIR_WHITESPACE, nvert+1)) {
      sprintf(err, "%s: couldn't parse %d ints from \"%s\" "
	      "for face %d (of %d)",
	      me, nvert+1, line, ii, nface);
      biffAdd(LIMN, err); airMopError(mop); return 1;
    }
    if (nvert+1+3 == airParseStrF(fbuff, line, AIR_WHITESPACE, nvert+1+3)) {
      /* could also parse color */
      lookIdx = limnObjectLookAdd(obj);
      ELL_4V_SET(obj->look[lookIdx].rgba,
		 fbuff[nvert+1+0], fbuff[nvert+1+1], fbuff[nvert+1+2], 1);
    } else {
      lookIdx = 0;
    }
    limnObjectFaceAdd(obj, rlut[ibuff[1]], lookIdx, nvert, ibuff+1);
  }
  
  airMopOkay(mop);
  return 0;
}

