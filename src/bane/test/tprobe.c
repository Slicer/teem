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


#include <stdio.h>
#include <bane.h>

int
main(int argc, char *argv[]) {
  char *me;
  float ans[BANE_PROBE_ANSLEN], *data, x, y, z;
  baneProbeK3Pack *pack;
  Nrrd *nin, *nout;
  int query, size, bx, by, bz, xi, yi, zi, ho;
  double t0, t1;

  me = argv[0];
  baneProbeDebug = 0;

  pack = baneProbeK3PackNew();


  pack->k[0] = nrrdKernelBCCubic;
  pack->k[1] = nrrdKernelBCCubicD;
  pack->k[2] = nrrdKernelBCCubicDD;

  pack->param[0][0] = 1.0; pack->param[0][1] = 1.0; pack->param[0][2] = 0;
  pack->param[1][0] = 1.0; pack->param[1][1] = 1.0; pack->param[1][2] = 0;
  pack->param[2][0] = 1.0; pack->param[2][1] = 1.0; pack->param[2][2] = 0;

  /*
  pack->param[0][0] = 1.0; pack->param[0][1] = 0.0; pack->param[0][2] = 0.5;
  pack->param[1][0] = 1.0; pack->param[1][1] = 0.0; pack->param[1][2] = 0.5;
  pack->param[2][0] = 1.0; pack->param[2][1] = 0.0; pack->param[2][2] = 0.5;
  */

  /*
  pack->k[0] = nrrdKernelAQuartic;
  pack->k[1] = nrrdKernelAQuarticD;
  pack->k[2] = nrrdKernelAQuarticDD;
  pack->param[0][0] = 1.0; pack->param[0][1] = 0.25;
  pack->param[1][0] = 1.0; pack->param[1][1] = 0.25;
  pack->param[2][0] = 1.0; pack->param[2][1] = 0.25;
  */

  /*
  printf("v = [\n");
  for (xi=0; xi<=30; xi++) {
    x = AIR_AFFINE(0, xi, 30, -2, 2);
    y = nrrdKernelBCCubic->eval(x, pack->param[0]);
    z = nrrdKernelBCCubicD->eval(x, pack->param[0]);
    printf("%g %g %g;\n", x, y, z);
  }
  printf("]\n");
  */

  if (!(nin = nrrdNewLoad(argv[1]))) {
    fprintf(stderr, "%s: can't open it.\n", me);
  }
  query = BANE_PROBE_GRADMAG | BANE_PROBE_2NDDD | BANE_PROBE_HESS;
  ho = baneProbeAnsOffset[baneProbeHess];

  bx = 4;
  by = 4;
  bz = 4;
  size = 100;
  nout = nrrdNewAlloc(6*size*size*size, nrrdTypeFloat, 4);
  nout->size[0] = 6;
  nout->size[1] = size;
  nout->size[2] = size;
  nout->size[3] = size;
  data = nout->data;
  t0 = airTime();
  for (zi=0; zi<=size-1; zi++) {
    z = AIR_AFFINE(0, zi, size-1, bx, bx+100);
    for (yi=0; yi<=size-1; yi++) {
      y = AIR_AFFINE(0, yi, size-1, by, by+100);
      for (xi=0; xi<=size-1; xi++) {
	x = AIR_AFFINE(0, xi, size-1, bz, bz+100);
	baneProbe(ans, nin, query, pack, x, y, z);
	
	data[0 + 6*(xi + size*(yi + size*zi))] = ans[0 + ho];
	data[1 + 6*(xi + size*(yi + size*zi))] = ans[1 + ho];
	data[2 + 6*(xi + size*(yi + size*zi))] = ans[2 + ho];
	data[3 + 6*(xi + size*(yi + size*zi))] = ans[3 + ho];
	data[4 + 6*(xi + size*(yi + size*zi))] = ans[4 + ho];
	data[5 + 6*(xi + size*(yi + size*zi))] = ans[5 + ho];

	/*
	data[xi + size*(yi + size*zi)] = 
	  ans[baneProbeAnsOffset[baneProbeGradMag]];
	*/
	/*
	data[xi + size*(yi + size*zi)] = *ans;
	*/
      }
    }
  }
  t1 = airTime();
  printf("probe rate = %g/sec\n", size*size*size/(t1-t0));
  
  nrrdSave("out.nrrd", nout);

  nrrdNuke(nin);
  nrrdNuke(nout);
  baneProbeK3PackNix(pack);
  exit(0);
}
