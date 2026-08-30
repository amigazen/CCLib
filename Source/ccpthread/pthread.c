/*
 * pthread.c - create / join / detach / exit / self
 *
 * Process model and MsgPort handshake match unix.lib.  CClib-only deltas:
 * worker SetupSTDIO / ClearSTDIO, thr_ctx on the session (SetupSTDIO owns
 * tc_UserData), and main-thread ensure via thr_ctx.
 */

#include <exec/types.h>
#include <exec/memory.h>
#include <exec/semaphores.h>
#include <exec/tasks.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/dostags.h>
#include <utility/tagitem.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/cclib.h>
#include <libraries/cclib.h>
#include <errno.h>
#include <string.h>

#include <private/cc_internal.h>
#include <private/cc_break.h>
#include "cc_pthread.h"

/* pthread_startup.asm — reload A4 after SetupSTDIO (arg in A4). */
void __stdargs cc_geta4(void);

/* Defined below; cancel check may pthread_exit. */
void pthread_exit(void *retval);

struct List cc_thr_list;
struct SignalSemaphore cc_thr_list_sem;
pthread_t cc_thr_next_id = 1;
static LONG cc_thr_inited;

/* Global TLS key table (unix.lib2 tls_keys). */
typedef struct
{
	void (*destructor)(void *);
	LONG used;
} cc_TLSKey;

static cc_TLSKey cc_tls_keys[PTHREAD_KEYS_MAX];
static struct SignalSemaphore cc_tls_sem;
static LONG cc_tls_inited;

static void
cc_thr_init_cancel(cc_Thread *tp)
{
	tp->cancel_state = PTHREAD_CANCEL_ENABLE;
	tp->cancel_type = PTHREAD_CANCEL_DEFERRED;
	tp->cancel_sm = CC_THR_RUNNING;
	InitSemaphore(&tp->cancel_sem);
	tp->cancel_inited = 1;
}

void
cc_thr_cancel_check(cc_Thread *tp)
{
	if (tp == (cc_Thread *)NULL || !tp->cancel_inited)
		return;
	if (tp->cancel_state != PTHREAD_CANCEL_ENABLE)
		return;

	ObtainSemaphore(&tp->cancel_sem);
	if (tp->cancel_sm == CC_THR_CANCEL_REQUESTED)
	{
		tp->cancel_sm = CC_THR_CANCELED;
		ReleaseSemaphore(&tp->cancel_sem);
		pthread_exit(PTHREAD_CANCELED);
		return;
	}
	ReleaseSemaphore(&tp->cancel_sem);
}

void
cc_thr_run_tls_dtors(cc_Thread *tp)
{
	int i;
	void *val;
	void (*dtor)(void *);

	if (tp == (cc_Thread *)NULL)
		return;

	/*
	 * Near-BSS destructors need A4 (library ClearSTDIO leaves A4 as
	 * the library base).  Idempotent: cleared slots are skipped.
	 */
	cc_geta4();

	for (i = 0; i < PTHREAD_KEYS_MAX; i++)
	{
		val = tp->tls_values[i];
		if (val == (void *)NULL)
			continue;
		dtor = (void (*)(void *))NULL;
		if (cc_tls_inited)
		{
			ObtainSemaphore(&cc_tls_sem);
			if (cc_tls_keys[i].used)
				dtor = cc_tls_keys[i].destructor;
			ReleaseSemaphore(&cc_tls_sem);
		}
		tp->tls_values[i] = (void *)NULL;
		if (dtor != (void (*)(void *))NULL)
			(*dtor)(val);
	}
}

/*
 * Poll break mask via private CheckBreak LVO (same policy as library
 * cc_check_break).  Do not call cc_check_break by name from this static
 * lib — it is not an LVO; parm=r would emit @cc_check_break and SLINK fails.
 */
void
cc_thr_check_break(void)
{
	(void)CheckBreak();
}

/*
 * Wait for sigmask, but always wake on CC_BREAK_MASK so the session abort
 * hook can force a panic/exit (join / cond / sem must not ignore breaks).
 */
