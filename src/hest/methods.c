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

#include "hest.h"
#include "private.h"
#include <limits.h>

hestParm *
hestParmNew() {
  hestParm *parm;
  
  parm = calloc(1, sizeof(hestParm));
  if (parm) {
    parm->verbosity = hestVerbosity;
    parm->respFileEnable = hestRespFileEnable;
    parm->columns = hestColumns;
    parm->respFileFlag = hestRespFileFlag;
    parm->respFileComment = hestRespFileComment;
  }
  return parm;
}

hestParm *
hestParmNix(hestParm *parm) {

  return airFree(parm);
}

void
_hestOptInit(hestOpt *opt) {

  opt->flag = opt->name = NULL;
  opt->type = opt->min = opt->max = 0;
  opt->valueP = NULL;
  opt->dflt = opt->info = NULL;
  opt->sawP = NULL;
  opt->kind = opt->alloc = 0;
}

hestOpt *
hestOptNew(void) {
  hestOpt *opt;
  
  opt = calloc(1, sizeof(hestOpt));
  if (opt) {
    _hestOptInit(opt);
    opt->min = 1;
  }
  return opt;
}

hestOpt *
hestOptAdd(hestOpt *opt, 
	   char *flag, char *name,
	   int type, int min, int max,
	   void *valueP, char *dflt, char *info, ...) {
  hestOpt *ret = NULL;
  int num;
  va_list ap;

  if (!opt)
    return NULL;

  num = _hestNumOpts(opt);
  if (!(ret = calloc(num+2, sizeof(hestOpt))))
    return NULL;

  if (num)
    memcpy(ret, opt, num*sizeof(hestOpt));
  ret[num].flag = airStrdup(flag);
  ret[num].name = airStrdup(name);
  ret[num].type = type;
  ret[num].min = min;
  ret[num].max = max;
  ret[num].valueP = valueP;
  ret[num].dflt = airStrdup(dflt);
  ret[num].info = airStrdup(info);
  if (5 == _hestKind(&(ret[num]))) {
    va_start(ap, info);
    ret[num].sawP = va_arg(ap, int*);
    va_end(ap);
  }
  _hestOptInit(&(ret[num+1]));
  ret[num+1].min = 1;
  free(opt);

  return ret;
}

void
_hestOptFree(hestOpt *opt) {
  
  airFree(opt->flag);
  airFree(opt->name);
  airFree(opt->dflt);
  airFree(opt->info);
}

hestOpt *
hestOptNix(hestOpt *opt) {
  int op, num;

  if (!opt)
    return NULL;

  num = _hestNumOpts(opt);
  if (opt[num+1].min) {
    /* we only try to free this if it looks like something we allocated */
    for (op=0; op<=num-1; op++) {
      _hestOptFree(opt+op);
    }
    free(opt);
  }
  return NULL;
}


/*
** _hestIdent()
**
** how to identify an option in error and usage messages
*/
char *
_hestIdent(char *ident, hestOpt *opt) {
  sprintf(ident, "%s%s%s%s", 
	  opt->flag ? "\"-"      : "<",
	  opt->flag ? opt->flag : opt->name,
	  opt->flag ? "\""       : ">",
	  " option");
  return ident;
}

int
_hestMax(int max) {
  
  if (-1 == max)
    max = INT_MAX;
  return max;
}

int
_hestKind(hestOpt *opt) {
  int max;
  
  max = _hestMax(opt->max);
  if (!( opt->min <= max ))
    /* invalid */
    return -1;

  if (0 == opt->min && 0 == max)
    /* flag */
    return 1;

  if (1 == opt->min && 1 == max)
    /* single fixed parameter */
    return 2;

  if (2 <= opt->min && 2 <= max && opt->min == max)
    /* multiple fixed parameters */
    return 3;
  
  if (0 == opt->min && 1 == max)
    /* single optional parameter */
    return 4;

  /* else multiple variable parameters */
  return 5;
}

void
_hestPrintArgv(int argc, char **argv) {
  int a;

  printf("argc=%d : ", argc);
  for (a=0; a<=argc-1; a++) {
    printf("%s ", argv[a]);
  }
  printf("\n");
}

/*
** _hestWhichFlag()
**
** given a string in "flag" (with the hypen prefix) finds which of
** the flags in the given array of options matches that.  Returns
** the index of the matching option, or -1 if there is no match.
*/
int
_hestWhichFlag(hestOpt *opt, char *flag) {
  char buff[AIR_STRLEN_HUGE];
  int op, numOpts;
  
  numOpts = _hestNumOpts(opt);
  for (op=0; op<=numOpts-1; op++) {
    if (!opt[op].flag)
      continue;
    sprintf(buff, "-%s", opt[op].flag);
    if (!strcmp(flag, buff))
      return op;
  }
  return -1;
}


/*
** _hestCase()
**
** helps figure out logic of interpreting parameters and defaults
** for kind 4 and kind 5 options.
*/
int
_hestCase(hestOpt *opt, int *udflt, int *nprm, int *appr, int op) {
  
  if (opt[op].flag && !appr[op]) {
    return 0;
  }
  else if ( (4 == opt[op].kind && udflt[op]) ||
	    (5 == opt[op].kind && !nprm[op]) ) {
    return 1;
  }
  else {
    return 2;
  }
}

/*
** _hestExtract()
**
** takes "np" parameters, starting at "a", out of the given argv, and puts
** them into a string WHICH THIS FUNCTION ALLOCATES, and also adjusts
** the argc value given as "*argcP".
*/
char *
_hestExtract(int *argcP, char **argv, int a, int np) {
  int len, n;
  char *ret;

  if (!np)
    return NULL;

  len = 0;
  for (n=0; n<=np-1; n++) {
    if (a+n==*argcP) {
      return NULL;
    }
    len += strlen(argv[a+n]);
  }
  len += np;
  ret = calloc(len, sizeof(char));
  strcpy(ret, "");
  for (n=0; n<=np-1; n++) {
    strcat(ret, argv[a+n]);
    if (n < np-1)
      strcat(ret, " ");
  }
  for (n=a+np; n<=*argcP; n++) {
    argv[n-np] = argv[n];
  }
  *argcP -= np;
  return ret;
}

int
_hestNumOpts(hestOpt *opt) {
  int num = 0;

  while (opt[num].flag || opt[num].name || opt[num].type) {
    num++;
  }
  printf("_hestNumOpts: %d\n", num);
  return num;
}

int
_hestArgc(char **argv) {
  int num = 0;

  while (argv && argv[num]) {
    num++;
  }
  return num;
}


