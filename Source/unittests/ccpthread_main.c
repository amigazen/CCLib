/* SPDX-License-Identifier: BSD-2-Clause
 * Copyright (c) 2026 amigazen project
 */

/*
 * ccpthread_main.c - CCPthread harness (cctest_util format)
 *
 * Covers first-cut ccpthread.lib.  After PASS, enters Ctrl-C hold with
 * atexit cleanup (same UX as the original smoke).  Pass "nohold" to exit
 * after SUMMARY (smake test).
 *
 * Workers must not touch CRT globals stdin/stdout/stderr.
 */

#include <exec/types.h>
#include <proto/dos.h>
#include <proto/cclib.h>
#include <libraries/cclib.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include "cctest_util.h"

extern int atexit(void (*func)(void));

static pthread_mutex_t g_mu = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t g_cv = PTHREAD_COND_INITIALIZER;
static sem_t g_sem;
static int g_once_count;
static pthread_once_t g_once = PTHREAD_ONCE_INIT;

static int g_flag;
static int g_ready;
static int g_wakes;
static int g_worker_equal_ok;
static int g_tls_dtor_hits;
static volatile int g_run;
static int g_hold_live;
static int g_sync_hold;
static pthread_t g_hold_th;

static void
once_fn(void)
{
	g_once_count++;
}

static void
abs_after(struct timespec *ts, long add_sec)
{
	time_t now;

	now = time((time_t *)NULL);
	ts->tv_sec = (long)now + add_sec;
	ts->tv_nsec = 0L;
}

static void *
worker_basic(void *arg)
{
	(void)arg;
	g_worker_equal_ok =
		(pthread_equal(pthread_self(), pthread_self()) != 0) ? 1 : 0;

	pthread_once(&g_once, once_fn);
	pthread_once(&g_once, once_fn);

	pthread_mutex_lock(&g_mu);
	g_flag = 1;
	pthread_cond_signal(&g_cv);
	pthread_mutex_unlock(&g_mu);

	sem_wait(&g_sem);
	return (void *)42L;
}

static void *
worker_exit(void *arg)
{
	(void)arg;
	pthread_exit((void *)99L);
	return (void *)0L;
}

static void *
worker_detach_done(void *arg)
{
	sem_t *done;

	done = (sem_t *)arg;
	(void)sem_post(done);
	return (void *)3L;
}

static void *
worker_bcast(void *arg)
{
	(void)arg;
	pthread_mutex_lock(&g_mu);
	g_ready++;
	pthread_cond_signal(&g_cv);
	while (g_flag == 0)
		pthread_cond_wait(&g_cv, &g_mu);
	g_wakes++;
	pthread_mutex_unlock(&g_mu);
	return (void *)NULL;
}

static void *
worker_try_timeout(void *arg)
{
	struct timespec ts;
	int err;
	int *out;

	out = (int *)arg;
	abs_after(&ts, 1L);
	err = pthread_mutex_timedlock(&g_mu, &ts);
	*out = err;
	if (err == 0)
		pthread_mutex_unlock(&g_mu);
	return (void *)NULL;
}

static void
tls_dtor(void *p)
{
	(void)p;
	g_tls_dtor_hits++;
}

static void *
worker_tls(void *arg)
{
	pthread_key_t key;
	int err;

	key = *(pthread_key_t *)arg;
	err = pthread_setspecific(key, (const void *)0x1234L);
	if (err != 0)
		return (void *)1L;
	if (pthread_getspecific(key) != (void *)0x1234L)
		return (void *)2L;
	return (void *)0L;
}

static void *
worker_cancel(void *arg)
{
	(void)arg;
	for (;;)
	{
		pthread_testcancel();
		Delay(1L);
	}
	return (void *)0L;
}

static void *
worker_rdlock(void *arg)
{
	pthread_rwlock_t *rw;

	rw = (pthread_rwlock_t *)arg;
	if (pthread_rwlock_rdlock(rw) != 0)
		return (void *)1L;
	Delay(5L);
	if (pthread_rwlock_unlock(rw) != 0)
		return (void *)2L;
	return (void *)0L;
}

