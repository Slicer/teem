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

#ifdef _WIN32
#  include <io.h>
#  include <fcntl.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*
**
** Twisted C-preprocessor tricks.  The idea is to make it as simple
** as possible to add new commands to unu, so that the new commands
** have to be added to only one thing in this source file, and
** the Makefile.
**
** Associated with each unu command are some pieces of information:
** the single word command (e.g. "slice") that is used by invoke it,
** the short (approx. one-line) description of its function, and the
** "main" function to call with the appropriate argc, argv.  It would
** be nice to use a struct to hold this information, and we can: the
** unrrduCmd struct is defined above.  It would also be nice to have
** all the command's information be held in one array of unrrduCmds.
** Unfortunately, declaring this is not possible unless all the
** unrrduCmds and their fields are IN THIS FILE, because otherwise
** they're not constant expressions, so they can't initialize an
** aggregate data type.  So, we instead make an array of unrrduCmd
** POINTERS, which can be initialized with the addresses of individual
** unrrduCmd structs, declared and defined in the global scope. is
** done in flotsam.c.  Each of the source files for the various unu
** commands are responsible for setting the fields (at compile-time)
** of their associated unrrduCmd.
**
** We use three macros to automate this somewhat:
** UNRRDU_DECLARE: declares unrrdu_xxxCmd as an extern unrrduCmd
**                 (defined in xxx.c), used later in this header file.
** UNRRDU_LIST:    the address of unrrdu_xxxCmd, for listing in the array of
**                 unrrduCmd structs in the (compile-time) definition of
**                 unrrduCmdList[].  This is used in flotsam.c.
****
** Then, to facilitate running these macros on each of the different
** commands, there is a UNRRDU_MAP macro which is used to essentially map
** the two macros above over the list of unu commands.  Functional
** programming meets the C pre-processor.  Therefore:
***********************************************************
    You add command foo to unu by:
    1) adding F(foo) to definition of UNRRDU_MAP()
    2) implement foo.c, and list foo.o in GNUmakefile and CmakeLists.txt
    That's it.
********************************************************** */
#define UNRRDU_DECLARE(C) UNRRDU_EXPORT const unrrduCmd unrrdu_##C##Cmd;
#define UNRRDU_LIST(C)    &unrrdu_##C##Cmd,
#define UNRRDU_MAP(F)                                                                   \
  F(about)                                                                              \
  F(env)                                                                                \
  F(i2w)                                                                                \
  F(w2i)                                                                                \
  F(make)                                                                               \
  F(head)                                                                               \
  F(data)                                                                               \
  F(convert)                                                                            \
  F(resample)                                                                           \
  F(fft)                                                                                \
  F(cmedian)                                                                            \
  F(dering)                                                                             \
  F(dist)                                                                               \
  F(minmax)                                                                             \
  F(cksum)                                                                              \
  F(diff)                                                                               \
  F(quantize)                                                                           \
  F(unquantize)                                                                         \
  F(project)                                                                            \
  F(slice)                                                                              \
  F(sselect)                                                                            \
  F(dice)                                                                               \
  F(splice)                                                                             \
  F(join)                                                                               \
  F(crop)                                                                               \
  F(acrop)                                                                              \
  F(inset)                                                                              \
  F(pad)                                                                                \
  F(reshape)                                                                            \
  F(permute)                                                                            \
  F(swap)                                                                               \
  F(shuffle)                                                                            \
  F(flip)                                                                               \
  F(unorient)                                                                           \
  F(basinfo)                                                                            \
  F(block)                                                                              \
  F(unblock)                                                                            \
  F(axinfo)                                                                             \
  F(axinsert)                                                                           \
  F(axsplit)                                                                            \
  F(axdelete)                                                                           \
  F(axmerge)                                                                            \
  F(tile)                                                                               \
  F(untile)                                                                             \
  F(histo)                                                                              \
  F(dhisto)                                                                             \
  F(jhisto)                                                                             \
  F(histax)                                                                             \
  F(heq)                                                                                \
  F(gamma)                                                                              \
  F(1op)                                                                                \
  F(2op)                                                                                \
  F(3op)                                                                                \
  F(affine)                                                                             \
  F(lut)                                                                                \
  F(mlut)                                                                               \
  F(subst)                                                                              \
  F(rmap)                                                                               \
  F(mrmap)                                                                              \
  F(imap)                                                                               \
  F(lut2)                                                                               \
  F(ccfind)                                                                             \
  F(ccadj)                                                                              \
  F(ccmerge)                                                                            \
  F(ccsettle)                                                                           \
  F(dnorm)                                                                              \
  F(vidicon)                                                                            \
  F(grid)                                                                               \
  F(ninspect)                                                                           \
  F(ilk)                                                                                \
  F(hack)                                                                               \
  F(aabplot)                                                                            \
  F(undos)                                                                              \
  F(uncmt)                                                                              \
  F(save)

