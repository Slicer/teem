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

#include <ctype.h>
#include <errno.h>

#include <teem/air.h>
#include <teem/hest.h>

char *info = ("Converts from DOS text files to normal (converting LF-CR pairs "
	      "to just CR), or, with the \"-r\" option, convert back to DOS, "
	      "for whatever sick and twisted reason you'd have to do that. ");

#define CR 10
#define LF 13
#define BAD_PERC 5.0

void
undosConvert(char *me, char *name, int reverse) {
  airArray *mop;
  FILE *fin, *fout;
  char *data=NULL;
  airArray *dataArr;
  int ci, car, numBad;

  mop = airMopNew();
  if (!airStrlen(name)) {
    fprintf(stderr, "%s: empty filename\n", me);
    airMopError(mop); return;
  }

  /* open input file */
  fin = airFopen(name, stdin, "rb");
  if (!fin) {
    fprintf(stderr, "%s: couldn't open \"%s\" for reading: \"%s\"\n", 
	    me, name, strerror(errno));
    airMopError(mop); return;
  }
  airMopAdd(mop, fin, (airMopper)airFclose, airMopOnError);

  /* create buffer */
  dataArr = airArrayNew((void**)&data, NULL, sizeof(char), AIR_STRLEN_HUGE);
  if (!dataArr) {
    fprintf(stderr, "%s: internal allocation error #1\n", me);
    airMopError(mop); return;
  }
  airMopAdd(mop, dataArr, (airMopper)airArrayNuke, airMopAlways);

  /* read input file, testing for binary-ness along the way */
  numBad = 0;
  car = getc(fin);
  if (EOF == car) {
    fprintf(stderr, "%s: \"%s\" was empty, skipping ...\n", me, name);
    airMopError(mop); return;
  }
  do {
    ci = airArrayIncrLen(dataArr, 1);
    if (-1 == ci) {
      fprintf(stderr, "%s: internal allocation error #2\n", me);
      airMopError(mop); return;
    }
    data[ci] = car;
    numBad += !(isprint(data[ci]) || isspace(data[ci]));
    car = getc(fin);
  } while (EOF != car && BAD_PERC > 100.0*numBad/dataArr->len);
  if (EOF != car) {
    fprintf(stderr, "%s: more than %g%% of \"%s\" is non-printing, "
	    "skipping ...\n", me, BAD_PERC, name);
    airMopError(mop); return;    
  }
  fin = airFclose(fin);

  /* open output file */
  fout = airFopen(name, stdout, "wb");
  if (!fout) {
    fprintf(stderr, "%s: couldn't open \"%s\" for writing: \"%s\"\n", 
	    me, name, strerror(errno));
    airMopError(mop); return;
  }
  airMopAdd(mop, fout, (airMopper)airFclose, airMopOnError);

  /* write output file */
  car = 'a';
  if (reverse) {
    for (ci=0; ci<dataArr->len; ci++) {
      if (CR == data[ci]) {
	car = putc(LF, fout);
	if (EOF != car) {
	  car = putc(CR, fout);
	}
      } else {
	car = putc(data[ci], fout);
      }
    }
  } else {
    for (ci=0; EOF != car && ci<dataArr->len; ci++) {
      if (LF == data[ci] && (ci+1<dataArr->len && CR == data[ci+1])) {
	car = putc(CR, fout);
	ci++;
      } else {
	car = putc(data[ci], fout);
      }
    }
  }
  if (EOF == car) {
    fprintf(stderr, "%s: ERROR writing \"%s\" (sorry!)\n", me, name);
  }
  fout = airFclose(fout);

  airMopOkay(mop);
  return;
}

int
main(int argc, char *argv[]) {
  char *me, **name;
  int lenName, ni, reverse;
  hestOpt *hopt = NULL;
  airArray *mop;

  me = argv[0];
  hestOptAdd(&hopt, "r", NULL, airTypeInt, 0, 0, &reverse, NULL,
	     "convert back to DOS, instead of convert from DOS to normal");
  hestOptAdd(&hopt, NULL, "file1 ", airTypeString, 1, -1, &name, NULL,
	     "all the files to convert.  Each file will be over-written "
	     "with its converted contents.  Makes an effort to not meddle "
	     "with binary files.  Use \"-\" to read from stdin "
	     "and write to stdout", &lenName);
  hestParseOrDie(hopt, argc-1, argv+1, NULL, me, info,
		 AIR_TRUE, AIR_TRUE, AIR_TRUE);
  mop = airMopNew();
  airMopAdd(mop, hopt, (airMopper)hestOptFree, airMopAlways);
  airMopAdd(mop, hopt, (airMopper)hestParseFree, airMopAlways);

  for (ni=0; ni<lenName; ni++) {
    undosConvert(me, name[ni], reverse);
  }
  
  airMopOkay(mop);
  return 0;
}
