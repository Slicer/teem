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

/* arrays.c */
extern int _gageSclNeedDeriv[GAGE_SCL_MAX+1];
extern unsigned int _gageSclPrereq[GAGE_SCL_MAX+1];

/* methods.c */
extern void _gageContextInit(gageContext *ctx);
extern void _gageContextDone(gageContext *ctx);
extern void _gageResetKernels(gageContext *ctx);
extern int _gageKernelSet(gageContext *ctx, 
			 int which, NrrdKernel *k, double *kparm);
extern int _gageKernelDependentSet(gageContext *ctx);
extern int _gageVolumeSet(gageContext *ctx, int pad, Nrrd *npad, int baseDim);
extern int _gageVolumeDependentSet(gageContext *ctx, Nrrd *npad, int baseDim);
extern int _gageUpdate(gageContext *ctx, int needK[GAGE_KERNEL_NUM]);

/* sclmethods.c */
extern int _gageSclKernelDependentSet(gageSclContext *sctx);
extern int _gageSclQueryDependentSet(gageSclContext *sctx);
extern int _gageSclVolumeDependentSet(gageSclContext *sctx);

/* general.c */
extern void _gageFslSet(gageContext *ctx);
extern void _gageFwValueRenormalize(gageContext *ctx, int wch);
extern void _gageFwDerivRenormalize(gageContext *ctx, int wch);
extern void _gageFwSet(gageContext *ctx);
extern int _gageLocationSet(gageContext *ctx, int *newBidxP,
			    gage_t x, gage_t y, gage_t z);

/* print.c */
extern void _gageSclPrint_query(unsigned int query);
extern void _gageSclPrint_iv3(gageSclContext *ctx);
extern void _gageSclPrint_fslw(gageSclContext *ctx, int doD1, int doD2);

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
  
#ifdef __cplusplus
}
#endif
