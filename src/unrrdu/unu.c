/*
  teem: Gordon Kindlmann's research software
  Copyright (C) 2002, 2001, 2000, 1999, 1998 University of Utah

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

#include <air.h>
#include <biff.h>
#include <hest.h>
#include <nrrd.h>

#include "private.h"

/* how we expect this program to identify itself */
#define UNU "unu"

/* used to allow over-riding of defaults in hestParse() */
hestParm *hparm;

/*
** DECLARE, LIST, ADD, MAP
** 
** Stupid C-preprocessor tricks.  The idea is to make it as simple
** as possible to add new commands to unu, so that the new commands
** have to be added to only one thing in this source file, and
** the Makefile.
** 
** Associated with each unu command are some pieces of information:
** the single word command (e.g. "slice") that is used by invoke it,
** the short (approx. one-line) description of its function, and
** the "main" function to call with the appropriate argc, argv. 
** It would be nice to use a struct to hold this information: the
** unuCmd struct is defined in private.h.  It would also be nice 
** to have all the command's information be held in one array 
** of unuCmds.  Unfortunately, declaring this is not possible unless
** all the unuCmds and their fields are IN THIS SOURCE FILE, because
** otherwise they're no constant expressions, so they can't initialize
** an aggregate data type.  So, we instead make an array of unuCmd 
** POINTERS, which can be initialized with the addresses of individual
** unuCmd structs, declared and defined (but not initialized) in the
** global scope.  Then, in main(), we set the fields of the structs
** to point to right things (the name, info, and main as defined by
** each source file).
**
** We use three macros for each of these: 
** DECLARE: declares xxxName, xsxInfo, xxsMain as externs, and defines
**          xxxCmd as a unuCmd struct, where xxx is the command name.
** LIST:    the address of xxCmd, for listing in the array of unuCmd structs
** ADD:     sets the field of xxxCmd to xxxName, xsxInfo, xxsMain
**
** Then, to facilitate running these macros on each of the different
** commands, there is a MAP macro which essentially maps the macros
** above over the list of commands.  Therefore ...
*/

/*********************************************************
    You add commands to unu by:
    1) adjusting the definition of MAP()
    2) listing the appropriate object in Makefile
    That's it.
**********************************************************/
#define MAP(F) \
F(make) \
F(convert) \
F(resample) \
F(quantize) \
F(project) \
F(slice) \
F(dice) \
F(join) \
F(crop) \
F(pad) \
F(reshape) \
F(permute) \
F(swap) \
F(shuffle) \
F(flip) \
F(block) \
F(unblock) \
F(histo) \
F(dhist) \
F(jhist) \
F(histax) \
F(heq) \
F(gamma) \
F(save)
/*********************************************************/

