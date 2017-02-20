/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1997-2009 Oracle.  All rights reserved.
 */
/*
 * Copyright (c) 1988, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $Id$
 */

/*
 * Copyright (c) 1982, 1985, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * __FBSDID("FreeBSD: /repoman/r/ncvs/src/lib/libc/gen/errlst.c,v 1.8 2005/04/02 12:33:28 das Exp $");
 */

#include "db_config.h"

#include "db_int.h"

/*
 * __db_strerror --
 *	Return the string associated with an errno.
 *
 * PUBLIC: #ifndef HAVE_STRERROR
 * PUBLIC: char *strerror __P((int));
 * PUBLIC: #endif
 */
char *
strerror(num)
	int num;
{
#define	ERRSTR(v, s) do {						\
	if (num == (v))							\
		return (s);						\
} while (0)
	ERRSTR(0, "Undefined error: 0");
	ERRSTR(EPERM, "Operation not permitted");
	ERRSTR(ENOENT, "No such file or directory");
	ERRSTR(ESRCH, "No such process");
	ERRSTR(EINTR, "Interrupted system call");
	ERRSTR(EIO, "Input/output error");
	ERRSTR(ENXIO, "Device not configured");
	ERRSTR(E2BIG, "Argument list too long");
	ERRSTR(ENOEXEC, "Exec format error");
	ERRSTR(EBADF, "Bad file descriptor");
	ERRSTR(ECHILD, "No child processes");
	ERRSTR(EDEADLK, "Resource deadlock avoided");
	ERRSTR(ENOMEM, "Cannot allocate memory");
	ERRSTR(EACCES, "Permission denied");
	ERRSTR(EFAULT, "Bad address");
	ERRSTR(ENOTBLK, "Block device required");
	ERRSTR(EBUSY, "Device busy");
	ERRSTR(EEXIST, "File exists");
	ERRSTR(EXDEV, "Cross-device link");
	ERRSTR(ENODEV, "Operation not supported by device");
	ERRSTR(ENOTDIR, "Not a directory");
	ERRSTR(EISDIR, "Is a directory");
	ERRSTR(EINVAL, "Invalid argument");
	ERRSTR(ENFILE, "Too many open files in system");
	ERRSTR(EMFILE, "Too many open files");
	ERRSTR(ENOTTY, "Inappropriate ioctl for device");
	ERRSTR(ETXTBSY, "Text file busy");
	ERRSTR(EFBIG, "File too large");
	ERRSTR(ENOSPC, "No space left on device");
	ERRSTR(ESPIPE, "Illegal seek");
	ERRSTR(EROFS, "Read-only file system");
	ERRSTR(EMLINK, "Too many links");
	ERRSTR(EPIPE, "Broken pipe");

/* math software */
	ERRSTR(EDOM, "Numerical argument out of domain");
	ERRSTR(ERANGE, "Result too large");

/* non-blocking and interrupt i/o */
	ERRSTR(EAGAIN, "Resource temporarily unavailable");
	ERRSTR(EWOULDBLOCK, "Resource temporarily unavailable");
	ERRSTR(EINPROGRESS, "Operation now in progress");
	ERRSTR(EALREADY, "Operation already in progress");

/* ipc/network software -- argument errors */
	ERRSTR(ENOTSOCK, "Socket operation on non-socket");
	ERRSTR(EDESTADDRREQ, "Destination address required");
	ERRSTR(EMSGSIZE, "Message too long");
	ERRSTR(EPROTOTYPE, "Protocol wrong type for socket");
	ERRSTR(ENOPROTOOPT, "Protocol not available");
	ERRSTR(EPROTONOSUPPORT, "Protocol not supported");
	ERRSTR(ESOCKTNOSUPPORT, "Socket type not supported");
	ERRSTR(EOPNOTSUPP, "Operation not supported");
	ERRSTR(EPFNOSUPPORT, "Protocol family not supported");
	ERRSTR(EAFNOSUPPORT, "Address family not supported by protocol family");
	ERRSTR(EADDRINUSE, "Address already in use");
	ERRSTR(EADDRNOTAVAIL, "Can't assign requested address");

/* ipc/network software -- operational errors */
	ERRSTR(ENETDOWN, "Network is down");
	ERRSTR(ENETUNREACH, "Network is unreachable");
	ERRSTR(ENETRESET, "Network dropped connection on reset");
	ERRSTR(ECONNABORTED, "Software caused connection abort");
	ERRSTR(ECONNRESET, "Connection reset by peer");
	ERRSTR(ENOBUFS, "No buffer space available");
	ERRSTR(EISCONN, "Socket is already connected");
	ERRSTR(ENOTCONN, "Socket is not connected");
	ERRSTR(ESHUTDOWN, "Can't send after socket shutdown");
	ERRSTR(ETOOMANYREFS, "Too many references: can't splice");
	ERRSTR(ETIMEDOUT, "Operation timed out");
	ERRSTR(ECONNREFUSED, "Connection refused");

	ERRSTR(ELOOP, "Too many levels of symbolic links");
	ERRSTR(ENAMETOOLONG, "File name too long");

/* should be rearranged */
	ERRSTR(EHOSTDOWN, "Host is down");
	ERRSTR(EHOSTUNREACH, "No route to host");
	ERRSTR(ENOTEMPTY, "Directory not empty");

/* quotas & mush */
	ERRSTR(EPROCLIM, "Too many processes");
	ERRSTR(EUSERS, "Too many users");
	ERRSTR(EDQUOT, "Disc quota exceeded");

/* Network File System */
	ERRSTR(ESTALE, "Stale NFS file handle");
	ERRSTR(EREMOTE, "Too many levels of remote in path");
	ERRSTR(EBADRPC, "RPC struct is bad");
	ERRSTR(ERPCMISMATCH, "RPC version wrong");
	ERRSTR(EPROGUNAVAIL, "RPC prog. not avail");
	ERRSTR(EPROGMISMATCH, "Program version wrong");
	ERRSTR(EPROCUNAVAIL, "Bad procedure for program");

	ERRSTR(ENOLCK, "No locks available");
	ERRSTR(ENOSYS, "Function not implemented");
	ERRSTR(EFTYPE, "Inappropriate file type or format");
#ifdef EAUTH
	ERRSTR(EAUTH, "Authentication error");
#endif
#ifdef ENEEDAUTH
	ERRSTR(ENEEDAUTH, "Need authenticator");
#endif
	ERRSTR(EIDRM, "Identifier removed");
	ERRSTR(ENOMSG, "No message of desired type");
#ifdef EOVERFLOW
	ERRSTR(EOVERFLOW, "Value too large to be stored in data type");
#endif
	ERRSTR(ECANCELED, "Operation canceled");
	ERRSTR(EILSEQ, "Illegal byte sequence");
#ifdef ENOATTR
	ERRSTR(ENOATTR, "Attribute not found");
#endif

/* General */
#ifdef EDOOFUS
	ERRSTR(EDOOFUS, "Programming error");
#endif

#ifdef EBADMSG
	ERRSTR(EBADMSG, "Bad message");
#endif
#ifdef EMULTIHOP
	ERRSTR(EMULTIHOP, "Multihop attempted");
#endif
#ifdef ENOLINK
	ERRSTR(ENOLINK, "Link has been severed");
#endif
#ifdef EPROTO
	ERRSTR(EPROTO, "Protocol error");
#endif

	return (__db_unknown_error(num));
}
