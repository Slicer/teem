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

#ifdef __cplusplus
extern "C" {
#endif

/* hvol.c */
extern int _baneAxisCheck(baneAxis *axis);

#define BANE_GKMS_CMD(name, info)                                                       \
  const unrrduCmd baneGkms_##name##Cmd = {#name, info, baneGkms_##name##Main, AIR_FALSE}

/* USAGE, PARSE
   all copied from unrrdu/privateUnrrdu.h */
#define USAGE(INFO)                                                                     \
  if (!argc && !hparm->noArgsIsNoProblem) {                                             \
    hestInfo(stderr, me, (INFO), hparm);                                                \
    hestUsage(stderr, opt, me, hparm);                                                  \
    hestGlossary(stderr, opt, hparm);                                                   \
    airMopError(mop);                                                                   \
    return 2;                                                                           \
  }

#define PARSE(INFO)                                                                     \
  if ((pret = hestParse(opt, argc, argv, &perr, hparm))) {                              \
    if (1 == pret) {                                                                    \
      fprintf(stderr, "%s: %s\n", me, perr);                                            \
      free(perr);                                                                       \
      hestUsage(stderr, opt, me, hparm);                                                \
      if (hparm && hparm->noArgsIsNoProblem) {                                          \
        fprintf(stderr, "\nFor more info: \"%s --help\"\n", me);                        \
      } else {                                                                          \
        fprintf(stderr, "\nFor more info: \"%s\" or \"%s --help\"\n", me, me);          \
      }                                                                                 \
      airMopError(mop);                                                                 \
      return 2;                                                                         \
    } else {                                                                            \
      exit(1);                                                                          \
    }                                                                                   \
  } else if (opt->helpWanted) {                                                         \
    hestInfo(stdout, me, (INFO), hparm);                                                \
    hestUsage(stdout, opt, me, hparm);                                                  \
    hestGlossary(stdout, opt, hparm);                                                   \
    return 0;                                                                           \
  }

#define USAGE_PARSE(INFO) USAGE(INFO) PARSE(INFO)

#ifdef __cplusplus
}
#endif
