/*
  teem: Gordon Kindlmann's research software
  Copyright (C) 2004, 2003, 2002, 2001, 2000, 1999, 1998 University of Utah

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "ten.h"
#include "privateTen.h"

#define INFO "Generate postscript or ray-traced renderings of 3D glyphs"
char *_tend_glyphInfoL =
  (INFO
   ".  Whether the output is postscript or a ray-traced image is controlled "
   "by the initial \"rt\" flag (by default, the output is postscript). "
   "Because this is doing viz/graphics, many parameters need to be set. "
   "Use a response file to simplify giving the command-line options which "
   "aren't changing between invocations. "
   "The postscript output is an EPS file, suitable for including as a figure "
   "in LaTeX, or viewing with ghostview, or distilling into PDF. "
   "The ray-traced output is a 5 channel (R,G,B,A,T) float nrrd, suitable for "
   "\"unu crop -min 0 0 0 -max 2 M M \" followed by "
   "\"unu gamma\" and/or \"unu quantize -b 8\".");

int
tend_glyphMain(int argc, char **argv, char *me, hestParm *hparm) {
  int pret, doRT = AIR_FALSE;
  hestOpt *hopt = NULL;
  char *perr, *err;
  airArray *mop;

  Nrrd *nten, *emap, *nraw, *npos, *nslc;
  char *outS;
  limnCamera *cam;
  limnObject *glyph;
  limnWindow *win;
  echoScene *scene;
  echoRTParm *eparm;
  echoGlobalState *gstate;
  tenGlyphParm *gparm;
  float bg[3];
  int ires[2], slice[2], nobg;

  /* so that command-line options can be read from file */
  hparm->respFileEnable = AIR_TRUE;
  hparm->elideSingleEmptyStringDefault = AIR_TRUE;

  mop = airMopNew();
  cam = limnCameraNew();
  airMopAdd(mop, cam, (airMopper)limnCameraNix, airMopAlways);
  glyph = limnObjectNew(512, AIR_TRUE);
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
  hestOptAdd(&hopt, "rt", NULL, airTypeFloat, 0, 0, &doRT, NULL,
	     "generate ray-traced output.  By default (not using this "
	     "option), postscript output is generated.");
  
  /* which points will rendered */
  hestOptAdd(&hopt, "ctr", "conf thresh", airTypeFloat, 1, 1,
	     &(gparm->confThresh), "0.5",
	     "Glyphs will be drawn only for tensors with confidence "
	     "values greater than this threshold");
  hestOptAdd(&hopt, "a", "aniso", airTypeEnum, 1, 1,
	     &(gparm->anisoType), "fa",
	     "Which anisotropy metric to use for thresholding the data "
	     "points to be drawn", NULL, tenAniso);
  hestOptAdd(&hopt, "atr", "aniso thresh", airTypeFloat, 1, 1,
	     &(gparm->anisoThresh), "0.5",
	     "Glyphs will be drawn only for tensors with anisotropy "
	     "greater than this threshold");
  hestOptAdd(&hopt, "p", "pos array", airTypeOther, 1, 1, &npos, "",
	     "Instead of being on a grid, tensors are at arbitrary locations, "
	     "as defined by this 3-by-N array of floats. Doing this makes "
	     "various other options moot. ", NULL, NULL,
	     nrrdHestNrrd);
  hestOptAdd(&hopt, "m", "mask vol", airTypeOther, 1, 1, &(gparm->nmask), "",
	     "Scalar volume (if any) for masking region in which glyphs are "
	     "drawn, in conjunction with \"mtr\" flag. ", NULL, NULL,
	     nrrdHestNrrd);
  hestOptAdd(&hopt, "mtr", "mask thresh", airTypeFloat, 1, 1,
	     &(gparm->maskThresh),
	     "0.5", "Glyphs will be drawn only for tensors with mask "
	     "value greater than this threshold");

  /* how glyphs will be shaped */
  hestOptAdd(&hopt, "g", "glyph shape", airTypeEnum, 1, 1,
	     &(gparm->glyphType), "box",
	     "shape of glyph to use for display.  Possibilities "
	     "include \"box\", \"sphere\", \"cylinder\", and "
	     "\"superquad\"", NULL, tenGlyphType);
  hestOptAdd(&hopt, "sh", "sharpness", airTypeFloat, 1, 1,
	     &(gparm->sqdSharp), "3.0",
	     "for superquadric glyphs, how much to sharp edges form as a "
	     "function of differences between eigenvalues.  Higher values "
	     "mean that edges form more easily");
  hestOptAdd(&hopt, "gsc", "scale", airTypeFloat, 1, 1, &(gparm->glyphScale),
	     "0.01", "over-all glyph size in world-space");

  /* how glyphs will be colored */
  hestOptAdd(&hopt, "v", "evector #", airTypeInt, 1, 1, &(gparm->colEvec), "0",
	     "which eigenvector should determine coloring. "
	     "\"0\", \"1\", \"2\" are principal, medium, and minor");
  hestOptAdd(&hopt, "sat", "saturation", airTypeFloat, 1, 1,
	     &(gparm->colMaxSat), "1.0",
	     "maximal saturation to use on glyph colors (use 0.0 to "
	     "create a black and white image)");
  hestOptAdd(&hopt, "ga", "aniso", airTypeEnum, 1, 1,
	     &(gparm->colAnisoType), "fa",
	     "Which anisotropy metric to use for modulating the "
	     "saturation of the glyph color", NULL, tenAniso);
  hestOptAdd(&hopt, "am", "aniso mod", airTypeFloat, 1, 1,
	     &(gparm->colAnisoModulate),
	     "0.0", "How much to modulate glyph color saturation by "
	     "anisotropy (as chosen by \"-ga\").  "
	     "If 1.0, then glyphs for zero anisotropy "
	     "data points will have no hue. ");
  hestOptAdd(&hopt, "gg", "gray", airTypeFloat, 1, 1, &(gparm->colIsoGray),
	     "1.0", "desaturating glyph color due to low anisotropy "
	     "tends towards this gray level");
  hestOptAdd(&hopt, "gam", "gamma", airTypeFloat, 1, 1, &(gparm->colGamma),
	     "0.7", "gamma to use on color components (after saturation)");
  hestOptAdd(&hopt, "emap", "env map", airTypeOther, 1, 1, &emap, "",
	     "environment map to use for shading glyphs.  By default, "
	     "there is no shading", NULL, NULL, nrrdHestNrrd);
  hestOptAdd(&hopt, "bg", "background", airTypeFloat, 3, 3, bg, "1 1 1",
	     "background RGB color; each component in range [0.0,1.0]");

  /* parameters for showing a dataset slice */
  hestOptAdd(&hopt, "slc", "axis pos", airTypeInt, 2, 2, slice, "-1 -1",
	     "For showing a gray-scale slice of anisotropy: the axis "
	     "and position along which to slice.  Use \"-1 -1\" to signify "
	     "that no slice should be shown");
  hestOptAdd(&hopt, "si", "slice image", airTypeOther, 1, 1, &nslc, "",
	     "Instead of showing a slice of the anisotropy used to cull "
	     "glyphs, show something else. ", NULL, NULL,
	     nrrdHestNrrd);
  hestOptAdd(&hopt, "off", "slice offset", airTypeFloat, 1, 1, 
	     &(gparm->sliceOffset), "0.0",
	     "Offset from slice position to render slice at (so that it "
	     "doesn't occlude glyphs).");
  hestOptAdd(&hopt, "sg", "slice gamma", airTypeFloat, 1, 1, 
	     &(gparm->sliceGamma), "1.7",
	     "Gamma to apply to values on slice.");
  hestOptAdd(&hopt, "sb", "slice bias", airTypeFloat, 1, 1, 
	     &(gparm->sliceBias), "0.05",
	     "amount by which to bump up slice gray values prior to gamma.");

  /* camera */
  hestOptAdd(&hopt, "fr", "from point", airTypeDouble, 3, 3, cam->from, NULL,
	     "position of camera, used to determine view vector");
  hestOptAdd(&hopt, "at", "at point", airTypeDouble, 3, 3, cam->at, "0 0 0",
	     "camera look-at point, used to determine view vector");
  hestOptAdd(&hopt, "up", "up vector", airTypeDouble, 3, 3, cam->up, "0 0 1",
	     "camera pseudo-up vector, used to determine view coordinates");
  hestOptAdd(&hopt, "rh", NULL, airTypeInt, 0, 0, &(cam->rightHanded), NULL,
	     "use a right-handed UVN frame (V points down)");
  hestOptAdd(&hopt, "or", NULL, airTypeInt, 0, 0, &(cam->orthographic), NULL,
	     "use orthogonal projection");
  hestOptAdd(&hopt, "ur", "uMin uMax", airTypeDouble, 2, 2, cam->uRange,
	     "-1 1", "range in U direction of image plane");
  hestOptAdd(&hopt, "vr", "vMin vMax", airTypeDouble, 2, 2, cam->vRange,
	     "-1 1", "range in V direction of image plane");

  /* postscript-specific options */
  hestOptAdd(&hopt, "gr", "glyph res", airTypeInt, 1, 1, &(gparm->facetRes),
	     "10", "(* postscript only *) "
	     "resolution of polygonalization of glyphs (all glyphs "
	     "other than the default box)");
  hestOptAdd(&hopt, "wd", "3 widths", airTypeFloat, 3, 3, gparm->edgeWidth,
	     "0.8 0.4 0.0",  "(* postscript only *) "
	     "width of edges drawn for three kinds of glyph "
	     "edges: silohuette, crease, non-crease");
  hestOptAdd(&hopt, "psc", "scale", airTypeFloat, 1, 1, &(win->scale), "300",
	      "(* postscript only *) "
	     "scaling from screen space units to postscript units "
	     "(in points)");
  hestOptAdd(&hopt, "nobg", NULL, airTypeInt, 0, 0, &nobg, NULL,
	      "(* postscript only *) "
	     "don't initially fill with background color");

  /* ray-traced-specific options */
  hestOptAdd(&hopt, "is", "nx ny", airTypeInt, 2, 2, ires, "256 256",
	     "(* ray-traced only *) "
	     "image size (resolution) to render");
  hestOptAdd(&hopt, "ns", "# samp", airTypeInt, 1, 1, &(eparm->numSamples),"4",
	     "(* ray-traced only *) "
	     "number of samples per pixel (must be a square number)");
  if (airThreadCapable) {
    hestOptAdd(&hopt, "nt", "# threads", airTypeInt, 1, 1,
	       &(eparm->numThreads), "1", 
	       "(* ray-traced only *) "
	       "number of threads to be used for rendering");
  }

  /* input/output */
  hestOptAdd(&hopt, "i", "nin", airTypeOther, 1, 1, &nten, "-",
	     "input diffusion tensor volume", NULL, NULL, nrrdHestNrrd);
  hestOptAdd(&hopt, "o", "nout", airTypeString, 1, 1, &outS, "-",
	     "output file");

  airMopAdd(mop, hopt, (airMopper)hestOptFree, airMopAlways);
  USAGE(_tend_glyphInfoL);
  PARSE();
  airMopAdd(mop, hopt, (airMopper)hestParseFree, airMopAlways);

  /* set up slicing stuff */
  if (!( -1 == slice[0] && -1 == slice[1] )) {
    gparm->doSlice = AIR_TRUE;
    gparm->sliceAxis = slice[0];
    gparm->slicePos = slice[1];
    gparm->sliceAnisoType = gparm->anisoType;
    /* gparm->sliceOffset set by hest */
  }

  if (npos) {
    fprintf(stderr, "%s: hack: turning off onlyPositive\n", me);
    gparm->onlyPositive = AIR_FALSE;
  }
  if (tenGlyphGen(doRT ? NULL : glyph, 
		  doRT ? scene : NULL,
		  gparm,
		  nten, npos, nslc)) {
    airMopAdd(mop, err = biffGetDone(TEN), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble generating glyphs:\n%s\n", me, err);
    airMopError(mop); return 1;
  }
  if (doRT) {
    nraw = nrrdNew();
    airMopAdd(mop, nraw, (airMopper)nrrdNuke, airMopAlways);
    gstate = echoGlobalStateNew();
    airMopAdd(mop, gstate, (airMopper)echoGlobalStateNix, airMopAlways);
    cam->neer = -2;
    cam->dist = 0;
    cam->faar = 2;
    cam->atRelative = AIR_TRUE;
    eparm->imgResU = ires[0];
    eparm->imgResV = ires[1];
    eparm->jitterType = (eparm->numSamples > 1
			 ? echoJitterJitter
			 : echoJitterNone);
    eparm->aperture = 0;
    eparm->renderBoxes = AIR_FALSE;
    eparm->seedRand = AIR_FALSE;
    ELL_3V_COPY(scene->bkgr, bg);
    scene->envmap = emap;
    if (echoRTRender(nraw, cam, scene, eparm, gstate)) {
      airMopAdd(mop, err = biffGetDone(ECHO), airFree, airMopAlways);
      fprintf(stderr, "%s: %s\n", me, err);
      airMopError(mop);
      return 1;
    }
    if (nrrdSave(outS, nraw, NULL)) {
      airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
      fprintf(stderr, "%s: %s\n", me, err);
      airMopError(mop);
      return 1;
    }
  } else {
    if (!(win->file = airFopen(outS, stdout, "wb"))) {
      fprintf(stderr, "%s: couldn't fopen(\"%s\",\"wb\"): %s\n", 
	      me, outS, strerror(errno));
      airMopError(mop); return 1;
    }
    airMopAdd(mop, win->file, (airMopper)airFclose, airMopAlways);
    cam->neer = -0.000000001;
    cam->dist = 0;
    cam->faar = 0.0000000001;
    cam->atRelative = AIR_TRUE;
    win->ps.lineWidth[limnEdgeTypeBackFacet] = 0;
    win->ps.lineWidth[limnEdgeTypeBackCrease] = 0;
    win->ps.lineWidth[limnEdgeTypeContour] = gparm->edgeWidth[0];
    win->ps.lineWidth[limnEdgeTypeFrontCrease] = gparm->edgeWidth[1];
    win->ps.lineWidth[limnEdgeTypeFrontFacet] = gparm->edgeWidth[2];
    win->ps.creaseAngle = 70;
    win->ps.noBackground = nobg;
    ELL_3V_COPY(win->ps.bg, bg);
    if (limnObjectRender(glyph, cam, win)
	|| limnObjectPSDraw(glyph, cam, emap, win)) {
      airMopAdd(mop, err = biffGetDone(LIMN), airFree, airMopAlways);
      fprintf(stderr, "%s: trouble drawing glyphs:\n%s\n", me, err);
      airMopError(mop); return 1;
    }
  }

  airMopOkay(mop);
  return 0;
}
/* TEND_CMD(glyph, INFO); */
unrrduCmd tend_glyphCmd = { "glyph", INFO, tend_glyphMain };
