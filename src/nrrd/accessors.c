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


#include "nrrd.h"
#include "limits.h"

/*
** general info on these arrays of functions:
** 
** nrrdXLoadY: dereference given pointer (of type *) and cast to type X
** nrrdXStoreY: cast given value (type X) and store at given address (type Y)
** nrrdXLookupY: find particular array (type Y) value and cast to type X
** nrrdXInsertY: cast given value (of type X) to array type (Y) and store
*/

/*
******** nrrdILoad
** 
** Dereference pointer v, cast that value to a integer, return it.
*/
int nrrdILoadC(char *v)                         {return(*v);}
int nrrdILoadUC(unsigned char *v)               {return(*v);}
int nrrdILoadS(short *v)                        {return(*v);}
int nrrdILoadUS(unsigned short *v)              {return(*v);}
int nrrdILoadI(int *v)                          {return(*v);}
int nrrdILoadUI(unsigned int *v)                {return(*v);}
int nrrdILoadLLI(long long int *v)              {return(*v);}
int nrrdILoadULLI(unsigned long long int *v)    {return(*v);}
int nrrdILoadF(float *v)                        {return(*v);}
int nrrdILoadD(double *v)                       {return(*v);}
int nrrdILoadLD(long double *v)                 {return(*v);}
int (*nrrdILoad[13])(void *) = {
  NULL,
  (int (*)(void*))nrrdILoadC,
  (int (*)(void*))nrrdILoadUC,
  (int (*)(void*))nrrdILoadS,
  (int (*)(void*))nrrdILoadUS,
  (int (*)(void*))nrrdILoadI,
  (int (*)(void*))nrrdILoadUI,
  (int (*)(void*))nrrdILoadLLI,
  (int (*)(void*))nrrdILoadULLI,
  (int (*)(void*))nrrdILoadF,
  (int (*)(void*))nrrdILoadD,
  (int (*)(void*))nrrdILoadLD,
  NULL};

/*
******** nrrdFLoad
** 
** Dereference pointer v, cast that value to a float, return it.
*/
float nrrdFLoadC(char *v)                       {return(*v);}
float nrrdFLoadUC(unsigned char *v)             {return(*v);}
float nrrdFLoadS(short *v)                      {return(*v);}
float nrrdFLoadUS(unsigned short *v)            {return(*v);}
float nrrdFLoadI(int *v)                        {return(*v);}
float nrrdFLoadUI(unsigned int *v)              {return(*v);}
float nrrdFLoadLLI(long long int *v)            {return(*v);}
float nrrdFLoadULLI(unsigned long long int *v)  {return(*v);}
float nrrdFLoadF(float *v)                      {return(*v);}
float nrrdFLoadD(double *v)                     {return(*v);}
float nrrdFLoadLD(long double *v)               {return(*v);}
float (*nrrdFLoad[13])(void *) = {
  NULL,
  (float (*)(void*))nrrdFLoadC,
  (float (*)(void*))nrrdFLoadUC,
  (float (*)(void*))nrrdFLoadS,
  (float (*)(void*))nrrdFLoadUS,
  (float (*)(void*))nrrdFLoadI,
  (float (*)(void*))nrrdFLoadUI,
  (float (*)(void*))nrrdFLoadLLI,
  (float (*)(void*))nrrdFLoadULLI,
  (float (*)(void*))nrrdFLoadF,
  (float (*)(void*))nrrdFLoadD,
  (float (*)(void*))nrrdFLoadLD,
  NULL};