void
cc_thr_wait_or_break(ULONG sigmask)
{
	ULONG got;
	ULONG brk;

	for (;;)
	{
		got = Wait(sigmask | CC_BREAK_MASK);
		brk = got & CC_BREAK_MASK;
		if (brk != 0UL)
		{
			/*
			 * Wait already cleared the bits from the task mask.
			 * Re-assert so CheckBreak sees them and runs abort_func.
			 */
			SignalBreak(brk);
			(void)CheckBreak();
			/* Test abort hooks may return; keep waiting. */
			continue;
		}
		if ((got & sigmask) != 0UL)
			return;
	}
}

/*
 * __saveds so A4 is valid for near-BSS before the app start routine runs.
 * Not __stdargs: under parm=r the stored start pointer is @fn.
 */
static void * __saveds
cc_thr_invoke(void *(*fn)(void *), void *arg)
{
	if (fn == (void *(*)(void *))NULL)
		return (void *)NULL;
	return (*fn)(arg);
}

static void __stdargs
cc_thr_abort(LONG code)
{
	cc_Thread *tp;
	cc_task_UserData *ud;
	struct Task *task;
	struct Process *parent;
	BYTE parent_sig;
	LONG detached;

	(void)code;

	/*
	 * Worker panic/Ctrl-C: wake joiners before RemTask so main does not
	 * hang in pthread_join / atexit.
	 */
	tp = (cc_Thread *)NULL;
	task = FindTask((STRPTR)NULL);
	if (task != (struct Task *)NULL)
	{
		ud = (cc_task_UserData *)task->tc_UserData;
		if (ud != (cc_task_UserData *)NULL)
			tp = (cc_Thread *)ud->thr_ctx;
	}

	ClearSTDIO();

	if (tp != (cc_Thread *)NULL && tp->magic == CC_THR_MAGIC && !tp->is_main)
	{
		Forbid();
		tp->result = PTHREAD_CANCELED;
		tp->finished = 1;
		detached = tp->detached;
		parent = tp->parent;
		parent_sig = tp->parent_sig;
		if (!detached && parent_sig >= 0 && parent != (struct Process *)NULL)
			Signal((struct Task *)parent, 1UL << parent_sig);
		Permit();
		if (detached)
		{
			ObtainSemaphore(&cc_thr_list_sem);
			Remove(&tp->node);
			ReleaseSemaphore(&cc_thr_list_sem);
			if (parent_sig >= 0)
				FreeSignal(parent_sig);
			FreeMem(tp, sizeof(cc_Thread));
		}
	}

	RemTask((struct Task *)NULL);
}

void __stdargs __saveds
cc_thr_session_teardown(APTR ctx)
{
	cc_Thread *tp;

	tp = (cc_Thread *)ctx;
	if (tp != (cc_Thread *)NULL && tp->magic == CC_THR_MAGIC)
		cc_thr_run_tls_dtors(tp);
}

void
cc_thr_newlist(struct List *list)
{
	if (list == (struct List *)NULL)
		return;
	list->lh_Head = (struct Node *)&list->lh_Tail;
	list->lh_Tail = (struct Node *)NULL;
	list->lh_TailPred = (struct Node *)list;
}

int
cc_thr_ensure_list(void)
{
	if (cc_thr_inited)
		return 0;
	cc_thr_newlist(&cc_thr_list);
	InitSemaphore(&cc_thr_list_sem);
	InitSemaphore(&cc_tls_sem);
	memset((APTR)cc_tls_keys, 0, sizeof(cc_tls_keys));
	cc_tls_inited = 1;
	cc_thr_next_id = 1;
	cc_thr_inited = 1;
	return 0;
}

cc_Thread *
cc_thr_find(pthread_t id)
{
	cc_Thread *tp;
	struct Node *n;

	ObtainSemaphore(&cc_thr_list_sem);
	n = cc_thr_list.lh_Head;
	while (n->ln_Succ != (struct Node *)NULL)
	{
		tp = (cc_Thread *)n;
		if (tp->magic == CC_THR_MAGIC && tp->id == id)
		{
			ReleaseSemaphore(&cc_thr_list_sem);
			return tp;
		}
		n = n->ln_Succ;
	}
	ReleaseSemaphore(&cc_thr_list_sem);
	return (cc_Thread *)NULL;
}

