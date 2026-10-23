# CCLib

CCLib is the Common C Library for Amiga, an implementation of the Standard C Library, or 'libc', in the form 
of a shared library that additionally provides many of the commonly used C functions from C99, POSIX, BSD, 
SysV and glibc, all implemented with advanced safety and reliability features designed to minimise 
shortcomings and improve reliability of the Amiga's runtime environment.

CCLib includes a C runtime startup module that works with most compilers, the core cclib.library, and 
additional, statically linked, POSIX helper functions including an implementation of libpthread built natively 
upon Amiga's Exec kernel.

## [amigazen project](http://www.amigazen.com)

*A web, suddenly*

*Forty years meditation*

*Minds awaken, free*

**amigazen project** is using modern software development tools and methods to update and rerelease classic
Amiga open source software. Projects include a new AWeb, a new Amiga Python 2, and the ToolKit project — a
universal SDK for Amiga development. All *amigazen project* releases are guaranteed to build against the
ToolKit standard so that anyone can download and begin contributing straightaway without having to tailor
the toolchain for their own setup.

This **4.0** release of CCLib is a reimplementation of the historic `cclib.library` ABI, from amigazen 
project. The original developer of cclib.library, Robert Albrecht, is not associated with amigazen project. It 
is distributed under the BSD 2-Clause License (see [LICENSE.md](LICENSE.md)).

The amigazen project philosophy is based on openness:

*Open* to anyone and everyone — *Open* source and free for all — *Open* your mind and create!