static void *
hold_worker(void *arg)
{
	(void)arg;
	/*
	 * No printf/fflush here.  Main is also writing the hold banner/dots to
	 * the same inherited Output(); concurrent dos.library Write on one CON:
	 * can corrupt stream metadata and trip panic 20 (forged FILE*).
	 */
	while (g_run)
	{
		if (sem_trywait(&g_sem) == 0)
			break;
		(void)GetSTDIO();
		Delay(10L);
	}
	return (void *)7L;
}

static void
hold_cleanup(void)
{
	void *rv;

	g_run = 0;
	if (g_sync_hold)
		(void)sem_post(&g_sem);

	if (g_hold_live)
	{
		(void)pthread_join(g_hold_th, &rv);
		g_hold_live = 0;
		printf((STRPTR)"cleanup: joined hold rv=%ld\n", (LONG)rv);
		fflush(stdout);
	}

	if (g_sync_hold)
	{
		(void)pthread_cond_destroy(&g_cv);
		(void)pthread_mutex_destroy(&g_mu);
		(void)sem_destroy(&g_sem);
		g_sync_hold = 0;
	}

	printf((STRPTR)"CCPthread cleanup done\n");
	fflush(stdout);
}

static void
suite_attr(void)
{
	pthread_attr_t attr;
	int state;
	size_t stack;
	int err;

	ct_suite_begin((STRPTR)"attr",
		(STRPTR)"pthread_attr_*");

	err = pthread_attr_init(&attr);
	ct_check_long((STRPTR)"attr", (STRPTR)"init",
		(STRPTR)"pthread_attr_init(&attr)",
		(STRPTR)"attr init must return 0",
		0L, (LONG)err);

	err = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	ct_check_long((STRPTR)"attr", (STRPTR)"setdetach",
		(STRPTR)"pthread_attr_setdetachstate(DETACHED)",
		(STRPTR)"setdetachstate must return 0",
		0L, (LONG)err);

	state = -1;
	err = pthread_attr_getdetachstate(&attr, &state);
	ct_check_true((STRPTR)"attr", (STRPTR)"getdetach",
		(STRPTR)"pthread_attr_getdetachstate",
		(STRPTR)"must report PTHREAD_CREATE_DETACHED",
		err == 0 && state == PTHREAD_CREATE_DETACHED);

	err = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	ct_check_long((STRPTR)"attr", (STRPTR)"setjoinable",
		(STRPTR)"pthread_attr_setdetachstate(JOINABLE)",
		(STRPTR)"setdetachstate joinable must return 0",
		0L, (LONG)err);

	err = pthread_attr_setstacksize(&attr, (size_t)16384);
	ct_check_long((STRPTR)"attr", (STRPTR)"setstack",
		(STRPTR)"pthread_attr_setstacksize(16384)",
		(STRPTR)"setstacksize must return 0",
		0L, (LONG)err);

	stack = 0;
	err = pthread_attr_getstacksize(&attr, &stack);
	ct_check_true((STRPTR)"attr", (STRPTR)"getstack",
		(STRPTR)"pthread_attr_getstacksize",
		(STRPTR)"must report 16384",
		err == 0 && stack == (size_t)16384);

	err = pthread_attr_destroy(&attr);
	ct_check_long((STRPTR)"attr", (STRPTR)"destroy",
		(STRPTR)"pthread_attr_destroy(&attr)",
		(STRPTR)"attr destroy must return 0",
		0L, (LONG)err);
}

static void
suite_self(void)
{
	pthread_t me;

	ct_suite_begin((STRPTR)"self",
		(STRPTR)"pthread_self / equal");

	me = pthread_self();
	ct_check_true((STRPTR)"self", (STRPTR)"nonzero",
		(STRPTR)"pthread_self()",
		(STRPTR)"main thread id must be non-zero",
		me != (pthread_t)0);
	ct_check_true((STRPTR)"self", (STRPTR)"equal_self",
		(STRPTR)"pthread_equal(self, self)",
		(STRPTR)"thread must equal itself",
		pthread_equal(me, me) != 0);
	ct_check_true((STRPTR)"self", (STRPTR)"equal_diff",
		(STRPTR)"pthread_equal(self, 0)",
		(STRPTR)"self must not equal id 0",
		pthread_equal(me, (pthread_t)0) == 0);
}

