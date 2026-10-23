/* SPDX-License-Identifier: BSD-2-Clause
 * Copyright (c) 2026 amigazen project
 */

/*
 * Default Workbench console spec (SAS/C __stdiowin / __stdiov37).
 *
 * Own object so a program can replace it: link your stdiowin.o instead of
 * this one.  __stdiowin[] must be an array, not a char* (SAS UG Ch.9).
 * An empty __stdiowin means no default console.
 * On dos V37+ the CRT appends __stdiov37 (AUTO/CLOSE/WAIT).
 * A WINDOW= tooltype still wins over this default.
 */
char __stdiowin[] = "CON:10/10/320/80/";
char __stdiov37[] = "/AUTO/CLOSE/WAIT";