/*
******** nrrdDLoad
**
** Dereference pointer v, cast that value to a double, return it.
*/
double nrrdDLoadC(char *v)                      {return(*v);}
double nrrdDLoadUC(unsigned char *v)            {return(*v);}
double nrrdDLoadS(short *v)                     {return(*v);}
double nrrdDLoadUS(unsigned short *v)           {return(*v);}
double nrrdDLoadI(int *v)                       {return(*v);}
double nrrdDLoadUI(unsigned int *v)             {return(*v);}
double nrrdDLoadLLI(long long int *v)           {return(*v);}
double nrrdDLoadULLI(unsigned long long int *v) {return(*v);}
double nrrdDLoadF(float *v)                     {return(*v);}
double nrrdDLoadD(double *v)                    {return(*v);}
double nrrdDLoadLD(long double *v)              {return(*v);}
double (*nrrdDLoad[13])(void *) = {
  NULL,
  (double (*)(void*))nrrdDLoadC,
  (double (*)(void*))nrrdDLoadUC,
  (double (*)(void*))nrrdDLoadS,
  (double (*)(void*))nrrdDLoadUS,
  (double (*)(void*))nrrdDLoadI,
  (double (*)(void*))nrrdDLoadUI,
  (double (*)(void*))nrrdDLoadLLI,
  (double (*)(void*))nrrdDLoadULLI,
  (double (*)(void*))nrrdDLoadF,
  (double (*)(void*))nrrdDLoadD,
  (double (*)(void*))nrrdDLoadLD,
  NULL};

/* 
******** nrrdIStore
**
** Cast given integer j to correct type, and store it at pointer address v.
** Returns the result of the assignment.
*/
int nrrdIStoreC(char *v, int j)                      {return(*v = j);}
int nrrdIStoreUC(unsigned char *v, int j)            {return(*v = j);}
int nrrdIStoreS(short *v, int j)                     {return(*v = j);}
int nrrdIStoreUS(unsigned short *v, int j)           {return(*v = j);}
int nrrdIStoreI(int *v, int j)                       {return(*v = j);}
int nrrdIStoreUI(unsigned int *v, int j)             {return(*v = j);}
int nrrdIStoreLLI(long long int *v, int j)           {return(*v = j);}
int nrrdIStoreULLI(unsigned long long int *v, int j) {return(*v = j);}
int nrrdIStoreF(float *v, int j)                     {return(*v = j);}
int nrrdIStoreD(double *v, int j)                    {return(*v = j);}
int nrrdIStoreLD(long double *v, int j)              {return(*v = j);}
int (*nrrdIStore[13])(void *, int) = {
  NULL,
  (int (*)(void*, int))nrrdIStoreC,
  (int (*)(void*, int))nrrdIStoreUC,
  (int (*)(void*, int))nrrdIStoreS,
  (int (*)(void*, int))nrrdIStoreUS,
  (int (*)(void*, int))nrrdIStoreI,
  (int (*)(void*, int))nrrdIStoreUI,
  (int (*)(void*, int))nrrdIStoreLLI,
  (int (*)(void*, int))nrrdIStoreULLI,
  (int (*)(void*, int))nrrdIStoreF,
  (int (*)(void*, int))nrrdIStoreD,
  (int (*)(void*, int))nrrdIStoreLD,
  NULL};

/*
******** nrrdFStore
**
** Cast given float f to correct type, and store it at pointer address v.
** Returns the result of the assignment.
*/
float nrrdFStoreC(char *v, float f)                      {return(*v = f);}
float nrrdFStoreUC(unsigned char *v, float f)            {return(*v = f);}
float nrrdFStoreS(short *v, float f)                     {return(*v = f);}
float nrrdFStoreUS(unsigned short *v, float f)           {return(*v = f);}
float nrrdFStoreI(int *v, float f)                       {return(*v = f);}
float nrrdFStoreUI(unsigned int *v, float f)             {return(*v = f);}
float nrrdFStoreLLI(long long int *v, float f)           {return(*v = f);}
float nrrdFStoreULLI(unsigned long long int *v, float f) {return(*v = f);}
float nrrdFStoreF(float *v, float f)                     {return(*v = f);}
float nrrdFStoreD(double *v, float f)                    {return(*v = f);}
float nrrdFStoreLD(long double *v, float f)              {return(*v = f);}
float (*nrrdFStore[13])(void *, float) = {
  NULL,
  (float (*)(void*, float))nrrdFStoreC,
  (float (*)(void*, float))nrrdFStoreUC,
  (float (*)(void*, float))nrrdFStoreS,
  (float (*)(void*, float))nrrdFStoreUS,
  (float (*)(void*, float))nrrdFStoreI,
  (float (*)(void*, float))nrrdFStoreUI,
  (float (*)(void*, float))nrrdFStoreLLI,
  (float (*)(void*, float))nrrdFStoreULLI,
  (float (*)(void*, float))nrrdFStoreF,
  (float (*)(void*, float))nrrdFStoreD,
  (float (*)(void*, float))nrrdFStoreLD,
  NULL};