static void
suite_mutex(void)
{
	pthread_mutex_t mu;
	struct timespec ts;
	pthread_t th;
	void *rv;
	int err;
	int timed_err;

	ct_suite_begin((STRPTR)"mutex",
		(STRPTR)"mutex lock / try / timed / static init");

	err = pthread_mutex_init(&mu, (const pthread_mutexattr_t *)NULL);
	ct_check_long((STRPTR)"mutex", (STRPTR)"init",
		(STRPTR)"pthread_mutex_init(&mu, NULL)",
		(STRPTR)"mutex init must return 0",
		0L, (LONG)err);

	err = pthread_mutex_trylock(&mu);
	ct_check_long((STRPTR)"mutex", (STRPTR)"trylock_ok",
		(STRPTR)"pthread_mutex_trylock(&mu) unlocked",
		(STRPTR)"trylock on free mutex must return 0",
		0L, (LONG)err);

	err = pthread_mutex_trylock(&mu);
	ct_check_long((STRPTR)"mutex", (STRPTR)"trylock_busy",
		(STRPTR)"pthread_mutex_trylock(&mu) while held (NORMAL)",
		(STRPTR)"same-task trylock on NORMAL mutex must return EBUSY",
		(LONG)EBUSY, (LONG)err);

	err = pthread_mutex_unlock(&mu);
	ct_check_long((STRPTR)"mutex", (STRPTR)"unlock",
		(STRPTR)"pthread_mutex_unlock(&mu)",
		(STRPTR)"unlock must return 0",
		0L, (LONG)err);

	err = pthread_mutex_lock(&g_mu);
	ct_check_long((STRPTR)"mutex", (STRPTR)"static_lock",
		(STRPTR)"pthread_mutex_lock(&g_mu) PTHREAD_MUTEX_INITIALIZER",
		(STRPTR)"static initializer mutex must lock",
		0L, (LONG)err);

	timed_err = -1;
	err = pthread_create(&th, (const pthread_attr_t *)NULL,
		worker_try_timeout, (void *)&timed_err);
	ct_check_long((STRPTR)"mutex", (STRPTR)"timed_create",
		(STRPTR)"pthread_create(worker_try_timeout)",
		(STRPTR)"create must return 0",
		0L, (LONG)err);
	err = pthread_join(th, &rv);
	ct_check_long((STRPTR)"mutex", (STRPTR)"timed_join",
		(STRPTR)"pthread_join(timed worker)",
		(STRPTR)"join must return 0",
		0L, (LONG)err);
	ct_check_long((STRPTR)"mutex", (STRPTR)"timedlock_timeout",
		(STRPTR)"pthread_mutex_timedlock while held by main",
		(STRPTR)"worker timedlock must return ETIMEDOUT",
		(LONG)ETIMEDOUT, (LONG)timed_err);

	(void)pthread_mutex_unlock(&g_mu);

	abs_after(&ts, 2L);
	err = pthread_mutex_timedlock(&g_mu, &ts);
	ct_check_long((STRPTR)"mutex", (STRPTR)"timedlock_ok",
		(STRPTR)"pthread_mutex_timedlock free mutex",
		(STRPTR)"timedlock on free mutex must return 0",
		0L, (LONG)err);
	(void)pthread_mutex_unlock(&g_mu);

	err = pthread_mutex_destroy(&mu);
	ct_check_long((STRPTR)"mutex", (STRPTR)"destroy",
		(STRPTR)"pthread_mutex_destroy(&mu)",
		(STRPTR)"destroy unlocked mutex must return 0",
		0L, (LONG)err);
}