cc_Thread *
cc_thr_current(void)
{
	cc_task_UserData *ud;
	cc_Thread *tp;
	struct Task *t;

	/* After SetupSTDIO the pair is on the session, not tc_UserData. */
	ud = (cc_task_UserData *)GetSTDIO();
	if (ud != (cc_task_UserData *)NULL && ud->thr_ctx != (APTR)NULL)
	{
		tp = (cc_Thread *)ud->thr_ctx;
		if (tp->magic == CC_THR_MAGIC)
			return tp;
	}

	/* unix.lib2: pair still in tc_UserData during MsgPort Wait handshake. */
	t = FindTask((STRPTR)NULL);
	if (t != (struct Task *)NULL)
	{
		tp = (cc_Thread *)t->tc_UserData;
		if (tp != (cc_Thread *)NULL && tp->magic == CC_THR_MAGIC)
			return tp;
	}
	return (cc_Thread *)NULL;
}

static cc_Thread *
cc_thr_ensure_main(void)
{
	cc_task_UserData *ud;
	cc_Thread *tp;

	ud = (cc_task_UserData *)GetSTDIO();
	if (ud == (cc_task_UserData *)NULL)
		return (cc_Thread *)NULL;
	if (ud->thr_ctx != (APTR)NULL)
		return (cc_Thread *)ud->thr_ctx;

	if (cc_thr_ensure_list() != 0)
		return (cc_Thread *)NULL;

	tp = (cc_Thread *)AllocMem(sizeof(cc_Thread), MEMF_CLEAR | MEMF_PUBLIC);
	if (tp == (cc_Thread *)NULL)
		return (cc_Thread *)NULL;

	tp->magic = CC_THR_MAGIC;
	tp->is_main = 1;
	tp->child = (struct Process *)FindTask((STRPTR)NULL);
	tp->parent = tp->child;
	tp->parent_sig = -1;
	tp->finished = 1;
	cc_thr_init_cancel(tp);

	ObtainSemaphore(&cc_thr_list_sem);
	tp->id = cc_thr_next_id++;
	AddTail(&cc_thr_list, &tp->node);
	ReleaseSemaphore(&cc_thr_list_sem);

	ud->thr_ctx = (APTR)tp;
	ud->thr_teardown = cc_thr_session_teardown;
	return tp;
}

/*
 * unix.lib2 ThreadEntry: Wait(MsgPort), read tc_UserData, run start.
 * CClib: SetupSTDIO needs a real session in tc_UserData — clear the
 * ThreadPair out before any LVO (SetupSTDIO's first act is cc_check_break /
 * cc_get_ud, which must not see the pair as a session).
 */
