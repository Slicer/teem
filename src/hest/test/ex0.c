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
  static int res[2], v, numIn;
  static char **in, *out;
  hestOpt opt[] = {
    {"res",   "sx sy", airTypeInt,    2,  2,   res,  NULL, 
     "image resolution"},
    {"v",     "level", airTypeInt,    1,  1,   &v,   "0",
     "verbosity level"},
    {"out",   "file",  airTypeString, 1,  1,   &out, "output.ppm",
     "PPM image output"},
    {NULL,    "input",  airTypeString, 1, -1,   &in,  NULL,
     "input image file(s)", &numIn},
    {NULL, NULL, 0}
  };
  char *err = NULL;

  if (hestOptCheck(opt, &err)) {
    fprintf(stderr, "ERROR: %s\n", err); free(err); 
    exit(1);
  }
  printf("hestOpt array looks fine.\n");
  exit(0);
}
