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

#define GT GAGE_TYPE
#define GT_FLOAT GAGE_TYPE_FLOAT

/* arrays.c */
extern int _gageSclNeedDeriv[GAGE_SCL_MAX+1];
extern unsigned int _gageSclPrereq[GAGE_SCL_MAX+1];

/* methods.c */
extern void _gageSclSetQueryDependent(gageSclContext *ctx);
extern void _gageSclResetKernelDependent(gageSclContext *ctx);
extern int _gageSclSetKernelDependent(gageSclContext *ctx);
extern int _gageSclSetVolumeDependent(gageSclContext *ctx);

/* print.c */
extern void _gageSclPrint_query(unsigned int query);
extern void _gageSclPrint_iv3(gageSclContext *ctx);
extern void _gageSclPrint_fslw(gageSclContext *ctx, int doD1, int doD2);

/* sclfilt.c */
extern void _gageScl3PFilter2(GT *iv3, GT *iv2, GT *iv1,
			      GT *fw00, GT *fw11, GT *fw22,
			      GT *val, GT *gvec, GT *hess,
			      int doD1, int doD2);
extern void _gageScl3PFilter4(GT *iv3, GT *iv2, GT *iv1,
			      GT *fw00, GT *fw11, GT *fw22,
			      GT *val, GT *gvec, GT *hess,
			      int doD1, int doD2);
extern void _gageScl3PFilterN(int fd,
			      GT *iv3, GT *iv2, GT *iv1,
			      GT *fw00, GT *fw11, GT *fw22,
			      GT *val, GT *gvec, GT *hess,
			      int doD1, int doD2);
