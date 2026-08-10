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

AbsExecBase	EQU	4

	xdef	__startup
	xdef	__cc_ctors_addr
	xdef	__cc_dtors_addr

	xref	LinkerDB
	xref	_BSSBAS
	xref	_BSSLEN
	xref	__main
	xref	__ctors
	xref	__dtors

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
* sc_crt/c.a: jsr __fpinit then jsr __main with command line on stack.
* Pass NULL; __main uses GetArgStr for the Lattice args-only string.
*
	clr.l	-(a7)
	jsr	__main
	addq.l	#4,a7

	movem.l	(a7)+,d1-d6/a0-a6
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

	end
