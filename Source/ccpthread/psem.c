/*
 * psem.c - unnamed POSIX semaphores (Exec SignalSemaphore + waiter list)
 */

#include <exec/types.h>
#include <exec/lists.h>
#include <exec/semaphores.h>
#include <exec/tasks.h>
#include <dos/dos.h>
#include <dos/dosextens.h>

#include <proto/exec.h>
#include <proto/cclib.h>
#include <errno.h>
#include <time.h>
#include <semaphore.h>

#include "cc_pthread.h"

#define CC_SEM_MAGIC 0x4353454DUL /* 'CSEM' */

typedef struct
{
	struct Node node;
	struct Task *task;
	ULONG sigmask;
} cc_SemWaiter;

int
sem_init(sem_t *sem, int pshared, unsigned int value)
{
	if (sem == (sem_t *)NULL || value > SEM_VALUE_MAX)
	{
		errno = EINVAL;
		return -1;
	}
	if (pshared != 0)
	{
		errno = ENOSYS;
		return -1;
	}
	InitSemaphore(&sem->lock);
	cc_thr_newlist(&sem->waiters);
	sem->magic = CC_SEM_MAGIC;
	sem->value = (LONG)value;
	return 0;
}

int
sem_destroy(sem_t *sem)
{
	if (sem == (sem_t *)NULL || sem->magic != CC_SEM_MAGIC)
	{
		errno = EINVAL;
		return -1;
	}
	ObtainSemaphore(&sem->lock);
	if (sem->waiters.lh_Head->ln_Succ != (struct Node *)NULL)
	{
		ReleaseSemaphore(&sem->lock);
		errno = EBUSY;
		return -1;
	}
	sem->magic = 0UL;
	ReleaseSemaphore(&sem->lock);
	return 0;
}

int
sem_trywait(sem_t *sem)
{
	if (sem == (sem_t *)NULL || sem->magic != CC_SEM_MAGIC)
	{
		errno = EINVAL;
		return -1;
	}
	ObtainSemaphore(&sem->lock);
	if (sem->value > 0L)
	{
		sem->value--;
		ReleaseSemaphore(&sem->lock);
		return 0;
	}
	ReleaseSemaphore(&sem->lock);
	errno = EAGAIN;
	return -1;
}

int
sem_wait(sem_t *sem)
{
	cc_SemWaiter w;
	struct Process *proc;
	ULONG mask;
	BYTE sig;

	if (sem == (sem_t *)NULL || sem->magic != CC_SEM_MAGIC)
	{
		errno = EINVAL;
		return -1;
	}

	sig = AllocSignal(-1L);
	if (sig < 0)
	{
		errno = EAGAIN;
		return -1;
	}

	proc = (struct Process *)FindTask((STRPTR)NULL);
	mask = 1UL << sig;

	ObtainSemaphore(&sem->lock);
	if (sem->value > 0L)
	{
		sem->value--;
		ReleaseSemaphore(&sem->lock);
		FreeSignal(sig);
		return 0;
	}
	w.task = (struct Task *)proc;
	w.sigmask = mask;
	w.node.ln_Succ = (struct Node *)NULL;
	w.node.ln_Pred = (struct Node *)NULL;
	AddTail(&sem->waiters, &w.node);
	ReleaseSemaphore(&sem->lock);

	cc_thr_wait_or_break(mask);

	ObtainSemaphore(&sem->lock);
	if (w.node.ln_Succ != (struct Node *)NULL)
		Remove(&w.node);
	if (sem->value > 0L)
		sem->value--;
	ReleaseSemaphore(&sem->lock);
	FreeSignal(sig);
	return 0;
}

int
sem_timedwait(sem_t *sem, const struct timespec *abstime)
{
	cc_SemWaiter w;
	struct Process *proc;
	ULONG mask;
	int err;
	BYTE sig;

	if (sem == (sem_t *)NULL || sem->magic != CC_SEM_MAGIC)
	{
		errno = EINVAL;
		return -1;
	}

	sig = AllocSignal(-1L);
	if (sig < 0)
	{
		errno = EAGAIN;
		return -1;
	}

	proc = (struct Process *)FindTask((STRPTR)NULL);
	mask = 1UL << sig;

	ObtainSemaphore(&sem->lock);
	if (sem->value > 0L)
	{
		sem->value--;
		ReleaseSemaphore(&sem->lock);
		FreeSignal(sig);
		return 0;
	}
	w.task = (struct Task *)proc;
	w.sigmask = mask;
	w.node.ln_Succ = (struct Node *)NULL;
	w.node.ln_Pred = (struct Node *)NULL;
	AddTail(&sem->waiters, &w.node);
	ReleaseSemaphore(&sem->lock);

	err = cc_thr_wait_timeout(mask, abstime);

	ObtainSemaphore(&sem->lock);
	if (w.node.ln_Succ != (struct Node *)NULL)
		Remove(&w.node);
	if (err == 0 && sem->value > 0L)
		sem->value--;
	else if (err == ETIMEDOUT)
	{
		ReleaseSemaphore(&sem->lock);
		FreeSignal(sig);
		errno = ETIMEDOUT;
		return -1;
	}
	ReleaseSemaphore(&sem->lock);
	FreeSignal(sig);
	if (err != 0)
	{
		errno = err;
		return -1;
	}
	return 0;
}

int
sem_post(sem_t *sem)
{
	cc_SemWaiter *w;

	if (sem == (sem_t *)NULL || sem->magic != CC_SEM_MAGIC)
	{
		errno = EINVAL;
		return -1;
	}
	ObtainSemaphore(&sem->lock);
	if (sem->value >= (LONG)SEM_VALUE_MAX)
	{
		ReleaseSemaphore(&sem->lock);
		errno = EOVERFLOW;
		return -1;
	}
	sem->value++;
	w = (cc_SemWaiter *)RemHead(&sem->waiters);
	if (w != (cc_SemWaiter *)NULL)
	{
		w->node.ln_Succ = (struct Node *)NULL;
		w->node.ln_Pred = (struct Node *)NULL;
		Signal(w->task, w->sigmask);
	}
	ReleaseSemaphore(&sem->lock);
	return 0;
}

int
sem_getvalue(sem_t *sem, int *sval)
{
	if (sem == (sem_t *)NULL || sem->magic != CC_SEM_MAGIC
	    || sval == (int *)NULL)
	{
		errno = EINVAL;
		return -1;
	}
	ObtainSemaphore(&sem->lock);
	*sval = (int)sem->value;
	ReleaseSemaphore(&sem->lock);
	return 0;
}
