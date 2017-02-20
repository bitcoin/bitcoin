/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1999-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"

/*
 * __os_get_errno_ret_zero --
 *	Return the last system error, including an error of zero.
 *
 * PUBLIC: int __os_get_errno_ret_zero __P((void));
 */
int
__os_get_errno_ret_zero()
{
	/* This routine must be able to return the same value repeatedly. */
	return (errno);
}

/*
 * We've seen cases where system calls failed but errno was never set.  For
 * that reason, __os_get_errno() and __os_get_syserr set errno to EAGAIN if
 * it's not already set, to work around the problem.  For obvious reasons,
 * we can only call this function if we know an error has occurred, that
 * is, we can't test the return for a non-zero value after the get call.
 *
 * __os_get_errno --
 *	Return the last ANSI C "errno" value or EAGAIN if the last error
 *	is zero.
 *
 * PUBLIC: int __os_get_errno __P((void));
 */
int
__os_get_errno()
{
	/* This routine must be able to return the same value repeatedly. */
	return (__os_get_syserr());
}

#if 0
/*
 * __os_get_neterr --
 *      Return the last network-related error or EAGAIN if the last
 *	error is zero.
 *
 * PUBLIC: int __os_get_neterr __P((void));
 */
int
__os_get_neterr()
{
	/* This routine must be able to return the same value repeatedly. */
	return (__os_get_syserr());
}
#endif

/*
 * __os_get_syserr --
 *	Return the last system error or EAGAIN if the last error is zero.
 *
 * PUBLIC: int __os_get_syserr __P((void));
 */
int
__os_get_syserr()
{
	/* This routine must be able to return the same value repeatedly. */
	if (errno == 0)
		__os_set_errno(EAGAIN);
	return (errno);
}

/*
 * __os_set_errno --
 *	Set the value of errno.
 *
 * PUBLIC: void __os_set_errno __P((int));
 */
void
__os_set_errno(evalue)
	int evalue;
{
	/*
	 * This routine is called by the compatibility interfaces (DB 1.85,
	 * dbm and hsearch).  Force values > 0, that is, not one of DB 2.X
	 * and later's public error returns.  If something bad has happened,
	 * default to EFAULT -- a nasty return.  Otherwise, default to EINVAL.
	 * As the compatibility APIs aren't included on Windows, the Windows
	 * version of this routine doesn't need this behavior.
	 */
	errno =
	    evalue >= 0 ? evalue : (evalue == DB_RUNRECOVERY ? EFAULT : EINVAL);
}

/*
 * __os_strerror --
 *	Return a string associated with the system error.
 *
 * PUBLIC: char *__os_strerror __P((int, char *, size_t));
 */
char *
__os_strerror(error, buf, len)
	int error;
	char *buf;
	size_t len;
{
	/* No translation is needed in the POSIX layer. */
	(void)strncpy(buf, strerror(error), len - 1);
	buf[len - 1] = '\0';

	return (buf);
}

/*
 * __os_posix_err
 *	Convert a system error to a POSIX error.
 *
 * PUBLIC: int __os_posix_err __P((int));
 */
int
__os_posix_err(error)
	int error;
{
	return (error);
}
