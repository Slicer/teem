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
** Dereference pointer v, cast that value to a integer, return it.  */
int _nrrdILoadC(char *v)                         {return(*v);}
int _nrrdILoadUC(unsigned char *v)               {return(*v);}
int _nrrdILoadS(short *v)                        {return(*v);}
int _nrrdILoadUS(unsigned short *v)              {return(*v);}
int _nrrdILoadI(int *v)                          {return(*v);}
int _nrrdILoadUI(unsigned int *v)                {return(*v);}
int _nrrdILoadLLI(long long int *v)              {return(*v);}
int _nrrdILoadULLI(unsigned long long int *v)    {return(*v);}
int _nrrdILoadF(float *v)                        {return(*v);}
int _nrrdILoadD(double *v)                       {return(*v);}
int _nrrdILoadLD(long double *v)                 {return(*v);}
int (*nrrdILoad[NRRD_MAX_TYPE+1])(void *) = {
  NULL,
  (int (*)(void*))_nrrdILoadC,
  (int (*)(void*))_nrrdILoadUC,
  (int (*)(void*))_nrrdILoadS,
  (int (*)(void*))_nrrdILoadUS,
  (int (*)(void*))_nrrdILoadI,
  (int (*)(void*))_nrrdILoadUI,
  (int (*)(void*))_nrrdILoadLLI,
  (int (*)(void*))_nrrdILoadULLI,
  (int (*)(void*))_nrrdILoadF,
  (int (*)(void*))_nrrdILoadD,
  (int (*)(void*))_nrrdILoadLD,
  NULL};

/*
******** nrrdFLoad
** 
** Dereference pointer v, cast that value to a float, return it.
*/
float _nrrdFLoadC(char *v)                       {return(*v);}
float _nrrdFLoadUC(unsigned char *v)             {return(*v);}
float _nrrdFLoadS(short *v)                      {return(*v);}
float _nrrdFLoadUS(unsigned short *v)            {return(*v);}
float _nrrdFLoadI(int *v)                        {return(*v);}
float _nrrdFLoadUI(unsigned int *v)              {return(*v);}
float _nrrdFLoadLLI(long long int *v)            {return(*v);}
float _nrrdFLoadULLI(unsigned long long int *v)  {return(*v);}
float _nrrdFLoadF(float *v)                      {return(*v);}
float _nrrdFLoadD(double *v)                     {return(*v);}
float _nrrdFLoadLD(long double *v)               {return(*v);}
float (*nrrdFLoad[NRRD_MAX_TYPE+1])(void *) = {
  NULL,
  (float (*)(void*))_nrrdFLoadC,
  (float (*)(void*))_nrrdFLoadUC,
  (float (*)(void*))_nrrdFLoadS,
  (float (*)(void*))_nrrdFLoadUS,
  (float (*)(void*))_nrrdFLoadI,
  (float (*)(void*))_nrrdFLoadUI,
  (float (*)(void*))_nrrdFLoadLLI,
  (float (*)(void*))_nrrdFLoadULLI,
  (float (*)(void*))_nrrdFLoadF,
  (float (*)(void*))_nrrdFLoadD,
  (float (*)(void*))_nrrdFLoadLD,
  NULL};

/*
******** nrrdDLoad
**
** Dereference pointer v, cast that value to a double, return it.
*/
double _nrrdDLoadC(char *v)                      {return(*v);}
double _nrrdDLoadUC(unsigned char *v)            {return(*v);}
double _nrrdDLoadS(short *v)                     {return(*v);}
double _nrrdDLoadUS(unsigned short *v)           {return(*v);}
double _nrrdDLoadI(int *v)                       {return(*v);}
double _nrrdDLoadUI(unsigned int *v)             {return(*v);}
double _nrrdDLoadLLI(long long int *v)           {return(*v);}
double _nrrdDLoadULLI(unsigned long long int *v) {return(*v);}
double _nrrdDLoadF(float *v)                     {return(*v);}
double _nrrdDLoadD(double *v)                    {return(*v);}
double _nrrdDLoadLD(long double *v)              {return(*v);}
double (*nrrdDLoad[NRRD_MAX_TYPE+1])(void *) = {
  NULL,
  (double (*)(void*))_nrrdDLoadC,
  (double (*)(void*))_nrrdDLoadUC,
  (double (*)(void*))_nrrdDLoadS,
  (double (*)(void*))_nrrdDLoadUS,
  (double (*)(void*))_nrrdDLoadI,
  (double (*)(void*))_nrrdDLoadUI,
  (double (*)(void*))_nrrdDLoadLLI,
  (double (*)(void*))_nrrdDLoadULLI,
  (double (*)(void*))_nrrdDLoadF,
  (double (*)(void*))_nrrdDLoadD,
  (double (*)(void*))_nrrdDLoadLD,
  NULL};

