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
  float ans[BANE_PROBE_ANSLEN], *data, x, y, z;
  baneProbeK3Pack *pack;
  Nrrd *nin, *nout;
  int query, size, bx, by, bz, xi, yi, zi;

  baneProbeDebug = 0;


  pack = baneProbeK3PackNew();
  pack->k0 = nrrdKernelBCCubic;
  pack->k1 = nrrdKernelBCCubicD;
  pack->param0[0] = 1.0; pack->param0[1] = 1.0; pack->param0[2] = 0;
  pack->param1[0] = 1.0; pack->param1[1] = 1.0; pack->param1[2] = 0;

  printf("v = [\n");
  for (xi=0; xi<=30; xi++) {
    x = AIR_AFFINE(0, xi, 30, -2, 2);
    y = nrrdKernel[nrrdKernelBCCubic].eval(x, pack->param0);
    z = nrrdKernel[nrrdKernelBCCubicD].eval(x, pack->param0);
    printf("%g %g %g;\n", x, y, z);
  }
  printf("]\n");

  /*
  pack = baneProbeK3PackNew();
  pack->k0 = nrrdKernelTent;
  pack->k1 = nrrdKernelForwDiff;
  */

  nin = nrrdNewOpen("engine-crop.nrrd");
  query = BANE_PROBE_GRADMAG;

  bx = 30;
  by = 80;
  bz = 10;
  size = 120;
  nout = nrrdNewAlloc(size*size*size, nrrdTypeFloat, 3);
  nout->size[0] = size;
  nout->size[1] = size;
  nout->size[2] = size;
  data = nout->data;
  for (zi=0; zi<=size-1; zi++) {
    z = AIR_AFFINE(0, zi, size-1, bx, bx+50);
    for (yi=0; yi<=size-1; yi++) {
      y = AIR_AFFINE(0, yi, size-1, by, by+50);
      for (xi=0; xi<=size-1; xi++) {
	x = AIR_AFFINE(0, xi, size-1, bz, bz+50);
	baneProbe(ans, nin, query, pack, x, y, z);
	data[xi + size*(yi + size*zi)] = ans[baneProbeAnsOffset[baneProbeGradMag]];
	/* data[xi + size*(yi + size*zi)] = *ans; */
      }
    }
  }
  
  nrrdSave("out.nrrd", nout);

  nrrdNuke(nin);
  nrrdNuke(nout);
  baneProbeK3PackNix(pack);
  exit(0);
}
