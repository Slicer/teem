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


#include "../biff.h"

int
main() {
  char *tmp;

  /*
  biffSet("axis", "the first error axis");
  biffAdd("axis", "the second error axis");
  biffAdd("axis", "the third error axis");
  biffSet("chard", "the first error chard");
  biffAdd("chard", "the second error chard");
  biffAdd("chard", "the third error chard");
  biffAdd("bingo", "zero-eth bingo message");
  biffMove("bingo", NULL, "chard");
  biffAdd("bingo", "the first error bingo");
  biffAdd("bingo", "the second bll boo boo boo error bingo");
  biffAdd("bingo", "the third error bingo");
  printf("%s\n", (tmp = biffGet("bingo")));
  free(tmp);
  biffDone("bingo");
  printf("%s\n", (tmp = biffGet("chard")));
  free(tmp);
  biffDone("chard");
  printf("%s\n", (tmp = biffGet("axis")));
  free(tmp);
  biffDone("axis");
  
  biffSet("harold", "the first error harold");
  biffAdd("harold", "the second error harold");
  biffAdd("harold", "the third error harold");
  printf("%s\n", (tmp = biffGet("harold")));
  free(tmp);
  */
  biffSet("axis", "the first error axis");
  biffAdd("axis", "the second error axis");
  biffAdd("axis", "the third error axis");
  biffAdd("axis", "the fourth error axis");
  biffAdd("axis", "the fifth error axis");
  printf("%s\n", (tmp = biffGet("axis")));
  free(tmp);
  biffDone("axis");
}