/* 
******** nrrdIStore
**
** Cast given integer j to correct type, and store it at pointer address v.
** Returns the result of the assignment.
*/
int _nrrdIStoreC(char *v, int j)                      {return(*v = j);}
int _nrrdIStoreUC(unsigned char *v, int j)            {return(*v = j);}
int _nrrdIStoreS(short *v, int j)                     {return(*v = j);}
int _nrrdIStoreUS(unsigned short *v, int j)           {return(*v = j);}
int _nrrdIStoreI(int *v, int j)                       {return(*v = j);}
int _nrrdIStoreUI(unsigned int *v, int j)             {return(*v = j);}
int _nrrdIStoreLLI(long long int *v, int j)           {return(*v = j);}
int _nrrdIStoreULLI(unsigned long long int *v, int j) {return(*v = j);}
int _nrrdIStoreF(float *v, int j)                     {return(*v = j);}
int _nrrdIStoreD(double *v, int j)                    {return(*v = j);}
int _nrrdIStoreLD(long double *v, int j)              {return(*v = j);}
int (*nrrdIStore[NRRD_MAX_TYPE+1])(void *, int) = {
  NULL,
  (int (*)(void*, int))_nrrdIStoreC,
  (int (*)(void*, int))_nrrdIStoreUC,
  (int (*)(void*, int))_nrrdIStoreS,
  (int (*)(void*, int))_nrrdIStoreUS,
  (int (*)(void*, int))_nrrdIStoreI,
  (int (*)(void*, int))_nrrdIStoreUI,
  (int (*)(void*, int))_nrrdIStoreLLI,
  (int (*)(void*, int))_nrrdIStoreULLI,
  (int (*)(void*, int))_nrrdIStoreF,
  (int (*)(void*, int))_nrrdIStoreD,
  (int (*)(void*, int))_nrrdIStoreLD,
  NULL};

/*
******** nrrdFStore
**
** Cast given float f to correct type, and store it at pointer address v.
** Returns the result of the assignment.
*/
float _nrrdFStoreC(char *v, float f)                      {return(*v = f);}
float _nrrdFStoreUC(unsigned char *v, float f)            {return(*v = f);}
float _nrrdFStoreS(short *v, float f)                     {return(*v = f);}
float _nrrdFStoreUS(unsigned short *v, float f)           {return(*v = f);}
float _nrrdFStoreI(int *v, float f)                       {return(*v = f);}
float _nrrdFStoreUI(unsigned int *v, float f)             {return(*v = f);}
float _nrrdFStoreLLI(long long int *v, float f)           {return(*v = f);}
float _nrrdFStoreULLI(unsigned long long int *v, float f) {return(*v = f);}
float _nrrdFStoreF(float *v, float f)                     {return(*v = f);}
float _nrrdFStoreD(double *v, float f)                    {return(*v = f);}
float _nrrdFStoreLD(long double *v, float f)              {return(*v = f);}
float (*nrrdFStore[NRRD_MAX_TYPE+1])(void *, float) = {
  NULL,
  (float (*)(void*, float))_nrrdFStoreC,
  (float (*)(void*, float))_nrrdFStoreUC,
  (float (*)(void*, float))_nrrdFStoreS,
  (float (*)(void*, float))_nrrdFStoreUS,
  (float (*)(void*, float))_nrrdFStoreI,
  (float (*)(void*, float))_nrrdFStoreUI,
  (float (*)(void*, float))_nrrdFStoreLLI,
  (float (*)(void*, float))_nrrdFStoreULLI,
  (float (*)(void*, float))_nrrdFStoreF,
  (float (*)(void*, float))_nrrdFStoreD,
  (float (*)(void*, float))_nrrdFStoreLD,
  NULL};


/*
******** nrrdDStore
**
** Cast given double d to correct type, and store it at pointer address v.
** Returns the result of the assignment.
*/
double _nrrdDStoreC(char *v, double d)                      {return(*v = d);}
double _nrrdDStoreUC(unsigned char *v, double d)            {return(*v = d);}
double _nrrdDStoreS(short *v, double d)                     {return(*v = d);}
double _nrrdDStoreUS(unsigned short *v, double d)           {return(*v = d);}
double _nrrdDStoreI(int *v, double d)                       {return(*v = d);}
double _nrrdDStoreUI(unsigned int *v, double d)             {return(*v = d);}
double _nrrdDStoreLLI(long long int *v, double d)           {return(*v = d);}
double _nrrdDStoreULLI(unsigned long long int *v, double d) {return(*v = d);}
double _nrrdDStoreF(float *v, double d)                     {return(*v = d);}
double _nrrdDStoreD(double *v, double d)                    {return(*v = d);}
double _nrrdDStoreLD(long double *v, double d)              {return(*v = d);}
double (*nrrdDStore[NRRD_MAX_TYPE+1])(void *, double) = {
  NULL,
  (double (*)(void*, double))_nrrdDStoreC,
  (double (*)(void*, double))_nrrdDStoreUC,
  (double (*)(void*, double))_nrrdDStoreS,
  (double (*)(void*, double))_nrrdDStoreUS,
  (double (*)(void*, double))_nrrdDStoreI,
  (double (*)(void*, double))_nrrdDStoreUI,
  (double (*)(void*, double))_nrrdDStoreLLI,
  (double (*)(void*, double))_nrrdDStoreULLI,
  (double (*)(void*, double))_nrrdDStoreF,
  (double (*)(void*, double))_nrrdDStoreD,
  (double (*)(void*, double))_nrrdDStoreLD,
  NULL};

