/*
 * process.c - process spawn / wait / kill for cclib.library
 *
 * Workbench-style children are tracked on ud->Children. CLI commands use
 * Execute()/SystemTagList. Async WB launches use LoadSeg + CreateNewProc.
 */

#include <exec/types.h>
#include <exec/memory.h>
#include <exec/tasks.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <dos/dosextens.h>
#include <workbench/workbench.h>
#include <workbench/startup.h>
#include <utility/tagitem.h>

#include <proto/exec.h>
#include <proto/dos.h>

#include "private/cc_internal.h"
#include "cclib_funcs.h"

#define CC_BREAK_MASK \
	(SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_D | SIGBREAKF_CTRL_E | SIGBREAKF_CTRL_F)

/* Ensure the shared reply port for child WBStartup messages exists */
static struct MsgPort *cc_get_exeport(cc_task_UserData *ud)
{
	if (ud->exeport != NULL)
		return ud->exeport;

	ud->exeport = CreateMsgPort();
	return ud->exeport;
}

/* True if this Child payload is linked on ud->Children */
static LONG cc_child_on_list(cc_task_UserData *ud, cc_Child *child)
{
	cc_node *n;
	cc_Child *cand;

	n = ud->Children.first;
	while (n != NULL)
	{
		cand = (cc_Child *)((char *)n + sizeof(cc_node));
		if (cand == child)
			return 1L;
		n = n->next;
	}
	return 0L;
}

/* Find Child whose WBStartup message matches msg */
static cc_Child *cc_child_from_msg(cc_task_UserData *ud, struct Message *msg)
{
	cc_node *n;
	cc_Child *cand;

	n = ud->Children.first;
	while (n != NULL)
	{
		cand = (cc_Child *)((char *)n + sizeof(cc_node));
		if ((struct Message *)&cand->startup == msg)
			return cand;
		n = n->next;
	}
	return NULL;
}

/* Unload segment / locks; remove from Children when linked */
static void cc_reap_child(cc_task_UserData *ud, cc_Child *child)
{
	BPTR seg;
	LONG linked;
	cc_node *node;
	ULONG *hp;

	if (child == NULL)
		return;

	linked = cc_child_on_list(ud, child);

	seg = child->startup.sm_Segment;
	if (seg != NULL)
	{
		UnLoadSeg(seg);
		child->startup.sm_Segment = NULL;
	}

	if (child->arg.wa_Lock != NULL)
	{
		UnLock(child->arg.wa_Lock);
		child->arg.wa_Lock = NULL;
	}

	child->pid = NULL;

	if (linked)
	{
		node = (cc_node *)remv_node((APTR)&ud->Children, (APTR)child);
		if (node != NULL)
		{
			hp = ((ULONG *)node) - 1;
			FreeMem(hp, *hp);
		}
	}
}

/*
 * Wait until want replies (or any listed child if want is NULL).
 * Stack-local Children from wbexec are matched by message address.
 */
static LONG cc_wait_one(cc_task_UserData *ud, cc_Child *want)
{
	struct MsgPort *port;
	struct Message *msg;
	cc_Child *ch;
	ULONG sig;

	port = cc_get_exeport(ud);
	if (port == NULL)
		return 0L;

	for (;;)
	{
		if (want != NULL && want->pid == NULL)
			return 1L;

		if (want == NULL && ud->Children.length == 0L)
			return 1L;

		while ((msg = GetMsg(port)) != NULL)
		{
			ch = NULL;

			if (want != NULL && (struct Message *)&want->startup == msg)
				ch = want;
			else
				ch = cc_child_from_msg(ud, msg);

			if (ch != NULL)
			{
				cc_reap_child(ud, ch);
				if (want == NULL || want == ch)
					return 1L;
			}
		}

		if (want == NULL && ud->Children.length == 0L)
			return 1L;

		sig = Wait((1UL << port->mp_SigBit) | CC_BREAK_MASK);
		if (sig & CC_BREAK_MASK)
		{
			if (ud->abort_func != NULL)
				(*ud->abort_func)((LONG)(sig & CC_BREAK_MASK));
		}
	}
}

