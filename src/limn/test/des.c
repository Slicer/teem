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


#include <limn.h>

char *me;

int
main(int argc, char *argv[]) {
  limnObj *obj;

  me = argv[0];

  obj = limnObjNew(AIR_TRUE);
  limnObjCubeAdd(obj, 2, 0.2, 0.4, 0.6);
  limnObjCubeAdd(obj, 2, 2, 4, 6);
  limnObjDescribe(stdout, obj);
  
  obj = limnObjNuke(obj);
  return 0;
}
