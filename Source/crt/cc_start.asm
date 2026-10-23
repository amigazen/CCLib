*
* crt/cc_start.asm - NOSTARTUP entry (minimal sc_crt/c.a)
*
* Must be first on the SLINK FROM line.  Assemble from crt/ like sc_crt:
*   asm -u cc_start.asm
*
* Loads A4 from LinkerDB and clears BSS before any C runs - required for
* near data (unittest SCOPTIONS match sc_crt; not data=far).
*
* Also exports __cc_ctors_addr / __cc_dtors_addr for __fpinit: SLINK
* substitutes absolute 0 for __ctors/__dtors when no _STI/_STD exist
* (SAS UG Ch.10).  A C check on `extern T arr[]` can be treated as
* always non-NULL; lea of the linker symbol cannot.
*
* Stack: do not copy sc_crt/c.a.  That startup reads cli_DefaultStack
* (not tc_SPUpper-tc_SPLower) and, on exec V36+, always StackSwap's the
* task bounds down to that CLI default minus 128 bytes — even when it
* did not AllocMem.  dos.library has already applied a $STACK: cookie
* to the real bounds, so c.a hides it (c:stack vs SPUpper-SPLower differs
* by about 128 bytes) and RunCommand's stack is swapped away.  Here the
* process stack is kept when it is already >= __stack.  A larger block
* is allocated only when the real bounds are smaller, and cc_crt_exit
* swaps back and FreeMem's it before dos Exit ( _exit does not return
* to this frame ).
*

AbsExecBase	EQU	4
ThisTaskOff	EQU	276
TC_SPLOWER	EQU	58
TC_SPUPPER	EQU	62
LIB_VERSION	EQU	20
LVODisable	EQU	-120
LVOEnable	EQU	-126
LVOAllocMem	EQU	-198
LVOFreeMem	EQU	-210
LVOStackSwap	EQU	-732
LVOExit		EQU	-144
TC_SPREG	EQU	54
MEMF_PUBCLEAR	EQU	$10001

	xdef	__startup
	xdef	__cc_ctors_addr
	xdef	__cc_dtors_addr
*
* __regargs publishes @cc_crt_exit.  asm -u only adds '_' to a plain xdef,
* which leaves the C call unresolved.
*
	xdef	@cc_crt_exit
	xdef	cc_reload_a4

	xref	LinkerDB
	xref	_BSSBAS
	xref	_BSSLEN
	xref	__main
	xref	__ctors
	xref	__dtors
	xref	__stack
	xref	DOSBase

	csect	text

__startup:
start:
	movem.l	d1-d6/a0-a6,-(a7)

	lea	LinkerDB,a4
	move.l	AbsExecBase.W,a6

	lea	_BSSBAS,a3
	moveq	#0,d1
	move.l	#_BSSLEN,d0
	bra.s	clr_lp
clr_bss:
	move.l	d1,(a3)+
clr_lp:
	dbf	d0,clr_bss

*
* Grow only when the task's real stack is smaller than __stack.
* $STACK: / c:stack already live in tc_SPLower/tc_SPUpper.
*
	cmpi.w	#36,LIB_VERSION(a6)
	blt.s	no_grow
	move.l	__stack(a4),d0
	beq.s	no_grow
	move.l	ThisTaskOff(a6),a3
	move.l	TC_SPUPPER(a3),d1
	sub.l	TC_SPLOWER(a3),d1
	cmp.l	d0,d1
	bcc.s	no_grow

*
* Snapshot the real bounds before AllocMem.  a3 is ThisTask here.
* StackSwap is allowed to scratch A4; near data (__stack, stdout, DOSBase)
* is A4-relative, so a stale A4 makes the first putc look like a forged FILE
* and cc_panic(20).  Reload LinkerDB on the other side.
*
	move.l	TC_SPLOWER(a3),cc_oldlow(a4)
	move.l	TC_SPUPPER(a3),cc_oldup(a4)
	move.l	a7,cc_oldsp(a4)
	move.l	a7,cc_restore_sp(a4)

	move.l	d0,cc_newsize(a4)
	move.l	#MEMF_PUBCLEAR,d1
	jsr	LVOAllocMem(a6)
	lea	LinkerDB,a4
	tst.l	d0
	beq.s	no_grow
	move.l	d0,cc_newstack(a4)
	move.l	d0,cc_newlow(a4)
	add.l	cc_newsize(a4),d0
	move.l	d0,cc_newup(a4)
