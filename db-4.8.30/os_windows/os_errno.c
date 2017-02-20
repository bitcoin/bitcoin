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

#ifdef HAVE_REPLICATION_THREADS
/*
 * __os_get_neterr --
 *	Return the last networking error or EAGAIN if the last error is zero.
 *
 * PUBLIC: #ifdef HAVE_REPLICATION_THREADS
 * PUBLIC: int __os_get_neterr __P((void));
 * PUBLIC: #endif
 */
int
__os_get_neterr()
{
	int err;

	/* This routine must be able to return the same value repeatedly. */
	err = WSAGetLastError();
	if (err == 0)
		WSASetLastError(err = ERROR_RETRY);
	return (err);
}
#endif

/*
 * __os_get_syserr --
 *	Return the last system error or EAGAIN if the last error is zero.
 */
int
__os_get_syserr()
{
	int err;

	/* This routine must be able to return the same value repeatedly. */
	err = GetLastError();
	if (err == 0)
		SetLastError(err = ERROR_RETRY);
	return (err);
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
#ifdef DB_WINCE
#define	MAX_TMPBUF_LEN 512
	_TCHAR tbuf[MAX_TMPBUF_LEN];
	size_t  maxlen;

	DB_ASSERT(NULL, error != 0);

	memset(tbuf, 0, sizeof(_TCHAR)*MAX_TMPBUF_LEN);
	maxlen = (len > MAX_TMPBUF_LEN ? MAX_TMPBUF_LEN : len);
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, (DWORD)error,
		0, tbuf, maxlen-1, NULL);

	if (WideCharToMultiByte(CP_UTF8, 0, tbuf, -1,
		buf, len, 0, NULL) == 0)
		strncpy(buf, "Error message translation failed.", len);
#else
	DB_ASSERT(NULL, error != 0);
	/*
	 * Explicitly call FormatMessageA, since we want to receive a char
	 * string back, not a tchar string.
	 */
	FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM,
	    0, (DWORD)error, 0, buf, (DWORD)(len - 1), NULL);
	buf[len - 1] = '\0';
#endif

	return (buf);
}

/*
 * __os_posix_err --
 *	Convert a system error to a POSIX error.
 */
