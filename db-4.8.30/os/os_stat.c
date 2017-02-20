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
 * __os_exists --
 *	Return if the file exists.
 *
 * PUBLIC: int __os_exists __P((ENV *, const char *, int *));
 */
int
__os_exists(env, path, isdirp)
	ENV *env;
	const char *path;
	int *isdirp;
{
	DB_ENV *dbenv;
	struct stat sb;
	int ret;

	dbenv = env == NULL ? NULL : env->dbenv;

	if (dbenv != NULL &&
	    FLD_ISSET(dbenv->verbose, DB_VERB_FILEOPS | DB_VERB_FILEOPS_ALL))
		__db_msg(env, "fileops: stat %s", path);

	if (DB_GLOBAL(j_exists) != NULL)
		return (DB_GLOBAL(j_exists)(path, isdirp));

	RETRY_CHK((stat(CHAR_STAR_CAST path, &sb)), ret);
	if (ret != 0)
		return (__os_posix_err(ret));

#if !defined(S_ISDIR) || defined(STAT_MACROS_BROKEN)
#undef	S_ISDIR
#ifdef _S_IFDIR
#define	S_ISDIR(m)	(_S_IFDIR & (m))
#else
#define	S_ISDIR(m)	(((m) & 0170000) == 0040000)
#endif
#endif
	if (isdirp != NULL)
		*isdirp = S_ISDIR(sb.st_mode);

	return (0);
}

/*
 * __os_ioinfo --
 *	Return file size and I/O size; abstracted to make it easier
 *	to replace.
 *
 * PUBLIC: int __os_ioinfo __P((ENV *, const char *,
 * PUBLIC:    DB_FH *, u_int32_t *, u_int32_t *, u_int32_t *));
 */
int
__os_ioinfo(env, path, fhp, mbytesp, bytesp, iosizep)
	ENV *env;
	const char *path;
	DB_FH *fhp;
	u_int32_t *mbytesp, *bytesp, *iosizep;
{
	struct stat sb;
	int ret;

	if (DB_GLOBAL(j_ioinfo) != NULL)
		return (DB_GLOBAL(j_ioinfo)(path,
		    fhp->fd, mbytesp, bytesp, iosizep));

	DB_ASSERT(env, F_ISSET(fhp, DB_FH_OPENED) && fhp->fd != -1);

	RETRY_CHK((fstat(fhp->fd, &sb)), ret);
	if (ret != 0) {
		__db_syserr(env, ret, "fstat");
		return (__os_posix_err(ret));
	}

	/* Return the size of the file. */
	if (mbytesp != NULL)
		*mbytesp = (u_int32_t)(sb.st_size / MEGABYTE);
	if (bytesp != NULL)
		*bytesp = (u_int32_t)(sb.st_size % MEGABYTE);

	/*
	 * Return the underlying filesystem I/O size, if available.
	 *
	 * XXX
	 * Check for a 0 size -- the HP MPE/iX architecture has st_blksize,
	 * but it's always 0.
	 */
#ifdef HAVE_STRUCT_STAT_ST_BLKSIZE
	if (iosizep != NULL && (*iosizep = sb.st_blksize) == 0)
		*iosizep = DB_DEF_IOSIZE;
#else
	if (iosizep != NULL)
		*iosizep = DB_DEF_IOSIZE;
#endif
	return (0);
}