/*
******** nrrdDStore
**
** Cast given double d to correct type, and store it at pointer address v.
** Returns the result of the assignment.
*/
double nrrdDStoreC(char *v, double d)                      {return(*v = d);}
double nrrdDStoreUC(unsigned char *v, double d)            {return(*v = d);}
double nrrdDStoreS(short *v, double d)                     {return(*v = d);}
double nrrdDStoreUS(unsigned short *v, double d)           {return(*v = d);}
double nrrdDStoreI(int *v, double d)                       {return(*v = d);}
double nrrdDStoreUI(unsigned int *v, double d)             {return(*v = d);}
double nrrdDStoreLLI(long long int *v, double d)           {return(*v = d);}
double nrrdDStoreULLI(unsigned long long int *v, double d) {return(*v = d);}
double nrrdDStoreF(float *v, double d)                     {return(*v = d);}
double nrrdDStoreD(double *v, double d)                    {return(*v = d);}
double nrrdDStoreLD(long double *v, double d)              {return(*v = d);}
double (*nrrdDStore[13])(void *, double) = {
  NULL,
  (double (*)(void*, double))nrrdDStoreC,
  (double (*)(void*, double))nrrdDStoreUC,
  (double (*)(void*, double))nrrdDStoreS,
  (double (*)(void*, double))nrrdDStoreUS,
  (double (*)(void*, double))nrrdDStoreI,
  (double (*)(void*, double))nrrdDStoreUI,
  (double (*)(void*, double))nrrdDStoreLLI,
  (double (*)(void*, double))nrrdDStoreULLI,
  (double (*)(void*, double))nrrdDStoreF,
  (double (*)(void*, double))nrrdDStoreD,
  (double (*)(void*, double))nrrdDStoreLD,
  NULL};

/*
******** nrrdILookup
**
** Casts v[I] to an integer and returns it.
*/
int nrrdILookupC(char *v, NRRD_BIG_INT I)                      {return(v[I]);}
int nrrdILookupUC(unsigned char *v, NRRD_BIG_INT I)            {return(v[I]);}
int nrrdILookupS(short *v, NRRD_BIG_INT I)                     {return(v[I]);}
int nrrdILookupUS(unsigned short *v, NRRD_BIG_INT I)           {return(v[I]);}
int nrrdILookupI(int *v, NRRD_BIG_INT I)                       {return(v[I]);}
int nrrdILookupUI(unsigned int *v, NRRD_BIG_INT I)             {return(v[I]);}
int nrrdILookupLLI(long long int *v, NRRD_BIG_INT I)           {return(v[I]);}
int nrrdILookupULLI(unsigned long long int *v, NRRD_BIG_INT I) {return(v[I]);}
int nrrdILookupF(float *v, NRRD_BIG_INT I)                     {return(v[I]);}
int nrrdILookupD(double *v, NRRD_BIG_INT I)                    {return(v[I]);}
int nrrdILookupLD(long double *v, NRRD_BIG_INT I)              {return(v[I]);}
int (*nrrdILookup[13])(void *, NRRD_BIG_INT) = {
  NULL,
  (int (*)(void*, NRRD_BIG_INT))nrrdILookupC,
  (int (*)(void*, NRRD_BIG_INT))nrrdILookupUC,
  (int (*)(void*, NRRD_BIG_INT))nrrdILookupS,
  (int (*)(void*, NRRD_BIG_INT))nrrdILookupUS,
  (int (*)(void*, NRRD_BIG_INT))nrrdILookupI,
  (int (*)(void*, NRRD_BIG_INT))nrrdILookupUI,
  (int (*)(void*, NRRD_BIG_INT))nrrdILookupLLI,
  (int (*)(void*, NRRD_BIG_INT))nrrdILookupULLI,
  (int (*)(void*, NRRD_BIG_INT))nrrdILookupF,
  (int (*)(void*, NRRD_BIG_INT))nrrdILookupD,
  (int (*)(void*, NRRD_BIG_INT))nrrdILookupLD,
  NULL};