static void
suite_cond(void)
{
	pthread_t t1;
	pthread_t t2;
	void *rv;
	struct timespec ts;
	int err;

	ct_suite_begin((STRPTR)"cond",
		(STRPTR)"cond wait / broadcast / timedwait");

	g_flag = 0;
	g_ready = 0;
	g_wakes = 0;

	err = pthread_create(&t1, (const pthread_attr_t *)NULL,
		worker_bcast, (void *)NULL);
	ct_check_long((STRPTR)"cond", (STRPTR)"bcast_create1",
		(STRPTR)"pthread_create(worker_bcast) #1",
		(STRPTR)"create must return 0",
		0L, (LONG)err);
	err = pthread_create(&t2, (const pthread_attr_t *)NULL,
		worker_bcast, (void *)NULL);
	ct_check_long((STRPTR)"cond", (STRPTR)"bcast_create2",
		(STRPTR)"pthread_create(worker_bcast) #2",
		(STRPTR)"create must return 0",
		0L, (LONG)err);

	pthread_mutex_lock(&g_mu);
	while (g_ready < 2)
		pthread_cond_wait(&g_cv, &g_mu);
	g_flag = 1;
	err = pthread_cond_broadcast(&g_cv);
	ct_check_long((STRPTR)"cond", (STRPTR)"broadcast",
		(STRPTR)"pthread_cond_broadcast(&g_cv)",
		(STRPTR)"broadcast must return 0",
		0L, (LONG)err);
	pthread_mutex_unlock(&g_mu);

	err = pthread_join(t1, &rv);
	ct_check_long((STRPTR)"cond", (STRPTR)"bcast_join1",
		(STRPTR)"pthread_join(bcast #1)",
		(STRPTR)"join must return 0",
		0L, (LONG)err);
	err = pthread_join(t2, &rv);
	ct_check_long((STRPTR)"cond", (STRPTR)"bcast_join2",
		(STRPTR)"pthread_join(bcast #2)",
		(STRPTR)"join must return 0",
		0L, (LONG)err);
	ct_check_long((STRPTR)"cond", (STRPTR)"bcast_wakes",
		(STRPTR)"g_wakes after broadcast",
		(STRPTR)"both waiters must wake (wakes==2)",
		2L, (LONG)g_wakes);

	pthread_mutex_lock(&g_mu);
	abs_after(&ts, -1L);
	err = pthread_cond_timedwait(&g_cv, &g_mu, &ts);
	ct_check_long((STRPTR)"cond", (STRPTR)"timedwait_past",
		(STRPTR)"pthread_cond_timedwait past deadline",
		(STRPTR)"must return ETIMEDOUT",
		(LONG)ETIMEDOUT, (LONG)err);
	pthread_mutex_unlock(&g_mu);
}

