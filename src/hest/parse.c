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

/*
learned: well duh: when you send arguments to printf(), they will
be evaluated before printf() sees them, so you can't use _hestIdent()
twice with differen values
*/

#include "hest.h"
#include "private.h"

#define ME ((parm && parm->verbosity) ? me : "")

/*
** _hestArgsInResponseFiles()
**
** returns the number of args that will be parsed from the response files.
** The role of this function is solely to simplify the task of avoiding
** memory leaks.  By knowing exactly how many args we'll get in the response
** file, then hestParse() can allocate its local argv[] for exactly as
** long as it needs to be, and we can avoid using an airArray.  The drawback
** is that we open and read through the response files twice.  Alas.
*/
int
_hestArgsInResponseFiles(int *argcP, int *nrfP,
			 char **argv, char *err, hestParm *parm) {
  FILE *file;
  char me[]="_hestArgsInResponseFiles: ", line[AIR_STRLEN_HUGE];
  int ai, len;

  *argcP = 0;
  *nrfP = 0;
  if (!parm->respFileEnable) {
    /* don't do response files; we're done */
    return 0;
  }

  ai = 0;
  while (argv[ai]) {
    if (parm->respFileFlag == argv[ai][0]) {
      if (!(file = fopen(argv[ai]+1, "r"))) {
	/* can't open the indicated response file for reading */
	sprintf(err, "%scouldn't open \"%s\" for reading as response file",
		ME, argv[ai]+1);
	*argcP = 0;
	*nrfP = 0;
	return 1;
      }
      len = airOneLine(file, line, AIR_STRLEN_HUGE);
      while (len > 0) {
	if (parm->respFileComment != line[0]) {
	  /* process this only if its not a comment */
	  airOneLinify(line);
	  *argcP += airStrntok(line, " ");
	}
	len = airOneLine(file, line, AIR_STRLEN_HUGE);
      }
      fclose(file);
      (*nrfP)++;
    }
    ai++;
  }
  return 0;
}

/*
** _hestResponseFiles()
**
** 
*/
int
_hestResponseFiles(char **newArgv, char **oldArgv, int nrf,
		   char *err, hestParm *parm, airArray *pmop) {
  char line[AIR_STRLEN_HUGE];
  int len, newArgc, oldArgc, numArgs, ai;
  FILE *file;
  
  if (!parm->respFileEnable) {
    /* don't do response files; we're done */
    return 0;
  }
  newArgc = oldArgc = 0;
  while(oldArgv[oldArgc]) {
    if (parm->respFileFlag != oldArgv[oldArgc][0]) {
      /* nothing to do with a response file, just copy the arg over.
	 We are not allocating new memory in this case. */
      newArgv[newArgc] = oldArgv[oldArgc];
      newArgc += 1;
    }
    else {
      /* It is a response file.  Error checking on being able to open it
	 should have been done by _hestArgsInResponseFiles() */
      file = fopen(oldArgv[oldArgc]+1, "r");
      len = airOneLine(file, line, AIR_STRLEN_HUGE);
      while (len > 0) {
	if (parm->respFileComment != line[0]) {
	  /* process this only if its not a comment */
	  airOneLinify(line);
	  numArgs = airStrntok(line, " ");
	  airParseStrS(newArgv + newArgc, line, " ", numArgs);
	  for (ai=0; ai<=numArgs-1; ai++) {
	    /* This time, we did allocate memory.  We can use airFree and
	       not airFreeP because these will not be reset before mopping */
	    airMopAdd(pmop, newArgv[newArgc+ai], airFree, AIR_FALSE);
	  }
	  newArgc += numArgs;
	}
	len = airOneLine(file, line, AIR_STRLEN_HUGE);
      }
      fclose(file);
    }
    oldArgc++;
  }
  newArgv[newArgc] = NULL;

  return 0;
}