void __stdargs __saveds
cc_thread_entry(void)
{
	cc_Thread *tp;
	cc_task_UserData *ud;
	struct Process *proc;
	void *rv;
	LONG detached;
	struct Process *parent;
	BYTE parent_sig;

	proc = (struct Process *)FindTask((STRPTR)NULL);

	/* Wait for parent Signal after tc_UserData = pair (unix.lib2). */
	Wait(1UL << proc->pr_MsgPort.mp_SigBit);

	Forbid();
	tp = (cc_Thread *)proc->pr_Task.tc_UserData;
	/* Pair must not remain in UserData across CClib LVOs. */
	proc->pr_Task.tc_UserData = (APTR)NULL;
	Permit();

	if (tp == (cc_Thread *)NULL || tp->magic != CC_THR_MAGIC)
		return;

	/*
	 * CClib session.  Stream pointers on the ThreadPair so CRT globals
	 * stdin/stdout/stderr (parent) are not overwritten.
	 */
	tp->thr_errno = 0;
	tp->fp_stdin = (FILE *)NULL;
	tp->fp_stdout = (FILE *)NULL;
	tp->fp_stderr = (FILE *)NULL;
	/*
	 * Install abort only after thr_ctx is set.  If SetupSTDIO panics with
	 * cc_thr_abort armed but thr_ctx still NULL, RemTask runs and the
	 * parent hangs forever in pthread_join.
	 */
	if (!SetupSTDIO(
		(FILE **)&tp->fp_stdin, (FILE **)&tp->fp_stdout,
		(FILE **)&tp->fp_stderr,
		&tp->thr_errno, (LONG *)NULL, (char **)NULL,
		(APTR)NULL, 0L, (char *)NULL, (APTR)NULL,
		(APTR)NULL))
	{
		Forbid();
		tp->result = (void *)NULL;
		tp->finished = 1;
		parent_sig = tp->parent_sig;
		parent = tp->parent;
		if (parent_sig >= 0 && parent != (struct Process *)NULL)
			Signal((struct Task *)parent, 1UL << parent_sig);
		Permit();
		return;
	}

	/*
	 * SetupSTDIO's libcall passes blocksize in A4; restore near-data base
	 * before touching app BSS.
	 */
	cc_geta4();

	ud = (cc_task_UserData *)GetSTDIO();
	if (ud != (cc_task_UserData *)NULL)
	{
		ud->thr_ctx = (APTR)tp;
		ud->thr_teardown = cc_thr_session_teardown;
	}
	SetAbortFunc((APTR)cc_thr_abort);

	/*
	 * Library LVOs do not preserve A4.  Invoke via __saveds trampoline
	 * so near-BSS (mutexes, flags) is reachable in the start routine.
	 */
	cc_thr_cancel_check(tp);
	rv = cc_thr_invoke(tp->start, tp->arg);

	/*
	 * TLS destructors need app A4; run before ClearSTDIO (which is a
	 * library LVO).  thr_teardown will no-op once slots are cleared.
	 */
	cc_geta4();
	cc_thr_run_tls_dtors(tp);

	ClearSTDIO();
	cc_geta4();

	Forbid();
	tp->result = rv;
	tp->finished = 1;
	detached = tp->detached;
	parent = tp->parent;
	parent_sig = tp->parent_sig;
	if (!detached && parent_sig >= 0 && parent != (struct Process *)NULL)
		Signal((struct Task *)parent, 1UL << parent_sig);
	Permit();

	if (detached)
	{
		ObtainSemaphore(&cc_thr_list_sem);
		Remove(&tp->node);
		ReleaseSemaphore(&cc_thr_list_sem);
		if (parent_sig >= 0)
			FreeSignal(parent_sig);
		FreeMem(tp, sizeof(cc_Thread));
	}
}

int
pthread_attr_init(pthread_attr_t *attr)
{
	if (attr == (pthread_attr_t *)NULL)
		return EINVAL;
	attr->detachstate = PTHREAD_CREATE_JOINABLE;
	attr->stacksize = (ULONG)PTHREAD_STACK_MIN;
	return 0;
}

int
pthread_attr_destroy(pthread_attr_t *attr)
{
	if (attr == (pthread_attr_t *)NULL)
		return EINVAL;
	return 0;
}

int
pthread_attr_setdetachstate(pthread_attr_t *attr, int state)
{
	if (attr == (pthread_attr_t *)NULL)
		return EINVAL;
	if (state != PTHREAD_CREATE_JOINABLE && state != PTHREAD_CREATE_DETACHED)
		return EINVAL;
	attr->detachstate = state;
	return 0;
}

int
pthread_attr_getdetachstate(const pthread_attr_t *attr, int *state)
{
	if (attr == (const pthread_attr_t *)NULL || state == (int *)NULL)
		return EINVAL;
	*state = attr->detachstate;
	return 0;
}

int
pthread_attr_setstacksize(pthread_attr_t *attr, size_t stacksize)
{
	if (attr == (pthread_attr_t *)NULL)
		return EINVAL;
	if (stacksize < (size_t)PTHREAD_STACK_MIN)
		return EINVAL;
	attr->stacksize = (ULONG)stacksize;
	return 0;
}

