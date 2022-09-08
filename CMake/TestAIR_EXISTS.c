/* In the context of making a configure-time test, this has to
   to be completely stand-alone, rather than being able to draw
   upon things as they are set up in Teem sources. These definitions
   are copied from Teem source.

   Specifically: AIR_EXISTS has to match what is in teem/src/air/air.h,
   or else this test is testing the wrong thing, and it becomes useless
   for helping the pre-processor do the right thing for #define AIR_EXISTS
   in teem/src/air/air.h
*/

#include <stdio.h>

/* from teem/src/air/air.h */
typedef union {
  unsigned int i;
  float f;
} airFloat;
/* from teem/src/air/754.c: this could actually be a quiet NaN
   or a signalling NaN, but that shouldn't make a difference for
   this configure-time test  */
const airFloat airFloatQNaN = {0x7fffffff};
const airFloat airFloatPosInf = {0x7f800000};
const airFloat airFloatNegInf = {0xff800000};

#define AIR_CAST(t, v) ((t)(v))
/* MUST BE COPIED DIRECTLY FROM from air.h !! */
#define AIR_EXISTS(x)  (AIR_CAST(int, !((x) - (x))))

int
main(int argc, char *argv[]) {
  (void)argc; /* quiet warnings about unused */
  (void)argv; /* quiet warnings about unused */
  float nanF = airFloatQNaN.f;
  float pinfF = airFloatPosInf.f;
  float ninfF = airFloatNegInf.f;
  float piF = 3.1415926535897932384626433f;
  double nanD = (double)nanF;
  double pinfD = (double)pinfF;
  double ninfD = (double)ninfF;
  float piD = 3.1415926535897932384626433;
  int ret;
  if (AIR_EXISTS(nanF) || AIR_EXISTS(pinfF) || AIR_EXISTS(ninfF)    /* */
      || AIR_EXISTS(nanD) || AIR_EXISTS(pinfD) || AIR_EXISTS(ninfD) /* */
      || !AIR_EXISTS(piF) || !AIR_EXISTS(piD)                       /* */
      || !AIR_EXISTS(0.0f) || !AIR_EXISTS(0.0)) {
    printf("No, AIR_EXISTS FAILS!\n");
    ret = 1;
  } else {
    /* all is well */
    printf("Yes, AIR_EXISTS seems to work\n");
    ret = 0;
  }
  return ret;
}
