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

#include "../hest.h"

static int
pos_parse(void *_ptr, const char *str, char *err) {
  double *ptr;
  int ret;

  ptr = _ptr;
  ret = sscanf(str, "%lf,%lf", ptr + 0, ptr + 1);
  if (2 != ret) {
    sprintf(err, "parsed %d (not 2) doubles", ret);
    return 1;
  }
  return 0;
}

hestCB posCB = {2 * sizeof(double), "position", pos_parse, NULL};

typedef struct {
  char *str;
  double val;
} Quat;

static int
quat_parse(void *_ptr, const char *str, char *err) {
  Quat **ptrP;
  Quat *ptr;

  ptrP = _ptr;
  ptr = (*ptrP) = AIR_MALLOC(1, Quat);
  /* printf("%s: ptrP = %p  ---malloc-->  ptr = *ptrP = %p\n", __func__, ptrP, *ptrP); */
  ptr->str = NULL;
  if (1 != sscanf(str, "%lf", &(ptr->val))) {
    sprintf(err, "didn't parse a double from %s", str);
    return 1;
  }
  ptr->str = airStrdup(str);
  return 0;
}

Quat *
quat_free(void *_ptr) {
  Quat *ptr = _ptr;
  if (ptr) {
    free(ptr->str);
    free(ptr);
  }
  return NULL;
}

hestCB quatCB = {sizeof(Quat *), "quatty", quat_parse, (airMopper)quat_free};