*
* Leave 32 bytes at the top.  Real exec RTS's through the new stack, so
* the longword at stk_Pointer must be the continuation, not the cleared 0.
*
	sub.l	#32,d0
	move.l	d0,cc_newsp(a4)
	move.l	d0,a0
	move.l	#grow_cont,(a0)
	lea	cc_newlow(a4),a0
	jsr	LVOStackSwap(a6)
grow_cont:
	lea	LinkerDB,a4

no_grow:
*
* sc_crt/c.a: jsr __fpinit then jsr __main with command line on stack.
* Pass NULL; __main uses GetArgStr for the Lattice args-only string.
* __main normally never returns (_exit -> cc_crt_exit -> dos Exit).
*
	clr.l	-(a7)
	jsr	__main
	addq.l	#4,a7

*
* __main returned (dos/utility open failed).  If a stack was grown, swap
* back before popping the entry MOVEM — that frame lives on the original
* stack.  Do not BSR across StackSwap: the return address would stay on
* the grown stack.
*
	tst.l	cc_newstack(a4)
	beq.s	start_restore
*
* Do not StackSwap back: that RTS pops a saved register off the shell
* stack (jump into data).  Hop onto the tiny static stack, put the
* original bounds back on the task, FreeMem, then return to the
* entry MOVEM on the shell stack.
*
	moveq	#0,d3
	lea	cc_exitstk_end(a4),a7
	bra.s	cc_drop_grown

start_restore:
	movem.l	(a7)+,d1-d6/a0-a6
	rts

*
* __regargs entry: D0 = exit code.  Does not return to C.  When a stack
* was grown, StackSwap restores the startup SP (after the entry MOVEM)
* and the C frame is abandoned; dos Exit does not come back either.
*
@cc_crt_exit:
cc_crt_exit:
	move.l	d0,d2
	lea	LinkerDB,a4
	tst.l	cc_newstack(a4)
	beq.s	crt_exit_dos
	moveq	#1,d3
	lea	cc_exitstk_end(a4),a7
	bra.s	cc_drop_grown

crt_exit_dos:
	move.l	DOSBase(a4),d0
	beq.s	exit_no_dos
	move.l	d0,a6
	move.l	d2,d1
	jsr	LVOExit(a6)
exit_no_dos:
	rts

*
* A7 is already cc_exitstk_end.  D3=0: back to the shell MOVEM.
* D3!=0: dos Exit (D2=code).  D2/D3/A4 survive Disable/FreeMem.
*
cc_drop_grown:
	move.l	AbsExecBase.W,a6
	jsr	LVODisable(a6)
	move.l	ThisTaskOff(a6),a3
	move.l	cc_oldlow(a4),TC_SPLOWER(a3)
	move.l	cc_oldup(a4),TC_SPUPPER(a3)
	move.l	cc_newstack(a4),a1
	move.l	cc_newsize(a4),d0
	clr.l	cc_newstack(a4)
	jsr	LVOFreeMem(a6)
	lea	LinkerDB,a4
	tst.l	d3
	bne.s	crt_exit_dos
	move.l	cc_restore_sp(a4),a7
	move.l	a7,TC_SPREG(a3)
	move.l	AbsExecBase.W,a6
	jsr	LVOEnable(a6)
	bra.s	start_restore

*
* exec CloseLibrary is allowed to scratch A4.  Near data (SysBase,
* CCLibBase, DOSBase) is A4-relative, so the next C call after a close
* (__chkabort / __fpclear) jumps through garbage.  Reload after every
* CloseLibrary.
*
cc_reload_a4:
	lea	LinkerDB,a4
	rts

*
* Return linker address of __ctors / __dtors in D0 (0 when abs-substituted).
*
__cc_ctors_addr:
	move.l	#__ctors,d0
	rts

__cc_dtors_addr:
	move.l	#__dtors,d0
	rts

	csect	__MERGED,bss

cc_newstack	ds.l	1
cc_newsize	ds.l	1
cc_newlow	ds.l	1
cc_newup	ds.l	1
cc_newsp	ds.l	1
cc_oldlow	ds.l	1
cc_oldup	ds.l	1
cc_oldsp	ds.l	1
cc_restore_sp	ds.l	1
cc_exitstk	ds.l	32
cc_exitstk_end	ds.l	0

	end
