*
* crt/cc_start.asm - NOSTARTUP entry (minimal sc_crt/c.a)
*
* Must be first on the SLINK FROM line.  Assemble from crt/ like sc_crt:
*   asm -u cc_start.asm
*
* Loads A4 from LinkerDB and clears BSS before any C runs — required for
* near data (unittest SCOPTIONS match sc_crt; not data=far).
*

AbsExecBase	EQU	4

	xdef	__startup

	xref	LinkerDB
	xref	_BSSBAS
	xref	_BSSLEN
	xref	__main

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

	end
