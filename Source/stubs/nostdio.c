/*
 * Set non-zero to skip the default Workbench CON: (SAS nostdio / __tinymain).
 * Own object so a program can replace it without replacing __stdiowin.
 * WINDOW= tooltypes are still honored.
 */
short __nostdio = 0;