/*
** _hestPanic()
**
** all error checking on the given hest array itself (not the
** command line to be parsed).  Also, sets the "kind" field of
** the opt struct
*/
int
_hestPanic(hestOpt *opt, char *err, hestParm *parm) {
  char me[]="_hestPanic: ";
  int bufflen, numvar, op, numOpts;
  char tbuff[AIR_STRLEN_HUGE];
  float tmpF = 0;

  numOpts = _hestNumOpts(opt);

  bufflen = 0;
  for (op=0; op<=numOpts-1; op++) {
    bufflen = AIR_MAX(bufflen, airStrlen(opt[op].flag));
  }
  
  numvar = 0;
  for (op=0; op<=numOpts-1; op++) {
    opt[op].kind = _hestKind(opt + op);
    if (-1 == opt[op].kind) {
      if (err)
	sprintf(err, "%s!!!!!! opt[%d]'s min (%d) and max (%d) incompatible",
		ME, op, opt[op].min, opt[op].max);
      return 1;
    }
    if (5 == opt[op].kind && !(opt[op].sawP)) {
      if (err)
	sprintf(err, "%s!!!!!! have multiple variable parameters, "
		"but sawP is NULL", ME);
      return 1;
    }
    if (!(AIR_BETWEEN(airTypeUnknown, opt[op].type, airTypeLast))) {
      if (err)
	sprintf(err, "%s!!!!!! opt[%d].type (%d) not in valid range [%d,%d]",
		ME, op, opt[op].type, airTypeUnknown+1, airTypeLast-1);
      return 1;
    }
    if (opt[op].flag) {
      if (!strlen(opt[op].flag)) {
	if (err)
	  sprintf(err, "%s!!!!!! opt[%d].flag is non-NULL but zero length",
		  ME, op);
	return 1;
      }
      sprintf(tbuff, "-%s", opt[op].flag);
      if (1 == sscanf(tbuff, "%f", &tmpF)) {
	if (err)
	  sprintf(err, "%s!!!!!! opt[%d].flag (\"%s\") is numeric, bad news",
		  ME, op, opt[op].flag);
	return 1;
      }
    }
    if (1 == opt[op].kind) {
      if (!opt[op].flag) {
	if (err)
	  sprintf(err, "%s!!!!!! flags must have flags", ME);
	return 1;
      }
    }
    else {
      if (!opt[op].name) {
	if (err)
	  sprintf(err, "%s!!!!!! opt[%d] isn't a flag: must have \"name\"",
		  ME, op);
	return 1;
      }
    }
    numvar += (opt[op].min < _hestMax(opt[op].max) && (NULL == opt[op].flag));
  }
  if (numvar > 1) {
    if (err)
      sprintf(err, "%s!!!!!! can't have %d unflagged min<max opts, only one", 
	      ME, numvar);
    return 1;
  }
  return 0;
}