/*
******** nrrdILookup
**
** Casts v[I] to an integer and returns it.
*/
int _nrrdILookupC(char *v, NRRD_BIG_INT I)                      {return(v[I]);}
int _nrrdILookupUC(unsigned char *v, NRRD_BIG_INT I)            {return(v[I]);}
int _nrrdILookupS(short *v, NRRD_BIG_INT I)                     {return(v[I]);}
int _nrrdILookupUS(unsigned short *v, NRRD_BIG_INT I)           {return(v[I]);}
int _nrrdILookupI(int *v, NRRD_BIG_INT I)                       {return(v[I]);}
int _nrrdILookupUI(unsigned int *v, NRRD_BIG_INT I)             {return(v[I]);}
int _nrrdILookupLLI(long long int *v, NRRD_BIG_INT I)           {return(v[I]);}
int _nrrdILookupULLI(unsigned long long int *v, NRRD_BIG_INT I) {return(v[I]);}
int _nrrdILookupF(float *v, NRRD_BIG_INT I)                     {return(v[I]);}
int _nrrdILookupD(double *v, NRRD_BIG_INT I)                    {return(v[I]);}
int _nrrdILookupLD(long double *v, NRRD_BIG_INT I)              {return(v[I]);}
int (*nrrdILookup[NRRD_MAX_TYPE+1])(void *, NRRD_BIG_INT) = {
  NULL,
  (int (*)(void*, NRRD_BIG_INT))_nrrdILookupC,
  (int (*)(void*, NRRD_BIG_INT))_nrrdILookupUC,
  (int (*)(void*, NRRD_BIG_INT))_nrrdILookupS,
  (int (*)(void*, NRRD_BIG_INT))_nrrdILookupUS,
  (int (*)(void*, NRRD_BIG_INT))_nrrdILookupI,
  (int (*)(void*, NRRD_BIG_INT))_nrrdILookupUI,
  (int (*)(void*, NRRD_BIG_INT))_nrrdILookupLLI,
  (int (*)(void*, NRRD_BIG_INT))_nrrdILookupULLI,
  (int (*)(void*, NRRD_BIG_INT))_nrrdILookupF,
  (int (*)(void*, NRRD_BIG_INT))_nrrdILookupD,
  (int (*)(void*, NRRD_BIG_INT))_nrrdILookupLD,
  NULL};

/*
******** nrrdFLookup
**
** Casts v[I] to a float and returns it.
*/
float _nrrdFLookupC(char *v, NRRD_BIG_INT I)                   {return(v[I]);}
float _nrrdFLookupUC(unsigned char *v, NRRD_BIG_INT I)         {return(v[I]);}
float _nrrdFLookupS(short *v, NRRD_BIG_INT I)                  {return(v[I]);}
float _nrrdFLookupUS(unsigned short *v, NRRD_BIG_INT I)        {return(v[I]);}
float _nrrdFLookupI(int *v, NRRD_BIG_INT I)                    {return(v[I]);}
float _nrrdFLookupUI(unsigned int *v, NRRD_BIG_INT I)          {return(v[I]);}
float _nrrdFLookupLLI(long long int *v, NRRD_BIG_INT I)        {return(v[I]);}
float _nrrdFLookupULLI(unsigned long long int *v, NRRD_BIG_INT I) {
  return(v[I]);}
float _nrrdFLookupF(float *v, NRRD_BIG_INT I)                  {return(v[I]);}
float _nrrdFLookupD(double *v, NRRD_BIG_INT I)                 {return(v[I]);}
float _nrrdFLookupLD(long double *v, NRRD_BIG_INT I)           {return(v[I]);}
float (*nrrdFLookup[NRRD_MAX_TYPE+1])(void *, NRRD_BIG_INT) = {
  NULL,
  (float (*)(void*, NRRD_BIG_INT))_nrrdFLookupC,
  (float (*)(void*, NRRD_BIG_INT))_nrrdFLookupUC,
  (float (*)(void*, NRRD_BIG_INT))_nrrdFLookupS,
  (float (*)(void*, NRRD_BIG_INT))_nrrdFLookupUS,
  (float (*)(void*, NRRD_BIG_INT))_nrrdFLookupI,
  (float (*)(void*, NRRD_BIG_INT))_nrrdFLookupUI,
  (float (*)(void*, NRRD_BIG_INT))_nrrdFLookupLLI,
  (float (*)(void*, NRRD_BIG_INT))_nrrdFLookupULLI,
  (float (*)(void*, NRRD_BIG_INT))_nrrdFLookupF,
  (float (*)(void*, NRRD_BIG_INT))_nrrdFLookupD,
  (float (*)(void*, NRRD_BIG_INT))_nrrdFLookupLD,
  NULL};