/*
******** nrrdFLookup
**
** Casts v[I] to a float and returns it.
*/
float nrrdFLookupC(char *v, NRRD_BIG_INT I)                   {return(v[I]);}
float nrrdFLookupUC(unsigned char *v, NRRD_BIG_INT I)         {return(v[I]);}
float nrrdFLookupS(short *v, NRRD_BIG_INT I)                  {return(v[I]);}
float nrrdFLookupUS(unsigned short *v, NRRD_BIG_INT I)        {return(v[I]);}
float nrrdFLookupI(int *v, NRRD_BIG_INT I)                    {return(v[I]);}
float nrrdFLookupUI(unsigned int *v, NRRD_BIG_INT I)          {return(v[I]);}
float nrrdFLookupLLI(long long int *v, NRRD_BIG_INT I)        {return(v[I]);}
float nrrdFLookupULLI(unsigned long long int *v, NRRD_BIG_INT I) {
  return(v[I]);}
float nrrdFLookupF(float *v, NRRD_BIG_INT I)                  {return(v[I]);}
float nrrdFLookupD(double *v, NRRD_BIG_INT I)                 {return(v[I]);}
float nrrdFLookupLD(long double *v, NRRD_BIG_INT I)           {return(v[I]);}
float (*nrrdFLookup[13])(void *, NRRD_BIG_INT) = {
  NULL,
  (float (*)(void*, NRRD_BIG_INT))nrrdFLookupC,
  (float (*)(void*, NRRD_BIG_INT))nrrdFLookupUC,
  (float (*)(void*, NRRD_BIG_INT))nrrdFLookupS,
  (float (*)(void*, NRRD_BIG_INT))nrrdFLookupUS,
  (float (*)(void*, NRRD_BIG_INT))nrrdFLookupI,
  (float (*)(void*, NRRD_BIG_INT))nrrdFLookupUI,
  (float (*)(void*, NRRD_BIG_INT))nrrdFLookupLLI,
  (float (*)(void*, NRRD_BIG_INT))nrrdFLookupULLI,
  (float (*)(void*, NRRD_BIG_INT))nrrdFLookupF,
  (float (*)(void*, NRRD_BIG_INT))nrrdFLookupD,
  (float (*)(void*, NRRD_BIG_INT))nrrdFLookupLD,
  NULL};


/*
******** nrrdDLookup
**
** Casts v[I] to a double and returns it.
*/
double nrrdDLookupC(char *v, NRRD_BIG_INT I)                   {return(v[I]);}
double nrrdDLookupUC(unsigned char *v, NRRD_BIG_INT I)         {return(v[I]);}
double nrrdDLookupS(short *v, NRRD_BIG_INT I)                  {return(v[I]);}
double nrrdDLookupUS(unsigned short *v, NRRD_BIG_INT I)        {return(v[I]);}
double nrrdDLookupI(int *v, NRRD_BIG_INT I)                    {return(v[I]);}
double nrrdDLookupUI(unsigned int *v, NRRD_BIG_INT I)          {return(v[I]);}
double nrrdDLookupLLI(long long int *v, NRRD_BIG_INT I)        {return(v[I]);}
double nrrdDLookupULLI(unsigned long long int *v, NRRD_BIG_INT I) {
  return(v[I]);}
double nrrdDLookupF(float *v, NRRD_BIG_INT I)                  {return(v[I]);}
double nrrdDLookupD(double *v, NRRD_BIG_INT I)                 {return(v[I]);}
double nrrdDLookupLD(long double *v, NRRD_BIG_INT I)           {return(v[I]);}
double (*nrrdDLookup[13])(void *, NRRD_BIG_INT) = {
  NULL,
  (double (*)(void*, NRRD_BIG_INT))nrrdDLookupC,
  (double (*)(void*, NRRD_BIG_INT))nrrdDLookupUC,
  (double (*)(void*, NRRD_BIG_INT))nrrdDLookupS,
  (double (*)(void*, NRRD_BIG_INT))nrrdDLookupUS,
  (double (*)(void*, NRRD_BIG_INT))nrrdDLookupI,
  (double (*)(void*, NRRD_BIG_INT))nrrdDLookupUI,
  (double (*)(void*, NRRD_BIG_INT))nrrdDLookupLLI,
  (double (*)(void*, NRRD_BIG_INT))nrrdDLookupULLI,
  (double (*)(void*, NRRD_BIG_INT))nrrdDLookupF,
  (double (*)(void*, NRRD_BIG_INT))nrrdDLookupD,
  (double (*)(void*, NRRD_BIG_INT))nrrdDLookupLD,
  NULL};

