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

#include <teem/air.h>
#include <teem/hest.h>

char *info = ("Converts from DOS text files to unix (converting CR-LF pairs "
	      "to just LF).  Note: there is no capability to convert to DOS "
	      "text files, because there is no legitimate or rational "
	      "reason for ever doing this. ");

int
main(int argc, char *argv[]) {
  char *me, **names;
  int lenNames, ni;
  hestOpt *hopt = NULL;
  airArray *mop;

  me = argv[0];
  hestOptAdd(&hopt, NULL, "file1 ", airTypeString, 1, -1, &names, NULL,
	     "all the files to convert.  Each file will be over-written "
	     "with its converted contents.  Makes an effort to not meddle "
	     "with binary files.  Use \"-\" to read from stdin "
	     "and write to stdout", &lenNames);
  hestParseOrDie(hopt, argc-1, argv+1, NULL, me, info,
		 AIR_TRUE, AIR_TRUE, AIR_TRUE);
  mop = airMopNew();
  airMopAdd(mop, hopt, (airMopper)hestOptFree, airMopAlways);
  airMopAdd(mop, hopt, (airMopper)hestParseFree, airMopAlways);

  for (ni=0; ni<lenNames; ni++) {
    fprintf(stderr, "%d: \"%s\"\n", ni, names[ni]);
  }
  
  airMopOkay(mop);
  return 0;
}