/*
******** nrrdDLookup
**
** Casts v[I] to a double and returns it.
*/
double _nrrdDLookupC(char *v, NRRD_BIG_INT I)                   {return(v[I]);}
double _nrrdDLookupUC(unsigned char *v, NRRD_BIG_INT I)         {return(v[I]);}
double _nrrdDLookupS(short *v, NRRD_BIG_INT I)                  {return(v[I]);}
double _nrrdDLookupUS(unsigned short *v, NRRD_BIG_INT I)        {return(v[I]);}
double _nrrdDLookupI(int *v, NRRD_BIG_INT I)                    {return(v[I]);}
double _nrrdDLookupUI(unsigned int *v, NRRD_BIG_INT I)          {return(v[I]);}
double _nrrdDLookupLLI(long long int *v, NRRD_BIG_INT I)        {return(v[I]);}
double _nrrdDLookupULLI(unsigned long long int *v, NRRD_BIG_INT I) {
  return(v[I]);}
double _nrrdDLookupF(float *v, NRRD_BIG_INT I)                  {return(v[I]);}
double _nrrdDLookupD(double *v, NRRD_BIG_INT I)                 {return(v[I]);}
double _nrrdDLookupLD(long double *v, NRRD_BIG_INT I)           {return(v[I]);}
double (*nrrdDLookup[NRRD_MAX_TYPE+1])(void *, NRRD_BIG_INT) = {
  NULL,
  (double (*)(void*, NRRD_BIG_INT))_nrrdDLookupC,
  (double (*)(void*, NRRD_BIG_INT))_nrrdDLookupUC,
  (double (*)(void*, NRRD_BIG_INT))_nrrdDLookupS,
  (double (*)(void*, NRRD_BIG_INT))_nrrdDLookupUS,
  (double (*)(void*, NRRD_BIG_INT))_nrrdDLookupI,
  (double (*)(void*, NRRD_BIG_INT))_nrrdDLookupUI,
  (double (*)(void*, NRRD_BIG_INT))_nrrdDLookupLLI,
  (double (*)(void*, NRRD_BIG_INT))_nrrdDLookupULLI,
  (double (*)(void*, NRRD_BIG_INT))_nrrdDLookupF,
  (double (*)(void*, NRRD_BIG_INT))_nrrdDLookupD,
  (double (*)(void*, NRRD_BIG_INT))_nrrdDLookupLD,
  NULL};

/*
******** nrrdIInsert
**
** Stores given integer j at v[I], and returns j.
*/
int _nrrdIInsertC(char *v, NRRD_BIG_INT I, int j) {
  return(v[I] = j);}
int _nrrdIInsertUC(unsigned char *v, NRRD_BIG_INT I, int j) {
  return(v[I] = j);}
int _nrrdIInsertS(short *v, NRRD_BIG_INT I, int j) {
  return(v[I] = j);}
int _nrrdIInsertUS(unsigned short *v, NRRD_BIG_INT I, int j) {
  return(v[I] = j);}
int _nrrdIInsertI(int *v, NRRD_BIG_INT I, int j) {
  return(v[I] = j);}
int _nrrdIInsertUI(unsigned int *v, NRRD_BIG_INT I, int j) {
  return(v[I] = j);}
int _nrrdIInsertLLI(long long int *v, NRRD_BIG_INT I, int j) {
  return(v[I] = j);}
int _nrrdIInsertULLI(unsigned long long *v, NRRD_BIG_INT I, int j) {
  return(v[I] = j);}
int _nrrdIInsertF(float *v, NRRD_BIG_INT I, int j) {
  return(v[I] = j);}
int _nrrdIInsertD(double *v, NRRD_BIG_INT I, int j) {
  return(v[I] = j);}
int _nrrdIInsertLD(long double *v, NRRD_BIG_INT I, int j) {
  return(v[I] = j);}
int (*nrrdIInsert[NRRD_MAX_TYPE+1])(void *, NRRD_BIG_INT, int) = {
  NULL,
  (int (*)(void*, NRRD_BIG_INT, int))_nrrdIInsertC,
  (int (*)(void*, NRRD_BIG_INT, int))_nrrdIInsertUC,
  (int (*)(void*, NRRD_BIG_INT, int))_nrrdIInsertS,
  (int (*)(void*, NRRD_BIG_INT, int))_nrrdIInsertUS,
  (int (*)(void*, NRRD_BIG_INT, int))_nrrdIInsertI,
  (int (*)(void*, NRRD_BIG_INT, int))_nrrdIInsertUI,
  (int (*)(void*, NRRD_BIG_INT, int))_nrrdIInsertLLI,
  (int (*)(void*, NRRD_BIG_INT, int))_nrrdIInsertULLI,
  (int (*)(void*, NRRD_BIG_INT, int))_nrrdIInsertF,
  (int (*)(void*, NRRD_BIG_INT, int))_nrrdIInsertD,
  (int (*)(void*, NRRD_BIG_INT, int))_nrrdIInsertLD,
  NULL};

