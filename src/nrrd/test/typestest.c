#include <stdio.h>

#define NRRD_CHAR signed char
#define NRRD_UCHAR unsigned char
#define NRRD_SHORT signed short int
#define NRRD_USHORT unsigned short int
#define NRRD_INT signed int
#define NRRD_UINT unsigned int
#define NRRD_LLONG signed long long int
#define NRRD_ULLONG unsigned long long int
#define NRRD_FLOAT float
#define NRRD_DOUBLE double
#define NRRD_LDOUBLE long double
#define NRRD_MED_STRLEN    257  /* phrases, single line of error message */

main() {
  char str1[NRRD_MED_STRLEN], str2[NRRD_MED_STRLEN], 
    str3[NRRD_MED_STRLEN];
  NRRD_CHAR c, c2;
  NRRD_UCHAR uc, uc2;
  NRRD_SHORT s, s2;
  NRRD_USHORT us, us2;
  NRRD_INT i, i2;
  NRRD_UINT ui, ui2;
  NRRD_LLONG lli, lli2;
  NRRD_ULLONG ulli, ulli2;
  NRRD_FLOAT f;
  NRRD_DOUBLE d;
  NRRD_LDOUBLE ld;

  c = c2 = 0;
  while (1) {
    c += 1;
    if (c < c2) {
      printf("char: %d -> %d\n", c, c2);
      break;
    }
    c2 = c;
  }

  uc = uc2 = 0;
  while (1) {
    uc -= 1;
    if (uc > uc2) {
      printf("unsigned char: %u -> %u\n", uc2, uc);
      break;
    }
    uc2 = uc;
  }

  s = s2 = 0;
  while (1) {
    s += 1;
    if (s < s2) {
      printf("short: %hd -> %hd\n", s, s2);
      break;
    }
    s2 = s;
  }

  us = us2 = 0;
  while (1) {
    us -= 1;
    if (us > us2) {
      printf("unsigned short: %hu -> %hu\n", us2, us);
      break;
    }
    us2 = us;
  }

  i = i2 = -2147483640;
  while (1) {
    i -= 1;
    if (i > i2) {
      printf("int: %d -> %d\n", i2, i);
      break;
    }
    i2 = i;
  }

  ui = ui2 = 0;
  while (1) {
    ui -= 1;
    if (ui > ui2) {
      printf("unsigned int: %u -> %u\n", ui2, ui);
      break;
    }
    ui2 = ui;
  }

  lli = lli2 = -9223372036854775800;
  while (1) {
    lli -= 1;
    if (lli > lli2) {
      printf("long long int %lld -> %lld\n", lli2, lli);
      break;
    }
    lli2 = lli;
  }

  ulli = ulli2 = 0;
  while (1) {
    ulli -= 1;
    if (ulli > ulli2) {
      printf("unsigned long long int %llu -> %llu\n", ulli2, ulli);
      break;
    }
    ulli2 = ulli;
  }

  printf("sizeof(NRRD_CHAR) = %d\n", sizeof(NRRD_CHAR));
  printf("sizeof(NRRD_UCHAR) = %d\n", sizeof(NRRD_UCHAR));
  printf("sizeof(NRRD_SHORT) = %d\n", sizeof(NRRD_SHORT));
  printf("sizeof(NRRD_USHORT) = %d\n", sizeof(NRRD_USHORT));
  printf("sizeof(NRRD_INT) = %d\n", sizeof(NRRD_INT));
  printf("sizeof(NRRD_UINT) = %d\n", sizeof(NRRD_UINT));
  printf("sizeof(NRRD_LLONG) = %d\n", sizeof(NRRD_LLONG));
  printf("sizeof(NRRD_ULLONG) = %d\n", sizeof(NRRD_ULLONG));
  printf("sizeof(NRRD_FLOAT) = %d\n", sizeof(NRRD_FLOAT));
  printf("sizeof(NRRD_DOUBLE) = %d\n", sizeof(NRRD_DOUBLE));
  printf("sizeof(NRRD_LDOUBLE) = %d\n", sizeof(NRRD_LDOUBLE));
  printf("sizeof(size_t) = %d\n", sizeof(size_t));

  c = -10;
  uc = 10;
  s = -10;
  us = 10;
  i = -10;
  ui = 10;
  lli = -10;
  ulli = 10;
  f = 3.14159324234098320948172304987123;
  d = 3.14159324234098320948172304987123;
  ld = 3.14159324234098320948172304987123;
  
  printf("c: %d\n", c);
  printf("uc: %u\n", uc);
  printf("s: %hd\n", s);
  printf("us: %hu\n", us);
  printf("i: %d\n", i);
  printf("ui: %u\n", ui);
  printf("lli: %lld\n", lli);
  printf("ulli: %llu\n", ulli);
  printf("f: %f\n", f);
  printf("d: %lf\n", d);
  printf("ld: %Lf\n", ld);

  sprintf(str1, "-10");
  sprintf(str2, "10");
  sprintf(str3, "3.14159324234098320948172304987123");

  /*
  sscanf(str1, "%d", &c);
  sscanf(str2, "%u", &uc);
  */
  sscanf(str1, "%hd", &s);
  sscanf(str2, "%hu", &us);
  sscanf(str1, "%d", &i);
  sscanf(str2, "%u", &ui);
  sscanf(str1, "%lld", &lli);
  sscanf(str2, "%llu", &ulli);
  sscanf(str3, "%f", &f);
  sscanf(str3, "%lf", &d);
  sscanf(str3, "%Lf", &ld);

  printf("\n");
  printf("c: %d\n", c);
  printf("uc: %u\n", uc);
  printf("s: %hd\n", s);
  printf("us: %hu\n", us);
  printf("i: %d\n", i);
  printf("ui: %u\n", ui);
  printf("lli: %lld\n", lli);
  printf("ulli: %llu\n", ulli);
  printf("f: %f\n", f);
  printf("d: %lf\n", d);
  printf("ld: %Lf\n", ld);

}
