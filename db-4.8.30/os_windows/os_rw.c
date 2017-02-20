/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1997-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"

/*
 * __os_io --
 *	Do an I/O.
 */
int
__os_io(env, op, fhp, pgno, pgsize, relative, io_len, buf, niop)
	ENV *env;
	int op;
	DB_FH *fhp;
	db_pgno_t pgno;
	u_int32_t pgsize, relative, io_len;
	u_int8_t *buf;
	size_t *niop;
{
	int ret;

#ifndef DB_WINCE
	if (__os_is_winnt()) {
		DB_ENV *dbenv;
		DWORD nbytes;
		OVERLAPPED over;
		ULONG64 off;
		dbenv = env == NULL ? NULL : env->dbenv;
		if ((off = relative) == 0)
			off = (ULONG64)pgsize * pgno;
		over.Offset = (DWORD)(off & 0xffffffff);
		over.OffsetHigh = (DWORD)(off >> 32);
		over.hEvent = 0; /* we don't want asynchronous notifications */

		if (dbenv != NULL &&
		    FLD_ISSET(dbenv->verbose, DB_VERB_FILEOPS_ALL))
			__db_msg(env,
			    "fileops: %s %s: %lu bytes at offset %lu",
			    op == DB_IO_READ ? "read" : "write",
			    fhp->name, (u_long)io_len, (u_long)off);

		LAST_PANIC_CHECK_BEFORE_IO(env);

		switch (op) {
		case DB_IO_READ:
#if defined(HAVE_STATISTICS)
			++fhp->read_count;
#endif
			if (!ReadFile(fhp->handle,
			    buf, (DWORD)io_len, &nbytes, &over))
				goto slow;
			break;
		case DB_IO_WRITE:
#ifdef HAVE_FILESYSTEM_NOTZERO
			if (__os_fs_notzero())
				goto slow;
#endif
#if defined(HAVE_STATISTICS)
			++fhp->write_count;
#endif
			if (!WriteFile(fhp->handle,
			    buf, (DWORD)io_len, &nbytes, &over))
				goto slow;
			break;
		}
		if (nbytes == io_len) {
			*niop = (size_t)nbytes;
			return (0);
		}
	}

slow:
#endif
	MUTEX_LOCK(env, fhp->mtx_fh);

	if ((ret = __os_seek(env, fhp, pgno, pgsize, relative)) != 0)
		goto err;

	switch (op) {
	case DB_IO_READ:
		ret = __os_read(env, fhp, buf, io_len, niop);
		break;
	case DB_IO_WRITE:
		ret = __os_write(env, fhp, buf, io_len, niop);
		break;
	}

err:	MUTEX_UNLOCK(env, fhp->mtx_fh);

	return (ret);
}

/*
 * __os_read --
 *	Read from a file handle.
 */
int
__os_read(env, fhp, addr, len, nrp)
	ENV *env;
	DB_FH *fhp;
	void *addr;
	size_t len;
	size_t *nrp;
{
	DB_ENV *dbenv;
	DWORD count;
	size_t offset, nr;
	u_int8_t *taddr;
	int ret;

	dbenv = env == NULL ? NULL : env->dbenv;
	ret = 0;

#if defined(HAVE_STATISTICS)
	++fhp->read_count;
#endif
	if (dbenv != NULL && FLD_ISSET(dbenv->verbose, DB_VERB_FILEOPS_ALL))
		__db_msg(env,
		    "fileops: read %s: %lu bytes", fhp->name, (u_long)len);

	for (taddr = addr,
	    offset = 0; offset < len; taddr += nr, offset += nr) {
		LAST_PANIC_CHECK_BEFORE_IO(env);
		RETRY_CHK((!ReadFile(fhp->handle,
		    taddr, (DWORD)(len - offset), &count, NULL)), ret);
		if (count == 0 || ret != 0)
			break;
		nr = (size_t)count;
	}
	*nrp = taddr - (u_int8_t *)addr;
	if (ret != 0) {
		__db_syserr(env, ret, "read: 0x%lx, %lu",
		    P_TO_ULONG(taddr), (u_long)len - offset);
		ret = __os_posix_err(ret);
	}
	return (ret);
}

/*
 * __os_write --
 *	Write to a file handle.
 */
int
__os_write(env, fhp, addr, len, nwp)
	ENV *env;
	DB_FH *fhp;
	void *addr;
	size_t len;
	size_t *nwp;
{
	int ret;

#ifdef HAVE_FILESYSTEM_NOTZERO
	/* Zero-fill as necessary. */
	if (__os_fs_notzero() &&
	    (ret = __db_zero_fill(env, fhp)) != 0)
		return (ret);
#endif
	return (__os_physwrite(env, fhp, addr, len, nwp));
}

/*
 * __os_physwrite --
 *	Physical write to a file handle.
 */
int
__os_physwrite(env, fhp, addr, len, nwp)
	ENV *env;
	DB_FH *fhp;
	void *addr;
	size_t len;
	size_t *nwp;
{
	DB_ENV *dbenv;
	DWORD count;
	size_t offset, nw;
	u_int8_t *taddr;
	int ret;

	dbenv = env == NULL ? NULL : env->dbenv;
	ret = 0;

#if defined(HAVE_STATISTICS)
	++fhp->write_count;
#endif
	if (dbenv != NULL && FLD_ISSET(dbenv->verbose, DB_VERB_FILEOPS_ALL))
		__db_msg(env,
		    "fileops: write %s: %lu bytes", fhp->name, (u_long)len);

	for (taddr = addr,
	    offset = 0; offset < len; taddr += nw, offset += nw) {
		LAST_PANIC_CHECK_BEFORE_IO(env);
		RETRY_CHK((!WriteFile(fhp->handle,
		    taddr, (DWORD)(len - offset), &count, NULL)), ret);
		if (ret != 0)
			break;
		nw = (size_t)count;
	}
	*nwp = len;
	if (ret != 0) {
		__db_syserr(env, ret, "write: %#lx, %lu",
		    P_TO_ULONG(taddr), (u_long)len - offset);
		ret = __os_posix_err(ret);

		DB_EVENT(env, DB_EVENT_WRITE_FAILED, NULL);
	}
	return (ret);
}