/*
******** nrrdFInsert
**
** Stores given float f at v[I], and returns f.
*/
float _nrrdFInsertC(char *v, NRRD_BIG_INT I, float f) {
  return(v[I]=f);}
float _nrrdFInsertUC(unsigned char *v, NRRD_BIG_INT I, float f) {
  return(v[I]=f);}
float _nrrdFInsertS(short *v, NRRD_BIG_INT I, float f) {
  return(v[I]=f);}
float _nrrdFInsertUS(unsigned short *v, NRRD_BIG_INT I, float f) {
  return(v[I]=f);}
float _nrrdFInsertI(int *v, NRRD_BIG_INT I, float f) {
  return(v[I]=f);}
float _nrrdFInsertUI(unsigned int *v, NRRD_BIG_INT I, float f) {
  return(v[I]=f);}
float _nrrdFInsertLLI(long long int *v, NRRD_BIG_INT I, float f) {
  return(v[I]=f);}
float _nrrdFInsertULLI(unsigned long long *v, NRRD_BIG_INT I, float f) {
  return(v[I]=f);}
float _nrrdFInsertF(float *v, NRRD_BIG_INT I, float f) {
  return(v[I]=f);}
float _nrrdFInsertD(double *v, NRRD_BIG_INT I, float f) {
  return(v[I]=f);}
float _nrrdFInsertLD(long double *v, NRRD_BIG_INT I, float f) {
  return(v[I]=f);}
float (*nrrdFInsert[NRRD_MAX_TYPE+1])(void *, NRRD_BIG_INT, float) = {
  NULL,
  (float (*)(void*, NRRD_BIG_INT, float))_nrrdFInsertC,
  (float (*)(void*, NRRD_BIG_INT, float))_nrrdFInsertUC,
  (float (*)(void*, NRRD_BIG_INT, float))_nrrdFInsertS,
  (float (*)(void*, NRRD_BIG_INT, float))_nrrdFInsertUS,
  (float (*)(void*, NRRD_BIG_INT, float))_nrrdFInsertI,
  (float (*)(void*, NRRD_BIG_INT, float))_nrrdFInsertUI,
  (float (*)(void*, NRRD_BIG_INT, float))_nrrdFInsertLLI,
  (float (*)(void*, NRRD_BIG_INT, float))_nrrdFInsertULLI,
  (float (*)(void*, NRRD_BIG_INT, float))_nrrdFInsertF,
  (float (*)(void*, NRRD_BIG_INT, float))_nrrdFInsertD,
  (float (*)(void*, NRRD_BIG_INT, float))_nrrdFInsertLD,
  NULL};

/*
******** nrrdDInsert
**
** Stores given double d at v[I], and returns d.
*/
double _nrrdDInsertC(char *v, NRRD_BIG_INT I, double d) {
  return(v[I]=d);}
double _nrrdDInsertUC(unsigned char *v, NRRD_BIG_INT I, double d) {
  return(v[I]=d);}
double _nrrdDInsertS(short *v, NRRD_BIG_INT I, double d) {
  return(v[I]=d);}
double _nrrdDInsertUS(unsigned short *v, NRRD_BIG_INT I, double d) {
  return(v[I]=d);}
double _nrrdDInsertI(int *v, NRRD_BIG_INT I, double d) {
  return(v[I]=d);}
double _nrrdDInsertUI(unsigned int *v, NRRD_BIG_INT I, double d) {
  return(v[I]=d);}
double _nrrdDInsertLLI(long long int *v, NRRD_BIG_INT I, double d) {
  return(v[I]=d);}
double _nrrdDInsertULLI(unsigned long long *v, NRRD_BIG_INT I, double d) {
  return(v[I]=d);}
double _nrrdDInsertF(float *v, NRRD_BIG_INT I, double d) {
  return(v[I]=d);}
double _nrrdDInsertD(double *v, NRRD_BIG_INT I, double d) {
  return(v[I]=d);}
double _nrrdDInsertLD(long double *v, NRRD_BIG_INT I, double d) {
  return(v[I]=d);}
double (*nrrdDInsert[NRRD_MAX_TYPE+1])(void *, NRRD_BIG_INT, double) = {
  NULL,
  (double (*)(void*, NRRD_BIG_INT, double))_nrrdDInsertC,
  (double (*)(void*, NRRD_BIG_INT, double))_nrrdDInsertUC,
  (double (*)(void*, NRRD_BIG_INT, double))_nrrdDInsertS,
  (double (*)(void*, NRRD_BIG_INT, double))_nrrdDInsertUS,
  (double (*)(void*, NRRD_BIG_INT, double))_nrrdDInsertI,
  (double (*)(void*, NRRD_BIG_INT, double))_nrrdDInsertUI,
  (double (*)(void*, NRRD_BIG_INT, double))_nrrdDInsertLLI,
  (double (*)(void*, NRRD_BIG_INT, double))_nrrdDInsertULLI,
  (double (*)(void*, NRRD_BIG_INT, double))_nrrdDInsertF,
  (double (*)(void*, NRRD_BIG_INT, double))_nrrdDInsertD,
  (double (*)(void*, NRRD_BIG_INT, double))_nrrdDInsertLD,
  NULL};