/*
** _hestExtractFlagged()
**
** extracts the parameters associated with all flagged options from the
** given argc and argv, storing them in prms[], recording the number
** of parameters in nprm[], and whether or not the flagged option appeared
** in appr[].
**
** The "saw" information is not set here, since it is better set
** at value parsing time, which happens after defaults are enstated.
*/
int
_hestExtractFlagged(char **prms, int *nprm, int *appr,
		     int *argcP, char **argv, 
		     hestOpt *opt,
		     char *err, hestParm *parm, airArray *pmop) {
  char me[]="_hestExtractFlagged: ", ident1[AIR_STRLEN_HUGE],
    ident2[AIR_STRLEN_HUGE];
  int a, np, flag, endflag, numOpts, op;

  a = 0;
  while (a<=*argcP-1) {
    flag = _hestWhichFlag(opt, argv[a]);
    /* printf("!%s: A: a = %d -> flag = %d\n", me, a, flag); */
    if (-1 == flag) {
      /* not a flag, move on */
      a++;
      continue;
    }
    /* see if we can associate some parameters with the flag */
    np = 0;
    while (np < _hestMax(opt[flag].max) &&
	   a+np+1 <= *argcP-1 &&
	   -1 == (endflag = _hestWhichFlag(opt, argv[a+np+1]))) {
      np++;
      /* printf("!%s: ... np=%d, endflag = %d\n", me, np, endflag); */
    }
    /* printf("!%s: B: np = %d; endflag = %d\n", me, np, endflag); */
    if (np < opt[flag].min) {
      /* didn't get minimum number of parameters */
      if (!( a+np+1 <= *argcP-1 )) {
	sprintf(err, "%shit end of line before getting %d parameter%s for %s",
		ME, opt[flag].min, opt[flag].min > 1 ? "s" : "",
		_hestIdent(ident1, opt+flag));
      }
      else {
	sprintf(err, "%shit %s before getting %d parameter%s for %s",
		ME, _hestIdent(ident1, opt+endflag),
		opt[flag].min, opt[flag].min > 1 ? "s" : "",
		_hestIdent(ident2, opt+flag));
      }
      return 1;
    }
    nprm[flag] = np;
    /*
    printf("!%s:________ a=%d, *argcP = %d -> flag = %d\n", 
	   me, a, *argcP, flag);
    _hestPrintArgv(*argcP, argv);
    */
    /* lose the flag argument */
    free(_hestExtract(argcP, argv, a, 1));
    /* extract the args after the flag */
    if (appr[flag]) {
      prms[flag] = airFree(prms[flag]);
    }
    prms[flag] = _hestExtract(argcP, argv, a, nprm[flag]);
    if (!appr[flag]) {
      /* add this only once.  We used airFreeP because prms[flag]
	 may be free()d and set (a few times) before mopping */
      airMopAdd(pmop, &(prms[flag]), airFreeP, AIR_FALSE);
    }
    appr[flag] = AIR_TRUE;
    /*
    _hestPrintArgv(*argcP, argv);
    printf("!%s:^^^^^^^^ *argcP = %d\n", me, *argcP);
    printf("!%s: prms[%d] = %s\n", me, flag, prms[flag]);
    */
  }

  /* make sure that flagged options without default were given */
  numOpts = _hestNumOpts(opt);
  for (op=0; op<=numOpts-1; op++) {
    if (opt[op].flag && !opt[op].dflt && !appr[op]) {
      sprintf(err, "%sdidn't get required %s",
	      ME, _hestIdent(ident1, opt+op));
      return 1;
    }
  }

  return 0;
}

int
_hestNextUnflagged(int op, hestOpt *opt, int numOpts) {

  for(; op<=numOpts-1; op++) {
    if (!opt[op].flag)
      break;
  }
  return op;
}

