/*
  Teem: Gordon Kindlmann's research software
  Copyright (C) 2005  Gordon Kindlmann
  Copyright (C) 2004, 2003, 2002, 2001, 2000, 1999, 1998  University of Utah

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

#define INFO "Generate postscript renderings of 2D glyphs"
char *_tend_ellipseInfoL =
  (INFO
   ".  Not much to look at here.");

int
tend_ellipseDoit(FILE *file, Nrrd *nten, Nrrd *npos,
                 float min[2], float max[2],
                 float gscale, float cthresh, int invert) {
  int sx=0, sy=0, x, y, nt, ti;
  float aspect, minX, minY, maxX, maxY, px, py, 
    conf, Dxx, Dxy, Dyy, *tdata, *pdata;
  
  tdata = (float*)nten->data;
  if (npos) {
    pdata = (float*)npos->data;
    nt = npos->axis[1].size;
    aspect = (max[0] - min[0])/(max[1] - min[1]);
  } else {
    pdata = NULL;
    sx = nten->axis[1].size;
    sy = nten->axis[2].size;
    nt = sx*sy;
    aspect = (float)sx/sy;
  }
  if (aspect > 7.5/10) {
    /* image has a wider aspect ratio than safely printable page area */
    minX = 0.5;
    maxX = 8.0;
    minY = 5.50 - 7.5/2/aspect;
    maxY = 5.50 + 7.5/2/aspect;
  } else {
    /* image is taller ... */
    minX = 4.25 - 10.0/2*aspect;
    maxX = 4.25 + 10.0/2*aspect;
    minY = 0.5;
    maxY = 10.5;
  }
  minX *= 72; minY *= 72;
  maxX *= 72; maxY *= 72;

  fprintf(file, "%%!PS-Adobe-3.0 EPSF-3.0\n");
  fprintf(file, "%%%%Creator: tend ellipse\n");
  fprintf(file, "%%%%Title: blah blah blah\n");
  fprintf(file, "%%%%Pages: 1\n");
  fprintf(file, "%%%%BoundingBox: %d %d %d %d\n",
          (int)floor(minX), (int)floor(minY),
          (int)ceil(maxX), (int)ceil(maxY));
  fprintf(file, "%%%%HiResBoundingBox: %g %g %g %g\n", 
          minX, minY, maxX, maxY);
  fprintf(file, "%%%%EndComments\n");
  fprintf(file, "%%%%BeginProlog\n");
  fprintf(file, "%%%%EndProlog\n");
  fprintf(file, "%%%%Page: 1 1\n");
  fprintf(file, "gsave\n");
  fprintf(file, "0 setgray\n");
  if (invert) {
    fprintf(file, "%g %g moveto\n", minX, minY);
    fprintf(file, "%g %g lineto\n", maxX, minY);
    fprintf(file, "%g %g lineto\n", maxX, maxY);
    fprintf(file, "%g %g lineto\n", minX, maxY);
    fprintf(file, "closepath fill\n");
    fprintf(file, "1 setgray\n");
  }
  for (ti=0; ti<nt; ti++) {
    if (npos) {
      if (!AIR_EXISTS(pdata[0])) {
        pdata += 2;
        tdata += 4;
        continue;
      }
      px = AIR_AFFINE(min[0], pdata[0], max[0], minX, maxX);
      py = AIR_AFFINE(min[1], pdata[1], max[1], maxY, minY);
      pdata += 2;
    } else {
      x = ti % sx;
      y = ti / sx;
      px = NRRD_CELL_POS(minX, maxX, sx, x);
      py = NRRD_CELL_POS(minY, maxY, sy, sy-1-y);
    }
    conf = tdata[0];
    if (conf > cthresh) {
      Dxx = tdata[1];
      Dxy = tdata[2];
      Dyy = tdata[3];
      fprintf(file, "gsave\n");
      fprintf(file, "matrix currentmatrix\n");
      fprintf(file, "[%g %g %g %g %g %g] concat\n",
              Dxx, -Dxy, -Dxy, Dyy, px, py);
      fprintf(file, "0 0 %g 0 360 arc closepath\n", gscale);
      fprintf(file, "setmatrix\n");
      fprintf(file, "fill\n");
      fprintf(file, "grestore\n");
    }
    tdata += 4;
  }
  fprintf(file, "grestore\n");
  
  return 0;
}

