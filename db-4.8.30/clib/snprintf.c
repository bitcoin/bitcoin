/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"

#if !defined(HAVE_SNPRINTF) || !defined(HAVE_VSNPRINTF)
static void sprintf_overflow __P((void));
static int  sprintf_retcharpnt __P((void));
#endif

/*
 * snprintf --
 *	Bounded version of sprintf.
 *
 * PUBLIC: #ifndef HAVE_SNPRINTF
 * PUBLIC: int snprintf __P((char *, size_t, const char *, ...));
 * PUBLIC: #endif
 */
#ifndef HAVE_SNPRINTF
int
#ifdef STDC_HEADERS
snprintf(char *str, size_t n, const char *fmt, ...)
#else
snprintf(str, n, fmt, va_alist)
	char *str;
	size_t n;
	const char *fmt;
	va_dcl
#endif
{
	static int ret_charpnt = -1;
	va_list ap;
	size_t len;

	if (ret_charpnt == -1)
		ret_charpnt = sprintf_retcharpnt();

#ifdef STDC_HEADERS
	va_start(ap, fmt);
#else
	va_start(ap);
#endif
	len = (size_t)vsprintf(str, fmt, ap);
	if (ret_charpnt)
		len = strlen(str);

	va_end(ap);

	if (len >= n) {
		sprintf_overflow();
		/* NOTREACHED */
	}
	return ((int)len);
}
#endif

/*
 * vsnprintf --
 *	Bounded version of vsprintf.
 *
 * PUBLIC: #ifndef HAVE_VSNPRINTF
 * PUBLIC: int vsnprintf __P((char *, size_t, const char *, va_list));
 * PUBLIC: #endif
 */
#ifndef HAVE_VSNPRINTF
int
vsnprintf(str, n, fmt, ap)
	char *str;
	size_t n;
	const char *fmt;
	va_list ap;
{
	static int ret_charpnt = -1;
	size_t len;

	if (ret_charpnt == -1)
		ret_charpnt = sprintf_retcharpnt();

	len = (size_t)vsprintf(str, fmt, ap);
	if (ret_charpnt)
		len = strlen(str);

	if (len >= n) {
		sprintf_overflow();
		/* NOTREACHED */
	}
	return ((int)len);
}
#endif

#if !defined(HAVE_SNPRINTF) || !defined(HAVE_VSNPRINTF)
static void
sprintf_overflow()
{
	/*
	 * !!!
	 * We're potentially manipulating strings handed us by the application,
	 * and on systems without a real snprintf() the sprintf() calls could
	 * have overflowed the buffer.  We can't do anything about it now, but
	 * we don't want to return control to the application, we might have
	 * overwritten the stack with a Trojan horse.  We're not trying to do
	 * anything recoverable here because systems without snprintf support
	 * are pretty rare anymore.
	 */
#define	OVERFLOW_ERROR	"internal buffer overflow, process ended\n"
#ifndef	STDERR_FILENO
#define	STDERR_FILENO	2
#endif
	(void)write(STDERR_FILENO, OVERFLOW_ERROR, sizeof(OVERFLOW_ERROR) - 1);

	/* Be polite. */
	exit(1);

	/* But firm. */
	__os_abort(NULL);

	/* NOTREACHED */
}

static int
sprintf_retcharpnt()
{
	int ret_charpnt;
	char buf[10];

	/*
	 * Some old versions of sprintf return a pointer to the first argument
	 * instead of a character count.  Assume the return value of snprintf,
	 * vsprintf, etc. will be the same as sprintf, and check the easy one.
	 *
	 * We do this test at run-time because it's not a test we can do in a
	 * cross-compilation environment.
	 */

	ret_charpnt =
	    (int)sprintf(buf, "123") != 3 ||
	    (int)sprintf(buf, "123456789") != 9 ||
	    (int)sprintf(buf, "1234") != 4;

	return (ret_charpnt);
}
#endif
