/*
 * pthread_sync.c - mutex, condition variables, pthread_once
 */

#include <exec/types.h>
#include <exec/lists.h>
#include <exec/semaphores.h>
#include <exec/tasks.h>
#include <exec/memory.h>
#include <devices/timer.h>
#include <dos/dos.h>
#include <dos/dosextens.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/timer.h>
#include <proto/utility.h>
#include <proto/cclib.h>
#include <errno.h>
#include <time.h>
#include <pthread.h>

#include <private/cc_internal.h>
#include "cc_pthread.h"

typedef struct
{
	struct Node node;
	struct Task *task;
	ULONG sigmask;
} cc_CondWaiter;

/*
 * PTHREAD_MUTEX_INITIALIZER is zeroed BSS.  InitSemaphore sets
 * ss_QueueCount to -1; a still-zero QueueCount means never inited.
 */
static void
cc_mutex_ready(pthread_mutex_t *mutex)
{
	if (mutex->semaphore.ss_QueueCount == 0
	    && mutex->semaphore.ss_Owner == (struct Task *)NULL)
	{
		Forbid();
		if (mutex->semaphore.ss_QueueCount == 0
		    && mutex->semaphore.ss_Owner == (struct Task *)NULL)
			InitSemaphore(&mutex->semaphore);
		Permit();
	}
}

/* PTHREAD_COND_INITIALIZER: empty list head is NULL until NewList. */
static void
cc_cond_ready(pthread_cond_t *cond)
{
	if (cond->waiters.lh_Head == (struct Node *)NULL)
	{
		Forbid();
		if (cond->waiters.lh_Head == (struct Node *)NULL)
		{
			InitSemaphore(&cond->semaphore);
			cc_thr_newlist(&cond->waiters);
		}
		Permit();
	}
}

/*
 * Wait until sigmask or absolute timeout.  Returns 0, ETIMEDOUT, or EAGAIN.
 */
int
cc_thr_wait_timeout(ULONG sigmask, const struct timespec *abstime)
{
	struct MsgPort *port;
	struct timerequest *tr;
	ULONG timsig;
	ULONG got;
	LONG secs;
	LONG micros;
	time_t now;

	if (abstime == (const struct timespec *)NULL)
	{
		cc_thr_wait_or_break(sigmask);
		return 0;
	}

	now = time((time_t *)NULL);
	if ((time_t)abstime->tv_sec < now
	    || ((time_t)abstime->tv_sec == now && abstime->tv_nsec <= 0L))
		return ETIMEDOUT;

	secs = (LONG)((time_t)abstime->tv_sec - now);
	/* nsec -> usec via utility.library (avoid app-link __CXD33). */
	{
		LONG nsec;

		nsec = (LONG)abstime->tv_nsec;
		if (nsec < 0L)
			nsec = 0L;
		if (nsec > 999999999L)
			nsec = 999999999L;
		micros = SDivMod32(nsec, 1000L);
	}

	port = CreateMsgPort();
	if (port == (struct MsgPort *)NULL)
		return EAGAIN;
	tr = (struct timerequest *)CreateIORequest(port, sizeof(*tr));
	if (tr == (struct timerequest *)NULL)
	{
		DeleteMsgPort(port);
		return EAGAIN;
	}
	if (OpenDevice((STRPTR)TIMERNAME, UNIT_VBLANK, (struct IORequest *)tr, 0UL)
	    != 0L)
	{
		DeleteIORequest((struct IORequest *)tr);
		DeleteMsgPort(port);
		return EAGAIN;
	}

	tr->tr_node.io_Command = TR_ADDREQUEST;
	tr->tr_time.tv_secs = secs;
	tr->tr_time.tv_micro = micros;
	SendIO((struct IORequest *)tr);

	timsig = 1UL << port->mp_SigBit;
	for (;;)
	{
		got = Wait(sigmask | timsig | SIGBREAKF_CTRL_C);
		if ((got & SIGBREAKF_CTRL_C) != 0UL)
		{
			SetSignal(0L, SIGBREAKF_CTRL_C);
			if (!CheckIO((struct IORequest *)tr))
				AbortIO((struct IORequest *)tr);
			WaitIO((struct IORequest *)tr);
			CloseDevice((struct IORequest *)tr);
			DeleteIORequest((struct IORequest *)tr);
			DeleteMsgPort(port);
			cc_thr_check_break();
			return EINTR;
		}
		if ((got & timsig) != 0UL && (got & sigmask) == 0UL)
		{
			/* Timeout won. */
			if (!CheckIO((struct IORequest *)tr))
				AbortIO((struct IORequest *)tr);
			WaitIO((struct IORequest *)tr);
			CloseDevice((struct IORequest *)tr);
			DeleteIORequest((struct IORequest *)tr);
			DeleteMsgPort(port);
			return ETIMEDOUT;
		}
		if ((got & sigmask) != 0UL)
			break;
	}

	if (!CheckIO((struct IORequest *)tr))
		AbortIO((struct IORequest *)tr);
	WaitIO((struct IORequest *)tr);
	CloseDevice((struct IORequest *)tr);
	DeleteIORequest((struct IORequest *)tr);
	DeleteMsgPort(port);
	return 0;
}

