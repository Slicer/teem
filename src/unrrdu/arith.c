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


#include <nrrd.h>
#include <math.h>
#include <string.h>

int
usage(char *me) {
                      /*  0      1      2       3         4 */
  fprintf(stderr, "usage: %s <NameIn1> <op> <NameIn2> <NameOut>\n", me);
  fprintf(stderr, "       <op> is '+', '-', '*', '/', '%%', 'm', 'M', 'p', 'e'\n");
  return 1;
}

int
main(int argc, char *argv[]) {
  FILE *fin = NULL;
  char *me, *err, *op, *in1Str, *in2Str, *outStr;
  Nrrd *nin = NULL, *nin1 = NULL, *nin2 = NULL, *nout = NULL;
  double op1, op2, result;
  nrrdBigInt i, len;
  double (*look1)(void *, nrrdBigInt) = NULL, 
    (*look2)(void *, nrrdBigInt) = NULL;

  me = argv[0];
  if (!(argc == 5))
    return usage(me);

  in1Str = argv[1];
  op = argv[2];
  in2Str = argv[3];
  outStr = argv[4];
  if (!(op[0] == '+' ||
	op[0] == '-' ||
	op[0] == '*' ||
	op[0] == '/' ||
	op[0] == '%' ||
	op[0] == 'm' ||
	op[0] == 'M' ||
	op[0] == 'p' ||
	op[0] == 'e'
	)) {
    fprintf(stderr, "%s: didn't get one of the supported operations\n", me);
    return usage(me);
  }
  /* fprintf(stderr, "%s: in1Str = |%s| (%d)\n", 
     me, in1Str, !strcmp(in1Str, "-")); */
  if (!strcmp(in1Str, "-") || (fin = fopen(in1Str, "r"))) {
    if (fin)
      fclose(fin);
    if (nrrdLoad(nin1=nrrdNew(), in1Str)) {
      err = biffGet(NRRD);
      fprintf(stderr, "%s: error reading first nrrd:\n%s\n", me, err);
      free(err);
      return 1;
    }
  }
  else {
    if (!(1 == sscanf(in1Str, "%lg", &op1))) {
      fprintf(stderr, "%s: can't open %s or parse it as float\n", me, in1Str);
      return 1;
    }
    fprintf(stderr, "%s: op1 is constant %g\n", me, op1);
  }

  /* fprintf(stderr, "%s: in2Str = |%s| (%d)\n", 
     me, in2Str, !strcmp(in2Str, "-")); */
  if (!strcmp(in2Str, "-") || (fin = fopen(in2Str, "r"))) {
    if (fin)
      fclose(fin);
    if (nrrdLoad(nin2=nrrdNew(), in2Str)) {
      err = biffGet(NRRD);
      fprintf(stderr, "%s: error reading second nrrd:\n%s\n", me, err);
      free(err);
      return 1;
    }
  }
  else {
    if (!(1 == sscanf(in2Str, "%lg", &op2))) {
      fprintf(stderr, "%s: can't open %s or parse it as float\n", me, in2Str);
      return 1;
    }
    fprintf(stderr, "%s: op2 is constant %g\n", me, op2);
  }

  if (!(nin1 || nin2)) {
    /* we do need at least one nrrd */
    fprintf(stderr, "%s: need at least one nrrd, got two constants\n", me);
    return 1;
  }
  nin = nin1 ? nin1 : nin2;

  if (nin1 && nin2) {
    /* see if they are compatible
       took out the enforcement of type equality */
    
    if (!(nrrdElementNumber(nin1) == nrrdElementNumber(nin2) &&
	  nin1->dim == nin2->dim)) {
      fprintf(stderr, "%s: (num,type,dim): (%d,%d,%d) != (%d,%d,%d)\n", me,
	      (int)nrrdElementNumber(nin1), nin1->type, nin1->dim,
	      (int)nrrdElementNumber(nin2), nin2->type, nin2->dim);
      return 1;
    }
  }

  /* we copy even though we'll be over-writing the data */
  if (nrrdCopy(nout=nrrdNew(), nin)) {
    err = biffGet(NRRD);
    fprintf(stderr, "%s: nrrdNewCopy failed:\n%s\n", me, err);
    free(err);
    return 1;
  }

  look1 = nin1 ? nrrdDLookup[nin1->type] : NULL;
  look2 = nin2 ? nrrdDLookup[nin2->type] : NULL;
  len = nrrdElementNumber(nin);
  for (i=0; i<=len-1; i++) {
    if (look1) {
      op1 = look1(nin1->data, i);
    }
    if (look2) {
      op2 = look2(nin2->data, i);
    }
    switch (op[0]) {
    case '+':
      result = op1 + op2;
      break;
    case '-':
      result = op1 - op2;
      break;
    case '*':
      result = op1 * op2;
      break;
    case '/':
      result = op1 / op2;
      break;
    case '%':
      result = ((int)op1) % ((int)op2);
      break;
    case 'm':
      result = AIR_MIN(op1,op2);
      break;
    case 'M':
      result = AIR_MAX(op1,op2);
      break;
    case 'p':
      result = pow(op1, op2);
      break;
    case 'e':
      result = AIR_EXISTS(op1) ? op1 : op2;
      break;
    };
    nrrdDInsert[nin->type](nout->data, i, result);
  }

  if (nrrdSave(outStr, nout, NULL)) {
    err = biffGet(NRRD);
    fprintf(stderr, "%s: trouble in nrrdSave:\n%s\n", me, err);
    free(err);
    return 1;
  }

  nrrdNuke(nin1);
  nrrdNuke(nin2);
  nrrdNuke(nout);
  return 0;
}