/*
******** nrrdSprint
**
** Dereferences pointer v and sprintf()s that value into given string s,
** returns the result of sprintf()
*/
int _nrrdSprintC(char *s, char *v) {
  return(sprintf(s, "%d", *v)); }
int _nrrdSprintUC(char *s, unsigned char *v) {
  return(sprintf(s, "%u", *v)); }
int _nrrdSprintS(char *s, short *v) {
  return(sprintf(s, "%d", *v)); }
int _nrrdSprintUS(char *s, unsigned short *v) {
  return(sprintf(s, "%u", *v)); }
int _nrrdSprintI(char *s, int *v) {
  return(sprintf(s, "%d", *v)); }
int _nrrdSprintUI(char *s, unsigned int *v) {
  return(sprintf(s, "%u", *v)); }
int _nrrdSprintLLI(char *s, long long int *v) {
  return(sprintf(s, "%lld", *v)); }
int _nrrdSprintULLI(char *s, unsigned long long *v) {
  return(sprintf(s, "%llu", *v)); }
int _nrrdSprintF(char *s, float *v) {
  return(sprintf(s, "%f", *v)); }
int _nrrdSprintD(char *s, double *v) {
  return(sprintf(s, "%f", *v)); }
int _nrrdSprintLD(char *s, long double *v) {
  return(sprintf(s, "%Lf", *v)); }
int (*nrrdSprint[NRRD_MAX_TYPE+1])(char *, void *) = {
  NULL,
  (int (*)(char *, void *))_nrrdSprintC,
  (int (*)(char *, void *))_nrrdSprintUC,
  (int (*)(char *, void *))_nrrdSprintS,
  (int (*)(char *, void *))_nrrdSprintUS,
  (int (*)(char *, void *))_nrrdSprintI,
  (int (*)(char *, void *))_nrrdSprintUI,
  (int (*)(char *, void *))_nrrdSprintLLI,
  (int (*)(char *, void *))_nrrdSprintULLI,
  (int (*)(char *, void *))_nrrdSprintF,
  (int (*)(char *, void *))_nrrdSprintD,
  (int (*)(char *, void *))_nrrdSprintLD,
  NULL};

/*
******** nrrdFprint
**
** Dereferences pointer v and fprintf()s that value into given file f;
** returns the result of fprintf()
*/
int _nrrdFprintC(FILE *f, char *v) {
  return(fprintf(f, "%d", *v)); }
int _nrrdFprintUC(FILE *f, unsigned char *v) {
  return(fprintf(f, "%u", *v)); }
int _nrrdFprintS(FILE *f, short *v) {
  return(fprintf(f, "%d", *v)); }
int _nrrdFprintUS(FILE *f, unsigned short *v) {
  return(fprintf(f, "%u", *v)); }
int _nrrdFprintI(FILE *f, int *v) {
  return(fprintf(f, "%d", *v)); }
int _nrrdFprintUI(FILE *f, unsigned int *v) {
  return(fprintf(f, "%u", *v)); }
int _nrrdFprintLLI(FILE *f, long long int *v) {
  return(fprintf(f, "%lld", *v)); }
int _nrrdFprintULLI(FILE *f, unsigned long long *v) {
  return(fprintf(f, "%llu", *v)); }
int _nrrdFprintF(FILE *f, float *v) {
  return(fprintf(f, "%f", *v)); }
int _nrrdFprintD(FILE *f, double *v) {
  return(fprintf(f, "%f", *v)); }
int _nrrdFprintLD(FILE *f, long double *v) {
  return(fprintf(f, "%Lf", *v)); }
int (*nrrdFprint[NRRD_MAX_TYPE+1])(FILE *, void *) = {
  NULL,
  (int (*)(FILE *, void *))_nrrdFprintC,
  (int (*)(FILE *, void *))_nrrdFprintUC,
  (int (*)(FILE *, void *))_nrrdFprintS,
  (int (*)(FILE *, void *))_nrrdFprintUS,
  (int (*)(FILE *, void *))_nrrdFprintI,
  (int (*)(FILE *, void *))_nrrdFprintUI,
  (int (*)(FILE *, void *))_nrrdFprintLLI,
  (int (*)(FILE *, void *))_nrrdFprintULLI,
  (int (*)(FILE *, void *))_nrrdFprintF,
  (int (*)(FILE *, void *))_nrrdFprintD,
  (int (*)(FILE *, void *))_nrrdFprintLD,
  NULL};