#define DECLARE(C) \
  extern char *C##Name; \
  extern char *C##Info; \
  extern int (C##Main)(int, char **, char*); \
  unuCmd C##Cmd;

#define LIST(C) &C##Cmd,

#define ADD(C) \
  C##Cmd.name = C##Name; \
  C##Cmd.info = C##Info; \
  C##Cmd.main = C##Main;

/* declare externs, define unuCmd structs */  
MAP(DECLARE)

/* create the array of unuCmd struct pointers */
unuCmd *cmdList[] = {
  MAP(LIST)
  NULL
};

/* set up hestCB structs for common non-trivial things */

int
unuParseNrrd(void *ptr, char *str, char err[AIR_STRLEN_HUGE]) {
  char me[] = "unuParseNrrd", *nerr;
  Nrrd **nrrdP;
  airArray *mop;
  
  if (!(ptr && str)) {
    sprintf(err, "%s: got NULL pointer", me);
    return 1;
  }
  nrrdP = ptr;
  mop = airMopInit();
  *nrrdP = nrrdNew();
  airMopAdd(mop, *nrrdP, (airMopper)nrrdNuke, airMopOnError);
  if (nrrdLoad(*nrrdP, str)) {
    airMopAdd(mop, nerr = biffGetDone(NRRD), airFree, airMopOnError);
    if (strlen(nerr) > AIR_STRLEN_HUGE - 1)
      nerr[AIR_STRLEN_HUGE - 1] = '\0';
    strcpy(err, nerr);
    airMopError(mop);
    return 1;
  }
  airMopOkay(mop);
  return 0;
}

hestCB unuNrrdHestCB = {
  sizeof(Nrrd *),
  "nrrd",
  unuParseNrrd,
  (airMopper)nrrdNuke
}; 

int
unuParsePos(void *ptr, char *str, char err[AIR_STRLEN_HUGE]) {
  char me[]="unuParsePos";
  int *pos;

  if (!(ptr && str)) {
    sprintf(err, "%s: got NULL pointer", me);
    return 1;
  }
  pos = ptr;
  if (!strcmp("M", str)) {
    pos[0] = 1;
    pos[1] = 0;
    return 0;
  }
  if ('M' == str[0]) {
    if (!('-' == str[1] || '+' == str[1])) {
      sprintf(err, "%s: M can be followed only by + or -", me);
      return 1;
    }
    pos[0] = 1;
    if (1 != sscanf(str+1, "%d", pos + 1)) {
      sprintf(err, "%s: can't parse \"%s\" as M+<int> or M-<int>", me, str);
      return 1;
    }
  }
  else {
    pos[0] = 0;
    if (1 != sscanf(str, "%d", pos + 1)) {
      sprintf(err, "%s: can't parse \"%s\" as int", me, str);
      return 1;
    }
  }
  return 0;
}

hestCB unuPosHestCB = {
  2*sizeof(int),
  "position",
  unuParsePos,
  NULL
};

void
usage(char *me) {
  int i, maxlen, len, c;
  char buff[AIR_STRLEN_LARGE], fmt[AIR_STRLEN_LARGE];

  maxlen = 0;
  for (i=0; cmdList[i]; i++) {
    maxlen = AIR_MAX(maxlen, strlen(cmdList[i]->name));
  }

  sprintf(buff, "--- %s: Utah nrrd utilities (unrrdu) command-line interface ---", me);
  sprintf(fmt, "%%%ds\n", (int)((80-strlen(buff))/2 + strlen(buff) - 1));
  fprintf(stderr, fmt, buff);
  
  for (i=0; cmdList[i]; i++) {
    len = strlen(cmdList[i]->name);
    strcpy(buff, "");
    for (c=len; c<maxlen; c++)
      strcat(buff, " ");
    strcat(buff, me);
    strcat(buff, " ");
    strcat(buff, cmdList[i]->name);
    strcat(buff, " ... ");
    len = strlen(buff);
    fprintf(stderr, "%s", buff);
    _hestPrintStr(stderr, len, len, 80, cmdList[i]->info, AIR_FALSE);
  }
}

int
main(int argc, char **argv) {
  int i, ret;
  char *argv0 = NULL;

  /* set the fields in all the unuCmd structs */
  MAP(ADD)

  /* if there are no arguments, then we give general usage information */
  if (1 >= argc) {
    usage(UNU);
    return 1;
  }
  /* else, we should see if they're asking for a command we know about */  
  for (i=0; cmdList[i]; i++) {
    if (!strcmp(argv[1], cmdList[i]->name))
      break;
  }
  if (cmdList[i]) {
    /* initialize variables used by the various commands */
    hparm = hestParmNew();
    hparm->elideSingleEnumType = AIR_TRUE;
    hparm->elideSingleOtherType = AIR_TRUE;
    hparm->elideSingleNonExistFloatDefault = AIR_TRUE;
    argv0 = malloc(strlen(UNU) + strlen(argv[1]) + 2);
    sprintf(argv0, "%s %s", UNU, argv[1]);
    ret = cmdList[i]->main(argc-2, argv+2, argv0);
    free(argv0);
    hestParmFree(hparm);
  }
  else {
    fprintf(stderr, "%s: unrecognized command: \"%s\"\n", argv[0], argv[1]);
    usage(UNU);
  }

  exit(ret);
}
