/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2005-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"

/*
 * printf --
 *
 * PUBLIC: #ifndef HAVE_PRINTF
 * PUBLIC: int printf __P((const char *, ...));
 * PUBLIC: #endif
 */
#ifndef HAVE_PRINTF
int
#ifdef STDC_HEADERS
printf(const char *fmt, ...)
#else
printf(fmt, va_alist)
	const char *fmt;
	va_dcl
#endif
{
	va_list ap;
	size_t len;
	char buf[2048];    /* !!!: END OF THE STACK DON'T TRUST SPRINTF. */

#ifdef STDC_HEADERS
	va_start(ap, fmt);
#else
	va_start(ap);
#endif
	len = (size_t)vsnprintf(buf, sizeof(buf), fmt, ap);
#ifdef HAVE_BREW
	/*
	 * The BREW vsnprintf function return count includes the trailing
	 * nul-termination character.
	 */
	if (len > 0 && len <= sizeof(buf) && buf[len - 1] == '\0')
		--len;
#endif

	va_end(ap);

	/*
	 * We implement printf/fprintf with fwrite, because Berkeley DB uses
	 * fwrite in other places.
	 */
	return (fwrite(
	    buf, sizeof(char), (size_t)len, stdout) == len ? (int)len: -1);
}
#endif /* HAVE_PRINTF */

/*
 * fprintf --
 *
 * PUBLIC: #ifndef HAVE_PRINTF
 * PUBLIC: int fprintf __P((FILE *, const char *, ...));
 * PUBLIC: #endif
 */
#ifndef HAVE_PRINTF
int
#ifdef STDC_HEADERS
fprintf(FILE *fp, const char *fmt, ...)
#else
fprintf(fp, fmt, va_alist)
	FILE *fp;
	const char *fmt;
	va_dcl
#endif
{
	va_list ap;
	size_t len;
	char buf[2048];    /* !!!: END OF THE STACK DON'T TRUST SPRINTF. */

#ifdef STDC_HEADERS
	va_start(ap, fmt);
#else
	va_start(ap);
#endif
	len = vsnprintf(buf, sizeof(buf), fmt, ap);
#ifdef HAVE_BREW
	/*
	 * The BREW vsnprintf function return count includes the trailing
	 * nul-termination character.
	 */
	if (len > 0 && len <= sizeof(buf) && buf[len - 1] == '\0')
		--len;
#endif

	va_end(ap);

	/*
	 * We implement printf/fprintf with fwrite, because Berkeley DB uses
	 * fwrite in other places.
	 */
	return (fwrite(
	    buf, sizeof(char), (size_t)len, fp) == len ? (int)len: -1);
}
#endif /* HAVE_PRINTF */

/*
 * vfprintf --
 *
 * PUBLIC: #ifndef HAVE_PRINTF
 * PUBLIC: int vfprintf __P((FILE *, const char *, va_list));
 * PUBLIC: #endif
 */
#ifndef HAVE_PRINTF
int
vfprintf(fp, fmt, ap)
	FILE *fp;
	const char *fmt;
	va_list ap;
{
	size_t len;
	char buf[2048];    /* !!!: END OF THE STACK DON'T TRUST SPRINTF. */

	len = vsnprintf(buf, sizeof(buf), fmt, ap);
#ifdef HAVE_BREW
	/*
	 * The BREW vsnprintf function return count includes the trailing
	 * nul-termination character.
	 */
	if (len > 0 && len <= sizeof(buf) && buf[len - 1] == '\0')
		--len;
#endif

	/*
	 * We implement printf/fprintf with fwrite, because Berkeley DB uses
	 * fwrite in other places.
	 */
	return (fwrite(
	    buf, sizeof(char), (size_t)len, fp) == len ? (int)len: -1);
}
#endif /* HAVE_PRINTF */
