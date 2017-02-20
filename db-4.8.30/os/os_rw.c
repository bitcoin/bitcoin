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
 *
 * PUBLIC: int __os_io __P((ENV *, int, DB_FH *, db_pgno_t,
 * PUBLIC:     u_int32_t, u_int32_t, u_int32_t, u_int8_t *, size_t *));
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
#if defined(HAVE_PREAD) && defined(HAVE_PWRITE)
	DB_ENV *dbenv;
	off_t offset;
	ssize_t nio;
#endif
	int ret;

	/*
	 * Check for illegal usage.
	 *
	 * This routine is used in one of two ways: reading bytes from an
	 * absolute offset and reading a specific database page.  All of
	 * our absolute offsets are known to fit into a u_int32_t, while
	 * our database pages might be at offsets larger than a u_int32_t.
	 * We don't want to specify an absolute offset in our caller as we
	 * aren't exactly sure what size an off_t might be.
	 */
	DB_ASSERT(env, F_ISSET(fhp, DB_FH_OPENED) && fhp->fd != -1);
	DB_ASSERT(env, (pgno == 0 && pgsize == 0) || relative == 0);

#if defined(HAVE_PREAD) && defined(HAVE_PWRITE)
	dbenv = env == NULL ? NULL : env->dbenv;

	if ((offset = relative) == 0)
		offset = (off_t)pgno * pgsize;
	switch (op) {
	case DB_IO_READ:
		if (DB_GLOBAL(j_read) != NULL)
			goto slow;
#if defined(HAVE_STATISTICS)
		++fhp->read_count;
#endif
		if (dbenv != NULL &&
		    FLD_ISSET(dbenv->verbose, DB_VERB_FILEOPS_ALL))
			__db_msg(env,
			    "fileops: read %s: %lu bytes at offset %lu",
			    fhp->name, (u_long)io_len, (u_long)offset);

		LAST_PANIC_CHECK_BEFORE_IO(env);
		nio = DB_GLOBAL(j_pread) != NULL ?
		    DB_GLOBAL(j_pread)(fhp->fd, buf, io_len, offset) :
		    pread(fhp->fd, buf, io_len, offset);
		break;
	case DB_IO_WRITE:
		if (DB_GLOBAL(j_write) != NULL)
			goto slow;
#ifdef HAVE_FILESYSTEM_NOTZERO
		if (__os_fs_notzero())
			goto slow;
#endif
#if defined(HAVE_STATISTICS)
		++fhp->write_count;
#endif
		if (dbenv != NULL &&
		    FLD_ISSET(dbenv->verbose, DB_VERB_FILEOPS_ALL))
			__db_msg(env,
			    "fileops: write %s: %lu bytes at offset %lu",
			    fhp->name, (u_long)io_len, (u_long)offset);

		LAST_PANIC_CHECK_BEFORE_IO(env);
		nio = DB_GLOBAL(j_pwrite) != NULL ?
		    DB_GLOBAL(j_pwrite)(fhp->fd, buf, io_len, offset) :
		    pwrite(fhp->fd, buf, io_len, offset);
		break;
	default:
		return (EINVAL);
	}
	if (nio == (ssize_t)io_len) {
		*niop = io_len;
		return (0);
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
	default:
		ret = EINVAL;
		break;
	}

err:	MUTEX_UNLOCK(env, fhp->mtx_fh);

	return (ret);

}

