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
 */
int
__os_get_errno()
{
	/* This routine must be able to return the same value repeatedly. */
	if (errno == 0)
		__os_set_errno(EAGAIN);
	return (errno);
}

/*
 * __os_get_syserr --
 *	Return the last system error or EAGAIN if the last error is zero.
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
 */
char *
__os_strerror(error, buf, len)
	int error;
	char *buf;
	size_t len;
{
	char *p;

	switch (error) {
	case EBADFILENAME:
		p = "EBADFILENAME";
		break;
	case EBADSEEKPOS:
		p = "EBADSEEKPOS";
		break;
#ifndef HAVE_BREW_SDK2
	case EDIRNOEXISTS:
		p = "EDIRNOEXISTS";
		break;
#endif
	case EDIRNOTEMPTY:
		p = "EDIRNOTEMPTY";
		break;
	case EFILEEOF:
		p = "EFILEEOF";
		break;
	case EFILEEXISTS:
		p = "EFILEEXISTS";
		break;
	case EFILENOEXISTS:
		p = "EFILENOEXISTS";
		break;
	case EFILEOPEN:
		p = "EFILEOPEN";
		break;
	case EFSFULL:
		p = "EFSFULL";
		break;
#ifndef HAVE_BREW_SDK2
	case EINVALIDOPERATION:
		p = "EINVALIDOPERATION";
		break;
	case ENOMEDIA:
		p = "ENOMEDIA";
		break;
#endif
	case ENOMEMORY:
		p = "ENOMEMORY";
		break;
	case EOUTOFNODES:
		p = "EOUTOFNODES";
		break;
	default:
		p = __db_unknown_error(error);
		break;
	}

	(void)strncpy(buf, p, len - 1);
	buf[len - 1] = '\0';

	return (buf);
}

/*
 * __os_posix_err
 *	Convert a system error to a POSIX error.
 */
int
__os_posix_err(error)
	int error;
{
	int ret;

	switch (error) {
	case EBADFILENAME:
#ifndef HAVE_BREW_SDK2
	case EDIRNOEXISTS:
#endif
	case EDIRNOTEMPTY:
	case EFILENOEXISTS:
		ret = ENOENT;
		break;

	case EOUTOFNODES:
		ret = EMFILE;
		break;

	case ENOMEMORY:
		ret = ENOMEM;
		break;

	case EFSFULL:
		ret = ENOSPC;
		break;

#ifndef HAVE_BREW_SDK2
	case EINVALIDOPERATION:
		ret = DB_OPNOTSUP;
		break;
#endif

	case EFILEEXISTS:
		ret = EEXIST;
		break;

	case EBADSEEKPOS:
	case EFILEEOF:
		ret = EIO;
		break;

	default:
		ret = EFAULT;
		break;
	}
	return (ret);
}