int
pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr)
{
	if (mutex == (pthread_mutex_t *)NULL)
		return EINVAL;
	InitSemaphore(&mutex->semaphore);
	mutex->kind = PTHREAD_MUTEX_DEFAULT;
	if (attr != (const pthread_mutexattr_t *)NULL)
		mutex->kind = attr->kind;
	mutex->waiters = 0;
	return 0;
}

int
pthread_mutex_destroy(pthread_mutex_t *mutex)
{
	if (mutex == (pthread_mutex_t *)NULL)
		return EINVAL;
	cc_mutex_ready(mutex);
	if (mutex->waiters != 0
	    || mutex->semaphore.ss_Owner != (struct Task *)NULL)
		return EBUSY;
	return 0;
}

int
pthread_mutex_lock(pthread_mutex_t *mutex)
{
	struct Task *me;

	if (mutex == (pthread_mutex_t *)NULL)
		return EINVAL;
	cc_mutex_ready(mutex);
	pthread_testcancel();
	/*
	 * Exec SignalSemaphores nest.  POSIX NORMAL must not: same-task
	 * relock is EDEADLK (RECURSIVE uses nest count as-is).
	 */
	me = FindTask((STRPTR)NULL);
	if (mutex->kind != PTHREAD_MUTEX_RECURSIVE
	    && mutex->semaphore.ss_Owner == me
	    && mutex->semaphore.ss_NestCount > 0)
		return EDEADLK;
	/*
	 * Poll AttemptSemaphore so Ctrl-C (cc_thr_check_break) can force abort
	 * while blocked — ObtainSemaphore alone ignores breaks.
	 */
	while (!AttemptSemaphore(&mutex->semaphore))
	{
		cc_thr_check_break();
		Delay(1L);
	}
	return 0;
}

int
pthread_mutex_trylock(pthread_mutex_t *mutex)
{
	struct Task *me;

	if (mutex == (pthread_mutex_t *)NULL)
		return EINVAL;
	cc_mutex_ready(mutex);
	me = FindTask((STRPTR)NULL);
	if (mutex->kind != PTHREAD_MUTEX_RECURSIVE
	    && mutex->semaphore.ss_Owner == me
	    && mutex->semaphore.ss_NestCount > 0)
		return EBUSY;
	if (AttemptSemaphore(&mutex->semaphore))
		return 0;
	return EBUSY;
}