static void
suite_create_join(void)
{
	pthread_t th;
	pthread_attr_t attr;
	void *rv;
	int err;
	int sval;

	ct_suite_begin((STRPTR)"create",
		(STRPTR)"create / join / exit / once / signal");

	g_once_count = 0;
	g_flag = 0;
	g_worker_equal_ok = 0;
	g_once.done = 0;
	g_once.sem_inited = 0;

	err = sem_init(&g_sem, 0, 0);
	ct_check_long((STRPTR)"create", (STRPTR)"sem_init",
		(STRPTR)"sem_init(&g_sem, 0, 0)",
		(STRPTR)"sem_init must return 0",
		0L, (LONG)err);

	err = pthread_create(&th, (const pthread_attr_t *)NULL,
		worker_basic, (void *)NULL);
	ct_check_long((STRPTR)"create", (STRPTR)"basic",
		(STRPTR)"pthread_create(worker_basic)",
		(STRPTR)"create must return 0",
		0L, (LONG)err);

	pthread_mutex_lock(&g_mu);
	while (g_flag == 0)
		pthread_cond_wait(&g_cv, &g_mu);
	pthread_mutex_unlock(&g_mu);
	ct_check_long((STRPTR)"create", (STRPTR)"cond_signal_wake",
		(STRPTR)"worker pthread_cond_signal",
		(STRPTR)"main must observe g_flag==1",
		1L, (LONG)g_flag);

	err = sem_post(&g_sem);
	ct_check_long((STRPTR)"create", (STRPTR)"sem_post",
		(STRPTR)"sem_post(&g_sem)",
		(STRPTR)"sem_post must return 0",
		0L, (LONG)err);

	err = pthread_join(th, &rv);
	ct_check_true((STRPTR)"create", (STRPTR)"join_basic",
		(STRPTR)"pthread_join(worker_basic)",
		(STRPTR)"join must return 0 with retval 42",
		err == 0 && rv == (void *)42L);
	ct_check_long((STRPTR)"create", (STRPTR)"once",
		(STRPTR)"pthread_once x2 in worker",
		(STRPTR)"init_routine must run exactly once",
		1L, (LONG)g_once_count);
	ct_check_true((STRPTR)"create", (STRPTR)"equal_in_worker",
		(STRPTR)"pthread_equal in worker",
		(STRPTR)"worker self must equal self",
		g_worker_equal_ok != 0);

	err = sem_getvalue(&g_sem, &sval);
	ct_check_true((STRPTR)"create", (STRPTR)"sem_getvalue",
		(STRPTR)"sem_getvalue after wait/post",
		(STRPTR)"count must be 0",
		err == 0 && sval == 0);

	err = pthread_create(&th, (const pthread_attr_t *)NULL,
		worker_exit, (void *)NULL);
	ct_check_long((STRPTR)"create", (STRPTR)"exit_create",
		(STRPTR)"pthread_create(worker_exit)",
		(STRPTR)"create must return 0",
		0L, (LONG)err);
	err = pthread_join(th, &rv);
	ct_check_true((STRPTR)"create", (STRPTR)"join_exit",
		(STRPTR)"pthread_join after pthread_exit(99)",
		(STRPTR)"join retval must be 99",
		err == 0 && rv == (void *)99L);

	err = pthread_attr_init(&attr);
	ct_check_long((STRPTR)"create", (STRPTR)"attr_init",
		(STRPTR)"pthread_attr_init for create",
		(STRPTR)"attr init must return 0",
		0L, (LONG)err);
	(void)pthread_attr_setstacksize(&attr, (size_t)12288);
	(void)pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	err = pthread_create(&th, &attr, worker_exit, (void *)NULL);
	ct_check_long((STRPTR)"create", (STRPTR)"with_attr",
		(STRPTR)"pthread_create(..., &attr, worker_exit)",
		(STRPTR)"create with stack/detach attrs must return 0",
		0L, (LONG)err);
	(void)pthread_attr_destroy(&attr);
	err = pthread_join(th, &rv);
	ct_check_true((STRPTR)"create", (STRPTR)"join_with_attr",
		(STRPTR)"pthread_join attr worker",
		(STRPTR)"join retval must be 99",
		err == 0 && rv == (void *)99L);
}

static void
suite_detach(void)
{
	pthread_t th;
	pthread_attr_t attr;
	sem_t done;
	int err;
	struct timespec ts;

	ct_suite_begin((STRPTR)"detach",
		(STRPTR)"pthread_detach / DETACHED create");

	err = sem_init(&done, 0, 0);
	ct_check_long((STRPTR)"detach", (STRPTR)"sem_init",
		(STRPTR)"sem_init(&done, 0, 0)",
		(STRPTR)"sem_init must return 0",
		0L, (LONG)err);

	err = pthread_create(&th, (const pthread_attr_t *)NULL,
		worker_detach_done, (void *)&done);
	ct_check_long((STRPTR)"detach", (STRPTR)"create",
		(STRPTR)"pthread_create(detach worker)",
		(STRPTR)"create must return 0",
		0L, (LONG)err);
	err = pthread_detach(th);
	ct_check_long((STRPTR)"detach", (STRPTR)"detach",
		(STRPTR)"pthread_detach(th)",
		(STRPTR)"detach must return 0",
		0L, (LONG)err);
	abs_after(&ts, 5L);
	err = sem_timedwait(&done, &ts);
	ct_check_long((STRPTR)"detach", (STRPTR)"finished",
		(STRPTR)"sem_timedwait(done) after detach",
		(STRPTR)"worker must post done (return 0)",
		0L, (LONG)err);
	/* Worker posts before ClearSTDIO; wait for teardown to finish. */
	Delay(50L);

	err = pthread_attr_init(&attr);
	(void)pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	err = pthread_create(&th, &attr, worker_detach_done, (void *)&done);
	ct_check_long((STRPTR)"detach", (STRPTR)"attr_create",
		(STRPTR)"pthread_create DETACHED attr",
		(STRPTR)"detached create must return 0",
		0L, (LONG)err);
	(void)pthread_attr_destroy(&attr);
	abs_after(&ts, 5L);
	err = sem_timedwait(&done, &ts);
	ct_check_long((STRPTR)"detach", (STRPTR)"attr_finished",
		(STRPTR)"sem_timedwait(done) detached create",
		(STRPTR)"detached worker must post done",
		0L, (LONG)err);
	Delay(50L);

	(void)sem_destroy(&done);
}