int
_hestExtractUnflagged(char **prms, int *nprm,
		      int *argcP, char **argv, 
		      hestOpt *opt,
		      char *err, hestParm *parm, airArray *pmop) {
  char me[]="_hestExtractUnflagged: ", ident[AIR_STRLEN_HUGE];
  int nvp, np, op, unflag1st, unflagVar, numOpts;

  numOpts = _hestNumOpts(opt);
  unflag1st = _hestNextUnflagged(0, opt, numOpts);
  if (numOpts == unflag1st) {
    /* no unflagged options; we're done */
    return 0;
  }

  for (unflagVar = unflag1st; 
       unflagVar != numOpts; 
       unflagVar = _hestNextUnflagged(unflagVar+1, opt, numOpts)) {
    if (opt[unflagVar].min < _hestMax(opt[unflagVar].max))
      break;
  }
  /* now, if there is a variable parameter unflagged opt, unflagVar is its
     index in opt[], or else unflagVar is numOpts */

  /* grab parameters for all unflagged opts before opt[t] */
  for (op = _hestNextUnflagged(0, opt, numOpts); 
       op < unflagVar; 
       op = _hestNextUnflagged(op+1, opt, numOpts)) {
    /* printf("!%s: op = %d; unflagVar = %d\n", me, op, unflagVar); */
    np = opt[op].min;  /* min == max */
    if (!(np <= *argcP)) {
      sprintf(err, "%sdon't have %d parameter%s %s%s%sfor %s", 
	      ME, np, np > 1 ? "s" : "", 
	      argv[0] ? "starting at \"" : "",
	      argv[0] ? argv[0] : "",
	      argv[0] ? "\" " : "",
	      _hestIdent(ident, opt+op));
      return 1;
    }
    prms[op] = _hestExtract(argcP, argv, 0, np);
    airMopMem(pmop, &(prms[op]), AIR_FALSE);
    nprm[op] = np;
  }
  /*
  _hestPrintArgv(*argcP, argv);
  */
  /* we skip over the variable parameter unflagged option, subtract from *argcP
     the number of parameters in all the opts which follow it, in order to get
     the number of parameters in the sole variable parameter option, 
     store this in nvp */
  nvp = *argcP;
  for (op = _hestNextUnflagged(unflagVar+1, opt, numOpts); 
       op < numOpts; 
       op = _hestNextUnflagged(op+1, opt, numOpts)) {
    nvp -= opt[op].min;  /* min == max */
  }
  if (nvp < 0) {
    op = _hestNextUnflagged(unflagVar+1, opt, numOpts);
    np = opt[op].min;
    sprintf(err, "%sdon't have %d parameter%s for %s", 
	    ME, np, np > 1 ? "s" : "", 
	    _hestIdent(ident, opt+op));
    return 1;
  }
  /* else we had enough args for all the unflagged options following
     the sole variable parameter unflagged option, so snarf them up */
  for (op = _hestNextUnflagged(unflagVar+1, opt, numOpts); 
       op < numOpts; 
       op = _hestNextUnflagged(op+1, opt, numOpts)) {
    np = opt[op].min;
    prms[op] = _hestExtract(argcP, argv, nvp, np);
    airMopMem(pmop, &(prms[op]), AIR_FALSE);
    nprm[op] = np;
  }

  /* now we grab the parameters of the sole variable parameter unflagged opt,
     if it exists (unflagVar < numOpts) */
  if (unflagVar < numOpts) {
    /*
    printf("!%s: unflagVar=%d: min, nvp, max = %d %d %d\n", me, unflagVar,
	   opt[unflagVar].min, nvp, _hestMax(opt[unflagVar].max));
    */
    /* we'll do error checking for unexpected args later */
    nvp = AIR_MIN(nvp, _hestMax(opt[unflagVar].max));
    if (nvp < opt[unflagVar].min) {
      sprintf(err, "%sdidn't get minimum of %d arg%s for %s (got %d)",
	      ME, opt[unflagVar].min, 
	      opt[unflagVar].min > 1 ? "s" : "",
	      _hestIdent(ident, opt+unflagVar), nvp);
      return 1;
    }
    if (nvp) {
      prms[unflagVar] = _hestExtract(argcP, argv, 0, nvp);
      airMopMem(pmop, &(prms[unflagVar]), AIR_FALSE);
      nprm[unflagVar] = nvp;
    }
    else {
      prms[unflagVar] = NULL;
      nprm[unflagVar] = 0;
    }
  }
  return 0;
}

