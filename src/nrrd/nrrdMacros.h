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


#ifndef NRRD_MACROS_HAS_BEEN_INCLUDED
#define NRRD_MACROS_HAS_BEEN_INCLUDED
#ifdef __cplusplus
extern "C" {
#endif

/*
******** NRRD_COORD_UPDATE
**
** This is for doing the "carrying" associated with gradually
** incrementing an array of coordinates.  Assuming that the
** given coordinate array "coord" has been incrementing by adding
** 1 to one of its elements (NB: this is a strong assumption),
** then, this macro is good for propagating the change up to
** higher axes, when the position has stepped over the limit on
** a lower axis.  Relies on the array of axes sizes "size", as
** as the length "dim" of "coord" and "size".  Relies also on a
** temporary variable "d".
**
** This may be turned into something more general purpose soon.
*/
#define NRRD_COORD_UPDATE(coord, size, dim, d)     \
for ((d)=0;                                        \
     (d) < (dim)-1 && (coord)[(d)] == (size)[(d)]; \
     (d)++) {                                      \
  (coord)[(d)] = 0;                                \
  (coord)[(d)+1]++;                                \
}

/*
******** NRRD_COORD_INCR
**
** same as NRRD_COORD_UPDATE, but starts by incrementing coord[0]
*/
#define NRRD_COORD_INCR(coord, size, dim, d)       \
for ((coord)[0]++, (d)=0;                          \
     (d) < (dim)-1 && (coord)[(d)] == (size)[(d)]; \
     (d)++) {                                      \
  (coord)[(d)] = 0;                                \
  (coord)[(d)+1]++;                                \
}

/*
******** NRRD_COORD_INDEX
**
** Given a coordinate array "coord", as well as the array sizes "size"
** and dimension "dim" (and temporary variable "d"), calculates
** the linear index, and stores it in I.  
*/
#define NRRD_COORD_INDEX(coord, size, dim, d, I)   \
for ((d)=(dim)-1, (I)=(coord)[(d)--];              \
     (d) >= 0;                                     \
     (d)--) {                                      \
  (I) = (coord)[(d)] + (size)[(d)]*(I);            \
}

/* extern C */
#ifdef __cplusplus
}
#endif
#endif /* NRRD_MACROS_HAS_BEEN_INCLUDED */
