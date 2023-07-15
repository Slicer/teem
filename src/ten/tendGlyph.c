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

#include "ten.h"
#include "privateTen.h"

#define INFO "Generate postscript or ray-traced renderings of 3D glyphs"
static const char *_tend_glyphInfoL
  = (INFO ".  Whether the output is postscript or a ray-traced image is controlled "
          "by the initial \"rt\" flag (by default, the output is postscript). "
          "Because this is doing viz/graphics, many parameters need to be set. "
          "Use a response file to simplify giving the command-line options which "
          "aren't changing between invocations. "
          "The postscript output is an EPS file, suitable for including as a figure "
          "in LaTeX, or viewing with ghostview, or distilling into PDF. "
          "The ray-traced output is a 5 channel (R,G,B,A,T) float nrrd, suitable for "
          "\"unu crop -min 0 0 0 -max 2 M M \" followed by "
          "\"unu gamma\" and/or \"unu quantize -b 8\".");

#define _LIMNMAGIC "LIMN0000"

static int /* Biff: 1 */
_tendGlyphReadCams(int imgSize[2], limnCamera **camP, unsigned int *numCamsP,
                   FILE *fin) {
  static const char me[] = "_tendGlyphReadCams";
  char line[AIR_STRLEN_HUGE + 1];
  int ki;
  double di, dn, df, fr[3], at[3], up[3], va, dwell;
  airArray *mop, *camA;

  if (!(0 < airOneLine(fin, line, AIR_STRLEN_HUGE + 1) && !strcmp(_LIMNMAGIC, line))) {
    biffAddf(TEN, "%s: couldn't read first line or it wasn't \"%s\"", me, _LIMNMAGIC);
    return 1;
  }
  if (!(0 < airOneLine(fin, line, AIR_STRLEN_HUGE + 1)
        && 2
             == (airStrtrans(airStrtrans(line, '{', ' '), '}', ' '),
                 sscanf(line, "imgSize %d %d", imgSize + 0, imgSize + 1)))) {
    biffAddf(TEN,
             "%s: couldn't read second line or it wasn't "
             "\"imgSize <sizeX> <sizeY>\"",
             me);
    return 1;
  }

  mop = airMopNew();
  camA = airArrayNew((void **)camP, numCamsP, sizeof(limnCamera), 1);
  airMopAdd(mop, camA, (airMopper)airArrayNix, airMopAlways);

  while (0 < airOneLine(fin, line, AIR_STRLEN_HUGE + 1)) {
    airStrtrans(airStrtrans(line, '{', ' '), '}', ' ');
    ki = airArrayLenIncr(camA, 1);
    if (14
        != sscanf(line,
                  "cam.di %lg cam.at %lg %lg %lg "
                  "cam.up %lg %lg %lg cam.dn %lg cam.df %lg cam.va %lg "
                  "relDwell %lg cam.fr %lg %lg %lg",
                  &di, at + 0, at + 1, at + 2, up + 0, up + 1, up + 2, &dn, &df, &va,
                  &dwell, fr + 0, fr + 1, fr + 2)) {
      biffAddf(TEN, "%s: trouble parsing line %d: \"%s\"", me, ki, line);
      airMopError(mop);
      return 1;
    }
    (*camP)[ki].neer = dn;
    (*camP)[ki].faar = df;
    (*camP)[ki].dist = di;
    ELL_3V_COPY((*camP)[ki].from, fr);
    ELL_3V_COPY((*camP)[ki].at, at);
    ELL_3V_COPY((*camP)[ki].up, up);
    (*camP)[ki].fov = va;
    (*camP)[ki].aspect = (double)imgSize[0] / imgSize[1];
    (*camP)[ki].atRelative = AIR_FALSE;
    (*camP)[ki].orthographic = AIR_FALSE;
    (*camP)[ki].rightHanded = AIR_TRUE;
  }

  airMopOkay(mop);
  return 0;
}

