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

#ifdef __cplusplus
extern "C" {
#endif

#include <air.h>
#include <biff.h>
#include <hest.h>
#include <nrrd.h>

#define UNRRDU_COLUMNS 78

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
             NULL, NULL, nrrdHestNrrd)

/* char *var */
#define OPT_ADD_NOUT(var, desc) \
  hestOptAdd(&opt, "o", "nout", airTypeString, 1, 1, &(var), "-", desc)

/* int var */
#define OPT_ADD_AXIS(var, desc) \
  hestOptAdd(&opt, "a", "axis", airTypeInt, 1, 1, &(var), NULL, desc)

/* int *var; int saw */
#define OPT_ADD_BOUND(name, var, desc, saw) \
  hestOptAdd(&opt, name, "pos0", airTypeOther, 1, -1, &(var), NULL, desc, \
	     &(saw), NULL, &unuPosHestCB)

/* int var */
#define OPT_ADD_TYPE(var, desc, dflt) \
  hestOptAdd(&opt, "t", "type", airTypeEnum, 1, 1, &(var), dflt, desc, \
             NULL, nrrdType)

extern hestParm *hparm;
extern hestCB unuPosHestCB;
extern hestCB unuMaybeTypeHestCB;

typedef struct {
  char *name, *info;
  int (*main)(int, char **, char*);
} unuCmd;

/*
** USAGE, PARSE, SAVE
**
** These are macros at their very worst.  Shudder.  This code is
** basically the same, verbatim, across all the different unrrdu
** functions, and having them as macros just shortens (without
** necessarily clarifying) their code.
**
** They all assume many many variables.
**
** NB: below is an unidiomatic use of hestMinNumArgs(), because of
** how unu's main invokes the "main" function of the different
** commands.  Normally the comparison is with argc-1, or argc-2
** the case of cvs-like commands.
*/
#define USAGE(info) \
  if ( (hparm->respFileEnable && !argc) || \
       (!hparm->respFileEnable && argc < hestMinNumArgs(opt)) ) { \
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

#define SAVE(outS, nout, io) \
  if (nrrdSave((outS), (nout), (io))) { \
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways); \
    fprintf(stderr, "%s: error saving nrrd to \"%s\":\n%s\n", me, out, err); \
    airMopError(mop); \
    return 1; \
  }

#ifdef __cplusplus
}
#endif
