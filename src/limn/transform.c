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
_limnObjectWHomog(limnObject *obj) {
  int vertIdx;
  limnVertex *vert;
  float h;
  
  for (vertIdx=0; vertIdx<obj->vertNum; vertIdx++) {
    vert = obj->vert + vertIdx;
    h = 1.0/vert->world[3];
    ELL_3V_SCALE(vert->world, h, vert->world);
    vert->world[3] = 1.0;
  }
  
  return 0;
}

int
_limnObjectNormals(limnObject *obj, int space) {
  int vii, faceIdx;
  limnFace *face;
  limnPart *part;
  limnVertex *vert0, *vert1, *vert2;
  float vec1[3], vec2[3], cross[3], nn[3], norm;
  
  for (faceIdx=0; faceIdx<obj->faceNum; faceIdx++) {
    face = obj->face + faceIdx;
    part = obj->part[face->partIdx];
    /* add up cross products at all vertices */
    ELL_3V_SET(nn, 0, 0, 0);
    for (vii=0; vii<face->sideNum; vii++) {
      vert0 = obj->vert 
        + part->vertIdx[face->vertIdxIdx[vii]];
      vert1 = obj->vert 
        + part->vertIdx[face->vertIdxIdx[AIR_MOD(vii+1, face->sideNum)]];
      vert2 = obj->vert
        + part->vertIdx[face->vertIdxIdx[AIR_MOD(vii-1, face->sideNum)]];
      if (limnSpaceWorld == space) {
        ELL_3V_SUB(vec1, vert1->world, vert0->world);
        ELL_3V_SUB(vec2, vert2->world, vert0->world);
      }
      else {
        ELL_3V_SUB(vec1, vert1->screen, vert0->screen);
        ELL_3V_SUB(vec2, vert2->screen, vert0->screen);
      }
      ELL_3V_CROSS(cross, vec1, vec2);
      ELL_3V_ADD2(nn, nn, cross);
    }

    if (limnSpaceWorld == space) {
      ELL_3V_NORM(face->worldNormal, nn, norm);
      /*
      printf("%s: wn[%d] = %g %g %g\n", "_limnObjectNormals", faceIdx,
             f->wn[0], f->wn[1], f->wn[2]);
      */
    }
    else {
      ELL_3V_NORM(face->screenNormal, nn, norm);
      /*
      printf("%s: sn[%d] = %g %g %g\n", "_limnObjectNormals", faceIdx,
             f->sn[0], f->sn[1], f->sn[2]);
      */
    }
  }

  return 0;
}

int
_limnObjectVTransform(limnObject *obj, limnCamera *cam) {
  int vertIdx;
  limnVertex *vert;
  float d;

  for (vertIdx=0; vertIdx<obj->vertNum; vertIdx++) {
    vert = obj->vert + vertIdx;
    ELL_4MV_MUL(vert->view, cam->W2V, vert->world);
    d = 1.0/vert->world[3];
    ELL_4V_SCALE(vert->view, d, vert->view);
    /*
    printf("%s: w[%d] = %g %g %g %g --> v = %g %g %g\n", 
           "_limnObjectVTransform",
           pi, p->w[0], p->w[1], p->w[2], p->w[3], p->v[0], p->v[1], p->v[2]);
    */
  }
  return 0;
}

int
_limnObjectSTransform(limnObject *obj, limnCamera *cam) {
  int vertIdx;
  limnVertex *vert;
  float d;

  for (vertIdx=0; vertIdx<obj->vertNum; vertIdx++) {
    vert = obj->vert + vertIdx;
    d = (cam->orthographic 
         ? 1
         : cam->vspDist/vert->view[2]);
    vert->screen[0] = d*vert->view[0];
    vert->screen[1] = d*vert->view[1];
    vert->screen[2] = vert->view[2];
    /*
    printf("%s: v[%d] = %g %g %g --> s = %g %g %g\n", "_limnObjectSTransform",
           pi, p->v[0], p->v[1], p->v[2], p->s[0], p->s[1], p->s[2]);
    */
  }
  return 0;
}

int
_limnObjectDTransform(limnObject *obj, limnCamera *cam, limnWindow *win) {
  int vertIdx;
  limnVertex *vert;
  float wy0, wy1, wx0, wx1, t;
  
  wx0 = 0;
  wx1 = (cam->uRange[1] - cam->uRange[0])*win->scale;
  wy0 = 0;
  wy1 = (cam->vRange[1] - cam->vRange[0])*win->scale;
  ELL_4V_SET(win->bbox, wx0, wy0, wx1, wy1);
  if (win->yFlip) {
    ELL_SWAP2(wy0, wy1, t);
  }
  for (vertIdx=0; vertIdx<obj->vertNum; vertIdx++) {
    vert = obj->vert + vertIdx;
    vert->device[0] = AIR_AFFINE(cam->uRange[0], vert->screen[0],
                                  cam->uRange[1], wx0, wx1);
    vert->device[1] = AIR_AFFINE(cam->vRange[0], vert->screen[1],
                                  cam->vRange[1], wy0, wy1);
    /*
    printf("%s: s[%d] = %g %g --> s = %g %g\n", "_limnObjectDTransform",
           pi, p->s[0], p->s[1], p->d[0], p->d[1]);
    */
  }
  return 0;
}

int
limnObjectHomog(limnObject *obj, int space) {
  char me[]="limnObjectHomog";
  int ret;

  switch(space) {
  case limnSpaceWorld:
    ret = _limnObjectWHomog(obj);
    break;
  default:
    fprintf(stderr, "%s: space %d unknown or unimplemented\n", me, space);
    ret = 1;
    break;
  }
  
  return ret;
}

