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

#include "airThread.h"

#if TEEM_PTHREAD || defined(_WIN32)
const int airMultiThreaded = 1;
#else
const int airMultiThreaded = 0;
#endif

/* ------------------------------------------------------------------ */
#if TEEM_PTHREADS /* --------------------------------------- PTHREADS */
/* ------------------------------------------------------------------ */

int
airThreadCreate(airThread* thread, void *(*threadBody)(void *), void *arg) {
  pthread_attr_t attr;

  pthread_attr_init(&attr);
#  ifdef __sgi
  pthread_attr_setscope(&attr, PTHREAD_SCOPE_BOUND_NP);
#  endif
  return pthread_create(thread->id, &attr, threadBody, arg);
}

int
airThreadJoin(airThread *thread, void **retP) {
  return pthread_join(thread->id, retP);
}

/* ------------------------------------------------------------------ */
#elif defined(_WIN32) /* ------------------------------------- WIN 32 */
/* ------------------------------------------------------------------ */

int WINAPI airThreadWin32Body(void *arg) {
  airThread *t;

  t = (airThread *)arg;
  t->ret = t->body(t->arg);
  return 0;
}

int
airThreadCreate(airThread *thread, void *(*threadBody)(void *), void *arg) {
  thread->body = threadBody;
  thread->arg = arg;
  thread->id = CreateThread(0, 0, airThreadWin32Body, (void *)&thread, 0, 0);
  return NULL == thread0->id;
}

int
airThreadJoin(airThread *thread, void **retP) {
  int err;

  err = (WAIT_FAILED == WaitForSingleObject(thread->id, INFINITE));
  *retP = thread->ret;
  return err;
}

/* ------------------------------------------------------------------ */
#else /* ------------------------------------------------------------ */
/* ------------------------------------------------------------------ */

airThread t;

int
airThreadCreate(airThread *thread, void *(*threadBody)(void *), void *arg) {
  thread->ret = (*threadBody)(arg);
  return 0;
}

int
airThreadJoin(airThread *thread, void **retP) {
  retP = thread->ret;
  return 0;
}

#endif