LONG __ASM__ __SAVE_DS__ wb_exec(
	__REG__(a0, STRPTR filename),
	__REG__(a1, STRPTR procname),
	__REG__(a2, STRPTR toolname),
	__REG__(a3, STRPTR toolwindow),
	__REG__(a4, BPTR curdir),
	__REG__(a5, APTR rport),
	__REG__(d0, LONG stk),
	__REG__(d1, LONG pri),
	__REG__(d2, APTR child))
{
	cc_task_UserData *ud;
	cc_Child *ch;
	struct MsgPort *reply;
	struct Process *parent;
	struct Process *proc;
	BPTR seg;
	BPTR dir;
	STRPTR pname;
	LONG stack;
	char pathbuf[128];

	ud = cc_get_ud();
	ch = (cc_Child *)child;
	if (ud == NULL || filename == NULL || ch == NULL)
		return 0L;

	parent = (struct Process *)FindTask(NULL);
	reply = (struct MsgPort *)rport;
	if (reply == NULL)
		reply = cc_get_exeport(ud);
	if (reply == NULL)
		return 0L;

	pname = procname != NULL ? procname : filename;
	stack = stk;
	if (stack <= 0L)
		stack = parent->pr_StackSize;

	/* Load from current dir, then fall back to C: */
	seg = LoadSeg(filename);
	if (seg == NULL)
	{
		strcpy(pathbuf, (STRPTR)"C:");
		strcat(pathbuf, filename);
		seg = LoadSeg(pathbuf);
	}
	if (seg == NULL)
		return 0L;

	dir = curdir;
	if (dir == (BPTR)-1L)
		dir = DupLock(parent->pr_CurrentDir);
	else if (dir != NULL)
		dir = DupLock(dir);

	proc = CreateNewProcTags(
		NP_Seglist, (ULONG)seg,
		NP_FreeSeglist, FALSE,
		NP_StackSize, stack,
		NP_Priority, pri,
		NP_Name, (ULONG)pname,
		NP_Input, (ULONG)NULL,
		NP_Output, (ULONG)NULL,
		NP_Error, (ULONG)NULL,
		NP_CloseInput, FALSE,
		NP_CloseOutput, FALSE,
		NP_CloseError, FALSE,
		TAG_DONE);

	if (proc == NULL)
	{
		UnLoadSeg(seg);
		if (dir != NULL)
			UnLock(dir);
		return 0L;
	}

	ch->pid = proc;
	ch->startup.sm_Message.mn_Node.ln_Type = NT_MESSAGE;
	ch->startup.sm_Message.mn_Length = (UWORD)sizeof(struct WBStartup);
	ch->startup.sm_Message.mn_ReplyPort = reply;
	ch->startup.sm_Process = &proc->pr_MsgPort;
	ch->startup.sm_Segment = seg;
	ch->startup.sm_NumArgs = 1L;
	ch->startup.sm_ToolWindow = toolwindow;
	ch->startup.sm_ArgList = &ch->arg;

	ch->arg.wa_Lock = dir;
	if (toolname != NULL && toolname[0] != '\0')
		ch->arg.wa_Name = toolname;
	else
		ch->arg.wa_Name = filename;

	PutMsg(&proc->pr_MsgPort, (struct Message *)&ch->startup);
	return 1L;
}

LONG __ASM__ __SAVE_DS__ wbexec(
	__REG__(a0, STRPTR name),
	__REG__(d0, LONG stk),
	__REG__(d1, LONG pri))
{
	cc_task_UserData *ud;
	cc_Child child;
	LONG ok;

	ud = cc_get_ud();
	if (ud == NULL || name == NULL)
		return 0L;

	/* Clear so reap sees clean locks/seg on failure paths */
	child.startup.sm_Segment = NULL;
	child.arg.wa_Lock = NULL;
	child.pid = NULL;

	ok = wb_exec(name, name, (STRPTR)"", NULL, (BPTR)-1L,
		(APTR)cc_get_exeport(ud), stk, pri, (APTR)&child);
	if (!ok)
		return 0L;

	return wait_child((APTR)&child);
}

