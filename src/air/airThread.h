/*
  teem: Gordon Kindlmann's research software
  Copyright (C) 2003, 2002, 2001, 2000, 1999, 1998 University of Utah

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

#ifndef AIR_THREAD_HAS_BEEN_INCLUDED
#define AIR_THREAD_HAS_BEEN_INCLUDED

#if TEEM_PTHREAD
#include <pthread.h>
#elif defined(_WIN32)
#include <windows.h>
#endif

#if defined(_WIN32) && !defined(TEEM_STATIC)
#define air_export __declspec(dllimport)
#else
#define air_export
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* threadAir.c: simplistic wrapper functions for multi-threading  */

/* ------------------------------------------------------------------ */
#if TEEM_PTHREAD /* ----------------------------------------- PTHREAD */
/* ------------------------------------------------------------------ */

/* perhaps these should be direct typedef's, without a wrapping struct */

typedef struct {
  pthread_t id;
} airThread;

typedef struct {
  pthread_mutex_t id;
} airThreadMutex;

typedef struct {
  pthread_cond_t id;
} airThreadCond;

/* ------------------------------------------------------------------ */
#elif defined(_WIN32) /* ------------------------------------- WIN 32 */
/* ------------------------------------------------------------------ */

typedef struct {
  HANDLE handle;
  void *(*body)(void *);
  void *arg;
  void *ret;
} airThread;

typedef struct {
  HANDLE handle;
} airThreadMutex;

typedef struct {
  int count;
  CRITICAL_SECTION lock;
  HANDLE sema;
  HANDLE done;
  size_t broadcast;
} airThreadCond;

/* ------------------------------------------------------------------ */
#else /* --------------------------------------- (no multi-threading) */
/* ------------------------------------------------------------------ */

typedef struct {
  void *ret;
} airThread;

typedef struct {
  int dummy;
} airThreadMutex;

typedef struct {
  int dummy;
} airThreadCond;

/* ------------------------------------------------------------------ */
#endif /* ----------------------------------------------------------- */
/* ------------------------------------------------------------------ */

typedef struct {
  unsigned int numUsers, numDone;
  airThreadMutex doneMutex;
  airThreadCond doneCond;
} airThreadBarrier;

/* if non-zero: we have some kind of multi-threading available, either
   via pthreads, or via Windows stuff */
extern air_export const int airThreadCapable;

/* When multi-threading is not available, and hence constructs like
   mutexes are not available, the operations on them will be no-ops. When
   this variable is non-zero, we fprintf(stderr) a warning to this effect
   when those constructs are used */
extern air_export int airThreadNoopWarning; 

extern int airThreadCreate(airThread *thread, void *(*threadBody)(void *),
			   void *arg);
extern int airThreadJoin(airThread *thread, void **retP);

extern int airThreadMutexInit(airThreadMutex *mutex);
extern int airThreadMutexLock(airThreadMutex *mutex);
extern int airThreadMutexUnlock(airThreadMutex *mutex);
extern int airThreadMutexDone(airThreadMutex *mutex);

extern int airThreadCondInit(airThreadCond *cond);
extern int airThreadCondWait(airThreadCond *cond, airThreadMutex *mutex);
extern int airThreadCondSignal(airThreadCond *cond);
extern int airThreadCondBroadcast(airThreadCond *cond);
extern int airThreadCondDone(airThreadCond *cond);

extern int airThreadBarrierInit(airThreadBarrier *barrier, unsigned numUsers);
extern int airThreadBarrierWait(airThreadBarrier *barrier);
extern int airThreadBarrierDone(airThreadBarrier *barrier);

#ifdef __cplusplus
}
#endif

#endif /* AIR_THREAD_HAS_BEEN_INCLUDED */
