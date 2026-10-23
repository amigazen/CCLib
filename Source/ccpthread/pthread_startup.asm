* SPDX-License-Identifier: BSD-2-Clause
* Copyright (c) 2026 amigazen project
*
*
* pthread_startup.asm - CreateNewProc NP_Entry (unix.lib2 pattern).
*
* Assemble WITHOUT -u so C __stdargs names are literal:
*   _cc_pthread_startup / _cc_thread_entry / _cc_geta4
*
* A4 from _LinkerDB (same as unix.lib2 child.asm).  cc_start.asm uses
* bare LinkerDB only because it is assembled with asm -u.
*
* After ThreadEntry returns, rts so dos reaps the process.  Do not call
* dos Exit — CreateNewProc workers have no CLI (unix.lib2 same).
*

	xdef	_cc_pthread_startup
	xdef	_cc_geta4
	xref	_LinkerDB
	xref	_cc_thread_entry

	csect	text

_cc_pthread_startup:
	lea	_LinkerDB,a4
	jsr	_cc_thread_entry
	rts

*
* Reload A4 after LVOs that pass an argument in A4 (e.g. SetupSTDIO).
* nostartup builds have no sc.lib geta4.
*
_cc_geta4:
	lea	_LinkerDB,a4
	rts

	end
