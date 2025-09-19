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
  You should have received a copy of the GNU Lesser General Public License
  along with this library; if not, see <https://www.gnu.org/licenses/>.
*/

#ifdef __cplusplus
extern "C" {
#endif

/* Having hest depend on biff is new for TeemV2: with the new and more modular
re-write of hestParse, not having a useful way to accumulate error messages across a
deeper call stack was getting just too annoying.
It is still the case, however, the hest users do not need to call into biff */
#include <teem/biff.h>

typedef unsigned int uint;

// pre-TeemV2, these used to be change-able defaults in defaultsHest.c:
//   char hestDefaultRespFileFlag = '@';
//   char hestDefaultRespFileComment = '#';
//   char hestDefaultVarParamStopFlag = '-';
//   char hestDefaultMultiFlagSep = ',';
// with corresponding fields in the hestParm defined in hest.h
//   char respFileFlag,        /* the character at the beginning of an argument
//                                indicating that this is a response file name */
//     respFileComment,        /* comment character for the response files */
//     varParamStopFlag, /* prefixed by '-' to form the flag (usually "--") that signals
//                          the end of a *flagged* variable parameter option (single or
//                          multiple). This is important to use if there is a flagged
//                          variable parameter option preceeding an unflagged variable
//                          parameter option, because otherwise how will you know where
//                          the first stops and the second begins */
//     multiFlagSep;     /* character in flag which signifies that there is a long and
//                          short version, and which separates the two.  Or, can be set
//                          to '\0' to disable this behavior entirely. */
// However, there is more confusion than utility created by allowing these
// change. The actual value in giving these things names was in code legibility by
// removing magic constants, so that's the role of these #define's now.
#define RESPONSE_FILE_FLAG    '@'
#define RESPONSE_FILE_COMMENT '#'
#define VAR_PARM_STOP_FLAG    '-'
#define MULTI_FLAG_SEP        ','

#define HEST _hestBiffKey

#define HIST_DEPTH_MAX 10 // max sensibly depth of hestInputStack

/* methodsHest.c */
extern const char *const _hestBiffKey;
extern int _hestKind(const hestOpt *opt);
extern int _hestMax(int max);
extern int _hestOptCheck(const hestOpt *opt, char *err, const hestParm *parm);

/* parseHest.c */
extern uint _hestErrStrlen(const hestOpt *opt, int argc, const char **argv);

#ifdef __cplusplus
}
#endif
