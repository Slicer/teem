/*
  The contents of this file are subject to the University of Utah Public
  License (the "License"); you may not use this file except in
  compliance with the License.
  
  Software distributed under the License is distributed on an "AS IS"
  basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See
  the License for the specific language governing rights and limitations
  under the License.

  The Original Source Code is "teem", released March 23, 2001.
  
  The Original Source Code was developed by the University of Utah.
  Portions created by UNIVERSITY are Copyright (C) 2001, 1998 University
  of Utah. All Rights Reserved.
*/




/* subset.c */
extern int nrrdILoadC(char *v);
extern int nrrdILoadUC(unsigned char *v);
extern int nrrdILoadS(short *v);
extern int nrrdILoadUS(unsigned short *v);
extern int nrrdILoadI(int *v);
extern int nrrdILoadUI(unsigned int *v);
extern int nrrdILoadLLI(long long int *v);
extern int nrrdILoadULLI(unsigned long long int *v);
extern int nrrdILoadF(float *v);
extern int nrrdILoadD(double *v);
extern int nrrdILoadLD(long double *v);

extern float nrrdFLoadC(char *v);
extern float nrrdFLoadUC(unsigned char *v);
extern float nrrdFLoadS(short *v);
extern float nrrdFLoadUS(unsigned short *v);
extern float nrrdFLoadI(int *v);
extern float nrrdFLoadUI(unsigned int *v);
extern float nrrdFLoadLLI(long long int *v);
extern float nrrdFLoadULLI(unsigned long long int *v);
extern float nrrdFLoadF(float *v);
extern float nrrdFLoadD(double *v);
extern float nrrdFLoadLD(long double *v);

extern double nrrdDLoadC(char *v);
extern double nrrdDLoadUC(unsigned char *v);
extern double nrrdDLoadS(short *v);
extern double nrrdDLoadUS(unsigned short *v);
extern double nrrdDLoadI(int *v);
extern double nrrdDLoadUI(unsigned int *v);
extern double nrrdDLoadLLI(long long int *v);
extern double nrrdDLoadULLI(unsigned long long int *v);
extern double nrrdDLoadF(float *v);
extern double nrrdDLoadD(double *v);
extern double nrrdDLoadLD(long double *v);

extern int nrrdIStoreC(char *v, int i);
extern int nrrdIStoreUC(unsigned char *v, int i);
extern int nrrdIStoreS(short *v, int i);
extern int nrrdIStoreUS(unsigned short *v, int i);
extern int nrrdIStoreI(int *v, int i);
extern int nrrdIStoreUI(unsigned int *v, int i);
extern int nrrdIStoreLLI(long long int *v, int i);
extern int nrrdIStoreULLI(unsigned long long int *v, int i);
extern int nrrdIStoreF(float *v, int i);
extern int nrrdIStoreD(double *v, int i);
extern int nrrdIStoreLD(long double *v, int i);

extern float nrrdFStoreC(char *v, float f);
extern float nrrdFStoreUC(unsigned char *v, float f);
extern float nrrdFStoreS(short *v, float f);
extern float nrrdFStoreUS(unsigned short *v, float f);
extern float nrrdFStoreI(int *v, float f);
extern float nrrdFStoreUI(unsigned int *v, float f);
extern float nrrdFStoreLLI(long long int *v, float f);
extern float nrrdFStoreULLI(unsigned long long int *v, float f);
extern float nrrdFStoreF(float *v, float f);
extern float nrrdFStoreD(double *v, float f);
extern float nrrdFStoreLD(long double *v, float f);

extern double nrrdDStoreC(char *v, double d);
extern double nrrdDStoreUC(unsigned char *v, double d);
extern double nrrdDStoreS(short *v, double d);
extern double nrrdDStoreUS(unsigned short *v, double d);
extern double nrrdDStoreI(int *v, double d);
extern double nrrdDStoreUI(unsigned int *v, double d);
extern double nrrdDStoreLLI(long long int *v, double d);
extern double nrrdDStoreULLI(unsigned long long int *v, double d);
extern double nrrdDStoreF(float *v, double d);
extern double nrrdDStoreD(double *v, double d);
extern double nrrdDStoreLD(long double *v, double d);

extern int nrrdSprintC(char *s, char *v);
extern int nrrdSprintUC(char *s, unsigned char *v);
extern int nrrdSprintS(char *s, short *v);
extern int nrrdSprintUS(char *s, unsigned short *v);
extern int nrrdSprintI(char *s, int *v);
extern int nrrdSprintUI(char *s, unsigned int *v);
extern int nrrdSprintLLI(char *s, long long int *v);
extern int nrrdSprintULLI(char *s, unsigned long long *v);
extern int nrrdSprintF(char *s, float *v);
extern int nrrdSprintD(char *s, double *v);
extern int nrrdSprintLD(char *s, long double *v);

extern float nrrdFClampC(float v);
extern float nrrdFClampUC(float v);
extern float nrrdFClampS(float v);
extern float nrrdFClampUS(float v);
extern float nrrdFClampI(float v);
extern float nrrdFClampUI(float v);
extern float nrrdFClampLLI(float v);
extern float nrrdFClampULLI(float v);
extern float nrrdFClampF(float v);
extern float nrrdFClampD(float v);
extern float nrrdFClampLD(float v);

extern double nrrdDClampC(double v);
extern double nrrdDClampUC(double v);
extern double nrrdDClampS(double v);
extern double nrrdDClampUS(double v);
extern double nrrdDClampI(double v);
extern double nrrdDClampUI(double v);
extern double nrrdDClampLLI(double v);
extern double nrrdDClampULLI(double v);
extern double nrrdDClampF(double v);
extern double nrrdDClampD(double v);
extern double nrrdDClampLD(double v);

