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

/* HEY: the whole matter of function returns has to be standardized ... */

int airThreadNoopWarning = AIR_TRUE;

/* ------------------------------------------------------------------ */
#if TEEM_PTHREAD /* ----------------------------------------- PTHREAD */
/* ------------------------------------------------------------------ */

#include <pthread.h>

const int airThreadCapable = AIR_TRUE;

typedef struct {
  pthread_t id;
} _airThread;

typedef struct {
  pthread_mutex_t id;
} _airThreadMutex;

typedef struct {
  pthread_cond_t id;
} _airThreadCond;

airThread *
airThreadNew(void) {
  _airThread *thread;

  thread = (_airThread *)calloc(1, sizeof(_airThread));
  /* HEY: not sure if this can be usefully initialized */
  return (airThread *)thread;
}

int
airThreadStart(airThread *_thread, void *(*threadBody)(void *), void *arg) {
  pthread_attr_t attr;
  _airThread *thread;

  thread = (_airThread *)_thread;
  pthread_attr_init(&attr);
#ifdef __sgi
  pthread_attr_setscope(&attr, PTHREAD_SCOPE_BOUND_NP);
#endif
  return pthread_create(&(thread->id), &attr, threadBody, arg);
}

int
airThreadJoin(airThread *_thread, void **retP) {
  _airThread *thread;

  thread = (_airThread *)_thread;
  return pthread_join(thread->id, retP);
}

airThread *
airThreadNix(airThread *_thread) {

  return airFree(_thread);
}

airThreadMutex *
airThreadMutexNew(void) {
  _airThreadMutex *mutex;

  mutex = (_airThreadMutex *)calloc(1, sizeof(_airThreadMutex));
  if (mutex) {
    if (pthread_mutex_init(&(mutex->id), NULL)) {
      mutex = airFree(mutex);
    }
  }
  return (airThreadMutex *)mutex;
}

int
airThreadMutexLock(airThreadMutex *_mutex) {
  _airThreadMutex *mutex;

  mutex = (_airThreadMutex *)_mutex;
  return pthread_mutex_lock(&(mutex->id));
}

int
airThreadMutexUnlock(airThreadMutex *_mutex) {
  _airThreadMutex *mutex;

  mutex = (_airThreadMutex *)_mutex;
  return pthread_mutex_unlock(&(mutex->id));
}

airThreadMutex *
airThreadMutexNix(airThreadMutex *_mutex) {
  _airThreadMutex *mutex;

  if (_mutex) {
    mutex = (_airThreadMutex *)_mutex;
    if (!pthread_mutex_destroy(&(mutex->id))) {
      /* there was no error */
      _mutex = airFree(_mutex);
    }
  }
  return _mutex;
}

airThreadCond *
airThreadCondNew(void) {
  _airThreadCond *cond;
  
  cond = (_airThreadCond *)calloc(1, sizeof(_airThreadCond));
  if (cond) {
    if (pthread_cond_init(&(cond->id), NULL)) {
      /* there was an error */
      cond = airFree(cond);
    }
  }
  return (airThreadCond *)cond;
}

int
airThreadCondWait(airThreadCond *_cond, airThreadMutex *_mutex) {
  _airThreadCond *cond;
  _airThreadMutex *mutex;

  cond = (_airThreadCond *)_cond;
  mutex = (_airThreadMutex *)_mutex;
  return pthread_cond_wait(&(cond->id), &(mutex->id));
}

int
airThreadCondSignal(airThreadCond *_cond) {
  _airThreadCond *cond;

  cond = (_airThreadCond *)_cond;
  return pthread_cond_signal(&(cond->id));
}

int
airThreadCondBroadcast(airThreadCond *_cond) {
  _airThreadCond *cond;

  cond = (_airThreadCond *)_cond;
  return pthread_cond_broadcast(&(cond->id));
}

airThreadCond *
airThreadCondNix(airThreadCond *_cond) {
  _airThreadCond *cond;
  
  if (_cond) {
    cond = (_airThreadCond *)_cond;
    if (!pthread_cond_destroy(&(cond->id))) {
      /* there was no error */
      _cond = airFree(_cond);
    }
  }
  return _cond;
}

/* ------------------------------------------------------------------ */
#elif defined(_WIN32) /* ------------------------------------- WIN 32 */
/* ------------------------------------------------------------------ */

#if defined(_WIN32)
   /* SignalObjectAndWait supported by NT4.0 and greater only */
#  define _WIN32_WINNT 0x400
#  include <windows.h>
#endif

const int airThreadCapable = AIR_TRUE;

typedef struct {
  HANDLE handle;
  void *(*body)(void *);
  void *arg;
  void *ret;
} _airThread;

typedef struct {
  HANDLE handle;
} _airThreadMutex;