PRs for all projects are gratefully received at [GitHub](https://github.com/amigazen/). While the focus now
is on classic 68k software, it is intended that all amigazen project releases can be ported to other
Amiga-like systems including AROS and MorphOS where feasible.

## CCLib Features

* **Modern Standard C Library functions:** cclib.library implements the Standard C library function families
stdio, string, stdlib, time, ctype as well as many of the most common C99, BSD, POSIX and Linux functions
* **CRT Startup code:** CCLib provides its own runtime startup module, that is required to make full use of
advanced features like forged handle panics, safe aborts, and resource tracking
* **Compiler support:** SAS/C, VBCC and GCC are all support, as well as the new ToolChain AC compiler from 
amigazen project, for which it is the recommended 'libc' and C runtime library
* **Resource tracking:** Heap blocks, file descriptors, `FILE*` streams, `DIR*` cookies, child processes, 
locale state and scratch buffers allocated through C library functions are tracked on a per-task session and 
automatically freed on normal and abnormal exits. `malloc` uses an exec memory pool created at `SetupSTDIO`.
* **Capability-checked handles:** fds, `FILE*`, `DIR*` and `malloc` pointers are resolved against the session 
list before any call to the underlying Amiga native libraries. `NULL` and other sentinels set `errno` without 
aborting while forged or stale handles will panic. Closed objects are poisoned so use after free errors can be 
trapped
* **Safe 'panic' handling:** If a library function is called with a forged or stale `FILE*`, fd, `DIR*` or 
`malloc` pointer, or tries to double-free, the runtime will 'panic' rather than crash: it prints a stacktrace, 
then aborts safely and cleans up resources
* **Amiga native POSIX threads implementation:** A separate static link library `ccpthread.lib` implements a 
substantial subset of the libpthread API, natively on top of Amiga's Exec kernel Task and Semaphore objects
* **Fully Amiga native:** Encapsulates the Amiga's own exec, dos, locale and utility library functions where 
they provide libc-like services while adding reliability features and cross-platform standards compliance
* **Locale and wchar support:** `setlocale`, `localeconv`, `strcoll`, `strxfrm` and ctype use locale.library
* **Self-debugging executables:** Where `HUNK_DEBUG` (LINE) or `HUNK_SYMBOL` information is present in the 
executable, the CRT will prints a stacktrace to the console on panic, self-identifying the line where the 
panic occurred without the need for Enforcer, SegTracker and FindHit
* **SAS/C compatibility:** the most useful paradigms from SAS/C's runtime including Workbench startup and 
console open, `_STI`/`_STD` init and exit callbacks, `atexit` (also run on `_exit`, `abort`, panic and 
Ctrl-C), `__chkabort`, and `__stack` growth via `StackSwap` (without the known defects of the original)
* **libm compatibility:** CCLib is designed to be linked with `ma.lib`also from amigazen project
* **Backwards compatible to original cclib.library:** Clean-room reimplementation of the original Aminet 
`cclib.library` jump table, including Lattice/BSD leftovers (`bcopy`, `index`, `agetc`/`aputc`, list helpers) 
so software written against the V3 ABI can link
* **Free and Open Source:** CCLib is fully open source, released under a BSD 2-Clause License

## What is a 'libc'?

A libc (C standard library) is a core software library that provides the standard functions used by programs 
written in C, such as memory allocation (malloc), string manipulation (strlen), input/output (printf), and 
process management. It acts as an interface between applications and the operating system, often wrapping 
native system calls to provide a consistent programming interface for portable programs written in C.

Common libc implementations include:
- glibc (GNU C Library): The most widely used implementation on GNU/Linux systems, particularly Debian and 
Ubuntu.
- musl libc: A lightweight, standards-compliant implementation popular in Alpine Linux and statically linked 
applications.
- Newlib: Another libc implementation targeting embedded systems
- Bionic: Android's C library, developed by Google for the Android operating system.
- Apple Libsystem: Apple's system library, providing C library functionality on macOS and other Apple 
platforms.
- Microsoft UCRT (Universal C Runtime): Microsoft's C runtime implementation for Windows.

## Why is a new 'libc' for Amiga needed?

Historically, every C compiler on Amiga came with it's own implementation of the standard C library - 
* SAS/C has sc.lib, containing a rich set of functions from both K&R C and later ANSI C89/C90 standard, and 
including many, but not all, of the functions proposed to be part of the C99 standard. Crucially, sc.lib did 
not offer memory safe string handling functions like 'snprintf' which is why so many open source projects for 
Amiga include their own
* GCC has both ixemul.library a shared library (that contained a Unix-like kernel implementation) and libnix, 
a more traditional static link library
* clib2, by stalwart Amiga developer Olaf Barthel, provided a newer, C99 libc implementation for use with GCC 
and SAS/C
* VBCC has vc.lib, and a richer PosixLib version is also available for better compatibility when porting from 
other platforms
* Older products like Lattice (which later became SAS/C) and Aztec also provided their own standard libraries, 
typically implementing defacto pre-ANSI or draft ANSI standard C

Amiga OS4 additionally has versions:
* clib2, now expanded and modernized to become clib4 
* Newlib, an Amiga port of the reference embedded C library

None of these libc implementations do much more than provide the raw C functions — this is a missed 
opportunity. Classic Amiga lacks many safety and reliability features that modern operating systems implement 
in the kernel, so on Amiga they have to live in the application which means they can also be provided in the C runtime, to maximise reusability. 

Taking the same idea as Redox OS's relibc — putting reliability in the runtime when the OS does not — CCLib 
adds handle validation, panic-on-forge, and resource tracking to the C library itself, to reduce the risk of a 
fatal crash - albeit not all gaps can be filled by the runtime, most especially memory protection in Amiga's 
single shared address space.

## Is CCLib a POSIX compliant 'libc'?

CCLib is a hosted Standard C library for AmigaOS that also implements many of the most _common_ POSIX, BSD
and Linux functions needed by portable C programs, but it is not fully 'POSIX compliant' - indeed, few C 
libraries are.

What CCLib offers is the usual standard C surface — stdio, string, stdlib, time, ctype, locale — plus
selected POSIX pieces such as `dirent`, `getenv`/`setenv`, `sleep`/`usleep`/`nanosleep`, and a
substantial `pthread` implementation in `ccpthread.lib`. 

Like other Amiga C libraries, there is no `fork`/`execve`/`waitpid`, no full `sigaction` and no sockets - they 
belong in bsdsocket.library 

## Isn't dos.library the Amiga's 'libc'?

No. dos.library is AmigaDOS: files, locks, directories, the CLI, Workbench startup messages, and
environment variables. Those are operating-system services, closer to Unix syscalls than to a C
library. A libc is the language runtime that sits on top — `malloc`, `FILE*`, `printf`, `strlen`,
`qsort`, `exit` — so the same C source can run on more than one OS.

AmigaOS never shipped that layer as a system library. Each compiler brought its own (`sc.lib`,
`vc.lib`, libnix and others, as described earlier). dos.library has a few C-shaped helpers (`FPrintf``FGets`), 
and newer versions of utility.library have `SNPrintf`, but none of that is exactly `<stdio.h>` or 
`<stdlib.h>`. Amiga native functions deliberately have different capitalisatins to allow both to coexist. 
There is no portable heap, no `FILE*` buffering, no `errno` contract, and no cleanup if the program aborts.

CCLib is that missing layer. It calls dos, exec, locale and utility with handle validation, and
it tracks the resources those calls create. You still use dos.library for Amiga-specific work;
you use CCLib when you want the C API, and when you want the session's C objects to be cleaned up on exit, 
even under error conditions. Unlike other libc implementations on Amiga, CCLib makes maximum use of the system 
libraries own versions of the Standard C Library functions, like StrToLong(), SNPrintf(), Strncat() and FPutC()
encapsulating them with just enough code to add the reliability features and compatibility enhancements needed 
to support the common libc implementation expected by portable software.


## Does CCLib include a 'libm' math library as well?

CCLib is designed to be used with ma.lib, a C99 libm implementation also from amigazen project. This is 
entirely optional - Amiga native software may wish to use the Amiga math libraries, especially the defacto 
standard IEEE libraries, directly. 

## What is ccpthread.lib?

libpthread is a library that provides POSIX threads (pthreads), a standard API for creating and managing 
multiple threads within a single process.

Threads allow a program to perform multiple tasks concurrently while sharing the same process memory and 
resources.

What does libpthread provide?
- Thread creation and management: pthread_create(), pthread_join(), pthread_exit()
- Synchronization: mutexes (pthread_mutex_t), condition variables (pthread_cond_t), and read-write locks
- Thread-specific data: storage associated with individual threads
- Thread attributes: configuring stack sizes, scheduling parameters, and detached state

The CCLib implementation of libpthread uses Exec native objects including Tasks and Semaphores to implement 
the pthreads API, including thread cancellation.

## Is a shared library implementation more or less efficient than a statically linked 'libc'?

This is an excellent question because a statically linked library will on the one hand only link the code for 
functions that are actually used, so the whole library, which can be 200KB+ in size, does not need to be 
linked into the executable file. However every application linking the library, will carry its own copy of 
every function it uses, and a system running many programs will therefore have many copies of the most 
commonly used functions loaded into RAM at any one time. 

A shared library on Amiga cannot be partially loaded in the way a Linux shared object can be loaded virtually 
in small chunks, the whole library must be loaded if any one function is needed. However, at around only 60KB, 
the cclib.library becomes more efficient as soon as you have a few programs all using it at the same time. The 
assumption is also made that RAM and storage are cheap in 2026 even on classic Amiga platforms, and so the 
next question asks _what are the benefits received for giving up a little extra memory and performance?_

## Do the reliability/safety features impact performance?

Yes, capability checks and the per-task resource lists add CPU time and a little RAM to some of
the most frequently used C functions. That is a deliberate choice: CCLib favours safety and
reliability over raw speed and the smallest possible footprint. Where other operating systems
provide process isolation, memory protection and automatic cleanup in the kernel, classic Amiga
does not, so CCLib tries to fill part of that gap in the runtime — not unlike how early OpenStep /
Cocoa programs relied on the Objective-C runtime to track objects the OS would not reclaim.

The assumption is made that in 2026, where most Amigas will be accelerated and equipped with ample RAM, 
that impact is a fair trade off against more reliable, harder to crash programs, while the existing libc options remain available for when performance and efficiency on lower end hardware remains the requirement.

## What is a 'panic'?

A panic is a controlled abort on error. When CCLib detects an error that would otherwise crash the machine or 
corrupt AmigaOS — such as a forged or stale `FILE*`, file descriptor or `DIR*`, a double-free, or a pointer 
that does not point at readable memory — it refuses to pass that address to dos.library or exec. Instead it 
prints a message, dumps a stacktrace if the executable has HUNK debug information, then runs the same cleanup 
path as a normal `exit` i.e `atexit` handlers, followed by `ClearSTDIO`, which frees every resource the 
session still owns.

A panic is not a Guru Meditation and not an Enforcer hit. Those fire after the damage is already done. A panic 
is the library deciding the program is no longer safe to continue, and leaving the system in a known state.

Ordinary failures — `fopen` on a missing file, `free(NULL)`, a `NULL` `FILE*` - are not panics. They set 
`errno` and return, as ISO C requires.

Programs that do not use the CCLib CRT should install their own abort hook with `SetAbortFunc` so a panic 
can still clean up the session's resources.

## What is the CCLib's connection to the original cclib.library found on Aminet?

When amigazen project started looking into the possibility of providing a new libc for Amiga, the original
[cclib.library](https://aminet.net/package/dev/c/CCLib) by Robert W. Albrecht (1989–1990) stood out as a
well constructed example of how an Amiga shared library should implement the standard C library.

The Aminet (originally in fact a Fred Fish release) ships the `CClib.library` binary, headers, autodocs, and 
compiler glue, plus startup code for Aztec and Lattice `_main` replacements, connection-routine stubs, and an 
early GCC `crt0` from Paul Gittings — but not the source to the library function bodies themselves.

CCLib reimplements the original cclib.library ABI from first principles then adds common functions from later 
libc implementations as well as POSIX/BSD/Linux helpers, and safety and reliability features never before seen 
on Amiga.

## Contact

- At GitHub https://github.com/amigazen/CCLib/
- On the web at http://www.amigazen.com/ (Amiga browser compatible)
- Or email toolkit@amigazen.com

## Acknowledgements

The historic `cclib.library` ABI was designed by **Robert W. Albrecht** (1989–1990), with contributions
noted in that release from **Paul Gittings** (GCC startup) and **Kevin Kenny** (Lattice glue). This
**4.0** ToolKit release is a clean-room reimplementation by **amigazen project** (2026). See
[LICENSE.md](LICENSE.md) for copyright and redistribution terms.

*Amiga* is a trademark of **Amiga Corporation**.