int
pthread_attr_getstacksize(const pthread_attr_t *attr, size_t *stacksize)
{
	if (attr == (const pthread_attr_t *)NULL || stacksize == (size_t *)NULL)
		return EINVAL;
	*stacksize = (size_t)attr->stacksize;
	return 0;
}

int
pthread_create(pthread_t *thread, const pthread_attr_t *attr,
	void *(*start_routine)(void *), void *arg)
{
	cc_Thread *tp;
	struct Process *child;
	ULONG stack;
	LONG detached;
	BYTE sig;
	char name[32];
	struct TagItem tags[12];
	LONG ti;
	BPTR in_fh;
	BPTR out_fh;

	if (thread == (pthread_t *)NULL || start_routine == (void *(*)(void *))NULL)
		return EINVAL;
	if (GetSTDIO() == (APTR)NULL)
		return EAGAIN;
	if (cc_thr_ensure_list() != 0)
		return EAGAIN;

	stack = (ULONG)PTHREAD_STACK_MIN;
	detached = 0;
	if (attr != (const pthread_attr_t *)NULL)
	{
		stack = attr->stacksize;
		if (stack < (ULONG)PTHREAD_STACK_MIN)
			stack = (ULONG)PTHREAD_STACK_MIN;
		detached = (attr->detachstate == PTHREAD_CREATE_DETACHED) ? 1L : 0L;
	}

	tp = (cc_Thread *)AllocMem(sizeof(cc_Thread), MEMF_CLEAR | MEMF_PUBLIC);
	if (tp == (cc_Thread *)NULL)
		return EAGAIN;

	sig = AllocSignal(-1L);
	if (sig < 0)
	{
		FreeMem(tp, sizeof(cc_Thread));
		return EAGAIN;
	}

	tp->magic = CC_THR_MAGIC;
	tp->parent = (struct Process *)FindTask((STRPTR)NULL);
	tp->parent_sig = sig;
	tp->start = start_routine;
	tp->arg = arg;
	tp->detached = detached;
	cc_thr_init_cancel(tp);

	ObtainSemaphore(&cc_thr_list_sem);
	tp->id = cc_thr_next_id++;
	AddTail(&cc_thr_list, &tp->node);
	ReleaseSemaphore(&cc_thr_list_sem);

	strcpy(name, (STRPTR)"CCPthread");

	/*
	 * Inherit parent console (unix.lib2 leaves NIL:; CClib workers need
	 * a real Output for printf / breadcrumbs).  Do not close on exit.
	 */
	in_fh = Input();
	out_fh = Output();
	ti = 0;
	tags[ti].ti_Tag = NP_Name;
	tags[ti].ti_Data = (ULONG)name;
	ti++;
	tags[ti].ti_Tag = NP_Entry;
	tags[ti].ti_Data = (ULONG)cc_pthread_startup;
	ti++;
	tags[ti].ti_Tag = NP_StackSize;
	tags[ti].ti_Data = stack;
	ti++;
	tags[ti].ti_Tag = NP_Priority;
	tags[ti].ti_Data = 0UL;
	ti++;
	tags[ti].ti_Tag = NP_Input;
	tags[ti].ti_Data = (ULONG)in_fh;
	ti++;
	tags[ti].ti_Tag = NP_CloseInput;
	tags[ti].ti_Data = FALSE;
	ti++;
	tags[ti].ti_Tag = NP_Output;
	tags[ti].ti_Data = (ULONG)out_fh;
	ti++;
	tags[ti].ti_Tag = NP_CloseOutput;
	tags[ti].ti_Data = FALSE;
	ti++;
	/* Match Output so ErrorOutput() is defined without a second BPTR. */
	tags[ti].ti_Tag = NP_Error;
	tags[ti].ti_Data = (ULONG)out_fh;
	ti++;
	tags[ti].ti_Tag = NP_CloseError;
	tags[ti].ti_Data = FALSE;
	ti++;
	tags[ti].ti_Tag = TAG_DONE;
	tags[ti].ti_Data = 0UL;

	child = CreateNewProc(tags);
	if (child == (struct Process *)NULL)
	{
		ObtainSemaphore(&cc_thr_list_sem);
		Remove(&tp->node);
		ReleaseSemaphore(&cc_thr_list_sem);
		FreeSignal(sig);
		FreeMem(tp, sizeof(cc_Thread));
		return EAGAIN;
	}

	tp->child = child;
	child->pr_Task.tc_UserData = (APTR)tp;
	Signal((struct Task *)child, 1UL << child->pr_MsgPort.mp_SigBit);

	*thread = tp->id;
	return 0;
}