int
pthread_mutex_timedlock(pthread_mutex_t *mutex,
	const struct timespec *abstime)
{
	BYTE sig;
	ULONG mask;
	int err;
	struct Task *me;

	if (mutex == (pthread_mutex_t *)NULL
	    || abstime == (const struct timespec *)NULL)
		return EINVAL;
	cc_mutex_ready(mutex);
	me = FindTask((STRPTR)NULL);
	if (mutex->kind != PTHREAD_MUTEX_RECURSIVE
	    && mutex->semaphore.ss_Owner == me
	    && mutex->semaphore.ss_NestCount > 0)
		return EDEADLK;
	if (AttemptSemaphore(&mutex->semaphore))
		return 0;

	/*
	 * Poll AttemptSemaphore against timer.  Not as fair as a waiter
	 * queue but adequate for the first ccpthread cut.
	 */
	sig = AllocSignal(-1L);
	if (sig < 0)
		return EAGAIN;
	mask = 1UL << sig;
	for (;;)
	{
		if (AttemptSemaphore(&mutex->semaphore))
		{
			FreeSignal(sig);
			return 0;
		}
		err = cc_thr_wait_timeout(mask, abstime);
		if (err == ETIMEDOUT)
		{
			FreeSignal(sig);
			return ETIMEDOUT;
		}
		if (err != 0)
		{
			FreeSignal(sig);
			return err;
		}
		/* Spurious / poll slice: re-check Attempt. */
	}
}

int
pthread_mutex_unlock(pthread_mutex_t *mutex)
{
	if (mutex == (pthread_mutex_t *)NULL)
		return EINVAL;
	cc_mutex_ready(mutex);
	ReleaseSemaphore(&mutex->semaphore);
	return 0;
}

int
pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr)
{
	(void)attr;
	if (cond == (pthread_cond_t *)NULL)
		return EINVAL;
	InitSemaphore(&cond->semaphore);
	cc_thr_newlist(&cond->waiters);
	return 0;
}

int
pthread_cond_destroy(pthread_cond_t *cond)
{
	if (cond == (pthread_cond_t *)NULL)
		return EINVAL;
	cc_cond_ready(cond);
	ObtainSemaphore(&cond->semaphore);
	if (cond->waiters.lh_Head->ln_Succ != (struct Node *)NULL)
	{
		ReleaseSemaphore(&cond->semaphore);
		return EBUSY;
	}
	ReleaseSemaphore(&cond->semaphore);
	return 0;
}

int
pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex)
{
	cc_CondWaiter w;
	struct Process *proc;
	ULONG mask;
	BYTE sig;

	if (cond == (pthread_cond_t *)NULL || mutex == (pthread_mutex_t *)NULL)
		return EINVAL;
	cc_cond_ready(cond);
	cc_mutex_ready(mutex);
	pthread_testcancel();

	/*
	 * Private AllocSignal — not pr_MsgPort (shared with DOS / create
	 * handshake).  Matches unix.lib2 timed-wait practice.
	 */
	sig = AllocSignal(-1L);
	if (sig < 0)
		return EAGAIN;

	proc = (struct Process *)FindTask((STRPTR)NULL);
	mask = 1UL << sig;
	w.task = (struct Task *)proc;
	w.sigmask = mask;
	w.node.ln_Succ = (struct Node *)NULL;
	w.node.ln_Pred = (struct Node *)NULL;

	ObtainSemaphore(&cond->semaphore);
	AddTail(&cond->waiters, &w.node);
	mutex->waiters++;
	ReleaseSemaphore(&cond->semaphore);

	ReleaseSemaphore(&mutex->semaphore);
	cc_thr_wait_or_break(mask);

	ObtainSemaphore(&cond->semaphore);
	if (w.node.ln_Succ != (struct Node *)NULL)
		Remove(&w.node);
	ReleaseSemaphore(&cond->semaphore);

	ObtainSemaphore(&mutex->semaphore);
	mutex->waiters--;
	FreeSignal(sig);
	return 0;
}

