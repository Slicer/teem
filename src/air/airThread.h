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

/* thread.c: general threading functions */
typedef struct {
#if TEEM_PTHREAD
  pthread_t id;
#elif defined(_WIN32)
  HANDLE id;
  void *(*body)(void *);
  void *arg;
  void *ret;
#else
  void *ret;
#endif
} airThread;

typedef struct {
#if _WIN32
/* STRONGBAD */
#elif TEEM_PTHREAD
/* HOMESTARRUNNER.NET */
#else
/* POOPSMITH */
#endif
} airThreadBarrier;

extern int airThreadCreate(airThread *thread, void *(*threadBody)(void *), void *arg);
extern int airThreadJoin(airThread *thread, void **retP);
extern int airThreadBarrierInit(airThreadBarrier *barrier, unsigned int count);
extern int airThreadBarrierWait(airThreadBarrier *barrier);

#endif /* AIR_THREAD_HAS_BEEN_INCLUDED */
