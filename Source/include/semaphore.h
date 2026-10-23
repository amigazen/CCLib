/* SPDX-License-Identifier: BSD-2-Clause
 * Copyright (c) 2026 amigazen project
 */

#ifndef SEMAPHORE_H
#define SEMAPHORE_H 1

/*
 * Unnamed POSIX semaphores - static ccpthread.lib.
 */

#include <exec/types.h>
#include <exec/semaphores.h>
#include <time.h>

#define SEM_FAILED ((sem_t *)0L)
#define SEM_VALUE_MAX 32767U

typedef struct
{
	struct SignalSemaphore lock;
	ULONG magic;
	LONG value;
	struct List waiters;
} sem_t;

int sem_init(sem_t *sem, int pshared, unsigned int value);
int sem_destroy(sem_t *sem);
int sem_wait(sem_t *sem);
int sem_trywait(sem_t *sem);
int sem_timedwait(sem_t *sem, const struct timespec *abstime);
int sem_post(sem_t *sem);
int sem_getvalue(sem_t *sem, int *sval);

#endif /* SEMAPHORE_H */
