/*
  teem: Gordon Kindlmann's research software
  Copyright (C) 2003, 2002, 2001, 2000, 1999, 1998 University of Utah

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


#include "../alan.h"

int
main(int argc, char *argv[]) {
  alanContext *actx;
  char *err, *me;

  me = argv[0];

  airSrand();
  actx = alanContextNew();
  if (alanDimensionSet(actx, 2)
      || alan2DSizeSet(actx, 128, 128)
      || alanParmSet(actx, alanParmMaxIteration, 10000)
      || alanParmSet(actx, alanParmVerbose, 1)
      || alanParmSet(actx, alanParmTextureType, alanTextureTypeTuring)
      || alanParmSet(actx, alanParmRandRange, 0.1)
      || alanParmSet(actx, alanParmK, 0.0125)
      || alanParmSet(actx, alanParmSaveInterval, 0)
      || alanParmSet(actx, alanParmFrameInterval, 10)
      ) {
    err = biffGetDone(ALAN);
    fprintf(stderr, "%s: trouble: %s\n", me, err); 
    free(err); return 1;
  }

  if (alanRun(actx)) {
    err = biffGetDone(ALAN);
    fprintf(stderr, "%s: trouble: %s\n", me, err); 
    free(err); return 1;
  }
  fprintf(stderr, "%s: stop = %d\n", me, actx->stop);
  nrrdSave("lev0.nrrd", actx->nlev[0], NULL);
  nrrdSave("lev1.nrrd", actx->nlev[1], NULL);
  
  return 0;
}
