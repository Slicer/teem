#include <stdio.h>
#include <string.h>

#include <air.h>

#define BIFF_MAXKEYLEN 128  /* maximum allowed key length (not counting 
			       the null termination) */

extern void biffSet(char *key, char *err);
extern void biffAdd(char *key, char *err);
extern void biffDone(char *key);
extern void biffMove(char *destKey, char *err, char *srcKey);
extern char *biffGet(char *key);
extern char *biffGetDone(char *key);

/* some common error messages */
#define BIFF_NULL "%s: got NULL pointer"
#define BIFF_NRRDNEW "%s: couldn't create output nrrd struct"
#define BIFF_NRRDALLOC "%s: couldn't allocate space for output nrrd data"