typedef struct {
  int count;
  CRITICAL_SECTION lock;
  HANDLE sema;
  HANDLE done;
  size_t broadcast;
} _airThreadCond;

airThread *
airThreadNew(void) {
  _airThread *thread;

  thread = (_airThread *)calloc(1, sizeof(_airThread));
  /* HEY: any useful way to initialized a HANDLE */
  thread->body = NULL;
  thread->arg = thread->reg = NULL;
  return (airThread *)thread;
}

int WINAPI _airThreadWin32Body(void *arg) {
  airThread *t;

  t = (airThread *)arg;
  t->ret = t->body(t->arg);
  return 0;
}

int
airThreadStart(airThread *_thread, void *(*threadBody)(void *), void *arg) {
  _airThread *thread;

  thread = (_airThread *)_thread;
  thread->body = threadBody;
  thread->arg = arg;
  thread->handle = CreateThread(0, 0, _airThreadWin32Body, 
				(void *)&thread, 0, 0);
  return NULL == thread->handle;
}

int
airThreadJoin(airThread *_thread, void **retP) {
  int err;
  _airThread *thread;

  thread = (_airThread *)_thread;
  err = (WAIT_FAILED == WaitForSingleObject(thread->handle, INFINITE));
  *retP = thread->ret;
  return err;
}

airThread *
airThreadNix(airThread *_thread) {

  return airFree(_thread);
}

airThreadMutex *
airThreadMutexNew() {
  airThreadMutex *mutex;

  mutex = (airThreadMutex *)calloc(1, sizeof(_airThreadMutex));
  if (mutex) {
    if (!(mutex->handle = CreateMutex(NULL, TRUE, NULL))) {
      return airFree(mutex);
    }
  }
  return mutex;
}

int
airThreadMutexLock(airThreadMutex *mutex) {
  return WAIT_FAILED == WaitForSingleObject(mutex->handle, INFINITE);
}

int
airThreadMutexUnlock(airThreadMutex *mutex) {
  return 0 == ReleaseMutex(mutex->handle);
}

airThreadMutex *
airThreadMutexNix(airThreadMutex *mutex) {

  if (0 == CloseHandle(mutex->handle)) {
    mutex = airFree(mutex);
  }
  return mutex;
}

airThreadCond *
airThreadCondNew(void) {
  airThreadCond *cond;

  cond = (airThreadCond *)calloc(1, sizeof(_airThreadCond));
  if (cond) {
    cond->count = 0;
    cond->broadcast = 0;
    cond->sema = CreateSemaphore(NULL, 0, 0x7fffffff, NULL);
    if (NULL == cond->sema) {
      return airFree(cond);
    }
    InitializeCriticalSection(&(cond->lock));
    cond->done = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (NULL == cond->done) {
      CloseHandle(cond->sema);
      return airFree(cond);
    }
  }
  return cond;
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
  if (WAIT_FAILED == SignalObjectAndWait(mutex->handle, cond->sema,
					 INFINITE, FALSE)) {
    return 1;
  }
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
    if (WAIT_FAILED == SignalObjectAndWait(cond->done, mutex->handle,
					   INFINITE, FALSE)) {
      return 1;
    }
  } else {
    /* regain the external mutex since that's the guarantee to our callers */
    if (WAIT_FAILED == WaitForSingleObject(mutex->handle, INFINITE)) {
      return 1;
    }
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
    if (0 == ReleaseSemaphore(cond->sema, 1, NULL)) {
      return 1;
    }
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
    if (0 == ReleaseSemaphore(cond->sema, cond->count, 0)) {
      return 1;
    }
    LeaveCriticalSection(&(cond->lock));
    /* wait for all the awakened threads to acquire the counting semaphore */ 
    if (WAIT_FAILED == WaitForSingleObject(cond->done, INFINITE)) {
      return 1;
    }
    /* this assignment is okay, even without the lock held 
       because no other waiter threads can wake up to access it */
    cond->broadcast = 0;
  } else {
    LeaveCriticalSection(&(cond->lock));
  }
  return 0;
}

airThreadCond *
airThreadCondNix(airThreadCond *cond) {
  airThreadCond *ret;

  if (cond) {
    cond->count = 0;
    cond->broadcast = 0;
    if (0 == CloseHandle(cond->done)) {
      ret = cond;
    }
    DeleteCriticalSection(&(cond->lock));
    if (0 == CloseHandle(cond->sema)) {
      ret = cond;
    }
    ret = airFree(cond);
  }
  return ret;
}

/* ------------------------------------------------------------------ */
#else /* --------------------------------------- (no multi-threading) */
/* ------------------------------------------------------------------ */

const int airThreadCapable = AIR_FALSE;

typedef struct {
  void *ret;
} _airThread;

