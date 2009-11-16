/*
  Teem: Tools to process and visualize scientific data and images              
  Copyright (C) 2008, 2007, 2006, 2005  Gordon Kindlmann
  Copyright (C) 2004, 2003, 2002, 2001, 2000, 1999, 1998  University of Utah

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public License
  (LGPL) as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  The terms of redistributing and/or modifying this software also
  include exceptions to the LGPL that facilitate static linking.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with this library; if not, write to Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "meet.h"


meetPullVol *
meetPullVolNew(void) {
  meetPullVol *ret;

  ret = AIR_CAST(meetPullVol *, calloc(1, sizeof(meetPullVol)));
  if (ret) {
    ret->kind = NULL;
    ret->fileName = ret->volName = NULL;
    ret->scaleDerivNorm = ret->leeching = AIR_FALSE;
    ret->numSS = 0;
    ret->rangeSS[0] = ret->rangeSS[1] = AIR_NAN;
    ret->scalePos = NULL;
    ret->nin = NULL;
    ret->ninSS = NULL;
  }
  return ret;
}

/*
******** meetPullVolParse
**
** parses a string to extract all the information necessary to create
** the pullVolume (this function originated in Deft/src/main-pull.c)
*/
int
meetPullVolParse(meetPullVol *mpv, const char *_str) {
  static const char me[]="meetPullVolParse";
#define VFMT_SHRT "<fileName>:<kind>:<volName>"
#define SFMT "<minScl>-<#smp>-<maxScl>[-SN]"
#define VFMT_LONG "<fileName>:<kind>:" SFMT ":<volName>"
  char *str, *ctok, *clast=NULL, *dtok, *dlast=NULL;
  airArray *mop;
  int wantSS;

  if (!(mpv && _str)) {
    biffAddf(MEET, "%s: got NULL pointer", me);
    return 1;
  }
  if (!( str = airStrdup(_str) )) {
    biffAddf(MEET, "%s: couldn't strdup input", me);
    return 1;
  }

  mop = airMopNew();
  airMopAdd(mop, str, airFree, airMopAlways);
  if (!( 3 == airStrntok(str, ":") || 4 == airStrntok(str, ":") )) {
    biffAddf(MEET, "%s: didn't get 3 or 4 \":\"-separated tokens in \"%s\"; "
             "not of form " VFMT_SHRT " or " VFMT_LONG , me, _str);
    airMopError(mop); return 1;
  }
  mpv->nin = nrrdNew();
  airMopAdd(mop, &(mpv->nin), (airMopper)airSetNull, airMopOnError);
  airMopAdd(mop, mpv->nin, (airMopper)nrrdNuke, airMopOnError);
  wantSS = (4 == airStrntok(str, ":"));
  
  ctok = airStrtok(str, ":", &clast);
  if (!(mpv->fileName = airStrdup(ctok))) {
    biffAddf(MEET, "%s: couldn't strdup fileName", me);
    airMopError(mop); return 1;
  }
  airMopAdd(mop, &(mpv->fileName), (airMopper)airSetNull, airMopOnError);
  airMopAdd(mop, mpv->fileName, airFree, airMopOnError);
  ctok = airStrtok(NULL, ":", &clast);
  if (!(mpv->kind = meetConstGageKindParse(ctok))) {
    biffAddf(MEET, "%s: couldn't parse \"%s\" as kind", me, ctok);
    airMopError(mop); return 1;
  }
  if (wantSS) {
    int maybeSN;
    ctok = airStrtok(NULL, ":", &clast);
    if (!( 3 == airStrntok(ctok, "-") || 4 == airStrntok(ctok, "-") )) {
      biffAddf(MEET, "%s: didn't get 3 or 4 \"-\"-separated tokens in \"%s\"; "
               "not of form SFMT" , me, ctok);
      airMopError(mop); return 1;
    }
    maybeSN = (4 == airStrntok(ctok, "-"));
    dtok = airStrtok(ctok, "-", &dlast);
    if (1 != sscanf(dtok, "%lg", &(mpv->rangeSS[0]))) {
      biffAddf(MEET, "%s: couldn't parse \"%s\" as min scale", me, dtok);
      airMopError(mop); return 1;
    }
    dtok = airStrtok(NULL, "-", &dlast);
    if (1 != sscanf(dtok, "%u", &(mpv->numSS))) {
      biffAddf(MEET, "%s: couldn't parse \"%s\" as # scale samples", me, dtok);
      airMopError(mop); return 1;
    }
    if (!( mpv->numSS >= 2 )) {
      biffAddf(MEET, "%s: need # scale samples >= 2 (not %u)", me, mpv->numSS);
      airMopError(mop); return 1;
    }
    dtok = airStrtok(NULL, "-", &dlast);
    if (1 != sscanf(dtok, "%lg", &(mpv->rangeSS[1]))) {
      biffAddf(MEET, "%s: couldn't parse \"%s\" as max scale", me, dtok);
      airMopError(mop); return 1;
    }
    if (maybeSN) {
      dtok = airStrtok(NULL, "-", &dlast);
      if (strcmp("SN", dtok)) {
        biffAddf(MEET, "%s: expected \"-SN\" at end of scale spec not \"-%s\"",
                 me, dtok);
        airMopError(mop); return 1;
      } else {
        mpv->scaleDerivNorm = AIR_TRUE;
      }
    } else {
      mpv->scaleDerivNorm = AIR_FALSE;
    }
    mpv->ninSS = AIR_CAST(Nrrd **, calloc(mpv->numSS, sizeof(Nrrd *)));
    airMopAdd(mop, &(mpv->ninSS), (airMopper)airSetNull, airMopOnError);
    airMopAdd(mop, mpv->ninSS, airFree, airMopOnError);
    mpv->scalePos = AIR_CAST(double *, calloc(mpv->numSS, sizeof(double)));
    airMopAdd(mop, &(mpv->scalePos), (airMopper)airSetNull, airMopOnError);
    airMopAdd(mop, mpv->scalePos, airFree, airMopOnError);
    /* we don't actually create nrrds nor load the volumes here,
       because we don't know cachePath, and because we might want
       different pullVolumes to share the same underlying Nrrds */
  } else {
    mpv->numSS = 0;
    mpv->rangeSS[0] = mpv->rangeSS[1] = AIR_NAN;
    mpv->ninSS = NULL;
    mpv->scalePos = NULL;
    mpv->scaleDerivNorm = AIR_FALSE;
  }
  ctok = airStrtok(NULL, ":", &clast);
  if (!(mpv->volName = airStrdup(ctok))) {
    biffAddf(MEET, "%s: couldn't strdup volName", me);
    airMopError(mop); return 1;
  }
  airMopAdd(mop, &(mpv->volName), (airMopper)airSetNull, airMopOnError);
  airMopAdd(mop, mpv->volName, airFree, airMopOnError);
  
  if (strchr(ctok, '-')) {
    biffAddf(MEET, "%s: you probably don't want \"-\" in volume name \"%s\"; "
             "forgot last \":\" in scale sampling specification?", me, ctok);
    airMopError(mop); return 1;
  }
  
  airMopOkay(mop);
  return 0;
}

