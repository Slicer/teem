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

#ifndef HEST_PRIVATE_HAS_BEEN_INCLUDED
#define HEST_PRIVATE_HAS_BEEN_INCLUDED
#ifdef __cplusplus
extern "C" {
#endif

/* methods.c */
extern char *_hestIdent(char *ident, hestOpt *opt, hestParm *parm, int brief);
extern int _hestKind(hestOpt *opt);
extern void _hestPrintArgv(int argc, char **argv);
extern int _hestWhichFlag(hestOpt *opt, char *flag, hestParm *parm);
extern int _hestCase(hestOpt *opt, int *udflt, int *nprm, int *appr, int op);
extern char *_hestExtract(int *argcP, char **argv, int a, int np);
extern int _hestNumOpts(hestOpt *opt);
extern int _hestArgc(char **argv);
extern int _hestMax(int max);

/* parse.c */
extern int _hestPanic(hestOpt *opt, char *err, hestParm *parm);
extern int _hestErrStrlen(hestOpt *opt, int argc, char **argv);


#ifdef __cplusplus
}
#endif
#endif /* HEST_HAS_BEEN_INCLUDED */
