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

#include <air.h>
#include <biff.h>
#include <hest.h>
#include <nrrd.h>

/*
** OPT_ADD_...
**
** These macros are used for setting up command-line options for the various
** unu commands.  They define options which are common across many different
** commands, so that the unu interface is as consistent as possible.  They
** all assume a hestOpt *opt variable, but they take the option variable
** and option description as arguments.  The expected type of the variable
** is given before each macro.
*/
/* Nrrd *var */
#define OPT_ADD_NIN(var, desc) \
  hestOptAdd(&opt, "i", "nin", airTypeOther, 1, 1, &(var), "-", desc, \
	     NULL, &unuNrrdHestCB)

/* char *var */
#define OPT_ADD_NOUT(var, desc) \
  hestOptAdd(&opt, "o", "nout", airTypeString, 1, 1, &(var), "-", desc)

/* int var */
#define OPT_ADD_AXIS(var, desc) \
  hestOptAdd(&opt, "a", "axis", airTypeInt, 1, 1, &(var), NULL, desc)

/* int *var; int saw */
#define OPT_ADD_BOUND(name, var, desc, saw) \
  hestOptAdd(&opt, name, "pos0", airTypeOther, 1, -1, &(var), NULL, desc, \
	     &(saw), &unuPosHestCB)

/* int var */
#define OPT_ADD_TYPE(var, desc) \
  hestOptAdd(&opt, "t", "type", airTypeOther, 1, 1, &(var), NULL, desc, \
             NULL, &unuTypeHestCB)

extern hestParm *hparm;
extern hestCB unuNrrdHestCB;
extern hestCB unuTypeHestCB;
extern hestCB unuPosHestCB;
extern hestCB unuBoundaryHestCB;
extern hestCB unuEncodingHestCB;

typedef struct {
  char *name, *info;
  int (*main)(int, char **, char*);
} unuCmd;

/*
** USAGE, PARSE, SAVE
**
** These are macros at their worst.  This code is basically the same,
** verbatim, across all the different unrrdu functions, and having them
** as macros just shortens (without necessarily clarifying) their code.
**
** They all assume many many variables.
*/
#define USAGE(info) \
  if (!argc) { \
    hestInfo(stderr, me, (info), hparm); \
    hestUsage(stderr, opt, me, hparm); \
    hestGlossary(stderr, opt, hparm); \
    airMopError(mop); \
    return 1; \
  }

#define PARSE() \
  if (hestParse(opt, argc, argv, &err, hparm)) { \
    fprintf(stderr, "%s: %s\n", me, err); free(err); \
    hestUsage(stderr, opt, me, hparm); \
    hestGlossary(stderr, opt, hparm); \
    airMopError(mop); \
    return 1; \
  }

#define SAVE(nout, io) \
  if (nrrdSave(out, (nout), (io))) { \
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways); \
    fprintf(stderr, "%s: error saving nrrd to \"%s\":\n%s\n", me, out, err); \
    airMopError(mop); \
    return 1; \
  }