int
main(int argc, const char **argv) {
  hestOpt *opt = NULL;
  hestParm *parm;
  char *err = NULL,
       info[] = "This program does nothing in particular, though it does attempt "
                "to pose as some sort of command-line image processing program. "
                "As usual, any implied functionality is purely coincidental, "
                "especially since this is the output of a gray-haired unicyclist.";

  parm = hestParmNew();
  parm->respFileEnable = AIR_TRUE;
  parm->respectDashDashHelp = AIR_TRUE;
  parm->noArgsIsNoProblem = AIR_TRUE;
  parm->dieLessVerbose = AIR_TRUE;
  parm->verbosity = 0;

  opt = NULL;
  /* going past C89 to have declarations here */
  int flag;
  hestOptAdd_Flag(&opt, "f,flag", &flag, "a flag created via hestOptAdd_Flag");

  int b1;
  hestOptAdd_1_Bool(&opt, "b1", "bool1", &b1, "false", "test of hestOptAdd_1_Bool");
  int i1;
  hestOptAdd_1_Int(&opt, "i1", "int1", &i1, "42", "test of hestOptAdd_1_Int");
  unsigned int ui1;
  hestOptAdd_1_UInt(&opt, "ui1", "uint1", &ui1, "42", "test of hestOptAdd_1_UInt");
  long int li1;
  hestOptAdd_1_LongInt(&opt, "li1", "lint1", &li1, "42", "test of hestOptAdd_1_LongInt");
  unsigned long int uli1;
  hestOptAdd_1_ULongInt(&opt, "uli1", "ulint1", &uli1, "42",
                        "test of hestOptAdd_1_ULongInt");
  size_t sz1;
  hestOptAdd_1_Size_t(&opt, "sz1", "size1", &sz1, "42", "test of hestOptAdd_1_Size_t");
  float fl1;
  hestOptAdd_1_Float(&opt, "fl1", "float1", &fl1, "4.2", "test of hestOptAdd_1_Float");
  double db1;
  hestOptAdd_1_Double(&opt, "db1", "double1", &db1, "4.2",
                      "test of hestOptAdd_1_Double");
  char c1;
  hestOptAdd_1_Char(&opt, "c1", "char1", &c1, "x", "test of hestOptAdd_1_Char");
  char *s1;
  hestOptAdd_1_String(&opt, "s1", "string1", &s1, "bingo bob",
                      "test of hestOptAdd_1_String");
  int e1;
  hestOptAdd_1_Enum(&opt, "e1", "enum1", &e1, "little", "test of hestOptAdd_1_Enum",
                    airEndian);
  double p1[2];
  hestOptAdd_1_Other(&opt, "p1", "pos", &p1, "1.5,5.25", "test of hestOptAdd_1_Other A",
                     &posCB);
  Quat *q1;
  hestOptAdd_1_Other(&opt, "q1", "quat", &q1, "12.34", "test of hestOptAdd_1_Other B",
                     &quatCB);

  int b1v;
  hestOptAdd_1v_Bool(&opt, "b1v", "bool1", &b1v, "false", "test of hestOptAdd_1v_Bool");
  int i1v;
  hestOptAdd_1v_Int(&opt, "i1v", "int1", &i1v, "42", "test of hestOptAdd_1v_Int");
  unsigned int ui1v;
  hestOptAdd_1v_UInt(&opt, "ui1v", "uint1", &ui1v, "42", "test of hestOptAdd_1v_UInt");
  long int li1v;
  hestOptAdd_1v_LongInt(&opt, "li1v", "lint1", &li1v, "42",
                        "test of hestOptAdd_1v_LongInt");
  unsigned long int uli1v;
  hestOptAdd_1v_ULongInt(&opt, "uli1v", "ulint1", &uli1v, "42",
                         "test of hestOptAdd_1v_ULongInt");
  size_t sz1v;
  hestOptAdd_1v_Size_t(&opt, "sz1v", "size1", &sz1v, "42",
                       "test of hestOptAdd_1v_Size_t");
  float fl1v;
  hestOptAdd_1v_Float(&opt, "fl1v", "float1", &fl1v, "0.0",
                      "test of hestOptAdd_1v_Float");
  double db1v;
  hestOptAdd_1v_Double(&opt, "db1v", "double1", &db1v, "4.2",
                       "test of hestOptAdd_1v_Double");
  char c1v;
  hestOptAdd_1v_Char(&opt, "c1v", "char1", &c1v, "x", "test of hestOptAdd_1v_Char");
  char *s1v;
  hestOptAdd_1v_String(&opt, "s1v", "string1", &s1v, "bingo",
                       "test of hestOptAdd_1v_String");
  int e1v;
  hestOptAdd_1v_Enum(&opt, "e1v", "enum1", &e1v, "little", "test of hestOptAdd_1v_Enum",
                     airEndian);
  double p1v[2];
  hestOptAdd_1v_Other(&opt, "p1v", "pos", &p1v, "1.5,5.25",
                      "test of hestOptAdd_1v_Other A", &posCB);
  Quat *q1v;
  hestOptAdd_1v_Other(&opt, "q1v", "quat", &q1v, "12.34",
                      "test of hestOptAdd_1v_Other B", &quatCB);

  int b2[2];
  hestOptAdd_2_Bool(&opt, "b2", "bool1 bool2", b2, "true false",
                    "test of hestOptAdd_2_Bool");
  int i2[2];
  hestOptAdd_2_Int(&opt, "i2", "int1 int2", i2, "42 24", "test of hestOptAdd_2_Int");
  unsigned int ui2[2];
  hestOptAdd_2_UInt(&opt, "ui2", "uint1 uint2", ui2, "42 24",
                    "test of hestOptAdd_2_UInt");
  long int li2[2];
  hestOptAdd_2_LongInt(&opt, "li2", "lint1 lint2", li2, "42 24",
                       "test of hestOptAdd_2_LongInt");
  unsigned long int uli2[2];
  hestOptAdd_2_ULongInt(&opt, "uli2", "ulint1 ulint2", uli2, "42 24",
                        "test of hestOptAdd_2_ULongInt");
  size_t sz2[2];
  hestOptAdd_2_Size_t(&opt, "sz2", "size1 size2", sz2, "42 24",
                      "test of hestOptAdd_2_Size_t");
  float fl2[2];
  hestOptAdd_2_Float(&opt, "fl2", "float1 float2", fl2, "4.2 2.4",
                     "test of hestOptAdd_2_Float");
  double db2[2];
  hestOptAdd_2_Double(&opt, "db2", "double1 double2", db2, "4.2 2.4",
                      "test of hestOptAdd_2_Double");
  char c2[2];
  hestOptAdd_2_Char(&opt, "c2", "char1 char2", c2, "x y", "test of hestOptAdd_2_Char");
  char *s2[2];
  hestOptAdd_2_String(&opt, "s2", "str1 str2", s2, "bingo bob",
                      "test of hestOptAdd_2_String");
  int e2[2];
  hestOptAdd_2_Enum(&opt, "e2", "enum1 enum2", e2, "little big",
                    "test of hestOptAdd_2_Enum", airEndian);
  double p2[2][2];
  hestOptAdd_2_Other(&opt, "p2", "pos1 pos2", p2, "1.5,5.25  2.9,9.2",
                     "test of hestOptAdd_2_Other A", &posCB);
  Quat *q2[2];
  hestOptAdd_2_Other(&opt, "q2", "quat1 quat2", q2, "12.34  43.21",
                     "test of hestOptAdd_2_Other B", &quatCB);

  int b3[3];
  hestOptAdd_3_Bool(&opt, "b3", "bool1 bool2 bool3", b3, "true false true",
                    "test of hestOptAdd_3_Bool");
  int i3[3];
  hestOptAdd_3_Int(&opt, "i3", "int1 int2 int3", i3, "43 3 34",
                   "test of hestOptAdd_3_Int");
  unsigned int ui3[3];
  hestOptAdd_3_UInt(&opt, "ui3", "uint1 uint2 uint3", ui3, "43 3 34",
                    "test of hestOptAdd_3_UInt");
  long int li3[3];
  hestOptAdd_3_LongInt(&opt, "li3", "lint1 lint2 lint3", li3, "43 4 34",
                       "test of hestOptAdd_3_LongInt");
  unsigned long int uli3[3];
  hestOptAdd_3_ULongInt(&opt, "uli3", "ulint1 ulint2 ulint3", uli3, "43 5 34",
                        "test of hestOptAdd_3_ULongInt");
  size_t sz3[3];
  hestOptAdd_3_Size_t(&opt, "sz3", "size1 size2 size3", sz3, "43 6 34",
                      "test of hestOptAdd_3_Size_t");
  float fl3[3];
  hestOptAdd_3_Float(&opt, "fl3", "float1 float2 float3", fl3, "4.3 1.1 3.4",
                     "test of hestOptAdd_3_Float");
  double db3[3];
  hestOptAdd_3_Double(&opt, "db3", "double1 double2 double3", db3, "4.3 2.2 3.4",
                      "test of hestOptAdd_3_Double");
  char c3[3];
  hestOptAdd_3_Char(&opt, "c3", "char1 char2 char3", c3, "x y z",
                    "test of hestOptAdd_3_Char");
  char *s3[3];
  hestOptAdd_3_String(&opt, "s3", "str1 str2 str3", s3, "bingo bob susan",
                      "test of hestOptAdd_3_String");
  int e3[3];
  hestOptAdd_3_Enum(&opt, "e3", "enum1 enum2 enum3", e3, "little big little",
                    "test of hestOptAdd_3_Enum", airEndian);
  double p3[3][2];
  hestOptAdd_3_Other(&opt, "p3", "pos1 pos2 pos3", p3, "1.5,5.35  3.9,9.3  6.7,7.6",
                     "test of hestOptAdd_3_Other A", &posCB);
  Quat *q3[3];
  hestOptAdd_3_Other(&opt, "q3", "quat1 quat2 quat3", q3, "13.34  43.31  66.77",
                     "test of hestOptAdd_3_Other B", &quatCB);

  int b4[4];
  hestOptAdd_4_Bool(&opt, "b4", "bool1 bool2 bool3 bool4", b4, "true false no true",
                    "test of hestOptAdd_4_Bool");
  int i4[4];
  hestOptAdd_4_Int(&opt, "i4", "int1 int2 int3 int4", i4, "44 4 33 44",
                   "test of hestOptAdd_4_Int");
  unsigned int ui4[4];
  hestOptAdd_4_UInt(&opt, "ui4", "uint1 uint2 uint3 uint4", ui4, "44 4 33 44",
                    "test of hestOptAdd_4_UInt");
  long int li4[4];
  hestOptAdd_4_LongInt(&opt, "li4", "lint1 lint2 lint3 lint4", li4, "44 4 33 44",
                       "test of hestOptAdd_4_LongInt");
  unsigned long int uli4[4];
  hestOptAdd_4_ULongInt(&opt, "uli4", "ulint1 ulint2 ulint3 ulint4", uli4, "44 5 33 44",
                        "test of hestOptAdd_4_ULongInt");
  size_t sz4[4];
  hestOptAdd_4_Size_t(&opt, "sz4", "size1 size2 size3 size4", sz4, "44 6 33 44",
                      "test of hestOptAdd_4_Size_t");
  float fl4[4];
  hestOptAdd_4_Float(&opt, "fl4", "float1 float2 float3 float4", fl4, "4.4 1.1 3.3 4.4",
                     "test of hestOptAdd_4_Float");
  double db4[4];
  hestOptAdd_4_Double(&opt, "db4", "double1 double2 double3 double4", db4,
                      "4.4 2.2 3.3 4.4", "test of hestOptAdd_4_Double");
  char c4[4];
  hestOptAdd_4_Char(&opt, "c4", "char1 char2 char3 char4", c4, "x y z w",
                    "test of hestOptAdd_4_Char");
  char *s4[4];
  hestOptAdd_4_String(&opt, "s4", "str1 str2 str3 str4", s4, "bingo bob frank susan",
                      "test of hestOptAdd_4_String");
  int e4[4];
  hestOptAdd_4_Enum(&opt, "e4", "enum1 enum2 enum3 enum4", e4, "little big big little",
                    "test of hestOptAdd_4_Enum", airEndian);
  double p4[4][2];
  hestOptAdd_4_Other(&opt, "p4", "pos1 pos2 pos3 pos4", p4,
                     "1.5,5.45  4.9,9.4  6.7,7.6  63.4,97,3",
                     "test of hestOptAdd_4_Other A", &posCB);
  Quat *q4[4];
  hestOptAdd_4_Other(&opt, "q4", "quat1 quat2 quat3 quat4", q4,
                     "14.44  44.41  66.77  88.99", "test of hestOptAdd_4_Other B",
                     &quatCB);

  int b5[5];
  hestOptAdd_N_Bool(&opt, "b5", "bool1 bool2 bool3 bool4 bool5", 5, b5,
                    "true false no yes true", "test of hestOptAdd_N_Bool");
  int i5[5];
  hestOptAdd_N_Int(&opt, "i5", "int1 int2 int3 int4 int5", 5, i5, "55 5 33 500 55",
                   "test of hestOptAdd_N_Int");
  unsigned int ui5[5];
  hestOptAdd_N_UInt(&opt, "ui5", "uint1 uint2 uint3 uint4 uint5", 5, ui5,
                    "55 5 33 500 55", "test of hestOptAdd_N_UInt");
  long int li5[5];
  hestOptAdd_N_LongInt(&opt, "li5", "lint1 lint2 lint3 lint4 lint5", 5, li5,
                       "55 5 33 500 55", "test of hestOptAdd_N_LongInt");
  unsigned long int uli5[5];
  hestOptAdd_N_ULongInt(&opt, "uli5", "ulint1 ulint2 ulint3 ulint4 ulint5", 5, uli5,
                        "55 5 33 500 55", "test of hestOptAdd_N_ULongInt");
  size_t sz5[5];
  hestOptAdd_N_Size_t(&opt, "sz5", "size1 size2 size3 size5", 5, sz5, "55 6 33 500 55",
                      "test of hestOptAdd_N_Size_t");
  float fl5[5];
  hestOptAdd_N_Float(&opt, "fl5", "float1 float2 float3 float4 float5", 5, fl5,
                     "5.5 1.1 3.3 500 5.5", "test of hestOptAdd_N_Float");
  double db5[5];
  hestOptAdd_N_Double(&opt, "db5", "double1 double2 double3 double4 double5", 5, db5,
                      "5.5 2.2 3.3 4.4 5.5", "test of hestOptAdd_N_Double");
  char c5[5];
  hestOptAdd_N_Char(&opt, "c5", "char1 char2 char3 char4 char5", 5, c5, "x y z w v",
                    "test of hestOptAdd_N_Char");
  char *s5[5];
  hestOptAdd_N_String(&opt, "s5", "str1 str2 str3 str4, str5", 5, s5,
                      "bingo bob frank harry susan", "test of hestOptAdd_N_String");
  int e5[5];
  hestOptAdd_N_Enum(&opt, "e5", "enum1 enum2 enum3 enum4 enum5", 5, e5,
                    "little big little big little", "test of hestOptAdd_N_Enum",
                    airEndian);
  double p5[5][2];
  hestOptAdd_N_Other(&opt, "p5", "pos1 pos2 pos3 pos4 pos5", 5, p5,
                     "1.5,5.55  5.9,9.5  6.7,7.6  63.5,97,3  300,400",
                     "test of hestOptAdd_N_Other A", &posCB);
  Quat *q5[5];
  hestOptAdd_N_Other(&opt, "q5", "quat1 quat2 quat3 quat4 quat5", 5, q5,
                     "15.55  55.51  66.77  88.99  100.2", "test of hestOptAdd_N_Other B",
                     &quatCB);

  /* HEY also try 0, -1 */
  int *bv;
  unsigned int bvSaw;
  hestOptAdd_Nv_Bool(&opt, "bv", "bool1", 1, -1, &bv, "true false",
                     "test of hestOptAdd_Nv_Bool", &bvSaw);
  int *iv;
  unsigned int ivSaw;
  hestOptAdd_Nv_Int(&opt, "iv", "int1", 1, -1, &iv, "42 24", "test of hestOptAdd_Nv_Int",
                    &ivSaw);
  unsigned int *uiv;
  unsigned int uivSaw;
  hestOptAdd_Nv_UInt(&opt, "uiv", "uint1", 1, -1, &uiv, "42 24",
                     "test of hestOptAdd_Nv_UInt", &uivSaw);
  long int *liv;
  unsigned int livSaw;
  hestOptAdd_Nv_LongInt(&opt, "liv", "lint1", 1, -1, &liv, "42 24",
                        "test of hestOptAdd_Nv_LongInt", &livSaw);
  unsigned long int *uliv;
  unsigned int ulivSaw;
  hestOptAdd_Nv_ULongInt(&opt, "uliv", "ulint1", 1, -1, &uliv, "42 24",
                         "test of hestOptAdd_Nv_ULongInt", &ulivSaw);
  size_t *szv;
  unsigned int szvSaw;
  hestOptAdd_Nv_Size_t(&opt, "szv", "size1", 1, -1, &szv, "42 24",
                       "test of hestOptAdd_Nv_Size_t", &szvSaw);
  float *flv;
  unsigned int flvSaw;
  hestOptAdd_Nv_Float(&opt, "flv", "float1", 1, -1, &flv, "4.2 2.4",
                      "test of hestOptAdd_Nv_Float", &flvSaw);
  double *dbv;
  unsigned int dbvSaw;
  hestOptAdd_Nv_Double(&opt, "dbv", "double1", 1, -1, &dbv, "4.2 2.4",
                       "test of hestOptAdd_Nv_Double", &dbvSaw);
  char *cv;
  unsigned int cvSaw;
  hestOptAdd_Nv_Char(&opt, "cv", "char1", 1, -1, &cv, "x y",
                     "test of hestOptAdd_Nv_Char", &cvSaw);
  char **sv;
  unsigned int svSaw;
  hestOptAdd_Nv_String(&opt, "sv", "str1", 1, -1, &sv, "bingo bob",
                       "test of hestOptAdd_Nv_String", &svSaw);
  int *ev;
  unsigned int evSaw;
  hestOptAdd_Nv_Enum(&opt, "ev", "enum1", 1, -1, &ev, "little big",
                     "test of hestOptAdd_Nv_Enum", &evSaw, airEndian);
  double *pv;
  unsigned int pvSaw;
  hestOptAdd_Nv_Other(&opt, "pv", "pos1", 1, -1, &pv, "1.5,5.25  2.9,9.2",
                      "test of hestOptAdd_Nv_Other A", &pvSaw, &posCB);
  Quat **qv;
  unsigned int qvSaw;
  hestOptAdd_Nv_Other(&opt, "qv", "quat1", 1, -1, &qv, "12.34  43.21",
                      "test of hestOptAdd_Nv_Other B", &qvSaw, &quatCB);

  if (2 == argc && !strcmp("decls", argv[1])) {
    printf("Writing decls.h and then bailing\n");
    FILE *ff = fopen("decls.h", "w");
    hestOptAddDeclsPrint(ff);
    fclose(ff);
    exit(0);
  }
  /* else not writing decls.h; remove it to ensure freshness */
  remove("decls.h");

  hestParseOrDie(opt, argc - 1, argv + 1, parm, argv[0], info, AIR_TRUE, AIR_TRUE,
                 AIR_TRUE);

  if (0) {
    unsigned int opi, numO;
    numO = hestOptNum(opt);
    for (opi = 0; opi < numO; opi++) {
      printf("opt %u/%u:\n", opi, numO);
      printf("  flag=%s; ", opt[opi].flag ? opt[opi].flag : "(null)");
      printf("  name=%s\n", opt[opi].name ? opt[opi].name : "(null)");
      printf("  source=%s; ", hestSourceDefault == opt[opi].source
                                ? "default"
                                : (hestSourceUser == opt[opi].source ? "user" : "???"));
      printf("  parmStr=|%s|\n", opt[opi].parmStr ? opt[opi].parmStr : "(null)");
    }
  }
  printf("(err = %s)\n", err ? err : "(null)");
  printf("flag = %d\n\n", flag);

  printf("b1 = %d\n", b1);
  printf("i1 = %d\n", i1);
  printf("ui1 = %u\n", ui1);
  printf("li1 = %ld\n", li1);
  printf("uli1 = %lu\n", uli1);
  printf("sz1 = %zu\n", sz1);
  printf("fl1 = %g\n", fl1);
  printf("db1 = %g\n", db1);
  printf("c1 = |%c| (%d)\n", c1, c1);
  printf("s1 = |%s|\n", s1);
  printf("e1 = %d\n", e1);
  printf("p1 = %g,%g\n", p1[0], p1[1]);
  printf("q1 (@ %p) = %g(%s)\n", q1, q1->val, q1->str);
  printf("\n");

  printf("b1v = %d\n", b1v);
  printf("i1v = %d\n", i1v);
  printf("ui1v = %u\n", ui1v);
  printf("li1v = %ld\n", li1v);
  printf("uli1v = %lu\n", uli1v);
  printf("sz1v = %zu\n", sz1v);
  printf("fl1v = %g\n", fl1v);
  printf("db1v = %g\n", db1v);
  printf("c1v = |%c| (%d)\n", c1v, c1v);
  printf("s1v = |%s|\n", s1v);
  printf("e1v = %d\n", e1v);
  printf("p1v = %g,%g\n", p1v[0], p1v[1]);
  printf("q1v (@ %p) = %g(%s)\n", q1v, q1v->val, q1v->str);
  printf("\n");

  printf("b2 = %d %d\n", b2[0], b2[1]);
  printf("i2 = %d %d\n", i2[0], i2[1]);
  printf("ui2 = %u %u\n", ui2[0], ui2[1]);
  printf("li2 = %ld %ld\n", li2[0], li2[1]);
  printf("uli2 = %lu %lu\n", uli2[0], uli2[1]);
  printf("sz2 = %zu %zu\n", sz2[0], sz2[1]);
  printf("fl2 = %g %g\n", fl2[0], fl2[1]);
  printf("db2 = %g %g\n", db2[0], db2[1]);
  printf("c2 = %c %c\n", c2[0], c2[1]);
  printf("s2 = |%s| |%s|\n", s2[0], s2[1]);
  printf("e2 = %d %d\n", e2[0], e2[1]);
  printf("p2 = %g,%g  %g,%g\n", p2[0][0], p2[0][1], p2[1][0], p2[1][1]);
  printf(" (q2 = %p : [%p %p])\n", q2, q2[0], q2[1]);
  printf("q2 = %g(%s)  %g(%s)\n", q2[0]->val, q2[0]->str, q2[1]->val, q2[1]->str);

  printf("b3 = %d %d %d\n", b3[0], b3[1], b3[2]);
  printf("i3 = %d %d %d\n", i3[0], i3[1], i3[2]);
  printf("ui3 = %u %u %u\n", ui3[0], ui3[1], ui3[2]);
  printf("li3 = %ld %ld %ld\n", li3[0], li3[1], li3[2]);
  printf("uli3 = %lu %lu %lu\n", uli3[0], uli3[1], uli3[2]);
  printf("sz3 = %zu %zu %zu\n", sz3[0], sz3[1], sz3[2]);
  printf("fl3 = %g %g %g\n", fl3[0], fl3[1], fl3[2]);
  printf("db3 = %g %g %g\n", db3[0], db3[1], db3[2]);
  printf("c3 = %c %c %c\n", c3[0], c3[1], c3[2]);
  printf("s3 = |%s| |%s| |%s|\n", s3[0], s3[1], s3[2]);
  printf("e3 = %d %d %d\n", e3[0], e3[1], e3[2]);
  printf("p3 = %g,%g  %g,%g  %g,%g\n", p3[0][0], p3[0][1], p3[1][0], p3[1][1], p3[2][0],
         p3[2][1]);
  printf("q3 = %g(%s)  %g(%s)  %g(%s)\n", q3[0]->val, q3[0]->str, q3[1]->val, q3[1]->str,
         q3[2]->val, q3[2]->str);

  printf("b4 = %d %d %d %d\n", b4[0], b4[1], b4[2], b4[3]);
  printf("i4 = %d %d %d %d\n", i4[0], i4[1], i4[2], i4[3]);
  printf("ui4 = %u %u %u %u\n", ui4[0], ui4[1], ui4[2], ui4[3]);
  printf("li4 = %ld %ld %ld %ld\n", li4[0], li4[1], li4[2], li4[3]);
  printf("uli4 = %lu %lu %lu %lu\n", uli4[0], uli4[1], uli4[2], uli4[3]);
  printf("sz4 = %zu %zu %zu %zu\n", sz4[0], sz4[1], sz4[2], sz4[3]);
  printf("fl4 = %g %g %g %g\n", fl4[0], fl4[1], fl4[2], fl4[3]);
  printf("db4 = %g %g %g %g\n", db4[0], db4[1], db4[2], db4[3]);
  printf("c4 = %c %c %c %c\n", c4[0], c4[1], c4[2], c4[3]);
  printf("s4 = |%s| |%s| |%s| |%s|\n", s4[0], s4[1], s4[2], s4[3]);
  printf("e4 = %d %d %d %d\n", e4[0], e4[1], e4[2], e4[3]);
  printf("p4 = %g,%g  %g,%g  %g,%g  %g,%g\n", p4[0][0], p4[0][1], p4[1][0], p4[1][1],
         p4[2][0], p4[2][1], p4[3][0], p4[3][1]);
  printf("q4 = %g(%s)  %g(%s)  %g(%s)  %g(%s)\n", q4[0]->val, q4[0]->str, q4[1]->val,
         q4[1]->str, q4[2]->val, q4[2]->str, q4[3]->val, q4[3]->str);

  printf("b5 = %d %d %d %d %d\n", b5[0], b5[1], b5[2], b5[3], b5[4]);
  printf("i5 = %d %d %d %d %d\n", i5[0], i5[1], i5[2], i5[3], i5[4]);
  printf("ui5 = %u %u %u %u %u\n", ui5[0], ui5[1], ui5[2], ui5[3], ui5[4]);
  printf("li5 = %ld %ld %ld %ld %ld\n", li5[0], li5[1], li5[2], li5[3], li5[4]);
  printf("uli5 = %lu %lu %lu %lu %lu\n", uli5[0], uli5[1], uli5[2], uli5[3], uli5[4]);
  printf("sz5 = %zu %zu %zu %zu %zu\n", sz5[0], sz5[1], sz5[2], sz5[3], sz5[4]);
  printf("fl5 = %g %g %g %g %g\n", fl5[0], fl5[1], fl5[2], fl5[3], fl5[4]);
  printf("db5 = %g %g %g %g %g\n", db5[0], db5[1], db5[2], db5[3], db5[4]);
  printf("c5 = %c %c %c %c %c\n", c5[0], c5[1], c5[2], c5[3], c5[4]);
  printf("s5 = |%s| |%s| |%s| |%s| |%s|\n", s5[0], s5[1], s5[2], s5[3], s5[4]);
  printf("e5 = %d %d %d %d %d\n", e5[0], e5[1], e5[2], e5[3], e5[4]);
  printf("p5 = %g,%g  %g,%g  %g,%g  %g,%g  %g,%g\n", p5[0][0], p5[0][1], p5[1][0],
         p5[1][1], p5[2][0], p5[2][1], p5[3][0], p5[3][1], p5[4][0], p5[4][1]);
  printf("q5 = %g(%s)  %g(%s)  %g(%s)  %g(%s)  %g(%s)\n", q5[0]->val, q5[0]->str,
         q5[1]->val, q5[1]->str, q5[2]->val, q5[2]->str, q5[3]->val, q5[3]->str,
         q5[4]->val, q5[4]->str);

  unsigned int ii;
  printf("bv (%u) =", bvSaw);
  for (ii = 0; ii < bvSaw; ii++) {
    printf(" %d", bv[ii]);
  }
  printf("\n");
  printf("iv (%u) =", ivSaw);
  for (ii = 0; ii < ivSaw; ii++) {
    printf(" %d", iv[ii]);
  }
  printf("\n");
  printf("uiv (%u) =", uivSaw);
  for (ii = 0; ii < uivSaw; ii++) {
    printf(" %u", uiv[ii]);
  }
  printf("\n");
  printf("liv (%u) =", livSaw);
  for (ii = 0; ii < livSaw; ii++) {
    printf(" %ld", liv[ii]);
  }
  printf("\n");
  printf("uliv (%u) =", ulivSaw);
  for (ii = 0; ii < ulivSaw; ii++) {
    printf(" %lu", uliv[ii]);
  }
  printf("\n");
  printf("szv (%u) =", szvSaw);
  for (ii = 0; ii < szvSaw; ii++) {
    printf(" %zu", szv[ii]);
  }
  printf("\n");
  printf("flv (%u) =", flvSaw);
  for (ii = 0; ii < flvSaw; ii++) {
    printf(" %g", flv[ii]);
  }
  printf("\n");
  printf("dbv (%u) =", dbvSaw);
  for (ii = 0; ii < dbvSaw; ii++) {
    printf(" %g", dbv[ii]);
  }
  printf("\n");
  printf("cv (%u) =", cvSaw);
  for (ii = 0; ii < cvSaw; ii++) {
    printf(" |%c|", cv[ii]);
  }
  printf("\n");
  printf("sv (%u) =", svSaw);
  for (ii = 0; ii < svSaw; ii++) {
    printf(" |%s|", sv[ii]);
  }
  printf("\n");
  printf("ev (%u) =", evSaw);
  for (ii = 0; ii < evSaw; ii++) {
    printf(" %d", ev[ii]);
  }
  printf("\n");
  printf("pv (%u) =", pvSaw);
  for (ii = 0; ii < pvSaw; ii++) {
    printf("  %g,%g", pv[0 + 2 * ii], pv[1 + 2 * ii]);
  }
  printf("\n");
  printf("qv (%u) =", qvSaw);
  for (ii = 0; ii < qvSaw; ii++) {
    printf("  %g(%s)", qv[ii]->val, qv[ii]->str);
  }
  printf("\n");

  /* free the memory allocated by parsing ... */
  hestParseFree(opt);
  /* ... and the other stuff */
  opt = hestOptFree(opt);
  parm = hestParmFree(parm);
  exit(0);
}
