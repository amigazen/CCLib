/* SPDX-License-Identifier: BSD-2-Clause
 * Copyright (c) 2026 amigazen project
 */

#ifndef PTHREAD_H
#define PTHREAD_H 1

/*
 * pthread - static ccpthread.lib (not cclib.library LVOs).
 * Requires an active CClib session (SetupSTDIO) on each thread.
 * Surface tracks unix.lib2 pthread.lib (CClib session deltas only).
 */

#include <exec/types.h>
#include <exec/lists.h>
#include <exec/semaphores.h>
#include <stddef.h>
#include <time.h>
#include <errno.h>
#include <libraries/cclib.h>

#ifndef SEMAPHORE_H
#include <semaphore.h>
#endif

typedef ULONG pthread_t;
typedef ULONG pthread_key_t;
typedef struct pthread_attr pthread_attr_t;
typedef struct pthread_mutex pthread_mutex_t;
typedef struct pthread_mutexattr pthread_mutexattr_t;
typedef struct pthread_cond pthread_cond_t;
typedef struct pthread_condattr pthread_condattr_t;
typedef struct pthread_rwlock pthread_rwlock_t;
typedef struct pthread_rwlockattr pthread_rwlockattr_t;
typedef struct pthread_once pthread_once_t;

#define PTHREAD_CREATE_JOINABLE 0
#define PTHREAD_CREATE_DETACHED 1

#define PTHREAD_MUTEX_NORMAL    0
#define PTHREAD_MUTEX_RECURSIVE 1
#define PTHREAD_MUTEX_DEFAULT   PTHREAD_MUTEX_NORMAL

#define PTHREAD_CANCEL_ENABLE   0
#define PTHREAD_CANCEL_DISABLE  1
#define PTHREAD_CANCEL_DEFERRED 0
#define PTHREAD_CANCEL_ASYNCHRONOUS 1

#define PTHREAD_CANCELED ((void *)(-1L))

#define PTHREAD_THREADS_MAX 64
#define PTHREAD_KEYS_MAX    32
#define PTHREAD_STACK_MIN   8192

#define PTHREAD_ONCE_INIT { 0 }

struct pthread_attr
{
	int detachstate;
	ULONG stacksize;
};

struct pthread_mutex
{
	struct SignalSemaphore semaphore;
	int kind;
	int waiters;
};

struct pthread_mutexattr
{
	int kind;
};

struct pthread_cond
{
	struct SignalSemaphore semaphore;
	struct List waiters;
};

struct pthread_condattr
{
	int dummy;
};

struct pthread_rwlock
{
	pthread_mutex_t lock;
	pthread_cond_t readers_cond;
	pthread_cond_t writers_cond;
	int readers_active;
	int writers_active;
	int writers_waiting;
};

struct pthread_rwlockattr
{
	int dummy;
};

struct pthread_once
{
	int done;
	struct SignalSemaphore sem;
	int sem_inited;
};

#define PTHREAD_MUTEX_INITIALIZER { {0}, PTHREAD_MUTEX_DEFAULT, 0 }
#define PTHREAD_COND_INITIALIZER { {0}, {0} }
#define PTHREAD_RWLOCK_INITIALIZER \
	{ PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER, \
	  PTHREAD_COND_INITIALIZER, 0, 0, 0 }

int pthread_attr_init(pthread_attr_t *attr);
int pthread_attr_destroy(pthread_attr_t *attr);
int pthread_attr_setdetachstate(pthread_attr_t *attr, int state);
int pthread_attr_getdetachstate(const pthread_attr_t *attr, int *state);
int pthread_attr_setstacksize(pthread_attr_t *attr, size_t stacksize);
int pthread_attr_getstacksize(const pthread_attr_t *attr, size_t *stacksize);

int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
	void *(*start_routine)(void *), void *arg);
int pthread_join(pthread_t thread, void **retval);
int pthread_detach(pthread_t thread);
void pthread_exit(void *retval);
pthread_t pthread_self(void);
int pthread_equal(pthread_t t1, pthread_t t2);

int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr);
int pthread_mutex_destroy(pthread_mutex_t *mutex);
int pthread_mutex_lock(pthread_mutex_t *mutex);
int pthread_mutex_trylock(pthread_mutex_t *mutex);
int pthread_mutex_timedlock(pthread_mutex_t *mutex,
	const struct timespec *abstime);
int pthread_mutex_unlock(pthread_mutex_t *mutex);

int pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr);
int pthread_cond_destroy(pthread_cond_t *cond);
int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex);
int pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex,
	const struct timespec *abstime);
int pthread_cond_signal(pthread_cond_t *cond);
int pthread_cond_broadcast(pthread_cond_t *cond);

int pthread_rwlock_init(pthread_rwlock_t *rwlock,
	const pthread_rwlockattr_t *attr);
int pthread_rwlock_destroy(pthread_rwlock_t *rwlock);
int pthread_rwlock_rdlock(pthread_rwlock_t *rwlock);
int pthread_rwlock_tryrdlock(pthread_rwlock_t *rwlock);
int pthread_rwlock_wrlock(pthread_rwlock_t *rwlock);
int pthread_rwlock_trywrlock(pthread_rwlock_t *rwlock);
int pthread_rwlock_unlock(pthread_rwlock_t *rwlock);

int pthread_key_create(pthread_key_t *key, void (*destructor)(void *));
int pthread_key_delete(pthread_key_t key);
int pthread_setspecific(pthread_key_t key, const void *value);
void *pthread_getspecific(pthread_key_t key);

int pthread_cancel(pthread_t thread);
void pthread_testcancel(void);
int pthread_setcancelstate(int state, int *oldstate);
int pthread_setcanceltype(int type, int *oldtype);

int pthread_once(pthread_once_t *once_control, void (*init_routine)(void));

#endif /* PTHREAD_H */