APTR __ASM__ __SAVE_DS__ wbAexec(
	__REG__(a0, STRPTR name),
	__REG__(d0, LONG stk),
	__REG__(d1, LONG pri))
{
	cc_task_UserData *ud;
	cc_Child *ch;
	LONG ok;
	cc_node *node;
	ULONG *hp;

	ud = cc_get_ud();
	if (ud == NULL || name == NULL)
		return NULL;

	ch = (cc_Child *)add_head((APTR)&ud->Children, (LONG)sizeof(cc_Child));
	if (ch == NULL)
	{
		cc_set_errno(CC_ENOMEM);
		return NULL;
	}

	ch->startup.sm_Segment = NULL;
	ch->arg.wa_Lock = NULL;
	ch->pid = NULL;

	ok = wb_exec(name, name, (STRPTR)"", NULL, (BPTR)-1L,
		(APTR)cc_get_exeport(ud), stk, pri, (APTR)ch);
	if (!ok)
	{
		node = (cc_node *)remv_node((APTR)&ud->Children, (APTR)ch);
		if (node != NULL)
		{
			hp = ((ULONG *)node) - 1;
			FreeMem(hp, *hp);
		}
		return NULL;
	}
	return (APTR)ch;
}

LONG __ASM__ __SAVE_DS__ wait_child(
	__REG__(a0, APTR child))
{
	cc_task_UserData *ud;

	ud = cc_get_ud();
	if (ud == NULL)
		return 0L;

	if (child == NULL)
		return wait_children();

	return cc_wait_one(ud, (cc_Child *)child);
}

LONG __ASM__ __SAVE_DS__ wait_children(void)
{
	cc_task_UserData *ud;

	ud = cc_get_ud();
	if (ud == NULL)
		return 0L;

	while (ud->Children.length > 0L)
	{
		if (!cc_wait_one(ud, NULL))
			return 0L;
	}
	return 1L;
}

LONG __ASM__ __SAVE_DS__ signal_child(
	__REG__(a0, APTR child),
	__REG__(d0, ULONG sig))
{
	cc_Child *ch;

	ch = (cc_Child *)child;
	if (ch == NULL || ch->pid == NULL)
		return 0L;

	Signal((struct Task *)ch->pid, sig);
	return 1L;
}

LONG __ASM__ __SAVE_DS__ kill(
	__REG__(a0, APTR child))
{
	cc_Child *ch;

	ch = (cc_Child *)child;
	if (ch == NULL)
		return 0L;

	if (ch->pid != NULL)
		Signal((struct Task *)ch->pid, CC_BREAK_MASK);

	return wait_child(child);
}

LONG __ASM__ __SAVE_DS__ kill_children(void)
{
	cc_task_UserData *ud;
	cc_node *n;
	cc_Child *ch;

	ud = cc_get_ud();
	if (ud == NULL)
		return 0L;

	n = ud->Children.first;
	while (n != NULL)
	{
		ch = (cc_Child *)((char *)n + sizeof(cc_node));
		if (ch->pid != NULL)
			Signal((struct Task *)ch->pid, CC_BREAK_MASK);
		n = n->next;
	}
	return wait_children();
}

LONG __ASM__ __SAVE_DS__ cliexec(
	__REG__(a0, STRPTR cmd))
{
	struct Process *pr;
	LONG ok;

	if (cmd == NULL)
		return 0L;

	pr = (struct Process *)FindTask(NULL);
	if (pr->pr_CLI == NULL)
		return 0L;

	ok = Execute(cmd, Input(), Output());
	return ok ? 1L : 0L;
}

LONG __ASM__ __SAVE_DS__ exec(
	__REG__(a0, STRPTR cmd))
{
	struct Process *pr;
	STRPTR p;
	char name[108];
	LONG i;
	LONG ok;

	if (cmd == NULL)
		return 0L;

	pr = (struct Process *)FindTask(NULL);

	/* CLI parent: run through the shell and wait */
	if (pr->pr_CLI != NULL)
	{
		ok = SystemTags(cmd,
			SYS_Input, Input(),
			SYS_Output, Output(),
			SYS_Error, Output(),
			TAG_DONE);
		return (ok >= 0L) ? 1L : 0L;
	}

	/* Workbench parent: strip args and wbexec synchronously */
	i = 0L;
	p = cmd;
	while (*p && *p != ' ' && *p != '\t' && i < 107L)
	{
		name[i] = *p;
		i++;
		p++;
	}
	name[i] = '\0';

	if (name[0] == '\0')
		return 0L;

	return wbexec(name, 0L, 0L);
}