/*
******** nrrdIInsert
**
** Stores given integer j at v[I], and returns j.
*/
int nrrdIInsertC(char *v, NRRD_BIG_INT I, int j) {
  return(v[I] = j);}
int nrrdIInsertUC(unsigned char *v, NRRD_BIG_INT I, int j) {
  return(v[I] = j);}
int nrrdIInsertS(short *v, NRRD_BIG_INT I, int j) {
  return(v[I] = j);}
int nrrdIInsertUS(unsigned short *v, NRRD_BIG_INT I, int j) {
  return(v[I] = j);}
int nrrdIInsertI(int *v, NRRD_BIG_INT I, int j) {
  return(v[I] = j);}
int nrrdIInsertUI(unsigned int *v, NRRD_BIG_INT I, int j) {
  return(v[I] = j);}
int nrrdIInsertLLI(long long int *v, NRRD_BIG_INT I, int j) {
  return(v[I] = j);}
int nrrdIInsertULLI(unsigned long long *v, NRRD_BIG_INT I, int j) {
  return(v[I] = j);}
int nrrdIInsertF(float *v, NRRD_BIG_INT I, int j) {
  return(v[I] = j);}
int nrrdIInsertD(double *v, NRRD_BIG_INT I, int j) {
  return(v[I] = j);}
int nrrdIInsertLD(long double *v, NRRD_BIG_INT I, int j) {
  return(v[I] = j);}
int (*nrrdIInsert[13])(void *, NRRD_BIG_INT, int) = {
  NULL,
  (int (*)(void*, NRRD_BIG_INT, int))nrrdIInsertC,
  (int (*)(void*, NRRD_BIG_INT, int))nrrdIInsertUC,
  (int (*)(void*, NRRD_BIG_INT, int))nrrdIInsertS,
  (int (*)(void*, NRRD_BIG_INT, int))nrrdIInsertUS,
  (int (*)(void*, NRRD_BIG_INT, int))nrrdIInsertI,
  (int (*)(void*, NRRD_BIG_INT, int))nrrdIInsertUI,
  (int (*)(void*, NRRD_BIG_INT, int))nrrdIInsertLLI,
  (int (*)(void*, NRRD_BIG_INT, int))nrrdIInsertULLI,
  (int (*)(void*, NRRD_BIG_INT, int))nrrdIInsertF,
  (int (*)(void*, NRRD_BIG_INT, int))nrrdIInsertD,
  (int (*)(void*, NRRD_BIG_INT, int))nrrdIInsertLD,
  NULL};

/*
******** nrrdFInsert
**
** Stores given float f at v[I], and returns f.
*/
float nrrdFInsertC(char *v, NRRD_BIG_INT I, float f) {
  return(v[I]=f);}
float nrrdFInsertUC(unsigned char *v, NRRD_BIG_INT I, float f) {
  return(v[I]=f);}
float nrrdFInsertS(short *v, NRRD_BIG_INT I, float f) {
  return(v[I]=f);}
float nrrdFInsertUS(unsigned short *v, NRRD_BIG_INT I, float f) {
  return(v[I]=f);}
float nrrdFInsertI(int *v, NRRD_BIG_INT I, float f) {
  return(v[I]=f);}
float nrrdFInsertUI(unsigned int *v, NRRD_BIG_INT I, float f) {
  return(v[I]=f);}
float nrrdFInsertLLI(long long int *v, NRRD_BIG_INT I, float f) {
  return(v[I]=f);}
float nrrdFInsertULLI(unsigned long long *v, NRRD_BIG_INT I, float f) {
  return(v[I]=f);}