/* xxx.c */
/* Declare the extern unrrduCmds unrrdu_xxxCmd, for all xxx.  These are
   defined in as many different source files as there are commands. */
UNRRDU_MAP(UNRRDU_DECLARE)

/*
******** UNRRDU_CMD
**
** This is used at the very end of the various command sources
** ("xxx.c") to simplify defining a unrrduCmd.  "name" should just be
** the command, UNQUOTED, such as flip or slice.
*/
#define UNRRDU_CMD(name, info)                                                          \
  const unrrduCmd unrrdu_##name##Cmd = {#name, info, unrrdu_##name##Main, AIR_FALSE}
#define UNRRDU_CMD_HIDE(name, info)                                                     \
  const unrrduCmd unrrdu_##name##Cmd = {#name, info, unrrdu_##name##Main, AIR_TRUE}

/* handling of "quiet quit", to avoid having a string of piped unu
   commands generate multiple pages of unwelcome usage info */
/* the environment variable to look for */
#define UNRRDU_QUIET_QUIT_ENV "UNRRDU_QUIET_QUIT"
/* the string to search for in the error message that signifies that
   we should die quietly; this is clearly a hack because it depends on
   the text of error messages set by a different library.  With more
   work it could be made into less of a hack.  At worst the hack's
   breakage will lead again to the many error messages that inspired
   the hack in the first place , and will inspire fixing it again */
#define UNRRDU_QUIET_QUIT_STR "[nrrd] _nrrdRead: immediately hit EOF"

/*
** OPT_ADD_XXX
**
** These macros are used for setting up command-line options for the various
** unu commands.  They define options which are common across many different
** commands, so that the unu interface is as consistent as possible.  They
** all assume a hestOpt *opt variable, but they take the option variable
** and option description as arguments.  The expected type of the variable
** is given before each macro.
*/
/* Nrrd *var */
#define OPT_ADD_NIN(var, desc)                                                          \
  hestOptAdd_1_Other(&opt, "i,input", "nin", &(var), "-", desc, nrrdHestNrrd)

/* char *var */
#define OPT_ADD_NOUT(var, desc)                                                         \
  hestOptAdd_1_String(&opt, "o,output", "nout", &(var), "-", desc)

/* unsigned int var */
#define OPT_ADD_AXIS(var, desc)                                                         \
  hestOptAdd_1_UInt(&opt, "a,axis", "axis", &(var), NULL, desc)

/* int *var; int saw */
#define OPT_ADD_BOUND(name, needmin, var, deflt, desc, saw)                             \
  hestOptAdd_Nv_Other(&opt, name, "pos0", needmin, -1, &(var), deflt, desc, &(saw),     \
                      &unrrduHestPosCB)

/* int var */
#define OPT_ADD_TYPE(var, desc, dflt)                                                   \
  hestOptAdd_1_Enum(&opt, "t,type", "type", &(var), dflt, desc, nrrdType)