/*
******** nrrdFClamp
**
** clamps a given float to the range to the range representable
** by the given fixed-point type; for floating point types we
** just return the given number.
*/
float _nrrdFClampC(float v) {
  if (v < SCHAR_MIN) return SCHAR_MIN;
  if (v > SCHAR_MAX) return SCHAR_MAX;
  return v;
}
float _nrrdFClampUC(float v) {
  if (v < 0) return 0;
  if (v > UCHAR_MAX) return UCHAR_MAX;
  return v;
}
float _nrrdFClampS(float v) {
  if (v < SHRT_MIN) return SHRT_MIN;
  if (v > SHRT_MAX) return SHRT_MAX;
  return v;
}
float _nrrdFClampUS(float v) {
  if (v < 0) return 0;
  if (v > USHRT_MAX) return USHRT_MAX;
  return v;
}
float _nrrdFClampI(float v) {
  if (v < INT_MIN) return INT_MIN;
  if (v > INT_MAX) return INT_MAX;
  return v;
}
float _nrrdFClampUI(float v) {
  if (v < 0) return 0;
  if (v > UINT_MAX) return UINT_MAX;
  return v;
}
float _nrrdFClampLLI(float v) {
  if (v < NRRD_LLONG_MIN) return NRRD_LLONG_MIN;
  if (v > NRRD_LLONG_MAX) return NRRD_LLONG_MAX;
  return v;
}
float _nrrdFClampULLI(float v) {
  if (v < 0) return 0;
  if (v > NRRD_ULLONG_MAX) return NRRD_ULLONG_MAX;
  return v;
}
float _nrrdFClampF(float v) { return v; }
float _nrrdFClampD(float v) { return v; }
float _nrrdFClampLD(float v) { return v; }
float (*nrrdFClamp[NRRD_MAX_TYPE+1])(float) = {
  NULL,
  _nrrdFClampC,
  _nrrdFClampUC,
  _nrrdFClampS,
  _nrrdFClampUS,
  _nrrdFClampI,
  _nrrdFClampUI,
  _nrrdFClampLLI,
  _nrrdFClampULLI,
  _nrrdFClampF,
  _nrrdFClampD,
  _nrrdFClampLD,
  NULL};

/*
******** nrrdDClamp
**
** same as nrrdDClamp, but for doubles
*/
double _nrrdDClampC(double v) {
  if (v < SCHAR_MIN) return SCHAR_MIN;
  if (v > SCHAR_MAX) return SCHAR_MAX;
  return v;
}
double _nrrdDClampUC(double v) {
  if (v < 0) return 0;
  if (v > UCHAR_MAX) return UCHAR_MAX;
  return v;
}
double _nrrdDClampS(double v) {
  if (v < SHRT_MIN) return SHRT_MIN;
  if (v > SHRT_MAX) return SHRT_MAX;
  return v;
}
double _nrrdDClampUS(double v) {
  if (v < 0) return 0;
  if (v > USHRT_MAX) return USHRT_MAX;
  return v;
}
double _nrrdDClampI(double v) {
  if (v < INT_MIN) return INT_MIN;
  if (v > INT_MAX) return INT_MAX;
  return v;
}
double _nrrdDClampUI(double v) {
  if (v < 0) return 0;
  if (v > UINT_MAX) return UINT_MAX;
  return v;
}
double _nrrdDClampLLI(double v) {
  if (v < NRRD_LLONG_MIN) return NRRD_LLONG_MIN;
  if (v > NRRD_LLONG_MAX) return NRRD_LLONG_MAX;
  return v;
}
double _nrrdDClampULLI(double v) {
  if (v < 0) return 0;
  if (v > NRRD_ULLONG_MAX) return NRRD_ULLONG_MAX;
  return v;
}
double _nrrdDClampF(double v) { return v; }
double _nrrdDClampD(double v) { return v; }
double _nrrdDClampLD(double v) { return v; }
double (*nrrdDClamp[NRRD_MAX_TYPE+1])(double) = {
  NULL,
  _nrrdDClampC,
  _nrrdDClampUC,
  _nrrdDClampS,
  _nrrdDClampUS,
  _nrrdDClampI,
  _nrrdDClampUI,
  _nrrdDClampLLI,
  _nrrdDClampULLI,
  _nrrdDClampF,
  _nrrdDClampD,
  _nrrdDClampLD,
  NULL};

/* about here is where Gordon admits he might have some use for C++ */

#define _MM_ARGS(type) type *minP, type *maxP, NRRD_BIG_INT N, type *v

