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

#ifndef UNU_HAS_BEEN_INCLUDED
#define UNU_HAS_BEEN_INCLUDED

#include <air.h>
#include <biff.h>
#include <hest.h>
#include <nrrd.h>

#if defined(WIN32) && !defined(TEEM_BUILD)
#define unu_export __declspec(dllimport)
#else
#define unu_export
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* 
** this violates the convention of what is and isn't shortened
** with long library names, but this is because (currently) the
** library basically exists to serve unu and nothing else
*/
#define UNU "unu"

#define UNU_COLUMNS 78  /* how many characters per line do we allow hest */

/*
******** unuCmd
**
** How we associate the one word for the unu command ("name"),
** the one-line info string ("info"), and the single function we
** which implements the command ("main").
*/
typedef struct {
  const char *name, *info;
  int (*main)(int argc, char **argv, char *me, hestParm *hparm);
} unuCmd;

/*
** UNU_DECLARE, UNU_LIST, UNU_MAP
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
** unuCmd struct is defined above.  It would also be nice to have all
** the command's information be held in one array of unuCmds.
** Unfortunately, declaring this is not possible unless all the
** unuCmds and their fields are IN THIS FILE, because otherwise
** they're not constant expressions, so they can't initialize an
** aggregate data type.  So, we instead make an array of unuCmd
** POINTERS, which can be initialized with the addresses of individual
** unuCmd structs, declared and defined in the global scope. is done
** in flotsam.c.  Each of the source files for the various unu
** commands are responsible for setting the fields (at compile-time)
** of their associated unuCmd.
**
** We use three macros to automate this somewhat:
** UNU_DECLARE: declares unu_xxxCmd as an extern unuCmd (defined in xxx.c),
**              used later in this header file.
** UNU_LIST:    the address of unu_xxxCmd, for listing in the array of
**              unuCmd structs in the (compile-time) definition of 
**              unuCmdList[].  This is used in flotsam.c.
**
** Then, to facilitate running these macros on each of the different
** commands, there is a UNU_MAP macro which is used to essentially map
** the two macros above over the list of unu commands.  Functional
** programming meets the C pre-processor.  Therefore:
***********************************************************
    You add commands to unu by:
    1) adjusting the definition of UNU_MAP()
    2) listing the appropriate object in Makefile
    That's it.
********************************************************** */
#define UNU_DECLARE(C) extern unu_export unuCmd unu_##C##Cmd;
#define UNU_LIST(C) &unu_##C##Cmd,
#define UNU_MAP(F) \
F(make) \
F(convert) \
F(resample) \
F(cmedian) \
F(quantize) \
F(unquantize) \
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
F(dhisto) \
F(jhisto) \
F(histax) \
F(heq) \
F(gamma) \
F(1op) \
F(2op) \
F(3op) \
F(lut) \
F(rmap) \
F(imap) \
F(save)

/*
******** UNU_CMD
**
** This is used at the very end of the various command sources
** ("xxx.c") to simplify defining a unuCmd.  "name" should just be the
** command, UNQUOTED, such as flip or slice.
*/
#define UNU_CMD(name, info) \
unuCmd unu_##name##Cmd = { #name, info, unu_##name##Main };

/* xxx.c */
/* Declare the extern unuCmds unu_xxxCmd, for all xxx.  These are
   defined in as many different source files as there are commands. */
UNU_MAP(UNU_DECLARE)

/* flotsam.c */
extern unu_export int unuDefNumColumns;
extern unu_export unuCmd *unuCmdList[];    /* addresses of all unu_xxxCmd */
extern void unuUsage(char *me, hestParm *hparm);
extern unu_export hestCB unuPosHestCB;
extern unu_export hestCB unuMaybeTypeHestCB;
extern unu_export hestCB unuScaleHestCB;
extern unu_export hestCB unuBitsHestCB;
extern unu_export hestCB unuFileHestCB;


#ifdef __cplusplus
}
#endif

#endif /* UNU_HAS_BEEN_INCLUDED */