/*
 * __os_read --
 *	Read from a file handle.
 *
 * PUBLIC: int __os_read __P((ENV *, DB_FH *, void *, size_t, size_t *));
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
	size_t offset;
	ssize_t nr;
	int ret;
	u_int8_t *taddr;

	dbenv = env == NULL ? NULL : env->dbenv;
	ret = 0;

	DB_ASSERT(env, F_ISSET(fhp, DB_FH_OPENED) && fhp->fd != -1);

#if defined(HAVE_STATISTICS)
	++fhp->read_count;
#endif
	if (dbenv != NULL && FLD_ISSET(dbenv->verbose, DB_VERB_FILEOPS_ALL))
		__db_msg(env,
		    "fileops: read %s: %lu bytes", fhp->name, (u_long)len);

	if (DB_GLOBAL(j_read) != NULL) {
		*nrp = len;
		LAST_PANIC_CHECK_BEFORE_IO(env);
		if (DB_GLOBAL(j_read)(fhp->fd, addr, len) != (ssize_t)len) {
			ret = __os_get_syserr();
			__db_syserr(env, ret, "read: %#lx, %lu",
			    P_TO_ULONG(addr), (u_long)len);
			ret = __os_posix_err(ret);
		}
		return (ret);
	}

	for (taddr = addr, offset = 0;
	    offset < len; taddr += nr, offset += (u_int32_t)nr) {
		LAST_PANIC_CHECK_BEFORE_IO(env);
		RETRY_CHK(((nr = read(fhp->fd,
		    CHAR_STAR_CAST taddr, len - offset)) < 0 ? 1 : 0), ret);
		if (nr == 0 || ret != 0)
			break;
	}
	*nrp = (size_t)(taddr - (u_int8_t *)addr);
	if (ret != 0) {
		__db_syserr(env, ret, "read: %#lx, %lu",
		    P_TO_ULONG(taddr), (u_long)len - offset);
		ret = __os_posix_err(ret);
	}
	return (ret);
}

/*
 * __os_write --
 *	Write to a file handle.
 *
 * PUBLIC: int __os_write __P((ENV *, DB_FH *, void *, size_t, size_t *));
 */
int
__os_write(env, fhp, addr, len, nwp)
	ENV *env;
	DB_FH *fhp;
	void *addr;
	size_t len;
	size_t *nwp;
{
	DB_ASSERT(env, F_ISSET(fhp, DB_FH_OPENED) && fhp->fd != -1);

#ifdef HAVE_FILESYSTEM_NOTZERO
	/* Zero-fill as necessary. */
	if (__os_fs_notzero()) {
		int ret;
		if ((ret = __db_zero_fill(env, fhp)) != 0)
			return (ret);
	}
#endif
	return (__os_physwrite(env, fhp, addr, len, nwp));
}

/*
 * __os_physwrite --
 *	Physical write to a file handle.
 *
 * PUBLIC: int __os_physwrite
 * PUBLIC:     __P((ENV *, DB_FH *, void *, size_t, size_t *));
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
	size_t offset;
	ssize_t nw;
	int ret;
	u_int8_t *taddr;

	dbenv = env == NULL ? NULL : env->dbenv;
	ret = 0;

#if defined(HAVE_STATISTICS)
	++fhp->write_count;
#endif
	if (dbenv != NULL && FLD_ISSET(dbenv->verbose, DB_VERB_FILEOPS_ALL))
		__db_msg(env,
		    "fileops: write %s: %lu bytes", fhp->name, (u_long)len);

#if defined(HAVE_FILESYSTEM_NOTZERO) && defined(DIAGNOSTIC)
	if (__os_fs_notzero()) {
		struct stat sb;
		off_t cur_off;

		DB_ASSERT(env, fstat(fhp->fd, &sb) != -1 &&
		    (cur_off = lseek(fhp->fd, (off_t)0, SEEK_CUR)) != -1 &&
		    cur_off <= sb.st_size);
	}
#endif
	if (DB_GLOBAL(j_write) != NULL) {
		*nwp = len;
		LAST_PANIC_CHECK_BEFORE_IO(env);
		if (DB_GLOBAL(j_write)(fhp->fd, addr, len) != (ssize_t)len) {
			ret = __os_get_syserr();
			__db_syserr(env, ret, "write: %#lx, %lu",
			    P_TO_ULONG(addr), (u_long)len);
			ret = __os_posix_err(ret);

			DB_EVENT(env, DB_EVENT_WRITE_FAILED, NULL);
		}
		return (ret);
	}

	for (taddr = addr, offset = 0;
	    offset < len; taddr += nw, offset += (u_int32_t)nw) {
		LAST_PANIC_CHECK_BEFORE_IO(env);
		RETRY_CHK(((nw = write(fhp->fd,
		    CHAR_STAR_CAST taddr, len - offset)) < 0 ? 1 : 0), ret);
		if (ret != 0)
			break;
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
