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

char *me;

void
usage() {
                      /*  0      1      2       3         4 */
  fprintf(stderr, "usage: %s <NameIn1> <op> <NameIn2> <NameOut>\n", me);
  fprintf(stderr, "       <op> is '+', '-', '*', '/', 'm', 'M', 'e'\n");
  exit(1);
}

int
main(int argc, char *argv[]) {
  FILE *fin, *fout;
  char *err, *op;
  Nrrd *nin = NULL, *nin1 = NULL, *nin2 = NULL, *nout = NULL;
  double op1, op2, result;
  NRRD_BIG_INT i, len;
  double (*look1)(void *, NRRD_BIG_INT) = NULL, 
    (*look2)(void *, NRRD_BIG_INT) = NULL;

  me = argv[0];
  if (!(argc == 5))
    usage();

  op = argv[2];
  if (!(op[0] == '+' ||
	op[0] == '-' ||
	op[0] == '*' ||
	op[0] == '/' ||
	op[0] == 'm' ||
	op[0] == 'M' ||
	op[0] == 'e'
	)) {
    fprintf(stderr, "%s: didn't get one of the supported operations\n", me);
    usage();
  }
  if (fin = fopen(argv[1], "r")) {
    if (!(nin1 = nrrdNewRead(fin))) {
      err = biffGet(NRRD);
      fprintf(stderr, "%s: error reading first nrrd:%s\n", me, err);
      exit(1);
    }
    fclose(fin);
    /* OKAY */
  }
  else {
    if (!(1 == sscanf(argv[1], "%lg", &op1))) {
      fprintf(stderr, "%s: can't open %s or parse it as float\n", me, argv[1]);
      exit(1);
    }
    printf("%s: op1 is constant %g\n", me, op1);
  }

  if (fin = fopen(argv[3], "r")) {
    if (!(nin2 = nrrdNewRead(fin))) {
      err = biffGet(NRRD);
      fprintf(stderr, "%s: error reading second nrrd:%s\n", me, err);
      exit(1);
    }
    fclose(fin);
  }
  else {
    if (!(1 == sscanf(argv[3], "%lg", &op2))) {
      fprintf(stderr, "%s: can't open %s or parse it as float\n", me, argv[3]);
      exit(1);
    }
    printf("%s: op2 is constant %g\n", me, op2);
  }

  if (!(nin1 || nin2)) {
    /* we do need at least one nrrd */
    fprintf(stderr, "%s: need at least one nrrd, got two constants\n", me);
    exit(1);
  }
  nin = nin1 ? nin1 : nin2;

  if (nin1 && nin2) {
    /* see if they are compatible
       took out the enforcement of type equality */
    if (!(nin1->num == nin2->num &&
	  nin1->dim == nin2->dim)) {
      fprintf(stderr, "%s: (num,type,dim): (%d,%d,%d) != (%d,%d,%d)\n", me,
	      (int)nin1->num, nin1->type, nin1->dim,
	      (int)nin2->num, nin2->type, nin2->dim);
      exit(1);
    }
  }

  /* we copy even though we'll be over-writing the data */
  if (!(nout = nrrdNewCopy(nin))) {
    fprintf(stderr, "%s: nrrdNewCopy failed:\n%s\n", 
	    me, biffGet(NRRD));
    exit(1);
  }

  look1 = nin1 ? nrrdDLookup[nin1->type] : NULL;
  look2 = nin2 ? nrrdDLookup[nin2->type] : NULL;
  len = nin1 ? nin1->num : nin2->num;
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
    case 'm':
      result = AIR_MIN(op1,op2);
      break;
    case 'M':
      result = AIR_MAX(op1,op2);
      break;
    case 'e':
      result = AIR_EXISTS(op1) ? op1 : op2;
      break;
    };
    nrrdDInsert[nin->type](nout->data, i, result);
  }

  if (!(fout = fopen(argv[4], "w"))) {
    fprintf(stderr, "%s: couldn't open %s for writing\n", me, argv[4]);
    exit(1);
  }
  if (strlen(argv[4]) > 5 &&
      nout->dim == 2 &&
      nout->type == nrrdTypeUChar &&
      !strcmp(".pgm", argv[4] + strlen(argv[4]) - 4)) {
    if (nrrdWritePNM(fout, nout)) {
      fprintf(stderr, "%s: trouble writing as PGM:\n%s\n", me, 
	      biffGet(NRRD));
      exit(1);
    }
  }
  else {
    if (nrrdWrite(fout, nout)) {
      fprintf(stderr, "%s: trouble in nrrdWrite:\n%s\n", me, biffGet(NRRD));
      exit(1);
    }
  }
  fclose(fout);

  nrrdNuke(nin1);
  nrrdNuke(nin2);
  nrrdNuke(nout);
  exit(0);
}