int
tend_ellipseMain(int argc, char **argv, char *me, hestParm *hparm) {
  int pret;
  hestOpt *hopt = NULL;
  char *perr;
  airArray *mop;

  Nrrd *nten, *npos;
  char *outS;
  float gscale, cthresh, min[2], max[2];
  FILE *fout;
  int invert;

  mop = airMopNew();

  hestOptAdd(&hopt, "ctr", "conf thresh", airTypeFloat, 1, 1, &cthresh, "0.5",
             "Glyphs will be drawn only for tensors with confidence "
             "values greater than this threshold");
  hestOptAdd(&hopt, "gsc", "scale", airTypeFloat, 1, 1, &gscale, "1",
             "over-all glyph size");
  hestOptAdd(&hopt, "inv", NULL, airTypeInt, 0, 0, &invert, NULL,
             "use white ellipses on black background, instead of reverse");
  hestOptAdd(&hopt, "min", "minX minY", airTypeFloat, 2, 2, min, "-1 -1",
             "when using \"-p\", minimum corner");
  hestOptAdd(&hopt, "max", "maxX maxY", airTypeFloat, 2, 2, max, "1 1",
             "when using \"-p\", maximum corner");

  /* input/output */
  hestOptAdd(&hopt, "i", "nin", airTypeOther, 1, 1, &nten, "-",
             "image of 2D tensors", NULL, NULL, nrrdHestNrrd);
  hestOptAdd(&hopt, "p", "pos array", airTypeOther, 1, 1, &npos, "",
             "Instead of being on a grid, tensors are at arbitrary locations, "
             "as defined by this 2-by-N array of floats", NULL, NULL,
             nrrdHestNrrd);
  hestOptAdd(&hopt, "o", "nout", airTypeString, 1, 1, &outS, "-",
             "output PostScript file");

  airMopAdd(mop, hopt, (airMopper)hestOptFree, airMopAlways);
  USAGE(_tend_ellipseInfoL);
  PARSE();
  airMopAdd(mop, hopt, (airMopper)hestParseFree, airMopAlways);

  if (npos) {
    if (!( 2 == nten->dim && 4 == nten->axis[0].size
           && 2 == npos->dim && 2 == npos->axis[0].size
           && nten->axis[1].size == npos->axis[1].size )) {
      fprintf(stderr, "%s: didn't get matching lists of tensors and pos's\n",
              me);
      airMopError(mop); return 1;
    }
    if (!( nrrdTypeFloat == npos->type )) {
      fprintf(stderr, "%s: didn't get float type positions\n", me);
      airMopError(mop); return 1;
    }
  } else {
    if (!(3 == nten->dim && 4 == nten->axis[0].size)) {
      fprintf(stderr, "%s: didn't get a 3-D 4-by-X-by-Y 2D tensor array\n",
              me);
      airMopError(mop); return 1;
    }
  }
  if (!( nrrdTypeFloat == nten->type )) {
    fprintf(stderr, "%s: didn't get float type tensors\n", me);
    airMopError(mop); return 1;
  }
  if (!(fout = airFopen(outS, stdout, "wb"))) {
    fprintf(stderr, "%s: couldn't open \"%s\" for writing\n", me, outS);
    airMopError(mop); return 1;
  }
  airMopAdd(mop, fout, (airMopper)airFclose, airMopAlways);

  tend_ellipseDoit(fout, nten, npos, min, max, gscale, cthresh, invert);

  airMopOkay(mop);
  return 0;
}
/* TEND_CMD(glyph, INFO); */
unrrduCmd tend_ellipseCmd = { "ellipse", INFO, tend_ellipseMain };