int
limnObjectNormals(limnObject *obj, int space) {
  char me[]="limnObjectNormals";
  int ret;
  
  switch(space) {
  case limnSpaceWorld:
  case limnSpaceScreen:
    ret = _limnObjectNormals(obj, space);
    break;
  default:
    fprintf(stderr, "%s: space %d unknown or unimplemented\n", me, space);
    ret = 1;
    break;
  }

  return ret;
}

int
limnObjectSpaceTransform(limnObject *obj, limnCamera *cam,
                      limnWindow *win, int space) {
  char me[]="limnObjectSpaceTransform";
  int ret;

  /* HEY: deal with cam->orthographic */
  switch(space) {
  case limnSpaceView:
    ret = _limnObjectVTransform(obj, cam);
    break;
  case limnSpaceScreen:
    ret = _limnObjectSTransform(obj, cam);
    break;
  case limnSpaceDevice:
    ret = _limnObjectDTransform(obj, cam, win);
    break;
  default:
    fprintf(stderr, "%s: space %d unknown or unimplemented\n", me, space);
    ret = 1;
    break;
  }

  return ret;
}

int
limnObjectPartTransform(limnObject *obj, int partIdx, float xform[16]) {
  int vertIdxIdx;
  limnPart *part;
  limnVertex *vert;
  float tmp[4];
  
  part= obj->part[partIdx];
  for (vertIdxIdx=0; vertIdxIdx<part->vertIdxNum; vertIdxIdx++) {
    vert = obj->vert + part->vertIdx[vertIdxIdx];
    ELL_4MV_MUL(tmp, xform, vert->world);
    ELL_4V_COPY(vert->world, tmp);
  }

  return 0;
}

int
_limnPartDepthCompare(const void *_a, const void *_b) {
  limnPart **a;
  limnPart **b;

  a = (limnPart **)_a;
  b = (limnPart **)_b;
  return AIR_COMPARE((*b)->depth, (*a)->depth);
}

int
limnObjectDepthSortParts(limnObject *obj) {
  limnPart *part;
  limnVertex *vert;
  limnFace *face;
  limnEdge *edge;
  int partIdx, ii;

  for (partIdx=0; partIdx<obj->partNum; partIdx++) {
    part = obj->part[partIdx];
    part->depth = 0;
    for (ii=0; ii<part->vertIdxNum; ii++) {
      vert = obj->vert + part->vertIdx[ii];
      part->depth += vert->screen[2];
    }
    part->depth /= part->vertIdxNum;
  }
  
  qsort(obj->part, obj->partNum, sizeof(limnPart*), _limnPartDepthCompare);
  
  /* re-assign partIdx, post-sorting */
  for (partIdx=0; partIdx<obj->partNum; partIdx++) {
    part = obj->part[partIdx];
    for (ii=0; ii<part->vertIdxNum; ii++) {
      vert = obj->vert + part->vertIdx[ii];
      vert->partIdx = partIdx;
    }
    for (ii=0; ii<part->edgeIdxNum; ii++) {
      edge = obj->edge + part->edgeIdx[ii];
      edge->partIdx = partIdx;
    }
    for (ii=0; ii<part->faceIdxNum; ii++) {
      face = obj->face + part->faceIdx[ii];
      face->partIdx = partIdx;
    }
  }

  return 0;
}

int
_limnFaceDepthCompare(const void *_a, const void *_b) {
  limnFace **a;
  limnFace **b;

  a = (limnFace **)_a;
  b = (limnFace **)_b;
  return -AIR_COMPARE((*a)->depth, (*b)->depth);
}

int
limnObjectDepthSortFaces(limnObject *obj) {
  limnFace *face;
  limnVertex *vert;
  limnPart *part;
  int faceIdx, vii;

  obj->faceSort = (limnFace **)calloc(obj->faceNum, sizeof(limnFace *));
  for (faceIdx=0; faceIdx<obj->faceNum; faceIdx++) {
    face = obj->face + faceIdx;
    part = obj->part[face->partIdx];
    face->depth = 0;
    for (vii=0; vii<face->sideNum; vii++) {
      vert = obj->vert + part->vertIdx[face->vertIdxIdx[vii]];
      face->depth += vert->screen[2];
    }
    face->depth /= face->sideNum;
    obj->faceSort[faceIdx] = face;
  }

  qsort(obj->faceSort, obj->faceNum,
        sizeof(limnFace *), _limnFaceDepthCompare);

  return 0;
}

int
limnObjectFaceReverse(limnObject *obj) {
  char me[]="limnObjectFaceReverse", err[AIR_STRLEN_MED];
  limnFace *face; int faceIdx;
  int *buff, sii;

  if (!obj) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(LIMN, err); return 1;
  }
  buff = NULL;
  for (faceIdx=0; faceIdx<obj->faceNum; faceIdx++) {
    face = obj->face + faceIdx;
    buff = (int *)calloc(face->sideNum, sizeof(int));
    if (!(buff)) {
      sprintf(err, "%s: couldn't allocate %d side buffer for face %d\n", 
              me, face->sideNum, faceIdx);
      biffAdd(LIMN, err); return 1;
    }
    memcpy(buff, face->vertIdxIdx, face->sideNum*sizeof(int));
    for (sii=0; sii<face->sideNum; sii++) {
      face->vertIdxIdx[sii] = buff[face->sideNum-1-sii];
    }
    memcpy(buff, face->edgeIdxIdx, face->sideNum*sizeof(int));
    for (sii=0; sii<face->sideNum; sii++) {
      face->edgeIdxIdx[sii] = buff[face->sideNum-1-sii];
    }
    free(buff);
  }
  return 0;
}