int
pthread_join(pthread_t thread, void **retval)
{
	cc_Thread *tp;
	ULONG mask;

	tp = cc_thr_find(thread);
	if (tp == (cc_Thread *)NULL)
		return ESRCH;
	if (tp->is_main)
		return EDEADLK;
	if (tp->detached)
		return EINVAL;
	if (tp->child == (struct Process *)FindTask((STRPTR)NULL))
		return EDEADLK;

	mask = 1UL << tp->parent_sig;
	while (!tp->finished)
		cc_thr_wait_or_break(mask);

	if (retval != (void **)NULL)
		*retval = tp->result;

	ObtainSemaphore(&cc_thr_list_sem);
	Remove(&tp->node);
	ReleaseSemaphore(&cc_thr_list_sem);
	if (tp->parent_sig >= 0)
		FreeSignal(tp->parent_sig);
	FreeMem(tp, sizeof(cc_Thread));
	return 0;
}

int
pthread_detach(pthread_t thread)
{
	cc_Thread *tp;

	tp = cc_thr_find(thread);
	if (tp == (cc_Thread *)NULL)
		return ESRCH;
	if (tp->is_main)
		return EINVAL;
	if (tp->detached)
		return EINVAL;

	Forbid();
	if (tp->finished)
	{
		Permit();
		ObtainSemaphore(&cc_thr_list_sem);
		Remove(&tp->node);
		ReleaseSemaphore(&cc_thr_list_sem);
		if (tp->parent_sig >= 0)
			FreeSignal(tp->parent_sig);
		FreeMem(tp, sizeof(cc_Thread));
		return 0;
	}
	tp->detached = 1;
	Permit();
	return 0;
}

void
pthread_exit(void *retval)
{
	cc_Thread *tp;
	struct Process *proc;
	struct Process *parent;
	BYTE parent_sig;
	LONG detached;

	tp = cc_thr_current();
	if (tp != (cc_Thread *)NULL && !tp->is_main)
	{
		cc_geta4();
		cc_thr_run_tls_dtors(tp);
		ClearSTDIO();
		proc = (struct Process *)FindTask((STRPTR)NULL);
		Forbid();
		tp->result = retval;
		tp->finished = 1;
		detached = tp->detached;
		parent = tp->parent;
		parent_sig = tp->parent_sig;
		if (proc != (struct Process *)NULL)
			proc->pr_Task.tc_UserData = (APTR)NULL;
		if (!detached && parent_sig >= 0 && parent != (struct Process *)NULL)
			Signal((struct Task *)parent, 1UL << parent_sig);
		Permit();
		if (detached)
		{
			ObtainSemaphore(&cc_thr_list_sem);
			Remove(&tp->node);
			ReleaseSemaphore(&cc_thr_list_sem);
			if (parent_sig >= 0)
				FreeSignal(parent_sig);
			FreeMem(tp, sizeof(cc_Thread));
		}
		RemTask((struct Task *)NULL);
	}
	if (DOSBase != (struct DosLibrary *)NULL)
		Exit(0);
}

pthread_t
pthread_self(void)
{
	cc_Thread *tp;

	tp = cc_thr_current();
	if (tp == (cc_Thread *)NULL)
		tp = cc_thr_ensure_main();
	if (tp == (cc_Thread *)NULL)
		return (pthread_t)0;
	return tp->id;
}

int
pthread_equal(pthread_t t1, pthread_t t2)
{
	return (t1 == t2) ? 1 : 0;
}