/*
** USAGE, PARSE, SAVE
**
** These are macros at their worst and most fragile, because of how
** many local variables are assumed.  This code is basically the same,
** verbatim, across all the different unrrdu functions, and having
** them as macros just shortens (without necessarily clarifying) their
** code.
**
** The return value for generating usage information was changed from
** 1 to 0 with the thought that merely asking for usage info shouldn't
** be treated as an erroneous invocation; unu about and unu env (which
** don't use this macro) had already been this way.
*/
#define USAGE(INFO)                                                                     \
  if (!argc && !hparm->noArgsIsNoProblem) {                                             \
    hestInfo(stdout, me, (INFO), hparm);                                                \
    hestUsage(stdout, opt, me, hparm);                                                  \
    hestGlossary(stdout, opt, hparm);                                                   \
    airMopError(mop);                                                                   \
    return 0;                                                                           \
  }

/*

I nixed this because it meant unu invocations with only a
few args (less than hestMinNumArgs()), which were botched
because they were missing options, were not being described
in the error messages.

**
** NB: below is an unidiomatic use of hestMinNumArgs(), because of
** how unu's main invokes the "main" function of the different
** commands.  Normally the comparison is with argc-1, or argc-2
** the case of cvs/svn-like commands.


if ( (hparm->respFileEnable && !argc) || \
     (!hparm->respFileEnable && argc < hestMinNumArgs(opt)) ) { \
*/

/*
** NOTE: of all places it is inside the PARSE() macro that the
** "quiet-quit" functionality is implemented; this is defensible
** because all unu commands use PARSE
*/
#define PARSE(INFO)                                                                     \
  if ((pret = hestParse(opt, argc, argv, &err, hparm))) {                               \
    if (1 == pret || 2 == pret) {                                                       \
      if (!(getenv(UNRRDU_QUIET_QUIT_ENV)                                               \
            && airEndsWith(err, UNRRDU_QUIET_QUIT_STR "\n"))) {                         \
        fprintf(stderr, "%s: %s\n", me, err);                                           \
        free(err);                                                                      \
        hestUsage(stderr, opt, me, hparm);                                              \
        /* Its gotten too annoying to always get this glossary; */                      \
        /* unu <cmd> --help will print it. */                                           \
        /* hestGlossary(stderr, opt, hparm); */                                         \
        if (hparm && hparm->noArgsIsNoProblem) {                                        \
          fprintf(stderr, "\nFor more info: \"%s --help\"\n", me);                      \
        } else {                                                                        \
          fprintf(stderr, "\nFor more info: \"%s\" or \"%s --help\"\n", me, me);        \
        }                                                                               \
      }                                                                                 \
      airMopError(mop);                                                                 \
      return 1;                                                                         \
    } else {                                                                            \
      /* . . . like tears . . . in rain. Time . . . to die. */                          \
      exit(1);                                                                          \
    }                                                                                   \
  } else if (opt->helpWanted) {                                                         \
    hestInfo(stdout, me, (INFO), hparm);                                                \
    hestUsage(stdout, opt, me, hparm);                                                  \
    hestGlossary(stdout, opt, hparm);                                                   \
    return 0;                                                                           \
  }

/* For the (many) times that USAGE and PARSE are used together, useful
   now that using INFO is part of how PARSE responds to --help */
#define USAGE_OR_PARSE(INFO)                                                            \
  USAGE(INFO)                                                                           \
  PARSE(INFO)

#define SAVE(outS, nout, io)                                                            \
  if (nrrdSave((outS), (nout), (io))) {                                                 \
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);                     \
    fprintf(stderr, "%s: error saving nrrd to \"%s\":\n%s\n", me, (outS), err);         \
    airMopError(mop);                                                                   \
    return 1;                                                                           \
  }

#ifdef __cplusplus
}
#endif
