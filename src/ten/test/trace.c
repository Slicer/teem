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

#include "../ten.h"

int
main(int argc, char *argv[]) {
  char *me, *err, *outS;
  hestOpt *hopt;
  hestParm *hparm;
  airArray *mop;
  Nrrd *dtvol, *nfib;
  double start[3], step;
  tenFiberContext *tfx;
  

  me = argv[0];
  mop = airMopNew();
  hparm = hestParmNew();
  hopt = NULL;
  airMopAdd(mop, hparm, (airMopper)hestParmFree, airMopAlways);
  hestOptAdd(&hopt, "i", "nin", airTypeOther, 1, 1, &dtvol, NULL,
	     "diffusion tensor volume", NULL, NULL, nrrdHestNrrd);
  hestOptAdd(&hopt, "s", "start pos", airTypeDouble, 3, 3, start, NULL,
	     "where to start tracking, in index space");
  hestOptAdd(&hopt, "o", "nout", airTypeString, 1, 1, &outS, "-",
	     "filename of output nrrd");
  hestOptAdd(&hopt, "step", "stepsize", airTypeDouble, 1, 1, &step, "0.01",
	     "stepsize along fiber");
  hestParseOrDie(hopt, argc-1, argv+1, hparm,
		 me, "test fiber tracking", AIR_TRUE, AIR_TRUE, AIR_TRUE);
  airMopAdd(mop, hopt, (airMopper)hestOptFree, airMopAlways);
  airMopAdd(mop, hopt, (airMopper)hestParseFree, airMopAlways);

  tfx = tenFiberContextNew(dtvol);
  airMopAdd(mop, tfx, (airMopper)tenFiberContextNix, airMopAlways);
  if (tenFiberStyleSet(tfx, tenFiberStyleEvec1)
      || tenFiberUpdate(tfx)) {
    airMopAdd(mop, err = biffGetDone(TEN), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble:\n%s\n", me, err);
    airMopError(mop); exit(1);
  }
  tfx->maxHalfLen = 10;
  tfx->step = step;

  nfib = nrrdNew();
  airMopAdd(mop, nfib, (airMopper)nrrdNuke, airMopAlways);
  if (tenFiberTrace(tfx, nfib, start[0], start[1], start[2])) {
    airMopAdd(mop, err = biffGetDone(TEN), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble:\n%s\n", me, err);
    airMopError(mop); exit(1);
  }
  fprintf(stderr, "%s: stop[backward] = %d; stop[forward] = %d\n", 
	  me, tfx->stop[0], tfx->stop[1]);
  if (nrrdSave(outS, nfib, NULL)) {
    airMopAdd(mop, err=biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble writing:\n%s\n", me, err);
    airMopError(mop); exit(1);
  }
  
  airMopOkay(mop);
  exit(0);
}