float nrrdFInsertF(float *v, NRRD_BIG_INT I, float f) {
  return(v[I]=f);}
float nrrdFInsertD(double *v, NRRD_BIG_INT I, float f) {
  return(v[I]=f);}
float nrrdFInsertLD(long double *v, NRRD_BIG_INT I, float f) {
  return(v[I]=f);}
float (*nrrdFInsert[13])(void *, NRRD_BIG_INT, float) = {
  NULL,
  (float (*)(void*, NRRD_BIG_INT, float))nrrdFInsertC,
  (float (*)(void*, NRRD_BIG_INT, float))nrrdFInsertUC,
  (float (*)(void*, NRRD_BIG_INT, float))nrrdFInsertS,
  (float (*)(void*, NRRD_BIG_INT, float))nrrdFInsertUS,
  (float (*)(void*, NRRD_BIG_INT, float))nrrdFInsertI,
  (float (*)(void*, NRRD_BIG_INT, float))nrrdFInsertUI,
  (float (*)(void*, NRRD_BIG_INT, float))nrrdFInsertLLI,
  (float (*)(void*, NRRD_BIG_INT, float))nrrdFInsertULLI,
  (float (*)(void*, NRRD_BIG_INT, float))nrrdFInsertF,
  (float (*)(void*, NRRD_BIG_INT, float))nrrdFInsertD,
  (float (*)(void*, NRRD_BIG_INT, float))nrrdFInsertLD,
  NULL};

/*
******** nrrdDInsert
**
** Stores given double d at v[I], and returns d.
*/
double nrrdDInsertC(char *v, NRRD_BIG_INT I, double d) {
  return(v[I]=d);}
double nrrdDInsertUC(unsigned char *v, NRRD_BIG_INT I, double d) {
  return(v[I]=d);}
double nrrdDInsertS(short *v, NRRD_BIG_INT I, double d) {
  return(v[I]=d);}
double nrrdDInsertUS(unsigned short *v, NRRD_BIG_INT I, double d) {
  return(v[I]=d);}
double nrrdDInsertI(int *v, NRRD_BIG_INT I, double d) {
  return(v[I]=d);}
double nrrdDInsertUI(unsigned int *v, NRRD_BIG_INT I, double d) {
  return(v[I]=d);}
double nrrdDInsertLLI(long long int *v, NRRD_BIG_INT I, double d) {
  return(v[I]=d);}
double nrrdDInsertULLI(unsigned long long *v, NRRD_BIG_INT I, double d) {
  return(v[I]=d);}
double nrrdDInsertF(float *v, NRRD_BIG_INT I, double d) {
  return(v[I]=d);}
double nrrdDInsertD(double *v, NRRD_BIG_INT I, double d) {
  return(v[I]=d);}
double nrrdDInsertLD(long double *v, NRRD_BIG_INT I, double d) {
  return(v[I]=d);}
double (*nrrdDInsert[13])(void *, NRRD_BIG_INT, double) = {
  NULL,
  (double (*)(void*, NRRD_BIG_INT, double))nrrdDInsertC,
  (double (*)(void*, NRRD_BIG_INT, double))nrrdDInsertUC,
  (double (*)(void*, NRRD_BIG_INT, double))nrrdDInsertS,
  (double (*)(void*, NRRD_BIG_INT, double))nrrdDInsertUS,
  (double (*)(void*, NRRD_BIG_INT, double))nrrdDInsertI,
  (double (*)(void*, NRRD_BIG_INT, double))nrrdDInsertUI,
  (double (*)(void*, NRRD_BIG_INT, double))nrrdDInsertLLI,
  (double (*)(void*, NRRD_BIG_INT, double))nrrdDInsertULLI,
  (double (*)(void*, NRRD_BIG_INT, double))nrrdDInsertF,
  (double (*)(void*, NRRD_BIG_INT, double))nrrdDInsertD,
  (double (*)(void*, NRRD_BIG_INT, double))nrrdDInsertLD,
  NULL};

/*
******** nrrdSprint
**
** Dereferences pointer v and sprintf()s that value into given string s,
** returns the result of sprintf()
*/
int nrrdSprintC(char *s, char *v) {
  return(sprintf(s, "%d", *v)); }
