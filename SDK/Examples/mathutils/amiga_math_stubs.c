/* SPDX-License-Identifier: BSD-2-Clause
 * Copyright (c) 2026 amigazen project
 */

/*
 * amiga_math_stubs.c - satisfy ma.lib refs to unfinished Amiga backends
 *
 * Several fdlibm wrappers in ma.lib call m99_has_trans() / amiga_* when
 * those objects were built with __AMIGA__. The backend .o files are not
 * in the shipped archive. Returning 0 from m99_has_* forces the fdlibm path.
 */

int
m99_has_ffp(void)
{
	return 0;
}

int
m99_has_ieee(void)
{
	return 0;
}

int
m99_has_trans(void)
{
	return 0;
}

/* Referenced from hypot.o / cbrt.o / expm1.o / acosh.o — never reached. */
double
amiga_ieee_sqrt(double x)
{
	return x;
}

double
amiga_ieee_pow(double x, double y)
{
	(void)y;
	return x;
}

double
amiga_expm1(double x)
{
	return x;
}

double
amiga_acosh(double x)
{
	return x;
}
