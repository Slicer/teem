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


#include "limn.h"

void
_limnPSPreamble(limnObj *obj, limnCam *cam, limnWin *win) {
  
  fprintf(win->file, "%%!PS-Adobe-2.0 EPSF-2.0\n");
  fprintf(win->file, "%%%%Creator: limn\n");
  fprintf(win->file, "%%%%Pages: 1\n");
  fprintf(win->file, "%%%%BoundingBox: %d %d %d %d\n", 
	  (int)(win->bbox[0]), 
	  (int)(win->bbox[1]), 
	  (int)(win->bbox[2]), 
	  (int)(win->bbox[3]));
  fprintf(win->file, "%%%%EndComments\n");
  fprintf(win->file, "%%%%EndProlog\n");
  fprintf(win->file, "%%%%Page: 1 1\n");
  fprintf(win->file, "gsave\n");
  fprintf(win->file, "%g %g moveto\n", win->bbox[0], win->bbox[1]);
  fprintf(win->file, "%g %g lineto\n", win->bbox[2], win->bbox[1]);
  fprintf(win->file, "%g %g lineto\n", win->bbox[2], win->bbox[3]);
  fprintf(win->file, "%g %g lineto\n", win->bbox[0], win->bbox[3]);
  fprintf(win->file, "closepath\n");
  fprintf(win->file, "gsave %g setgray fill grestore\n", win->ps.bgGray);
  fprintf(win->file, "clip\n");
  fprintf(win->file, "gsave newpath\n");
  fprintf(win->file, "1 setlinejoin\n");
  fprintf(win->file, "1 setlinecap\n");
  fprintf(win->file, "5 setlinewidth\n");
  fprintf(win->file, "/M {moveto} bind def\n");
  fprintf(win->file, "/L {lineto} bind def\n");
  fprintf(win->file, "/W {setlinewidth} bind def\n");
  fprintf(win->file, "/F {fill} bind def\n");
  fprintf(win->file, "/S {stroke} bind def\n");
  fprintf(win->file, "/CP {closepath} bind def\n");
  fprintf(win->file, "/RGB {setrgbcolor} bind def\n");
  fprintf(win->file, "/Gr {setgray} bind def\n");
  fprintf(win->file, "\n");
}

void
_limnPSEpilogue(limnObj *obj, limnCam *cam, limnWin *win) {

  fprintf(win->file, "grestore\n");
  fprintf(win->file, "grestore\n");
  /* fprintf(win->file, "showpage\n"); */
  fprintf(win->file, "%%%%Trailer\n");
}

void
_limnPSDrawFace(limnObj *obj, limnPart *r, limnFace *f, 
		limnCam *cam, Nrrd *nmap, limnWin *win) {
  int vi;
  limnPoint *p;
  unsigned short qn;
  float *map, R, G, B;

  qn = limnQNVto16(f->wn, AIR_FALSE);
  map = nmap->data;
  for (vi=0; vi<f->vNum; vi++) {
    p = obj->p + obj->v[vi + f->vBase];
    fprintf(win->file, "%g %g %s\n", 
	    p->d[0], p->d[1], vi ? "L" : "M");
  }
  R = r->rgba[0]/255.0;
  G = r->rgba[1]/255.0;
  B = r->rgba[2]/255.0;
  R *= map[0 + 3*qn];
  G *= map[1 + 3*qn];
  B *= map[2 + 3*qn];
  if (R == G && G == B) {
    fprintf(win->file, "CP %g Gr F\n", R);
  }
  else {
    fprintf(win->file, "CP %g %g %g RGB F\n", R, G, B);
  }
}

void
_limnPSDrawEdge(limnObj *obj, limnPart *r, limnEdge *e, 
		limnCam *cam, limnWin *win) {
  limnPoint *p0, *p1;

  if (win->ps.edgeWidth[e->visib]) {
    p0 = obj->p + e->v0;
    p1 = obj->p + e->v1;
    fprintf(win->file, "%g %g M ", p0->d[0], p0->d[1]);
    fprintf(win->file, "%g %g L ", p1->d[0], p1->d[1]);
    fprintf(win->file, "%g W ", win->ps.edgeWidth[e->visib]);
    fprintf(win->file, "S\n");
  }
}

int
limnObjPSRender(limnObj *obj, limnCam *cam, Nrrd *map, limnWin *win) {
  int vis0, vis1, inside;
  float angle, widthTmp;
  limnFace *f, *f0, *f1; int fi;
  limnEdge *e; int ei;
  limnPart *r; int ri;
  limnPoint *p; int pi;
  
  _limnPSPreamble(obj, cam, win);

  for (ri=0; ri<obj->rA->len; ri++) {
    r = &(obj->r[ri]);

    inside = 0;
    for (pi=0; pi<r->pNum; pi++) {
      p = &(obj->p[r->pBase + pi]);
      inside |= (AIR_INSIDE(win->bbox[0], p->d[0], win->bbox[2]) &&
		 AIR_INSIDE(win->bbox[1], p->d[1], win->bbox[3]));
      if (inside)
	break;
    }
    
    if (inside) {
      if (1 == r->eNum) {
	/* this part is just one lone edge */
	e = &(obj->e[r->eBase]);
	widthTmp = win->ps.edgeWidth[e->visib];
	fprintf(win->file, "%g setgray\n", 1 - win->ps.bgGray);
	win->ps.edgeWidth[e->visib] = 8;
	_limnPSDrawEdge(obj, r, e, cam, win);
	fprintf(win->file, "%g %g %g RGB\n", 
		r->rgba[0]/255.0, r->rgba[1]/255.0, r->rgba[2]/255.0);
	win->ps.edgeWidth[e->visib] = 4;
	_limnPSDrawEdge(obj, r, e, cam, win);
	win->ps.edgeWidth[e->visib] = widthTmp;
      }
      else {
	/* this part is either a lone face or a solid */
	for (fi=0; fi<r->fNum; fi++) {
	  f = &(obj->f[r->fBase + fi]);
	  f->visib = f->sn[2] < 0;
	  if (f->visib)
	    _limnPSDrawFace(obj, r, f, cam, map, win);
	}
	
	fprintf(win->file, "0 setgray\n");
	
	for (ei=0; ei<r->eNum; ei++) {
	  e = &(obj->e[r->eBase + ei]);
	  f0 = &(obj->f[e->f0]);
	  f1 = &(obj->f[e->f1]);
	  vis0 = f0->visib;
	  vis1 = f1->visib;
	  angle = 180/M_PI*acos(ELL_3V_DOT(f0->wn, f1->wn));
	  if (vis0 && vis1) {
	    e->visib = 3 + (angle > win->ps.creaseAngle);
	  }
	  else if (!!vis0 ^ !!vis1) {
	    e->visib = 2;
	  }
	  else {
	    e->visib = 1 - (angle > win->ps.creaseAngle);
	  }
	  _limnPSDrawEdge(obj, r, e, cam, win);
	}
      }
    }
  }

  _limnPSEpilogue(obj, cam, win);

  return 0;
}
