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
 */
int
__os_read(env, fhp, addr, len, nrp)
	ENV *env;
	DB_FH *fhp;
	void *addr;
	size_t len;
	size_t *nrp;
{
	FileInfo pInfo;
	int ret;
	size_t offset, nr;
	char *taddr;

	ret = 0;

#if defined(HAVE_STATISTICS)
	++fhp->read_count;
#endif

	for (taddr = addr, offset = 0, nr = 0;
	    offset < len; taddr += nr, offset += (u_int32_t)nr) {
		LAST_PANIC_CHECK_BEFORE_IO(env);
		nr = (size_t)IFILE_Read(fhp->ifp, addr, len);
		/* an error occured, or we reached the end of the file */
		if (nr == 0)
			break;
	}
	if (nr == 0) {
		IFILE_GetInfo(fhp->ifp, &pInfo);
		if (pInfo.dwSize != 0) {/* not an empty file */
	/*
	 * If we have not reached the end of the file,
	 * we got an error in IFILE_Read
	 */
			if (IFILE_Seek(
			    fhp->ifp, _SEEK_CURRENT, 0) != pInfo.dwSize) {
				ret = __os_get_syserr();
				__db_syserr(env, ret, "IFILE_Read: %#lx, %lu",
				    P_TO_ULONG(addr), (u_long)len);
				ret = __os_posix_err(ret);
			}
		}
	}
	*nrp = (size_t)(taddr - (u_int8_t *)addr);
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
 */
int
__os_physwrite(env, fhp, addr, len, nwp)
	ENV *env;
	DB_FH *fhp;
	void *addr;
	size_t len;
	size_t *nwp;
{
	int ret;

	ret = 0;

#if defined(HAVE_STATISTICS)
	++fhp->write_count;
#endif

	LAST_PANIC_CHECK_BEFORE_IO(env);
	if ((*nwp = (size_t)IFILE_Write(fhp->ifp, addr, len)) != len) {
		ret = __os_get_syserr();
		__db_syserr(env, ret, "IFILE_Write: %#lx, %lu",
		    P_TO_ULONG(addr), (u_long)len);
		ret = __os_posix_err(ret);
	}
	return (ret);
}