int
pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex,
	const struct timespec *abstime)
{
	cc_CondWaiter w;
	struct Process *proc;
	ULONG mask;
	int err;
	BYTE sig;

	if (cond == (pthread_cond_t *)NULL || mutex == (pthread_mutex_t *)NULL)
		return EINVAL;
	cc_cond_ready(cond);
	cc_mutex_ready(mutex);
	pthread_testcancel();

	sig = AllocSignal(-1L);
	if (sig < 0)
		return EAGAIN;

	proc = (struct Process *)FindTask((STRPTR)NULL);
	mask = 1UL << sig;
	w.task = (struct Task *)proc;
	w.sigmask = mask;
	w.node.ln_Succ = (struct Node *)NULL;
	w.node.ln_Pred = (struct Node *)NULL;

	ObtainSemaphore(&cond->semaphore);
	AddTail(&cond->waiters, &w.node);
	mutex->waiters++;
	ReleaseSemaphore(&cond->semaphore);

	ReleaseSemaphore(&mutex->semaphore);
	err = cc_thr_wait_timeout(mask, abstime);

	ObtainSemaphore(&cond->semaphore);
	if (w.node.ln_Succ != (struct Node *)NULL)
		Remove(&w.node);
	ReleaseSemaphore(&cond->semaphore);

	ObtainSemaphore(&mutex->semaphore);
	mutex->waiters--;
	FreeSignal(sig);
	return err;
}

int
pthread_cond_signal(pthread_cond_t *cond)
{
	cc_CondWaiter *w;

	if (cond == (pthread_cond_t *)NULL)
		return EINVAL;
	cc_cond_ready(cond);
	ObtainSemaphore(&cond->semaphore);
	w = (cc_CondWaiter *)RemHead(&cond->waiters);
	if (w != (cc_CondWaiter *)NULL)
	{
		w->node.ln_Succ = (struct Node *)NULL;
		w->node.ln_Pred = (struct Node *)NULL;
		Signal(w->task, w->sigmask);
	}
	ReleaseSemaphore(&cond->semaphore);
	return 0;
}

int
pthread_cond_broadcast(pthread_cond_t *cond)
{
	cc_CondWaiter *w;

	if (cond == (pthread_cond_t *)NULL)
		return EINVAL;
	cc_cond_ready(cond);
	ObtainSemaphore(&cond->semaphore);
	for (;;)
	{
		w = (cc_CondWaiter *)RemHead(&cond->waiters);
		if (w == (cc_CondWaiter *)NULL)
			break;
		w->node.ln_Succ = (struct Node *)NULL;
		w->node.ln_Pred = (struct Node *)NULL;
		Signal(w->task, w->sigmask);
	}
	ReleaseSemaphore(&cond->semaphore);
	return 0;
}

/*
 * Read-write locks — ported from unix.lib2 pthread_sync.c (mutex + cond).
 */
int
pthread_rwlock_init(pthread_rwlock_t *rwlock,
	const pthread_rwlockattr_t *attr)
{
	int err;

	(void)attr;
	if (rwlock == (pthread_rwlock_t *)NULL)
		return EINVAL;
	err = pthread_mutex_init(&rwlock->lock, (const pthread_mutexattr_t *)NULL);
	if (err != 0)
		return err;
	err = pthread_cond_init(&rwlock->readers_cond,
		(const pthread_condattr_t *)NULL);
	if (err != 0)
		return err;
	err = pthread_cond_init(&rwlock->writers_cond,
		(const pthread_condattr_t *)NULL);
	if (err != 0)
		return err;
	rwlock->readers_active = 0;
	rwlock->writers_active = 0;
	rwlock->writers_waiting = 0;
	return 0;
}

int
pthread_rwlock_destroy(pthread_rwlock_t *rwlock)
{
	int err;

	if (rwlock == (pthread_rwlock_t *)NULL)
		return EINVAL;
	if (rwlock->readers_active != 0 || rwlock->writers_active != 0
	    || rwlock->writers_waiting != 0)
		return EBUSY;
	err = pthread_cond_destroy(&rwlock->readers_cond);
	if (err != 0)
		return err;
	err = pthread_cond_destroy(&rwlock->writers_cond);
	if (err != 0)
		return err;
	return pthread_mutex_destroy(&rwlock->lock);
}