static void
suite_tls(void)
{
	pthread_key_t key;
	pthread_t th;
	void *rv;
	int err;

	ct_suite_begin((STRPTR)"tls",
		(STRPTR)"pthread_key / get/setspecific / dtor");

	g_tls_dtor_hits = 0;
	err = pthread_key_create(&key, tls_dtor);
	ct_check_long((STRPTR)"tls", (STRPTR)"key_create",
		(STRPTR)"pthread_key_create(&key, tls_dtor)",
		(STRPTR)"key_create must return 0",
		0L, (LONG)err);

	err = pthread_setspecific(key, (const void *)0xABCDL);
	ct_check_long((STRPTR)"tls", (STRPTR)"set_main",
		(STRPTR)"pthread_setspecific on main",
		(STRPTR)"setspecific must return 0",
		0L, (LONG)err);
	ct_check_true((STRPTR)"tls", (STRPTR)"get_main",
		(STRPTR)"pthread_getspecific on main",
		(STRPTR)"must return the value set",
		pthread_getspecific(key) == (void *)0xABCDL);

	err = pthread_create(&th, (const pthread_attr_t *)NULL,
		worker_tls, (void *)&key);
	ct_check_long((STRPTR)"tls", (STRPTR)"create",
		(STRPTR)"pthread_create(worker_tls)",
		(STRPTR)"create must return 0",
		0L, (LONG)err);
	err = pthread_join(th, &rv);
	ct_check_true((STRPTR)"tls", (STRPTR)"join",
		(STRPTR)"pthread_join(worker_tls)",
		(STRPTR)"worker must set/get key (rv==0)",
		err == 0 && rv == (void *)0L);
	ct_check_long((STRPTR)"tls", (STRPTR)"dtor",
		(STRPTR)"TLS destructor after worker ClearSTDIO",
		(STRPTR)"destructor must run once for worker value",
		1L, (LONG)g_tls_dtor_hits);

	/* Main still has its value; delete key without running main dtor. */
	err = pthread_key_delete(key);
	ct_check_long((STRPTR)"tls", (STRPTR)"key_delete",
		(STRPTR)"pthread_key_delete(key)",
		(STRPTR)"key_delete must return 0",
		0L, (LONG)err);
}

