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


#include "ell.h"

/*
******** ellDebug
**
** some functions may use this value to control printing of
** verbose debugging information
*/
int ellDebug = 0;


void
ell4mPrint_f(FILE *f, float s[16]) {

  fprintf(f, "% 15.7f % 15.7f % 15.7f % 15.7f\n", 
	  s[0], s[4], s[8], s[12]);
  fprintf(f, "% 15.7f % 15.7f % 15.7f % 15.7f\n", 
	  s[1], s[5], s[9], s[13]);
  fprintf(f, "% 15.7f % 15.7f % 15.7f % 15.7f\n", 
	  s[2], s[6], s[10], s[14]);
  fprintf(f, "% 15.7f % 15.7f % 15.7f % 15.7f\n", 
	  s[3], s[7], s[11], s[15]);
}

void
ell4mPrint_d(FILE *f, double s[16]) {

  fprintf(f, "% 31.15lf % 31.15lf % 31.15lf % 31.15lf\n", 
	  s[0], s[4], s[8], s[12]);
  fprintf(f, "% 31.15lf % 31.15lf % 31.15lf % 31.15lf\n", 
	  s[1], s[5], s[9], s[13]);
  fprintf(f, "% 31.15lf % 31.15lf % 31.15lf % 31.15lf\n", 
	  s[2], s[6], s[10], s[14]);
  fprintf(f, "% 31.15lf % 31.15lf % 31.15lf % 31.15lf\n", 
	  s[3], s[7], s[11], s[15]);
}