int
pthread_rwlock_rdlock(pthread_rwlock_t *rwlock)
{
	if (rwlock == (pthread_rwlock_t *)NULL)
		return EINVAL;
	pthread_testcancel();
	pthread_mutex_lock(&rwlock->lock);
	while (rwlock->writers_active > 0 || rwlock->writers_waiting > 0)
		pthread_cond_wait(&rwlock->readers_cond, &rwlock->lock);
	rwlock->readers_active++;
	pthread_mutex_unlock(&rwlock->lock);
	return 0;
}

int
pthread_rwlock_tryrdlock(pthread_rwlock_t *rwlock)
{
	int err;

	if (rwlock == (pthread_rwlock_t *)NULL)
		return EINVAL;
	err = pthread_mutex_trylock(&rwlock->lock);
	if (err != 0)
		return err;
	if (rwlock->writers_active > 0 || rwlock->writers_waiting > 0)
	{
		pthread_mutex_unlock(&rwlock->lock);
		return EBUSY;
	}
	rwlock->readers_active++;
	pthread_mutex_unlock(&rwlock->lock);
	return 0;
}

int
pthread_rwlock_wrlock(pthread_rwlock_t *rwlock)
{
	if (rwlock == (pthread_rwlock_t *)NULL)
		return EINVAL;
	pthread_testcancel();
	pthread_mutex_lock(&rwlock->lock);
	rwlock->writers_waiting++;
	while (rwlock->readers_active > 0 || rwlock->writers_active > 0)
		pthread_cond_wait(&rwlock->writers_cond, &rwlock->lock);
	rwlock->writers_waiting--;
	rwlock->writers_active++;
	pthread_mutex_unlock(&rwlock->lock);
	return 0;
}

int
pthread_rwlock_trywrlock(pthread_rwlock_t *rwlock)
{
	int err;

	if (rwlock == (pthread_rwlock_t *)NULL)
		return EINVAL;
	err = pthread_mutex_trylock(&rwlock->lock);
	if (err != 0)
		return err;
	if (rwlock->readers_active > 0 || rwlock->writers_active > 0)
	{
		pthread_mutex_unlock(&rwlock->lock);
		return EBUSY;
	}
	rwlock->writers_active++;
	pthread_mutex_unlock(&rwlock->lock);
	return 0;
}

int
pthread_rwlock_unlock(pthread_rwlock_t *rwlock)
{
	if (rwlock == (pthread_rwlock_t *)NULL)
		return EINVAL;
	pthread_mutex_lock(&rwlock->lock);
	if (rwlock->writers_active > 0)
	{
		rwlock->writers_active--;
		if (rwlock->writers_waiting > 0)
			pthread_cond_signal(&rwlock->writers_cond);
		else
			pthread_cond_broadcast(&rwlock->readers_cond);
	}
	else if (rwlock->readers_active > 0)
	{
		rwlock->readers_active--;
		if (rwlock->readers_active == 0 && rwlock->writers_waiting > 0)
			pthread_cond_signal(&rwlock->writers_cond);
	}
	pthread_mutex_unlock(&rwlock->lock);
	return 0;
}

int
pthread_once(pthread_once_t *once_control, void (*init_routine)(void))
{
	if (once_control == (pthread_once_t *)NULL
	    || init_routine == (void (*)(void))NULL)
		return EINVAL;

	if (!once_control->sem_inited)
	{
		Forbid();
		if (!once_control->sem_inited)
		{
			InitSemaphore(&once_control->sem);
			once_control->sem_inited = 1;
		}
		Permit();
	}

	ObtainSemaphore(&once_control->sem);
	if (!once_control->done)
	{
		(*init_routine)();
		once_control->done = 1;
	}
	ReleaseSemaphore(&once_control->sem);
	return 0;
}
