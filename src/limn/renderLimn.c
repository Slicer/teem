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
limnObjectRender(limnObject *obj, limnCamera *cam, limnWindow *win) {
  char me[]="limnObjectRender", err[AIR_STRLEN_MED];
  int E;
  
  E = 0;
  if (!E) E |= limnCameraUpdate(cam);
  fprintf(stderr, "%s: true up = %g %g %g\n", me,
	  -cam->V[0], -cam->V[1], -cam->V[2]);
  fprintf(stderr, "%s: true right = %g %g %g\n", me,
	  cam->U[0], cam->U[1], cam->U[2]);
  if (!E) E |= limnObjectHomog(obj, limnSpaceWorld);
  if (!E) E |= limnObjectNormals(obj, limnSpaceWorld);
  if (!E) E |= limnObjectSpaceTransform(obj, cam, win, limnSpaceView);
  if (!E) E |= limnObjectSpaceTransform(obj, cam, win, limnSpaceScreen);
  if (!E) E |= limnObjectSpaceTransform(obj, cam, win, limnSpaceDevice);
  if (!E) E |= limnObjectNormals(obj, limnSpaceScreen);
  if (E) {
    sprintf(err, "%s: trouble", me);
    biffAdd(LIMN, err); return 1;
  }
  return 0;
}

