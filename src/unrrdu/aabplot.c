/*
  Teem: Tools to process and visualize scientific data and images
  Copyright (C) 2009--2023  University of Chicago
  Copyright (C) 2005--2008  Gordon Kindlmann
  Copyright (C) 1998--2004  University of Utah

  This library is free software; you can redistribute it and/or modify it under the terms
  of the GNU Lesser General Public License (LGPL) as published by the Free Software
  Foundation; either version 2.1 of the License, or (at your option) any later version.
  The terms of redistributing and/or modifying this software also include exceptions to
  the LGPL that facilitate static linking.

  This library is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
  PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License along with
  this library; if not, write to Free Software Foundation, Inc., 51 Franklin Street,
  Fifth Floor, Boston, MA 02110-1301 USA
*/

#include "unrrdu.h"
#include "privateUnrrdu.h"

#define INFO "Draws ASCII-art box plots"
static const char *_unrrdu_aabplotInfoL
  = (INFO ".  Because why not.\n "
          "* (uses nrrd, but no Nrrd function has this functionality)");

static int
unrrdu_aabplotMain(int argc, const char **argv, const char *me, hestParm *hparm) {
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

  hestOptAdd_1_UInt(&opt, "l", "len", &plen, "78", "number of characters in box plot");
  hestOptAdd_2_Double(&opt, "r", "min max", vrange, "0 100",
                      "values to use as absolute min and max (unfortunately "
                      "has to be same for all scanlines (rows).");
  hestOptAdd_1_Bool(&opt, "rs", "show", &rshow, "false", "show range above plots");
  hestOptAdd_1_Bool(&opt, "ms", "show", &medshow, "false", "print the median value");
  OPT_ADD_NIN(_nin, "input nrrd");
  hestOptAdd_1_Other(&opt, "s", "single", &_nsingle, "",
                     "if given a 1D nrrd here that matches the number of "
                     "rows in the \"-i\" input, interpret it as a list of values "
                     "that should be indicated with \"X\"s in the plots.",
                     nrrdHestNrrd);

  mop = airMopNew();
  airMopAdd(mop, opt, hestOptFree_vp, airMopAlways);
  USAGE_OR_PARSE(_unrrdu_aabplotInfoL);
  airMopAdd(mop, opt, (airMopper)hestParseFree, airMopAlways);

  if (!(2 == _nin->dim || 1 == _nin->dim)) {
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
      fprintf(stderr, "%s: \"-s\" input doesn't match size of \"-i\" input", me);
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
    single = (double *)nsingle->data;
  } else {
    nsingle = NULL;
    single = NULL;
  }

  {
#define PTNUM 5
    double *in, *buff, ptile[PTNUM] = {5, 25, 50, 75, 95};
    unsigned int xi, yi, pi, ti, sx, sy, pti[PTNUM];
    char *line, rbuff[128];
    Nrrd *nbuff;

    sx = AIR_UINT(nin->axis[0].size);
    sy = AIR_UINT(nin->axis[1].size);
    nbuff = nrrdNew();
    airMopAdd(mop, nbuff, (airMopper)nrrdNuke, airMopAlways);
    if (nrrdSlice(nbuff, nin, 1, 0)) {
      airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
      fprintf(stderr, "%s: error making buffer:\n%s", me, err);
      airMopError(mop);
      return 1;
    }
    line = AIR_CALLOC(plen + 1, char);
    in = (double *)nin->data;
    buff = (double *)nbuff->data;

    if (rshow) {
      for (pi = 0; pi < plen; pi++) {
        line[pi] = ' ';
      }
      sprintf(rbuff, "|<-- %g", vrange[0]);
      memcpy(line, rbuff, strlen(rbuff));
      sprintf(rbuff, "%g -->|", vrange[1]);
      memcpy(line + plen - strlen(rbuff), rbuff, strlen(rbuff));
      printf("%s", line);
      if (medshow) {
        printf(" median:");
      }
      printf("\n");
    }
    for (yi = 0; yi < sy; yi++) {
      for (xi = 0; xi < sx; xi++) {
        buff[xi] = in[xi + sx * yi];
      }
      qsort(buff, sx, sizeof(double), nrrdValCompare[nrrdTypeDouble]);
      for (ti = 0; ti < PTNUM; ti++) {
        pti[ti] = airIndexClamp(vrange[0], buff[airIndexClamp(0, ptile[ti], 100, sx)],
                                vrange[1], plen);
        /*
        fprintf(stderr, "ti %u (%g) -> buff[%u] = %g -> %u\n", ti,
                ptile[ti], airIndexClamp(0, ptile[ti], 100, sx),
                buff[airIndexClamp(0, ptile[ti], 100, sx)], pti[ti]);
        */
      }
      for (pi = 0; pi < plen; pi++) {
        line[pi] = pi % 2 ? ' ' : '.';
      }
      for (pi = pti[0]; pi <= pti[4]; pi++) {
        line[pi] = '-';
      }
      for (pi = pti[1]; pi <= pti[3]; pi++) {
        line[pi] = '=';
      }
      line[pti[2]] = 'm';
      if (pti[2] > 0) {
        line[pti[2] - 1] = '<';
      }
      if (pti[2] < plen - 1) {
        line[pti[2] + 1] = '>';
      }
      if (single) {
        line[airIndexClamp(vrange[0], single[yi], vrange[1], plen)] = 'X';
      }
      printf("%s", line);
      if (medshow) {
        printf(" %g", buff[airIndexClamp(0, 50, 100, sx)]);
      }
      printf("\n");
#if 0
      unsigned int ltt = (unsigned int)(-1);
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