static void
suite_rwlock(void)
{
	pthread_rwlock_t rw;
	pthread_t t1;
	pthread_t t2;
	void *rv;
	int err;

	ct_suite_begin((STRPTR)"rwlock",
		(STRPTR)"pthread_rwlock_*");

	err = pthread_rwlock_init(&rw, (const pthread_rwlockattr_t *)NULL);
	ct_check_long((STRPTR)"rwlock", (STRPTR)"init",
		(STRPTR)"pthread_rwlock_init(&rw, NULL)",
		(STRPTR)"rwlock init must return 0",
		0L, (LONG)err);

	err = pthread_rwlock_wrlock(&rw);
	ct_check_long((STRPTR)"rwlock", (STRPTR)"wrlock",
		(STRPTR)"pthread_rwlock_wrlock",
		(STRPTR)"wrlock must return 0",
		0L, (LONG)err);
	err = pthread_rwlock_tryrdlock(&rw);
	ct_check_long((STRPTR)"rwlock", (STRPTR)"tryrd_busy",
		(STRPTR)"pthread_rwlock_tryrdlock while write-held",
		(STRPTR)"must return EBUSY",
		(LONG)EBUSY, (LONG)err);
	err = pthread_rwlock_unlock(&rw);
	ct_check_long((STRPTR)"rwlock", (STRPTR)"wrunlock",
		(STRPTR)"pthread_rwlock_unlock writer",
		(STRPTR)"unlock must return 0",
		0L, (LONG)err);

	err = pthread_create(&t1, (const pthread_attr_t *)NULL,
		worker_rdlock, (void *)&rw);
	ct_check_long((STRPTR)"rwlock", (STRPTR)"rd_create1",
		(STRPTR)"pthread_create rdlock #1",
		(STRPTR)"create must return 0",
		0L, (LONG)err);
	err = pthread_create(&t2, (const pthread_attr_t *)NULL,
		worker_rdlock, (void *)&rw);
	ct_check_long((STRPTR)"rwlock", (STRPTR)"rd_create2",
		(STRPTR)"pthread_create rdlock #2",
		(STRPTR)"create must return 0",
		0L, (LONG)err);
	err = pthread_join(t1, &rv);
	ct_check_true((STRPTR)"rwlock", (STRPTR)"rd_join1",
		(STRPTR)"pthread_join rdlock #1",
		(STRPTR)"shared rdlock must succeed",
		err == 0 && rv == (void *)0L);
	err = pthread_join(t2, &rv);
	ct_check_true((STRPTR)"rwlock", (STRPTR)"rd_join2",
		(STRPTR)"pthread_join rdlock #2",
		(STRPTR)"shared rdlock must succeed",
		err == 0 && rv == (void *)0L);

	err = pthread_rwlock_destroy(&rw);
	ct_check_long((STRPTR)"rwlock", (STRPTR)"destroy",
		(STRPTR)"pthread_rwlock_destroy",
		(STRPTR)"destroy must return 0",
		0L, (LONG)err);
}

static void
suite_cancel(void)
{
	pthread_t th;
	void *rv;
	int err;

	ct_suite_begin((STRPTR)"cancel",
		(STRPTR)"cooperative pthread_cancel");

	err = pthread_create(&th, (const pthread_attr_t *)NULL,
		worker_cancel, (void *)NULL);
	ct_check_long((STRPTR)"cancel", (STRPTR)"create",
		(STRPTR)"pthread_create(worker_cancel)",
		(STRPTR)"create must return 0",
		0L, (LONG)err);

	Delay(5L);
	err = pthread_cancel(th);
	ct_check_long((STRPTR)"cancel", (STRPTR)"cancel",
		(STRPTR)"pthread_cancel(th)",
		(STRPTR)"cancel must return 0",
		0L, (LONG)err);

	err = pthread_join(th, &rv);
	ct_check_true((STRPTR)"cancel", (STRPTR)"join_canceled",
		(STRPTR)"pthread_join after cancel",
		(STRPTR)"retval must be PTHREAD_CANCELED",
		err == 0 && rv == PTHREAD_CANCELED);
}

