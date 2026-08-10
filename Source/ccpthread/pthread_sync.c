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
#include <proto/timer.h>
#include <proto/utility.h>
#include <proto/cclib.h>
#include <errno.h>
#include <time.h>
#include <pthread.h>

#include "cc_pthread.h"

typedef struct
{
	struct Node node;
	struct Task *task;
	ULONG sigmask;
} cc_CondWaiter;

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
		Wait(sigmask);
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
	got = Wait(sigmask | timsig);

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
	if (mutex->waiters != 0
	    || mutex->semaphore.ss_Owner != (struct Task *)NULL)
		return EBUSY;
	return 0;
}

int
pthread_mutex_lock(pthread_mutex_t *mutex)
{
	if (mutex == (pthread_mutex_t *)NULL)
		return EINVAL;
	ObtainSemaphore(&mutex->semaphore);
	return 0;
}

int
pthread_mutex_trylock(pthread_mutex_t *mutex)
{
	if (mutex == (pthread_mutex_t *)NULL)
		return EINVAL;
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

	if (mutex == (pthread_mutex_t *)NULL
	    || abstime == (const struct timespec *)NULL)
		return EINVAL;
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
	Wait(mask);

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
