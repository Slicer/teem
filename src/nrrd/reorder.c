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


#include "nrrd.h"

/*
******** nrrdPermuteAxes
**
** changes the scanline ordering of the data in a nrrd
** 
** This is a newer version of the function which is simpler and 
** requires no memory overhead, compared to the older version.
** However, it creates the new nrrd one element at a time, not
** taking advantage of any memory coherence that may be better
** served by memcpy().
*/
int
nrrdPermuteAxes(Nrrd *nin, Nrrd *nout, int *axes) {
  char err[NRRD_MED_STRLEN], me[] = "nrrdPermuteAxes", tmpstr[512];
  NRRD_BIG_INT I,            /* I don't need to justify every single var */
    tmp;                     /* divided and mod'ed to produce coords */
  char *src, *dest;
  int used[NRRD_MAX_DIM],    /* records times a given axis is listed
				in permuted order (should only be once) */
    coord[NRRD_MAX_DIM],     /* holder for coordinates (in new nrrd) of 
				current point */
    d,                       /* running index along dimensions */
    elSize;                  /* size of one element */

  if (!(nin && nout && axes)) {
    sprintf(err, "%s: invalid args", me);
    biffSet(NRRD, err); return 1;
  }
  memset(used, 0, NRRD_MAX_DIM*sizeof(int));
  for (d=0; d<=nin->dim-1; d++) {
    if (!AIR_INSIDE(0, axes[d], nin->dim-1)) {
      sprintf(err, "%s: axis#%d == %d out of bounds", me, d, axes[d]);
      biffSet(NRRD, err); return 1;
    }
    used[axes[d]] += 1;
  }
  for (d=0; d<=nin->dim-1; d++) {
    if (1 != used[d]) {
      sprintf(err, "%s: axis %d used %d times, instead of once", 
	      me, d, used[d]);
      biffSet(NRRD, err); return 1;
    }
  }
  if (!(nout->data)) {
    if (nrrdAlloc(nout, nin->num, nin->type, nin->dim)) {
      sprintf(err, "%s: nrrdAlloc() failed to create slice", me);
      biffAdd(NRRD, err); return 1;
    }
  }

  /* produce array of coordinates inside original array of the
  ** elements that comprise the volume.  We go linearly through the
  ** indices of the permuted volume, and then div and mod this to
  ** produce the necessary coordinates (of successive elements of
  ** the permuted volume, in the coordinate space of the original)
  **
  ** We are not (at this point) trying to be clever about memory
  ** coherence- the chunks being memcpy()d are the size of elements,
  ** not scanlines, or anything else larger.
  */
  src = nin->data;
  dest = nout->data;
  elSize = nrrdElementSize(nin);
  for (I=0; I<=nin->num-1; I++) {
    tmp = I;
    for (d=0; d<=nin->dim-1; d++) {
      coord[axes[d]] = tmp % nin->size[axes[d]];
      tmp /= nin->size[axes[d]];
    }
    /* now go from coordinates to linear index (in new space) */
    tmp = coord[nin->dim-1];
    for (d=nin->dim-2; d>=0; d--)
      tmp = coord[d] + nin->size[d]*tmp;
    memcpy(dest + I*elSize, src + tmp*elSize, elSize);
  }

  /* set information in new volume */
  for (d=0; d<=nin->dim-1; d++) {
    nout->size[d] = nin->size[axes[d]];
    nout->spacing[d] = nin->spacing[axes[d]];
    nout->axisMin[d] = nin->axisMin[axes[d]];
    nout->axisMax[d] = nin->axisMax[axes[d]];
    strcpy(nout->label[d], nin->label[axes[d]]);
  }
  sprintf(nout->content, "permute(%s,", nin->content);
  for (d=0; d<=nin->dim-1; d++) {
    sprintf(tmpstr, "%d%c", axes[d], d == nin->dim-1 ? ')' : ',');
    strcat(nout->content, tmpstr);
  }
  nout->blockSize = nin->blockSize;
  nin->min = airNand();
  nin->max = airNand();

  /* bye */
  return 0;
}

/*
******** nrrdNewPermuteAxes
**
*/
Nrrd *
nrrdNewPermuteAxes(Nrrd *nin, int *axes) {
  char err[NRRD_MED_STRLEN], me[] = "nrrdNewPermuteAxes";
  Nrrd *nout;

  if (!(nout = nrrdNew())) {
    sprintf(err, "%s: nrrdNew() failed", me);
    biffAdd(NRRD, err);
    return NULL;
  }
  if (nrrdPermuteAxes(nin, nout, axes)) {
    sprintf(err, "%s: nrrdPermuteAxes() failed", me);
    biffAdd(NRRD, err);
    nrrdNuke(nout);
    return NULL;
  }
  return nout;
}


/*
******** nrrdShuffle
**
** rearranges hyperslices of a nrrd along a given axis according
** to given permutation.  This could be used, for example, to
** re-order the elements along the 0th axis when the nrrd is a
** 4D array representing a volume of vectors.
**
** the given permutation array must allocated for at least as long
** as the input nrrd along the chosen axis.  perm[j] = i means that
** the value at position j in the _new_ array should come from
** position i in the _old_array.  The standpoint is from the new,
** looking at where to find the values amid the old array.  This
** allows multiple positions in the new array to copy from the same
** old position, and insures that there is an source for all positions
** along the new array.
*/
int
nrrdShuffle(Nrrd *nin, Nrrd *nout, int axis, int *perm) {
  char err[NRRD_MED_STRLEN], me[]="nrrdShuffle";
  int typesize, *size, d, dim, ci[NRRD_MAX_DIM+1], co[NRRD_MAX_DIM+1];
  NRRD_BIG_INT I, idxI, idxO, N;
  unsigned char *dataI, *dataO;

  if (!(nin && nout && perm)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffSet(NRRD, err);
    return 1;
  }
  if (!AIR_INSIDE(0, axis, nin->dim-1)) {
    sprintf(err, "%s: axis %d outside valid range [0,%d]", 
	    me, axis, nin->dim-1);
    biffSet(NRRD, err);
    return 1;
  }
  if (!(nout->data)) {
    /* HEY!!: this is just a hack for the time being */
    if (nrrdCopy(nin, nout)) {
      sprintf(err, "%s: failed to allocate output", me);
      biffAdd(NRRD, err); return 1;
    }
  }
  
  N = nin->num;
  size = nin->size;
  dim = nin->dim;
  dataI = nin->data;
  dataO = nout->data;
  typesize = nrrdTypeSize[nin->type];
  memset(ci, 0, (NRRD_MAX_DIM+1)*sizeof(int));
  memset(co, 0, (NRRD_MAX_DIM+1)*sizeof(int));
  for (I=0; I<=N-1; I++) {
    memcpy(ci, co, NRRD_MAX_DIM*sizeof(int));
    ci[axis] = perm[co[axis]];
    idxI = ci[dim-1];
    idxO = co[dim-1];
    for (d=dim-2; d>=0; d--) {
      idxI = ci[d] + size[d]*idxI;
      idxO = co[d] + size[d]*idxO;
    }
    memcpy(dataO + typesize*idxO, dataI + typesize*idxI, typesize);
    d = 0;
    co[d]++;
    while (d <= dim-1 && co[d] == size[d]) {
      co[d] = 0;
      d++;
      co[d]++; 
    }
  }
  return 0;
}