int nrrdSprintUC(char *s, unsigned char *v) {
  return(sprintf(s, "%u", *v)); }
int nrrdSprintS(char *s, short *v) {
  return(sprintf(s, "%d", *v)); }
int nrrdSprintUS(char *s, unsigned short *v) {
  return(sprintf(s, "%u", *v)); }
int nrrdSprintI(char *s, int *v) {
  return(sprintf(s, "%d", *v)); }
int nrrdSprintUI(char *s, unsigned int *v) {
  return(sprintf(s, "%u", *v)); }
int nrrdSprintLLI(char *s, long long int *v) {
  return(sprintf(s, "%lld", *v)); }
int nrrdSprintULLI(char *s, unsigned long long *v) {
  return(sprintf(s, "%llu", *v)); }
int nrrdSprintF(char *s, float *v) {
  return(sprintf(s, "%f", *v)); }
int nrrdSprintD(char *s, double *v) {
  return(sprintf(s, "%f", *v)); }
int nrrdSprintLD(char *s, long double *v) {
  return(sprintf(s, "%Lf", *v)); }
int (*nrrdSprint[13])(char *, void *) = {
  NULL,
  (int (*)(char *, void *))nrrdSprintC,
  (int (*)(char *, void *))nrrdSprintUC,
  (int (*)(char *, void *))nrrdSprintS,
  (int (*)(char *, void *))nrrdSprintUS,
  (int (*)(char *, void *))nrrdSprintI,
  (int (*)(char *, void *))nrrdSprintUI,
  (int (*)(char *, void *))nrrdSprintLLI,
  (int (*)(char *, void *))nrrdSprintULLI,
  (int (*)(char *, void *))nrrdSprintF,
  (int (*)(char *, void *))nrrdSprintD,
  (int (*)(char *, void *))nrrdSprintLD,
  NULL};

/*
******** nrrdFprint
**
** Dereferences pointer v and fprintf()s that value into given file f;
** returns the result of fprintf()
*/
int nrrdFprintC(FILE *f, char *v) {
  return(fprintf(f, "%d", *v)); }
int nrrdFprintUC(FILE *f, unsigned char *v) {
  return(fprintf(f, "%u", *v)); }
int nrrdFprintS(FILE *f, short *v) {
  return(fprintf(f, "%d", *v)); }
int nrrdFprintUS(FILE *f, unsigned short *v) {
  return(fprintf(f, "%u", *v)); }
int nrrdFprintI(FILE *f, int *v) {
  return(fprintf(f, "%d", *v)); }
int nrrdFprintUI(FILE *f, unsigned int *v) {
  return(fprintf(f, "%u", *v)); }
int nrrdFprintLLI(FILE *f, long long int *v) {
  return(fprintf(f, "%lld", *v)); }
int nrrdFprintULLI(FILE *f, unsigned long long *v) {
  return(fprintf(f, "%llu", *v)); }
int nrrdFprintF(FILE *f, float *v) {
  return(fprintf(f, "%f", *v)); }
int nrrdFprintD(FILE *f, double *v) {
  return(fprintf(f, "%f", *v)); }
int nrrdFprintLD(FILE *f, long double *v) {
  return(fprintf(f, "%Lf", *v)); }
int (*nrrdFprint[13])(FILE *, void *) = {
  NULL,
  (int (*)(FILE *, void *))nrrdFprintC,
  (int (*)(FILE *, void *))nrrdFprintUC,
  (int (*)(FILE *, void *))nrrdFprintS,
  (int (*)(FILE *, void *))nrrdFprintUS,
  (int (*)(FILE *, void *))nrrdFprintI,
  (int (*)(FILE *, void *))nrrdFprintUI,
  (int (*)(FILE *, void *))nrrdFprintLLI,
  (int (*)(FILE *, void *))nrrdFprintULLI,
  (int (*)(FILE *, void *))nrrdFprintF,
  (int (*)(FILE *, void *))nrrdFprintD,
  (int (*)(FILE *, void *))nrrdFprintLD,
  NULL};

