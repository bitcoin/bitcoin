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
 * __os_open --
 *	Open a file descriptor (including page size and log size information).
 *
 * PUBLIC: int __os_open __P((ENV *,
 * PUBLIC:     const char *, u_int32_t, u_int32_t, int, DB_FH **));
 */
int
__os_open(env, name, page_size, flags, mode, fhpp)
	ENV *env;
	const char *name;
	u_int32_t page_size, flags;
	int mode;
	DB_FH **fhpp;
{
	DB_ENV *dbenv;
	DB_FH *fhp;
	int oflags, ret;

	COMPQUIET(page_size, 0);

	dbenv = env == NULL ? NULL : env->dbenv;
	*fhpp = NULL;
	oflags = 0;

	if (dbenv != NULL &&
	    FLD_ISSET(dbenv->verbose, DB_VERB_FILEOPS | DB_VERB_FILEOPS_ALL))
		__db_msg(env, "fileops: open %s", name);

#define	OKFLAGS								\
	(DB_OSO_ABSMODE | DB_OSO_CREATE | DB_OSO_DIRECT | DB_OSO_DSYNC |\
	DB_OSO_EXCL | DB_OSO_RDONLY | DB_OSO_REGION | DB_OSO_SEQ |	\
	DB_OSO_TEMP | DB_OSO_TRUNC)
	if ((ret = __db_fchk(env, "__os_open", flags, OKFLAGS)) != 0)
		return (ret);

#if defined(O_BINARY)
	/*
	 * If there's a binary-mode open flag, set it, we never want any
	 * kind of translation.  Some systems do translations by default,
	 * e.g., with Cygwin, the default mode for an open() is set by the
	 * mode of the mount that underlies the file.
	 */
	oflags |= O_BINARY;
#endif

	/*
	 * DB requires the POSIX 1003.1 semantic that two files opened at the
	 * same time with DB_OSO_CREATE/O_CREAT and DB_OSO_EXCL/O_EXCL flags
	 * set return an EEXIST failure in at least one.
	 */
	if (LF_ISSET(DB_OSO_CREATE))
		oflags |= O_CREAT;

	if (LF_ISSET(DB_OSO_EXCL))
		oflags |= O_EXCL;

#ifdef HAVE_O_DIRECT
	if (LF_ISSET(DB_OSO_DIRECT))
		oflags |= O_DIRECT;
#endif
#ifdef O_DSYNC
	if (LF_ISSET(DB_OSO_DSYNC))
		oflags |= O_DSYNC;
#endif

	if (LF_ISSET(DB_OSO_RDONLY))
		oflags |= O_RDONLY;
	else
		oflags |= O_RDWR;

	if (LF_ISSET(DB_OSO_TRUNC))
		oflags |= O_TRUNC;

	/*
	 * Undocumented feature: allow applications to create intermediate
	 * directories whenever a file is opened.
	 */
	if (dbenv != NULL &&
	    env->dir_mode != 0 && LF_ISSET(DB_OSO_CREATE) &&
	    (ret = __db_mkpath(env, name)) != 0)
		return (ret);

	/* Open the file. */
#ifdef HAVE_QNX
	if (LF_ISSET(DB_OSO_REGION))
		ret = __os_qnx_region_open(env, name, oflags, mode, &fhp);
	else
#endif
	ret = __os_openhandle(env, name, oflags, mode, &fhp);
	if (ret != 0)
		return (ret);

	if (LF_ISSET(DB_OSO_REGION))
		F_SET(fhp, DB_FH_REGION);
#ifdef HAVE_FCHMOD
	/*
	 * If the code using Berkeley DB is a library, that code may not be able
	 * to control the application's umask value.  Allow applications to set
	 * absolute file modes.  We can't fix the race between file creation and
	 * the fchmod call -- we can't modify the process' umask here since the
	 * process may be multi-threaded and the umask value is per-process, not
	 * per-thread.
	 */
	if (LF_ISSET(DB_OSO_CREATE) && LF_ISSET(DB_OSO_ABSMODE))
		(void)fchmod(fhp->fd, mode);
#endif

#ifdef O_DSYNC
	/*
	 * If we can configure the file descriptor to flush on write, the
	 * file descriptor does not need to be explicitly sync'd.
	 */
	if (LF_ISSET(DB_OSO_DSYNC))
		F_SET(fhp, DB_FH_NOSYNC);
#endif

#if defined(HAVE_DIRECTIO) && defined(DIRECTIO_ON)
	/*
	 * The Solaris C library includes directio, but you have to set special
	 * compile flags to #define DIRECTIO_ON.  Require both in order to call
	 * directio.
	 */
	if (LF_ISSET(DB_OSO_DIRECT))
		(void)directio(fhp->fd, DIRECTIO_ON);
#endif

	/*
	 * Delete any temporary file.
	 *
	 * !!!
	 * There's a race here, where we've created a file and we crash before
	 * we can unlink it.  Temporary files aren't common in DB, regardless,
	 * it's not a security problem because the file is empty.  There's no
	 * reasonable way to avoid the race (playing signal games isn't worth
	 * the portability nightmare), so we just live with it.
	 */
	if (LF_ISSET(DB_OSO_TEMP)) {
#if defined(HAVE_UNLINK_WITH_OPEN_FAILURE) || defined(CONFIG_TEST)
		F_SET(fhp, DB_FH_UNLINK);
#else
		(void)__os_unlink(env, name, 0);
#endif
	}

	*fhpp = fhp;
	return (0);
}
