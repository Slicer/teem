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

#include "air.h"
#include "airThread.h"

/* HEY: the whole matter of function returns has to be standardized ... */

int airThreadNoopWarning = AIR_TRUE;

/* ------------------------------------------------------------------ */
#if TEEM_PTHREAD /* ----------------------------------------- PTHREAD */
/* ------------------------------------------------------------------ */

const int airThreadCapable = AIR_TRUE;

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

const int airThreadCapable = AIR_TRUE;

int WINAPI _airThreadWin32Body(void *arg) {
  airThread *t;

  t = (airThread *)arg;
  t->ret = t->body(t->arg);
  return 0;
}

int
airThreadCreate(airThread *thread, void *(*threadBody)(void *), void *arg) {

  thread->body = threadBody;
  thread->arg = arg;
  thread->handle = CreateThread(0, 0, _airThreadWin32Body, (void *)&thread, 0, 0);
  return NULL == thread->handle;
}

int
airThreadJoin(airThread *thread, void **retP) {
  int err;

  err = (WAIT_FAILED == WaitForSingleObject(thread->handle, INFINITE));
  *retP = thread->ret;
  return err;
}

int
airThreadMutexInit(airThreadMutex *mutex) {
  mutex->handle = CreateMutex(NULL, TRUE, NULL);
  return NULL == mutex->handle;
}

int
airThreadMutexLock(airThreadMutex *mutex) {
  return WAIT_FAILED == WaitForSingleObject(mutex->handle, INFINITE);
}

int
airThreadMutexUnlock(airThreadMutex *mutex) {
  return 0 == ReleaseMutex(mutex->handle);
}

int
airThreadMutexDone(airThreadMutex *mutex) {
  return 0 == CloseHandle(mutex->handle);
}

int
airThreadCondInit(airThreadCond *cond) {
  cond->count = 0;
  cond->broadcast = 0;
  cond->sema = CreateSemaphore(NULL, 0, 0x7fffffff, NULL);
  if (NULL == cond->sema) return 1;
  InitializeCriticalSection(&(cond->lock));
  cond->done = CreateEvent(NULL, FALSE, FALSE, NULL);
  if (NULL == cond->done) {
    CloseHandle(cond->sema);
    return 1;
  }
  return 0;
}

int
airThreadCondWait(airThreadCond *cond, airThreadMutex *mutex) {
  int last;

  /* increment count */
  EnterCriticalSection(&(cond->lock)); /* avoid race conditions */
  cond->count++;
  LeaveCriticalSection(&(cond->lock));
  /* atomically release the mutex and wait on the
     semaphore until airThreadCondSignal or airThreadCondBroadcast
     are called by another thread */
  if (WAIT_FAILED == SignalObjectAndWait(mutex->handle, cond->sema, INFINITE, FALSE))
    return 1;
  /* reacquire lock to avoid race conditions */
  EnterCriticalSection(&(cond->lock));
  /* we're no longer waiting... */
  cond->count--;
  /* check to see if we're the last waiter after airThreadCondBroadcast */
  last = (cond->broadcast && 0 == cond->count);
  LeaveCriticalSection(&(cond->lock));
  /* if we're the last waiter thread during this particular broadcast
     then let all the other threads proceed */
  if (last) {
    /* atomically signal the done event and waits until
       we can acquire the mutex (this is required to ensure fairness) */
    if (WAIT_FAILED == SignalObjectAndWait(cond->done, mutex->handle, INFINITE, FALSE))
      return 1;
  } else {
    /* regain the external mutex since that's the guarantee to our callers */
    if (WAIT_FAILED == WaitForSingleObject(mutex->handle, INFINITE))
      return 1;
  }
  return 0;
}

