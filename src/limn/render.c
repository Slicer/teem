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

}

void
_limnPSEpilogue(limnObj *obj, limnCam *cam, limnWin *win) {

  fprintf(win->file, "grestore\n");
  fprintf(win->file, "grestore\n");
  fprintf(win->file, "showpage\n");
  fprintf(win->file, "%%%%Trailer\n");
}

void
_limnPSDrawFace(limnObj *obj, limnFace *f, 
		limnCam *cam, Nrrd *nmap, limnWin *win) {
  int vi;
  limnPoint *p;
  unsigned short qn;
  float *map;
  
  qn = limnQNVto16(f->wn, AIR_FALSE);
  map = nmap->data;
  for (vi=0; vi<f->vNum; vi++) {
    p = obj->p + obj->v[vi + f->vBase];
    fprintf(win->file, "%g %g %s\n", 
	    p->d[0], p->d[1], vi ? "lineto" : "moveto");
  }
  fprintf(win->file, "closepath %g %g %g setrgbcolor fill\n",
	  map[0 + 3*qn], map[1 + 3*qn], map[2 + 3*qn]);
}

void
_limnPSDrawEdge(limnObj *obj, limnEdge *e, 
		limnCam *cam, limnWin *win) {
  
  if (win->ps.edgeWidth[e->visib]) {
    fprintf(win->file, "%g %g moveto\n", 
	    obj->p[e->v0].d[0], obj->p[e->v0].d[1]);
    fprintf(win->file, "%g %g lineto %g setlinewidth stroke\n",
	    obj->p[e->v1].d[0], obj->p[e->v1].d[1],
	    win->ps.edgeWidth[e->visib]);
  }
}

int
limnObjPSRender(limnObj *obj, limnCam *cam, Nrrd *map, limnWin *win) {
  int vis0, vis1;
  float angle;
  limnFace *f, *f0, *f1; int fi;
  limnEdge *e; int ei;
  limnPart *r; int ri;
  
  _limnPSPreamble(obj, cam, win);

  for (ri=0; ri<obj->rA->len; ri++) {
    r = &(obj->r[ri]);

    fprintf(win->file, "\n%% faces for part %d (%d)\n\n", ri, r->origIdx);
    for (fi=0; fi<r->fNum; fi++) {
      f = &(obj->f[r->fBase + fi]);
      f->visib = f->sn[2] < 0;
      if (f->visib)
	_limnPSDrawFace(obj, f, cam, map, win);
    }

    fprintf(win->file, "0 setgray\n");

    fprintf(win->file, "%% ...... edges ......\n");
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
      _limnPSDrawEdge(obj, e, cam, win);
    }
  }

  _limnPSEpilogue(obj, cam, win);

  return 0;
}
