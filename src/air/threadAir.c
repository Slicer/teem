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

/* HEY: the whole matter of function returns has to be standardized ... */

int airThreadNoopWarning = 1;

/* ------------------------------------------------------------------ */
#if TEEM_PTHREAD /* ----------------------------------------- PTHREAD */
/* ------------------------------------------------------------------ */

const int airThreadCapable = 1;

int
airThreadCreate(airThread* thread, void *(*threadBody)(void *), void *arg) {
  pthread_attr_t attr;

  pthread_attr_init(&attr);
#ifdef __sgi
  pthread_attr_setscope(&attr, PTHREAD_SCOPE_BOUND_NP);
#endif
  return pthread_create(&(thread->id), &attr, threadBody, arg);
}

int
airThreadJoin(airThread *thread, void **retP) {
  return pthread_join(thread->id, retP);
}

int
airThreadMutexInit(airThreadMutex *mutex) {
  return pthread_mutex_init(&(mutex->id), NULL);
}

int
airThreadMutexLock(airThreadMutex *mutex) {
  return pthread_mutex_lock(&(mutex->id));
}

int
airThreadMutexUnlock(airThreadMutex *mutex) {
  return pthread_mutex_unlock(&(mutex->id));
}

int
airThreadMutexDone(airThreadMutex *mutex) {
  return pthread_mutex_destroy(&(mutex->id));
}

int
airThreadCondInit(airThreadCond *cond) {
  return pthread_cond_init(&(cond->id), NULL);
}

int
airThreadCondWait(airThreadCond *cond, airThreadMutex *mutex) {
  return pthread_cond_wait(&(cond->id), &(mutex->id));
}

int
airThreadCondSignal(airThreadCond *cond) {
  return pthread_cond_signal(&(cond->id));
}

int
airThreadCondBroadcast(airThreadCond *cond) {
  return pthread_cond_broadcast(&(cond->id));
}

int
airThreadCondDone(airThreadCond *cond) {
  return pthread_cond_destroy(&(cond->id));
}

/* ------------------------------------------------------------------ */
#elif defined(_WIN32) /* ------------------------------------- WIN 32 */
/* ------------------------------------------------------------------ */

const int airThreadCapable = 1;

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

int
airThreadMutexInit(airThreadMutex *mutex) {

}

int
airThreadMutexLock(airThreadMutex *mutex) {

}

int
airThreadMutexUnlock(airThreadMutex *mutex) {

}

int
airThreadMutexDone(airThreadMutex *mutex) {

}

int
airThreadCondInit(airThreadCond *cond) {

}

int
airThreadCondWait(airThreadCond *cond, airThreadMutex *mutex) {

}

int
airThreadCondSignal(airThreadCond *cond) {

}

int
airThreadCondBroadcast(airThreadCond *cond) {

}

int
airThreadCondDone(airThreadCond *cond) {

}

/* ------------------------------------------------------------------ */
#else /* --------------------------------------- (no multi-threading) */
/* ------------------------------------------------------------------ */

const int airThreadCapable = AIR_FALSE;

int
airThreadCreate(airThread *thread, void *(*threadBody)(void *), void *arg) {
  thread->ret = (*threadBody)(arg);
  return 0;
}

int
airThreadJoin(airThread *thread, void **retP) {
  *retP = thread->ret;
  return 0;
}

int
airThreadMutexInit(airThreadMutex *mutex) {
  char me[]="airThreadMutexInit";
  if (airThreadNoopWarning) {
    fprintf(stderr, "%s: WARNING: without threads, this is a noop!\n");
  }
}

int
airThreadMutexLock(airThreadMutex *mutex) {
  char me[]="airThreadMutexLock";
  if (airThreadNoopWarning) {
    fprintf(stderr, "%s: WARNING: without threads, this is a noop!\n");
  }
}

int
airThreadMutexUnlock(airThreadMutex *mutex) {
  char me[]="airThreadMutexUnlock";
  if (airThreadNoopWarning) {
    fprintf(stderr, "%s: WARNING: without threads, this is a noop!\n");
  }
}

int
airThreadMutexDone(airThreadMutex *mutex) {
  char me[]="airThreadMutexDone";
  if (airThreadNoopWarning) {
    fprintf(stderr, "%s: WARNING: without threads, this is a noop!\n");
  }
}

int
airThreadCondInit(airThreadCond *cond) {
  char me[]="airThreadCondInit";
  if (airThreadNoopWarning) {
    fprintf(stderr, "%s: WARNING: without threads, this is a noop!\n");
  }
}

int
airThreadCondWait(airThreadCond *cond, airThreadMutex *mutex) {
  char me[]="airThreadCondWait";
  if (airThreadNoopWarning) {
    fprintf(stderr, "%s: WARNING: without threads, this is a noop!\n");
  }
}

int
airThreadCondSignal(airThreadCond *cond) {
  char me[]="airThreadCondSignal";
  if (airThreadNoopWarning) {
    fprintf(stderr, "%s: WARNING: without threads, this is a noop!\n");
  }
}

int
airThreadCondBroadcast(airThreadCond *cond) {
  char me[]="airThreadCondBroadcast";
  if (airThreadNoopWarning) {
    fprintf(stderr, "%s: WARNING: without threads, this is a noop!\n");
  }
}

int
airThreadCondDone(airThreadCond *cond) {
  char me[]="airThreadCondDone";
  if (airThreadNoopWarning) {
    fprintf(stderr, "%s: WARNING: without threads, this is a noop!\n");
  }
}

/* ------------------------------------------------------------------ */
#endif /* ----------------------------------------------------------- */
/* ------------------------------------------------------------------ */

int
airThreadBarrierInit(airThreadBarrier *barrier, unsigned int numUsers) {
  int ret1, ret2;
  
  barrier->numUsers = numUsers;
  barrier->numDone = 0;
  ret1 = airThreadMutexInit(&(barrier->doneMutex));
  ret2 = airThreadCondInit(&(barrier->doneCond));
  return (ret1 | ret2);
}

int
airThreadBarrierWait(airThreadBarrier *barrier) {

  airThreadMutexLock(&(barrier->doneMutex));
  barrier->numDone += 1;
  if (barrier->numDone < barrier->numUsers) {
    airThreadCondWait(&(barrier->doneCond), &(barrier->doneMutex));
  } else {
    barrier->numDone = 0;
    airThreadCondBroadcast(&(barrier->doneCond));
  }
  airThreadMutexUnlock(&(barrier->doneMutex));
  return 0;
}

int
airThreadBarrierDone(airThreadBarrier *barrier) {
  
  airThreadMutexDone(&(barrier->doneMutex));
  airThreadCondDone(&(barrier->doneCond));
  return 0;
}
