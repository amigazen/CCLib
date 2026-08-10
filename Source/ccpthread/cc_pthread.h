#ifndef CC_PTHREAD_H
#define CC_PTHREAD_H 1

/*
 * Private thread control block for static ccpthread.lib.
 * Same ThreadPair model as unix.lib2; after worker SetupSTDIO the pair
 * lives on cc_task_UserData.thr_ctx (tc_UserData becomes the session).
 */

#include <exec/types.h>
#include <exec/lists.h>
#include <exec/semaphores.h>
#include <exec/tasks.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <pthread.h>

#define CC_THR_MAGIC 0x43435448UL /* 'CCTH' */

/* Cooperative cancel state machine (unix.lib2). */
typedef enum
{
	CC_THR_RUNNING = 0,
	CC_THR_CANCEL_REQUESTED = 1,
	CC_THR_CANCELED = 2
} cc_thr_cancel_sm_t;

typedef struct cc_Thread
{
	struct Node node;
	ULONG magic;
	pthread_t id;
	struct Process *parent;
	struct Process *child;
	BYTE parent_sig;
	BYTE child_sigbit; /* pr_MsgPort bit used for handshake */
	void *(*start)(void *); /* registerized under parm=r */
	void *arg;
	void *result;
	LONG finished;
	LONG detached;
	LONG is_main;
	int thr_errno;
	/* Per-worker stdio slots — do not overwrite CRT globals (parent). */
	FILE *fp_stdin;
	FILE *fp_stdout;
	FILE *fp_stderr;
	/* TLS values (unix.lib2 tp_TLSValues). */
	void *tls_values[PTHREAD_KEYS_MAX];
	/* Cooperative cancel (unix.lib2). */
	int cancel_state;
	int cancel_type;
	cc_thr_cancel_sm_t cancel_sm;
	struct SignalSemaphore cancel_sem;
	LONG cancel_inited;
} cc_Thread;

extern struct List cc_thr_list;
extern struct SignalSemaphore cc_thr_list_sem;
extern pthread_t cc_thr_next_id;

/* Asm NP_Entry (pthread_startup.asm); __stdargs => _cc_pthread_startup. */
void __stdargs cc_pthread_startup(void);
/* C body after asm A4 setup; __saveds like unix.lib2 ThreadEntry. */
void __stdargs __saveds cc_thread_entry(void);

cc_Thread *cc_thr_find(pthread_t id);
cc_Thread *cc_thr_current(void);
int cc_thr_ensure_list(void);
void __stdargs __saveds cc_thr_session_teardown(APTR ctx);
void cc_thr_run_tls_dtors(cc_Thread *tp);
void cc_thr_cancel_check(cc_Thread *tp);
/* Local Ctrl-C -> session abort_func (do not call library cc_check_break). */
void cc_thr_check_break(void);
/* Wait(sigmask | Ctrl-C); Ctrl-C -> cc_thr_check_break. */
void cc_thr_wait_or_break(ULONG sigmask);

/*
 * NewList is amiga.lib (stackargs).  Under parm=r with no prototype the
 * compiler emits @NewList and jumps into the weeds — use this instead.
 */
void cc_thr_newlist(struct List *list);

/* Shared timed-wait helper (timer.device). Returns 0 wake, ETIMEDOUT, else err. */
int cc_thr_wait_timeout(ULONG sigmask, const struct timespec *abstime);

#endif /* CC_PTHREAD_H */
