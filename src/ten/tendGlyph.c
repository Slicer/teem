/*
  teem: Gordon Kindlmann's research software
  Copyright (C) 2002, 2001, 2000, 1999, 1998 University of Utah

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
#include "tenPrivate.h"

#define INFO "Generate postscript renderings of box glyphs"
char *_tend_glyphInfoL =
  (INFO
   ".  Because this is doing viz/graphics, many parameters need to be set. "
   "Use a response file to simplify giving the command-line options which "
   "aren't changing between invocations. "
   "The output is an EPS file suitable for including as a figure in LaTeX, "
   "or viewing with ghostview, or distilling into PDF.");

int
tend_glyphMain(int argc, char **argv, char *me, hestParm *hparm) {
  int pret;
  hestOpt *hopt = NULL;
  char *perr, *err;
  airArray *mop;

  Nrrd *nten, *emap;
  char *outS;
  limnCam *cam;
  limnObj *glyph;
  limnWin *win;
  tenGlyphParm *gparm;
  float width[2];

  /* so that command-line options can be read from file */
  hparm->respFileEnable = AIR_TRUE;

  mop = airMopNew();
  airMopAdd(mop, hopt, (airMopper)hestOptFree, airMopAlways);
  cam = limnCamNew();
  airMopAdd(mop, cam, (airMopper)limnCamNix, airMopAlways);
  glyph = limnObjNew(512, AIR_TRUE);
  airMopAdd(mop, glyph, (airMopper)limnObjNix, airMopAlways);
  win = limnWinNew(limnDevicePS);
  airMopAdd(mop, win, (airMopper)limnWinNix, airMopAlways);
  gparm = tenGlyphParmNew();
  airMopAdd(mop, gparm, (airMopper)tenGlyphParmNix, airMopAlways);
  
  hestOptAdd(&hopt, "a", "aniso", airTypeEnum, 1, 1, &(gparm->anisoType),
	     "fa", "Which anisotropy metric to use for thresholding the data "
	     "points to be drawn", NULL, tenAniso);
  hestOptAdd(&hopt, "atr", "aniso thresh", airTypeFloat, 1, 1,
	     &(gparm->anisoThresh),
	     "0.5", "Glyphs will be drawn only for tensors with anisotropy "
	     "greater than this threshold");
  hestOptAdd(&hopt, "ctr", "conf thresh", airTypeFloat, 1, 1,
	     &(gparm->confThresh),
	     "0.5", "Glyphs will be drawn only for tensors with confidence "
	     "values greater than this threshold");
  hestOptAdd(&hopt, "m", "mask vol", airTypeOther, 1, 1, &(gparm->nmask), 
	     "", "Scalar volume for masking region in which glyphs are "
	     "drawn, in conjunction with \"mtr\" flag.  ", NULL, NULL,
	     nrrdHestNrrd);
  hestOptAdd(&hopt, "mtr", "mask thresh", airTypeFloat, 1, 1,
	     &(gparm->maskThresh),
	     "0.0", "Glyphs will be drawn only for tensors with mask "
	     "value greater than this threshold");
  hestOptAdd(&hopt, "wd", "silo, edge width", airTypeFloat, 2, 2, width,
	     "0.8 0.4", "width of edges drawn for glyph silohuette edges, "
	     "and for non-silohuette glyph edges");
  hestOptAdd(&hopt, "sat", "saturation", airTypeFloat, 1, 1, &(gparm->colSat),
	     "1.0", "saturation to use on glyph colors (use 0.0 for B+W)");
  hestOptAdd(&hopt, "gam", "gamma", airTypeFloat, 1, 1, &(gparm->colGamma),
	     "0.7", "gamma to use on color components (after saturation)");
  hestOptAdd(&hopt, "psc", "scale", airTypeFloat, 1, 1, &(win->scale), "100",
	     "scaling from unit in screen space to postscript points");
  hestOptAdd(&hopt, "gsc", "scale", airTypeFloat, 1, 1, &(gparm->glyphScale),
	     "0.01", "over-all glyph size in world-space");
  hestOptAdd(&hopt, "fr", "from point", airTypeDouble, 3, 3, cam->from,"4 4 4",
	     "position of camera, used to determine view vector");
  hestOptAdd(&hopt, "at", "at point", airTypeDouble, 3, 3, cam->at, "0 0 0",
	     "camera look-at point, used to determine view vector");
  hestOptAdd(&hopt, "up", "up vector", airTypeDouble, 3, 3, cam->up, "0 0 1",
	     "camera pseudo-up vector, used to determine view coordinates");
  hestOptAdd(&hopt, "rh", NULL, airTypeInt, 0, 0, &(cam->rightHanded), NULL,
	     "use a right-handed UVN frame (V points down)");
  hestOptAdd(&hopt, "or", NULL, airTypeInt, 0, 0, &(cam->ortho), NULL,
	     "use orthogonal projection");
  hestOptAdd(&hopt, "ur", "uMin uMax", airTypeDouble, 2, 2, cam->uRange,
	     "-1 1", "range in U direction of image plane");
  hestOptAdd(&hopt, "vr", "vMin vMax", airTypeDouble, 2, 2, cam->vRange,
	     "-1 1", "range in V direction of image plane");
  hestOptAdd(&hopt, "emap", "env map", airTypeOther, 1, 1, &emap,
	     "", "environment map to use for shading glyphs.  By default, "
	     "there is no shading", NULL, NULL, nrrdHestNrrd);
  hestOptAdd(&hopt, "i", "nin", airTypeOther, 1, 1, &nten, "-",
	     "input diffusion tensor volume", NULL, NULL, nrrdHestNrrd);
  hestOptAdd(&hopt, "o", "nout", airTypeString, 1, 1, &outS, NULL,
	     "output EPS file");

  USAGE(_tend_glyphInfoL);
  PARSE();
  airMopAdd(mop, hopt, (airMopper)hestParseFree, airMopAlways);

  gparm->siloWidth = width[0];
  gparm->edgeWidth = width[1];
  if (tenGlyphGen(glyph, nten, gparm)) {
    airMopAdd(mop, err = biffGetDone(TEN), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble generating glyphs:\n%s\n", me, err);
    airMopError(mop); return 1;
  }
  if (!(win->file = airFopen(outS, stdout, "w"))) {
    fprintf(stderr, "%s: couldn't fopen(\"%s\",\"rb\"): %s\n", 
	    me, outS, strerror(errno));
    airMopError(mop); return 1;
  }
  airMopAdd(mop, win->file, (airMopper)airFclose, airMopAlways);
  cam->neer = -0.000000001;
  cam->dist = 0;
  cam->faar = 0.0000000001;
  cam->atRel = AIR_TRUE;
  win->ps.edgeWidth[0] = 0;
  win->ps.edgeWidth[1] = 0;
  win->ps.edgeWidth[2] = gparm->siloWidth;
  win->ps.edgeWidth[3] = gparm->edgeWidth;
  win->ps.edgeWidth[4] = gparm->edgeWidth;
  if (limnObjRender(glyph, cam, win)
      || limnObjPSDraw(glyph, cam, emap, win)) {
    airMopAdd(mop, err = biffGetDone(LIMN), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble drawing glyphs:\n%s\n", me, err);
    airMopError(mop); return 1;
  }

  airMopOkay(mop);
  return 0;
}
/* TEND_CMD(glyph, INFO); */
unrrduCmd tend_glyphCmd = { "glyph", INFO, tend_glyphMain };
