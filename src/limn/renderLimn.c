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
limnObjRender(limnObj *obj, limnCamera *cam, limnWin *win) {
  char me[]="limnObjRender", err[AIR_STRLEN_MED];
  int E;
  
  E = 0;
  if (!E) E |= limnCameraUpdate(cam);
  fprintf(stderr, "%s: true up = %g %g %g\n", me,
	  -cam->V[0], -cam->V[1], -cam->V[2]);
  fprintf(stderr, "%s: true right = %g %g %g\n", me,
	  cam->U[0], cam->U[1], cam->U[2]);
  if (!E) E |= limnObjHomog(obj, limnSpaceWorld);
  if (!E) E |= limnObjNormals(obj, limnSpaceWorld);
  if (!E) E |= limnObjSpaceTransform(obj, cam, win, limnSpaceView);
  if (!E) E |= limnObjSpaceTransform(obj, cam, win, limnSpaceScreen);
  if (!E) E |= limnObjSpaceTransform(obj, cam, win, limnSpaceDevice);
  if (!E) E |= limnObjNormals(obj, limnSpaceScreen);
  if (E) {
    sprintf(err, "%s: trouble", me);
    biffAdd(LIMN, err); return 1;
  }
  return 0;
}

void
_limnPSPreamble(limnObj *obj, limnCamera *cam, limnWin *win) {
  
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
  fprintf(win->file, "gsave %g %g %g setrgbcolor fill grestore\n",
	  win->ps.bg[0], win->ps.bg[1], win->ps.bg[2]);
  fprintf(win->file, "clip\n");
  fprintf(win->file, "gsave newpath\n");
  fprintf(win->file, "1 setlinejoin\n");
  fprintf(win->file, "1 setlinecap\n");
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
_limnPSEpilogue(limnObj *obj, limnCamera *cam, limnWin *win) {

  fprintf(win->file, "grestore\n");
  fprintf(win->file, "grestore\n");
  if (win->ps.showpage) {
    fprintf(win->file, "showpage\n");
  }
  fprintf(win->file, "%%%%Trailer\n");
}

void
_limnPSDrawFace(limnObj *obj, limnPart *r, limnFace *f, 
		limnCamera *cam, Nrrd *nmap, limnWin *win) {
  int vi;
  limnPoint *p;
  limnSP *sp;
  int qn;
  float *map, R, G, B;

  for (vi=0; vi<f->vNum; vi++) {
    p = obj->p + obj->v[vi + f->vBase];
    fprintf(win->file, "%g %g %s\n", 
	    p->d[0], p->d[1], vi ? "L" : "M");
  }
  sp = obj->s + f->sp;
  /*
  fprintf(stderr, "RGB = %g %g %g ->", r->rgba[0], r->rgba[1], r->rgba[2]);
  */
  R = sp->k[0]*sp->rgba[0];
  G = sp->k[0]*sp->rgba[1];
  B = sp->k[0]*sp->rgba[2];
  /*
  fprintf(stderr, "RGB = %g %g %g ->", R, G, B);
  */
  if (nmap) {
    qn = limnVtoQN_f[limnQN16checker](f->wn);
    map = nmap->data;
    R += sp->k[1]*sp->rgba[0]*map[0 + 3*qn];
    G += sp->k[1]*sp->rgba[1]*map[1 + 3*qn];
    B += sp->k[1]*sp->rgba[2]*map[2 + 3*qn];
  } else {
    R += sp->k[1]*sp->rgba[0];
    G += sp->k[1]*sp->rgba[1];
    B += sp->k[1]*sp->rgba[2];
  }
  /* HEY: not evaluating phong specular for now */
  /*
  fprintf(stderr, "%g %g %g ->", R, G, B);
  */
  R = AIR_CLAMP(0, R, 1);
  G = AIR_CLAMP(0, G, 1);
  B = AIR_CLAMP(0, B, 1);
  /*
  fprintf(stderr, "%g %g %g\n", R, G, B);
  */
  if (R == G && G == B) {
    fprintf(win->file, "CP %g Gr F\n", R);
  }
  else {
    fprintf(win->file, "CP %g %g %g RGB F\n", R, G, B);
  }
}

void
_limnPSDrawEdge(limnObj *obj, limnPart *r, limnEdge *e, 
		limnCamera *cam, limnWin *win) {
  limnPoint *p0, *p1;

  if (win->ps.lineWidth[e->type]) {
    p0 = obj->p + e->v0;
    p1 = obj->p + e->v1;
    fprintf(win->file, "%g %g M ", p0->d[0], p0->d[1]);
    fprintf(win->file, "%g %g L ", p1->d[0], p1->d[1]);
    fprintf(win->file, "%g W ", win->ps.lineWidth[e->type]);
    fprintf(win->file, "S\n");
  }
}

/*
******** limnObjPSDrawOld
**
** draws a "rendered" limn object to postscript.
** limnObjRender MUST be called first.
**
** The current (feeble) justification for using an environment map is
** that its an expressive way of shading things based on surface
** normal, in a context where, if flat shading is all you have,
** correct specular lighting is not possible
*/
int
limnObjPSDrawOld(limnObj *obj, limnCamera *cam, Nrrd *nmap, limnWin *win) {
  char me[]="limnObjPSDraw", err[AIR_STRLEN_MED];
  int inside;
  float angle;
  limnFace *f, *f0, *f1; int fi;
  limnEdge *e; int ei;
  limnPart *r; int ri;
  limnPoint *p; int pi;

  if (nmap) {
    if (limnEnvMapCheck(nmap)) {
      sprintf(err, "%s: trouble", me); 
      biffAdd(LIMN, err); return 1;
    }
  }
  
  limnObjDepthSortParts(obj);

  _limnPSPreamble(obj, cam, win);

  for (ri=0; ri<obj->rA->len; ri++) {
    r = &(obj->r[ri]);

    /* only draw the parts that are inside the field of view */
    inside = 0;
    for (pi=0; pi<r->pNum; pi++) {
      p = &(obj->p[r->pBase + pi]);
      /*
      printf("p[%d] = %g %g\n", pi, p->d[0], p->d[1]);
      */
      inside |= (AIR_IN_CL(win->bbox[0], p->d[0], win->bbox[2]) &&
		 AIR_IN_CL(win->bbox[1], p->d[1], win->bbox[3]));
      if (inside)
	break;
    }
    if (!inside)
      continue;

    /* draw the part */
    if (1 == r->eNum) {
      /* this part is just one lone edge */
      /* HEY: this is a mess */
      /*
      e = &(obj->e[r->eBase]);
      widthTmp = win->ps.lineWidth[e->type];
      fprintf(win->file, "%g setgray\n", 1 - win->ps.bg[0]);
      win->ps.edgeWidth[e->type] = 8;
      _limnPSDrawEdge(obj, r, e, cam, win);
      fprintf(win->file, "%g %g %g RGB\n", 
	      r->rgba[0], r->rgba[1], r->rgba[2]);
      win->ps.edgeWidth[e->visib] = 4;
      _limnPSDrawEdge(obj, r, e, cam, win);
      win->ps.edgeWidth[e->visib] = widthTmp;
      */
    } else {
      /* this part is either a lone face or a solid */

      /* draw the front-facing, shaded faces */
      for (fi=0; fi<r->fNum; fi++) {
	f = &(obj->f[r->fBase + fi]);
	/*
	printf("f[%d]->sn = %g %g %g\n", fi, f->sn[0], f->sn[1], f->sn[2]);
	*/
	/* The consequence of having a left-handed frame is that world-space
	   CC-wise vertex traversal becomes C-wise screen-space traversal, so
	   all the normals are backwards of what we want */
	f->visib = (cam->rightHanded 
		    ? f->sn[2] < 0
		    : f->sn[2] > 0);
	if (f->vNum == r->pNum && !f->visib) {
	  f->visib = AIR_TRUE;
	  ELL_3V_SCALE(f->wn, -1, f->wn);
	}
	if (!win->ps.wireFrame && f->visib) {
	  _limnPSDrawFace(obj, r, f, cam, nmap, win);
	}
      }
      
      /* draw ALL edges */
      fprintf(win->file, "0 setgray\n");
      for (ei=0; ei<r->eNum; ei++) {
	e = &(obj->e[r->eBase + ei]);
	f0 = &(obj->f[e->f0]);
	f1 = e->f1 != -1 ? &(obj->f[e->f1]) : NULL;
	if (!f1) {
	  e->type = limnEdgeTypeBorder;
	} else {
	  angle = 180/M_PI*acos(ELL_3V_DOT(f0->wn, f1->wn));
	  if (f0->visib && f1->visib) {
	    e->type = (angle > win->ps.creaseAngle
		       ? limnEdgeTypeFrontCrease
		       : limnEdgeTypeFrontFacet);
	  } else if (f0->visib ^ f1->visib) {
	    e->type = limnEdgeTypeContour;
	  } else {
	    e->type = (angle > win->ps.creaseAngle
		       ? limnEdgeTypeBackCrease
		       : limnEdgeTypeBackFacet);
	  }
	}
	_limnPSDrawEdge(obj, r, e, cam, win);
      }
    }
  }

  _limnPSEpilogue(obj, cam, win);

  return 0;
}

void
_limnPSDrawPointPair(limnObj *obj, limnPart *r,
		     int v0, int v1, int type,
		     limnCamera *cam, limnWin *win) {
  
  if (win->ps.lineWidth[type]) {
    fprintf(win->file, "%g %g M ", obj->p[v0].d[0], obj->p[v0].d[1]);
    fprintf(win->file, "%g %g L ", obj->p[v1].d[0], obj->p[v1].d[1]);
    fprintf(win->file, "%g W ", win->ps.lineWidth[type]);
    fprintf(win->file, "S\n");
  }
}

/*
** HEY: drawing non-convex objects has to be done one face at a time,
** and faces are responsible for drawing their edges.  The STUPID
** thing about limn right now is that faces do not have a list of 
** their edges; only a list of their vertex indices...
*/

/*
******** limnObjPSDraw
**
** new version of the above, which works per-face instead of per-part
*/
int
limnObjPSDraw(limnObj *obj, limnCamera *cam, Nrrd *nmap, limnWin *win) {
  char me[]="limnObjPSDraw", err[AIR_STRLEN_MED];
  int inside, vi, psize, v0, v1;
  float angle;
  limnFace *f, *f0, *f1; int fi;
  limnEdge *e; int ei;
  limnPart *r; int ri;
  limnPoint *p; int pi;
  Nrrd *neinfo;
  unsigned char *einfo;
  airArray *mop;

  if (nmap) {
    if (limnEnvMapCheck(nmap)) {
      sprintf(err, "%s: trouble", me); 
      biffAdd(LIMN, err); return 1;
    }
  }
  
  mop = airMopNew();
  neinfo = nrrdNew();
  airMopAdd(mop, neinfo, (airMopper)nrrdNuke, airMopAlways);
  psize = obj->pA->len;
  if (nrrdMaybeAlloc(neinfo, nrrdTypeUChar, 2, psize, psize)) {
    sprintf(err, "%s: couldn't allocate %d x %d array of point pairs",
	    me, obj->pA->len, obj->pA->len);
    biffMove(LIMN, err, NRRD); return 1;
  }
  einfo = (unsigned char*)(neinfo->data);

  limnObjDepthSortFaces(obj);

  _limnPSPreamble(obj, cam, win);
  
  /* categorize all edges by traversing edge array, and storing
     edge type information in einfo[] */
  for (ei=0; ei<obj->eA->len; ei++) {
    e = obj->e + ei;
    if (e->v0 < e->v1) {
      v0 = e->v0;
      v1 = e->v1;
    } else {
      v0 = e->v1;
      v1 = e->v0;
    }
    f0 = &(obj->f[e->f0]);
    f1 = e->f1 != -1 ? &(obj->f[e->f1]) : NULL;
    if (!f1) {
      einfo[v0 + psize*v1] = limnEdgeTypeBorder;
    } else {
      angle = 180/M_PI*acos(ELL_3V_DOT(f0->wn, f1->wn));
      if (f0->visib && f1->visib) {
	einfo[v0 + psize*v1] = (angle > win->ps.creaseAngle
				? limnEdgeTypeFrontCrease
				: limnEdgeTypeFrontFacet);
      } else if (f0->visib ^ f1->visib) {
	einfo[v0 + psize*v1] = limnEdgeTypeContour;
      } else {
	einfo[v0 + psize*v1] = (angle > win->ps.creaseAngle
				? limnEdgeTypeBackCrease
				: limnEdgeTypeBackFacet);
      }
    }
  }

  /* draw front-faces and non-back edges */
  for (fi=0; fi<obj->fA->len; fi++) {
    f = obj->f + obj->fSort[fi];
    f->visib = (cam->rightHanded 
		? f->sn[2] < 0
		: f->sn[2] > 0);
    if (f->vNum == r->pNum && !f->visib) {
      f->visib = AIR_TRUE;
      ELL_3V_SCALE(f->wn, -1, f->wn);
    }
    if (!win->ps.wireFrame && f->visib) {
      _limnPSDrawFace(obj, r, f, cam, nmap, win);
    }
    /*
    for (vi=0; vi<f->vNum; vi++) {
      p = obj->p + obj->v[vi + f->vBase];
      fprintf(win->file, "%g %g %s\n", 
	      p->d[0], p->d[1], vi ? "L" : "M");
    }
    */
  }

  _limnPSEpilogue(obj, cam, win);

  return 0;
}
