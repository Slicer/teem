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

#ifdef __cplusplus
extern "C" {
#endif

#define GT gage_t
#define GT_FLOAT GAGE_TYPE_FLOAT

#if GT_FLOAT
#define ell3vPRINT ell3vPrint_f
#define ell3mPRINT ell3mPrint_f
#define ell3vPERP ell3vPerp_f
#define nrrdLOOKUP nrrdFLookup
#else
#define ell3vPRINT ell3vPrint_d
#define ell3mPRINT ell3mPrint_d
#define ell3vPERP ell3vPerp_d
#define nrrdLOOKUP nrrdDLookup
#endif

#define RESET(p) p = airFree(p)

/* methods.c */
extern gageSclAnswer *_gageSclAnswerNew();
extern gageSclAnswer *_gageSclAnswerNix(gageSclAnswer *san);
extern gageVecAnswer *_gageVecAnswerNew();
extern gageVecAnswer *_gageVecAnswerNix(gageVecAnswer *van);

/* arrays.c */
extern unsigned int _gageSclPrereq[GAGE_SCL_MAX+1];
extern unsigned int _gageVecPrereq[GAGE_VEC_MAX+1];
extern int _gageSclNeedDeriv[GAGE_SCL_MAX+1];
extern int _gageVecNeedDeriv[GAGE_VEC_MAX+1];

/* enums.c */
extern airEnum _gageScl;
extern airEnum _gageVec;

/* print.c */
extern void _gagePrint_off(FILE *, gageContext *ctx);
extern void _gagePrint_fslw(FILE *, gageContext *ctx);

/* filter.c */
extern int _gageLocationSet(gageContext *ctx, int *newBidxP,
			    gage_t x, gage_t y, gage_t z);

/* sclprint.c */
extern void _gageSclPrint_query(FILE *, unsigned int query);
extern void _gageSclIv3Print(FILE *, gageContext *ctx, gagePerVolume *pvl);

/* vecprint.c */
extern void _gageVecPrint_query(FILE *, unsigned int query);
extern void _gageVecIv3Print(FILE *, gageContext *ctx, gagePerVolume *pvl);

/* scl.c */
extern void _gageSclFilter(gageContext *ctx, gagePerVolume *pvl);
extern void _gageSclAnswer(gageContext *ctx, gagePerVolume *pvl);
extern void _gageSclIv3Fill(gageContext *ctx, gagePerVolume *pvl, void *here);

/* vec.c */
extern void _gageVecFilter(gageContext *ctx, gagePerVolume *pvl);
extern void _gageVecAnswer(gageContext *ctx, gagePerVolume *pvl);
extern void _gageVecIv3Fill(gageContext *ctx, gagePerVolume *pvl, void *here);

/* sclfilt.c */
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

/* general.c */
extern int _gageKernelDependentSet(gageContext *ctx);
extern int _gageVolumeDependentSet(gageContext *ctx, Nrrd *npad,
				   gageKind *kind);
  
#ifdef __cplusplus
}
#endif
