/*
  teem: Gordon Kindlmann's research software
  Copyright (C) 2002, 2001, 2000, 1999, 1998 University of Utah

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

#include "../nrrd.h"

int
main(int argc, char *argv[]) {
  char *me, *ninS, *noutS, *err;
  Nrrd *nin;

  me = argv[0];
  if (3 != argc) {
    /*                       0   1     2   (3) */
    fprintf(stderr, "usage: %s <nin> <nout>\n", me);
    exit(1);
  }
  ninS = argv[1];
  noutS = argv[2];
  nin = nrrdNew();
  if (nrrdLoad(nin, ninS)) {
    fprintf(stderr, "%s: couldn't open nrrd \"%s\":\n%s", me, ninS,
	    err = biffGetDone(NRRD));
    free(err); exit(1);
  }
  if (nrrdSave(noutS, nin, NULL)) {
    fprintf(stderr, "%s: trouble saving nrrd to \"%s\":\n%s", me, noutS,
	    err = biffGetDone(NRRD));
    free(err); exit(1);
  }
  nrrdNuke(nin);
  exit(0);
}