int
_hestDefaults(char **prms, int *udflt, int *nprm, int *appr, 
	      hestOpt *opt,
	      char *err, hestParm *parm, airArray *mop) {
  char *tmpS, me[]="_hestDefaults: ", ident[AIR_STRLEN_HUGE];
  int op, numOpts;

  numOpts = _hestNumOpts(opt);
  for (op=0; op<=numOpts-1; op++) {
    switch(opt[op].kind) {
    case 1:
      /* -------- (no-parameter) boolean flags -------- */
      /* default is always ignored */
      udflt[op] = 0;
      break;
    case 2:
    case 3:
      /* -------- one required parameter -------- */
      /* -------- multiple required parameters -------- */
      /* we'll used defaults if the flag didn't appear */
      udflt[op] = !appr[op];
      break;
    case 4:
      /* -------- optional single variables -------- */
      /* if the flag appeared (if there is a flag), we'll "invert" the
	 default, if the flag didn't appear (or if there isn't a flag),
	 we'll use the default.  In either case, nprm[op] will be zero */
      udflt[op] = (0 == nprm[op]);
      break;
    case 5:
      /* -------- multiple optional parameters -------- */
      /* we'll use the default if the flag didn't appear (if there is a
	 flag) Otherwise, if nprm[op] is zero, then the user is saying,
	 I want zero parameters */
      udflt[op] = opt[op].flag && !appr[op];
      break;
    }
    if (!udflt[op])
      continue;
    prms[op] = airStrdup(opt[op].dflt);
    if (prms[op]) {
      airMopMem(mop, &(prms[op]), AIR_FALSE);
      airOneLinify(prms[op]);
      tmpS = airStrdup(prms[op]);
      nprm[op] = airStrntok(tmpS, " ");
      tmpS = airFree(tmpS);
      /* printf("!%s: nprm[%d] in default = %d\n", me, op, nprm[op]); */
      if (opt[op].min < _hestMax(opt[op].max)) {
	if (!( AIR_INSIDE(opt[op].min, nprm[op], _hestMax(opt[op].max)) )) {
	  sprintf(err, "%s# parameters (in default) for %s is %d, "
		  "but need between %d and %d", 
		  ME, _hestIdent(ident, opt+op), nprm[op],
		  opt[op].min, _hestMax(opt[op].max));
	  return 1;
	}
      }
    }
  }
  return 0;
}

int
_hestSetValues(char **prms, int *udflt, int *nprm, int *appr,
	       hestOpt *opt,
	       char *err, hestParm *parm, airArray *pmop) {
  char ident[AIR_STRLEN_HUGE], me[]="_hestSetValues: ";
  double tmpD;
  int op, type, numOpts;
  void *vP;

  numOpts = _hestNumOpts(opt);
  for (op=0; op<=numOpts-1; op++) {
    _hestIdent(ident, opt+op);
    type = opt[op].type;
    vP = opt[op].valueP;
    switch(opt[op].kind) {
    case 1:
      /* -------- parameter-less boolean flags -------- */
      if (vP)
	*((int*)vP) = appr[op];
      opt[op].alloc = 0;
      break;
    case 2:
      /* -------- one required parameter -------- */
      if (prms[op] && vP) {
	if (1 != airParseStr[type](vP, prms[op], " ", 1)) {
	  sprintf(err, "%scouldn't parse %s\"%s\" as %s for %s", 
		  ME, udflt[op] ? "(default) " : "", prms[op],
		  airTypeStr[type], ident);
	  return 1;
	}
	opt[op].alloc = (opt[op].type == airTypeString ? 1 : 0);
      }
      else {
	opt[op].alloc = 0;
      }
      break;
    case 3:
      /* -------- multiple required parameters -------- */
      if (prms[op] && vP) {
	if (opt[op].min !=   /* min == max */
	    airParseStr[type](vP, prms[op], " ", opt[op].min)) {
	  sprintf(err, "%scouldn't parse %s\"%s\" as %d %s%s for %s",
		  ME, udflt[op] ? "(default) " : "", prms[op],
		  opt[op].min, airTypeStr[type], 
		  opt[op].min > 1 ? "s" : "", ident);
	  return 1;
	}
	opt[op].alloc = (opt[op].type == airTypeString ? 2 : 0);
      }
      else {
	opt[op].alloc = 0;
      }
      break;
    case 4:
      /* -------- optional single variables -------- */
      if (prms[op] && vP) {
	if (1 != airParseStr[type](vP, prms[op], " ", 1)) {
	  sprintf(err, "%scouldn't parse %s\"%s\" as %s for %s",
		  ME, udflt[op] ? "(default) " : "", prms[op],
		  airTypeStr[type], ident);
	  return 1;
	}
	opt[op].alloc = (opt[op].type == airTypeString ? 1 : 0);
	if (1 == _hestCase(opt, udflt, nprm, appr, op)) {
	  /* we just parsed the default, but now we want to "invert" it */
	  if (airTypeString == type) {
	    *((char**)vP) = airFree(*((char**)vP));
	    opt[op].alloc = 0;
	  }
	  else {
	    tmpD = airDLoad(vP, type);
	    airIStore(vP, type, tmpD ? 0 : 1);
	  }
	}
      }
      else {
	opt[op].alloc = 0;
      }
      break;
    case 5:
      /* -------- multiple optional parameters -------- */
      if (prms[op] && vP) {
	if (1 == _hestCase(opt, udflt, nprm, appr, op)) {
	  *((void**)vP) = NULL;
	  opt[op].alloc = 0;
	  *(opt[op].sawP) = 0;
	}
	else {
	  /* printf("!%s: nprm[%d] = %d\n", me, op, nprm[op]); */
	  *((void**)vP) = calloc(nprm[op], airTypeSize[type]);
	  if (nprm[op] != 
	      airParseStr[type](*((void**)vP), prms[op], " ", nprm[op])) {
	    sprintf(err, "%scouldn't parse %s\"%s\" as %d %s%s for %s",
		    ME, udflt[op] ? "(default) " : "", prms[op],
		    nprm[op], airTypeStr[type], 
		    nprm[op] > 1 ? "s" : "", ident);
	    return 1;
	  }
	  *(opt[op].sawP) = nprm[op];
	  opt[op].alloc = (airTypeString == opt[op].type ? 3 : 1);
	}
      }
      else {
	opt[op].alloc = 0;
	*(opt[op].sawP) = 0;
      }
      break;
    }
  }
  return 0;
}