/*
******** nrrdFClamp
**
** clamps a given float to the range to the range representable
** by the given fixed-point type; for floating point types we
** just return the given number.
*/
float nrrdFClampC(float v) {
  if (v < SCHAR_MIN) return SCHAR_MIN;
  if (v > SCHAR_MAX) return SCHAR_MAX;
  return v;
}
float nrrdFClampUC(float v) {
  if (v < 0) return 0;
  if (v > UCHAR_MAX) return UCHAR_MAX;
  return v;
}
float nrrdFClampS(float v) {
  if (v < SHRT_MIN) return SHRT_MIN;
  if (v > SHRT_MAX) return SHRT_MAX;
  return v;
}
float nrrdFClampUS(float v) {
  if (v < 0) return 0;
  if (v > USHRT_MAX) return USHRT_MAX;
  return v;
}
float nrrdFClampI(float v) {
  if (v < INT_MIN) return INT_MIN;
  if (v > INT_MAX) return INT_MAX;
  return v;
}
float nrrdFClampUI(float v) {
  if (v < 0) return 0;
  if (v > UINT_MAX) return UINT_MAX;
  return v;
}
float nrrdFClampLLI(float v) {
  if (v < NRRD_LLONG_MIN) return NRRD_LLONG_MIN;
  if (v > NRRD_LLONG_MAX) return NRRD_LLONG_MAX;
  return v;
}
float nrrdFClampULLI(float v) {
  if (v < 0) return 0;
  if (v > NRRD_ULLONG_MAX) return NRRD_ULLONG_MAX;
  return v;
}
float nrrdFClampF(float v) { return v; }
float nrrdFClampD(float v) { return v; }
float nrrdFClampLD(float v) { return v; }
float (*nrrdFClamp[13])(float) = {
  NULL,
  nrrdFClampC,
  nrrdFClampUC,
  nrrdFClampS,
  nrrdFClampUS,
  nrrdFClampI,
  nrrdFClampUI,
  nrrdFClampLLI,
  nrrdFClampULLI,
  nrrdFClampF,
  nrrdFClampD,
  nrrdFClampLD,
  NULL};

/*
******** nrrdDClamp
**
** same as nrrdDClamp, but for doubles
*/
double nrrdDClampC(double v) {
  if (v < SCHAR_MIN) return SCHAR_MIN;
  if (v > SCHAR_MAX) return SCHAR_MAX;
  return v;
}
double nrrdDClampUC(double v) {
  if (v < 0) return 0;
  if (v > UCHAR_MAX) return UCHAR_MAX;
  return v;
}
double nrrdDClampS(double v) {
  if (v < SHRT_MIN) return SHRT_MIN;
  if (v > SHRT_MAX) return SHRT_MAX;
  return v;
}
double nrrdDClampUS(double v) {
  if (v < 0) return 0;
  if (v > USHRT_MAX) return USHRT_MAX;
  return v;
}
double nrrdDClampI(double v) {
  if (v < INT_MIN) return INT_MIN;
  if (v > INT_MAX) return INT_MAX;
  return v;
}
double nrrdDClampUI(double v) {
  if (v < 0) return 0;
  if (v > UINT_MAX) return UINT_MAX;
  return v;
}
double nrrdDClampLLI(double v) {
  if (v < NRRD_LLONG_MIN) return NRRD_LLONG_MIN;
  if (v > NRRD_LLONG_MAX) return NRRD_LLONG_MAX;
  return v;
}
double nrrdDClampULLI(double v) {
  if (v < 0) return 0;
  if (v > NRRD_ULLONG_MAX) return NRRD_ULLONG_MAX;
  return v;
}
double nrrdDClampF(double v) { return v; }
double nrrdDClampD(double v) { return v; }
double nrrdDClampLD(double v) { return v; }
double (*nrrdDClamp[13])(double) = {
  NULL,
  nrrdDClampC,
  nrrdDClampUC,
  nrrdDClampS,
  nrrdDClampUS,
  nrrdDClampI,
  nrrdDClampUI,
  nrrdDClampLLI,
  nrrdDClampULLI,
  nrrdDClampF,
  nrrdDClampD,
  nrrdDClampLD,
  NULL};
