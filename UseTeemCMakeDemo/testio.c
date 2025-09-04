/*
  testio.c: demos nrrdSanity(), nrrdLoad(), nrrdSave from Teem
  Copyright (C) 2025

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any
  damages arising from the use of this software.

  Permission is granted to anyone to use this software for any
  purpose, including commercial applications, and to alter it and
  redistribute it freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must
     not claim that you wrote the original software. If you use this
     software in a product, an acknowledgment in the product
     documentation would be appreciated but is not required.

  2. Altered source versions must be plainly marked as such, and must
     not be misrepresented as being the original software.

  3. This notice may not be removed or altered from any source distribution.
*/

#include <teem/nrrd.h>

static void
usage(const char *me) {
  /*                      0      1        2      (3) */
  fprintf(stderr, "usage: %s [<input> [<output>]]\n", me);
  fprintf(stderr, "<input> is file for nrrdLoad() to read (or - for stdin)\n");
  fprintf(stderr, "<output> is where nrrdSave() writes to (or - for stout)\n");
}

int
main(int argc, const char *argv[]) {
  int ret;
  const char *me;
  char *err;

  me = argv[0];
  if (argc > 3 || (argc == 2 && !strcmp("--help", argv[1]))) {
    usage(me);
    return 1;
  }

  /* we start by running nrrdSanity in any case */
  if (!nrrdSanity()) {
    fprintf(stderr, "%s: nrrdSanity() failed:\n%s", me, err = biffGetDone(NRRD));
    free(err);
    return 1;
  }
  /* else */
  fprintf(stderr, "%s: nrrdSanity() passed\n", me);
  if (argc <= 1) {
    /* we're done */
    return 1;
  }

  const char *fin = argv[1];
  Nrrd *nrrd = nrrdNew();
  if (nrrdLoad(nrrd, fin, NULL)) {
    fprintf(stderr, "%s: trouble loading \"%s\":\n%s", me, fin, err = biffGet(NRRD));
    free(err);
    return 1;
  } else {
    printf("%s: loaded array from \"%s\"\n", me, fin);
  }

  /* describe the nrrd just read in */
  printf("%s: Describing array:\n", me);
  nrrdDescribe(stdout, nrrd);
  if (argc == 2) {
    /* we're done */
    return 1;
  }

  /* write out the array */
  const char *fout = argv[2];
  if (nrrdSave(fout, nrrd, NULL)) {
    fprintf(stderr, "%s: trouble writing to\"%s\":\n%s", me, fout, err = biffGet(NRRD));
    free(err);
    return 1;
  } else {
    printf("%s: saved array to \"%s\"\n", me, fout);
  }

  return 0;
}