int
__os_posix_err(error)
	int error;
{
	/* Handle calls on successful returns. */
	if (error == 0)
		return (0);

	/*
	 * Translate the Windows error codes we care about.
	 */
	switch (error) {
	case ERROR_INVALID_PARAMETER:
		return (EINVAL);

	case ERROR_FILE_NOT_FOUND:
	case ERROR_INVALID_DRIVE:
	case ERROR_PATH_NOT_FOUND:
		return (ENOENT);

	case ERROR_NO_MORE_FILES:
	case ERROR_TOO_MANY_OPEN_FILES:
		return (EMFILE);

	case ERROR_ACCESS_DENIED:
		return (EPERM);

	case ERROR_INVALID_HANDLE:
		return (EBADF);

	case ERROR_NOT_ENOUGH_MEMORY:
		return (ENOMEM);

	case ERROR_DISK_FULL:
		return (ENOSPC);

	case ERROR_ARENA_TRASHED:
	case ERROR_BAD_COMMAND:
	case ERROR_BAD_ENVIRONMENT:
	case ERROR_BAD_FORMAT:
	case ERROR_GEN_FAILURE:
	case ERROR_INVALID_ACCESS:
	case ERROR_INVALID_BLOCK:
	case ERROR_INVALID_DATA:
	case ERROR_READ_FAULT:
	case ERROR_WRITE_FAULT:
		return (EFAULT);

	case ERROR_ALREADY_EXISTS:
	case ERROR_FILE_EXISTS:
		return (EEXIST);

	case ERROR_NOT_SAME_DEVICE:
		return (EXDEV);

	case ERROR_WRITE_PROTECT:
		return (EACCES);

	case ERROR_LOCK_FAILED:
	case ERROR_LOCK_VIOLATION:
	case ERROR_NOT_READY:
	case ERROR_SHARING_VIOLATION:
		return (EBUSY);

	case ERROR_RETRY:
		return (EINTR);
	}

	/*
	 * Translate the Windows socket error codes.
	 */
	switch (error) {
	case WSAEADDRINUSE:
#ifdef EADDRINUSE
		return (EADDRINUSE);
#else
		break;
#endif
	case WSAEADDRNOTAVAIL:
#ifdef EADDRNOTAVAIL
		return (EADDRNOTAVAIL);
#else
		break;
#endif
	case WSAEAFNOSUPPORT:
#ifdef EAFNOSUPPORT
		return (EAFNOSUPPORT);
#else
		break;
#endif
	case WSAEALREADY:
#ifdef EALREADY
		return (EALREADY);
#else
		break;
#endif
	case WSAEBADF:
		return (EBADF);
	case WSAECONNABORTED:
#ifdef ECONNABORTED
		return (ECONNABORTED);
#else
		break;
#endif
	case WSAECONNREFUSED:
#ifdef ECONNREFUSED
		return (ECONNREFUSED);
#else
		break;
#endif
	case WSAECONNRESET:
#ifdef ECONNRESET
		return (ECONNRESET);
#else
		break;
#endif
	case WSAEDESTADDRREQ:
#ifdef EDESTADDRREQ
		return (EDESTADDRREQ);
#else
		break;
#endif
	case WSAEFAULT:
		return (EFAULT);
	case WSAEHOSTDOWN:
#ifdef EHOSTDOWN
		return (EHOSTDOWN);
#else
		break;
#endif
	case WSAEHOSTUNREACH:
#ifdef EHOSTUNREACH
		return (EHOSTUNREACH);
#else
		break;
#endif
	case WSAEINPROGRESS:
#ifdef EINPROGRESS
		return (EINPROGRESS);
#else
		break;
#endif
	case WSAEINTR:
		return (EINTR);
	case WSAEINVAL:
		return (EINVAL);
	case WSAEISCONN:
#ifdef EISCONN
		return (EISCONN);
#else
		break;
#endif
	case WSAELOOP:
#ifdef ELOOP
		return (ELOOP);
#else
		break;
#endif
	case WSAEMFILE:
		return (EMFILE);
	case WSAEMSGSIZE:
#ifdef EMSGSIZE
		return (EMSGSIZE);
#else
		break;
#endif
	case WSAENAMETOOLONG:
		return (ENAMETOOLONG);
	case WSAENETDOWN:
#ifdef ENETDOWN
		return (ENETDOWN);
#else
		break;
#endif
	case WSAENETRESET:
#ifdef ENETRESET
		return (ENETRESET);
#else
		break;
#endif
	case WSAENETUNREACH:
#ifdef ENETUNREACH
		return (ENETUNREACH);
#else
		break;
#endif
	case WSAENOBUFS:
#ifdef ENOBUFS
		return (ENOBUFS);
#else
		break;
#endif
	case WSAENOPROTOOPT:
#ifdef ENOPROTOOPT
		return (ENOPROTOOPT);
#else
		break;
#endif
	case WSAENOTCONN:
#ifdef ENOTCONN
		return (ENOTCONN);
#else
		break;
#endif
	case WSANOTINITIALISED:
		return (EAGAIN);
	case WSAENOTSOCK:
#ifdef ENOTSOCK
		return (ENOTSOCK);
#else
		break;
#endif
	case WSAEOPNOTSUPP:
		return (DB_OPNOTSUP);
	case WSAEPFNOSUPPORT:
#ifdef EPFNOSUPPORT
		return (EPFNOSUPPORT);
#else
		break;
#endif
	case WSAEPROTONOSUPPORT:
#ifdef EPROTONOSUPPORT
		return (EPROTONOSUPPORT);
#else
		break;
#endif
	case WSAEPROTOTYPE:
#ifdef EPROTOTYPE
		return (EPROTOTYPE);
#else
		break;
#endif
	case WSAESHUTDOWN:
#ifdef ESHUTDOWN
		return (ESHUTDOWN);
#else
		break;
#endif
	case WSAESOCKTNOSUPPORT:
#ifdef ESOCKTNOSUPPORT
		return (ESOCKTNOSUPPORT);
#else
		break;
#endif
	case WSAETIMEDOUT:
#ifdef ETIMEDOUT
		return (ETIMEDOUT);
#else
		break;
#endif
	case WSAETOOMANYREFS:
#ifdef ETOOMANYREFS
		return (ETOOMANYREFS);
#else
		break;
#endif
	case WSAEWOULDBLOCK:
#ifdef EWOULDBLOCK
		return (EWOULDBLOCK);
#else
		return (EAGAIN);
#endif
	case WSAHOST_NOT_FOUND:
#ifdef EHOSTUNREACH
		return (EHOSTUNREACH);
#else
		break;
#endif
	case WSASYSNOTREADY:
		return (EAGAIN);
	case WSATRY_AGAIN:
		return (EAGAIN);
	case WSAVERNOTSUPPORTED:
		return (DB_OPNOTSUP);
	case WSAEACCES:
		return (EACCES);
	}

	/*
	 * EFAULT is the default if we don't have a translation.
	 */
	return (EFAULT);
}
