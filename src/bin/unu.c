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

#include <teem/unrrdu.h>

#define UNU "unu"

int
main(int argc, const char **argv) {
  int i, ret, listAll;
  const char *me;
  char *argv0 = NULL;
  hestParm *hparm;
  airArray *mop;

  me = argv[0];

  /* parse environment variables first, in case they break nrrdDefault*
     or nrrdState* variables in a way that nrrdSanity() should see */
  nrrdDefaultGetenv();
  nrrdStateGetenv();

  /* if user hasn't tried to set nrrdStateKindNoop by an environment
     variable, we set it to false, since its probably what people expect */
  if (!getenv(nrrdEnvVarStateKindNoop)) {
    nrrdStateKindNoop = AIR_FALSE;
  }

  /* if user hasn't tried to set nrrdStateKeyValuePairsPropagate by an envvar,
     we set it to true, since that's probably what unu users expect */
  if (!getenv(nrrdEnvVarStateKeyValuePairsPropagate)) {
    nrrdStateKeyValuePairsPropagate = AIR_TRUE;
  }

  /* no harm done in making sure we're sane */
  nrrdSanityOrDie(me);

  mop = airMopNew();
  hparm = hestParmNew();
  airMopAdd(mop, hparm, hestParmFree_vp, airMopAlways);
  hparm->elideSingleEnumType = AIR_TRUE;
  hparm->elideSingleOtherType = AIR_TRUE;
  /*
   * This prevents clarifying that the default input is "-" i.e. stdin, and it was
   * explicitly turned off in multiple commands (such as resample). In the interests of
   * clarity, no longer want to elide this info.
   * hparm->elideSingleOtherDefault = AIR_TRUE;
   */
  hparm->elideSingleNonExistFloatDefault = AIR_TRUE;
  hparm->elideMultipleNonExistFloatDefault = AIR_TRUE;
  hparm->elideSingleEmptyStringDefault = AIR_TRUE;
  hparm->elideMultipleEmptyStringDefault = AIR_TRUE;
  /* say that we look for, and know how to handle, seeing "--help" */
  hparm->respectDashDashHelp = AIR_TRUE;
  /* set hparm->columns from ioctl if possible, else use unrrduDefNumColumns */
  hestParmColumnsIoctl(hparm, unrrduDefNumColumns);
  hparm->greedySingleString = AIR_TRUE;

  /* if there are no arguments, or "unu list" (or "unu all" shhh), then we give general
  usage information */
  listAll = (2 == argc && !strcmp("all", argv[1]));
  if (1 >= argc || listAll || (2 == argc && !strcmp("list", argv[1]))) {
    unrrduUsageUnu("unu", hparm, listAll /* alsoHidden */);
    airMopError(mop);
    exit(1);
  }
  /* else, we see if its unu --version */
  if (!strcmp("--version", argv[1])) {
    char vbuff[AIR_STRLEN_LARGE];
    airTeemVersionSprint(vbuff);
    printf("%s\n", vbuff);
    exit(0);
  }
  /* else, we should see if they're asking for a command we know about */
  for (i = 0; unrrduCmdList[i]; i++) {
    if (!strcmp(argv[1], unrrduCmdList[i]->name)) {
      break;
    }
    if (!strcmp("about", unrrduCmdList[i]->name)) {
      /* we interpret "unu help" and "unu --help" as asking for "unu about" */
      if (!strcmp("--help", argv[1]) || !strcmp("help", argv[1])) {
        break;
      }
    }
  }
  /* unrrduCmdList[] is NULL-terminated */
  if (unrrduCmdList[i]) {
    /* yes, we have that command */
    /* initialize variables used by the various commands */
    argv0 = AIR_CALLOC(strlen(UNU) + strlen(argv[1]) + 2, char);
    airMopMem(mop, &argv0, airMopAlways);
    sprintf(argv0, "%s %s", UNU, argv[1]);

    /* run the individual unu program, saving its exit status */
    ret = unrrduCmdList[i]->main(argc - 2, argv + 2, argv0, hparm);
  } else {
    fprintf(stderr,
            "%s: unrecognized command \"%s\"; type \"%s\" for "
            "complete list\n",
            me, argv[1], me);
    ret = 1;
  }

  airMopDone(mop, ret);
  return ret;
}