int
meetHestPullVolParse(void *ptr, char *str, char err[AIR_STRLEN_HUGE]) {
  static const char me[]="meetHestPullVolParse";
  meetPullVol *mpv, **mpvP;
  airArray *mop;

  if (!(ptr && str)) {
    sprintf(err, "%s: got NULL pointer", me);
    return 1;
  }
  mop = airMopNew();
  mpvP = AIR_CAST(meetPullVol **, ptr);
  *mpvP = mpv = meetPullVolNew();
  airMopAdd(mop, mpvP, (airMopper)airSetNull, airMopOnError);
  airMopAdd(mop, mpv, (airMopper)meetPullVolNuke, airMopOnError);
  if (meetPullVolParse(mpv, str)) {
    char *err;
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopOnError);
    strncpy(err, err, AIR_STRLEN_HUGE-1);
    airMopError(mop);
    return 1;
  }
  airMopOkay(mop);
  return 0;
}

/*
******** meetPullVolNix
**
** this frees stuff allocated by ???
*/
meetPullVol *
meetPullVolNuke(meetPullVol *mpv) {

  if (mpv) {
    if (!mpv->leeching) {
      nrrdNuke(mpv->nin);
    }
    if (mpv->numSS) {
      unsigned int ssi;
      for (ssi=0; ssi<mpv->numSS; ssi++) {
        if (!mpv->leeching) {
          nrrdNuke(mpv->ninSS[ssi]);
        }
      }
      airFree(mpv->ninSS);
      airFree(mpv->scalePos);
    }
    airFree(mpv->fileName);
    airFree(mpv->volName);
    airFree(mpv);
  }
  return NULL;
}

hestCB
_meetHestPullVol = {
  sizeof(meetPullVol *),
  "meetPullVol",
  meetHestPullVolParse,
  (airMopper)meetPullVolNuke,
}; 

hestCB *
meetHestPullVol = &_meetHestPullVol;

/*
******** meetPullVolLeechable
**
** indicates whether lchr can leech from orig
*/
int
meetPullVolLeechable(const meetPullVol *orig,
                     const meetPullVol *lchr) {
  int can;

  can = !!strcmp(orig->fileName, "-");  /* can, if not reading from stdin */
  can &= !strcmp(orig->fileName, lchr->fileName);  /* come from same file */
  can &= (orig->kind == lchr->kind);               /* same kind */
  /* need to have different volname */
  can &= (orig->numSS == lchr->numSS);             /* same scale space */
  if (lchr->numSS) {
    /* DO allow difference in scaleDerivNorm (the main reason for leeching) */
    can &= (orig->rangeSS[0] == lchr->rangeSS[0]);   /* same SS range */
    can &= (orig->rangeSS[1] == lchr->rangeSS[1]);
  }
  return can;
}