typedef struct {
  int dummy;
} _airThreadMutex;

typedef struct {
  int dummy;
} _airThreadCond;

airThread *
airThreadNew(void) {
  _airThread *thread;

  thread = (_airThread *)calloc(1, sizeof(_airThread));
  thread->read = NULL;
  return (airThread *)thread;
}

int
airThreadStart(airThread *_thread, void *(*threadBody)(void *), void *arg) {
  _airThread *thread;

  thread = (_airThread *)_thread;
  thread->ret = (*threadBody)(arg);
  return 0;
}

int
airThreadJoin(airThread *_thread, void **retP) {
  _airThread *thread;

  thread = (_airThread *)_thread;
  *retP = thread->ret;
  return 0;
}

airThread *
airThreadNix(airThread *_thread) {

  return airFree(_thread);
}

airThreadMutex *
airThreadMutexNew(void) {
  char me[]="airThreadMutexInit";
  airThreadMutex *mutex;

  mutex = (airThreadMutex *)calloc(1, sizeof(_airThreadMutex));
  if (mutex) {
    if (airThreadNoopWarning) {
      fprintf(stderr, "%s: WARNING: all mutex usage is a no-op!\n", me);
    }
  }
  return mutex;
}

int
airThreadMutexLock(airThreadMutex *mutex) {
  char me[]="airThreadMutexLock";
  if (airThreadNoopWarning) {
    fprintf(stderr, "%s: WARNING: all mutex usage is a no-op!\n", me);
  }
  return 0;
}

int
airThreadMutexUnlock(airThreadMutex *mutex) {
  char me[]="airThreadMutexUnlock";
  if (airThreadNoopWarning) {
    fprintf(stderr, "%s: WARNING: all mutex usage is a no-op!\n", me);
  }
  return 0;
}

airThreadMutex *
airThreadMutexNix(airThreadMutex *mutex) {
  
  return airFree(mutex);
}

airThreadCond *
airThreadCondNew(void) {
  char me[]="airThreadCondInit";
  airThreadCond *cond;
  
  cond = (airThreadCond *)calloc(1, sizeof(_airThreadCond));
  if (cond) {
    if (airThreadNoopWarning) {
      fprintf(stderr, "%s: WARNING: all cond usage is a no-op!\n", me);
    }
  }
  return cond;
}

int
airThreadCondWait(airThreadCond *cond, airThreadMutex *mutex) {
  char me[]="airThreadCondWait";
  if (airThreadNoopWarning) {
    fprintf(stderr, "%s: WARNING: all cond usage is a no-op!\n", me);
  }
  return 0;
}

int
airThreadCondSignal(airThreadCond *cond) {
  char me[]="airThreadCondSignal";
  if (airThreadNoopWarning) {
    fprintf(stderr, "%s: WARNING: all cond usage is a no-op!\n", me);
  }
  return 0;
}

int
airThreadCondBroadcast(airThreadCond *cond) {
  char me[]="airThreadCondBroadcast";
  if (airThreadNoopWarning) {
    fprintf(stderr, "%s: WARNING: all cond usage is a no-op!\n", me);
  }
  return 0;
}

airThreadCond *
airThreadCondDone(airThreadCond *cond) {

  return airFree(cond);
}

/* ------------------------------------------------------------------ */
#endif /* ----------------------------------------------------------- */
/* ------------------------------------------------------------------ */

airThreadBarrier *
airThreadBarrierNew(unsigned int numUsers) {
  airThreadBarrier *barrier;
  
  barrier = (airThreadBarrier *)calloc(1, sizeof(airThreadBarrier));
  if (barrier) {
    barrier->numUsers = numUsers;
    barrier->numDone = 0;
    if (!(barrier->doneMutex = airThreadMutexNew())) {
      return airFree(barrier);
    }
    if (!(barrier->doneCond = airThreadCondNew())) {
      barrier->doneMutex = airThreadMutexNix(barrier->doneMutex);
      return airFree(barrier);
    }
  }
  return barrier;
}

int
airThreadBarrierWait(airThreadBarrier *barrier) {

  airThreadMutexLock(barrier->doneMutex);
  barrier->numDone += 1;
  if (barrier->numDone < barrier->numUsers) {
    airThreadCondWait(barrier->doneCond, barrier->doneMutex);
  } else {
    barrier->numDone = 0;
    airThreadCondBroadcast(barrier->doneCond);
  }
  airThreadMutexUnlock(barrier->doneMutex);
  return 0;
}

airThreadBarrier *
airThreadBarrierNix(airThreadBarrier *barrier) {
  
  barrier->doneMutex = airThreadMutexNix(barrier->doneMutex);
  barrier->doneCond = airThreadCondNix(barrier->doneCond);
  return airFree(barrier);
}
