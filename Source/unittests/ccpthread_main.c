/*
 * ccpthread_main.c - smoke test + Ctrl-C hold for ccpthread.lib
 *
 * Workers must not touch CRT globals stdin/stdout/stderr — those belong to
 * the parent session.  Use printf/puts (session stdout_p) only; no
 * fflush(stdout).
 *
 * Ctrl-C on main → CRT exit → atexit wakes/joins the hold worker.
 */

#include <exec/types.h>
#include <proto/dos.h>
#include <proto/cclib.h>
#include <libraries/cclib.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <errno.h>

static pthread_mutex_t g_mu;
static pthread_cond_t g_cv;
static sem_t g_sem;
static int g_flag;
static int g_once_count;
static pthread_once_t g_once = PTHREAD_ONCE_INIT;

static volatile int g_run;
static int g_sync_live;
static int g_hold_live;
static pthread_t g_hold_th;

extern int atexit(void (*func)(void));

static void
once_fn(void)
{
	g_once_count++;
}

static void
ccpthread_cleanup(void)
{
	void *rv;

	g_run = 0;
	if (g_sync_live)
		(void)sem_post(&g_sem);

	if (g_hold_live)
	{
		(void)pthread_join(g_hold_th, &rv);
		g_hold_live = 0;
		printf((STRPTR)"cleanup: joined hold rv=%ld\n", (LONG)rv);
	}

	if (g_sync_live)
	{
		(void)pthread_cond_destroy(&g_cv);
		(void)pthread_mutex_destroy(&g_mu);
		(void)sem_destroy(&g_sem);
		g_sync_live = 0;
	}

	printf((STRPTR)"CCPthread cleanup done\n");
}

static void *
worker(void *arg)
{
	LONG n;

	(void)arg;
	n = (LONG)pthread_self();
	/* printf uses session stdout_p — never fflush(stdout) here. */
	printf((STRPTR)"worker self=%ld\n", n);

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
hold_worker(void *arg)
{
	(void)arg;
	printf((STRPTR)"hold worker up\n");

	while (g_run)
	{
		if (sem_trywait(&g_sem) == 0)
			break;
		(void)GetSTDIO();
		Delay(10L);
	}

	printf((STRPTR)"hold worker exit\n");
	return (void *)7L;
}

int
main(int argc, char **argv)
{
	pthread_t th;
	void *rv;
	int err;
	int sval;
	ULONG ticks;

	(void)argc;
	(void)argv;

	printf((STRPTR)"CCPthread smoke\n");
	fflush(stdout);
	printf((STRPTR)"main self=%ld\n", (LONG)pthread_self());
	fflush(stdout);

	err = pthread_mutex_init(&g_mu, (const pthread_mutexattr_t *)NULL);
	if (err != 0)
	{
		printf((STRPTR)"mutex_init %d\n", err);
		return 20;
	}
	printf((STRPTR)"mutex ok\n");
	fflush(stdout);

	err = pthread_cond_init(&g_cv, (const pthread_condattr_t *)NULL);
	if (err != 0)
	{
		printf((STRPTR)"cond_init %d\n", err);
		return 20;
	}
	printf((STRPTR)"cond ok\n");
	fflush(stdout);

	if (sem_init(&g_sem, 0, 0) != 0)
	{
		printf((STRPTR)"sem_init errno=%d\n", errno);
		return 20;
	}
	g_sync_live = 1;
	printf((STRPTR)"inits ok, creating\n");
	fflush(stdout);

	err = pthread_create(&th, (const pthread_attr_t *)NULL, worker, (void *)NULL);
	if (err != 0)
	{
		printf((STRPTR)"create %d\n", err);
		return 20;
	}
	printf((STRPTR)"created\n");
	fflush(stdout);

	printf((STRPTR)"waiting on cond\n");
	fflush(stdout);
	pthread_mutex_lock(&g_mu);
	while (g_flag == 0)
		pthread_cond_wait(&g_cv, &g_mu);
	pthread_mutex_unlock(&g_mu);
	printf((STRPTR)"cond woken flag=%d\n", g_flag);
	fflush(stdout);

	sem_post(&g_sem);
	err = pthread_join(th, &rv);
	if (err != 0)
	{
		printf((STRPTR)"join %d\n", err);
		return 20;
	}

	sem_getvalue(&g_sem, &sval);
	printf((STRPTR)"join rv=%ld once=%d sem=%d\n",
		(LONG)rv, g_once_count, sval);
	fflush(stdout);

	if (rv != (void *)42L || g_once_count != 1)
		return 20;
	printf((STRPTR)"CCPthread smoke OK\n");
	fflush(stdout);

	if (atexit(ccpthread_cleanup) != 0)
	{
		printf((STRPTR)"atexit failed\n");
		return 20;
	}

	g_run = 1;
	while (sem_trywait(&g_sem) == 0)
		;

	err = pthread_create(&g_hold_th, (const pthread_attr_t *)NULL,
		hold_worker, (void *)NULL);
	if (err != 0)
	{
		printf((STRPTR)"hold create %d\n", err);
		return 20;
	}
	g_hold_live = 1;

	printf((STRPTR)"holding — press Ctrl-C to exit cleanly\n");
	fflush(stdout);

	ticks = 0UL;
	for (;;)
	{
		(void)GetSTDIO();
		printf((STRPTR)".");
		if ((ticks % 40UL) == 39UL)
			printf((STRPTR)"\n");
		fflush(stdout);
		Delay(5L);
		ticks++;
	}

	return 0;
}