int
pthread_key_create(pthread_key_t *key, void (*destructor)(void *))
{
	int i;

	if (key == (pthread_key_t *)NULL)
		return EINVAL;
	if (cc_thr_ensure_list() != 0)
		return EAGAIN;

	ObtainSemaphore(&cc_tls_sem);
	for (i = 0; i < PTHREAD_KEYS_MAX; i++)
	{
		if (!cc_tls_keys[i].used)
		{
			cc_tls_keys[i].used = 1;
			cc_tls_keys[i].destructor = destructor;
			*key = (pthread_key_t)i;
			ReleaseSemaphore(&cc_tls_sem);
			return 0;
		}
	}
	ReleaseSemaphore(&cc_tls_sem);
	return EAGAIN;
}

int
pthread_key_delete(pthread_key_t key)
{
	if (key >= (pthread_key_t)PTHREAD_KEYS_MAX)
		return EINVAL;
	if (!cc_tls_inited)
		return EINVAL;

	ObtainSemaphore(&cc_tls_sem);
	if (!cc_tls_keys[key].used)
	{
		ReleaseSemaphore(&cc_tls_sem);
		return EINVAL;
	}
	cc_tls_keys[key].used = 0;
	cc_tls_keys[key].destructor = (void (*)(void *))NULL;
	ReleaseSemaphore(&cc_tls_sem);
	return 0;
}

int
pthread_setspecific(pthread_key_t key, const void *value)
{
	cc_Thread *tp;

	if (key >= (pthread_key_t)PTHREAD_KEYS_MAX)
		return EINVAL;
	tp = cc_thr_current();
	if (tp == (cc_Thread *)NULL)
		tp = cc_thr_ensure_main();
	if (tp == (cc_Thread *)NULL)
		return EINVAL;
	tp->tls_values[key] = (void *)value;
	return 0;
}

void *
pthread_getspecific(pthread_key_t key)
{
	cc_Thread *tp;

	if (key >= (pthread_key_t)PTHREAD_KEYS_MAX)
		return (void *)NULL;
	tp = cc_thr_current();
	if (tp == (cc_Thread *)NULL)
		return (void *)NULL;
	return tp->tls_values[key];
}

int
pthread_cancel(pthread_t thread)
{
	cc_Thread *tp;

	tp = cc_thr_find(thread);
	if (tp == (cc_Thread *)NULL)
		return ESRCH;
	if (!tp->cancel_inited)
		return 0;

	ObtainSemaphore(&tp->cancel_sem);
	if (tp->cancel_sm == CC_THR_RUNNING)
		tp->cancel_sm = CC_THR_CANCEL_REQUESTED;
	ReleaseSemaphore(&tp->cancel_sem);
	return 0;
}

void
pthread_testcancel(void)
{
	cc_Thread *tp;

	tp = cc_thr_current();
	if (tp == (cc_Thread *)NULL)
		return;
	cc_thr_cancel_check(tp);
}

int
pthread_setcancelstate(int state, int *oldstate)
{
	cc_Thread *tp;

	if (state != PTHREAD_CANCEL_ENABLE && state != PTHREAD_CANCEL_DISABLE)
		return EINVAL;
	tp = cc_thr_current();
	if (tp == (cc_Thread *)NULL)
		tp = cc_thr_ensure_main();
	if (tp == (cc_Thread *)NULL)
		return EINVAL;
	if (oldstate != (int *)NULL)
		*oldstate = tp->cancel_state;
	tp->cancel_state = state;
	return 0;
}

int
pthread_setcanceltype(int type, int *oldtype)
{
	cc_Thread *tp;

	if (type != PTHREAD_CANCEL_DEFERRED
	    && type != PTHREAD_CANCEL_ASYNCHRONOUS)
		return EINVAL;
	tp = cc_thr_current();
	if (tp == (cc_Thread *)NULL)
		tp = cc_thr_ensure_main();
	if (tp == (cc_Thread *)NULL)
		return EINVAL;
	if (oldtype != (int *)NULL)
		*oldtype = tp->cancel_type;
	/*
	 * ASYNCHRONOUS stored but not honoured — cooperative deferred
	 * only (unix.lib2 same practical model).
	 */
	tp->cancel_type = type;
	return 0;
}

