/* testutil: minimal testing library functions for Teem
   Copyright (C) 2025  University of Chicago
   See ../../LICENSE.txt for licensing terms */

#ifndef TEEM_TESTUTIL_H
#define TEEM_TESTUTIL_H

#ifdef __cplusplus
extern "C" {
#endif

extern char *teemTestDataPath(const char *fName);
extern char *teemTestTmpPath(const char *fName);

#ifdef __cplusplus
}
#endif

#endif
