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

#ifndef GAGE_PRIVATE_HAS_BEEN_INCLUDED
#define GAGE_PRIVATE_HAS_BEEN_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#define GT gage_t

#if GAGE_TYPE_FLOAT
#  define ell3vPRINT ell3vPrint_f
#  define ell3mPRINT ell3mPrint_f
#  define ell3vPERP ell3vPerp_f
#  define nrrdLOOKUP nrrdFLookup
#  define EVALN evalN_f               /* NrrdKernel method */
#else
#  define ell3vPRINT ell3vPrint_d
#  define ell3mPRINT ell3mPrint_d
#  define ell3vPERP ell3vPerp_d
#  define nrrdLOOKUP nrrdDLookup
#  define EVALN evalN_d               /* NrrdKernel method */
#endif

#define PADSIZE(sx, sy, sz, ctx) \
  ((sx) = (ctx)->shape.sx + 2*((ctx)->havePad), \
   (sy) = (ctx)->shape.sy + 2*((ctx)->havePad), \
   (sz) = (ctx)->shape.sz + 2*((ctx)->havePad))
#define ANSWER(pvl, m) \
  ((pvl)->ans + (pvl)->kind->ansOffset[(m)])

/* pvl.c */
extern gagePerVolume *_gagePerVolumeCopy(gagePerVolume *pvl, int fd);

/* print.c */
extern void _gagePrint_off(FILE *, gageContext *ctx);
extern void _gagePrint_fslw(FILE *, gageContext *ctx);

/* filter.c */
extern int _gageLocationSet(gageContext *ctx, gage_t x, gage_t y, gage_t z);

/* sclprint.c */
extern void _gageSclPrint_query(FILE *, unsigned int query);
extern void _gageSclIv3Print(FILE *, gageContext *ctx, gagePerVolume *pvl);

/* sclfilter.c */
extern void _gageScl3PFilter2(GT *iv3, GT *iv2, GT *iv1,
			      GT *fw00, GT *fw11, GT *fw22,
			      GT *val, GT *gvec, GT *hess,
			      int doV, int doD1, int doD2);
extern void _gageScl3PFilter4(GT *iv3, GT *iv2, GT *iv1,
			      GT *fw00, GT *fw11, GT *fw22,
			      GT *val, GT *gvec, GT *hess,
			      int doV, int doD1, int doD2);
extern void _gageScl3PFilterN(int fd,
			      GT *iv3, GT *iv2, GT *iv1,
			      GT *fw00, GT *fw11, GT *fw22,
			      GT *val, GT *gvec, GT *hess,
			      int doV, int doD1, int doD2);
extern void _gageSclFilter(gageContext *ctx, gagePerVolume *pvl);

/* sclanswer.c */
extern void _gageSclAnswer(gageContext *ctx, gagePerVolume *pvl);

/* vecprint.c */
extern void _gageVecPrint_query(FILE *, unsigned int query);
extern void _gageVecIv3Print(FILE *, gageContext *ctx, gagePerVolume *pvl);

/* misc.c */
extern Nrrd* _gageStandardPadder(Nrrd *nin, gageKind *kind,
				 int padding, gagePerVolume *pvl);
extern void _gageStandardNixer(Nrrd *npad, gageKind *kind,
			       gagePerVolume *pvl);

#ifdef __cplusplus
}
#endif

#endif /* GAGE_PRIVATE_HAS_BEEN_INCLUDED */
