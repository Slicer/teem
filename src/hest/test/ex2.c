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
#include "air.h"
#include "hest.h"

int
main(int argc, char **argv) {
  int res[2], v, numIn;
  char **in, *out;
  int n;
  hestOpt *opt = NULL;
  hestParm *parm;
  char *err = NULL, info[] = 
    "This program does nothing in particular, though it does attempt "
    "to pose as some sort of command-line image processing program. "
    "As usual, any implied functionality is purely coincidental, "
    "especially since this is the output of a unicyclist.";

  parm = hestParmNew();
  parm->respFileEnable = AIR_TRUE;

  opt = NULL;
  hestOptAdd(&opt, "res",   "sx sy", airTypeInt,    2,  2,  res,  NULL,
	     "image resolution");
  hestOptAdd(&opt, "v",     "level", airTypeInt,    0,  1,  &v,   "0",
	     "verbosity level");
  hestOptAdd(&opt, "out",   "file",  airTypeString, 1,  1,  &out, "output.ppm",
	     "PPM image output");
  hestOptAdd(&opt, NULL,    "input", airTypeString, 1, -1,  &in,  NULL,
	     "input image file(s)", &numIn);
  
  if (1 == argc) {
    /* didn't get anything at all on command line */
    /* print program information ... */
    hestInfo(stderr, argv[0], info, parm);
    /* ... and usage information ... */
    hestUsage(stderr, opt, argv[0], parm);
    hestGlossary(stderr, opt, parm);
    /* ... and avoid memory leaks */
    opt = hestOptFree(opt);
    parm = hestParmFree(parm);
    exit(1);
  }

  /* else we got something, see if we can parse it */
  if (hestParse(opt, argc-1, argv+1, &err, parm)) {
    fprintf(stderr, "ERROR: %s\n", err); free(err);
    /* print usage information ... */
    hestUsage(stderr, opt, argv[0], parm);
    hestGlossary(stderr, opt, parm);
    /* ... and then avoid memory leaks */
    opt = hestOptFree(opt);
    parm = hestParmFree(parm);
    exit(1);
  }
  
  printf("(err = %s)\n", err);
  printf("res = %d %d\n", res[0], res[1]);
  printf("  v = %d\n", v);
  printf("out = \"%s\"\n", out);
  printf(" in = %d files:", numIn);
  for (n=0; n<=numIn-1; n++) {
    printf(" \"%s\"", in[n]);
  }
  printf("\n");

  /* free the memory allocated by parsing ... */
  hestParseFree(opt);
  /* ... and the other stuff */
  opt = hestOptFree(opt);
  parm = hestParmFree(parm);
  exit(0);
}