static int
tend_glyphMain(int argc, const char **argv, const char *me, hestParm *hparm) {
  int pret, doRT = AIR_FALSE;
  hestOpt *hopt = NULL;
  char *perr, *err;
  airArray *mop;

  Nrrd *nin, *emap, *nraw, *npos, *nslc;
  char *outS;
  limnCamera *cam, *hackcams;
  limnObject *glyph;
  limnWindow *win;
  echoObject *rect = NULL;
  echoScene *scene;
  echoRTParm *eparm;
  echoGlobalState *gstate;
  tenGlyphParm *gparm;
  float bg[3], edgeColor[3], buvne[5], shadow, creaseAngle;
  int ires[2], slice[2], nobg, ambocc, concave;
  unsigned int hackci, hacknumcam;
  size_t hackmin[3] = {0, 0, 0}, hackmax[3] = {2, 0, 0};
  char *hackFN, hackoutFN[AIR_STRLEN_SMALL + 1];
  FILE *hackF;
  Nrrd *hacknpng, *hacknrgb;
  NrrdRange *hackrange;

  double v2w[9], ldir[3], edir[3], fdir[3], corn[3], len;

  /* so that command-line options can be read from file */
  hparm->respFileEnable = AIR_TRUE;
  hparm->elideSingleEmptyStringDefault = AIR_TRUE;

  mop = airMopNew();
  cam = limnCameraNew();
  airMopAdd(mop, cam, (airMopper)limnCameraNix, airMopAlways);
  glyph = limnObjectNew(1000, AIR_TRUE);
  airMopAdd(mop, glyph, (airMopper)limnObjectNix, airMopAlways);
  scene = echoSceneNew();
  airMopAdd(mop, scene, (airMopper)echoSceneNix, airMopAlways);
  win = limnWindowNew(limnDevicePS);
  airMopAdd(mop, win, (airMopper)limnWindowNix, airMopAlways);
  gparm = tenGlyphParmNew();
  airMopAdd(mop, gparm, (airMopper)tenGlyphParmNix, airMopAlways);
  eparm = echoRTParmNew();
  airMopAdd(mop, eparm, (airMopper)echoRTParmNix, airMopAlways);

  /* do postscript or ray-traced? */
  /* Do you see the problem here?
   *   hestOptAdd(&hopt, "rt", NULL, airTypeFloat, 0, 0, &doRT, NULL,
   * Since r1421 2003-05-29 13:47:41 this line has been creating a flag but passed a
   * bogus airTypeFloat, which is harmless because that type was ignored, but it is
   * still a bummer that it took 20 years to get a version of hestOptAdd with some
   * type checking */
  hestOptAdd_Flag(&hopt, "rt", &doRT,
                  "generate ray-traced output.  By default (not using this "
                  "option), postscript output is generated.");

  hestOptAdd_1_Int(&hopt, "v", "level", &(gparm->verbose), "0", "verbosity level");

  /* which points will rendered */
  hestOptAdd_1_Float(&hopt, "ctr", "conf thresh", &(gparm->confThresh), "0.5",
                     "Glyphs will be drawn only for tensors with confidence "
                     "values greater than this threshold");
  hestOptAdd_1_Enum(&hopt, "a", "aniso", &(gparm->anisoType), "fa",
                    "Which anisotropy metric to use for thresholding the data "
                    "points to be drawn",
                    tenAniso);
  hestOptAdd_1_Float(&hopt, "atr", "aniso thresh", &(gparm->anisoThresh), "0.5",
                     "Glyphs will be drawn only for tensors with anisotropy "
                     "greater than this threshold");
  hestOptAdd_1_Other(&hopt, "p", "pos array", &npos, "",
                     "Instead of being on a grid, tensors are at arbitrary locations, "
                     "as defined by this 3-by-N array of floats. Doing this makes "
                     "various other options moot",
                     nrrdHestNrrd);
  hestOptAdd_1_Other(&hopt, "m", "mask vol", &(gparm->nmask), "",
                     "Scalar volume (if any) for masking region in which glyphs are "
                     "drawn, in conjunction with \"mtr\" flag. ",
                     nrrdHestNrrd);
  hestOptAdd_1_Float(&hopt, "mtr", "mask thresh", &(gparm->maskThresh), "0.5",
                     "Glyphs will be drawn only for tensors with mask "
                     "value greater than this threshold");

  /* how glyphs will be shaped */
  hestOptAdd_1_Enum(&hopt, "g", "glyph shape", &(gparm->glyphType), "box",
                    "shape of glyph to use for display.  Possibilities "
                    "include \"box\", \"sphere\", \"cylinder\", and "
                    "\"superquad\"",
                    tenGlyphType);
  hestOptAdd_1_Float(&hopt, "sh", "sharpness", &(gparm->sqdSharp), "3.0",
                     "for superquadric glyphs, how much to sharp edges form as a "
                     "function of differences between eigenvalues.  Higher values "
                     "mean that edges form more easily");
  hestOptAdd_1_Float(&hopt, "gsc", "scale", &(gparm->glyphScale), "0.01",
                     "over-all glyph size in world-space");

  /* how glyphs will be colored */
  hestOptAdd_1_Int(&hopt, "c", "evector #", &(gparm->colEvec), "0",
                   "which eigenvector should determine coloring. "
                   "(formally \"v\") "
                   "\"0\", \"1\", \"2\" are principal, medium, and minor");
  hestOptAdd_1_Float(&hopt, "sat", "saturation", &(gparm->colMaxSat), "1.0",
                     "maximal saturation to use on glyph colors (use 0.0 to "
                     "create a black and white image)");
  hestOptAdd_1_Enum(&hopt, "ga", "aniso", &(gparm->colAnisoType), "fa",
                    "Which anisotropy metric to use for modulating the "
                    "saturation of the glyph color",
                    tenAniso);
  hestOptAdd_1_Float(&hopt, "am", "aniso mod", &(gparm->colAnisoModulate), "0.0",
                     "How much to modulate glyph color saturation by "
                     "anisotropy (as chosen by \"-ga\").  "
                     "If 1.0, then glyphs for zero anisotropy "
                     "data points will have no hue. ");
  hestOptAdd_1_Float(&hopt, "gg", "gray", &(gparm->colIsoGray), "1.0",
                     "desaturating glyph color due to low anisotropy "
                     "tends towards this gray level");
  hestOptAdd_1_Float(&hopt, "gam", "gamma", &(gparm->colGamma), "0.7",
                     "gamma to use on color components (after saturation)");
  hestOptAdd_1_Other(&hopt, "emap", "env map", &emap, "",
                     "environment map to use for shading glyphs.  By default, "
                     "there is no shading",
                     nrrdHestNrrd);
  hestOptAdd_4_Float(&hopt, "adsp", "phong", gparm->ADSP, "0 1 0 30",
                     "phong ambient, diffuse, specular components, "
                     "and specular power");
  hestOptAdd_3_Float(&hopt, "bg", "background", bg, "1 1 1",
                     "background RGB color; each component in range [0.0,1.0]");
  hestOptAdd_3_Float(&hopt, "ec", "edge rgb", edgeColor, "0 0 0",
                     "edge RGB color; each component in range [0.0,1.0]");

  /* parameters for showing a dataset slice */
  hestOptAdd_2_Int(&hopt, "slc", "axis pos", slice, "-1 -1",
                   "For showing a gray-scale slice of anisotropy: the axis "
                   "and position along which to slice.  Use \"-1 -1\" to signify "
                   "that no slice should be shown");
  hestOptAdd_1_Other(&hopt, "si", "slice image", &nslc, "",
                     "Instead of showing a slice of the anisotropy used to cull "
                     "glyphs, show something else. ",
                     nrrdHestNrrd);
  hestOptAdd_1_Float(&hopt, "off", "slice offset", &(gparm->sliceOffset), "0.0",
                     "Offset from slice position to render slice at (so that it "
                     "doesn't occlude glyphs).");
  hestOptAdd_1_Float(&hopt, "sg", "slice gamma", &(gparm->sliceGamma), "1.7",
                     "Gamma to apply to values on slice.");
  hestOptAdd_1_Float(&hopt, "sb", "slice bias", &(gparm->sliceBias), "0.05",
                     "amount by which to bump up slice gray values prior to gamma.");

  /* camera */
  limnHestCameraOptAdd(&hopt, cam,             /* */
                       NULL, "0 0 0", "0 0 1", /* fr, at, up*/
                       "-2", "0", "2",         /* dn, di, df */
                       "-1 1", "-1 1", "nan"); /* ur, vr, fv */

  /* postscript-specific options */
  hestOptAdd_1_Int(&hopt, "gr", "glyph res", &(gparm->facetRes), "10",
                   "(* postscript only *) "
                   "resolution of polygonalization of glyphs (all glyphs "
                   "other than the default box)");
  hestOptAdd_3_Float(&hopt, "wd", "3 widths", gparm->edgeWidth, "0.8 0.4 0.0",
                     "(* postscript only *) "
                     "width of edges drawn for three kinds of glyph "
                     "edges: silohuette, crease, non-crease");
  hestOptAdd_1_Float(&hopt, "psc", "scale", &(win->scale), "300",
                     "(* postscript only *) "
                     "scaling from screen space units to postscript units "
                     "(in points)");
  hestOptAdd_1_Float(&hopt, "ca", "angle", &creaseAngle, "70",
                     "(* postscript only *) "
                     "minimum crease angle");
  hestOptAdd_Flag(&hopt, "nobg", &nobg,
                  "(* postscript only *) "
                  "don't initially fill with background color");
  hestOptAdd_Flag(&hopt, "concave", &concave,
                  "use slightly buggy rendering method suitable for "
                  "concave or self-occluding objects");

  /* ray-traced-specific options */
  hestOptAdd_2_Int(&hopt, "is", "nx ny", ires, "256 256",
                   "(* ray-traced only *) "
                   "image size (resolution) to render");
  hestOptAdd_1_Int(&hopt, "ns", "# samp", &(eparm->numSamples), "4",
                   "(* ray-traced only *) "
                   "number of samples per pixel (must be a square number)");
  if (airThreadCapable) {
    hestOptAdd_1_Int(&hopt, "nt", "# threads", &(eparm->numThreads), "1",
                     "(* ray-traced only *) "
                     "number of threads to be used for rendering");
  }
  hestOptAdd_N_Float(&hopt, "al", "B U V N E", 5, buvne, "0 -1 -1 -4 0.7",
                     "(* ray-traced only *) "
                     "brightness (B), view-space location (U V N), "
                     "and length of edge (E) "
                     "of a square area light source, for getting soft shadows. "
                     "Requires lots more samples \"-ns\" to converge.  Use "
                     "brightness 0 (the default) to turn this off, and use "
                     "environment map-based shading (\"-emap\") instead. ");
  hestOptAdd_Flag(&hopt, "ao", &ambocc,
                  "set up 6 area lights in a box to approximate "
                  "ambient occlusion");
  hestOptAdd_1_Float(&hopt, "shadow", "s", &shadow, "1.0",
                     "the extent to which shadowing occurs");
  hestOptAdd_1_String(&hopt, "hack", "hack", &hackFN, "", "don't mind me");

  /* input/output */
  hestOptAdd_1_Other(&hopt, "i", "nin", &nin, "-", "input diffusion tensor volume",
                     nrrdHestNrrd);
  hestOptAdd_1_String(&hopt, "o", "nout", &outS, "-", "output file");

  airMopAdd(mop, hopt, (airMopper)hestOptFree, airMopAlways);
  USAGE_PARSE(_tend_glyphInfoL);
  airMopAdd(mop, hopt, (airMopper)hestParseFree, airMopAlways);

  /* set up slicing stuff */
  if (!(-1 == slice[0] && -1 == slice[1])) {
    gparm->doSlice = AIR_TRUE;
    gparm->sliceAxis = slice[0];
    gparm->slicePos = slice[1];
    gparm->sliceAnisoType = gparm->anisoType;
    /* gparm->sliceOffset set by hest */
  }

  if (npos) {
    fprintf(stderr, "!%s: have npos --> turning off onlyPositive \n", me);
    gparm->onlyPositive = AIR_FALSE;
  }

  if (gparm->verbose) {
    fprintf(stderr, "%s: verbose = %d\n", me, gparm->verbose);
  }
  if (tenGlyphGen(doRT ? NULL : glyph, doRT ? scene : NULL, gparm, nin, npos, nslc)) {
    airMopAdd(mop, err = biffGetDone(TEN), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble generating glyphs:\n%s\n", me, err);
    airMopError(mop);
    return 1;
  }
  if (AIR_EXISTS(cam->fov)) {
    if (limnCameraAspectSet(cam, ires[0], ires[1], nrrdCenterCell)) {
      airMopAdd(mop, err = biffGetDone(LIMN), airFree, airMopAlways);
      fprintf(stderr, "%s: trouble with camera:\n%s\n", me, err);
      airMopError(mop);
      return 1;
    }
  }
  /*
   * Prior to using limnHestCameraOptAdd above, cam->dist was explicitly set to 0 here;
   * now it is the default to "-di". But the "-ar" cam->atRelative flag has no default,
   * so there's no way to tell limnHestCameraOptAdd to set cam->atRelative = AIR_TRUE,
   * or to not include the "-ar" option. So at the risk of minor confusion we set here:
   */
  cam->atRelative = AIR_TRUE;
  if (limnCameraUpdate(cam)) {
    airMopAdd(mop, err = biffGetDone(LIMN), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble with camera:\n%s\n", me, err);
    airMopError(mop);
    return 1;
  }
  if (doRT) {
    nraw = nrrdNew();
    airMopAdd(mop, nraw, (airMopper)nrrdNuke, airMopAlways);
    gstate = echoGlobalStateNew();
    airMopAdd(mop, gstate, (airMopper)echoGlobalStateNix, airMopAlways);
    eparm->shadow = shadow;
    if (buvne[0] > 0) {
      ELL_34M_EXTRACT(v2w, cam->V2W);
      ELL_3MV_MUL(ldir, v2w, buvne + 1);
      ell_3v_perp_d(edir, ldir);
      ELL_3V_NORM(edir, edir, len);
      ELL_3V_CROSS(fdir, ldir, edir);
      ELL_3V_NORM(fdir, fdir, len);
      ELL_3V_SCALE(edir, buvne[4] / 2, edir);
      ELL_3V_SCALE(fdir, buvne[4] / 2, fdir);
      ELL_3V_ADD4(corn, cam->at, ldir, edir, fdir);
      rect = echoObjectNew(scene, echoTypeRectangle);
      echoRectangleSet(rect, corn[0], corn[1], corn[2], -edir[0] * 2, -edir[1] * 2,
                       -edir[2] * 2, -fdir[0] * 2, -fdir[1] * 2, -fdir[2] * 2);
      echoColorSet(rect, 1, 1, 1, 1);
      echoMatterLightSet(scene, rect, buvne[0], 0);
      echoObjectAdd(scene, rect);
    }
    if (ambocc) {
      double eye[3], llen;
      ELL_3V_SUB(eye, cam->from, cam->at);
      llen = 4 * ELL_3V_LEN(eye);

      ELL_3V_COPY(corn, cam->at);
      corn[0] -= llen / 2;
      corn[1] -= llen / 2;
      corn[2] -= llen / 2;
      rect = echoObjectNew(scene, echoTypeRectangle);
      echoRectangleSet(rect, corn[0], corn[1], corn[2], llen, 0, 0, 0, llen, 0);
      echoColorSet(rect, 1, 1, 1, 1);
      echoMatterLightSet(scene, rect, 1, AIR_CAST(echoCol_t, llen));
      echoObjectAdd(scene, rect);
      rect = echoObjectNew(scene, echoTypeRectangle);
      echoRectangleSet(rect, corn[0], corn[1], corn[2], 0, 0, llen, llen, 0, 0);
      echoColorSet(rect, 1, 1, 1, 1);
      echoMatterLightSet(scene, rect, 1, AIR_CAST(echoCol_t, llen));
      echoObjectAdd(scene, rect);
      rect = echoObjectNew(scene, echoTypeRectangle);
      echoRectangleSet(rect, corn[0], corn[1], corn[2], 0, llen, 0, 0, 0, llen);
      echoColorSet(rect, 1, 1, 1, 1);
      echoMatterLightSet(scene, rect, 1, AIR_CAST(echoCol_t, llen));
      echoObjectAdd(scene, rect);

      corn[0] += llen / 2;
      corn[1] += llen / 2;
      corn[2] += llen / 2;
      rect = echoObjectNew(scene, echoTypeRectangle);
      echoRectangleSet(rect, corn[0], corn[1], corn[2], 0, -llen, 0, -llen, 0, 0);
      echoColorSet(rect, 1, 1, 1, 1);
      echoMatterLightSet(scene, rect, 1, AIR_CAST(echoCol_t, llen));
      echoObjectAdd(scene, rect);
      rect = echoObjectNew(scene, echoTypeRectangle);
      echoRectangleSet(rect, corn[0], corn[1], corn[2], -llen, 0, 0, 0, 0, -llen);
      echoColorSet(rect, 1, 1, 1, 1);
      echoMatterLightSet(scene, rect, 1, AIR_CAST(echoCol_t, llen));
      echoObjectAdd(scene, rect);
      rect = echoObjectNew(scene, echoTypeRectangle);
      echoRectangleSet(rect, corn[0], corn[1], corn[2], 0, 0, -llen, 0, -llen, 0);
      echoColorSet(rect, 1, 1, 1, 1);
      echoMatterLightSet(scene, rect, 1, AIR_CAST(echoCol_t, llen));
      echoObjectAdd(scene, rect);
    }
    eparm->imgResU = ires[0];
    eparm->imgResV = ires[1];
    eparm->jitterType = (eparm->numSamples > 1 ? echoJitterJitter : echoJitterNone);
    eparm->aperture = 0;
    eparm->renderBoxes = AIR_FALSE;
    eparm->seedRand = AIR_FALSE;
    eparm->renderLights = AIR_FALSE;
    ELL_3V_COPY(scene->bkgr, bg);
    scene->envmap = emap;
    if (!airStrlen(hackFN)) {
      /* normal operation: one ray-tracing for one invocation */
      if (echoRTRender(nraw, cam, scene, eparm, gstate)) {
        airMopAdd(mop, err = biffGetDone(ECHO), airFree, airMopAlways);
        fprintf(stderr, "%s: trouble ray-tracing %s\n", me, err);
        airMopError(mop);
        return 1;
      }
      if (nrrdSave(outS, nraw, NULL)) {
        airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
        fprintf(stderr, "%s: trouble saving ray-tracing output %s\n", me, err);
        airMopError(mop);
        return 1;
      }
    } else {
      /* hack: multiple renderings per invocation */
      if (!(hackF = airFopen(hackFN, stdin, "rb"))) {
        fprintf(stderr, "%s: couldn't fopen(\"%s\",\"rb\"): %s\n", me, hackFN,
                strerror(errno));
        airMopError(mop);
        return 1;
      }
      if (_tendGlyphReadCams(ires, &hackcams, &hacknumcam, hackF)) {
        airMopAdd(mop, err = biffGetDone(TEN), airFree, airMopAlways);
        fprintf(stderr, "%s: trouble reading frames %s\n", me, err);
        airMopError(mop);
        return 1;
      }
      eparm->imgResU = ires[0];
      eparm->imgResV = ires[1];
      hackmax[1] = ires[0] - 1;
      hackmax[2] = ires[1] - 1;
      hacknrgb = nrrdNew();
      hacknpng = nrrdNew();
      airMopAdd(mop, hacknrgb, (airMopper)nrrdNuke, airMopAlways);
      airMopAdd(mop, hacknpng, (airMopper)nrrdNuke, airMopAlways);
      hackrange = nrrdRangeNew(0.0, 1.0);
      airMopAdd(mop, hackrange, (airMopper)nrrdRangeNix, airMopAlways);
      for (hackci = 0; hackci < hacknumcam; hackci++) {
        memcpy(cam, hackcams + hackci, sizeof(limnCamera));
        /* rightHanded and orthographic not handled nicely */

        if (rect) {
          if (limnCameraUpdate(cam)) {
            airMopAdd(mop, err = biffGetDone(LIMN), airFree, airMopAlways);
            fprintf(stderr, "%s: trouble with camera:\n%s\n", me, err);
            airMopError(mop);
            return 1;
          }
          ELL_34M_EXTRACT(v2w, cam->V2W);
          ELL_3MV_MUL(ldir, v2w, buvne + 1);
          ell_3v_perp_d(edir, ldir);
          ELL_3V_NORM(edir, edir, len);
          ELL_3V_CROSS(fdir, ldir, edir);
          ELL_3V_NORM(fdir, fdir, len);
          ELL_3V_SCALE(edir, buvne[4] / 2, edir);
          ELL_3V_SCALE(fdir, buvne[4] / 2, fdir);
          ELL_3V_ADD4(corn, cam->at, ldir, edir, fdir);
          echoRectangleSet(rect, corn[0], corn[1], corn[2], edir[0] * 2, edir[1] * 2,
                           edir[2] * 2, fdir[0] * 2, fdir[1] * 2, fdir[2] * 2);
        }

        if (echoRTRender(nraw, cam, scene, eparm, gstate)) {
          airMopAdd(mop, err = biffGetDone(ECHO), airFree, airMopAlways);
          fprintf(stderr, "%s: trouble ray-tracing %s\n", me, err);
          airMopError(mop);
          return 1;
        }
        sprintf(hackoutFN, "%04d.png", hackci);
        if (nrrdCrop(hacknrgb, nraw, hackmin, hackmax)
            || nrrdQuantize(hacknpng, hacknrgb, hackrange, 8)
            || nrrdSave(hackoutFN, hacknpng, NULL)) {
          airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
          fprintf(stderr, "%s: trouble saving output %s\n", me, err);
          airMopError(mop);
          return 1;
        }
      }
    }
  } else {
    if (!(win->file = airFopen(outS, stdout, "wb"))) {
      fprintf(stderr, "%s: couldn't fopen(\"%s\",\"wb\"): %s\n", me, outS,
              strerror(errno));
      airMopError(mop);
      return 1;
    }
    airMopAdd(mop, win->file, (airMopper)airFclose, airMopAlways);
    cam->neer = -0.000000001;
    cam->faar = 0.0000000001;
    win->ps.lineWidth[limnEdgeTypeBackFacet] = 0;
    win->ps.lineWidth[limnEdgeTypeBackCrease] = 0;
    win->ps.lineWidth[limnEdgeTypeContour] = gparm->edgeWidth[0];
    win->ps.lineWidth[limnEdgeTypeFrontCrease] = gparm->edgeWidth[1];
    win->ps.lineWidth[limnEdgeTypeFrontFacet] = gparm->edgeWidth[2];
    win->ps.lineWidth[limnEdgeTypeBorder] = 0;
    /* win->ps.lineWidth[limnEdgeTypeFrontCrease]; */
    win->ps.creaseAngle = creaseAngle;
    win->ps.noBackground = nobg;
    ELL_3V_COPY(win->ps.bg, bg);
    ELL_3V_COPY(win->ps.edgeColor, edgeColor);
    if (limnObjectRender(glyph, cam, win)
        || (concave ? limnObjectPSDrawConcave(glyph, cam, emap, win)
                    : limnObjectPSDraw(glyph, cam, emap, win))) {
      airMopAdd(mop, err = biffGetDone(LIMN), airFree, airMopAlways);
      fprintf(stderr, "%s: trouble drawing glyphs:\n%s\n", me, err);
      airMopError(mop);
      return 1;
    }
  }

  airMopOkay(mop);
  return 0;
}
TEND_CMD(glyph, INFO);
