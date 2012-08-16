/*
  Teem: Tools to process and visualize scientific data and images              
  Copyright (c) 2012, 2011, 2010, 2009  University of Chicago
  Copyright (C) 2008, 2007, 2006, 2005  Gordon Kindlmann
  Copyright (C) 2004, 2003, 2002, 2001, 2000, 1999, 1998  University of Utah

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public License
  (LGPL) as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  The terms of redistributing and/or modifying this software also
  include exceptions to the LGPL that facilitate static linking.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with this library; if not, write to Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/


#include "teem/air.h"

/*
** to test:

AIR_EXPORT FILE *airFopen(const char *name, FILE *std, const char *mode);
AIR_EXPORT FILE *airFclose(FILE *file);
AIR_EXPORT int airSinglePrintf(FILE *file, char *str, const char *fmt, ...);
AIR_EXPORT unsigned int airIndex(double min, double val, double max,
                                 unsigned int N);
AIR_EXPORT unsigned int airIndexClamp(double min, double val, double max,
                                      unsigned int N);
AIR_EXPORT airULLong airIndexULL(double min, double val, double max,
                                 airULLong N);
AIR_EXPORT airULLong airIndexClampULL(double min, double val, double max,
                                      airULLong N);
AIR_EXPORT char *airDoneStr(double start, double here, double end, char *str);
AIR_EXPORT void airBinaryPrintUInt(FILE *file, int digits, unsigned int N);
AIR_EXPORT int airILoad(void *v, int t);
AIR_EXPORT float airFLoad(void *v, int t);
AIR_EXPORT double airDLoad(void *v, int t);
AIR_EXPORT int airIStore(void *v, int t, int i);
AIR_EXPORT float airFStore(void *v, int t, float f);
AIR_EXPORT double airDStore(void *v, int t, double d);
AIR_EXPORT void airEqvAdd(airArray *eqvArr, unsigned int j, unsigned int k);
AIR_EXPORT unsigned int airEqvMap(airArray *eqvArr,
                                  unsigned int *map, unsigned int len);
AIR_EXPORT unsigned int airEqvSettle(unsigned int *map, unsigned int len);

*/

int
main(int argc, const char *argv[]) {
  const char *me;
  void *ptr, *ptr2;

  AIR_UNUSED(argc);
  me = argv[0];

  /* airNull */
  {
    ptr = airNull();
    if (NULL != ptr) {
      fprintf(stderr, "%s: airNull() returned %p not NULL\n", me, ptr);
      exit(1);
    }
  }

  /* airSetNull */
  {
    ptr = AIR_CAST(void *, airNull);
    if (NULL == ptr) {
      fprintf(stderr, "%s: couldn't set a non-NULL pointer", me);
      exit(1);
    }
    ptr2 = airSetNull(&ptr);
    if (!(NULL == ptr && NULL == ptr2)) {
      fprintf(stderr, "%s: airSetNull() didn't set (%p) or return (%p) NULL\n",
              me, ptr, ptr2);
      exit(1);
    }
  }

  /* airFree, airTime */
  {
    unsigned int big = 1024, times = 200*1024, ii;
    big = big*big*big/2; /* half Gig */
    double time0, dtime;
    time0 = airTime();
    for (ii=0; ii<times; ii++) {
      ptr = AIR_CALLOC(big, char);
      ptr = airFree(ptr);
    }
    /* will run out of memory with less than 100 terabytes of memory,
       if airFree() didn't free() */
    if (!(NULL == ptr)) {
      fprintf(stderr, "%s: airFree() returned %p not NULL\n", me, ptr);
      exit(1);
    }
    dtime = airTime() - time0;
    if (!( dtime > 0 )) {
      fprintf(stderr, "%s: airTime() => nonsense delta time %g\n", me, dtime);
      exit(1);
    }
  }

  /* airStderr, airStdout, airStdin */
  {
    FILE *fret;
    fret = airStderr();
    if (stderr != fret) {
      fprintf(stderr, "%s: airStderr() returned %p not stderr %p\n", me,
              AIR_CAST(void *, fret), AIR_CAST(void *, stderr));
      exit(1);
    }
    fret = airStdout();
    if (stdout != fret) {
      fprintf(stdout, "%s: airStdout() returned %p not stdout %p\n", me,
              AIR_CAST(void *, fret), AIR_CAST(void *, stdout));
      exit(1);
    }
    fret = airStdin();
    if (stdin != fret) {
      fprintf(stdin, "%s: airStdin() returned %p not stdin %p\n", me,
              AIR_CAST(void *, fret), AIR_CAST(void *, stdin));
      exit(1);
    }
  }

  /* airSprintSize_t, airSprintPtrdiff_t in pptest.c */
  
  /* airPrettySprintSize_t */
  {
    char prstmp[AIR_STRLEN_SMALL];
    size_t vals[] = {0,  /* 0 */
                     800, /* 1 */
                     1024, /* 2 */
                     1024 + 1,   /* 3 */
                     500*1024, /* 4 */ 
                     1024*1024,  /* 5 */
                     1024*(1024 + 1), /* 6 */
                     500*1024*1024, /* 7 */
                     1024*1024*1024ul, /* 8 */
                     1024*1024*(1024ul + 1), /* 9 */
                     500*1024*1024*1024ul, /* 10 */
                     1024*1024*1024*1024ul, /* 11 */
                     1024*1024*1024*(1024ul + 1), /* 12 */
                     0};
    char *string[] = {
      "0 bytes",
      "800 bytes",
      "1024 bytes",
      "1.00098 KB",
      "500 KB",
      "1024 KB",
      "1.00098 MB",
      "500 MB",
      "1024 MB",
      "1.00098 GB",
      "500 GB",
      "1024 GB",
      "1.00098 TB"};
    unsigned int ii;
    for (ii=0; !ii || vals[ii]; ii++) {
      airPrettySprintSize_t(prstmp, vals[ii]);
      if (strcmp(string[ii], prstmp)) {
        fprintf(stderr, "%s: airPrettySprintSize_t made |%s| not |%s|\n",
                me, prstmp, string[ii]);
        exit(1);
      }
    }
  }


  exit(0);
}



