/*
  teem: Gordon Kindlmann's research software
  Copyright (C) 2005  Gordon Kindlmann
  Copyright (C) 2004, 2003, 2002, 2001, 2000, 1999, 1998  University of Utah

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


#include "../air.h"

char *me;

int
main(int argc, char *argv[]) {
  char *fname, *multS, *nwritesS, *data;
  FILE *file;
  double time0, time1, ttime;
  int fd, align, mult, min, max, ret, nwrites, nn;
  size_t size;
  airArray *mop;

  me = argv[0];
  if (4 != argc) {
    /*                      0      1         2        3   (4) */
    fprintf(stderr, "usage: %s <filename> <mult> <# writes>\n", me);
    return 1;
  }
  fname = argv[1];
  multS = argv[2];
  nwritesS = argv[3];
  if (1 != sscanf(multS, "%d", &mult)) {
    fprintf(stderr, "%s: couln't parse mult %s as int\n", me, multS);
    return 1;
  }
  if (1 != sscanf(nwritesS, "%d", &nwrites)) {
    fprintf(stderr, "%s: couln't parse nwrites %s as int\n", me, nwritesS);
    return 1;
  }

  mop = airMopNew();
  if (!(file = fopen(fname, "w"))) {
    fprintf(stderr, "%s: couldn't open %s for writing\n", me, fname);
    airMopError(mop); return 1;
  }
  airMopAdd(mop, file, (airMopper)airFclose, airMopAlways);
  fd = fileno(file);
  if (-1 == fd) {
    fprintf(stderr, "%s: couldn't get underlying descriptor\n", me);
    airMopError(mop); return 1;
  }
  fprintf(stderr, "%s: fd(%s) = %d\n", me, fname, fd);

  ret = airDioTest(fd, NULL, 0);
  if (airNoDio_okay != ret) {
    fprintf(stderr, "%s: no good: \"%s\"\n", me, airNoDioErr(ret));
    airMopError(mop); return 1;
  }

  airDioInfo(&align, &min, &max, fd);
  fprintf(stderr, "%s: --> align=%d, min=%d, max=%d\n", me, align, min, max);
  size = (size_t)max*mult;
  data = airDioMalloc(size, fd);
  if (!data) {
    fprintf(stderr, "%s: airDioMalloc(" _AIR_SIZE_T_FMT ") failed\n", me,
            size);
    airMopError(mop); return 1;
  }
  airMopAdd(mop, data, airFree, airMopAlways);
  /*
  F_SETFL   Set file status flags to the third argument, arg, taken as an
    object of type int.  Only the following flags can be set [see
    fcntl(5)]:  FAPPEND, FSYNC, DSYNC, RSYNC, FNDELAY, FNONBLK,
    FLCFLUSH, FLCINVAL, FDIRECT, and FASYNC.
  */

  ttime = 0;
  for (nn=0; nn<nwrites; nn++) {
    time0 = airTime();
    if (size-1 != write(fd, data+1, size-1)) {
      fprintf(stderr, "%s: write %d failed\n", me, nn);
      airMopError(mop); return 1;
    }
    ttime += airTime() - time0;
  }
  time0 = airTime();
  fsync(fd);
  time1 = airTime();
  fprintf(stderr, "%s: time = %g + %g = %g\n", 
          me, ttime, time1 - time0, ttime + time1 - time0);

  airMopSub(mop, file, (airMopper)airFclose);
  fclose(file);
  file = fopen(fname, "w");
  airMopAdd(mop, file, (airMopper)airFclose, airMopAlways);
  fd = fileno(file);

  ttime = 0;
  for (nn=0; nn<nwrites; nn++) {
    time0 = airTime();
    if (size != write(fd, data, size)) {
      fprintf(stderr, "%s: write %d failed\n", me, nn);
      airMopError(mop); return 1;
    }
    ttime += airTime() - time0;
  }
  time0 = airTime();
  fsync(fd);
  time1 = airTime();
  fprintf(stderr, "%s: time = %g + %g = %g\n", 
          me, ttime, time1 - time0, ttime + time1 - time0);

  airMopSub(mop, file, (airMopper)airFclose);
  fclose(file);
  file = fopen(fname, "w");
  airMopAdd(mop, file, (airMopper)airFclose, airMopAlways);
  fd = fileno(file);

  ttime = 0;
  for (nn=0; nn<nwrites; nn++) {
    time0 = airTime();
    if (size != airDioWrite(fd, data, size)) {
      fprintf(stderr, "%s: write %d failed\n", me, nn);
      airMopError(mop); return 1;
    }
    ttime += airTime() - time0;
  }
  time0 = airTime();
  fsync(fd);
  time1 = airTime();
  fprintf(stderr, "%s: time = %g + %g = %g\n", 
          me, ttime, time1 - time0, ttime + time1 - time0);

  /* learned: with direct I/O, the time it normally takes for fsync()
     is basically zero, and all the time is in the write(), but all 
     together its still faster than no using direct I/O */

  airMopError(mop); 
  exit(0);
}