int
airThreadCondSignal(airThreadCond *cond) {
  int waiters;
  
  EnterCriticalSection(&(cond->lock));
  waiters = cond->count > 0;
  LeaveCriticalSection(&(cond->lock));
  /* if there aren't any waiters, then this is a no-op */
  if (waiters) {
    if (0 == ReleaseSemaphore(cond->sema, 1, NULL))
      return 1;
  }
  return 0;
}

int
airThreadCondBroadcast(airThreadCond *cond) {
  int waiters;

  /* need to ensure that cond->count and cond->broadcast are consistent */
  EnterCriticalSection(&(cond->lock));
  waiters = 0;
  if (cond->count > 0) {
    /* we are broadcasting, even if there is just one waiter...
       record that we are broadcasting, which helps optimize
       airThreadCondWait for the non-broadcast case */
    cond->broadcast = 1;
    waiters = 1;
  }
  if (waiters) {
    /* wake up all the waiters atomically */
    if (0 == ReleaseSemaphore(cond->sema, cond->count, 0))
      return 1;
    LeaveCriticalSection(&(cond->lock));
    /* wait for all the awakened threads to acquire the counting semaphore */ 
    if (WAIT_FAILED == WaitForSingleObject(cond->done, INFINITE))
      return 1;
    /* this assignment is okay, even without the lock held 
       because no other waiter threads can wake up to access it */
    cond->broadcast = 0;
  } else {
    LeaveCriticalSection(&(cond->lock));
  }
  return 0;
}

int
airThreadCondDone(airThreadCond *cond) {
  cond->count = 0;
  cond->broadcast = 0;
  if (0 == CloseHandle(cond->done))
    return 1;
  DeleteCriticalSection(&(cond->lock));
  if (0 == CloseHandle(cond->sema))
    return 1;
  return 0;
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
    fprintf(stderr, "%s: WARNING: without threads, this is a noop!\n", me);
  }
  return 0;
}

int
airThreadMutexLock(airThreadMutex *mutex) {
  char me[]="airThreadMutexLock";
  if (airThreadNoopWarning) {
    fprintf(stderr, "%s: WARNING: without threads, this is a noop!\n", me);
  }
  return 0;
}

int
airThreadMutexUnlock(airThreadMutex *mutex) {
  char me[]="airThreadMutexUnlock";
  if (airThreadNoopWarning) {
    fprintf(stderr, "%s: WARNING: without threads, this is a noop!\n", me);
  }
  return 0;
}

int
airThreadMutexDone(airThreadMutex *mutex) {
  char me[]="airThreadMutexDone";
  if (airThreadNoopWarning) {
    fprintf(stderr, "%s: WARNING: without threads, this is a noop!\n", me);
  }
  return 0;
}

int
airThreadCondInit(airThreadCond *cond) {
  char me[]="airThreadCondInit";
  if (airThreadNoopWarning) {
    fprintf(stderr, "%s: WARNING: without threads, this is a noop!\n", me);
  }
  return 0;
}

int
airThreadCondWait(airThreadCond *cond, airThreadMutex *mutex) {
  char me[]="airThreadCondWait";
  if (airThreadNoopWarning) {
    fprintf(stderr, "%s: WARNING: without threads, this is a noop!\n", me);
  }
  return 0;
}

int
airThreadCondSignal(airThreadCond *cond) {
  char me[]="airThreadCondSignal";
  if (airThreadNoopWarning) {
    fprintf(stderr, "%s: WARNING: without threads, this is a noop!\n", me);
  }
  return 0;
}

int
airThreadCondBroadcast(airThreadCond *cond) {
  char me[]="airThreadCondBroadcast";
  if (airThreadNoopWarning) {
    fprintf(stderr, "%s: WARNING: without threads, this is a noop!\n", me);
  }
  return 0;
}

int
airThreadCondDone(airThreadCond *cond) {
  char me[]="airThreadCondDone";
  if (airThreadNoopWarning) {
    fprintf(stderr, "%s: WARNING: without threads, this is a noop!\n", me);
  }
  return 0;
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
