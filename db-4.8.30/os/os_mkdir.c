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
 * __os_mkdir --
 *	Create a directory.
 *
 * PUBLIC: int __os_mkdir __P((ENV *, const char *, int));
 */
int
__os_mkdir(env, name, mode)
	ENV *env;
	const char *name;
	int mode;
{
	DB_ENV *dbenv;
	int ret;

	dbenv = env == NULL ? NULL : env->dbenv;
	if (dbenv != NULL &&
	    FLD_ISSET(dbenv->verbose, DB_VERB_FILEOPS | DB_VERB_FILEOPS_ALL))
		__db_msg(env, "fileops: mkdir %s", name);

	/* Make the directory, with paranoid permissions. */
#if defined(HAVE_VXWORKS)
	RETRY_CHK((mkdir(CHAR_STAR_CAST name)), ret);
#else
	RETRY_CHK((mkdir(name, DB_MODE_700)), ret);
#endif
	if (ret != 0)
		return (__os_posix_err(ret));

	/* Set the absolute permissions, if specified. */
#if !defined(HAVE_VXWORKS)
	if (mode != 0) {
		RETRY_CHK((chmod(name, mode)), ret);
		if (ret != 0)
			ret = __os_posix_err(ret);
	}
#endif
	return (ret);
}
