/*
  Teem: Tools to process and visualize scientific data and images             .
  Copyright (C) 2013, 2012, 2011, 2010, 2009  University of Chicago
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

#include "unrrdu.h"
#include "privateUnrrdu.h"

#define INFO "Draws ASCII-art box plots"
static const char *_unrrdu_aabplotInfoL =
  (INFO
   ".  Because why not.\n "
   "* (uses nrrd, but no Nrrd implements this functionality)");

int
unrrdu_aabplotMain(int argc, const char **argv, const char *me,
                   hestParm *hparm) {
  /* these are stock for unrrdu */
  hestOpt *opt = NULL;
  airArray *mop;
  int pret;
  char *err;
  /* these are specific to this command */
  int medshow, rshow;
  Nrrd *_nin, *nin, *_nsingle, *nsingle;
  unsigned int plen;
  double vrange[2], *single;

  hestOptAdd(&opt, "l", "len", airTypeUInt, 1, 1, &plen, "78",
             "number of characters in box plot");
  hestOptAdd(&opt, "r", "min max", airTypeDouble, 2, 2, vrange, "0 100",
             "values to use as absolute min and max (unfortunately "
             "has to be same for all scanlines (rows).");
  hestOptAdd(&opt, "rs", "show", airTypeBool, 1, 1, &rshow, "false",
             "show range above plots");
  hestOptAdd(&opt, "ms", "show", airTypeBool, 1, 1, &medshow, "false",
             "print the median value");
  OPT_ADD_NIN(_nin, "input nrrd");
  hestOptAdd(&opt, "s", "single", airTypeOther, 1, 1, &_nsingle, "",
             "if given a 1D nrrd here that matches the number of "
             "rows in the \"-i\" input, interpret it as a list of values "
             "that should be indicated with \"X\"s in the plots.",
             NULL, NULL, nrrdHestNrrd);

  mop = airMopNew();
  airMopAdd(mop, opt, (airMopper)hestOptFree, airMopAlways);
  USAGE(_unrrdu_aabplotInfoL);
  PARSE();
  airMopAdd(mop, opt, (airMopper)hestParseFree, airMopAlways);

  if (!( 2 == _nin->dim || 1 == _nin->dim )) {
    fprintf(stderr, "%s: need 1-D or 2-D array\n", me);
    airMopError(mop);
    return 1;
  }
  nin = nrrdNew();
  airMopAdd(mop, nin, (airMopper)nrrdNuke, airMopAlways);
  if (nrrdConvert(nin, _nin, nrrdTypeDouble)) {
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: error converting \"-s\" input:\n%s", me, err);
    airMopError(mop);
    return 1;
  }
  if (1 == nin->dim) {
    if (nrrdAxesInsert(nin, nin, 1)) {
      airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
      fprintf(stderr, "%s: error making 2-D from 1-D:\n%s", me, err);
      airMopError(mop);
      return 1;
    }
  }
  if (_nsingle) {
    if (nrrdElementNumber(_nsingle) != nin->axis[1].size) {
      fprintf(stderr, "%s: \"-s\" input doesn't match size of \"-i\" input");
      airMopError(mop);
      return 1;
    }
    nsingle = nrrdNew();
    airMopAdd(mop, nsingle, (airMopper)nrrdNuke, airMopAlways);
    if (nrrdConvert(nsingle, _nsingle, nrrdTypeDouble)) {
      airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
      fprintf(stderr, "%s: error converting \"-s\" input:\n%s", me, err);
      airMopError(mop);
      return 1;
    }
    single = (double*)nsingle->data;
  } else {
    nsingle = NULL;
    single = NULL;
  }

  {
#define PTNUM 5
    double *in, *buff, ptile[PTNUM]={5,25,50,75,95};
    unsigned int xi, yi, pi, ti, ltt, sx, sy, pti[PTNUM];
    char *line, rbuff[128];
    Nrrd *nbuff;

    sx = AIR_CAST(unsigned int, nin->axis[0].size);
    sy = AIR_CAST(unsigned int, nin->axis[1].size);
    nbuff = nrrdNew();
    airMopAdd(mop, nbuff, (airMopper)nrrdNuke, airMopAlways);
    if (nrrdSlice(nbuff, nin, 1, 0)) {
      airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
      fprintf(stderr, "%s: error making buffer:\n%s", me, err);
      airMopError(mop);
      return 1;
    }
    line = calloc(plen+1, sizeof(char));
    in = (double*)nin->data;
    buff = (double*)nbuff->data;

    if (rshow) {
      for (pi=0; pi<plen; pi++) {
        line[pi] = ' ';
      }
      sprintf(rbuff, "|<-- %g", vrange[0]);
      memcpy(line, rbuff, strlen(rbuff));
      sprintf(rbuff, "%g -->|", vrange[1]);
      memcpy(line + plen - strlen(rbuff), rbuff, strlen(rbuff));
      printf("%s", line);
      if (medshow) {
        printf(" median");
      }
      printf("\n");
    }
    for (yi=0; yi<sy; yi++) {
      for (xi=0; xi<sx; xi++) {
        buff[xi] = in[xi + sx*yi];
      }
      qsort(buff, sx, sizeof(double), nrrdValCompare[nrrdTypeDouble]);
      for (ti=0; ti<PTNUM; ti++) {
        pti[ti] = airIndexClamp(vrange[0],
                                buff[airIndexClamp(0, ptile[ti], 100, sx)],
                                vrange[1], plen);
        /*
        fprintf(stderr, "ti %u (%g) -> buff[%u] = %g -> %u\n", ti,
                ptile[ti], airIndexClamp(0, ptile[ti], 100, sx),
                buff[airIndexClamp(0, ptile[ti], 100, sx)], pti[ti]);
        */
      }
      ltt = (unsigned int)(-1);
      for (pi=0; pi<plen; pi++) {
        line[pi] = pi % 2 ? ' ' : '.';
      }
      for (pi=pti[0]; pi<=pti[4]; pi++) {
        line[pi] = '-';
      }
      for (pi=pti[1]; pi<=pti[3]; pi++) {
        line[pi] = '=';
      }
      line[pti[2]]='m';
      if (pti[2] > 0) {
        line[pti[2]-1]='<';
      }
      if (pti[2] < plen-1) {
        line[pti[2]+1]='>';
      }
      if (single) {
        line[airIndexClamp(vrange[0], single[yi], vrange[1], plen)]='X';
      }
      printf("%s", line);
      if (medshow) {
        printf(" %g", buff[airIndexClamp(0, 50, 100, sx)]);
      }
      printf("\n");
#if 0
      /* printf("["); */
      for (pi=0; pi<plen; pi++) {
        for (tt=0; tt<PTNUM && pti[tt] < pi; tt++) {
          /*
          fprintf(stderr, "(pi %u < pti[%u]==%u)", pi, tt, pti[tt]);
          */
        }
        /* fprintf(stderr, " --> tt=%u\n", tt); */
        if (2 == ltt && 3 == tt) {
          printf("M");
        } else {
          printf("%c", cc[tt]);
        }
        ltt = tt;
      }
      /* printf("]\n"); */
      printf("\n");
#endif
    }
  }
  airMopOkay(mop);
  return 0;
}

UNRRDU_CMD_HIDE(aabplot, INFO);