#define _MM_FIXED(type)                                                  \
  NRRD_BIG_INT I, T;                                                     \
  type a, b, min, max;                                                   \
                                                                         \
  if (!(minP && maxP))                                                   \
    return;                                                              \
                                                                         \
  /* get initial values */                                               \
  min = max = v[0];                                                      \
                                                                         \
  /* run through array in pairs; by doing a compare on successive        \
     elements, we can do three compares per pair instead of the naive    \
     four.  In one very unexhaustive test on irix6.64, this resulted     \
     in a 20% decrease in running time */                                \
  T = N/2;                                                               \
  for (I=0; I<=T; I++) {                                                 \
    a = v[0 + 2*I];                                                      \
    b = v[1 + 2*I];                                                      \
    if (a < b) {                                                         \
      min = AIR_MIN(a, min);                                             \
      max = AIR_MAX(b, max);                                             \
    }                                                                    \
    else {                                                               \
      max = AIR_MAX(a, max);                                             \
      min = AIR_MIN(b, min);                                             \
    }                                                                    \
  }                                                                      \
                                                                         \
  /* get the very last one (may be redundant) */                         \
  a = v[N-1];                                                            \
  if (a < min) {                                                         \
    min = a;                                                             \
  }                                                                      \
  else {                                                                 \
    if (a > max) {                                                       \
      max = a;                                                           \
    }                                                                    \
  }                                                                      \
                                                                         \
  /* record results */                                                   \
  *minP = min;                                                           \
  *maxP = max;

#define _MM_FLOAT(type)                                                  \
  NRRD_BIG_INT I;                                                        \
  type a, min, max;                                                      \
  int something;                                                         \
                                                                         \
  if (!(minP && maxP))                                                   \
    return;                                                              \
                                                                         \
  /* we can't easily get initial values for min and max because we       \
     don't know where to find non-NaN values.  So we check for NaN       \
     at every element and assume that branch predictors will work */     \
  something = 0;                                                         \
  for (I=0; I<N; I++) {                                                  \
    a = v[I];                                                            \
    if (!AIR_EXISTS(a))                                                  \
      continue;                                                          \
    if (something) {                                                     \
      if (a < min) {                                                     \
	min = a;                                                         \
      }                                                                  \
      else {                                                             \
	if (a > max) {                                                   \
	  max = a;                                                       \
	}                                                                \
      }                                                                  \
    }                                                                    \
    else {                                                               \
      min = max = a;                                                     \
      something = 1;                                                     \
    }                                                                    \
  }                                                                      \
                                                                         \
  /* if we got something, then we have results, otherwise we don't */    \
  if (something) {                                                       \
    *minP = min;                                                         \
    *maxP = max;                                                         \
  }                                                                      \
  else {                                                                 \
    *minP = *maxP = AIR_NAN;                                             \
  }

void _nrrdMinMaxC   (_MM_ARGS(char))           { _MM_FIXED(char) }
void _nrrdMinMaxUC  (_MM_ARGS(unsigned char))  { _MM_FIXED(unsigned char) }
void _nrrdMinMaxS   (_MM_ARGS(short))          { _MM_FIXED(short) }
void _nrrdMinMaxUS  (_MM_ARGS(unsigned short)) { _MM_FIXED(unsigned short) }
void _nrrdMinMaxI   (_MM_ARGS(int))            { _MM_FIXED(int) }
void _nrrdMinMaxUI  (_MM_ARGS(unsigned int))   { _MM_FIXED(unsigned int) }
void _nrrdMinMaxLLI (_MM_ARGS(long long int))  { _MM_FIXED(long long int) }
void _nrrdMinMaxULLI(_MM_ARGS(unsigned long long int))   { 
  _MM_FIXED(unsigned long long int) }
void _nrrdMinMaxF   (_MM_ARGS(float))          { _MM_FLOAT(float) }
void _nrrdMinMaxD   (_MM_ARGS(double))         { _MM_FLOAT(double) }
void _nrrdMinMaxLD  (_MM_ARGS(long double))    { _MM_FLOAT(long double) }
void (*_nrrdMinMaxFind[NRRD_MAX_TYPE+1])(void *, void *, 
					 NRRD_BIG_INT, void *) = {
  NULL,
  (void (*)(void *, void *, NRRD_BIG_INT, void *))_nrrdMinMaxC,
  (void (*)(void *, void *, NRRD_BIG_INT, void *))_nrrdMinMaxUC,
  (void (*)(void *, void *, NRRD_BIG_INT, void *))_nrrdMinMaxS,
  (void (*)(void *, void *, NRRD_BIG_INT, void *))_nrrdMinMaxUS,
  (void (*)(void *, void *, NRRD_BIG_INT, void *))_nrrdMinMaxI,
  (void (*)(void *, void *, NRRD_BIG_INT, void *))_nrrdMinMaxUI,
  (void (*)(void *, void *, NRRD_BIG_INT, void *))_nrrdMinMaxLLI,
  (void (*)(void *, void *, NRRD_BIG_INT, void *))_nrrdMinMaxULLI,
  (void (*)(void *, void *, NRRD_BIG_INT, void *))_nrrdMinMaxF,
  (void (*)(void *, void *, NRRD_BIG_INT, void *))_nrrdMinMaxD,
  (void (*)(void *, void *, NRRD_BIG_INT, void *))_nrrdMinMaxLD,
  NULL
};