int
hestParse(hestOpt *opt, int _argc, char **_argv,
	  char **_errP, hestParm *_parm) {
  char me[]="hestParse: ";
  char **argv, **prms, *err;
  int a, argc, argr, *nprm, *appr, *udflt, nrf, numOpts, big;
  airArray *mop;
  hestParm *parm;
  
  numOpts = _hestNumOpts(opt);

  /* -------- initialize the mop! */
  mop = airMopInit();

  /* -------- either copy given _parm, or allocate one */
  if (_parm) {
    parm = _parm;
  }
  else {
    parm = hestParmNew();
    airMopAdd(mop, parm, (airMopper)hestParmNix, AIR_FALSE);
  }

  /* -------- allocate the err string.  To determine its size with total
     ridiculous safety we have to find the biggest things which can appear
     in the string. */
  big = 0;
  for (a=0; a<=_argc-1; a++) {
    big = AIR_MAX(big, airStrlen(_argv[a]));
  }
  for (a=0; a<=numOpts-1; a++) {
    big = AIR_MAX(big, airStrlen(opt[a].flag));
    big = AIR_MAX(big, airStrlen(opt[a].name));
  }
  for (a=airTypeUnknown+1; a<=airTypeLast-1; a++) {
    big = AIR_MAX(big, airStrlen(airTypeStr[a]));
  }
  big += 4 * 12;  /* as many as 4 ints per error message */
  big += 512;     /* function name and text of error message */
  if (!(err = calloc(big, sizeof(char)))) {
    fprintf(stderr, "%s PANIC: couldn't allocate error message "
	    "buffer (size %d)\n", me, big);
    exit(1);
  }
  /* the error message buffer _is_ a keeper */
  airMopAdd(mop, err, airFree, AIR_TRUE);
  /* if turns out that there was no error, we'll reset *_errP */
  if (_errP)
    *_errP = err;

  /* -------- check on validity of the hest array */
  if (_hestPanic(opt, err, parm)) {
    airMopDone(mop, AIR_TRUE); return 1;
  }

  /* -------- Create all the local arrays used to save state during
     the processing of all the different options */
  nprm = calloc(numOpts, sizeof(int));   airMopMem(mop, &nprm, AIR_FALSE);
  appr = calloc(numOpts, sizeof(int));   airMopMem(mop, &appr, AIR_FALSE);
  udflt = calloc(numOpts, sizeof(int));  airMopMem(mop, &udflt, AIR_FALSE);
  prms = calloc(numOpts, sizeof(char*)); airMopMem(mop, &prms, AIR_FALSE);
  for (a=0; a<=numOpts-1; a++) {
    prms[a] = NULL;
  }

  /* -------- find out how big the argv array needs to be, first
     by seeing how many args are in the response files, and then adding
     on the args from the actual argv (getting this right the first time
     greatly simplifies the problem of eliminating memory leaks) */
  if (_hestArgsInResponseFiles(&argr, &nrf, _argv, err, parm)) {
    airMopDone(mop, AIR_TRUE); return 1;
  }
  argc = argr + _argc - nrf;
  /*
  printf("!%s: nrf = %d; argr = %d; _argc = %d --> argc = %d\n", 
	 me, nrf, argr, _argc, argc);
  */
  argv = calloc(argc+1, sizeof(char*));
  airMopMem(mop, &argv, AIR_FALSE);

  /* -------- process response file and set the remaining elements of argv */
  if (_hestResponseFiles(argv, _argv, nrf, err, parm, mop)) {
    airMopDone(mop, AIR_TRUE); return 1;
  }
  /*
  _hestPrintArgv(argc, argv);
  */
  /* -------- extract flags and their associated parameters from argv */
  if (_hestExtractFlagged(prms, nprm, appr, 
			   &argc, argv, 
			   opt,
			   err, parm, mop)) {
    airMopDone(mop, AIR_TRUE); return 1;
  }
  /*
  _hestPrintArgv(argc, argv);
  */
  /* -------- extract args for unflagged options */
  if (_hestExtractUnflagged(prms, nprm,
			    &argc, argv,
			    opt,
			    err, parm, mop)) {
    airMopDone(mop, AIR_TRUE); return 1;
  }

  /* any left over arguments indicate error */
  if (argc) {
    sprintf(err, "%sunexpected arg: \"%s\"", ME, argv[0]);
    airMopDone(mop, AIR_TRUE); return 1;
  }

  /* -------- learn defaults */
  if (_hestDefaults(prms, udflt, nprm, appr,
		    opt,
		    err, parm, mop)) {
    airMopDone(mop, AIR_TRUE); return 1;
  }
  
  /* -------- now, the actual parsing of values */
  if (_hestSetValues(prms, udflt, nprm, appr,
		     opt,
		     err, parm, mop)) {
    airMopDone(mop, AIR_TRUE); return 1;
  }

  airMopDone(mop, AIR_FALSE);
  if (_errP)
    *_errP = NULL;
  return 0;
}

/*
******** hestParseFree()
**
** free()s whatever was allocated by hestParse()
*/
void
hestParseFree(hestOpt *opt, hestParm *parm) {
  int op, i, numOpts;
  void **vP;
  char **str;
  char ***strP;

  numOpts = _hestNumOpts(opt);
  for (op=0; op<=numOpts-1; op++) {
    vP = opt[op].valueP;
    str = opt[op].valueP;
    strP = opt[op].valueP;
    switch (opt[op].alloc) {
    case 0:
      /* nothing was allocated */
      break;
    case 1:
      *vP = airFree(*vP);
      break;
    case 2:
      for (i=0; i<=opt[op].min-1; i++) {
	str[i] = airFree(str[i]);
      }
      break;
    case 3:
      for (i=0; i<=*(opt[op].sawP)-1; i++) {
	(*strP)[i] = airFree((*strP)[i]);
      }
      *strP = airFree(*strP);
      break;
    }
  }
  return;
}