void
_limnPSPreamble(limnObject *obj, limnCamera *cam, limnWindow *win) {
  
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
_limnPSEpilogue(limnObject *obj, limnCamera *cam, limnWindow *win) {

  fprintf(win->file, "grestore\n");
  fprintf(win->file, "grestore\n");
  if (win->ps.showpage) {
    fprintf(win->file, "showpage\n");
  }
  fprintf(win->file, "%%%%Trailer\n");
}

void
_limnPSDrawFace(limnObject *obj, limnFace *face,
		limnCamera *cam, Nrrd *nmap, limnWindow *win) {
  int vii;
  limnVertex *vert;
  limnLook *look;
  limnPart *part;
  int qn;
  float *map, R, G, B;

  look = obj->look + face->lookIdx;
  part = obj->part + face->partIdx;
  for (vii=0; vii<face->sideNum; vii++) {
    vert = obj->vert + part->vertIdx[face->vertIdxIdx[vii]];
    fprintf(win->file, "%g %g %s\n", 
	    vert->device[0], vert->device[1], vii ? "L" : "M");
  }
  R = look->kads[0]*look->rgba[0];
  G = look->kads[0]*look->rgba[1];
  B = look->kads[0]*look->rgba[2];
  if (nmap) {
    qn = limnVtoQN_f[limnQN16checker](face->worldNormal);
    map = nmap->data;
    R += look->kads[1]*look->rgba[0]*map[0 + 3*qn];
    G += look->kads[1]*look->rgba[1]*map[1 + 3*qn];
    B += look->kads[1]*look->rgba[2]*map[2 + 3*qn];
  } else {
    R += look->kads[1]*look->rgba[0];
    G += look->kads[1]*look->rgba[1];
    B += look->kads[1]*look->rgba[2];
  }
  /* HEY: not evaluating phong specular for now */
  R = AIR_CLAMP(0, R, 1);
  G = AIR_CLAMP(0, G, 1);
  B = AIR_CLAMP(0, B, 1);
  if (R == G && G == B) {
    fprintf(win->file, "CP %g Gr F\n", R);
  }
  else {
    fprintf(win->file, "CP %g %g %g RGB F\n", R, G, B);
  }
}

void
_limnPSDrawEdge(limnObject *obj, limnEdge *edge,
		limnCamera *cam, limnWindow *win) {
  limnVertex *vert0, *vert1;
  limnPart *part;

  part = obj->part + edge->partIdx;
  if (win->ps.lineWidth[edge->type]) {
    vert0 = obj->vert + part->vertIdx[edge->vertIdxIdx[0]];
    vert1 = obj->vert + part->vertIdx[edge->vertIdxIdx[1]];
    fprintf(win->file, "%g %g M ", vert0->device[0], vert0->device[1]);
    fprintf(win->file, "%g %g L ", vert1->device[0], vert1->device[1]);
    fprintf(win->file, "%g W 0 Gr ", win->ps.lineWidth[edge->type]);
    fprintf(win->file, "S\n");
  }
}

/*
******** limnObjectPSDraw
**
** draws a "rendered" limn object to postscript.
** limnObjectRender MUST be called first.
**
** The current (feeble) justification for using an environment map is
** that its an expressive way of shading things based on surface
** normal, in a context where, if flat shading is all you have,
** correct specular lighting is not possible
*/
int
limnObjectPSDraw(limnObject *obj, limnCamera *cam,
		 Nrrd *nmap, limnWindow *win) {
  char me[]="limnObjectPSDraw", err[AIR_STRLEN_MED];
  int inside;
  float angle;
  limnFace *face, *face0, *face1; int fii;
  limnEdge *edge; int eii;
  limnPart *part; int partIdx;
  limnVertex *vert; int vii;

  if (nmap) {
    if (limnEnvMapCheck(nmap)) {
      sprintf(err, "%s: trouble", me); 
      biffAdd(LIMN, err); return 1;
    }
  }
  
  limnObjectDepthSortParts(obj);
  
  _limnPSPreamble(obj, cam, win);

  for (partIdx=0; partIdx<obj->partNum; partIdx++) {
    part = obj->part + partIdx;

    /* only draw the parts that are inside the field of view */
    inside = 0;
    for (vii=0; vii<part->vertIdxNum; vii++) {
      vert = obj->vert + part->vertIdx[vii];
      inside |= (AIR_IN_CL(win->bbox[0], vert->device[0], win->bbox[2]) &&
		 AIR_IN_CL(win->bbox[1], vert->device[1], win->bbox[3]));
      if (inside) {
	/* at least vertex is in, we know we can't skip this part */
	break;
      }
    }
    if (!inside) {
      /* none of the vertices were in, we can skip this part */
      continue;
    }

    /* draw the part */
    if (1 == part->edgeIdxNum) {
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
      win->ps.edgeWidth[e->visible] = 4;
      _limnPSDrawEdge(obj, r, e, cam, win);
      win->ps.edgeWidth[e->visible] = widthTmp;
      */
    } else {
      /* this part is either a lone face or a solid:
	 draw the front-facing, shaded faces */
      for (fii=0; fii<part->faceIdxNum; fii++) {
	face = obj->face + part->faceIdx[fii];
	/* The consequence of having a left-handed frame is that world-space
	   CC-wise vertex traversal becomes C-wise screen-space traversal, so
	   all the normals are backwards of what we want */
	face->visible = (cam->rightHanded 
			 ? face->screenNormal[2] < 0
			 : face->screenNormal[2] > 0);
	if (face->sideNum == part->vertIdxNum && !face->visible) {
	  /* lone faces are always visible */
	  face->visible = AIR_TRUE;
	  ELL_3V_SCALE(face->worldNormal, -1, face->worldNormal);
	}
	if (!win->ps.wireFrame && face->visible) {
	  _limnPSDrawFace(obj, face, cam, nmap, win);
	}
      }
      
      /* draw ALL edges */
      fprintf(win->file, "0 setgray\n");
      for (eii=0; eii<part->edgeIdxNum; eii++) {
	edge = obj->edge + part->edgeIdx[eii];
	face0 = obj->face + part->faceIdx[edge->faceIdxIdx[0]];
	face1 = (-1 == edge->faceIdxIdx[1]
		 ? NULL
		 : obj->face + part->faceIdx[edge->faceIdxIdx[1]]);
	if (!face1) {
	  edge->type = limnEdgeTypeBorder;
	} else {
	  angle = 180/M_PI*acos(ELL_3V_DOT(face0->worldNormal, 
					   face1->worldNormal));
	  if (face0->visible && face1->visible) {
	    edge->type = (angle > win->ps.creaseAngle
			  ? limnEdgeTypeFrontCrease
			  : limnEdgeTypeFrontFacet);
	  } else if (face0->visible ^ face1->visible) {
	    edge->type = limnEdgeTypeContour;
	  } else {
	    edge->type = (angle > win->ps.creaseAngle
			  ? limnEdgeTypeBackCrease
			  : limnEdgeTypeBackFacet);
	  }
	}
	_limnPSDrawEdge(obj, edge, cam, win);
      }
    }
  }

  _limnPSEpilogue(obj, cam, win);

  return 0;
}

/*
******** limnObjectPSDrawConcave
**
** new version of the above, which works per-face instead of per-part,
** thus better handling self-occlusions, but at the cost of not getting
** contours near oblique faces correct...
*/
int
limnObjectPSDrawConcave(limnObject *obj, limnCamera *cam,
		     Nrrd *nmap, limnWindow *win) {
  char me[]="limnObjectPSDrawConcave", err[AIR_STRLEN_MED];
  float angle;
  limnPart *part;
  limnFace *face, *face0, *face1; int faceIdx;
  limnEdge *edge; int edgeIdx, eii;

  if (nmap) {
    if (limnEnvMapCheck(nmap)) {
      sprintf(err, "%s: trouble", me); 
      biffAdd(LIMN, err); return 1;
    }
  }
  
  limnObjectDepthSortFaces(obj);

  _limnPSPreamble(obj, cam, win);

  /* set every face's visibility */
  for (faceIdx=0; faceIdx<obj->faceNum; faceIdx++) {
    face = obj->face + faceIdx;
    part = obj->part + face->partIdx;
    face->visible = (cam->rightHanded 
		     ? face->screenNormal[2] < 0
		     : face->screenNormal[2] > 0);
    if (face->sideNum == part->vertIdxNum && !face->visible) {
      /* lone faces are always visible */
      face->visible = AIR_TRUE;
      ELL_3V_SCALE(face->worldNormal, -1, face->worldNormal);
    }
  }
  
  /* categorize all edges by traversing edge array, and looking
     at each of their two faces */
  for (edgeIdx=0; edgeIdx<obj->edgeNum; edgeIdx++) {
    edge = obj->edge + edgeIdx;
    part = obj->part + edge->partIdx;
    face0 = obj->face + part->faceIdx[edge->faceIdxIdx[0]];
    face1 = (-1 == edge->faceIdxIdx[1]
	     ? NULL
	     : obj->face + part->faceIdx[edge->faceIdxIdx[1]]);
    if (!face1) {
      edge->type = limnEdgeTypeBorder;
    } else {
      angle = 180/M_PI*acos(ELL_3V_DOT(face0->worldNormal,
				       face1->worldNormal));
      if (face0->visible && face1->visible) {
	edge->type = (angle > win->ps.creaseAngle
		      ? limnEdgeTypeFrontCrease
		      : limnEdgeTypeFrontFacet);
      } else if (face0->visible ^ face1->visible) {
	edge->type = limnEdgeTypeContour;
      } else {
	edge->type = (angle > win->ps.creaseAngle
		      ? limnEdgeTypeBackCrease
		      : limnEdgeTypeBackFacet);
      }
    }
  }

  /* draw front-faces and their edges
     (contours, front crease, front non-crease) */
  for (faceIdx=0; faceIdx<obj->faceNum; faceIdx++) {
    face = obj->face + obj->faceSort[faceIdx];
    part = obj->part + face->partIdx;
    if (!face->visible) {
      continue;
    }
    if (!win->ps.wireFrame) {
      _limnPSDrawFace(obj, face, cam, nmap, win);
    }
    /* draw those edges around the face that won't be seen again by 
       future faces in the depth-first traversal */
    for (eii=0; eii<face->sideNum; eii++) {
      edge = obj->edge + part->edgeIdx[face->edgeIdxIdx[eii]];
      if (limnEdgeTypeContour == edge->type) {
	_limnPSDrawEdge(obj, edge, cam, win);
      } else if (limnEdgeTypeFrontCrease == edge->type 
		 || limnEdgeTypeFrontFacet == edge->type) {
	if (edge->once) {
	  /* its been seen once already, okay to draw */
	  _limnPSDrawEdge(obj, edge, cam, win);
	  edge->once = AIR_FALSE;
	} else {
	  /* we're the first to see it, and we're not the last, don't draw */
	  edge->once = AIR_TRUE;
	}
      }
    }
  }

  _limnPSEpilogue(obj, cam, win);

  return 0;
}