static void
suite_sem(void)
{
	sem_t sem;
	struct timespec ts;
	int err;
	int sval;

	ct_suite_begin((STRPTR)"sem",
		(STRPTR)"unnamed sem_* extras + destroy fixtures");

	errno = 0;
	err = sem_init(&sem, 1, 0);
	ct_check_true((STRPTR)"sem", (STRPTR)"pshared_enosys",
		(STRPTR)"sem_init(pshared=1)",
		(STRPTR)"process-shared must fail with ENOSYS",
		err == -1 && errno == ENOSYS);

	err = sem_init(&sem, 0, 1);
	ct_check_long((STRPTR)"sem", (STRPTR)"init1",
		(STRPTR)"sem_init(&sem, 0, 1)",
		(STRPTR)"sem_init must return 0",
		0L, (LONG)err);

	err = sem_trywait(&sem);
	ct_check_long((STRPTR)"sem", (STRPTR)"trywait_ok",
		(STRPTR)"sem_trywait value 1",
		(STRPTR)"trywait must return 0",
		0L, (LONG)err);

	errno = 0;
	err = sem_trywait(&sem);
	ct_check_true((STRPTR)"sem", (STRPTR)"trywait_again",
		(STRPTR)"sem_trywait value 0",
		(STRPTR)"must fail with EAGAIN",
		err == -1 && errno == EAGAIN);

	errno = 0;
	abs_after(&ts, -1L);
	err = sem_timedwait(&sem, &ts);
	ct_check_true((STRPTR)"sem", (STRPTR)"timedwait_past",
		(STRPTR)"sem_timedwait past deadline",
		(STRPTR)"must fail with ETIMEDOUT",
		err == -1 && errno == ETIMEDOUT);

	err = sem_post(&sem);
	ct_check_long((STRPTR)"sem", (STRPTR)"post",
		(STRPTR)"sem_post(&sem)",
		(STRPTR)"post must return 0",
		0L, (LONG)err);

	abs_after(&ts, 2L);
	err = sem_timedwait(&sem, &ts);
	ct_check_long((STRPTR)"sem", (STRPTR)"timedwait_ok",
		(STRPTR)"sem_timedwait with value available",
		(STRPTR)"timedwait must return 0",
		0L, (LONG)err);

	err = sem_getvalue(&sem, &sval);
	ct_check_true((STRPTR)"sem", (STRPTR)"getvalue0",
		(STRPTR)"sem_getvalue",
		(STRPTR)"count must be 0",
		err == 0 && sval == 0);

	err = sem_destroy(&sem);
	ct_check_long((STRPTR)"sem", (STRPTR)"destroy",
		(STRPTR)"sem_destroy(&sem)",
		(STRPTR)"destroy must return 0",
		0L, (LONG)err);

	err = pthread_cond_destroy(&g_cv);
	ct_check_long((STRPTR)"sem", (STRPTR)"cond_destroy",
		(STRPTR)"pthread_cond_destroy(&g_cv)",
		(STRPTR)"destroy static cond must return 0",
		0L, (LONG)err);
	err = pthread_mutex_destroy(&g_mu);
	ct_check_long((STRPTR)"sem", (STRPTR)"mutex_destroy_static",
		(STRPTR)"pthread_mutex_destroy(&g_mu)",
		(STRPTR)"destroy static mutex must return 0",
		0L, (LONG)err);
	err = sem_destroy(&g_sem);
	ct_check_long((STRPTR)"sem", (STRPTR)"sem_destroy_g",
		(STRPTR)"sem_destroy(&g_sem)",
		(STRPTR)"destroy g_sem must return 0",
		0L, (LONG)err);
}

static int
run_hold(void)
{
	int err;

	if (atexit(hold_cleanup) != 0)
	{
		printf((STRPTR)"atexit failed\n");
		fflush(stdout);
		return 20;
	}

	(void)pthread_mutex_init(&g_mu, (const pthread_mutexattr_t *)NULL);
	(void)pthread_cond_init(&g_cv, (const pthread_condattr_t *)NULL);
	(void)sem_init(&g_sem, 0, 0);
	g_sync_hold = 1;

	/*
	 * Banner before create so only main touches the console during hold.
	 * Hold worker stays silent (shared Output is not dos-reentrant).
	 */
	printf((STRPTR)"\n# holding -- press Ctrl-C to exit cleanly\n");
	fflush(stdout);

	g_run = 1;
	err = pthread_create(&g_hold_th, (const pthread_attr_t *)NULL,
		hold_worker, (void *)NULL);
	if (err != 0)
	{
		printf((STRPTR)"hold create %d\n", err);
		fflush(stdout);
		return 20;
	}
	g_hold_live = 1;

	for (;;)
	{
		(void)GetSTDIO();
		printf((STRPTR)".");
		fflush(stdout);
		Delay(10L);
	}
	return 0;
}

int
main(int argc, char **argv)
{
	int nohold;

	nohold = 0;
	if (argc > 1 && argv[1] != (char *)NULL
	    && strcmp(argv[1], (char *)"nohold") == 0)
		nohold = 1;

	ct_init_named((STRPTR)"CCPthread");

	suite_attr();
	suite_self();
	suite_mutex();
	suite_cond();
	suite_create_join();
	suite_detach();
	suite_tls();
	suite_rwlock();
	suite_cancel();
	suite_sem();

	ct_summary();

	if (ct_fail != 0L)
		return 20;

	if (nohold)
		return 0;
	return run_hold();
}
